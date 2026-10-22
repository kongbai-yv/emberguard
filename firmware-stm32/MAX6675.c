#include "MAX6675.h"
#include "delay.h"

/**
 * @brief 初始化 MAX6675 所用 GPIO（软件 SPI）
 */
void MAX6675_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 GPIOA 和 GPIOB 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* 配置 SCK (PB8) 和 CS (PB14) 为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = MAX6675_SCK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MAX6675_SCK_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = MAX6675_CS_PIN;
    GPIO_Init(MAX6675_CS_PORT, &GPIO_InitStructure);

    /* 配置 MISO (PA6) 为浮空输入 */
    GPIO_InitStructure.GPIO_Pin = MAX6675_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(MAX6675_MISO_PORT, &GPIO_InitStructure);

    /* 初始状态：SCK 低电平，CS 高电平 */
    MAX6675_SCK_L();
    MAX6675_CS_H();
}

/**
 * @brief 通过软件 SPI 读取 16 位原始数据
 */
static uint16_t MAX6675_ReadRaw(void)
{
    uint16_t data = 0;
    uint8_t i;

    MAX6675_CS_L();             // 使能片选
    DelayUs(10);                // 等待稳定

    for (i = 0; i < 16; i++)
    {
        MAX6675_SCK_H();        // 时钟上升沿
        DelayUs(1);
        data <<= 1;
        if (MAX6675_MISO_READ())
            data |= 0x01;
        MAX6675_SCK_L();        // 时钟下降沿
        DelayUs(1);
    }

    MAX6675_CS_H();             // 禁用片选
    DelayUs(1);

    return data;
}

/**
 * @brief 读取温度值（℃），若热电偶断开返回 -273.15
 */
float MAX6675_ReadTemperature(void)
{
    uint16_t raw = MAX6675_ReadRaw();

    if (raw & 0x04)             // D2 为热电偶断开标志
        return -273.15f;

    raw >>= 3;                  // 右移3位得到12位温度数据
    return raw * 0.25f;
}

/**
 * @brief 检查热电偶是否连接
 * @return 1:已连接, 0:断开
 */
uint8_t MAX6675_IsThermocoupleConnected(void)
{
    uint16_t raw = MAX6675_ReadRaw();
    return (raw & 0x04) ? 0 : 1;
}

