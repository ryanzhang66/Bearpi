#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_i2c.h"
#include "iot_i2c_ex.h"
osStatus_t status;

typedef enum {
    OFF = 0,
    ON
}Status;

static void motor(Status A)
{
    if (A == ON){
        IoTGpioSetOutputVal(8,1);
    }
    if (A == OFF){
        IoTGpioSetOutputVal(8,0);
    }
}
static void F1(void *arg)
{
    (void)arg;
    motor(ON);
}
static void F2(void *arg)
{
    (void)arg;
    motor(OFF);
}
static void button_task(void)
{
    //初始化GPIO8（电机）
    IoTGpioInit(8);
    IoTGpioSetFunc(8,0);
    IoTGpioSetDir(8,IOT_GPIO_DIR_OUT);
    // 设置F1
    IoTGpioInit(11);
    IoTGpioSetFunc(11,IOT_GPIO_FUNC_GPIO_11_GPIO);
    IoTGpioSetDir(11,IOT_GPIO_DIR_IN);
    IoTGpioSetPull(11,IOT_GPIO_PULL_UP);
    IoTGpioRegisterIsrFunc(11,IOT_INT_TYPE_EDGE,IOT_GPIO_EDGE_FALL_LEVEL_LOW,F1,NULL);

    IoTGpioInit(12);
    IoTGpioSetFunc(12,IOT_GPIO_FUNC_GPIO_12_GPIO);
    IoTGpioSetDir(12,IOT_GPIO_DIR_IN);
    IoTGpioSetPull(12,IOT_GPIO_PULL_UP);
    IoTGpioRegisterIsrFunc(12,IOT_INT_TYPE_EDGE,IOT_GPIO_EDGE_FALL_LEVEL_LOW,F2,NULL);
}

static void button_mon_example(void)
{

    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="ia1";
    if (osThreadNew((osThreadFunc_t)button_task,NULL,&attr) == NULL){
        printf("Failed to create button_task!\r\n");
    }
}
APP_FEATURE_INIT(button_mon_example);
