#include "DS18B20.h"		//step1
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_adc.h"
#include "iot_errno.h"
#include "iot_gpio_ex.h"
#include "ohos_init.h"

void task(void){
float currentTemp;
DS18B20_Init();		//step2
while(1){
	currentTemp=DS18B20_Read_Temperature();   //step3
	printf("\nCurrentTemperature = %.3f",currentTemp);
}
}
APP_FEATURE_INIT(task);
