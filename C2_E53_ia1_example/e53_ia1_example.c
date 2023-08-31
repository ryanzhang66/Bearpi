
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "E53_IA1.h"

#define TASK_STACK_SIZE (1024 * 8)
#define TASK_PRIO 25
#define MIN_LUX 20
#define MAX_HUM 70
#define MAX_TEMP 35
#define TASK_DELAY_1S 1000000
static void ia1_task(void)
{
    int ret;
    E53IA1Data data;
    ret = E53IA1Init();
    if (ret != 0){
        printf("Failed to init!\r\n");
        return;
    }
    while (1){
        
    ret = E53IA1ReadData(&data);
    if (ret != 0){
        printf("Failed to Read!\r\n");
    }
        printf("\r\n=======================================\r\n");
        printf("\r\n*************E53_IA1_example***********\r\n");
        printf("\r\n=======================================\r\n");
        printf("\r\n******************************Lux Value is  %.2f\r\n", data.Lux);
        printf("\r\n******************************Humidity is  %.2f\r\n", data.Humidity);
        printf("\r\n******************************Temperature is  %.2f\r\n", data.Temperature);
    if (data.Lux < 20 ){
        LightStatusSet(ON);
    }
    else {
        LightStatusSet(OFF);
    }
    if (data.Humidity > 70 || data.Temperature >20 ){
        MotorStatusSet(ON);
    }
    else {
        MotorStatusSet(OFF);
    }
    //控制电机的转动




    }
}
static void ia1_example(void)
{
    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="ia1";
    if (osThreadNew((osThreadFunc_t)ia1_task,NULL,&attr) == NULL){
        printf("Failed to create ia1_task!\r\n");
    }
}
APP_FEATURE_INIT(ia1_example);
