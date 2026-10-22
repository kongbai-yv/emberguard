#include "bsp_bmp180.h"
#include "si2c.h"
#include "delay.h"
#include "usart.h"
#include <math.h>

extern SI2C_TypeDef g_SI2C_BMP;

static struct {
    int16_t AC1, AC2, AC3;
    uint16_t AC4, AC5, AC6;
    int16_t B1, B2, MB, MC, MD;
} bmp180_cal;

static uint8_t BMP180_ReadByte(uint8_t reg) {
    uint8_t data;
    if (My_SI2C_SendBytes(&g_SI2C_BMP, BMP180_ADDR << 1, &reg, 1) != 0) return 0;
    if (My_SI2C_ReceiveBytes(&g_SI2C_BMP, BMP180_ADDR << 1, &data, 1) != 0) return 0;
    return data;
}

static uint16_t BMP180_ReadU16(uint8_t reg) {
    uint8_t buf[2];
    if (My_SI2C_SendBytes(&g_SI2C_BMP, BMP180_ADDR << 1, &reg, 1) != 0) return 0;
    if (My_SI2C_ReceiveBytes(&g_SI2C_BMP, BMP180_ADDR << 1, buf, 2) != 0) return 0;
    return (buf[0] << 8) | buf[1];
}

static int16_t BMP180_ReadS16(uint8_t reg) { return (int16_t)BMP180_ReadU16(reg); }

static void BMP180_WriteCmd(uint8_t cmd) {
    My_SI2C_SendBytes(&g_SI2C_BMP, BMP180_ADDR << 1, &cmd, 1);
}

static uint16_t BMP180_ReadUT(void) {
    uint8_t reg = BMP180_REG_DATA;
    uint8_t buf[2];
    BMP180_WriteCmd(BMP180_CMD_TEMP);
    Delay(15);   // 延长至15ms确保转换
    My_SI2C_SendBytes(&g_SI2C_BMP, BMP180_ADDR << 1, &reg, 1);
    My_SI2C_ReceiveBytes(&g_SI2C_BMP, BMP180_ADDR << 1, buf, 2);
    return (buf[0] << 8) | buf[1];
}

static uint32_t BMP180_ReadUP(uint8_t oss) {
    uint8_t reg = BMP180_REG_DATA;
    uint8_t buf[3];
    uint8_t cmd = BMP180_CMD_PRESS + (oss << 6);
    uint8_t wait = (oss == 0) ? 15 : 20;  // 延长等待
    BMP180_WriteCmd(cmd);
    Delay(wait);
    My_SI2C_SendBytes(&g_SI2C_BMP, BMP180_ADDR << 1, &reg, 1);
    My_SI2C_ReceiveBytes(&g_SI2C_BMP, BMP180_ADDR << 1, buf, 3);
    return (((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2]) >> (8 - oss);
}

int BMP180_Init(void) {
    uint8_t chip_id = BMP180_ReadByte(BMP180_REG_ID);
    My_USART_Printf(USART2, "Chip ID = 0x%02X\r\n", chip_id);
    if (chip_id != BMP180_CHIP_ID) return -1;
    bmp180_cal.AC1 = BMP180_ReadS16(BMP180_CAL_AC1);
    bmp180_cal.AC2 = BMP180_ReadS16(BMP180_CAL_AC2);
    bmp180_cal.AC3 = BMP180_ReadS16(BMP180_CAL_AC3);
    bmp180_cal.AC4 = BMP180_ReadU16(BMP180_CAL_AC4);
    bmp180_cal.AC5 = BMP180_ReadU16(BMP180_CAL_AC5);
    bmp180_cal.AC6 = BMP180_ReadU16(BMP180_CAL_AC6);
    bmp180_cal.B1  = BMP180_ReadS16(BMP180_CAL_B1);
    bmp180_cal.B2  = BMP180_ReadS16(BMP180_CAL_B2);
    bmp180_cal.MB  = BMP180_ReadS16(BMP180_CAL_MB);
    bmp180_cal.MC  = BMP180_ReadS16(BMP180_CAL_MC);
    bmp180_cal.MD  = BMP180_ReadS16(BMP180_CAL_MD);
    My_USART_Printf(USART2, "Calib OK\r\n");
    return 0;
}

int BMP180_ReadTemp(float *temp) {
    int32_t UT = BMP180_ReadUT();
    My_USART_Printf(USART2, "UT=%ld\r\n", UT);
    int32_t X1 = (((int32_t)UT - bmp180_cal.AC6) * bmp180_cal.AC5) >> 15;
    int32_t X2 = (bmp180_cal.MC << 11) / (X1 + bmp180_cal.MD);
    int32_t B5 = X1 + X2;
    *temp = (float)((B5 + 8) >> 4) / 10.0f;
    return 0;
}

int BMP180_ReadPressure(float *pressure) {
    int32_t UT = BMP180_ReadUT();
    int32_t UP = BMP180_ReadUP(0);
    My_USART_Printf(USART2, "UP=%ld\r\n", UP);
    int32_t X1 = (((int32_t)UT - bmp180_cal.AC6) * bmp180_cal.AC5) >> 15;
    int32_t X2 = (bmp180_cal.MC << 11) / (X1 + bmp180_cal.MD);
    int32_t B5 = X1 + X2;
    int32_t B6 = B5 - 4000;
    X1 = (bmp180_cal.B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = (bmp180_cal.AC2 * B6) >> 11;
    int32_t X3 = X1 + X2;
    int32_t B3 = (((bmp180_cal.AC1 * 4 + X3) << 0) + 2) / 4;
    X1 = (bmp180_cal.AC3 * B6) >> 13;
    X2 = (bmp180_cal.B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    uint32_t B4 = (bmp180_cal.AC4 * (uint32_t)(X3 + 32768)) >> 15;
    uint32_t B7 = ((uint32_t)UP - B3) * 50000;
    int32_t p;
    if (B7 < 0x80000000) p = (B7 * 2) / B4; else p = (B7 / B4) * 2;
    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p = p + ((X1 + X2 + 3791) >> 4);
    *pressure = p / 100.0f;
    return 0;
}

float BMP180_CalcAltitude(float pressure, float seaLevelhPa) {
    return 44330.0f * (1.0f - powf(pressure / seaLevelhPa, 1.0f / 5.255f));
}