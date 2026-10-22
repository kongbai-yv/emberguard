#include "bsp_oled_spi.h"
#include "delay.h"
#include "stdio.h"
#include <string.h>
/* ???? */
uint8_t SPI_OLED_GRAM[8][128];

/* ?? SPI ????? */
void SPI_OLED_WriteByte(uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        SPI_OLED_SCK_LOW();
        if (data & 0x80)
            SPI_OLED_MOSI_HIGH();
        else
            SPI_OLED_MOSI_LOW();
        data <<= 1;
        SPI_OLED_SCK_HIGH();
        DelayUs(1);
    }
    SPI_OLED_SCK_LOW();
}

/* ??? */
static void SPI_OLED_WriteCmd(uint8_t cmd)
{
    SPI_OLED_DC_LOW();
    SPI_OLED_CS_LOW();
    SPI_OLED_WriteByte(cmd);
    SPI_OLED_CS_HIGH();
}

/* GPIO ??? */
static void SPI_OLED_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    // SCK(PA5), MOSI(PA7)
    GPIO_InitStructure.GPIO_Pin = SPI_OLED_SCK_PIN | SPI_OLED_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_OLED_SCK_PORT, &GPIO_InitStructure);

    // CS(PB0), DC(PB1)
    GPIO_InitStructure.GPIO_Pin = SPI_OLED_CS_PIN | SPI_OLED_DC_PIN;
    GPIO_Init(SPI_OLED_CS_PORT, &GPIO_InitStructure);

    // RST(PC13)
    GPIO_InitStructure.GPIO_Pin = SPI_OLED_RST_PIN;
    GPIO_Init(SPI_OLED_RST_PORT, &GPIO_InitStructure);

    // ????
    SPI_OLED_CS_HIGH();
    SPI_OLED_DC_HIGH();
    SPI_OLED_RST_HIGH();
    SPI_OLED_SCK_LOW();
    SPI_OLED_MOSI_LOW();
}

/* SH1106 ????? */
void SPI_OLED_Init(void)
{
    SPI_OLED_GPIO_Init();

    // ????
    SPI_OLED_RST_LOW();
    Delay(10);
    SPI_OLED_RST_HIGH();
    Delay(10);

    // SH1106 ?????
    SPI_OLED_WriteCmd(0xAE);    // ????
    SPI_OLED_WriteCmd(0x02);    // ??4?
    SPI_OLED_WriteCmd(0x10);    // ??4?
    SPI_OLED_WriteCmd(0x40);    // ???
    SPI_OLED_WriteCmd(0xB0);    // ???
    SPI_OLED_WriteCmd(0x81);    // ???
    SPI_OLED_WriteCmd(0xFF);
    SPI_OLED_WriteCmd(0xA1);    // ????
    SPI_OLED_WriteCmd(0xC8);    // ????
    SPI_OLED_WriteCmd(0xA6);    // ????
    SPI_OLED_WriteCmd(0xA8);    // ????
    SPI_OLED_WriteCmd(0x3F);
    SPI_OLED_WriteCmd(0xD3);    // ????
    SPI_OLED_WriteCmd(0x00);
    SPI_OLED_WriteCmd(0xD5);    // ????
    SPI_OLED_WriteCmd(0x80);
    SPI_OLED_WriteCmd(0xD9);    // ???
    SPI_OLED_WriteCmd(0xF1);
    SPI_OLED_WriteCmd(0xDA);    // COM????
    SPI_OLED_WriteCmd(0x12);
    SPI_OLED_WriteCmd(0xDB);    // VCOMH
    SPI_OLED_WriteCmd(0x40);
    SPI_OLED_WriteCmd(0x8D);    // ???
    SPI_OLED_WriteCmd(0x14);
    SPI_OLED_WriteCmd(0xAF);    // ????

    SPI_OLED_Clear();
    SPI_OLED_Update();
}

/* ???? */
void SPI_OLED_SetCursor(uint8_t page, uint8_t col)
{
    col += 2;   // SH1106 ???
    SPI_OLED_WriteCmd(0xB0 | (page & 0x07));
    SPI_OLED_WriteCmd(0x10 | ((col & 0xF0) >> 4));
    SPI_OLED_WriteCmd(0x00 | (col & 0x0F));
}

/* ?? */
void SPI_OLED_Clear(void)
{
    uint8_t i, j;
    for (i = 0; i < 8; i++)
        for (j = 0; j < 128; j++)
            SPI_OLED_GRAM[i][j] = 0x00;
}

/* ???? */
void SPI_OLED_Update(void)
{
    uint8_t page, col;
    for (page = 0; page < 8; page++)
    {
        SPI_OLED_SetCursor(page, 0);
        SPI_OLED_DC_HIGH();
        SPI_OLED_CS_LOW();
        for (col = 0; col < 128; col++)
        {
            SPI_OLED_WriteByte(SPI_OLED_GRAM[page][col]);
        }
        SPI_OLED_CS_HIGH();
    }
}

/* ??(??) */
static void SPI_OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    if (x >= 128 || y >= 64) return;
    if (color)
        SPI_OLED_GRAM[page][x] |=  (1 << bit);
    else
        SPI_OLED_GRAM[page][x] &= ~(1 << bit);
}

/* ?? 8x16 ?? */
void SPI_OLED_ShowChar_8x16(uint8_t x, uint8_t y, char ch)
{
    uint8_t i, j;
    uint8_t temp;
    const uint8_t *pData = OLED_F8x16[ch - ' '];

    for (i = 0; i < 8; i++)
    {
        temp = pData[i];
        for (j = 0; j < 8; j++)          // ????(???)
        {
            if (temp & 0x01)
                SPI_OLED_DrawPoint(x + i, y + j, 1);
            else
                SPI_OLED_DrawPoint(x + i, y + j, 0);
            temp >>= 1;
        }
    }
    for (i = 0; i < 8; i++)
    {
        temp = pData[i + 8];              // ????(???)
        for (j = 0; j < 8; j++)
        {
            if (temp & 0x01)
                SPI_OLED_DrawPoint(x + i, y + j + 8, 1);
            else
                SPI_OLED_DrawPoint(x + i, y + j + 8, 0);
            temp >>= 1;
        }
    }
}

/* ?? 8x16 ??? */
void SPI_OLED_ShowString_8x16(uint8_t x, uint8_t y, char *str)
{
    while (*str)
    {
        SPI_OLED_ShowChar_8x16(x, y, *str++);
        x += 8;
        if (x > 128 - 8)
        {
            x = 0;
            y += 16;
        }
    }
	}

	
/**
 * @brief 显示一个16x16的汉字（UTF-8字符串格式）
 * @param x 左上角横坐标 (0~127)
 * @param y 左上角纵坐标 (0~63)
 * @param ChineseStr 指向汉字UTF-8字符串的指针（例如 "你"）
 */
	
	
void SPI_OLED_ShowChinese(uint8_t x, uint8_t y, char *ChineseStr)
{
    uint8_t col, row;
    uint8_t byteL, byteH;
    uint16_t index;
    const uint8_t *pData = NULL;

    /* 查找字模 */
    for (index = 0; ; index++)
    {
        if (strcmp(OLED_CF16x16[index].Index, "") == 0)
        {
            pData = OLED_CF16x16[index].Data;
            break;
        }
        if (strcmp(OLED_CF16x16[index].Index, ChineseStr) == 0)
        {
            pData = OLED_CF16x16[index].Data;
            break;
        }
    }

    /* 逐列绘制：左半8列 + 右半8列 */
    for (col = 0; col < 8; col++)          // 左半部分列
    {
        byteL = pData[col];                // 上方8点
        byteH = pData[col + 16];           // 下方8点
        for (row = 0; row < 8; row++)
        {
            if (byteL & (0x01 << row))
                SPI_OLED_DrawPoint(x + col, y + row, 1);
            else
                SPI_OLED_DrawPoint(x + col, y + row, 0);
            if (byteH & (0x01 << row))
                SPI_OLED_DrawPoint(x + col, y + row + 8, 1);
            else
                SPI_OLED_DrawPoint(x + col, y + row + 8, 0);
        }
    }
    for (col = 0; col < 8; col++)          // 右半部分列
    {
        byteL = pData[col + 8];            // 注意：右半部分列数据位于数组的后16字节中的前8字节？
        byteH = pData[col + 24];           // 根据字库结构：左半部分占前16字节，右半部分占后16字节。
        for (row = 0; row < 8; row++)
        {
            if (byteL & (0x01 << row))
                SPI_OLED_DrawPoint(x + col + 8, y + row, 1);
            else
                SPI_OLED_DrawPoint(x + col + 8, y + row, 0);
            if (byteH & (0x01 << row))
                SPI_OLED_DrawPoint(x + col + 8, y + row + 8, 1);
            else
                SPI_OLED_DrawPoint(x + col + 8, y + row + 8, 0);
        }
    }
}

/**
 * @brief 显示混合字符串（自动识别中英文）
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param str UTF-8 编码的字符串
 */
void SPI_OLED_ShowMixedString(uint8_t x, uint8_t y, char *str)
{
    uint8_t col = x;
    while (*str)
    {
        // 判断是否为中文（UTF-8 三字节，首字节 0xE0~0xEF）
        if ((*str & 0xE0) == 0xE0 && (*(str+1) & 0x80) && (*(str+2) & 0x80))
        {
            char chinese[4] = {str[0], str[1], str[2], '\0'};
            SPI_OLED_ShowChinese(col, y, chinese);
            col += 16;
            str += 3;
        }
        else  // ASCII 字符
        {
            SPI_OLED_ShowChar_8x16(col, y, *str);
            col += 8;
            str++;
        }
        if (col > 128 - 8) break; // 防止溢出
    }
}

