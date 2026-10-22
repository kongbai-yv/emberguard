#ifndef __BSP_HCSR04_H
#define __BSP_HCSR04_H

#include "stm32f10x.h"

/* 引脚定义 */
#define HCSR04_TRIG_PORT    GPIOA
#define HCSR04_TRIG_PIN     GPIO_Pin_4
#define HCSR04_ECHO_PORT    GPIOA
#define HCSR04_ECHO_PIN     GPIO_Pin_8   // 复用为 TIM1_CH1

void HCSR04_Init(void);
float HCSR04_GetDistance(void);

#endif

