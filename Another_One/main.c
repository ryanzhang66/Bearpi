//必用的头文件
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifi_connect.h"
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>
#include "E53_SC2.h"
#include <dtls_al.h>
#include <mqtt_al.h>
#include "EMG_filters.h"

//WiFi账号的宏定义
#define CONFIG_WIFI_SSID          "iQOO Z3"                            //修改为自己的WiFi 热点账号
//WiFi密码的宏定义
#define CONFIG_WIFI_PWD           "zyf123456"                        //修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP       "117.78.5.125"

#define CONFIG_APP_SERVERPORT     "1883"
//设备ID宏定义
#define CONFIG_APP_DEVICEID       "650838e22b7b2112739440be_2023_09_18"       //替换为注册设备后生成的deviceid
//设备密码宏定义
#define CONFIG_APP_DEVICEPWD      "12345678"        //替换为注册设备后生成的密钥
///< seconds
#define CONFIG_APP_LIFETIME       60     

#define CONFIG_QUEUE_TIMEOUT      (5*1000)
//消息队列宏定义
#define MSGQUEUE_OBJECTS 16 // number of Message Queue Objects

osMessageQueueId_t mid_MsgQueue; // message queue id

typedef enum
{
    en_msg_cmd = 0,
    en_msg_report,
}en_msg_type_t;

typedef struct
{
    char *request_id;
    char *payload;
} cmd_t;

typedef struct
{
    // 捏合次数（压力）
    int kneading;
    // 肌电信号    
    int mus;
    // MPU6050加速度x轴数据         
    char acc_x[150];
    // MPU6050加速度y轴数据 
    char acc_y[150]; 
    // MPU6050加速度z轴数据
    char acc_z[150];
    // MPU6050角速度x轴数据 
    int gr_x;
    // MPU6050角速度x轴数据        
    int gr_y;
    // MPU6050角速度x轴数据        
    int gr_z;
    // 食指弯曲传感器数据        
    int bd_1;
    // 中指弯曲传感器数据
    int bd_2;
    // 无名指弯曲传感器数据
    int bd_3;

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
    queue_t             *app_msg;
    int                 connected;
    // 捏合（压力传感器）
    int flag_kneading; 
    // 弯曲传感器
    int flag_bend;     
    // 加速度传感器
    int flag_tremor;   
    // 肌电信号传感器
    int flag_muscle;   
    //判断捏合时间是否到达
    int flag_timer_1;
    //判断弯曲时间是否到达
    int flag_timer_2; 
    //判断震颤时间是否到达    
    int flag_timer_3;
    //判断肌电时间是否到达     
    int flag_timer_4;
    //手指的初始状态：弯曲     
    int flag_finger_bend; 

} app_cb_t;
static app_cb_t g_app_cb = {0};

E53_SC2_Data_TypeDef E53_SC2_Data;

/**
 *********************************************
 ****** 算子下发的参数定义,采用结构体的形式******
 *********************************************
*/
// 映射函数：弯曲传感器,肌电传感器
typedef struct
{
    int in_min;
    int in_max;
    int out_min;
    int out_max;
} para_map;

//压力、弯曲以及肌电的限幅算法
typedef struct
{
    int low;
    int high;
} para_constrain_common;

// 加速度的限幅算法
typedef struct
{
    double low;
    double high;
}para_constrain_mpu6050;

/*整体参数汇总*/
typedef struct
{
    // 弯曲传感器的映射算子-参数
    para_map bend_map_para;                        
    // 肌电传感器的映射算子-参数
    para_map muscle_map_para;                      
    // 压力传感器的限幅算子-参数
    para_constrain_common kneading_constrain_para; 
    // 弯曲传感器的限幅算子-参数
    para_constrain_common bend_constrain_para;     
    // 肌电传感器的限幅算子-参数
    para_constrain_common muscle_constrain_para;   
    // MPU6050的限幅算子-参数
    para_constrain_mpu6050 tremor_constrain_para;   
} operator_para;
// 参数赋初值
static operator_para data_para = {{1580, 1080, 0, 90}, {120, 1800, 0, 1023}, {117, 450}, {1080, 1580}, {120, 1800}, {0.1, 0.9}}; 

/******  数字转化为字符串功能函数    ******/
void hex_to_str(int *hex, int hex_len, char *str)
{
	// 定义标志变量
int i, pos = 0;
// 定义标志变量
    char str_1[6];
    for (i = 0; i < hex_len; i++)
    {
        sprintf(str_1, "%d", hex[i]);
        sprintf(str + pos, "%s.", str_1);

        pos += strlen(str_1) + 1;
    }
}

/***** 获取电压值函数 *****/
static int GetVoltage(int i)
{
    unsigned int ret;
unsigned short data[6];
// 配置ADC通道和模式
    WifiIotAdcChannelIndex adc_channel[6] = {WIFI_IOT_ADC_CHANNEL_3, WIFI_IOT_ADC_CHANNEL_5, WIFI_IOT_ADC_CHANNEL_1, WIFI_IOT_ADC_CHANNEL_2, WIFI_IOT_ADC_CHANNEL_4, WIFI_IOT_ADC_CHANNEL_6};

    ret = AdcRead(adc_channel[i], &data[i], WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0xff);
    if (ret != WIFI_IOT_SUCCESS)
    {
        printf("ADC Read Fail\n");
    }

    return (int)data[i];
}

/***** 定时器1 回调函数 *****/
void Timer1_Callback(void *arg)
{
    (void)arg;
    g_app_cb.flag_timer_1 = 1;
}

/***** 定时器2 回调函数 *****/
void Timer2_Callback(void *arg)
{
    (void)arg;
    g_app_cb.flag_timer_2 = 1;
}

/***** 定时器3 回调函数 *****/
void Timer3_Callback(void *arg)
{
    (void)arg;
    g_app_cb.flag_timer_3 = 1;
}

/***** 定时器4 回调函数 *****/
void Timer4_Callback(void *arg)
{
    (void)arg;
    g_app_cb.flag_timer_4 = 1;
}

/**
 * 限幅语句之条件运算符写法
 ***/
int16_t constrain_u_int16_t(int16_t amt, int16_t low, int16_t high) //限幅
{
    return ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)));
}

/***** map 映射函数 *****/
int16_t map_c(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * 中值滤波算法
 ***/
unsigned int Median(int *bArray, int iFilterLen)
{
    // 定义循环变量
    int i, j; 
    unsigned char bTemp;

    // 用冒泡法对数组进行排序
    for (j = 0; j < iFilterLen - 1; j++)
    {
        for (i = 0; i < iFilterLen - j - 1; i++)
        {
            if (bArray[i] > bArray[i + 1])
            {
                // 互换
                bTemp = bArray[i];
                bArray[i] = bArray[i + 1];
                bArray[i + 1] = bTemp;
            }
        }
    }
    bTemp = (bArray[iFilterLen / 2] + bArray[iFilterLen / 2 + 1]) / 2;
    return bTemp;
}

/**
 * 加速度-限幅消抖算法
 */
void mpu6050_constrain(int *acc_x, int *acc_y, int *acc_z)
{
    if (*acc_x >= 0)
    {
        *acc_x = (int)*acc_x * data_para.tremor_constrain_para.high + *acc_x * data_para.tremor_constrain_para.low;
    }
    else
    {
        *acc_x = (int)*acc_x * data_para.tremor_constrain_para.high - *acc_x * data_para.tremor_constrain_para.low;
    }
    //对y轴进行限幅消抖滤波
    if (*acc_y >= 0)
    {
        *acc_y = (int)*acc_y * data_para.tremor_constrain_para.high + *acc_y * data_para.tremor_constrain_para.low;
    }
    else
    {
        *acc_y = (int)*acc_y * data_para.tremor_constrain_para.high - *acc_y * data_para.tremor_constrain_para.low;
    }
    //对z轴进行限幅消抖滤波
    if (*acc_z >= 0)
    {
        *acc_z = (int)*acc_z * data_para.tremor_constrain_para.high + *acc_z * data_para.tremor_constrain_para.low;
    }
    else
    {
        *acc_z = (int)*acc_z * data_para.tremor_constrain_para.high - *acc_z * data_para.tremor_constrain_para.low;
        *acc_z = *acc_z - 2330;
    }
    return;
}



static void deal_report_msg(report_t *report)
{
	// 定义MQTT物模型
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t muscle;
    oc_mqtt_profile_kv_t pressure;
    oc_mqtt_profile_kv_t accel_x;
    oc_mqtt_profile_kv_t accel_y;
    oc_mqtt_profile_kv_t accel_z;
    oc_mqtt_profile_kv_t gyro_x;
    oc_mqtt_profile_kv_t gyro_y;
    oc_mqtt_profile_kv_t gyro_z;
    oc_mqtt_profile_kv_t bend_1; 
    oc_mqtt_profile_kv_t bend_2; 
    oc_mqtt_profile_kv_t bend_3; 

    if(g_app_cb.connected != 1){
        return;
    }

    service.event_time = NULL;
    service.service_id = "Parkinson&Sensor";
    service.service_property = &muscle;
    service.nxt = NULL;
    // 肌电传感器
    muscle.key = "Muscle"; 
    muscle.value = &report->mus;
    muscle.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    muscle.nxt = &pressure;
    // 压力传感器
    pressure.key = "Pressure"; 
    pressure.value = &report->kneading;
    pressure.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    pressure.nxt = &accel_x;
    // 加速度x轴
    accel_x.key = "Accel_x";
    accel_x.value = &report->acc_x;
    accel_x.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    accel_x.nxt = &accel_y;
    // 加速度y轴
    accel_y.key = "Accel_y";
    accel_y.value = &report->acc_y;
    accel_y.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    accel_y.nxt = &accel_z;
    // 加速度z轴
    accel_z.key = "Accel_z";
    accel_z.value = &report->acc_z;
    accel_z.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    accel_z.nxt = &gyro_x;
    // 角速度x轴
    gyro_x.key = "Gyro_x";
    gyro_x.value = &report->gr_x;
    gyro_x.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    gyro_x.nxt = &gyro_y;
    // 角速度y轴
    gyro_y.key = "Gyro_y";
    gyro_y.value = &report->gr_y;
    gyro_y.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    gyro_y.nxt = &gyro_z;
    // 角速度z轴
    gyro_z.key = "Gyro_z";
    gyro_z.value = &report->gr_z;
    gyro_z.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    gyro_z.nxt = &bend_1;
    // 食指弯曲
    bend_1.key = "Bend_1";
    bend_1.value = &report->bd_1;
    bend_1.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    bend_1.nxt = &bend_2;
    // 中指弯曲
    bend_2.key = "Bend_2";
    bend_2.value = &report->bd_2;
    bend_2.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    bend_2.nxt = &bend_3;
    // 无名指弯曲
    bend_3.key = "Bend_3";
    bend_3.value = &report->bd_3;
    bend_3.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    bend_3.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL,&service);
    return;
}

// Message推送函数
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
	// 定义变量
    int    ret = 0;
    char  *buf;
    int    buf_len;
    app_msg_t *app_msg;
// 判断msg是否等于null
    if((NULL == msg) || (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS)){
        return ret;
    }
	// 计算buffer的长度
    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    if(NULL == buf){
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);
// 推送msg
    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.request_id = buf;
    buf_len = strlen(msg->request_id);
    buf += buf_len + 1;
    memcpy(app_msg->msg.cmd.request_id, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';
// 推送buffer长度
    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy(app_msg->msg.cmd.payload, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = queue_push(g_app_cb.app_msg,app_msg,10);
    if(ret != 0){
        free(app_msg);
    }

    return ret;
}

///< 指令处理磨矿
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
        goto EXIT_JSONPARSE;
    }

    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (NULL == obj_cmdname)
    {
        goto EXIT_CMDOBJ;
    }

    /* 
    * 根据不同的指令判断：进入对应的处理模块
    * 指令1：手指捏合 Kneading
    * 指令2：手指弯曲 Bend
    * 指令3：震颤 Tremor
    * 指令4：肌电 Muscle
    */

    // 指令1：手指捏合 Kneading
    if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Kneading"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            printf("EXIT_OBJPARAS!");
            goto EXIT_OBJPARAS;
        }

        obj_para = cJSON_GetObjectItem(obj_paras, "kneading_control");
        if (NULL == obj_para)
        {
            printf("EXIT_OBJPARA!");
            goto EXIT_OBJPARA;
        }
        ///< 打开压力传感器 here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            g_app_cb.flag_kneading = 1;
            // Light_StatusSet(ON);  // 此处可以添加LED灯，提示目前对应传感器正在操作中
            printf("压力传感器 On!");
        }
        ///< 关闭压力传感器
        else
        {
            g_app_cb.flag_kneading = 0;
            // Light_StatusSet(OFF);
            printf("压力传感器 Off!");
        }
        cmdret = 0;
    }
    // 指令2：手指弯曲 Bend
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Bend"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "bend_control");
        if (NULL == obj_para)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            g_app_cb.flag_bend = 1;
            // Motor_StatusSet(ON);
            printf("弯曲传感器 On!");
        }
        else
        {
            g_app_cb.flag_bend = 0;
            // Motor_StatusSet(OFF);
            printf("弯曲传感器 Off!");
        }
        cmdret = 0;
    }

    // 指令3：震颤 Tremor 加速度传感器MPU6050
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Tremor"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "tremor_control");
        if (NULL == obj_para)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            g_app_cb.flag_tremor = 1;
            // Motor_StatusSet(ON);
            printf("MPU6050 On!");
        }
        else
        {
            g_app_cb.flag_tremor = 0;
            // Motor_StatusSet(OFF);
            printf("MPU6050 Off!");
        }
        cmdret = 0;
    }

    // 指令4：肌电 Muscle
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Muscle"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "muscle_control");
        if (NULL == obj_para)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            g_app_cb.flag_muscle = 1;
            // Motor_StatusSet(ON);
            printf("肌电传感器 On!");
        }
        else
        {
            g_app_cb.flag_muscle = 0;
            // Motor_StatusSet(OFF);
            printf("肌电传感器 Off!");
        }
        cmdret = 0;
    }

    /*************************************************************/
    /******************以下指令为算法参数下发***********************/
    /*************************************************************/

    /* 指令5：弯曲传感器的映射算子-参数 {1580, 1080, 0, 90} */
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "弯曲传感器-映射算子"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para_in_min = cJSON_GetObjectItem(obj_paras, "in_min");
        obj_para_in_max = cJSON_GetObjectItem(obj_paras, "in_max");
        obj_para_out_min = cJSON_GetObjectItem(obj_paras, "out_min");
        obj_para_out_max = cJSON_GetObjectItem(obj_paras, "out_max");
        if (NULL == obj_para_in_min || NULL == obj_para_in_max || NULL == obj_para_out_min || NULL == obj_para_out_max)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        printf("弯曲传感器的映射算子-参数!\n");
        data_para.bend_map_para.in_min = (int)cJSON_GetNumberValue(obj_para_in_min);
        data_para.bend_map_para.in_max = (int)cJSON_GetNumberValue(obj_para_in_max);
        data_para.bend_map_para.out_min = (int)cJSON_GetNumberValue(obj_para_out_min);
        data_para.bend_map_para.out_max = (int)cJSON_GetNumberValue(obj_para_out_max);

        cmdret = 0;
    }

    /* 指令6：肌电传感器的映射算子-参数 {120, 1800, 0, 1023} */
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "肌电传感器-映射算子"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para_in_min = cJSON_GetObjectItem(obj_paras, "in_min");
        obj_para_in_max = cJSON_GetObjectItem(obj_paras, "in_max");
        obj_para_out_min = cJSON_GetObjectItem(obj_paras, "out_min");
        obj_para_out_max = cJSON_GetObjectItem(obj_paras, "out_max");
        if (NULL == obj_para_in_min || NULL == obj_para_in_max || NULL == obj_para_out_min || NULL == obj_para_out_max)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        printf("肌电传感器的映射算子-参数!\n");
        data_para.muscle_map_para.in_min = (int)cJSON_GetNumberValue(obj_para_in_min);
        data_para.muscle_map_para.in_max = (int)cJSON_GetNumberValue(obj_para_in_max);
        data_para.muscle_map_para.out_min = (int)cJSON_GetNumberValue(obj_para_out_min);
        data_para.muscle_map_para.out_max = (int)cJSON_GetNumberValue(obj_para_out_max);

        cmdret = 0;
    }

    /* 指令7：压力传感器的限幅算子-参数  {117, 450} */
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "压力传感器-限幅算子"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        
        obj_para_low = cJSON_GetObjectItem(obj_paras, "low");
        obj_para_high = cJSON_GetObjectItem(obj_paras, "high");

        if (NULL == obj_para_low || NULL == obj_para_high)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        printf("压力传感器的限幅算子-参数!\n");
        data_para.kneading_constrain_para.low = (int)cJSON_GetNumberValue(obj_para_low);
        data_para.kneading_constrain_para.high = (int)cJSON_GetNumberValue(obj_para_high);

        cmdret = 0;
    }

    /* 指令8：弯曲传感器的限幅算子-参数  {1080, 1580} */
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "弯曲传感器-限幅算子"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        
        obj_para_low = cJSON_GetObjectItem(obj_paras, "low");
        obj_para_high = cJSON_GetObjectItem(obj_paras, "high");

        if (NULL == obj_para_low || NULL == obj_para_high)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        printf("弯曲传感器的限幅算子-参数!\n");
        data_para.bend_constrain_para.low = (int)cJSON_GetNumberValue(obj_para_low);
        data_para.bend_constrain_para.high = (int)cJSON_GetNumberValue(obj_para_high);

        cmdret = 0;
    }

    /* 指令9：肌电传感器的限幅算子-参数  {120, 1800} */
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "肌电传感器-限幅算子"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        
        obj_para_low = cJSON_GetObjectItem(obj_paras, "low");
        obj_para_high = cJSON_GetObjectItem(obj_paras, "high");

        if (NULL == obj_para_low || NULL == obj_para_high)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        printf("肌电传感器的限幅算子-参数!\n");
        data_para.muscle_constrain_para.low = (int)cJSON_GetNumberValue(obj_para_low);
        data_para.muscle_constrain_para.high = (int)cJSON_GetNumberValue(obj_para_high);

        cmdret = 0;
    }

    /* 指令10：MPU6050的限幅算子-参数  {0.1, 0.9} */
    else if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "MPU6050-限幅算子"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        
        obj_para_low = cJSON_GetObjectItem(obj_paras, "low");
        obj_para_high = cJSON_GetObjectItem(obj_paras, "high");

        if (NULL == obj_para_low || NULL == obj_para_high)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Motor here
        printf("MPU6050的限幅算子-参数!\n");
        data_para.tremor_constrain_para.low = cJSON_GetNumberValue(obj_para_low);
        data_para.tremor_constrain_para.high = cJSON_GetNumberValue(obj_para_high);
        
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

static int task_main_entry(void)
{
    app_msg_t *app_msg;
    uint32_t ret ;
    
    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();

    g_app_cb.app_msg = queue_create("queue_rcvmsg",10,1);
    if(NULL ==  g_app_cb.app_msg){
        printf("Create receive msg queue failed");
        
    }
    oc_mqtt_profile_connect_t  connect_para;
    (void) memset( &connect_para, 0, sizeof(connect_para));

    connect_para.boostrap =      0;
    connect_para.device_id =     CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr =   CONFIG_APP_SERVERIP;
    connect_para.server_port =   CONFIG_APP_SERVERPORT;
    connect_para.life_time =     CONFIG_APP_LIFETIME;
    connect_para.rcvfunc =       msg_rcv_callback;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if((ret == (int)en_oc_mqtt_err_ok)){
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    }
    else
    {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }
    
    while (1)
    {
        app_msg = NULL;
        (void)queue_pop(g_app_cb.app_msg,(void **)&app_msg,0xFFFFFFFF);
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

static int task_sensor_entry(void)
{
    // 定时器
    osTimerId_t id1, id2, id3, id4;
    uint32_t timerDelay;
    osStatus_t status_1, status_2, status_3, status_4;
    uint32_t exec1, exec2, exec3, exec4;

    app_msg_t *app_msg;
    unsigned int ret;
    unsigned short data;
    // 1个肌电信号 1个压力传感器 3个弯曲信号
    int voltage_1[20], voltage_2[20], voltage_3[20], voltage_4[20], voltage_5[20]; 
    int kneading_pres = 0, bend_angle_1 = 0, bend_angle_2 = 0, bend_angle_3 = 0, bend_angle = 0, stretch_angle = 90, stretch_angle_1 = 0, stretch_angle_2 = 0, stretch_angle_3 = 0, muscle = 0;
    int Acc_x[20], Acc_y[20], Acc_z[20], Gr_x[20], Gr_y[20], Gr_z[20];
    int acc_x, acc_y, acc_z, gr_x, gr_y, gr_z;

    E53_SC2_Init();

    static int num_kneading = 0, frequency_tremor = 0, amplitude_tremor = 0;

    /* 肌电信号的处理参数 */
    int threshold = 10000; 
    int EMG_num = 0;       
    // ADC1  voltage_2 引脚4  压力传感器
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_4, WIFI_IOT_IO_PULL_DOWN); 
    // ADC2  voltage_3 引脚5  弯曲传感器1 食指
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_5, WIFI_IOT_IO_PULL_DOWN); 
    // ADC4  voltage_4 引脚9  弯曲传感器2 拇指 
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_IO_PULL_DOWN); 
    // ADC6  voltage  引脚13  弯曲传感器3 无名指 
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_13, WIFI_IOT_IO_PULL_DOWN); 
    //ADC5  voltage_1 引脚11  肌电传感器
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_PULL_DOWN); 
    // ADC3 引脚7  此ADC上传空数据
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_7, WIFI_IOT_IO_PULL_DOWN); 

    // 定时器1：用于手指捏合，目前设置时间为8秒触发
    exec1 = 1U;
    id1 = osTimerNew(Timer1_Callback, osTimerOnce, &exec1, NULL);

    // 定时器2：用于弯曲情况的时间限定，目前设置为4秒触发
    exec2 = 1U;
    id2 = osTimerNew(Timer2_Callback, osTimerOnce, &exec2, NULL);

    // 定时器3：用于震颤情况的时间限定，目前设置为30秒触发
    exec3 = 1U;
    id3 = osTimerNew(Timer3_Callback, osTimerOnce, &exec3, NULL);

    // 定时器4：用于肌电情况的时间限定，目前设置为4秒触发
    exec4 = 1U;
    id4 = osTimerNew(Timer4_Callback, osTimerOnce, &exec4, NULL);

    while (1)
    {
        /**
        * @功能 第一个判断语句：使用压力传感器采集捏合次数，并且将结果推送到消息队列中
        * @采集频率 每次采集20次，一共采集8秒
        ***/
        // 压力传感器
        if (g_app_cb.flag_kneading == 1)
        {
            timerDelay = 800U;
            printf("--------压力传感器获取数据---------\n");
             // 定时器开始
            status_1 = osTimerStart(id1, timerDelay);

            while (1)
            {
                for (int i = 0; i < 20; i++)
                {
                    // 获取20个数据
                    voltage_2[i] = constrain_u_int16_t(GetVoltage(2), data_para.kneading_constrain_para.low, data_para.kneading_constrain_para.high); 
                }
                // 中值滤波
                kneading_pres = Median(voltage_2, 20); 

                printf("--------压力传感器-kneading_pres：%d\n", kneading_pres);
                //延时0.01s
                usleep(10000); 

                if (kneading_pres > 140)
                {
                    while (constrain_u_int16_t(GetVoltage(2), 117, 450) < 140 && g_app_cb.flag_timer_1 == 0 && g_app_cb.flag_kneading == 1)
                    {
                        num_kneading++;
                        printf("--------检测中-num_kneading:%d--------\n", num_kneading);
                        break;
                    }
                }
                // 定时器，为1的时候：表示已经达到8秒，否则为0没有达到
                if (g_app_cb.flag_timer_1 == 1) 
                {
                    /* 数据推送到队列中 */
                    app_msg = malloc(sizeof(app_msg_t));
                    if (NULL != app_msg)
                    {
                        app_msg->msg_type = en_msg_report;
                        // 捏合次数
                        app_msg->msg.report.kneading = (int)num_kneading; 
                        /**************其他传感器**************/
                        app_msg->msg_type = en_msg_report;
                        sprintf(app_msg->msg.report.acc_x, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_y, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_z, "%d\0", 9);
                        app_msg->msg.report.gr_x = 9;
                        app_msg->msg.report.gr_y = 9;
                        app_msg->msg.report.gr_z = 9;
                        app_msg->msg.report.mus = 9;
                        app_msg->msg.report.bd_1 = 9;
                        app_msg->msg.report.bd_2 = 9;
                        app_msg->msg.report.bd_3 = 9;
                        printf("--------队列-num_kneading:%d--------\n", num_kneading);
                        if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT)){
                            free(app_msg);
                        }
                    }
                    /* 标志位复位 */
                    num_kneading = 0;
                    g_app_cb.flag_timer_1 = 0;
                    g_app_cb.flag_kneading = 0;
                    status_1 = osTimerStop(id1);
                    break;
                }
                // 此处为0 ，表示平台下发停止检测
                if (g_app_cb.flag_kneading == 0) 
                {
                    status_1 = osTimerStop(id1);
                    num_kneading = 0;
                    break;
                }
            }
        }

        /**
        * @功能 第二个判断语句：使用弯曲传感器采集患者的5次手指伸展情况，包括最大角度、以及最大时长，并且将结果推送到消息队列中
        * @采集频率 每秒采集20次，时间最长采集4秒
        ***/
        else if (g_app_cb.flag_bend == 1)
        {
            timerDelay = 400U;
            status_2 = osTimerStart(id2, timerDelay); // 定时器开始
            printf("--------传感器获取数据---------\n");
            while (1)
            {
                for (int i = 0; i < 10; i++)
                {
                    // 获取20个数据
                    voltage_3[i] = map_c(constrain_u_int16_t(GetVoltage(3), data_para.bend_constrain_para.low, data_para.bend_constrain_para.high), data_para.bend_map_para.in_min, data_para.bend_map_para.in_max, data_para.bend_map_para.out_min, data_para.bend_map_para.out_max); 
                    // 获取20个数据
                    voltage_4[i] = map_c(constrain_u_int16_t(GetVoltage(4), data_para.bend_constrain_para.low, data_para.bend_constrain_para.high), data_para.bend_map_para.in_min, data_para.bend_map_para.in_max, data_para.bend_map_para.out_min, data_para.bend_map_para.out_max); 
                    // 获取20个数据
                    voltage_5[i] = map_c(constrain_u_int16_t(GetVoltage(5), data_para.bend_constrain_para.low, data_para.bend_constrain_para.high), data_para.bend_map_para.in_min, data_para.bend_map_para.in_max, data_para.bend_map_para.out_min, data_para.bend_map_para.out_max); 
                }
                // 中值滤波1
                bend_angle_1 = Median(voltage_3, 10); 
                // 中值滤波2
                bend_angle_2 = Median(voltage_4, 10); 
                // 中值滤波3
                bend_angle_3 = Median(voltage_5, 10); 
                bend_angle = (bend_angle_1 + bend_angle_2 + bend_angle_3) / 3;
                printf("--------传感器获取数据-bend_angle---------%d:\n", bend_angle);

                if (stretch_angle >= bend_angle)
                {
                    // 保存伸直状态的极限情况
                    stretch_angle = bend_angle;
                    stretch_angle_1 = bend_angle_1;
                    stretch_angle_2 = bend_angle_2;
                    stretch_angle_3 = bend_angle_3;
                    printf("-----极限情况-stretch_angle1-stretch_angle2-stretch_angle3---%d-%d-%d:\n", stretch_angle_1, stretch_angle_2, stretch_angle_3);
                }

                // bend_angle大于60°，表示手指呈弯曲状态，此时为初始状态范围内，不进入判断;
                // 小于60表示呈伸直状态,进入判断；直到手指回归到弯曲情况，即大于60°，表示一个动作完成，进行上传
                // 此判断都是在4s内完成，但是此时的定时器还在操作中，因此需要进行关闭；另外其余的变量初始值复位
                if (g_app_cb.flag_finger_bend == 1 || bend_angle < 20) 
                {
                    g_app_cb.flag_finger_bend = 1;

                    if (bend_angle >= 20) 
                    {
                        //  此时说明回到弯曲状态，需要将手指伸展的角度上传到平台
                        /* 数据推送到队列中 */
                        app_msg = malloc(sizeof(app_msg_t));
                        if (NULL != app_msg)
                        {
                            app_msg->msg_type = en_msg_report;
                            // 食指伸展角度
                            app_msg->msg.report.bd_1 = (int)stretch_angle_1;
                            // 中指伸展角度 
                            app_msg->msg.report.bd_1 = (int)stretch_angle_2; 
                            // 无名指伸展角度
                            app_msg->msg.report.bd_1 = (int)stretch_angle_3; 
                            /**************其他传感器**************/
                            app_msg->msg_type = en_msg_report;
                            sprintf(app_msg->msg.report.acc_x, "%d\0", 9);
                            sprintf(app_msg->msg.report.acc_y, "%d\0", 9);
                            sprintf(app_msg->msg.report.acc_z, "%d\0", 9);
                            app_msg->msg.report.gr_x = 9;
                            app_msg->msg.report.gr_y = 9;
                            app_msg->msg.report.gr_z = 9;
                            app_msg->msg.report.mus = 9;
                            app_msg->msg.report.kneading = 9;
                            printf("----正常上传stretch_angle1-stretch_angle3-stretch_angle3----%d-%d-%d:\n", stretch_angle_1, stretch_angle_2, stretch_angle_3);
                            if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT))
                            {
                                free(app_msg);
                            }
                        }
                        /* 标志位复位 */
                        g_app_cb.flag_finger_bend = 0;
                        g_app_cb.flag_bend = 0;
                        stretch_angle = 90;
                        g_app_cb.flag_timer_2 = 0;
                        status_2 = osTimerStop(id2);
                        break;
                    }
                }

                /* 定时器，为1的时候：表示已经达到4秒，否则为0没有达到 */
                // 此时表示未达到伸展范围或者在伸展范围内超时，因此上传伸展极限角度
                // 初始值复位
                if (g_app_cb.flag_timer_2 == 1)
                {
                    /* 数据推送到队列中 */
                    app_msg = malloc(sizeof(app_msg_t));
                    if (NULL != app_msg)
                    {
                        app_msg->msg_type = en_msg_report;
                        // 食指伸展角度 
                        app_msg->msg.report.bd_1 = (int)stretch_angle_1;
                        // 中指伸展角度 
                        app_msg->msg.report.bd_2 = (int)stretch_angle_2;
                        // 无名指伸展角度
                        app_msg->msg.report.bd_3 = (int)stretch_angle_3; 
                        /**************其他传感器**************/
                        app_msg->msg_type = en_msg_report;
                        sprintf(app_msg->msg.report.acc_x, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_y, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_z, "%d\0", 9);
                        app_msg->msg.report.gr_x = 9;
                        app_msg->msg.report.gr_y = 9;
                        app_msg->msg.report.gr_z = 9;
                        app_msg->msg.report.mus = 9;
                        app_msg->msg.report.kneading = 9;
                        printf("----超时上传stretch_angle1-stretch_angle3-stretch_angle3----%d-%d-%d:\n", stretch_angle_1, stretch_angle_2, stretch_angle_3);
                        if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT))
                        {
                            free(app_msg);
                        }
                    }
                    /* 标志位复位 */
                    g_app_cb.flag_finger_bend = 0;
                    g_app_cb.flag_bend = 0;
                    g_app_cb.flag_timer_2 = 0;
                    stretch_angle = 90;
                    status_2 = osTimerStop(id2);
                    break;
                }
                // 此处为0 ，表示平台下发停止检测
                if (g_app_cb.flag_bend == 0) 
                {
                    g_app_cb.flag_finger_bend = 0;
                    g_app_cb.flag_bend = 0;
                    stretch_angle = 90;
                    g_app_cb.flag_timer_2 = 0;
                    status_2 = osTimerStop(id2);
                    break;
                }
                usleep(10000); //延时0.01s
            }
        }

        /**
        * @功能 第三个判断语句：使用MPU6050传感器采集震颤频率和震颤幅度，并且将结果推送到消息队列中
        * @采集频率 每次采集20个数据，一共采集30秒
        * @函数 使用IIC函数得到对应值
        ***/
        // MPU6050传感器
        else if (g_app_cb.flag_tremor == 1)
        {
            // 8s
            timerDelay = 800U;
            // 定时器开始                       
            status_3 = osTimerStart(id3, timerDelay); 
            printf("--------传感器获取数据---------\n");

            while (1)
            {
                for (int i = 0; i < 20; i++)
                {
                    E53_SC2_Read_Data();
                    acc_x = (int)E53_SC2_Data.Accel[0];
                    acc_y = (int)E53_SC2_Data.Accel[1];
                    acc_z = (int)E53_SC2_Data.Accel[2];

                    /*******限幅算法******/
                    mpu6050_constrain(&acc_x, &acc_y, &acc_z);

                    Acc_x[i] = acc_x;
                    Acc_y[i] = acc_y;
                    Acc_z[i] = acc_z;
                    Gr_x[i] = (int)E53_SC2_Data.Gyro[0]; 
                    Gr_y[i] = (int)E53_SC2_Data.Gyro[1]; 
                    Gr_z[i] = (int)E53_SC2_Data.Gyro[2]; 
                     //延时0.05s
                    usleep(50000);                      
                }
                // 中值滤波
                gr_x = Median(Gr_x, 20); 
                // 中值滤波
                gr_y = Median(Gr_y, 20); 
                // 中值滤波
                gr_z = Median(Gr_z, 20); 
                printf("---acc_x acc_y acc_z %d %d %d\n", acc_x, acc_y, acc_z);

                /************ 数据上传***********/
                /* 数据推送到队列中 */
                app_msg = malloc(sizeof(app_msg_t));
                if (NULL != app_msg)
                {
                    app_msg->msg_type = en_msg_report;
                    hex_to_str(Acc_x, 20, app_msg->msg.report.acc_x);
                    hex_to_str(Acc_y, 20, app_msg->msg.report.acc_y);
                    hex_to_str(Acc_z, 20, app_msg->msg.report.acc_z);

                    app_msg->msg.report.gr_x = gr_x;
                    app_msg->msg.report.gr_y = gr_y;
                    app_msg->msg.report.gr_z = gr_z;

                    app_msg->msg.report.mus = 0;
                    app_msg->msg.report.kneading = 0;
                    app_msg->msg.report.bd_1 = 0;
                    app_msg->msg.report.bd_2 = 0;
                    app_msg->msg.report.bd_3 = 0;

                    if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT))
                    {
                        free(app_msg);
                    }
                }
                // 定时器，为1的时候：表示已经达到8秒，否则为0没有达到
                if (g_app_cb.flag_timer_3 == 1) 
                {
                    /* 标志位复位 */

                    g_app_cb.flag_timer_3 = 0;
                    g_app_cb.flag_tremor = 0;
                    status_3 = osTimerStop(id3);
                    break;
                }
                // 此处为0 ，表示平台下发停止检测
                if (g_app_cb.flag_tremor == 0) 
                {
                    g_app_cb.flag_timer_3 = 0;
                    g_app_cb.flag_tremor = 0;
                    status_3 = osTimerStop(id3);
                    break;
                }
            }
        }

        /**
        * @功能 第四个判断语句：使用肌电传感器采集手臂弯曲的肌电信号，并且将结果推送到消息队列中
        * @采集频率 每次采集20个数据，最长采集4秒，一共5组，共***条数据
        * @函数 使用ADC函数得到对应值
        ***/
        // 肌电传感器
        else if (g_app_cb.flag_muscle == 1)
        {
            timerDelay = 1000U;
            // 定时器开始
            status_4 = osTimerStart(id4, timerDelay); 
            EMG_init(SAMPLE_FREQ_1000HZ, NOTCH_FREQ_50HZ, true, true, true);
            int max_envelope, envelope[5], j = 0;
            // 最后的峰峰值-均值
            int envelop_value = 0; 
            // 滤波计算的中间值
            int envelope_;         
            printf("--------肌电传感器获取数据---------\n");

            while (1)
            {
                for (int i = 0; i < 20; i++)
                {
                    voltage_1[i] = map_c(constrain_u_int16_t(GetVoltage(1), data_para.muscle_constrain_para.low, data_para.muscle_constrain_para.high), data_para.muscle_map_para.in_min, data_para.muscle_map_para.in_max, data_para.muscle_map_para.out_min, data_para.muscle_map_para.out_max); // 获取20个数据
                }
                // 中值滤波
                muscle = Median(voltage_1, 20);           
                int dataAfterFilter = EMG_update(muscle); 
                envelope_ = pow(dataAfterFilter, 2); 
                printf("------------峰峰值envelope_:%d\n", envelope_);
                envelope_ = (envelope_ >= threshold) ? envelope_ : 0; 

                if (threshold > 0)
                {
                    max_envelope = getEMGCount(envelope_);
                    if (max_envelope > 0)
                    {
                        envelope[EMG_num] = max_envelope;
                        EMG_num++;
                        printf("------------肌电测试结束:%d\n", envelope_);
                    }
                }

                // 手臂弯曲了五次，将肌电信号的峰峰值送入消息队列中
                if (EMG_num == 5)
                {
                    //  五次峰峰值的均值运算
                    envelop_value = (envelope[0] + envelope[1] + envelope[2] + envelope[3] + envelope[4]) / 5;
                    printf("------------五次峰峰值envelope_:%d\n", envelop_value);
                    /* 数据推送到队列中 */
                    app_msg = malloc(sizeof(app_msg_t));
                    if (NULL != app_msg)
                    {
                        app_msg->msg_type = en_msg_report;
                        app_msg->msg.report.mus = envelop_value; // 峰峰值
                        /**************其他传感器**************/
                        app_msg->msg_type = en_msg_report;
                        sprintf(app_msg->msg.report.acc_x, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_y, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_z, "%d\0", 9);
                        app_msg->msg.report.gr_x = 9;
                        app_msg->msg.report.gr_y = 9;
                        app_msg->msg.report.gr_z = 9;
                        app_msg->msg.report.kneading = 9;
                        app_msg->msg.report.bd_1 = 9;
                        app_msg->msg.report.bd_2 = 9;
                        app_msg->msg.report.bd_3 = 9;
                        if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT))
                        {
                            free(app_msg);
                        }
                    }
                    /* 标志位复位 */
                    g_app_cb.flag_muscle = 0;
                    g_app_cb.flag_timer_4 = 0;
                    EMG_num = 0;
                    envelop_value = 0;
                    status_4 = osTimerStop(id4);
                    break;
                }

                /* 定时器，为1的时候：表示已经达到4秒，否则为0没有达到 */
                if (g_app_cb.flag_timer_4 == 1)
                {
                    //  五次峰峰值的均值运算
                    if (EMG_num < 5)
                    {
                        envelop_value = 0;
                    }
                    /* 数据推送到队列中 */
                    app_msg = malloc(sizeof(app_msg_t));
                    if (NULL != app_msg)
                    {
                        app_msg->msg_type = en_msg_report;
                        // 峰峰值
                        app_msg->msg.report.mus = envelop_value; 
                        printf("------------超时-五次峰峰值envelope_:%d\n", envelop_value);
                        /**************其他传感器**************/
                        app_msg->msg_type = en_msg_report;
                        sprintf(app_msg->msg.report.acc_x, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_y, "%d\0", 9);
                        sprintf(app_msg->msg.report.acc_z, "%d\0", 9);
                        app_msg->msg.report.gr_x = 9;
                        app_msg->msg.report.gr_y = 9;
                        app_msg->msg.report.gr_z = 9;
                        app_msg->msg.report.kneading = 9;
                        app_msg->msg.report.bd_1 = 9;
                        app_msg->msg.report.bd_2 = 9;
                        app_msg->msg.report.bd_3 = 9;
                        if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT))
                        {
                            free(app_msg);
                        }
                    }
                    /* 标志位复位 */
                    g_app_cb.flag_muscle = 0;
                    g_app_cb.flag_timer_4 = 0;
                    EMG_num = 0;
                    envelop_value = 0;
                    status_4 = osTimerStop(id4);
                    break;
                }
                // 此处为0 ，表示平台下发停止检测
                if (g_app_cb.flag_muscle == 0) 
                {
                    g_app_cb.flag_muscle = 0;
                    g_app_cb.flag_timer_4 = 0;
                    EMG_num = 0;
                    envelop_value = 0;
                    status_4 = osTimerStop(id4);
                    break;
                }
                //延时0.01s
                usleep(10000); 
            }
        }

        app_msg = malloc(sizeof(app_msg_t));
        if (NULL != app_msg)
        {
            app_msg->msg_type = en_msg_report;
            sprintf(app_msg->msg.report.acc_x, "%d\0", 9);
            sprintf(app_msg->msg.report.acc_y, "%d\0", 9);
            sprintf(app_msg->msg.report.acc_z, "%d\0", 9);
            app_msg->msg.report.gr_x = 9;
            app_msg->msg.report.gr_y = 9;
            app_msg->msg.report.gr_z = 9;
            app_msg->msg.report.mus = 9;
            app_msg->msg.report.kneading = 9;
            app_msg->msg.report.bd_1 = 9;
            app_msg->msg.report.bd_2 = 9;
            app_msg->msg.report.bd_3 = 9;
            printf("------------传感器正在运行中-------acc_z：%s\n", app_msg->msg.report.acc_z);
            printf("调试使用：参数调试!\n");
            printf("弯曲传感器的映射算子-参数(调试):in_min:%d,in_max:%d,out_min:%d,out_max:%d!\n",data_para.bend_map_para.in_min,data_para.bend_map_para.in_max,data_para.bend_map_para.out_min,data_para.bend_map_para.out_max);

            if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT))
            {
                free(app_msg);
            }
        }
        sleep(2);
    }
    return 0;
}

static void OC_Demo(void)
{
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
    attr.stack_size = 6144;
    attr.priority = 25;
    attr.name = "task_sensor_entry";
    if (osThreadNew((osThreadFunc_t)task_sensor_entry, NULL, &attr) == NULL)
    {
        printf("Falied to create task_sensor_entry!\n");
    }
}

APP_FEATURE_INIT(OC_Demo); 
