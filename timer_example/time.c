#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
void Time1back(void)
{
    printf("This is BearPi Timer1Callback!\n");
}
void Time2back (void)
{
    printf("This is BearPi Timer2Callback!\n");
}
static void time_example(void)
{
    osTimerId_t ID1,ID2;
    uint32_t timerDelay;   //uint32_t == (unsigned int) 32B==4字节
    osStatus_t status;

    ID1=osTimerNew(Time1back, osTimerPeriodic, NULL, NULL);  //(任务函数名，定时器类型，仅限于osTimerOnce或osTimerPeriodic，定时器回调函数参数，定时器属性，当前不支持该参数，参数可为NULL )
    if (ID1 != NULL)
    {
        timerDelay=100U; //（100U == 1S）

        status= osTimerStart(ID1,timerDelay);  
        // 定时器开始运行函数 osOK，表示执行成功。
        //  osErrorParameter，表示参数错误。
        //  osErrorResource，表示其他错误。
        //  osErrorISR，表示在中断中调用本函数。
        //osTimerStop,osTimerDelete也是一样。
        if (status != osOK) {
            printf("Failed to start Timer1!\n");
        }
    }

    ID2=osTimerNew(Time2back, osTimerPeriodic, NULL, NULL);
    if (ID2 != NULL)
    {
        timerDelay=300U;

        status= osTimerStart(ID2,timerDelay);
        if (status != osOK) {
            printf("Failed to start Timer2!\n");
        }

    }
}

APP_FEATURE_INIT(time_example);