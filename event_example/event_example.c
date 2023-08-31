#include<stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"

#define FLAG_1 0x00000001U
#define FLAG_2 0x00000002U
#define FLAG_3 0x00000003U
osEventFlagsId_t event_id;

void Thread_1(void)
{
    while(1){
        osEventFlagsSet(event_id,FLAG_1);
        osEventFlagsSet(event_id,FLAG_2);
        osEventFlagsSet(event_id,FLAG_3);
        //暂停任务 
        osThreadYield();
        osDelay(100U); 
    }
}

void Thread_2(void)
{
    uint32_t flags;
    while(1){
        flags = osEventFlagsWait(event_id,FLAG_1|FLAG_2|FLAG_3,osFlagsWaitAll,osWaitForever);
        printf("Release falgs is %d\r\n",flags);
    }
}

static void event_example(void)
{
    event_id = osEventFlagsNew(NULL);
    if (event_id == NULL){
        printf("Fail to create eventid!\r\n");
    }

    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;

    attr.name=Thread_1;
    if (osThreadNew((osThreadFunc_t)Thread_1,NULL,&attr) == NULL){
        printf("Failed to create Thread_1!\r\n");
    }
    
    attr.name=Thread_2;
    if (osThreadNew((osThreadFunc_t)Thread_2,NULL,&attr) == NULL){
        printf("Failed to create Thread_2!\r\n");
    }
    
}

APP_FEATURE_INIT(event_example);