#include "bsp_flame.h"
#include "delay.h"

/**
 * @brief 初始化火焰传感器引脚
 *        - DO (PB5) 为上拉输入
 *        - AO (PA3) 为模拟输入，并配置ADC1
 */
void Flame_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* DO (PB5) 上拉输入 */
    GPIO_InitStructure.GPIO_Pin = FLAME_DO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(FLAME_DO_PORT, &GPIO_InitStructure);

    /* AO (PA3) 模拟输入（不重新初始化ADC1） */
    GPIO_InitStructure.GPIO_Pin = FLAME_AO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(FLAME_AO_PORT, &GPIO_InitStructure);
}
/**
 * @brief 检测火焰是否存在（基于DO引脚）
 * @return 1：有火焰（DO为低电平），0：无火焰
 */
uint8_t Flame_IsDetected(void)
{
    // 根据你的测试，有光时DO为高电平，无光时为低电平
    return (GPIO_ReadInputDataBit(FLAME_DO_PORT, FLAME_DO_PIN) == Bit_SET);
}

/**
 * @brief 读取AO通道的原始ADC值（0~4095）
 */
uint16_t Flame_ReadADC(void)
{
    ADC_RegularChannelConfig(FLAME_AO_ADC, FLAME_AO_CHANNEL, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(FLAME_AO_ADC, ENABLE);
    while(ADC_GetFlagStatus(FLAME_AO_ADC, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(FLAME_AO_ADC);
}

/**
 * @brief 获取火焰强度（模拟电压，mV）
 * @note 参考电压为3.3V，若传感器AO输出超过3.3V需分压
 */
float Flame_GetIntensity(void)
{
    uint16_t adc = Flame_ReadADC();
    // 参考电压 3.3V = 3300mV
    return (float)adc * 3300.0f / 4096.0f;
}

