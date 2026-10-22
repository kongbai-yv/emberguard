#ifndef __SOFT_I2C_H
#define __SOFT_I2C_H

#include "stm32f10x.h"

/* 用户可自定义引脚 */
#define SOFT_I2C_SCL_PIN    GPIO_Pin_6
#define SOFT_I2C_SDA_PIN    GPIO_Pin_7
#define SOFT_I2C_PORT       GPIOB

void Soft_I2C_Init(void);
void Soft_I2C_Start(void);
void Soft_I2C_Stop(void);
uint8_t Soft_I2C_WaitAck(void);
void Soft_I2C_SendByte(uint8_t data);
uint8_t Soft_I2C_ReceiveByte(uint8_t ack);
int Soft_I2C_SendBytes(uint8_t addr, const uint8_t *pData, uint16_t size);
int Soft_I2C_ReceiveBytes(uint8_t addr, uint8_t *pBuffer, uint16_t size);

#endif