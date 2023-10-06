#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_i2c.h"
#include "nfc.h"
#include "ohos_init.h"


#define TEXT "你好呀，哈哈哈！"
#define WEB "bilibili.com"
#define WIFI_IOT_IO_FUNC_GPIO_0_I2C1_SDA 6
#define WIFI_IOT_IO_FUNC_GPIO_1_I2C1_SCL 6
#define WIFI_IOT_I2C_IDX 1
#define WIFI_IOT_I2C_BAUDRATE 400000

static void nfc_task(void)
{
    uint8_t ret;    //用来判断是否nfc写入数据

    //初始化0的SDA功能
    IoTGpioInit(0);
    IoTGpioSetFunc(0,6);   //(0,WIFI_IOT_IO_FUNC_GPIO_0_I2C1_SDA)

    //初始化1的SDL功能
    IoTGpioInit(1);
    IoTGpioSetFunc(1,6);   //(0,WIFI_IOT_IO_FUNC_GPIO_1_I2C1_SCL)

    //设置I2C的波特率（400kb）
    IoTI2cInit(1,400000);  //(WIFI_IOT_I2C_IDX,WIFI_IOT_I2C_BAUDRATE)
    printf("NFC is running!\r\n");

    //开始写入NFC的数据
    ret=storeText(NDEFFirstPos,(uint8_t*)TEXT);
    if (ret != 1){
        printf("NFC Write data Failed!\r\n");
    }
    ret=storeUrihttp(NDEFLastPos,(uint8_t*)WEB);
    if (ret != 1){
        printf("NFC Write data Failed!\r\n");
    }
    while (1){
        printf("=======================================\n");
        printf("***********I2C_NFC_example**********\n");
        printf("=======================================\n");
        printf("Please use the mobile phone with NFC function close to the development board!\n");
        osDelay(100U);
    }
}

static void nfc_example(void)
{
    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="nfc_example";

    if (osThreadNew((osThreadFunc_t)nfc_task,NULL,&attr) == NULL){
        printf("Failed to create nfc_example!\r\n");
    }
}
APP_FEATURE_INIT(nfc_example);