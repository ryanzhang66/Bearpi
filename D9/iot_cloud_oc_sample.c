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
#include "wifi_connect.h"
//ADC的头文件
#include "iot_adc.h"
#include "iot_errno.h"
#include <math.h>
#include "iot_gpio_ex.h"
// --------------------------------------------------------------------------------------------------------------
#define CONFIG_WIFI_SSID "yuizhu" // 修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD "zhangran20040906" // 修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP "117.78.5.125"

#define CONFIG_APP_SERVERPORT "1883"

#define CONFIG_APP_DEVICEID "64afecc9b84c1334befae1af_20201030" // 替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD "zhangran20040906" // 替换为注册设备后生成的密钥

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
// --------------------------------------------------------------------------------------------------------
#define ADC_CHANNEL_1 5
#define ADC_CHANNEL_2 4
#define ADC_GPIO 11

#define ADC_VREF_VOL 1.8
#define ADC_COEFFICIENT 4
#define ADC_RATIO 4096
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
osTimerId_t ID1,ID2;
osMessageQueueId_t mid_MsgQueue;
typedef enum {
    en_msg_cmd = 0,
    en_msg_report,
    en_msg_conn,
    en_msg_disconn,
} en_msg_type_t;

typedef struct {
    char *request_id;
    char *payload;
} cmd_t;

typedef struct {
    int temp;
    int acce_x;
    int acce_y;
    int acce_z;
    int gyro_x;
    int gyro_y;
    int gyro_z;
    char band_1[10000];
} report_t;

typedef struct {
    en_msg_type_t msg_type;
    union {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct {
    osMessageQueueId_t app_msg;
    int connected;
    int mpu6050;
    int mpu6050_state;

    int flag_timer_1;     //判断运动检测时间是否到达
} app_cb_t;

static app_cb_t g_app_cb;
int g_coverStatus = 1;
int state = 0;
osTimerId_t ID1,ID2;

/***** 定时器1 回调函数 *****/
void Timer1_Callback(void *arg)
{
    (void)arg;
    g_app_cb.flag_timer_1 = 1;
}

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t temperature;
    oc_mqtt_profile_kv_t Accel_x;
    oc_mqtt_profile_kv_t Accel_y;
    oc_mqtt_profile_kv_t Accel_z;
    oc_mqtt_profile_kv_t Gyro_x;
    oc_mqtt_profile_kv_t Gyro_y;
    oc_mqtt_profile_kv_t Gyro_z;
    oc_mqtt_profile_kv_t voltage_1;
    oc_mqtt_profile_kv_t mpu6050;

    if (g_app_cb.connected != 1) {
        return;
    }

    service.event_time = NULL;
    service.service_id = "Manhole_Cover";
    service.service_property = &temperature;
    service.nxt = NULL;

    temperature.key = "Temperature";
    temperature.value = &report->temp;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temperature.nxt = &Accel_x;

    Accel_x.key = "Accel_x";
    Accel_x.value = &report->acce_x;
    Accel_x.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Accel_x.nxt = &Accel_y;

    Accel_y.key = "Accel_y";
    Accel_y.value = &report->acce_y;
    Accel_y.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Accel_y.nxt = &Accel_z;

    Accel_z.key = "Accel_z";
    Accel_z.value = &report->acce_z;
    Accel_z.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Accel_z.nxt = &Gyro_x;

    Gyro_x.key = "Gyro_x";
    Gyro_x.value = &report->gyro_x;
    Gyro_x.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Gyro_x.nxt = &Gyro_y;

    Gyro_y.key = "Gyro_y";
    Gyro_y.value = &report->gyro_y;
    Gyro_y.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Gyro_y.nxt = &Gyro_z;

    Gyro_z.key = "Gyro_z";
    Gyro_z.value = &report->gyro_z;
    Gyro_z.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Gyro_z.nxt = &voltage_1;

    voltage_1.key = "Angle_1";
    voltage_1.value = &report->band_1;
    voltage_1.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    voltage_1.nxt = &mpu6050;

    mpu6050.key = "MPU6050Status";
    mpu6050.value = g_app_cb.mpu6050 ? "ON" : "OFF";
    mpu6050.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    mpu6050.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL, &service);
    return;
}

//use this function to push all the message to the buffer
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int    ret = 0;
    char  *buf;
    int    buf_len;
    app_msg_t *app_msg;

    if((NULL == msg) || (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS)){
        return ret;
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    if(NULL == buf){
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);

    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.request_id = buf;
    buf_len = strlen(msg->request_id);
    buf += buf_len + 1;
    memcpy(app_msg->msg.cmd.request_id, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';

    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy(app_msg->msg.cmd.payload, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT);
    if(ret != 0){
        free(app_msg);
    }

    return ret;
}

///< COMMAND DEAL
#include <cJSON.h>
static void deal_cmd_msg(cmd_t *cmd)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;
    cJSON *obj_paras;
    cJSON *obj_para;
    cJSON *obj_para_low, *obj_para_high, *obj_para_in_min, *obj_para_in_max, *obj_para_out_min, *obj_para_out_max;

    int cmdret = 1;
    oc_mqtt_profile_cmdresp_t cmdresp;
    obj_root = cJSON_Parse(cmd->payload);

    if (NULL == obj_root)
    {
        printf("EXIT_JSONPARSE!");
        //goto EXIT_JSONPARSE;
    }

    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (NULL == obj_cmdname)
    {
       //goto EXIT_CMDOBJ;
    }

    /* 
    * 根据不同的指令判断：进入对应的处理模块
    * 指令1：手指捏合 Kneading
    * 指令2：手指弯曲 Bend
    * 指令3：震颤 Tremor
    */

    // 指令1：手指捏合 Kneading
    if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Kneading"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            printf("EXIT_OBJPARAS!");
            //goto EXIT_OBJPARAS;
        }

        obj_para = cJSON_GetObjectItem(obj_paras, "kneading_control");
        if (NULL == obj_para)
        {
            printf("EXIT_OBJPARA!");
           // goto EXIT_OBJPARA;
        }
        ///< 打开压力传感器 here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            g_app_cb.mpu6050 = 1;
            // Light_StatusSet(ON);  // 此处可以添加LED灯，提示目前对应传感器正在操作中
            g_app_cb.mpu6050_state = 1;
            printf("压力传感器 On!");
        }
        ///< 关闭压力传感器
        else
        {
            g_app_cb.mpu6050 = 0;
            // Light_StatusSet(OFF);
            g_app_cb.mpu6050_state = 0;
            printf("压力传感器 Off!");
        }
        cmdret = 0;
    }
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
    connect_para.rcvfunc = msg_rcv_callback;
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
                case en_msg_cmd:
                    deal_cmd_msg(&app_msg->msg.cmd);
                    break;
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

/*
   ADC获取电压（弯曲传感器 1）
*/
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

/*
    map函数（用来映射）
*/
float map(float val, float I_Min, float I_Max, float O_Min, float O_Max)
	{
		return(((val-I_Min)*((O_Max-O_Min)/(I_Max-I_Min)))+O_Min);
    }

static int SensorTaskEntry(void)
{
    // 定时器
    osTimerId_t id1, id2, id3, id4;
    uint32_t timerDelay;
    osStatus_t status_1, status_2, status_3, status_4;
    uint32_t exec1, exec2, exec3, exec4;


    app_msg_t *app_msg;
    uint8_t ret;
    E53SC2Data data;
    int X = 0, Y = 0, Z = 0;

    ret = E53SC2Init();
    if (ret != 0) {
        printf("E53_SC2 Init failed!\r\n");
        return;
    }
    // 定时器1：用于手指捏合，目前设置时间为8秒触发
    exec1 = 8U;
    id1 = osTimerNew(Timer1_Callback, osTimerOnce, &exec1, NULL);


    while (1) {

        if (g_app_cb.mpu6050_state == 1)
        {
            timerDelay = 500U;
            status_1 = osTimerStart(id1, timerDelay); // 定时器开始
            while (1)
            {
                ret = E53SC2ReadData(&data);
                if (ret != 0) {
                    printf("E53_SC2 Read Data!\r\n");
                    return;
                }
                printf("\r\n**************Temperature      is  %d\r\n", (int)data.Temperature);
                printf("\r\n**************Accel[0]         is  %lf\r\n", data.Accel[ACCEL_X_AXIS]);
                printf("\r\n**************Accel[1]         is  %lf\r\n", data.Accel[ACCEL_Y_AXIS]);
                printf("\r\n**************Accel[2]         is  %lf\r\n", data.Accel[ACCEL_Z_AXIS]);
                printf("\r\n**************Gyro[0]         is  %lf\r\n", data.Gyro[ACCEL_X_AXIS]);
                printf("\r\n**************Gyro[1]         is  %lf\r\n", data.Gyro[ACCEL_Y_AXIS]);
                printf("\r\n**************Gyro[2]         is  %lf\r\n", data.Gyro[ACCEL_Z_AXIS]);
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

                if (X == 0 && Y == 0 && Z == 0) {
                    X = (int)data.Accel[ACCEL_X_AXIS];
                    Y = (int)data.Accel[ACCEL_Y_AXIS];
                    Z = (int)data.Accel[ACCEL_Z_AXIS];
                } else {
                    if (X + FLIP_THRESHOLD < data.Accel[ACCEL_X_AXIS] || X - FLIP_THRESHOLD > data.Accel[ACCEL_X_AXIS] ||
                        Y + FLIP_THRESHOLD < data.Accel[ACCEL_Y_AXIS] || Y - FLIP_THRESHOLD > data.Accel[ACCEL_Y_AXIS] ||
                        Z + FLIP_THRESHOLD < data.Accel[ACCEL_Z_AXIS] || Z - FLIP_THRESHOLD > data.Accel[ACCEL_Z_AXIS]) {
                        LedD1StatusSet(OFF);
                        LedD2StatusSet(ON);
                        g_coverStatus = 1;
                    } else {
                        LedD1StatusSet(ON);
                        LedD2StatusSet(OFF);
                        g_coverStatus = 0;
                    }
                }
                if (g_app_cb.flag_timer_1 == 1)
                {
                    app_msg = malloc(sizeof(app_msg_t));
                    if (app_msg != NULL) {
                        app_msg->msg_type = en_msg_report;
                        app_msg->msg.report.temp = (int)data.Temperature;
                        app_msg->msg.report.acce_x = (int)data.Accel[ACCEL_X_AXIS];
                        app_msg->msg.report.acce_y = (int)data.Accel[ACCEL_Y_AXIS];
                        app_msg->msg.report.acce_z = (int)data.Accel[ACCEL_Z_AXIS];
                        app_msg->msg.report.gyro_x = (int)data.Gyro[ACCEL_X_AXIS];
                        app_msg->msg.report.gyro_y = (int)data.Gyro[ACCEL_Y_AXIS];
                        app_msg->msg.report.gyro_z = (int)data.Gyro[ACCEL_Z_AXIS];
                        sprintf(app_msg->msg.report.band_1,"%.2f",flex_angle_1);
                        
                        if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT) != 0) {
                            free(app_msg);
                        }
                    }
                    g_app_cb.flag_timer_1 = 0;
                    g_app_cb.mpu6050_state = 0;
                    status_1 = osTimerStop(id1);
                    break;
                }
                

                if (g_app_cb.mpu6050_state == 0)
                {
                    status_1 = osTimerStop(id1);
                    break;
                }
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