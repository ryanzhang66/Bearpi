#include "max30102.h"
#include "myiic.h"
#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

u8 max30102_Bus_Write(u8 Register_Address, u8 Word_Data)
{

	IIC_Start();
	IIC_Send_Byte(max30102_WR_address | I2C_WR);
	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}
	IIC_Send_Byte(Register_Address);
	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}
	IIC_Send_Byte(Word_Data);
	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}
	IIC_Stop();
	return 1;

#if 1
cmd_fail:

	IIC_Stop();
	printf("max30102_Bus_Write failed\r\n");
	return 0;
#endif
}

u8 max30102_Bus_Read(u8 Register_Address)
{
	u8 data;
	IIC_Start();
	IIC_Send_Byte(max30102_WR_address | I2C_WR);

	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}

	IIC_Send_Byte((uint8_t)Register_Address);
	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}

	IIC_Start();

	IIC_Send_Byte(max30102_WR_address | I2C_RD);

	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}

	{
		data = IIC_Read_Byte(0);

		IIC_NAck();
	}

	IIC_Stop();
	return data;

#if 1
cmd_fail:

	IIC_Stop();
	printf("max30102_Bus_Read failed\r\n");
	return 0;
#endif
}

void max30102_FIFO_ReadWords(u8 Register_Address, u16 Word_Data[][2], u8 count)
{
	u8 i = 0;
	u8 no = count;
	u8 data1, data2;
	IIC_Start();

	IIC_Send_Byte(max30102_WR_address | I2C_WR);

	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}

	IIC_Send_Byte((uint8_t)Register_Address);
	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}

	IIC_Start();

	IIC_Send_Byte(max30102_WR_address | I2C_RD);

	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}

	while (no)
	{
		data1 = IIC_Read_Byte(0);
		IIC_Ack();
		data2 = IIC_Read_Byte(0);
		IIC_Ack();
		Word_Data[i][0] = (((u16)data1 << 8) | data2);

		data1 = IIC_Read_Byte(0);
		IIC_Ack();
		data2 = IIC_Read_Byte(0);
		if (1 == no)
			IIC_NAck();
		else
			IIC_Ack();
		Word_Data[i][1] = (((u16)data1 << 8) | data2);

		no--;
		i++;
	}

	IIC_Stop();

#if 1
cmd_fail:

	IIC_Stop();
	printf("max30102_FIFO_ReadWords  failed\r\n");
#endif
}

void max30102_FIFO_ReadBytes(u8 Register_Address, u8 *Data)
{
	max30102_Bus_Read(REG_INTR_STATUS_1);
	max30102_Bus_Read(REG_INTR_STATUS_2);

	IIC_Start();

	IIC_Send_Byte(max30102_WR_address | I2C_WR);

	if (IIC_Wait_Ack() != 0)
	{

		goto cmd_fail;
	}

	IIC_Send_Byte((uint8_t)Register_Address);
	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}

	IIC_Start();

	IIC_Send_Byte(max30102_WR_address | I2C_RD);

	if (IIC_Wait_Ack() != 0)
	{
		goto cmd_fail;
	}
	Data[0] = IIC_Read_Byte(1);
	Data[1] = IIC_Read_Byte(1);
	Data[2] = IIC_Read_Byte(1);
	Data[3] = IIC_Read_Byte(1);
	Data[4] = IIC_Read_Byte(1);
	Data[5] = IIC_Read_Byte(0);
	IIC_Stop();

cmd_fail:
	IIC_Stop();
}

void max30102_init(void)
{
	printf("\r\n come in MAX30102 init\r\n");
	max30102_reset();

	max30102_Bus_Write(REG_INTR_ENABLE_1, 0xc0); // INTR setting
	max30102_Bus_Write(REG_INTR_ENABLE_2, 0x00);
	max30102_Bus_Write(REG_FIFO_WR_PTR, 0x00); //FIFO_WR_PTR[4:0]
	max30102_Bus_Write(REG_OVF_COUNTER, 0x00); //OVF_COUNTER[4:0]
	max30102_Bus_Write(REG_FIFO_RD_PTR, 0x00); //FIFO_RD_PTR[4:0]
	max30102_Bus_Write(REG_FIFO_CONFIG, 0x0f); //sample avg = 1, fifo rollover=false, fifo almost full = 17
	max30102_Bus_Write(REG_MODE_CONFIG, 0x03); //0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
	max30102_Bus_Write(REG_SPO2_CONFIG, 0x27); // SPO2_ADC range = 4096nA, SPO2 sample rate (100 Hz), LED pulseWidth (400uS)
	max30102_Bus_Write(REG_LED1_PA, 0x24);	   //Choose value for ~ 7mA for LED1
	max30102_Bus_Write(REG_LED2_PA, 0x24);	   // Choose value for ~ 7mA for LED2
	max30102_Bus_Write(REG_PILOT_PA, 0x7f);	   // Choose value for ~ 25mA for Pilot LED 
	printf("\r\n MAX30102 init over \r\n");
}

void max30102_reset(void)
{
	max30102_Bus_Write(REG_MODE_CONFIG, 0x40);
	max30102_Bus_Write(REG_MODE_CONFIG, 0x40);
}

void maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)
{

	IIC_Write_One_Byte(I2C_WRITE_ADDR, uch_addr, uch_data);
}

void maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)
{

	IIC_Read_One_Byte(I2C_WRITE_ADDR, uch_addr, puch_data);
}

void maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led)
{
	uint32_t un_temp;
	unsigned char uch_temp;
	char ach_i2c_data[6];
	*pun_red_led = 0;
	*pun_ir_led = 0;

	//read and clear status register
	maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_temp);
	maxim_max30102_read_reg(REG_INTR_STATUS_2, &uch_temp);

	IIC_ReadBytes(I2C_WRITE_ADDR, REG_FIFO_DATA, (u8 *)ach_i2c_data, 6);

	un_temp = (unsigned char)ach_i2c_data[0];
	un_temp <<= 16;
	*pun_red_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[1];
	un_temp <<= 8;
	*pun_red_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[2];
	*pun_red_led += un_temp;

	un_temp = (unsigned char)ach_i2c_data[3];
	un_temp <<= 16;
	*pun_ir_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[4];
	un_temp <<= 8;
	*pun_ir_led += un_temp;
	un_temp = (unsigned char)ach_i2c_data[5];
	*pun_ir_led += un_temp;
	*pun_red_led &= 0x03FFFF; //Mask MSB [23:18]
	*pun_ir_led &= 0x03FFFF;  //Mask MSB [23:18]
}
