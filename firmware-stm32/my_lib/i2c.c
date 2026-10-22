#include "i2c.h"
#include "delay.h"   // 提供 DelayUs 等

/**
 * @brief 初始化 I2C1 (100kHz 标准模式)
 */
void I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    /* 1. 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    /* 2. 配置 SCL 和 SDA 为复用开漏输出 */
    GPIO_InitStructure.GPIO_Pin = I2C1_SCL_PIN | I2C1_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C1_PORT, &GPIO_InitStructure);

    /* 3. 配置 I2C1 */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;        // 快速模式占空比 2:1
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;                 // 主机模式，无需自身地址
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;               // 使能应答
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;                // 100kHz
    I2C_Init(I2C1, &I2C_InitStructure);

    /* 4. 使能 I2C1 */
    I2C_Cmd(I2C1, ENABLE);
}

/**
 * @brief 通过 I2C1 发送多个字节
 * @param addr 从机地址 (7位右对齐，函数内部会自动左移)
 * @param pData 数据指针
 * @param size 数据长度
 * @return 0:成功, -1:寻址失败, -2:数据发送失败
 */
int I2C1_SendBytes(uint8_t addr, const uint8_t *pData, uint16_t size)
{
    uint32_t timeout;

    if (size == 0) return 0;

    /* 等待总线空闲 */
    timeout = 100000;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
        if (--timeout == 0) return -1;

    /* 发送起始条件 */
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        if (--timeout == 0) return -1;

    /* 发送从机地址 (写) */
    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Transmitter);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_AF);
            I2C_GenerateSTOP(I2C1, ENABLE);
            return -1;  // 寻址失败
        }
        if (--timeout == 0) return -1;
    }

    /* 发送数据 */
    for (uint16_t i = 0; i < size; i++)
    {
        I2C_SendData(I2C1, pData[i]);
        timeout = 100000;
        while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE))
            if (--timeout == 0) return -2;
    }

    /* 等待最后一个字节发送完成 (BTF) */
    timeout = 100000;
    while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF))
        if (--timeout == 0) return -2;

    /* 发送停止条件 */
    I2C_GenerateSTOP(I2C1, ENABLE);

    return 0;
}

/**
 * @brief 通过 I2C1 接收多个字节
 * @param addr 从机地址 (7位右对齐)
 * @param pBuffer 接收缓冲区
 * @param size 数据长度
 * @return 0:成功, -1:寻址失败
 */
int I2C1_ReceiveBytes(uint8_t addr, uint8_t *pBuffer, uint16_t size)
{
    uint32_t timeout;

    if (size == 0) return 0;

    /* 等待总线空闲 */
    timeout = 100000;
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
        if (--timeout == 0) return -1;

    /* 发送起始条件 */
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
        if (--timeout == 0) return -1;

    /* 发送从机地址 (读) */
    I2C_Send7bitAddress(I2C1, addr << 1, I2C_Direction_Receiver);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_AF);
            I2C_GenerateSTOP(I2C1, ENABLE);
            return -1;
        }
        if (--timeout == 0) return -1;
    }

    /* 接收数据 */
    if (size == 1)
    {
        /* 单字节接收：禁用 ACK 并在最后一个数据前产生 STOP */
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        __disable_irq();  // 关中断避免时序被打断
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        I2C_GenerateSTOP(I2C1, ENABLE);
        __enable_irq();

        timeout = 100000;
        while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE))
            if (--timeout == 0) return -1;
        pBuffer[0] = I2C_ReceiveData(I2C1);
    }
    else
    {
        /* 多字节接收 */
        for (uint16_t i = 0; i < size; i++)
        {
            /* 倒数第二个字节时设置 NACK 和 STOP */
            if (i == size - 1)
            {
                I2C_AcknowledgeConfig(I2C1, DISABLE);
                __disable_irq();
                (void)I2C1->SR1;
                (void)I2C1->SR2;
                I2C_GenerateSTOP(I2C1, ENABLE);
                __enable_irq();
            }

            timeout = 100000;
            while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE))
                if (--timeout == 0) return -1;
            pBuffer[i] = I2C_ReceiveData(I2C1);
        }
    }

    /* 重新使能 ACK */
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    return 0;
}

