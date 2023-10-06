/*
GpioInit 初始化GPIO
GpioDeinit 取消初始化GPIO
GpioSetDir 设置GPIO引脚方向(in\out)
GpioGetDir 获取GPIO引脚方向
GpioSetOutputVal 设置GPIO引脚输出电平值
GpioGetOutputVal 获取GPIO引脚输出电平值
IoSetPull 设置GPIO引脚上拉
IoGetPull 获取GPIO引脚上拉
IoSetFunc 设置GPIO引脚功能
IoGetFunc 获取GPIO引脚功能
IOSetDriverStrength 设置GPIO驱动能力
IOGetDriverStrength 获取GPIO驱动能力
GpioRegisterIsrFunc 设置GPIO引脚中断功能
                    (中断功能是指按压后抬起所形成的中断)
GpioUnregisterIsrFunc 取消GPIO引脚中断功能
GpioSetIsrMask 屏蔽GPIO引脚中断功能
GpioSetIsrMode 设置GPIO引脚中断触发模式
*/
#include<stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "ohos_init.h"
#include "iot_gpio_ex.h"

#define LED_GPIO 2
#define BUTTON_F1_GPIO 11
#define BUTTON_F2_GPIO 12
static void F1Pressed(char *ptr)
{
    (void)ptr;
    IoTGpioSetOutputVal(LED_GPIO,1);
}

static void F2Pressed(char *ptr)
{
    (void)ptr;
    IoTGpioSetOutputVal(LED_GPIO,0);
}

static void Button_example(void)
{
    //设置LED灯的初始参数
    IoTGpioInit(LED_GPIO);
    IoTGpioSetDir(LED_GPIO,IOT_GPIO_DIR_OUT);

    //设置F1按钮的初始参数
    IoTGpioInit(BUTTON_F1_GPIO);   //使其处于开启的状态
    IoTGpioSetFunc(BUTTON_F1_GPIO,IOT_GPIO_FUNC_GPIO_11_GPIO);   //设置这个引脚的功能
    IoTGpioSetDir(BUTTON_F1_GPIO,IOT_GPIO_DIR_IN);               //设置这个引脚的方向（输入or输出）
    IoTGpioSetPull(BUTTON_F1_GPIO,IOT_GPIO_PULL_UP);             //设置这个引脚的输入是上拉输入
    IoTGpioRegisterIsrFunc(BUTTON_F1_GPIO,IOT_INT_TYPE_EDGE,IOT_GPIO_EDGE_FALL_LEVEL_LOW,F1Pressed,NULL);  //设置引脚的中断功能

    //设置F2按钮的初始参数
    IoTGpioInit(BUTTON_F2_GPIO);
    IoTGpioSetFunc(BUTTON_F2_GPIO,IOT_GPIO_FUNC_GPIO_12_GPIO);
    IoTGpioSetDir(BUTTON_F2_GPIO,IOT_GPIO_DIR_IN);
    IoTGpioSetPull(BUTTON_F2_GPIO,IOT_GPIO_PULL_UP);
    IoTGpioRegisterIsrFunc(BUTTON_F2_GPIO,IOT_INT_TYPE_EDGE,IOT_GPIO_EDGE_FALL_LEVEL_LOW,F2Pressed,NULL);
}
APP_FEATURE_INIT(Button_example);