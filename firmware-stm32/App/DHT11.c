#include "DHT11.h"
#include "delay.h"

// 配置GPIO为输出模式
static void DHT11_Mode_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

// 配置GPIO为输入模式（使用上拉输入）
static void DHT11_Mode_In(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // ? 改为上拉输入
    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

// 复位DHT11（主机拉低至少18ms）
static void DHT11_Reset(void)
{
    DHT11_Mode_Out();       // 设置为输出
    DHT11_OUT_LOW();        // 拉低总线
    Delay(20);              // 延时20ms
    DHT11_OUT_HIGH();       // 拉高总线
    DelayUs(30);            // 延时30us
}

// 等待DHT11响应
// 返回值：1=响应成功，0=响应失败
static uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    
    DHT11_Mode_In();        // 设置为输入
    
    // 等待DHT11拉低总线（响应信号）
    while(DHT11_IN() && retry < 100)
    {
        DelayUs(1);
        retry++;
    }
    if(retry >= 100) return 0;
    
    retry = 0;
    // 等待DHT11拉高总线（80us后）
    while(!DHT11_IN() && retry < 100)
    {
        DelayUs(1);
        retry++;
    }
    if(retry >= 100) return 0;
    
    return 1;
}

// 读取一个位
// 返回值：0或1
static uint8_t DHT11_ReadBit(void)
{
    uint8_t retry = 0;
    
    // 等待高电平开始
    while(DHT11_IN() && retry < 100)
    {
        DelayUs(1);
        retry++;
    }
    
    retry = 0;
    // 等待低电平（数据开始）
    while(!DHT11_IN() && retry < 100)
    {
        DelayUs(1);
        retry++;
    }
    
    // 延时40us判断：26-28us是0，70us是1
    DelayUs(40);
    
    if(DHT11_IN())
    {
        // 高电平持续，表示1
        while(DHT11_IN() && retry < 100)  // 等待剩余的高电平结束
        {
            DelayUs(1);
            retry++;
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

// 读取一个字节
static uint8_t DHT11_ReadByte(void)
{
    uint8_t i, data = 0;
    
    for(i = 0; i < 8; i++)
    {
        data <<= 1;
        data |= DHT11_ReadBit();
    }
    
    return data;
}

// 初始化DHT11
void DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE);
    
    // 初始化为输出模式，并拉高
    DHT11_Mode_Out();
    DHT11_OUT_HIGH();
}

// 读取温湿度数据
// humi: 湿度值（整数）
// temp: 温度值（整数）
// 返回值：1=成功，0=失败
uint8_t DHT11_ReadData(uint8_t *humi, uint8_t *temp)
{
    uint8_t buf[5];
    uint8_t i;
    
    // 复位并等待响应
    DHT11_Reset();
    if(!DHT11_Check())
    {
        return 0;
    }
    
    // 读取5个字节（湿度整数、湿度小数、温度整数、温度小数、校验和）
    for(i = 0; i < 5; i++)
    {
        buf[i] = DHT11_ReadByte();
    }
    
    // 校验
    if(buf[0] + buf[1] + buf[2] + buf[3] == buf[4])
    {
        *humi = buf[0];
        *temp = buf[2];
        return 1;
    }
    
    return 0;
}