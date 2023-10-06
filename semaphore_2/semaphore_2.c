#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"

osSemaphoreId_t ID;

void Thread_1(void)
{
    while(1){
        osSemaphoreRelease(ID);
        osSemaphoreRelease(ID);
        printf("Release success!\r\n");
        osDelay(100);
    }
    
}
void Thread_2(void)
{
    while(1){
        osSemaphoreAcquire(ID,osWaitForever);
        printf("Thread_2 Acquire success!\r\n");
        osDelay(1);
    }
}
void Thread_3(void)
{
    while(1){
        osSemaphoreAcquire(ID,osWaitForever);
        printf("Thread_3 Acquire success!\r\n");
        osDelay(1);
    }
}
void semphore (void)
{
    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;

    attr.name=Thread_1;
    if (osThreadNew((osThreadFunc_t)Thread_1,NULL,&attr)==NULL){
        printf("Failed to create Theard_1!\r\n");
    }

    attr.name=Thread_2;
    if (osThreadNew((osThreadFunc_t)Thread_2,NULL,&attr)==NULL){
        printf("Failed to create Thread_2!\r\n");
    }

    attr.name=Thread_3;
    if (osThreadNew((osThreadFunc_t)Thread_3,NULL,&attr)==NULL){
        printf("Failed to create Thread_3!\r\n");
    }

    ID=osSemaphoreNew(4,0,NULL);
    if (ID == NULL){
        printf("Failed to create ID!\r\n");
    }
}
APP_FEATURE_INIT(semphore);