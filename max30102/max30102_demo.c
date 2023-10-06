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

#include "max30102_hello.h"
#include "algorithm.h"
#include "blood.h"           

/*
uint32_t aun_ir_buffer[500];   //IR LED sensor data
int32_t n_ir_buffer_length=500;    //data length
uint32_t aun_red_buffer[500];  //Red LED sensor data
int32_t n_sp02; //SPO2 value
int8_t ch_spo2_valid;   //indicator to show if the SP02 calculation is valid
int32_t n_heart_rate;   //heart rate value
int8_t  ch_hr_valid;
*/

uint8_t IR_channel_data, RED_channel_data;
uint8_t pun_red_led, pun_ir_led;
uint16_t fifo_red;
uint16_t fifo_ir;

static void *heartrateTask(const char *arg)
{ 
	(void)arg;
	

	IoTGpioInit(MAX_SDA_IO0);
	IoTGpioInit(MAX_SCL_IO1);
	hi_io_set_func(MAX_SDA_IO0, HI_IO_FUNC_GPIO_0_I2C1_SDA);
    hi_io_set_func(MAX_SCL_IO1, HI_IO_FUNC_GPIO_1_I2C1_SCL);
    hi_i2c_init(MAX_I2C_IDX, MAX_I2C_BAUDRATE);
	IoTGpioInit(9);
    IoTGpioSetDir(9,IOT_GPIO_DIR_OUT);   //==载板led初始化

    //成功的话，可读取ID REG_PART_ID = %d \n
	max30102_init();  //看输出Faild 还是Successful
	

	while(1)
	{/*
		max30102_FIFO_Read_Data( & RED_channel_data, & IR_channel_data);
        maxim_max30102_read_fifo(&pun_red_led, &pun_ir_led);		
		IoTGpioSetOutputVal(9,0); 
        usleep(300000);  
        IoTGpioSetOutputVal(9,1); 
        usleep(300000); 
        fifo_red = pun_red_led;
        fifo_ir = pun_ir_led;
        printf("——————————————————\n");
		/*
		printf("HR=%i, \r\n", n_heart_rate); 
		printf("HRvalid=%i,\r\n ", ch_hr_valid);
		printf("SpO2=%i,\r\n ", n_sp02);
		printf("SPO2Valid=%i\r\n", ch_spo2_valid);	*/	
        for(int i = 0;i < 128;i++) 
            {
                //while(MAX30102_INTPin_Read()==0)
                //{
                    //¶ÁÈ¡FIFO
                    max30102_FIFO_Read_Data( &pun_red_led,  &pun_ir_led);
                    //maxim_max30102_read_fifo(&pun_red_led, &pun_ir_led);
                    fifo_red = pun_red_led;
                    fifo_ir = pun_ir_led;
                //}
            }
            while (1)
                {
                    blood_Loop();				//»ñÈ¡ÑªÑõºÍÐÄÂÊ
                }
    }
    return NULL;
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
