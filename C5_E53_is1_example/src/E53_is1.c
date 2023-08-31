#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_pwm.h"
#include "E53_is1.h"

void E53Is1Init()
{
    IoTGpioInit(8);   //初始化GPIO_8
    IoTGpioSetFunc(8,5);   //设置GPIO8的复用功能为PWM1
    IoTGpioSetDir(8,IOT_GPIO_DIR_OUT);   //设置PWM为输出模式
    IoTPwmInit(1);       //初始化PWM1的端口

    IoTGpioInit(7);   //初始化Gpio7（当检测到人时，传感器会输出高电平，通过对GPIO_7的监测就能判断是否有人走动。）
    IoTGpioSetFunc(7,0);             //设置GPIO7的功能
    IoTGpioSetDir(7,IOT_GPIO_DIR_IN);     //设置GPIO7为输入模式
    IoTGpioSetPull(7,IOT_GPIO_PULL_UP);    //设置引脚上拉
}

int E53Is1ReadData(E53IS1CallbackFunc func)
{
    uint32_t ret;
    ret = IoTGpioRegisterIsrFunc(7, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_RISE_LEVEL_HIGH, func, NULL);
    //设置GPIO引脚中断功能
    //(中断功能是指按压后抬起所形成的中断)
    if (ret != 0) {
        return -1;
    }
    return 0;
}

void BeepStatusSet(E53IS1Status status)
{
    if (status == ON) {
        IoTPwmStart(WIFI_IOT_PWM_PORT_PWM1, PWM_DUTY, PWM_FREQ); // 输出PWM波
    }
    if (status == OFF) {
        IoTPwmStop(WIFI_IOT_PWM_PORT_PWM1);
    }
}