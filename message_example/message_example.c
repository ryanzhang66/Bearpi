#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"

osMessageQueueId_t message_id;
osStatus_t status;

typedef struct {
    char *name;
    uint8_t number;
}MSG;

MSG msg;

void message_Put(void)
{
    //(void)ptr;
    uint8_t num=0;
    msg.name="Hello BearPi-HM_Nano!";
    
    while(1){
        msg.number=num;
        status=osMessageQueuePut(message_id,&msg,0U,0U);
        num++;
        if (status != osOK){
            printf("Failed to create message_Put!\r\n");
        }
        osThreadYield();   //挂起任务
        osDelay(100U);
    }
}

void message_Get(void)
{
    while(1){
        uint32_t count;
        count = osMessageQueueGetCount(message_id);
        printf("message Queue count is %d\r\n",count);
        if (count > 14){
            osMessageQueueDelete(message_id);
            osMessageQueueDelete(message_id);
        }
        status=osMessageQueueGet(message_id,&msg,NULL,osWaitForever);
        if (status != osOK){
            printf("Failed to create message_Get!\r\n");
        }
        else {
            printf("message Get msg name:%s, number:%d\r\n",msg.name,msg.number);
        }
        osDelay(300U);
    }
}

static void message_example(void)
{
    message_id=osMessageQueueNew(16,100,NULL);   // (队列的节点数量 ,队列的单个节点空间大小 ,队列参数，若参数为NULL则使用默认配置)
    if (message_id == NULL){
        printf("Failed to create message!\r\n");
    }

    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;

    attr.name="Thread_1";
    attr.priority=25;
    if (osThreadNew((osThreadFunc_t)message_Put,NULL,&attr) == NULL){
        printf("Failed to create message_Put!\r\n");
    }

    attr.name="Thread_2";
    if (osThreadNew((osThreadFunc_t)message_Get,NULL,&attr) == NULL){
        printf("Failed to create message_Get!\r\n");
    }
    
}
APP_FEATURE_INIT(message_example);