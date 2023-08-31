#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "iot_watchdog.h"
#include "cmsis_os2.h"
#include "iot_errno.h" //==IOT_SUCCESS =0  

#include "hi_io.h"   //上拉、复用
#include "hi_gpio.h" //hi_gpio_set_dir()、hi_gpio_set(get)_output(input)_val()
#include "iot_gpio.h"//gpioInit
#include "hi_time.h"
#include "hi_i2c.h"
#include "max30102.h"

#define MAX_SDA_IO0  0
#define MAX_SCL_IO1  1  
#define MAX_I2C_IDX  1  //hi3861 i2c-1
#define MAX_I2C_BAUDRATE 400*1000 //iic work frequency 400k (400*1000) 

#define MAX30102_WR_address 0xAE
#define I2C_WRITE_ADDR 0xAE
#define I2C_READ_ADDR 0xAF

#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
#define REG_PILOT_PA 0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12
#define REG_TEMP_INTR 0x1F
#define REG_TEMP_FRAC 0x20
#define REG_TEMP_CONFIG 0x21
#define REG_PROX_INT_THRESH 0x30
#define REG_REV_ID 0xFE //get max30102 infomation 
#define REG_PART_ID 0xFF

uint32_t *pun_red_led;
uint32_t *pun_ir_led;

static void *heartrateTask(const char *arg)
{
    IoTGpioInit(MAX_SDA_IO0);
	IoTGpioInit(MAX_SCL_IO1);
    hi_io_set_func(MAX_SDA_IO0, HI_IO_FUNC_GPIO_0_I2C1_SDA);
    hi_io_set_func(MAX_SCL_IO1, HI_IO_FUNC_GPIO_1_I2C1_SCL);
    hi_i2c_init(MAX_I2C_IDX, MAX_I2C_BAUDRATE);
	IoTGpioInit(9);
    IoTGpioSetDir(9,IOT_GPIO_DIR_OUT);
    max30102_init();
    while(1)
    {
        maxim_max30102_read_fifo(&pun_red_led, &pun_ir_led);
        printf("--------------------------\r\n");
        printf("red:%d\r\n",pun_red_led);
        printf("ir:%d\r\n",pun_ir_led);
        printf("--------------------------");
        osDelay(10U);
        int i;
        i=MAX30102_INTPin_Read();
        printf("----------------------------------------------------------------------------------%d",i);
    }
    
}

static void HeartrateDemo(void)
{
    osThreadAttr_t attr;

	IoTWatchDogDisable();

    attr.name = "heartrateTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;

    if (osThreadNew(heartrateTask, NULL, &attr) == NULL) {
        printf("[HeartrateDemo] Falied to create heartrateTask!\n");
    }
    printf("\r\n[HeartrateDemo] Succ to create heartrateTask!\n");
}
 SYS_RUN(HeartrateDemo);