#include "myiic.h"

#include "hi_gpio.h"
#include "hi_io.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_i2c.h"
#include <hi_time.h>

void iic_scl(int value)
{
	int val = (value == 0) ? 0 : 1;

	hi_gpio_set_ouput_val(PIN_SCL, val);
}

void iic_sda(int value)
{
	int val = (value == 0) ? 0 : 1;

	hi_gpio_set_ouput_val(PIN_SDA, val);
}

int read_sda(void)
{
	int val = {0};

	hi_gpio_get_input_val(PIN_SDA, &val);

	return val;
}

//初始化IIC
void IIC_Init(void)
{
	hi_u32 ret;
	hi_gpio_value gpio_val = HI_GPIO_VALUE1;

	ret = hi_gpio_init();
	if (ret != HI_ERR_SUCCESS)
	{
		printf("===== ERROR ===== gpio -> hi_gpio_init ret:%d\r\n", ret);
		return;
	}
	printf("----- gpio init success-----\r\n");

	//initialize the IIC pins
	IoTGpioInit(PIN_SCL);
	hi_io_set_func(PIN_SCL, HI_IO_FUNC_GPIO_4_I2C1_SCL);
	IoTGpioSetDir(PIN_SCL, HI_GPIO_DIR_OUT);

	IoTGpioInit(PIN_SDA);
	hi_io_set_func(PIN_SDA, HI_IO_FUNC_GPIO_3_I2C1_SDA);
	IoTGpioSetDir(PIN_SCL, HI_GPIO_DIR_OUT);

	IIC_SCL(1);
	IIC_SDA(1);

	ret = hi_io_set_func(PIN_INT, HI_IO_FUNC_GPIO_0_GPIO);
	if (ret != HI_ERR_SUCCESS)
	{
		printf("===== ERROR ===== gpio -> hi_io_set_func ret:%d\r\n", ret);
		return;
	}
	printf("----- io set func success-----\r\n");

	ret = hi_gpio_set_dir(PIN_INT, HI_GPIO_DIR_IN);
	if (ret != HI_ERR_SUCCESS)
	{
		printf("===== ERROR ===== gpio -> hi_gpio_set_dir ret:%d\r\n", ret);
		return;
	}
	printf("----- gpio set dir success! -----\r\n");

	// ret = hi_gpio_get_input_val(HI_GPIO_IDX_0, &gpio_val);
	// if (ret != HI_ERR_SUCCESS)
	// {
	// 	printf("===== ERROR ===== gpio -> hi_gpio_get_input_val ret:%d\r\n", ret);
	// 	return;
	// }
	// printf("----- gpio input val is:%d. -----\r\n", gpio_val);

	ret = hi_gpio_deinit();
	if (ret != HI_ERR_SUCCESS)
	{
		printf("===== ERROR ===== gpio -> hi_gpio_deinit ret:%d\r\n", ret);
		return;
	}
	printf("----- gpio deinit success-----\r\n");

	// //initialize the INT pin, input mode pin
	// IoTGpioInit(PIN_INT);
	// hi_io_set_func(PIN_INT, HI_IO_FUNC_GPIO_0_GPIO);
	// IoTGpioSetDir(PIN_INT, HI_GPIO_DIR_IN);
}

void delay_ms(int32_t t)
{
	hi_udelay(t * 1000);
}

void delay_us(int32_t t)
{
	hi_udelay(t);
}

//产生IIC起始信号
void IIC_Start(void)
{
	SDA_OUT(); //sda线输出
	IIC_SDA(1);
	IIC_SCL(1);
	delay_us(4);
	IIC_SDA(0); //START:when CLK is high,DATA change form high to low
	delay_us(4);
	IIC_SCL(0); //钳住I2C总线，准备发送或接收数据
}
//产生IIC停止信号
void IIC_Stop(void)
{
	SDA_OUT(); //sda线输出
	IIC_SCL(0);
	IIC_SDA(0); //STOP:when CLK is high DATA change form low to high
	delay_us(4);
	IIC_SCL(1);
	IIC_SDA(1); //发送I2C总线结束信号
	delay_us(4);
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime = 0;
	SDA_IN(); //SDA设置为输入
	IIC_SDA(1);
	delay_us(1);
	IIC_SCL(1);
	delay_us(1);

#if 1
	while (READ_SDA)
	{
		ucErrTime++;
		if (ucErrTime > 250)
		{
			IIC_Stop();
			//printf("IIC_Wait_Ack erro\r\n");
			return 1;
		}
	}
#else
	ucErrTime++;
	delay_us(3);
#endif

	IIC_SCL(0); //时钟输出0
	return 0;
}
//产生ACK应答
void IIC_Ack(void)
{
	IIC_SCL(0);
	SDA_OUT();
	IIC_SDA(0);
	delay_us(2);
	IIC_SCL(1);
	delay_us(2);
	IIC_SCL(0);
}
//不产生ACK应答
void IIC_NAck(void)
{
	IIC_SCL(0);
	SDA_OUT();
	IIC_SDA(1);
	delay_us(2);
	IIC_SCL(1);
	delay_us(2);
	IIC_SCL(0);
}
//IIC发送一个字节
//返回从机有无应答
//1，有应答
//0，无应答
void IIC_Send_Byte(u8 txd)
{
	u8 t;
	SDA_OUT();
	IIC_SCL(0); //拉低时钟开始数据传输
	for (t = 0; t < 8; t++)
	{
		IIC_SDA((txd & 0x80) >> 7);
		txd <<= 1;
		delay_us(2); //对TEA5767这三个延时都是必须的
		IIC_SCL(1);
		delay_us(2);
		IIC_SCL(0);
		delay_us(2);
	}
}
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK
u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i, receive = 0;
	SDA_IN(); //SDA设置为输入
	for (i = 0; i < 8; i++)
	{
		IIC_SCL(0);
		delay_us(2);
		IIC_SCL(1);
		receive <<= 1;
		if (READ_SDA)
			receive++;
		delay_us(1);
	}
	if (!ack)
		IIC_NAck(); //发送nACK
	else
		IIC_Ack(); //发送ACK
	return receive;
}

void IIC_Write_Bytes(u8 WriteAddr, u8 *data, u8 dataLength)
{
	u8 i;
	IIC_Start();

	IIC_Send_Byte(WriteAddr); //发送写命令
	IIC_Wait_Ack();

	for (i = 0; i < dataLength; i++)
	{
		IIC_Send_Byte(data[i]);
		IIC_Wait_Ack();
	}
	IIC_Stop(); //产生一个停止条件
	delay_ms(10);
}

void IIC_Read_Bytes(u8 deviceAddr, u8 writeAddr, u8 *data, u8 dataLength)
{
	u8 i;
	IIC_Start();

	IIC_Send_Byte(deviceAddr); //发送写命令
	IIC_Wait_Ack();
	IIC_Send_Byte(writeAddr);
	IIC_Wait_Ack();
	IIC_Send_Byte(deviceAddr | 0X01); //进入接收模式
	IIC_Wait_Ack();

	for (i = 0; i < dataLength - 1; i++)
	{
		data[i] = IIC_Read_Byte(1);
	}
	data[dataLength - 1] = IIC_Read_Byte(0);
	IIC_Stop(); //产生一个停止条件
	delay_ms(10);
}

void IIC_Read_One_Byte(u8 daddr, u8 addr, u8 *data)
{
	IIC_Start();

	IIC_Send_Byte(daddr); //发送写命令
	IIC_Wait_Ack();
	IIC_Send_Byte(addr); //发送地址
	IIC_Wait_Ack();
	IIC_Start();
	IIC_Send_Byte(daddr | 0X01); //进入接收模式
	IIC_Wait_Ack();
	*data = IIC_Read_Byte(0);
	IIC_Stop(); //产生一个停止条件
}

void IIC_Write_One_Byte(u8 daddr, u8 addr, u8 data)
{
	IIC_Start();

	IIC_Send_Byte(daddr); //发送写命令
	IIC_Wait_Ack();
	IIC_Send_Byte(addr); //发送地址
	IIC_Wait_Ack();
	IIC_Send_Byte(data); //发送字节
	IIC_Wait_Ack();
	IIC_Stop(); //产生一个停止条件
	delay_ms(10);
}
