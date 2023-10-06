/*
 * Copyright (c) 2020 HiHope Community.
 * Description: oled demo
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */
#include "ssd1306_oled.h"
#include "iot_i2c.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include <unistd.h>
#include "code_tab.h"

#define SEND_CMD_LEN                (2)
#define Max_Column                  (128)

/*
    *@bref   向ssd1306 屏幕寄存器写入命令
    *status 0：表示写入成功，否则失败
*/
static IotU32_t i2c_write_byte(IotU8_t reg_addr, IotU8_t cmd)
{
    IotU32_t status =0;
    IotI2cIdx id = HI_I2C_IDX_0;//I2C 0
    IotU8_t send_len =2;
    IotU8_t user_data = cmd;
    IotI2cData oled_i2c_cmd = { 0 };
    IotI2cData oled_i2c_write_cmd = { 0 };

    IotU8_t send_user_cmd [SEND_CMD_LEN] = {OLED_ADDRESS_WRITE_CMD,user_data};
    IotU8_t send_user_data [SEND_CMD_LEN] = {OLED_ADDRESS_WRITE_DATA,user_data};

    /*如果是写命令，发写命令地址0x00*/
    if (reg_addr == OLED_ADDRESS_WRITE_CMD) {
        oled_i2c_write_cmd.sendBuf = send_user_cmd;
        oled_i2c_write_cmd.sendLen = send_len;
        status = IoTI2cWrite(id, OLED_ADDRESS, oled_i2c_write_cmd.sendBuf, oled_i2c_write_cmd.sendLen);
        if (status != 0) {
            return status;
        }
    }
    /*如果是写数据，发写数据地址0x40*/
    else if (reg_addr == OLED_ADDRESS_WRITE_DATA) {
        oled_i2c_cmd.sendBuf = send_user_data;
        oled_i2c_cmd.sendLen = send_len;
        status = IoTI2cWrite(id, OLED_ADDRESS, oled_i2c_cmd.sendBuf, oled_i2c_cmd.sendLen);
        if (status != 0) {
            return status;
        }
    }

    return 0;
}

/*写命令操作*/
static IotU32_t write_cmd(IotU8_t cmd)//写命令
{
    IotU8_t status =0;
    /*写设备地址*/
	status = i2c_write_byte(OLED_ADDRESS_WRITE_CMD, cmd);
    if (status != 0) {
        return -1;
    }
}
/*写数据操作*/
static IotU32_t write_data(IotU8_t i2c_data)//写数据
{
    IotU8_t status =0;
    /*写设备地址*/
	status = i2c_write_byte(OLED_ADDRESS_WRITE_DATA, i2c_data);
    if (status != 0) {
        return -1;
    }
}
/* ssd1306 oled 初始化*/
IotU32_t oled_init(void)
{
    IotU32_t status;
    hi_udelay(DELAY_100_MS);//100ms  这里的延时很重要

    status = write_cmd(DISPLAY_OFF);//--display off
    if (status != 0) {
        return -1;
    }
    status = write_cmd(SET_LOW_COLUMN_ADDRESS);//---set low column address
    if (status != 0) {
        return -1;
    }
    status = write_cmd(SET_HIGH_COLUMN_ADDRESS);//---set high column address
    if (status != 0) {
        return -1;
    }
    status = write_cmd(SET_START_LINE_ADDRESS);//--set start line address
    if (status != 0) {
        return -1;
    }
    status = write_cmd(SET_PAGE_ADDRESS);//--set page address
    if (status != 0) {
        return -1;
    }
    status = write_cmd(CONTRACT_CONTROL);// contract control
    if (status != 0) {
        return -1;
    }
    status = write_cmd(FULL_SCREEN);//--128
    if (status != 0) {
        return -1;
    }  
    status= write_cmd(SET_SEGMENT_REMAP);//set segment remap
    if (status != 0) {
        return -1;
    } 
    status = write_cmd(NORMAL);//--normal / reverse
    if (status != 0) {
        return -1;
    }
    status =write_cmd(SET_MULTIPLEX);//--set multiplex ratio(1 to 64)
    if (status != 0) {
        return -1;
    }
    status = write_cmd(DUTY);//--1/32 duty
    if (status != 0) {
        return -1;
    }
    status = write_cmd(SCAN_DIRECTION);//Com scan direction
    if (status != 0) {
        return -1;
    }
    status = write_cmd(DISPLAY_OFFSET);//-set display offset
    if (status != 0) {
        return -1;
    }
    status = write_cmd(DISPLAY_TYPE);
    if (status != 0) {
        return -1;
    }
    status = write_cmd(OSC_DIVISION);//set osc division
    if (status != 0) {
        return -1;
    }
    status = write_cmd(DIVISION);
    if (status != 0) {
        return -1;
    }
    status = write_cmd(COLOR_MODE_OFF);//set area color mode off
    if (status != 0) {
        return -1;
    }
    status= write_cmd(COLOR);
    if (status != 0) {
        return -1;
    }
    status = write_cmd(PRE_CHARGE_PERIOD);//Set Pre-Charge Period
    if (status != 0) {
        return -1;
    }
    status = write_cmd(PERIOD);
    if (status != 0) {
        return -1;
    }
    status = write_cmd(PIN_CONFIGUARTION);//set com pin configuartion
    if (status != 0) {
        return -1;
    }
    status = write_cmd(CONFIGUARTION);
    if (status != 0) {
        return -1;
    }
    status = write_cmd(SET_VCOMH);//set Vcomh
    if (status != 0) {
        return -1;
    }
    status = write_cmd(VCOMH);
    if (status != 0) {
        return -1;
    }
    status = write_cmd(SET_CHARGE_PUMP_ENABLE);//set charge pump enable
    if (status != 0) {
        return -1;
    }
    status = write_cmd(PUMP_ENABLE);
    if (status != 0) {
        return -1;
    }
    status = write_cmd(TURN_ON_OLED_PANEL);//--turn on oled panel
    if (status != 0) {
        return -1;
    }
    return 0;
}
/*
    @bref set start position 设置起始点坐标
    @param IotU8_t x:write start from x axis
           IotU8_t y:write start from y axis
*/
void oled_set_position(IotU8_t x, IotU8_t y) 
{ 
    write_cmd(0xb0+y);
    write_cmd(((x&0xf0)>>4)|0x10);
    write_cmd(x&0x0f);
}
/*全屏填充*/
void oled_fill_screen(IotU8_t fii_data)
{
    IotU8_t m =0;
    IotU8_t n =0;

    for (m = 0; m < 8; m++) {
        write_cmd(0xb0+m);
        write_cmd(0x00);
        write_cmd(0x10);

        for (n = 0; n < 128; n++) {
            write_data(fii_data);
        }
    }
}
/*
    @bref Clear from a location
    @param IotU8_t fill_data: write data to screen register 
           IotU8_t line:write positon start from Y axis 
           IotU8_t pos :write positon start from x axis 
           IotU8_t len:write data len
*/
void oled_position_clean_screen(IotU8_t fill_data, IotU8_t line, IotU8_t pos, IotU8_t len)
{
    IotU8_t m =line;
    IotU8_t n =0;

    write_cmd(0xb0+m);
    write_cmd(0x00);
    write_cmd(0x10);

    for (n=pos;n<len;n++) {
        write_data(fill_data);
    }   
}
/*
    @bref 8*16 typeface
    @param IotU8_t x:write positon start from x axis 
           IotU8_t y:write positon start from y axis
           IotU8_t chr:write data
           IotU8_t char_size:select typeface
 */
void oled_show_char(IotU8_t x, IotU8_t y, IotU8_t chr, IotU8_t char_size)
{      	
	IotU8_t c=0;
    IotU8_t i=0;

    c = chr-' '; //得到偏移后的值	

    if (x > Max_Column-1) {
        x=0;
        y=y+2;
    }

    if (char_size ==16) {
        oled_set_position(x,y);	
        for(i=0;i<8;i++){
            write_data(F8X16[c*16+i]);
        }
        
        oled_set_position(x,y+1);
        for (i=0;i<8;i++) {
            write_data(F8X16[c*16+i+8]);
        }
        
    } else {	
        oled_set_position(x,y);
        for (i=0;i<6;i++) {
            write_data(F6x8[c][i]);
        }            
    }
}

/*
    @bref display string
    @param IotU8_t x:write positon start from x axis 
           IotU8_t y:write positon start from y axis
           IotU8_t *chr:write data
           IotU8_t char_size:select typeface
*/ 
void oled_show_str(IotU8_t x, IotU8_t y, IotU8_t *chr, IotU8_t char_size)
{
	IotU8_t j=0;

    if (chr == NULL) {
        printf("param is NULL,Please check!!!\r\n");
        return;
    }

	while (chr[j] != '\0') {
        oled_show_char(x, y, chr[j], char_size);
		x += 8;
		if (x>120) {
            x = 0;
            y += 2;
        }
		j++;
	}
}
/*小数转字符串
 *输入：double 小数
 *输出：转换后的字符串
*/
IotU8_t  *float_to_string(double d, IotU8_t *str)
{
	IotU8_t str1[40] = {0};
	int j = 0;
    int k = 0;
    int i = 0;

    if (str == NULL) {
        return;
    }

	i = (int)d;//浮点数的整数部分
	while (i > 0) {
		str1[j++] = i % 10 + '0';
		i = i / 10;
	}

	for (k = 0;k < j;k++) {
		str[k] = str1[j-1-k];//被提取的整数部分正序存放到另一个数组
	}
	str[j++] = '.';
 
	d = d - (int)d;//小数部分提取
	for (i = 0;i <1;i++) {
		d = d*10;
		str[j++] = (int)d + '0';
		d = d - (int)d;
	}
	while(str[--j] == '0');
	str[++j] = '\0';
	return str;
}

