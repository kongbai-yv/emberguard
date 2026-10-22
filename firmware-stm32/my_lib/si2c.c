//#include "si2c.h"
//#include "delay.h"

//#define SI2C_DelayUs(us)   DelayUs(us)   // 使用你的精确微秒延时

//#define scl_w(v) GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, ((v)?Bit_SET:Bit_RESET))
//#define sda_w(v) GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, ((v)?Bit_SET:Bit_RESET))
//#define scl_r ((GPIO_ReadInputDataBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin) == Bit_SET) ? 1 : 0)
//#define sda_r ((GPIO_ReadInputDataBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin) == Bit_SET) ? 1 : 0)

//static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte);
//static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack);
//static void SendStop(SI2C_TypeDef *SI2C);

//__weak void My_SI2C_Init(SI2C_TypeDef *SI2C)
//{
//    if(SI2C->SCL_GPIOx == GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//    else if(SI2C->SCL_GPIOx == GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//    else if(SI2C->SCL_GPIOx == GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
//    else if(SI2C->SCL_GPIOx == GPIOD) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
//    
//    GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, Bit_SET);
//    GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, Bit_SET);
//    
//    if(SI2C->SDA_GPIOx == GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//    else if(SI2C->SDA_GPIOx == GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//    else if(SI2C->SDA_GPIOx == GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
//    else if(SI2C->SDA_GPIOx == GPIOD) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
//    
//    GPIO_InitTypeDef GPIO_InitStruct;
//    GPIO_InitStruct.GPIO_Pin = SI2C->SCL_GPIO_Pin;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
//    GPIO_Init(SI2C->SCL_GPIOx, &GPIO_InitStruct);
//    
//    GPIO_InitStruct.GPIO_Pin = SI2C->SDA_GPIO_Pin;
//    GPIO_Init(SI2C->SDA_GPIOx, &GPIO_InitStruct);
//}

//__weak int My_SI2C_SendBytes(SI2C_TypeDef *SI2C, uint8_t Addr, const uint8_t *pData, uint16_t Size)
//{
//    sda_w(1); scl_w(1); SI2C_DelayUs(10);
//    sda_w(0); SI2C_DelayUs(10);
//    
//    if(SendByte(SI2C, Addr & 0xfe) != 0) { SendStop(SI2C); return -1; }
//    for(uint16_t i=0; i<Size; i++) if(SendByte(SI2C, pData[i]) != 0) { SendStop(SI2C); return -2; }
//    SendStop(SI2C);
//    return 0;
//}

//__weak int My_SI2C_ReceiveBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t *pBuffer, uint16_t Size)
//{
//    sda_w(1); scl_w(1); SI2C_DelayUs(10);
//    sda_w(0); SI2C_DelayUs(10);
//    
//    if(SendByte(SI2C, Addr | 0x01) != 0) { SendStop(SI2C); return -1; }
//    for(uint16_t i=0; i<Size; i++) pBuffer[i] = ReceiveByte(SI2C, (i==Size-1)?1:0);
//    SendStop(SI2C);
//    return 0;
//}

//static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte)
//{
//    for(int8_t i=7; i>=0; i--)
//    {
//        scl_w(0);
//        sda_w((Byte & (0x01<<i)) ? 1 : 0);
//        SI2C_DelayUs(5);
//        scl_w(1);
//        SI2C_DelayUs(5);
//    }
//    scl_w(0); sda_w(1); SI2C_DelayUs(5);
//    scl_w(1); SI2C_DelayUs(5);
//    uint8_t ack = sda_r;
//    scl_w(0);
//    return ack;
//}

//static void SendStop(SI2C_TypeDef *SI2C)
//{
//    scl_w(0); sda_w(0); SI2C_DelayUs(5);
//    scl_w(1); SI2C_DelayUs(5);
//    sda_w(1); SI2C_DelayUs(5);
//}

//static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack)
//{
//    uint8_t ret = 0;
//    for(int8_t i=7; i>=0; i--)
//    {
//        scl_w(0); sda_w(1); SI2C_DelayUs(5);
//        scl_w(1); SI2C_DelayUs(5);
//        if(sda_r) ret |= (0x01 << i);
//    }
//    scl_w(0); sda_w(Ack?0:1); SI2C_DelayUs(5);
//    scl_w(1); SI2C_DelayUs(5);
//    scl_w(0);
//    return ret;
//}

