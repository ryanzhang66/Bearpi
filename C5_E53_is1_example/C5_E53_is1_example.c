#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "E53_is1.h"
#include "cmsis_os2.h"
#include "ohos_init.h"
osEventFlagsId_t event_id;

static void Buzzer_event(void *arg)
{
    (void)arg;
    osEventFlagsSet(event_id,0x00000001U);
}

static void E53_is1_GpioStart(void)
{
    int ret;
    E53Is1Init();
    ret = E53Is1ReadData(Buzzer_event);
    if (ret != 0){
        printf("Failed!\r\n");
        return;
    }
    while(1){
        osEventFlagsWait(event_id, 0x00000001U, osFlagsWaitAny, osWaitForever);
        BeepStatusSet(ON);
        osDelay(300U);
        BeepStatusSet(OFF);
    }
}

static void E53_is1_example(void)
{
    event_id = osEventFlagsNew(NULL);
    if (event_id != NULL){
        printf("Failed to create event_id!\r\n");
    }

    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="E53_is1";

    if (osThreadNew((osThreadFunc_t)E53_is1_GpioStart,NULL,&attr) == NULL){
        printf("Failed to create E53_is1_GpioStart!\r\n");
    }
}
APP_FEATURE_INIT(E53_is1_example);