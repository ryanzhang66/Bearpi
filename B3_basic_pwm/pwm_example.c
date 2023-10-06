#include<stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "ohos_init.h"
#include "iot_gpio_ex.h"
#include "iot_pwm.h"

#define LED_GPIO 2
static void pwm(void)
{
    uint32_t i;
    IoTGpioInit(LED_GPIO);    //初始化GPIO
    IoTGpioSetDir(LED_GPIO,IOT_GPIO_DIR_OUT);    //设置GPIO的引脚模式
    IoTGpioSetFunc(LED_GPIO, IOT_GPIO_FUNC_GPIO_2_PWM2_OUT);    //设置GPIO的引脚功能

    IoTPwmInit(LED_GPIO);    //初始化PWM的功能
    while (1){
        for (i=0; i<=100; i++){
            IoTPwmStart(LED_GPIO,i ,40000);      //PWM的启动函数
            printf("i is %d\r\n",i);
            osDelay(30U);
        }
    }
}
static void pwm_example(void)
{
    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="pwm_example";
    if (osThreadNew((osThreadFunc_t)pwm,NULL,&attr) == NULL){
        printf("Faile to create pwm!\r\n");
    }
}
APP_FEATURE_INIT(pwm_example);