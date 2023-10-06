#include<stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "ohos_init.h"
#include "iot_gpio_ex.h"

#define LED_GPIO 2

static void led(void)
{
    IoTGpioInit(LED_GPIO);
    IoTGpioSetDir(LED_GPIO,IOT_GPIO_DIR_OUT);
    IoTGpioSetFunc(2,IOT_GPIO_FUNC_GPIO_2_PWM2_OUT);
    IoTPwmInit(LED_GPIO);
    while(1){
        for (int i; i<100; i++)
        {
            IoTGpioSetOutputVal(LED_GPIO,i);  //0~255
            osDelay(10U);
        }
        
    }
}
static void led_example(void)
{
    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="led_1";
    if (osThreadNew((osThreadFunc_t)led,NULL,&attr) == NULL){
        printf("Failed to create led!\r\n");
    }
}
APP_FEATURE_INIT(led_example);
