#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

// 修改这里可以更换引脚
#define DHT11_PORT       GPIOB
#define DHT11_PIN       GPIO_Pin_12
#define DHT11_RCC        RCC_APB2Periph_GPIOA
// 引脚操作宏
#define DHT11_OUT_HIGH() GPIO_SetBits(DHT11_PORT, DHT11_PIN)
#define DHT11_OUT_LOW()  GPIO_ResetBits(DHT11_PORT, DHT11_PIN)
#define DHT11_IN()       GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)

// 函数声明
void DHT11_Init(void);
uint8_t DHT11_ReadData(uint8_t *humi, uint8_t *temp);

#endif

