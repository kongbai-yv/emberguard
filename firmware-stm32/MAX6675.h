#ifndef __MAX6675_H
#define __MAX6675_H

#include "stm32f10x.h"

/* 引脚定义：SCK 改为 PB8，MISO 保持 PA6，CS 保持 PB14 */
#define MAX6675_SCK_PORT    GPIOB
#define MAX6675_SCK_PIN     GPIO_Pin_8

#define MAX6675_MISO_PORT   GPIOA
#define MAX6675_MISO_PIN    GPIO_Pin_6

#define MAX6675_CS_PORT     GPIOB
#define MAX6675_CS_PIN      GPIO_Pin_14

/* 控制宏 */
#define MAX6675_SCK_H()     GPIO_SetBits(MAX6675_SCK_PORT, MAX6675_SCK_PIN)
#define MAX6675_SCK_L()     GPIO_ResetBits(MAX6675_SCK_PORT, MAX6675_SCK_PIN)
#define MAX6675_MISO_READ() GPIO_ReadInputDataBit(MAX6675_MISO_PORT, MAX6675_MISO_PIN)
#define MAX6675_CS_H()      GPIO_SetBits(MAX6675_CS_PORT, MAX6675_CS_PIN)
#define MAX6675_CS_L()      GPIO_ResetBits(MAX6675_CS_PORT, MAX6675_CS_PIN)

/* 函数声明 */
void MAX6675_Init(void);
float MAX6675_ReadTemperature(void);
uint8_t MAX6675_IsThermocoupleConnected(void);

#endif

