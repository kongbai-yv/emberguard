#ifndef __BSP_SHT11_H
#define __BSP_SHT11_H

#include "stm32f10x.h"

/* 引脚定义 (使用空闲的 PB12 和 PB13) */
#define SHT11_SCK_PORT      GPIOB
#define SHT11_SCK_PIN       GPIO_Pin_12
#define SHT11_DATA_PORT     GPIOB
#define SHT11_DATA_PIN      GPIO_Pin_13

/* 传感器命令定义 */
#define SHT11_CMD_TEMP      0x03    // 测量温度
#define SHT11_CMD_HUMI      0x05    // 测量湿度
#define SHT11_CMD_WR_REG    0x06    // 写状态寄存器
#define SHT11_CMD_RD_REG    0x07    // 读状态寄存器
#define SHT11_CMD_SOFT_RST  0x1E    // 软复位

/* 函数声明 */
void SHT11_Init(void);
int SHT11_Read(float *temp, float *humi);

#endif

