#ifndef _BLOOD_H
#define _BLOOD_H                 // Device header
#include "algorithm.h"
#include "math.h"
#include "max30102_hello.h"
typedef enum
{
	BLD_NORMAL,		//����
	BLD_ERROR,		//������
	
}BloodState;//ѪҺ״̬

typedef struct
{
	int 		heart;		//��������
	float 			SpO2;			//Ѫ������
}BloodData;
typedef unsigned char           uint8_t;  
typedef unsigned short int      uint16_t;  
typedef unsigned int            uint32_t;  

typedef signed char             int8_t;   
typedef short int               int16_t;  
typedef int                     int32_t;

void blood_data_translate(void);
void blood_data_update(void);
void blood_Loop(void);

#endif


