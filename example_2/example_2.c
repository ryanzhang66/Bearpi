#include<stdio.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "ohos_types.h"
osThreadId_t the_oneID;   //任务一宏定义
osThreadId_t the_twoID;   //任务二宏定义

void the_one(void)
{
    printf("the_one is 1\r\n");
    osDelay(1);
    printf("the_one is go to\r\n");
    osThreadSuspend(the_oneID);
    printf("the_one ThreadSuspend success\r\n");
    osThreadTerminate(the_oneID); 
}

void the_two(void)
{
    for (int i=0; i<10; i++)
    {
        printf("the_two is 2\r\n");
    }
    printf("the_two osThreadSuspend success\r\n");
    osThreadResume(the_oneID);
    osThreadTerminate(the_twoID);
}

static void example(void)
{
    osThreadAttr_t attr;    //该类型用于初始化线程的各项配置(一个结构体吧？)

    attr.name="the_one";    //建立线程当中的第一个任务
    attr.attr_bits=0U;           //线程属性位
    attr.cb_mem=NULL;       //用户指定的控制块指针
    attr.cb_size=0U;        //用户指定的控制块大小，单位：字节
    attr.stack_mem=NULL;    //用户指定的线程栈指针
    attr.stack_size=1024*4;        //线程栈大小，单位：字节
    attr.priority=25;       //线程优先级

    //创建第一个任务
    //osThreadNew函数详解:https://blog.csdn.net/bdzyt/article/details/126618582
    the_oneID=osThreadNew((osThreadFunc_t)the_one, NULL, &attr);
    if (the_oneID == NULL) {  
        printf("Failed to create Thread1!\n");
    }

    //创建第二个任务
    attr.name="the_two";
    attr.priority=24;
    the_twoID=osThreadNew((osThreadFunc_t)the_two, NULL, &attr);
    if (the_twoID == NULL) {  
        printf("Failed to create Thread2!\n");
    }
}

APP_FEATURE_INIT(example);