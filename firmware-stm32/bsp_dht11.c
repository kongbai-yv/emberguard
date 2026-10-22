#include "bsp_sht11.h"
#include "delay.h"

/* ---------- µ×²ãÒý½Å²Ù×÷ ---------- */
static void SHT11_SCK_H(void) { GPIO_SetBits(SHT11_SCK_PORT, SHT11_SCK_PIN); }
static void SHT11_SCK_L(void) { GPIO_ResetBits(SHT11_SCK_PORT, SHT11_SCK_PIN); }
static void SHT11_DATA_H(void) { GPIO_SetBits(SHT11_DATA_PORT, SHT11_DATA_PIN); }
static void SHT11_DATA_L(void) { GPIO_ResetBits(SHT11_DATA_PORT, SHT11_DATA_PIN); }
static uint8_t SHT11_DATA_READ(void) { return GPIO_ReadInputDataBit(SHT11_DATA_PORT, SHT11_DATA_PIN); }

/* ÉèÖÃDATAÒý½ÅÎªÊä³öÄ£Ê½ */
static void SHT11_DATA_SetOut(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SHT11_DATA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SHT11_DATA_PORT, &GPIO_InitStructure);
}

/* ÉèÖÃDATAÒý½ÅÎªÊäÈëÄ£Ê½ */
static void SHT11_DATA_SetIn(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SHT11_DATA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // ÉÏÀ­ÊäÈë
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SHT11_DATA_PORT, &GPIO_InitStructure);
}

/* ---------- ³õÊ¼»¯ ---------- */
void SHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* SCK ÍÆÍìÊä³ö */
    GPIO_InitStructure.GPIO_Pin = SHT11_SCK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SHT11_SCK_PORT, &GPIO_InitStructure);

    /* DATA ¿ªÂ©Êä³ö */
    SHT11_DATA_SetOut();

    /* ³õÊ¼µçÆ½ */
    SHT11_SCK_L();
    SHT11_DATA_H();

    Delay(15);  // µÈ´ý´«¸ÐÆ÷ÉÏµçÎÈ¶¨£¨>11ms£©
}

/* ---------- Æô¶¯´«ÊäÊ±Ðò ---------- */
static void SHT11_Start(void)
{
    SHT11_DATA_SetOut();
    SHT11_DATA_H(); SHT11_SCK_H(); DelayUs(1);
    SHT11_DATA_L(); DelayUs(1);
    SHT11_SCK_L();  DelayUs(1);
    SHT11_SCK_H();  DelayUs(1);
    SHT11_DATA_H(); DelayUs(1);
    SHT11_SCK_L();
}

/* ---------- ·¢ËÍÒ»¸ö×Ö½Ú£¬²¢·µ»ØACK ---------- */
static uint8_t SHT11_SendByte(uint8_t data)
{
    uint8_t i, ack;

    SHT11_DATA_SetOut();
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80) SHT11_DATA_H(); else SHT11_DATA_L();
        data <<= 1;
        SHT11_SCK_H(); DelayUs(1);
        SHT11_SCK_L(); DelayUs(1);
    }

    /* ÊÍ·ÅDATA£¬ÇÐ»»ÎªÊäÈëÒÔ¶ÁÈ¡ACK */
    SHT11_DATA_SetIn();
    SHT11_SCK_H(); DelayUs(1);
    ack = SHT11_DATA_READ(); // 0 = ACK, 1 = NACK
    SHT11_SCK_L();

    return ack;
}

/* ---------- ½ÓÊÕÒ»¸ö×Ö½Ú£¬²¢·¢ËÍACK/NACK ---------- */
static uint8_t SHT11_ReceiveByte(uint8_t ack)
{
    uint8_t i, data = 0;

    SHT11_DATA_SetIn();
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        SHT11_SCK_H(); DelayUs(1);
        if (SHT11_DATA_READ()) data |= 0x01;
        SHT11_SCK_L(); DelayUs(1);
    }

    /* ·¢ËÍACK (0) »ò NACK (1) */
    SHT11_DATA_SetOut();
    if (ack) SHT11_DATA_H(); else SHT11_DATA_L();
    SHT11_SCK_H(); DelayUs(1);
    SHT11_SCK_L();
    SHT11_DATA_H(); // ÊÍ·Å×ÜÏß

    return data;
}

/* ---------- Èí¸´Î» ---------- */
static void SHT11_SoftReset(void)
{
    SHT11_Start();
    SHT11_SendByte(SHT11_CMD_SOFT_RST);
    Delay(15);  // ¸´Î»ºóµÈ´ý
}

/* ---------- ¶ÁÈ¡²âÁ¿½á¹û ---------- */
static uint16_t SHT11_ReadRaw(uint8_t cmd)
{
    uint16_t raw = 0;
    uint8_t retry = 3;

    while (retry--)
    {
        SHT11_Start();
        if (SHT11_SendByte(cmd) == 0)  // ÊÕµ½ACK
        {
            /* µÈ´ý²âÁ¿Íê³É£¬DATAÀ­µÍ±íÊ¾Íê³É */
            uint16_t timeout = 50000;
            SHT11_DATA_SetIn();
            while (SHT11_DATA_READ() == 1)
                if (--timeout == 0) break;
            if (timeout == 0) continue; // ³¬Ê±£¬ÖØÊÔ

            /* ¶ÁÈ¡¸ß8Î» */
            raw = SHT11_ReceiveByte(0) << 8; // ACK
            /* ¶ÁÈ¡µÍ8Î» */
            raw |= SHT11_ReceiveByte(1);     // NACK
            break;
        }
    }
    return raw;
}

/* ---------- ¼ÆËãÎÂ¶È (¡æ) ---------- */
static float SHT11_CalcTemp(uint16_t raw)
{
    // ×ª»»¹«Ê½: T = d1 + d2 * SOT
    // d1 = -39.7 (5V) »ò -39.6 (3.3V)
    // d2 = 0.01 (14bit·Ö±æÂÊ)
    float d1 = -39.6f;
    float d2 = 0.01f;
    return d1 + d2 * (float)raw;
}

/* ---------- ¼ÆËãÊª¶È (%RH) ---------- */
static float SHT11_CalcHumi(uint16_t raw, float temp)
{
    // Êª¶È·ÇÏßÐÔ²¹³¥
    float rh_linear = -4.0f + 0.0405f * (float)raw - 2.8e-6f * (float)(raw * raw);
    // ÎÂ¶È²¹³¥
    return (temp - 25.0f) * (0.01f + 0.00008f * (float)raw) + rh_linear;
}

/* ---------- Ö÷¶Áº¯Êý ---------- */
int SHT11_Read(float *temp, float *humi)
{
    uint16_t raw_temp, raw_humi;
    float t;

    /* Èí¸´Î»£¨¿£¬Ìá¸ßÎÈ¶¨ÐÔ£© */
    static uint8_t first_read = 1;
    if (first_read) { SHT11_SoftReset(); first_read = 0; }

    /* ¶ÁÈ¡ÎÂ¶È */
    raw_temp = SHT11_ReadRaw(SHT11_CMD_TEMP);
    if (raw_temp == 0) return -1;

    /* ¶ÁÈ¡Êª¶È */
    raw_humi = SHT11_ReadRaw(SHT11_CMD_HUMI);
    if (raw_humi == 0) return -1;

    /* ¼ÆËãÎÂ¶È */
    t = SHT11_CalcTemp(raw_temp);
    /* ¼ÆËãÊª¶È£¨ÐèÒªÎÂ¶È²¹³¥£© */
    *humi = SHT11_CalcHumi(raw_humi, t);
    *temp = t;

    return 0;
}

