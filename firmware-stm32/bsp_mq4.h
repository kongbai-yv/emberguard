#ifndef __BSP_MQ4_H
#define __BSP_MQ4_H

#include "stm32f10x.h"

/* 引脚与ADC配置 */
#define MQ4_ADC_PORT            GPIOA
#define MQ4_ADC_PIN             GPIO_Pin_1
#define MQ4_ADC_CHANNEL         ADC_Channel_1
#define MQ4_ADC                 ADC1

/* 硬件参数（根据实际模块调整） */
#define MQ4_VREF                3300        // ADC参考电压(mV)
#define MQ4_RL_RESISTANCE       1000        // 负载电阻RL(Ω)，常见1kΩ或4.7kΩ
#define MQ4_RO_CLEAN_AIR_FACTOR 4.4f        // Rs/R0在洁净空气中的典型比值

/* 函数声明 */
void MQ4_Init(void);
uint16_t MQ4_ReadADC(void);
float MQ4_GetVoltage(void);
float MQ4_GetRs(void);
float MQ4_GetRatio(void);
float MQ4_GetPPM(void);
void MQ4_Calibrate(void);

#endif

