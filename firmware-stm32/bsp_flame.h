#ifndef __BSP_FLAME_H
#define __BSP_FLAME_H

#include "stm32f10x.h"

/* 数字输出（DO）引脚 */
#define FLAME_DO_PORT       GPIOB
#define FLAME_DO_PIN        GPIO_Pin_5

/* 模拟输出（AO）ADC配置 */
#define FLAME_AO_ADC        ADC1
#define FLAME_AO_CHANNEL    ADC_Channel_3
#define FLAME_AO_PORT       GPIOA
#define FLAME_AO_PIN        GPIO_Pin_3

/* 阈值定义 */
#define FLAME_AO_THRESHOLD  2000    // 模拟电压低于此值(mV)认为有火焰（反相传感器）

/* 函数声明 */
void Flame_Init(void);
uint8_t Flame_IsDetected(void);     // 返回1：检测到火焰（DO=0）
float Flame_GetIntensity(void);     // 返回火焰强度电压 (mV)

#endif
