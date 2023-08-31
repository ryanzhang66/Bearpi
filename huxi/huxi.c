
#include<stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "ohos_init.h"
#include "iot_gpio_ex.h"
#include "iot_pwm.h"
int a;
#define LED_GPIO 2
static viod LED_GPIO2(void){
    unit_32t i;
     IoTGpioInit(LED_GPIO);    //初始化GPIO
    IoTGpioSetDir(LED_GPIO,IOT_GPIO_DIR_OUT);    //设置GPIO的引脚模式
    IoTGpioSetFunc(LED_GPIO, IOT_GPIO_FUNC_GPIO_2_PWM2_OUT);    //设置GPIO的引脚功能
    IotPwmInit(LED_GPIO)
    while(1){
        for (i=0; i<=100; i++){
            IotPwmStart(LED_GPIO;i;40000);
            printf("i is %d/r/n ",i);
           osDelay(30U);
        }
        for (i=100; i>=0; i--){
             IotPwmStart(LED_GPIO;i;40000);
            printf("i is %d/r/n ",i);
           osDelay(30U);

        }
       IoTGpioSetOutputVal(LED_GPIO,1);
       sleep(1);
       IoTGpioSetOutputVal(2,0);
       sleep(1); //
   }     // =  == 
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
    attr.name="huxi_1";
    if (osThreadNew((osThreadFunc_t)LED_GPIO2,NULL,&attr) == NULL){
        printf("Failed to create led!\r\n");
    }
}
APP_FEATURE_INIT(led_example);
