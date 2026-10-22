#ifndef __I2C_H
#define __I2C_H

#include "stm32f10x.h"

/* I2C1 引脚：PB6(SCL), PB7(SDA) */
#define I2C1_SCL_PIN    GPIO_Pin_6
#define I2C1_SDA_PIN    GPIO_Pin_7
#define I2C1_PORT       GPIOB

/* 函数声明 */
void I2C1_Init(void);
int I2C1_SendBytes(uint8_t addr, const uint8_t *pData, uint16_t size);
int I2C1_ReceiveBytes(uint8_t addr, uint8_t *pBuffer, uint16_t size);

#endif

