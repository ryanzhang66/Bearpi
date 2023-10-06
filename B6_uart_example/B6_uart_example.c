#include<stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_uart.h"
#include "ohos_init.h"
static const char *data = "hello!";

static void uart_task(void)
{
    uint8_t uart_buf[1000]={0};
    uint8_t *uart_buf_ptr=uart_buf;
    uint8_t ret;
    
    IotUartAttribute uart_attr = {

        // baud_rate: 9600
        .baudRate = 9600,

        // data_bits: 8bits
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };

    ret = IoTUartInit(1,&uart_attr);
    if (ret != IOT_SUCCESS){
        printf("Failed!\r\n");
        return;
    }
    while(1){
        IoTUartWrite(1,(unsigned char*)data,strlen(data));
        IoTUartRead(1,uart_buf_ptr,1000);
        printf("Uart1 read data:%s\r\n", uart_buf_ptr);
        osDelay(100U);
    }
    

}

static void uart_example(void)
{
    osThreadAttr_t attr;
    attr.attr_bits=0U;
    attr.cb_mem=NULL;
    attr.cb_size=0U;
    attr.stack_mem=NULL;
    attr.stack_size=1024*4;
    attr.priority=25;
    attr.name="uart";

    if (osThreadNew((osThreadFunc_t)uart_task,NULL,&attr) == NULL){
        printf("Failed to create uart_task!\r\n");
    }

}
APP_FEATURE_INIT(uart_example);