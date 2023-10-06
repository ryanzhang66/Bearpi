#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_adc.h"
#include "iot_errno.h"
#include "iot_gpio_ex.h"
#include "ohos_init.h"
#define ADC_CHANNEL 5
#define ADC_GPIO 11

#define ADC_VREF_VOL 1.8
#define ADC_COEFFICIENT 4
#define ADC_RATIO 4096

static float GetVoltage(void)
{
    unsigned int ret;
    unsigned short data;

    ret=IoTAdcRead(ADC_CHANNEL,&data,IOT_ADC_EQU_MODEL_8,IOT_ADC_CUR_BAIS_DEFAULT,0xff);
    if (ret != IOT_SUCCESS){
        printf("Failed to read adc!\r\n");
    }
    return (float)data * ADC_VREF_VOL * ADC_COEFFICIENT / ADC_RATIO;
}

static void adc_task(void)
{
    float voltage;
    while(1){
        printf("=======================================\r\n");
        printf("***************ADC_example*************\r\n");
        printf("=======================================\r\n");

        voltage=GetVoltage();
        printf("voltage is %.3f\r\n",voltage);
        osDelay(100U);
    }
}

static void adc_example(void)
{
    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="adc_example";

    if (osThreadNew((osThreadFunc_t)adc_task,NULL,&attr) == NULL){
        printf("Failed to create adc_task!\r\n");
    }
}
APP_FEATURE_INIT(adc_example);