#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio_ex.h"
#include "wifiiot_errno.h"
#include "wifiiot_adc.h"
#include "wifi_connect.h"
#include <queue.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>
#include <dtls_al.h>
#include <mqtt_al.h>
#include "hi_io.h"
#include "hi_i2c.h"
#include "wifiiot_i2c.h"
#include "wifiiot_gpio.h"
#include "OLED.h"
#include "wifiiot_i2c_ex.h"              

#define OLED_I2C_BAUDRATE 400000

#define CONFIG_WIFI_SSID          "iQOO Z3"                            //修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD           "zyf123456"                        //修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP       "117.78.5.125"

#define CONFIG_APP_SERVERPORT     "1883"

#define CONFIG_APP_DEVICEID       "64ab6b6bae80ef457fc0ad66_202307_10"       //替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD      "12345678"        //替换为注册设备后生成的密钥

#define CONFIG_APP_LIFETIME       60     ///< seconds

#define CONFIG_QUEUE_TIMEOUT      (5*1000)

#define MSGQUEUE_OBJECTS 16 // number of Message Queue Objects

osMessageQueueId_t mid_MsgQueue; // message queue id
typedef enum //枚举
{
    // en_msg_cmd = 0,
    en_msg_report,
    en_msg_conn,
    en_msg_disconn,
}en_msg_type_t;

typedef struct//结构体
{
    char *request_id;
    char *payload;
} cmd_t;

typedef struct//结构体
{
    float vol;
    float cur;
    float t;
} report_t;

typedef struct//结构体
{
    en_msg_type_t msg_type;
    union//结构体嵌套
    {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct
{
    queue_t                     *app_msg;//消息队列（指针）
    int                          connected;

}app_cb_t;
static app_cb_t  g_app_cb;

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t    service;
    oc_mqtt_profile_kv_t         Voltage;
    oc_mqtt_profile_kv_t         Current;
    oc_mqtt_profile_kv_t         Electric_Quantity;

    if(g_app_cb.connected != 1){
        return;
    }

    service.event_time = NULL;
    service.service_id = "Voltager_Plus";
    service.service_property = &Voltage;
    service.nxt = NULL;

    Voltage.key = "Voltage";
    Voltage.value = &report->vol;
    Voltage.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    Voltage.nxt = &Current;

    Current.key = "Current";
    Current.value = &report->cur;
    Current.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    Current.nxt = &Electric_Quantity;

    Electric_Quantity.key = "Electric_Quantity";
    Electric_Quantity.value = &report->t;
    Electric_Quantity.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    Electric_Quantity.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL,&service);
    return;
}

//use this function to push all the message to the buffer - 固定功能回调函数
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int    ret = 0;
    char  *buf;
    int    buf_len;
    app_msg_t *app_msg;

    if((NULL == msg)|| (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS)){
        return ret;
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    //malloc时动态内存分配函数，用于申请一块连续的指定大小的内存块区域
    //以void*类型返回分配的内存区域地址
    if(NULL == buf){
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);

    //void *memcpy(void*dest, const void *src, size_t n);
    //由src指向地址为起始地址的连续n个字节的数据复制到以destin指向地址为起始地址的空间内。
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

static int task_main_entry(void)//主函数对应的线程函数
{
    app_msg_t *app_msg;
    uint32_t ret ;
    
    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);            //WiFi连接函数配置(名称，密码)
    dtls_al_init();                                            //DTLS协议的初始化
    mqtt_al_init();                                            //也许MQTT库需要进行一些初始化
    oc_mqtt_init();                                            //这是oc MQTT初始化函数，必须首先调用
    
    g_app_cb.app_msg = queue_create("queue_rcvmsg",10,1);      //使用这个函数创建一个队列来与其他线程通信。
    if(NULL ==  g_app_cb.app_msg){
        printf("Create receive msg queue failed");  
    }
    oc_mqtt_profile_connect_t  connect_para;
    (void) memset( &connect_para, 0, sizeof(connect_para));     //用给定的值填充内存区域

    connect_para.boostrap =      0;                             //我们是否使用引导模式
    connect_para.device_id =     CONFIG_APP_DEVICEID;           //注册设备后生成的deviceid
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;          //注册设备后生成的密钥
    connect_para.server_addr =   CONFIG_APP_SERVERIP;           //配置应用服务器IP
    connect_para.server_port =   CONFIG_APP_SERVERPORT;         //配置应用服务器端口
    connect_para.life_time =     CONFIG_APP_LIFETIME;           //配置应用生命周期
    connect_para.rcvfunc =       msg_rcv_callback;              //使用此函数将所有消息推送到缓冲区
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE; //En_DTLS_al安全类型无
    ret = oc_mqtt_profile_connect(&connect_para);               //mqtt连接
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
        //                    源队列         存储缓存数据    存储时间
        if(NULL != app_msg) {
            switch(app_msg->msg_type) {
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

/***** 获取电流值函数 *****/
static float GetCurrent(void)
{
    unsigned int ret;
    unsigned short data;

    ret = AdcRead(WIFI_IOT_ADC_CHANNEL_5, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0xff);
    if (ret != WIFI_IOT_SUCCESS)
    {
        printf("ADC Read Fail\n");
    }
    float mid = (float)data * 1.8 * 4 / 4096.0;
    return (mid - 2.5) / 0.185; 
}

/***** 获取电压值函数 *****/
static float GetVoltage(void)
{
    unsigned int ret;
    unsigned short data;

    ret = AdcRead(WIFI_IOT_ADC_CHANNEL_4, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0xff);
    //           GPIO对应的ADC通道为通道4 数据放置到data  读8次求平均值           功率默认                        
    //                      原|因                            |
    //               复用信号 7： ADC4                     采样8次
    if (ret != WIFI_IOT_SUCCESS)
    {
        printf("ADC Read Fail\n");
    }
    return (float)data * 1.8 * 4 / 4096.0; 
}


static void OLED_init_for_project(void) {
    GpioInit();
    //初始化
    I2cInit(WIFI_IOT_I2C_IDX_0, 100000); /* baudrate: 100000 */
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_13,WIFI_IOT_IO_FUNC_GPIO_13_I2C0_SDA);
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_14,WIFI_IOT_IO_FUNC_GPIO_14_I2C0_SCL);
    OLED_init();
}

static int task_sensor_entry(void)//传感器函数对应的线程函数
{
    float voltage;
    float current;
    float T,P;
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_9, WIFI_IOT_IO_PULL_UP);
    IoSetPull(WIFI_IOT_IO_NAME_GPIO_11, WIFI_IOT_IO_PULL_UP);
    OLED_init_for_project();
    while (1)
    {
        //获取电压值
        voltage = GetVoltage();
        current = GetCurrent();
        P = voltage * current;
        T = P / 1000;

        usleep(1000000);
        OLED_ShowString(0,0,"-----VALUE-----",16);//列(8-1)、行(16-1)
        OLED_ShowString(0,16,"Voltage:1.11V",16);
        OLED_ShowString(0,32,"Current:1.11A",16);
        OLED_ShowString(0,48,"P:1.11W*H",16);
        
        OLED_ShowNum(64,16,(int)voltage,1,16);
        OLED_ShowNum(80,16,((int)(voltage*100)%100),2,16);

        OLED_ShowNum(64,32,(int)current,1,16);  
        OLED_ShowNum(80,32,(((int)(current*100))%100),2,16);

        OLED_ShowNum(16,48,(int)P,1,16);
        OLED_ShowNum(32,48,((int)(P*100)%100),2,16);

        OLED_Refresh();
        printf("vlt:%.3fV\n", voltage);
        printf("cur:%.3fA\n", current);
        float P = voltage * current;
        printf("P:%.6fW*H\n", P);

        sleep(5);
    
        app_msg_t *app_msg;
        app_msg = malloc(sizeof(app_msg_t));
        if (NULL != app_msg)
        {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.vol = (float)voltage;
            app_msg->msg.report.cur = (float)current;
            app_msg->msg.report.t = (float)P;
            if(0 != queue_push(g_app_cb.app_msg,app_msg,CONFIG_QUEUE_TIMEOUT)){
                free(app_msg);
            }
        }
        sleep(3);
    }
    return 0;
}

static void OC_Demo(void)
{
    osThreadAttr_t attr;             //线程属性

    attr.name = "task_main_entry";   //线程名称(主函数)
    attr.attr_bits = 0U;             //线程属性位
    attr.cb_mem = NULL;              //用于线程控制块的内存
    attr.cb_size = 0U;               //线程控制块的内存大小
    attr.stack_mem = NULL;           //线程堆栈的内存
    attr.stack_size = 10240;         //线程堆栈内存的大小
    attr.priority = 24;              //线程对应的优先级(high)
    // osThreadNew((osThreadFunc_t)task_main_entry,  NULL,                    &attr)
    //                                |               |                         |
    //                             线程函数   作为启动参数传递给线程函数的指针   线程属性  
    // 返回线程ID;发生错误时返回NULL。                        
    if (osThreadNew((osThreadFunc_t)task_main_entry, NULL, &attr) == NULL)
    {
        printf("Falied to create task_main_entry!\n");
    }
    attr.stack_size = 2048;          //线程堆栈内存的大小
    attr.priority = 25;              //线程对应的优先级(low)
    attr.name = "task_sensor_entry"; //线程属性(传感器)
    if (osThreadNew((osThreadFunc_t)task_sensor_entry, NULL, &attr) == NULL)
    {
        printf("Falied to create task_sensor_entry!\n");
    }
}

APP_FEATURE_INIT(OC_Demo);
