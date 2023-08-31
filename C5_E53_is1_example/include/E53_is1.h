#ifndef __E53_IS1_H__
#define __E53_IS1_H__

#define WIFI_IOT_IO_NAME_GPIO_7 7
#define WIFI_IOT_IO_NAME_GPIO_8 8
#define WIFI_IOT_PWM_PORT_PWM1 1
#define WIFI_IOT_IO_FUNC_GPIO_8_PWM1_OUT 5
#define WIFI_IOT_IO_FUNC_GPIO_7_GPIO 0
#define PWM_DUTY 50
#define PWM_FREQ 4000

typedef void (*E53IS1CallbackFunc) (char *arg);   //typedef的功能是定义新的类型。第一句就是定义了一种PTRFUN的类型，
                                                  //并定义这种类型为指向某种函数的指针，这种函数以一个int为参数并返
                                                  //回char类型。后面就可以像使用int,char一样使用PTRFUN了。

typedef enum {
    OFF = 0,
    ON
} E53IS1Status;        //  enum 可选标签{ 内容.....}可选定义变量定义；

void E53Is1Init(void);
int E53Is1ReadData(E53IS1CallbackFunc func);
void BeepStatusSet(E53IS1Status status);

#endif