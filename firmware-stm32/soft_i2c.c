#include "soft_i2c.h"
#include "delay.h"

#define SCL_H() GPIO_SetBits(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN)
#define SCL_L() GPIO_ResetBits(SOFT_I2C_PORT, SOFT_I2C_SCL_PIN)
#define SDA_H() GPIO_SetBits(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN)
#define SDA_L() GPIO_ResetBits(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN)
#define SDA_READ() GPIO_ReadInputDataBit(SOFT_I2C_PORT, SOFT_I2C_SDA_PIN)

void Soft_I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = SOFT_I2C_SCL_PIN | SOFT_I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SOFT_I2C_PORT, &GPIO_InitStructure);
    SCL_H(); SDA_H();
}

void Soft_I2C_Start(void)
{
    SDA_H(); SCL_H(); DelayUs(5);
    SDA_L(); DelayUs(5); SCL_L();
}

void Soft_I2C_Stop(void)
{
    SDA_L(); SCL_H(); DelayUs(5);
    SDA_H(); DelayUs(5);
}

uint8_t Soft_I2C_WaitAck(void)
{
    uint8_t ack;
    SDA_H(); SCL_H(); DelayUs(5);
    ack = SDA_READ();
    SCL_L();
    return ack ? 1 : 0;
}

void Soft_I2C_SendByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80) SDA_H(); else SDA_L();
        data <<= 1;
        SCL_H(); DelayUs(5); SCL_L();
    }
    Soft_I2C_WaitAck();
}

uint8_t Soft_I2C_ReceiveByte(uint8_t ack)
{
    uint8_t data = 0;
    SDA_H();
    for (uint8_t i = 0; i < 8; i++)
    {
        data <<= 1;
        SCL_H(); DelayUs(5);
        if (SDA_READ()) data |= 0x01;
        SCL_L();
    }
    if (ack) SDA_L(); else SDA_H();
    SCL_H(); DelayUs(5); SCL_L();
    SDA_H();
    return data;
}

int Soft_I2C_SendBytes(uint8_t addr, const uint8_t *pData, uint16_t size)
{
    Soft_I2C_Start();
    Soft_I2C_SendByte(addr << 1);
    for (uint16_t i = 0; i < size; i++) Soft_I2C_SendByte(pData[i]);
    Soft_I2C_Stop();
    return 0;
}

int Soft_I2C_ReceiveBytes(uint8_t addr, uint8_t *pBuffer, uint16_t size)
{
    if (size == 0) return 0;
    Soft_I2C_Start();
    Soft_I2C_SendByte((addr << 1) | 0x01);
    for (uint16_t i = 0; i < size - 1; i++) pBuffer[i] = Soft_I2C_ReceiveByte(1);
    pBuffer[size - 1] = Soft_I2C_ReceiveByte(0);
    Soft_I2C_Stop();
    return 0;
}