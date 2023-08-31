#ifndef __MY_OLED_H
#define __MY_OLED_H

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int


u32 my_i2c_write(WifiIotI2cIdx id, u16 device_addr, u32 send_len);
void Write_IIC_Command(unsigned char IIC_Command);
void Write_IIC_Data(unsigned char IIC_Data);
void OLED_WR_Byte(unsigned dat,unsigned cmd);
void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y);
void OLED_ClearPoint(u8 x,u8 y);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1);
void OLED_ShowString(u8 x,u8 y,char *chr,u8 size1);
u32 OLED_Pow(u8 m,u8 n);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1);
void OLED_WR_BP(u8 x,u8 y);
void OLED_init(void);
void my_oled_demo(void);

#endif