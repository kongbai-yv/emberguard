#include "bsp_hcsr04.h"
#include "delay.h"

static volatile uint32_t g_capture_rise = 0;    // 上升沿捕获值
static volatile uint32_t g_capture_fall = 0;    // 下降沿捕获值
static volatile uint8_t  g_capture_flag = 0;    // 捕获完成标志

/**
 * @brief 初始化 HC-SR04 引脚和 TIM1 输入捕获
 */
void HCSR04_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 1. 使能 GPIOA、TIM1 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);

    /* 2. 配置 Trig (PA4) 推挽输出 */
    GPIO_InitStructure.GPIO_Pin = HCSR04_TRIG_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HCSR04_TRIG_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);

    /* 3. 配置 Echo (PA8) 为浮空输入，并复用为 TIM1_CH1 */
    GPIO_InitStructure.GPIO_Pin = HCSR04_ECHO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(HCSR04_ECHO_PORT, &GPIO_InitStructure);

    /* 4. 配置 TIM1 基本时基：计数频率 1MHz（1us 分辨率） */
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;          // 最大计数值
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;       // 72MHz/72 = 1MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* 5. 配置 TIM1_CH1 输入捕获：上升沿捕获 */
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;              // 不滤波
    TIM_ICInit(TIM1, &TIM_ICInitStructure);

    /* 6. 使能捕获中断 */
    TIM_ITConfig(TIM1, TIM_IT_CC1, ENABLE);

    /* 7. 配置 NVIC */
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 8. 使能定时器 */
    TIM_Cmd(TIM1, ENABLE);
}

/**
 * @brief 发送 10us 触发脉冲
 */
static void HCSR04_Trigger(void)
{
    GPIO_SetBits(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
    DelayUs(12);
    GPIO_ResetBits(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN);
}

/**
 * @brief 获取距离（厘米）
 * @return 距离值，失败返回 -1.0f
 */
float HCSR04_GetDistance(void)
{
    uint32_t pulse_width = 0;
    float distance = 0.0f;

    /* 清除标志并准备捕获 */
    g_capture_flag = 0;
    g_capture_rise = 0;
    g_capture_fall = 0;

    /* 发送触发脉冲 */
    HCSR04_Trigger();

    /* 等待捕获完成（超时 100ms） */
    uint32_t timeout = 100000;
    while (g_capture_flag == 0)
    {
        if (--timeout == 0) return -1.0f;
    }

    /* 计算高电平时间（单位：us） */
    if (g_capture_fall > g_capture_rise)
        pulse_width = g_capture_fall - g_capture_rise;
    else
        pulse_width = 0xFFFF - g_capture_rise + g_capture_fall + 1; // 计数值溢出处理

    /* 计算距离 */
    distance = pulse_width / 58.0f;

    /* 数据有效性检查 */
    if (distance < 2.0f || distance > 450.0f)
        return -1.0f;

    return distance;
}

/**
 * @brief TIM1 捕获中断服务函数
 */
void TIM1_CC_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_CC1) != RESET)
    {
        /* 读取捕获值，自动清除中断标志 */
        uint16_t capture_val = TIM_GetCapture1(TIM1);

        /* 判断当前极性 */
        if (TIM1->CCER & TIM_CCER_CC1P)  // 下降沿捕获
        {
            g_capture_fall = capture_val;
            g_capture_flag = 1;          // 捕获完成
            /* 切换回上升沿，为下次测量准备 */
            TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Rising);
        }
        else  // 上升沿捕获
        {
            g_capture_rise = capture_val;
            /* 切换为下降沿捕获 */
            TIM_OC1PolarityConfig(TIM1, TIM_ICPolarity_Falling);
        }

        TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);
    }
}

