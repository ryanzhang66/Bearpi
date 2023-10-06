#include<stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
osMutexId_t muxe_id;
void HIGH_Thread(void)
{
    osDelay(100U);
    while(1){
        osMutexAcquire(muxe_id,osWaitForever);
        printf("HIGH_Thread is running!\r\n");
        osDelay(300U);
        osMutexRelease(muxe_id);
    }
}
void MID_Thread(vboid)
{
    osDelay(100U);
    while(1){
        printf("MID_Thread is running!\r\n");
        osDelay(100U);
    }
}
void LOW_Thread(void)
{
    while(1){
        osMutexAcquire(muxe_id,osWaitForever);
        printf("LOW_Thread is running!\r\n");
        osDelay(300U);
        osMutexRelease(muxe_id);
    }
}
static void muxe_example(void)
{
    muxe_id=osMutexNew(NULL);
    if (muxe_id == NULL){
        printf("Failed to create Muxe!\r\n");
    }

    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;

    attr.name = "Thread_1";
    attr.priority = 24;     //高优先级
    if (osThreadNew((osThreadFunc_t)HIGH_Thread,NULL,&attr) == NULL){
        printf("Failed to create Thread_1!\r\n");
    }

    attr.name = "Thread_2";
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)MID_Thread,NULL,&attr) == NULL){
        printf("Failed to create Thread_2!\r\n");
    }

    attr.name = "Thread_3";
    attr.priority = 26;
    if (osThreadNew((osThreadFunc_t)LOW_Thread,NULL,&attr) == NULL){
        printf("Failed to create Thread_3!\r\n");
    }

}
APP_FEATURE_INIT(muxe_example);