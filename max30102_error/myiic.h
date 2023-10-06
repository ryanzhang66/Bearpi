#ifndef __MYIIC_H_
#define __MYIIC_H_

#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_i2c.h"

#define u8  unsigned char
#define u16 unsigned short

#define PIN_SCL		HI_IO_NAME_GPIO_4
#define PIN_SDA		HI_IO_NAME_GPIO_3
#define PIN_INT		HI_IO_NAME_GPIO_0

//IO方向设置
#define SDA_IN()  			hi_gpio_set_dir(PIN_SDA, HI_GPIO_DIR_IN);   
#define SDA_OUT() 			hi_gpio_set_dir(PIN_SDA, HI_GPIO_DIR_OUT);  

//IO操作函数	 
#define IIC_SCL(value)    	iic_scl(value)
#define IIC_SDA(value)    	iic_sda(value)
#define READ_SDA          	read_sda()


//IIC所有操作函数
void IIC_Init(void);                //初始化IIC的IO口				 
void IIC_Start(void);				//发送IIC开始信号
void IIC_Stop(void);	  			//发送IIC停止信号
void IIC_Send_Byte(u8 txd);			//IIC发送一个字节
u8 IIC_Read_Byte(unsigned char ack);//IIC读取一个字节
u8 IIC_Wait_Ack(void); 				//IIC等待ACK信号
void IIC_Ack(void);					//IIC发送ACK信号
void IIC_NAck(void);				//IIC不发送ACK信号

void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
void IIC_Read_One_Byte(u8 daddr,u8 addr,u8* data);

void IIC_WriteBytes(u8 WriteAddr,u8* data,u8 dataLength);
void IIC_ReadBytes(u8 deviceAddr, u8 writeAddr,u8* data,u8 dataLength);

void delay_ms(int32_t t);
void iic_scl(int value);
void iic_sda(int value);

#endif
