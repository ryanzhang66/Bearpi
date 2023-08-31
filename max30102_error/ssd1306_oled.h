#ifndef __SSD1306_OLED_H__
#define __SSD1306_OLED_H__

#include <hi_i2c.h>
#include "ssd1306_oled.h"
#include "iot_gpio.h"
#include "iot_errno.h"

#define OLED_ADDRESS                0x78 //默认地址为0x78
#define OLED_ADDRESS_WRITE_CMD      0x00 // 0000000 写命令
#define OLED_ADDRESS_WRITE_DATA     0x40 //0100 0000(0x40)
#define HI_I2C_IDX_BAUDRATE         400000//400k
/*delay*/
#define SLEEP_1_MS                  1
#define SLEEP_10_MS                 10
#define SLEEP_20_MS                 20
#define SLEEP_30_MS                 30
#define SLEEP_50_MS                 50
#define SLEEP_100_MS                100
#define SLEEP_1S                    1000
#define SLEEP_6_S                   6000
#define DELAY_5_MS                  5000
#define DELAY_10_MS                 10000
#define DELAY_100_MS                100000
#define DELAY_250_MS                250000
#define DELAY_500_MS                500000
#define DELAY_1_s                   1000000
#define DELAY_5_s                   5000000
#define DELAY_6_s                   6000000
#define DELAY_10_s                  10000000
#define DELAY_30_s                  30000000
#define DEFAULT_TYPE                ((hi_u8)0)
#define INIT_TIME_COUNT             ((hi_u8)1)
#define TIME_PEROID_COUNT           10       
/*pwm duty*/
#define PWM_LOW_DUTY                1
#define PWM_SLOW_DUTY               1000
#define PWM_SMALL_DUTY              4000
#define PWM_LITTLE_DUTY             10000
#define PWM_DUTY                    50
#define PWM_MIDDLE_DUTY             40000
#define PWM_FULL_DUTY               65530
/*oled init*/
#define OLED_CLEAN_SCREEN           ((hi_u8)0x00)
/*ssd1306 register cmd*/
#define DISPLAY_OFF                 0xAE
#define SET_LOW_COLUMN_ADDRESS      0x00
#define SET_HIGH_COLUMN_ADDRESS     0x10
#define SET_START_LINE_ADDRESS      0x40
#define SET_PAGE_ADDRESS            0xB0
#define CONTRACT_CONTROL            0x81
#define FULL_SCREEN                 0xFF
#define SET_SEGMENT_REMAP           0xA1
#define NORMAL                      0xA6
#define SET_MULTIPLEX               0xA8
#define DUTY                        0x3F
#define SCAN_DIRECTION              0xC8
#define DISPLAY_OFFSET              0xD3
#define DISPLAY_TYPE                0x00
#define OSC_DIVISION                0xD5
#define DIVISION                    0x80
#define COLOR_MODE_OFF              0xD8
#define COLOR                       0x05
#define PRE_CHARGE_PERIOD           0xD9
#define PERIOD                      0xF1
#define PIN_CONFIGUARTION           0xDA
#define CONFIGUARTION               0x12
#define SET_VCOMH                   0xDB
#define VCOMH                       0x30
#define SET_CHARGE_PUMP_ENABLE      0x8D
#define PUMP_ENABLE                 0x14
#define TURN_ON_OLED_PANEL          0xAF

#define IOT_IO_NAME_GPIO9            (9)
#define IOT_GPIO_IDX_9               (9)

typedef struct {
    /** Pointer to the buffer storing data to send */
    unsigned char *sendBuf;
    /** Length of data to send */
    unsigned int  sendLen;
    /** Pointer to the buffer for storing data to receive */
    unsigned char *receiveBuf;
    /** Length of data received */
    unsigned int  receiveLen;
} IotI2cData;

typedef enum {
    IOT_I2C_IDX_0,
    IOT_I2C_IDX_1,
} IotI2cIdx;

typedef unsigned char IotU8_t;
typedef unsigned short IotU16_t;
typedef unsigned int IotU32_t;
typedef  int IotS32_t;

static IotU32_t i2c_write_byte(IotU8_t reg_addr, IotU8_t cmd);
static IotU32_t write_cmd(IotU8_t cmd);
static IotU32_t write_data(IotU8_t i2c_data);
IotU32_t oled_init(void);
void oled_set_position(IotU8_t x, IotU8_t y);
void oled_fill_screen(IotU8_t fii_data);
void oled_show_char(IotU8_t x, IotU8_t y, IotU8_t chr, IotU8_t char_size);
void oled_show_str(IotU8_t x, IotU8_t y, IotU8_t *chr, IotU8_t char_size);
IotU8_t  *float_to_string(double d, IotU8_t *str);
#endif