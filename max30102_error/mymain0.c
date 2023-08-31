#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_watchdog.h"
#include "hi_gpio.h"
#include "i2c.h"
#include "ssd1306_oled.h"

#include "max30102.h"
#include "algorithm.h"

#include "myiic.h"

int max3012_interrupt(void)
{
	int val = {0};
	hi_gpio_get_input_val(PIN_INT, &val);
	return val;
}

#define MAX30102_INT max3012_interrupt()

uint32_t aun_ir_buffer[500];  //IR LED sensor data
int32_t n_ir_buffer_length;	  //data length
uint32_t aun_red_buffer[500]; //Red LED sensor data
int32_t n_sp02;				  //SPO2 value
int8_t ch_spo2_valid;		  //indicator to show if the SP02 calculation is valid
int32_t n_heart_rate;		  //heart rate value
int8_t ch_hr_valid;			  //indicator to show if the heart rate calculation is valid
uint8_t uch_dummy;

static int32_t pre_heartrate = 0;
static int32_t pre_n_sp02 = 0;

static char line[32] = {0};

#define MAX_BRIGHTNESS 255

#ifdef OLED_DISPLAY
void dis_DrawCurve(uint32_t *data, uint8_t x);
#endif

static void *heartrateTask(const char *arg)
{
	(void)arg;
	//variables to calculate the on-board LED brightness that reflects the heartbeats
	uint32_t un_min, un_max, un_prev_data;
	int i;
	int32_t n_brightness;
	float f_temp;
	uint8_t temp[6] = {0, 0, 0, 0, 0, 0};
	uint8_t dis_hr = 0, dis_spo2 = 0;

	IIC_Init();
	printf("\r\n IIC init \r\n");

	max30102_init();
	printf("\r\n MAX30102 init successfyl \r\n", __DATE__, __TIME__);
	oled_init();

	un_min = 0x3FFFF;
	un_max = 0;

	n_ir_buffer_length = 500; //buffer length of 100 stores 5 seconds of samples running at 100sps
							  //read the first 500 samples, and determine the signal range
	for (i = 0; i < n_ir_buffer_length; i++)
	{
		while (MAX30102_INT == 1)
			; //wait until the interrupt pin asserts
		max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
		aun_red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2]; // Combine values to get the actual number
		aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5];	 // Combine values to get the actual number

		if (un_min > aun_red_buffer[i])
			un_min = aun_red_buffer[i]; //update signal min
		if (un_max < aun_red_buffer[i])
			un_max = aun_red_buffer[i]; //update signal max
	}
	un_prev_data = aun_red_buffer[i];
	//calculate heart rate and SpO2 after first 500 samples (first 5 seconds of samples)
	maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);

	while (1)
	{
		i = 0;
		un_min = 0x3FFFF;
		un_max = 0;

		//dumping the first 100 sets of samples in the memory and shift the last 400 sets of samples to the top
		for (i = 100; i < 500; i++)
		{
			aun_red_buffer[i - 100] = aun_red_buffer[i];
			aun_ir_buffer[i - 100] = aun_ir_buffer[i];

			//update the signal min and max
			if (un_min > aun_red_buffer[i])
				un_min = aun_red_buffer[i];
			if (un_max < aun_red_buffer[i])
				un_max = aun_red_buffer[i];
		}
		//take 100 sets of samples before calculating the heart rate.
		for (i = 400; i < 500; i++)
		{
			un_prev_data = aun_red_buffer[i - 1];
			while (MAX30102_INT == 1)
				; //wait until the interrupt pin asserts
			max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
			aun_red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2]; // Combine values to get the actual number
			aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5];	 // Combine values to get the actual number

			if (aun_red_buffer[i] > un_prev_data)
			{
				f_temp = aun_red_buffer[i] - un_prev_data;
				f_temp /= (un_max - un_min);
				f_temp *= MAX_BRIGHTNESS;
				n_brightness -= (int)f_temp;
				if (n_brightness < 0)
					n_brightness = 0;
			}
			else
			{
				f_temp = un_prev_data - aun_red_buffer[i];
				f_temp /= (un_max - un_min);
				f_temp *= MAX_BRIGHTNESS;
				n_brightness += (int)f_temp;
				if (n_brightness > MAX_BRIGHTNESS)
					n_brightness = MAX_BRIGHTNESS;
			}
			//send samples and calculation result to terminal program through UART
			if (ch_hr_valid == 1 && n_heart_rate < 120) //**/ ch_hr_valid == 1 && ch_spo2_valid ==1 && n_heart_rate<120 && n_sp02<101
			{
				dis_hr = n_heart_rate;
				dis_spo2 = n_sp02;
			}
			else
			{
				dis_hr = 0;
				dis_spo2 = 0;
			}
			if (dis_spo2 == 0 && dis_hr == 0)
			{
			}
		}
		for (i = 0; i < 500; i++)
		{
			printf("0x%02x,", aun_ir_buffer[i]);
		}
		printf("\r\n");
		printf("--------0-0-----\r\n");
		for (i = 0; i < 500; i++)
		{
			printf("0x%02x,", aun_red_buffer[i]);
		}
		printf("\r\n");
		printf("--------0-0-----\r\n");
		maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
		printf("HR=%i, \r\n", n_heart_rate);
		printf("HRvalid=%i,\r\n ", ch_hr_valid);
		printf("SpO2=%i,\r\n ", n_sp02);
		printf("SPO2Valid=%i\r\n", ch_spo2_valid);
		oled_fill_screen(OLED_CLEAN_SCREEN);
		if (n_heart_rate == 999 || n_sp02 == 999 || n_heart_rate == -999 || n_sp02 == -999)
		{
			snprintf(line, sizeof(line), "HR: %d", pre_heartrate);
			oled_show_str(0, 1, line, 1);
			snprintf(line, sizeof(line), "SpO2: %d", pre_n_sp02);
			oled_show_str(0, 2, line, 1);
			sleep(1);
		}
		else
		{
			pre_heartrate = n_heart_rate;
			pre_n_sp02 = n_sp02;

			snprintf(line, sizeof(line), "HR: %d", n_heart_rate);
			oled_show_str(0, 1, line, 1);
			snprintf(line, sizeof(line), "SpO2: %d", n_sp02);
			oled_show_str(0, 2, line, 1);
			sleep(1);
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

	if (osThreadNew((osThreadFunc_t)heartrateTask, NULL, &attr) == NULL)
	{
		printf("[HeartrateDemo] Falied to create heartrateTask!\n");
	}
	printf("\r\n[HeartrateDemo] Succ to create heartrateTask!\n");
}

SYS_RUN(HeartrateDemo);
