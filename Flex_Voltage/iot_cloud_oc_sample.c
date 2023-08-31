/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "ohos_init.h"

#include <dtls_al.h>
#include <mqtt_al.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>
#include "E53_SC2.h"

#include "iot_adc.h"
#include "iot_errno.h"
#include <math.h>
#include "iot_gpio_ex.h"


#include "wifi_connect.h"

#define CONFIG_WIFI_SSID "BearPi" // 修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD "123456789" // 修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP "121.36.42.100"

#define CONFIG_APP_SERVERPORT "1883"

#define CONFIG_APP_DEVICEID "611493160ad1ed0286498b06_121212" // 替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD "123456789" // 替换为注册设备后生成的密钥

#define CONFIG_APP_LIFETIME 60 // < seconds

#define CONFIG_QUEUE_TIMEOUT (5 * 1000)

#define MSGQUEUE_COUNT 16
#define MSGQUEUE_SIZE 10
#define CLOUD_TASK_STACK_SIZE (1024 * 10)
#define CLOUD_TASK_PRIO 24
#define SENSOR_TASK_STACK_SIZE (1024 * 4)
#define SENSOR_TASK_PRIO 25
#define TASK_DELAY 3
#define FLIP_THRESHOLD 100

#define ADC_CHANNEL_1 5
#define ADC_CHANNEL_2 4
#define ADC_GPIO 11

#define ADC_VREF_VOL 1.8
#define ADC_COEFFICIENT 4
#define ADC_RATIO 4096

typedef enum {
    en_msg_cmd = 0,
    en_msg_report,
} en_msg_type_t;

typedef struct {
    char *request_id;
    char *payload;
} cmd_t;

typedef struct {
    char band_1[10000];
    char band_2[10000];
} report_t;

typedef struct {
    en_msg_type_t msg_type;
    union {
        report_t report;
    } msg;
} app_msg_t;

typedef struct {
    osMessageQueueId_t app_msg;
    int connected;
} app_cb_t;
static app_cb_t g_app_cb;

int g_coverStatus = 0;

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t voltage_1;
    oc_mqtt_profile_kv_t voltage_2;

    if (g_app_cb.connected != 1) {
        return;
    }

    service.event_time = NULL;
    service.service_id = "Flex_Sener";
    service.service_property = &voltage_1;
    service.nxt = NULL;

    voltage_1.key = "Angle_1";
    voltage_1.value = &report->band_1;
    voltage_1.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    voltage_1.nxt = &voltage_2;

    voltage_2.key = "Angle_2";
    voltage_2.value = &report->band_2;
    voltage_2.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    voltage_2.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL, &service);
    return;
}

static int CloudMainTaskEntry(void)
{
    app_msg_t *app_msg;
    uint32_t ret;

    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();

    g_app_cb.app_msg = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (g_app_cb.app_msg == NULL) {
        printf("Create receive msg queue failed");
    }
    oc_mqtt_profile_connect_t connect_para;
    (void)memset_s(&connect_para, sizeof(connect_para), 0, sizeof(connect_para));

    connect_para.boostrap = 0;
    connect_para.device_id = CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr = CONFIG_APP_SERVERIP;
    connect_para.server_port = CONFIG_APP_SERVERPORT;
    connect_para.life_time = CONFIG_APP_LIFETIME;
    connect_para.rcvfunc = NULL;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if ((ret == (int)en_oc_mqtt_err_ok)) {
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    } else {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }

    while (1) {
        app_msg = NULL;
        (void)osMessageQueueGet(g_app_cb.app_msg, (void **)&app_msg, NULL, 0xFFFFFFFF);
        if (app_msg != NULL) {
            switch (app_msg->msg_type) {
                case en_msg_report:
                    deal_report_msg(&app_msg->msg.report);
                    break;
                default:
                    break;
            }
            free(app_msg);
        }
    }
    return 0;
}

float map(float val, float I_Min, float I_Max, float O_Min, float O_Max)
	{
		return(((val-I_Min)*((O_Max-O_Min)/(I_Max-I_Min)))+O_Min);
    }

static float GetVoltage1(void)
{
    unsigned int ret;
    unsigned short data;

    ret=IoTAdcRead(ADC_CHANNEL_1,&data,IOT_ADC_EQU_MODEL_8,IOT_ADC_CUR_BAIS_DEFAULT,0xff);
    if (ret != IOT_SUCCESS){
        printf("Failed to read adc!\r\n");
    }
    return (float)data * ADC_VREF_VOL * ADC_COEFFICIENT / ADC_RATIO;
}

static float GetVoltage2(void)
{
    unsigned int ret;
    unsigned short data;

    ret=IoTAdcRead(ADC_CHANNEL_2,&data,IOT_ADC_EQU_MODEL_8,IOT_ADC_CUR_BAIS_DEFAULT,0xff);
    if (ret != IOT_SUCCESS){
        printf("Failed to read adc!\r\n");
    }
    return (float)data * ADC_VREF_VOL * ADC_COEFFICIENT / ADC_RATIO;
}

static int SensorTaskEntry(void)
{
    app_msg_t *app_msg;

    while (1) {
        float Voltage_1,Voltage_2;
        Voltage_1 = GetVoltage1();
        if (Voltage_1 > 1010)
        {
            Voltage_1 = 1010;
        }
        else if (Voltage_2 <450)
        {
            Voltage_1 = 450;
        }
        printf("vlt:%.3f\n",Voltage_1);
        float flex_angle_1 = map(Voltage_1,1010,450,0,90);
        printf("Angle_1:%.2f\n",flex_angle_1);
        usleep(250000);

        Voltage_2 = GetVoltage2();
        if (Voltage_2 > 1010)
        {
            Voltage_2 = 1010;
        }
        else if (Voltage_2 <450)
        {
            Voltage_2 = 450;
        }
        printf("vlt:%.3f\n",Voltage_2);
        float flex_angle_2 = map(Voltage_2,1010,450,0,90);
        printf("Angle_2:%.2f\n",flex_angle_2);
        usleep(250000);


        app_msg = malloc(sizeof(app_msg_t));
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;
            //app_msg->msg.report.vol_1 = (int)flex_angle_1;
            sprintf(app_msg->msg.report.band_1,"%.2f",flex_angle_1);
            //app_msg->msg.report.vol_2 = (int)flex_angle_2;
            sprintf(app_msg->msg.report.band_2,"%.2f",flex_angle_2);

            if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT) != 0) {
                free(app_msg);
            }
        }
        sleep(TASK_DELAY);
    }
    return 0;
}

static void IotMainTaskEntry(void)
{
    osThreadAttr_t attr;

    attr.name = "CloudMainTaskEntry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CLOUD_TASK_STACK_SIZE;
    attr.priority = CLOUD_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create CloudMainTaskEntry!\n");
    }
    attr.stack_size = SENSOR_TASK_STACK_SIZE;
    attr.priority = SENSOR_TASK_PRIO;
    attr.name = "SensorTaskEntry";
    if (osThreadNew((osThreadFunc_t)SensorTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create SensorTaskEntry!\n");
    }
}

APP_FEATURE_INIT(IotMainTaskEntry);