#ifndef __BSP_OLED_SPI_H
#define __BSP_OLED_SPI_H

#include "stm32f10x.h"
#include "OLED_Data.h"

/* ????:1 = SH1106, 0 = SSD1306 */
#define OLED_CHIP_SH1106   0

/* ???? */
#define SPI_OLED_SCK_PORT   GPIOA
#define SPI_OLED_SCK_PIN    GPIO_Pin_5
#define SPI_OLED_MOSI_PORT  GPIOA
#define SPI_OLED_MOSI_PIN   GPIO_Pin_7
#define SPI_OLED_CS_PORT    GPIOB
#define SPI_OLED_CS_PIN     GPIO_Pin_0
#define SPI_OLED_DC_PORT    GPIOB
#define SPI_OLED_DC_PIN     GPIO_Pin_1
#define SPI_OLED_RST_PORT   GPIOC
#define SPI_OLED_RST_PIN    GPIO_Pin_13

/* ??? */
#define SPI_OLED_CS_HIGH()  GPIO_SetBits(SPI_OLED_CS_PORT, SPI_OLED_CS_PIN)
#define SPI_OLED_CS_LOW()   GPIO_ResetBits(SPI_OLED_CS_PORT, SPI_OLED_CS_PIN)
#define SPI_OLED_DC_HIGH()  GPIO_SetBits(SPI_OLED_DC_PORT, SPI_OLED_DC_PIN)
#define SPI_OLED_DC_LOW()   GPIO_ResetBits(SPI_OLED_DC_PORT, SPI_OLED_DC_PIN)
#define SPI_OLED_RST_HIGH() GPIO_SetBits(SPI_OLED_RST_PORT, SPI_OLED_RST_PIN)
#define SPI_OLED_RST_LOW()  GPIO_ResetBits(SPI_OLED_RST_PORT, SPI_OLED_RST_PIN)
#define SPI_OLED_SCK_HIGH() GPIO_SetBits(SPI_OLED_SCK_PORT, SPI_OLED_SCK_PIN)
#define SPI_OLED_SCK_LOW()  GPIO_ResetBits(SPI_OLED_SCK_PORT, SPI_OLED_SCK_PIN)
#define SPI_OLED_MOSI_HIGH()GPIO_SetBits(SPI_OLED_MOSI_PORT, SPI_OLED_MOSI_PIN)
#define SPI_OLED_MOSI_LOW() GPIO_ResetBits(SPI_OLED_MOSI_PORT, SPI_OLED_MOSI_PIN)

/* ???? */
#define OLED_WIDTH   128
#define OLED_HEIGHT  64

/* ????(??,???????) */
extern uint8_t SPI_OLED_GRAM[8][128];
void SPI_OLED_ShowChinese(uint8_t x, uint8_t y, char *ChineseStr);
/* ???? */
void SPI_OLED_Init(void);
void SPI_OLED_Clear(void);
void SPI_OLED_Update(void);
void SPI_OLED_SetCursor(uint8_t page, uint8_t col);
void SPI_OLED_ShowChar_8x16(uint8_t x, uint8_t y, char ch);
void SPI_OLED_ShowString_8x16(uint8_t x, uint8_t y, char *str);
void SPI_OLED_WriteByte(uint8_t data);   // ??????????

#endif


