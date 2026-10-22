#include "bsp_mq4.h"
#include "delay.h"
#include <math.h>

static float g_R0 = 1.0f;   // 基准电阻，通过校准获得

/**
 * @brief 初始化MQ-4传感器ADC (PA1)
 */
void MQ4_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    /* 1. 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    /* 2. 配置PA1为模拟输入 */
    GPIO_InitStructure.GPIO_Pin = MQ4_ADC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(MQ4_ADC_PORT, &GPIO_InitStructure);

    /* 3. ADC配置 */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   // 单次转换
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(MQ4_ADC, &ADC_InitStructure);

    /* 4. 时钟分频 (PCLK2=72MHz，8分频得9MHz) */
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    /* 5. 使能ADC并校准 */
    ADC_Cmd(MQ4_ADC, ENABLE);
    ADC_ResetCalibration(MQ4_ADC);
    while(ADC_GetResetCalibrationStatus(MQ4_ADC));
    ADC_StartCalibration(MQ4_ADC);
    while(ADC_GetCalibrationStatus(MQ4_ADC));
}

/**
 * @brief 读取ADC原始值（单次转换）
 * @return 12位ADC值(0~4095)
 */
uint16_t MQ4_ReadADC(void)
{
    /* 配置通道，采样时间55.5周期 */
    ADC_RegularChannelConfig(MQ4_ADC, MQ4_ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);
    
    /* 启动转换 */
    ADC_SoftwareStartConvCmd(MQ4_ADC, ENABLE);
    
    /* 等待转换完成 */
    while(ADC_GetFlagStatus(MQ4_ADC, ADC_FLAG_EOC) == RESET);
    
    /* 返回结果 */
    return ADC_GetConversionValue(MQ4_ADC);
}

/**
 * @brief 获取传感器输出电压(mV)
 */
float MQ4_GetVoltage(void)
{
    uint16_t adc = MQ4_ReadADC();
    return (float)adc * MQ4_VREF / 4096.0f;
}

/**
 * @brief 计算传感器电阻Rs (Ω)
 * @note 公式：Rs = (Vc - Vout) * RL / Vout
 *       假设模块供电Vc = 5V = 5000mV
 */
float MQ4_GetRs(void)
{
    float vout = MQ4_GetVoltage();
    float vc = 5000.0f;   // 模块供电5V
    
    if (vout < 10.0f)     // 避免除零
        return 1e6f;
    
    return (vc - vout) * MQ4_RL_RESISTANCE / vout;
}

/**
 * @brief 获取电阻比率 Rs/R0
 * @return 比率值，若未校准返回-1
 */
float MQ4_GetRatio(void)
{
    if (g_R0 <= 0.0f)
        return -1.0f;
    
    return MQ4_GetRs() / g_R0;
}

/**
 * @brief 估算甲烷浓度(ppm)
 * @note 使用对数拟合公式：ppm = 1000 * (Rs/R0)^(1/slope)
 *       slope典型值约为-0.4（根据数据手册）
 */
float MQ4_GetPPM(void)
{
    float ratio = MQ4_GetRatio();
    if (ratio <= 0.0f)
        return -1.0f;
    
    /* 浓度计算公式 */
    float ppm = 1000.0f * powf(ratio, 1.0f / -0.4f);
    return ppm;
}

/**
 * @brief 校准传感器（在洁净空气中调用）
 * @note 测量当前Rs，除以清洁空气因子得到R0
 */
void MQ4_Calibrate(void)
{
    uint8_t i;
    float sum = 0.0f;
    
    /* 预热提示：实际使用前应确保传感器已预热20秒以上 */
    
    /* 多次采样取平均 */
    for (i = 0; i < 20; i++)
    {
        sum += MQ4_GetRs();
        Delay(10);
    }
    
    float rs_clean = sum / 20.0f;
    g_R0 = rs_clean / MQ4_RO_CLEAN_AIR_FACTOR;
}

