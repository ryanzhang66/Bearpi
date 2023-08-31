#ifndef _DS18B20_H_
#define _DS18B20_H_

#include <stdio.h>
#include "cmsis_os2.h"

uint8_t DS18B20_Init(void);
void DS18B20_Res(void);
uint8_t DS18B20_Check(void);
void DS18B20_Write_Byte(uint8_t byte);
uint8_t DS18B20_Read_Byte(void);
float DS18B20_Read_Temperature(void);

#endif /* _DS18B20_H_ */
