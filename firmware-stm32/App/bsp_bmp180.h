#ifndef __BSP_BMP180_H
#define __BSP_BMP180_H

#include "stm32f10x.h"

/* BMP180 7位地址（右对齐），软I2C函数会自动左移 */
#define BMP180_ADDR         0x77

/* 寄存器定义 */
#define BMP180_REG_ID        0xD0
#define BMP180_REG_CTRL      0xF4
#define BMP180_REG_DATA      0xF6
#define BMP180_CHIP_ID       0x55

/* 测量命令 */
#define BMP180_CMD_TEMP      0x2E
#define BMP180_CMD_PRESS     0x34

/* 校准系数寄存器 */
#define BMP180_CAL_AC1       0xAA
#define BMP180_CAL_AC2       0xAC
#define BMP180_CAL_AC3       0xAE
#define BMP180_CAL_AC4       0xB0
#define BMP180_CAL_AC5       0xB2
#define BMP180_CAL_AC6       0xB4
#define BMP180_CAL_B1        0xB6
#define BMP180_CAL_B2        0xB8
#define BMP180_CAL_MB        0xBA
#define BMP180_CAL_MC        0xBC
#define BMP180_CAL_MD        0xBE

/* 函数声明 */
int BMP180_Init(void);
int BMP180_ReadTemp(float *temp);
int BMP180_ReadPressure(float *pressure);
float BMP180_CalcAltitude(float pressure, float seaLevelhPa);

#endif