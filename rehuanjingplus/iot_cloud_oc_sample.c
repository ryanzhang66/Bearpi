
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "wifi_connect.h"
#include "lwip/sockets.h"

#include "oc_mqtt.h"
#include "E53_IA1.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"

#define MSGQUEUE_OBJECTS 16 // number of Message Queue Objects

typedef struct
{ // object data type
    char *Buf;
    uint8_t Idx;
} MSGQUEUE_OBJ_t;

MSGQUEUE_OBJ_t msg;
osMessageQueueId_t mid_MsgQueue; // message queue id

#define CLIENT_ID "63ee2408e5d7266f68eba19b_1321332_0_0_2023042312"
#define USERNAME "63ee2408e5d7266f68eba19b_1321332"
#define PASSWORD "3b513b12d164c8196d230cb2657bae955a9ef34977cebbc57be7fe2c99963657"


int light_model=0;
int motor_model=0;
typedef enum
{
    en_msg_cmd = 0,
    en_msg_report,
} en_msg_type_t;

typedef struct
{
    char *request_id;
    char *payload;
} cmd_t;

typedef struct
{
    int lum;
    int temp;
    int hum;
} report_t;

typedef struct
{
    en_msg_type_t msg_type;
    union
    {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct
{
    int connected;
    int led;
    int motor;
} app_cb_t;
static app_cb_t g_app_cb;

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t temperature;
    oc_mqtt_profile_kv_t humidity;
    oc_mqtt_profile_kv_t luminance;
    oc_mqtt_profile_kv_t led;
    oc_mqtt_profile_kv_t motor;

    service.event_time = NULL;
    service.service_id = "Agriculture";
    service.service_property = &temperature;
    service.nxt = NULL;

    temperature.key = "Temperature";
    temperature.value = &report->temp;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temperature.nxt = &humidity;

    humidity.key = "Humidity";
    humidity.value = &report->hum;
    humidity.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    humidity.nxt = &luminance;

    luminance.key = "Luminance";
    luminance.value = &report->lum;
    luminance.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    luminance.nxt = &led;

    led.key = "LightStatus";
    led.value = g_app_cb.led ? "ON" : "OFF";
    led.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    led.nxt = &motor;

    motor.key = "MotorStatus";
    motor.value = g_app_cb.motor ? "ON" : "OFF";
    motor.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    motor.nxt = NULL;

    oc_mqtt_profile_propertyreport(USERNAME, &service);
    return;
}

void oc_cmd_rsp_cb(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
    app_msg_t *app_msg;

    int ret = 0;
    app_msg = malloc(sizeof(app_msg_t));
    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.payload = (char *)recv_data;

    printf("recv data is %.*s\n", recv_size, recv_data);
    ret = osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U);
    if (ret != 0)
    {
        free(recv_data);
    }
    *resp_data = NULL;
    *resp_size = 0;
}

///< COMMAND DEAL
#include <cJSON.h>
static void deal_cmd_msg(cmd_t *cmd)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;
    cJSON *obj_paras;
    cJSON *obj_para;
    app_msg_t *app_msg;
    E53_IA1_Data_TypeDef data;
    E53_IA1_Read_Data(&data);
    int cmdret = 1;
    oc_mqtt_profile_cmdresp_t cmdresp;
    obj_root = cJSON_Parse(cmd->payload);
    if (NULL == obj_root)
    {
        goto EXIT_JSONPARSE;
    }

    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (NULL == obj_cmdname)
    {
        goto EXIT_CMDOBJ;
    }
    if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_light"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "Light");
        if (NULL == obj_para)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the LED here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            light_model=1;
            g_app_cb.led = 1;
            Light_StatusSet(ON);
            printf("Light On!");
            printf("1");
            // app_msg->msg.report.hum =1;
            // app_msg->msg.report.lum =1;
            // app_msg->msg.report.temp = (int)data.Temperature;
            // if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
            // {
            //     free(app_msg);
            // }
            // sleep(3);
        }
        else
        {
            light_model=0;
            g_app_cb.led = 0;
            Light_StatusSet(OFF);
            printf("Light OFF!");
            printf("0");
            // app_msg->msg.report.hum = (int)data.Humidity;
            // app_msg->msg.report.lum = (int)data.Lux;
            // app_msg->msg.report.temp =1;
            // if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
            // {
            //     free(app_msg);
            // }
            // sleep(3);
        }
        cmdret = 0;
    }
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_Motor"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "Motor");
        if (NULL == obj_para)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            motor_model=1;
            g_app_cb.motor = 1;
            Motor_StatusSet(OFF);
            printf("Motor shunshizheng!");
        }
        else
        {
            motor_model=2;
            g_app_cb.motor = 0;
            Motor_StatusSet(OFF);
            printf("Motor nishizheng!");
        }
        cmdret = 0;
    }

EXIT_OBJPARA:
EXIT_OBJPARAS:
EXIT_CMDOBJ:
    cJSON_Delete(obj_root);
EXIT_JSONPARSE:
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL, &cmdresp);
    return;
}

void thread1(void)
{
    /**
     * 28byj48永磁步进减速电机的步进序列
     * 顺序为IN1, IN2, IN3, IN4
     */
    unsigned char sequence[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001};
    /**
     * 控制步进电机运转指定的步数和方向
     * direction - 步进电机的方向（0 - 顺时针，1 - 逆时针）
     * steps - 步进电机需要运转的步数
     */
    //初始化GPIO
    GpioInit();
    //设置GPIO_2的复用功能为普通GPIO
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_0, WIFI_IOT_IO_FUNC_GPIO_0_GPIO);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_1, WIFI_IOT_IO_FUNC_GPIO_1_GPIO);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_2, WIFI_IOT_IO_FUNC_GPIO_2_GPIO);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_3, WIFI_IOT_IO_FUNC_GPIO_3_GPIO);
    //设置GPIO_2为输出模式
    GpioSetDir(WIFI_IOT_GPIO_IDX_0, WIFI_IOT_GPIO_DIR_OUT);
    GpioSetDir(WIFI_IOT_GPIO_IDX_1, WIFI_IOT_GPIO_DIR_OUT);
    GpioSetDir(WIFI_IOT_GPIO_IDX_2, WIFI_IOT_GPIO_DIR_OUT);
    GpioSetDir(WIFI_IOT_GPIO_IDX_3, WIFI_IOT_GPIO_DIR_OUT);
    void step(int direction, int steps)
    {
        int i, j;
        for (i = 0; i < steps; i++)
        {
            // 控制步进电机的运转方向
            if (direction == 0)   // 顺时针
            {
                for (j = 0; j < 8; j++)
                {
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_0,(sequence[j] & 0x08) >> 3);
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_1,(sequence[j] & 0x04) >> 2);
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_2,(sequence[j] & 0x02) >> 1);
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_3,sequence[j] & 0x01);
                    usleep(2);
                }
            }
            else    // 逆时针
            {
                for (j = 7; j >= 0; j--)
                {
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_0,(sequence[j] & 0x08) >> 3);
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_1,(sequence[j] & 0x04) >> 2);
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_2,(sequence[j] & 0x02) >> 1);
                    GpioSetOutputVal(WIFI_IOT_GPIO_IDX_3,sequence[j] & 0x01);
                    usleep(2);
                }
            }
        }
    }
    // 初始化GPIO引脚
    // 初始化E53_IA1扩展板
    step(0, 128);   // 顺时针运转512步
    sleep(1);
    step(1, 128);   // 逆时针运转512步
    sleep(1);
}

static int task_main_entry(void)
{
    app_msg_t *app_msg;
    uint32_t ret = WifiConnect("大聪明才能连", "12345678");
    device_info_init(CLIENT_ID, USERNAME, PASSWORD);
    oc_mqtt_init();
    oc_set_cmd_rsp_cb(oc_cmd_rsp_cb);
    while (1)
    {
        app_msg = NULL;
        (void)osMessageQueueGet(mid_MsgQueue, (void **)&app_msg, NULL, 0U);
        if (NULL != app_msg)
        {
            switch (app_msg->msg_type)
            {
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
// static int task_sensor_sent(void)
// {
//     app_msg_t *app_msg;
//     E53_IA1_Data_TypeDef data;
//     E53_IA1_Read_Data(&data);
//     while(1)
//     {
//         switch (light_model)
//         {
//             // case 0:
//             // app_msg->msg_type = en_msg_report;
//             // Light_StatusSet(ON);
//             // printf("hello");
//             // if (NULL != app_msg)
//             // {
//             //     app_msg->msg_type = en_msg_report;
//             //     app_msg->msg.report.hum =1;
//             //     app_msg->msg.report.lum =1;
//             //     app_msg->msg.report.temp = (int)data.Temperature;
//             //     if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
//             //     {
//             //         free(app_msg);
//             //     }
//             // }
//             // sleep(3);
//             // break;
//             case 1:
//             app_msg->msg_type = en_msg_report;
//             if (NULL != app_msg)
//             {
//                 app_msg->msg_type = en_msg_report;
//                 app_msg->msg.report.hum =1;
//                 app_msg->msg.report.lum =1;
//                 app_msg->msg.report.temp = (int)data.Temperature;
//                 if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
//                 {
//                     free(app_msg);
//                 }
//             }
//             sleep(3);
//             break;
//             case 2:
//             app_msg->msg_type = en_msg_report;
//             if (NULL != app_msg)
//             {
//                 app_msg->msg_type = en_msg_report;
//                 app_msg->msg.report.hum = (int)data.Humidity;
//                 app_msg->msg.report.lum = (int)data.Lux;
//                 app_msg->msg.report.temp =1;
//                 if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
//                 {
//                     free(app_msg);
//                 }
//             }
//             sleep(3);
//             default:
//             break;
//         }
//         sleep(3);
//     }
//     return 0;
// }
static int task_sensor_entry(void)
{
    app_msg_t *app_msg;
    E53_IA1_Data_TypeDef data;
    E53_IA1_Init();
    while (1)
    {
        E53_IA1_Read_Data(&data);
        app_msg = malloc(sizeof(app_msg_t));
        printf("SENSOR:lum:%.2f temp:%.2f hum:%.2f\r\n", data.Lux, data.Temperature, data.Humidity);
        switch (light_model)
        {
            case 0:
            app_msg->msg_type = en_msg_report;
            printf("dates:");
            if (NULL != app_msg)
            {
                app_msg->msg_type = en_msg_report;
                app_msg->msg.report.hum = (int)data.Humidity;
                app_msg->msg.report.lum = (int)data.Lux;
                app_msg->msg.report.temp = (int)data.Temperature;
                if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
                {
                    free(app_msg);
                }
            }
            break;
            case 1:
            app_msg->msg_type = en_msg_report;
            if (NULL != app_msg)
            {
                app_msg->msg_type = en_msg_report;
                app_msg->msg.report.hum =1;
                app_msg->msg.report.lum =1;
                app_msg->msg.report.temp = (int)data.Temperature;
                if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
                {
                    free(app_msg);
                }
            }
            break;
            case 2:
            app_msg->msg_type = en_msg_report;
            if (NULL != app_msg)
            {
                app_msg->msg_type = en_msg_report;
                app_msg->msg.report.hum = (int)data.Humidity;
                app_msg->msg.report.lum = (int)data.Lux;
                app_msg->msg.report.temp =1;
                if (0 != osMessageQueuePut(mid_MsgQueue, &app_msg, 0U, 0U))
                {
                    free(app_msg);
                }
            }
            default:
            break;
        }
    }
    return 0;
}

static void OC_Demo(void)
{
    mid_MsgQueue = osMessageQueueNew(MSGQUEUE_OBJECTS, 10, NULL);
    if (mid_MsgQueue == NULL)
    {
        printf("Falied to create Message Queue!\n");
    }

    osThreadAttr_t attr;

    attr.name = "task_main_entry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 24;
    if (osThreadNew((osThreadFunc_t)task_main_entry, NULL, &attr) == NULL)
    {
        printf("Falied to create task_main_entry!\n");
    }
    // attr.name = "task_sensor_sent";
    // attr.stack_size = 2048*5;
    // attr.priority = 25;
    // if (osThreadNew((osThreadFunc_t)task_sensor_sent, NULL, &attr) == NULL)
    // {
    //     printf("Falied to create task_sensor_entry!\n");
    // }
    attr.name = "task_sensor_entry";
    attr.stack_size = 2048*5;
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)task_sensor_entry, NULL, &attr) == NULL)
    {
        printf("Falied to create task_main_entry!\n");
    }
    attr.name = "thread1";
    attr.stack_size = 1024 * 4;
    attr.priority = 23;

    if (osThreadNew((osThreadFunc_t)thread1, NULL, &attr) == NULL)
    {
        printf("Falied to create thread1!\n");
    }
}

APP_FEATURE_INIT(OC_Demo);