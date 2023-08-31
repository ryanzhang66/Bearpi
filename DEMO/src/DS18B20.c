#include <stdio.h>
#include <unistd.h>
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "ds18b20.h"
#include "hi_time.h"

#define GPIO_DQ 7


IotGpioValue DS18B20_DQ_IN = {0};

/******************************************************
 * 函数名   ：DS18B20_IO_IN
 * 功能     ：设置端口为输入
 * 输入     ：无
 * 输出     ：无
 *******************************************************/
void DS18B20_IO_IN(void){
    //设置GPIO_DQ为输入模式
    IoTGpioSetDir(GPIO_DQ,IOT_GPIO_DIR_IN);
    //配置为浮空输入
    IoTGpioSetPull(GPIO_DQ,IOT_GPIO_PULL_NONE);
}

/******************************************************
 * 函数名   ：GPIO_GetInputValue
 * 功能     ：获取GPIO输入状态
 * 输入     ：id, *val
 * 输出     ：0/1
 *******************************************************/
uint8_t GPIO_GetInputValue(int id,IotGpioValue *val){
   IoTGpioGetInputVal(id,val);
   return *val;
}

/********************************************
 * 函数名   ：DS18B20_Init
 * 功能     ：初始化DS18B20
 * 输入     ：无
 * 输出     ：无
 ********************************************/
uint8_t DS18B20_Init(void){
    IoTGpioInit(7);
     //配置数据信号引脚GPIO_7（GPIO_DQ）
    IoTGpioSetFunc(GPIO_DQ,IOT_GPIO_FUNC_GPIO_7_GPIO);
    //设置GPIO_DQ为输出模式
    IoTGpioSetDir(GPIO_DQ,IOT_GPIO_DIR_OUT);
    //设置GPIO_DQ输出高电平
    IoTGpioSetOutputVal(GPIO_DQ,1);
    //复位DS18B20
    DS18B20_Res();
    //等待DS18B20的回应
    return DS18B20_Check();
}

/********************************************
 * 函数名   ：DS18B20_Res
 * 功能     ：复位DS18B20
 * 输入     ：无
 * 输出     ：无
 ********************************************/
void DS18B20_Res(void){
    //设置GPIO_DQ为输出模式
    IoTGpioSetDir(GPIO_DQ,IOT_GPIO_DIR_OUT);
    //拉低电平
    IoTGpioSetOutputVal(GPIO_DQ, 0);
    //延时750us
    usleep(750);
    //拉高电平
    IoTGpioSetOutputVal(GPIO_DQ, 1);
    //延时15us，为应答脉冲做准备
    hi_udelay(15);
}

/********************************************
 * 函数名   ：DS18B20_Check
 * 功能     ：将引脚设置为输入模式，等待DS18B20拉低电平的回应
 *           判断引脚低电平时间是否大于60us，小于240us
 *           返回应答结果
 * 输入     ：无
 * 输出     ：返回1 无响应 返回0 有响应
 * ******************************************/
uint8_t DS18B20_Check(void){
    uint8_t t=0;
    //配置GPIO_3为输入模式
    DS18B20_IO_IN();
    //等待应答，引脚一直为高，未被设备主动拉低。提供200us的超时时间
    while(GPIO_GetInputValue(GPIO_DQ,&DS18B20_DQ_IN) && t<200){
        t++;
        hi_udelay(1);
    }
    //若超时仍未响应，则返回1
    if(t>=200){
        printf("\nError: Waiting Response Overtime!Non-Respond!");
        return 1;
    }else{
        t=0;
    }
    //引脚响应，则判断引脚低电平时间是否大于60us，小于240us
    while((!GPIO_GetInputValue(GPIO_DQ,&DS18B20_DQ_IN)) && t<240){
        t++;
        hi_udelay(1);
    }
    //响应时间过长，应答失败
    if(t>=240){
        printf("\nError: Responsing Overtime! \n");
        return 1;
    }
    printf("\nResponse is OK !(t = %d)",t);
    return 0;
 
}

/********************************************
 * 函数名   ：DS18B20_Write_Byte
 * 功能     ：写一个字节到DS18B20
 * 输入     ：要写入的字节
 * 输出     ：无
 * ******************************************/
void DS18B20_Write_Byte(uint8_t byte){
    uint8_t i,test;
    //设置GPIO_3为输出模式
    IoTGpioSetDir(GPIO_DQ,IOT_GPIO_DIR_OUT);
    for(i=1;i<=8;i++){
        test = byte & 0x01;
        byte = byte >> 1;
        if(test){           //写1
            IoTGpioSetOutputVal(GPIO_DQ, 0);
            hi_udelay(2);
            IoTGpioSetOutputVal(GPIO_DQ, 1);
            hi_udelay(60);
        }else{              //写0
            IoTGpioSetOutputVal(GPIO_DQ, 0);
            hi_udelay(60);
            IoTGpioSetOutputVal(GPIO_DQ, 1);
            hi_udelay(2);
        }
    }
}

/********************************************
 * 函数名   ：DS18B20_Read_Byte
 * 功能     ：读取一个字节
 * 输入     ：无
 * 输出     ：1/0
 * ******************************************/
uint8_t DS18B20_Read_Bit(void){
    uint8_t t=0;
    //设置GPIO_3为输出模式
    IoTGpioSetDir(GPIO_DQ,IOT_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(GPIO_DQ, 0);      //拉低
    hi_udelay(2);
    IoTGpioSetOutputVal(GPIO_DQ, 1);       //释放
    //设置GPIO_3为输入模式
    DS18B20_IO_IN();
    hi_udelay(12);
    //获取输入电平值
    if(GPIO_GetInputValue(GPIO_DQ,&DS18B20_DQ_IN)){  
        hi_udelay(50);
        return 1;}
    else{
        hi_udelay(50);
        return 0;}
}

uint8_t DS18B20_Read_Byte(void){
    uint8_t i,j,data;
    data = 0;
    for(i=1;i<=8;i++){
        j=DS18B20_Read_Bit();
        data =(j<<7)|(data>>1);
        hi_udelay(2);
    }
    return data;
}

/********************************************
 * 函数名   ：DS18B20_Read_Temperature
 * 功能     ：温度读取
 * 输入     ：无
 * 输出     ：温度
 * ******************************************/
void DS18B20_Change_Temperature(void){
    DS18B20_Res();
    DS18B20_Check();
    DS18B20_Write_Byte(0xcc);
    DS18B20_Write_Byte(0x44);
}

float DS18B20_Read_Temperature(void){
    uint8_t TL,TH;
    uint16_t temp;
    short temperature;
    float result;
    DS18B20_Change_Temperature();
    DS18B20_Res();
    DS18B20_Check();
    DS18B20_Write_Byte(0xcc);   //skip rom
    DS18B20_Write_Byte(0xbe);   //covert
    TL = DS18B20_Read_Byte();   //lsb
    TH = DS18B20_Read_Byte();   //msb
   
    if(TH >7){
        TH = ~TH;
        TL = ~TL;
        temp = 0;
    }else{
        temp = 1;
    }

    temperature = TH;
    temperature = temperature << 8;
    temperature += TL;
    result = (float)temperature * 0.0625;
    printf("\nresult = %.3f",result); 

    if(temp){
        return result;
    }else{
        return -result;
    }
}
