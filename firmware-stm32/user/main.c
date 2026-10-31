#include "stm32f10x.h"
#include "delay.h"
#include "bsp_oled_spi.h"   // 1.3寸 SPI OLED
#include "oled.h"           // 0.96寸 IIC OLED SDA B6 SCL B7
#include "bsp_mq4.h"        // MQ-4 甲烷
#include "bsp_flame.h"      // 火焰传感器
#include "DHT11.h"          // DHT11 温湿度
#include "MAX6675.h"        // MAX6675（软件SPI，SCK=PB8）
#include "usart.h"          // 铁头山羊串口库
#include <stdio.h>
#include <math.h>
#include <string.h>


/* ========== 通信配置 ========== */
#define ESP32_USART         USART1
#define ESP32_BAUDRATE      115200

/* ========== 默认阈值 ========== */
#define DEFAULT_TEMP_HIGH      35      // 温度高报警阈值 (℃)
#define DEFAULT_HUMI_HIGH      75      // 湿度高报警阈值 (%)
#define DEFAULT_MQ4_HIGH       500     // 可燃气体高报警阈值 (ppm)
#define DEFAULT_TC_HIGH        50      // 热电偶（MAX6675）温度高报警阈值 (℃)

/* ========== 全局阈值变量 ========== */
volatile float g_threshold_temp_high  = DEFAULT_TEMP_HIGH;
volatile float g_threshold_humi_high  = DEFAULT_HUMI_HIGH;
volatile float g_threshold_mq4_high   = DEFAULT_MQ4_HIGH;
volatile float g_threshold_tc_high    = DEFAULT_TC_HIGH;

/* ========== 临时消息显示 ========== */
static uint8_t show_temp_msg = 0;
static uint8_t msg_remaining_loops = 0;

/* ========== IIC OLED 显示缓存 ========== */
static char disp_dht11[20]  = "---";
static char disp_flame[20]  = "---";

/* ========== 函数声明 ========== */
void ESP32_USART_Init(void);
void ProcessReceivedJson(char *json_str);
void UpdateIICOLED(void);

/* ========== 串口初始化 ========== */
void ESP32_USART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = ESP32_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(ESP32_USART, &USART_InitStructure);
    USART_Cmd(ESP32_USART, ENABLE);
}

/* ========== 处理接收到的 JSON 指令 ========== */
void ProcessReceivedJson(char *json_str)
{
    char cmd[32] = {0};
    char *p = strstr(json_str, "\"cmd\":\"");
    if (p)
    {
        p += 7;
        int i = 0;
        while (*p && *p != '\"' && i < 31) cmd[i++] = *p++;
        cmd[i] = '\0';
        if (strcmp(cmd, "set_threshold") != 0) return;
    }
    float val;
    int updated = 0;
    p = strstr(json_str, "\"temp_high\":");
    if (p && sscanf(p, "\"temp_high\":%f", &val) == 1) { g_threshold_temp_high = val; updated = 1; }
    p = strstr(json_str, "\"humi_high\":");
    if (p && sscanf(p, "\"humi_high\":%f", &val) == 1) { g_threshold_humi_high = val; updated = 1; }
    p = strstr(json_str, "\"mq4_high\":");
    if (p && sscanf(p, "\"mq4_high\":%f", &val) == 1) { g_threshold_mq4_high = val; updated = 1; }
    p = strstr(json_str, "\"tc_high\":");
    if (p && sscanf(p, "\"tc_high\":%f", &val) == 1) { g_threshold_tc_high = val; updated = 1; }

    if (updated)
    {
        show_temp_msg = 1;
        msg_remaining_loops = 13;
    }
}

/* ========== 刷新 IIC OLED（优化布局） ========== */
void UpdateIICOLED(void)
{
    // 清空两行显示区域
    OLED_ShowString(0, 0, "                ", OLED_8X16);
    OLED_ShowString(0, 16, "                ", OLED_8X16);

    if (show_temp_msg && msg_remaining_loops > 0)
    {
        // 临时消息居中显示在第一行
        OLED_ShowString(24, 0, "阈值已更新", OLED_8X16);
    }
    else
    {
        if (msg_remaining_loops == 0) show_temp_msg = 0;
        // 第一行：温湿度数据
        OLED_ShowString(0, 0, disp_dht11, OLED_8X16);
        // 第二行：火焰数据
        OLED_ShowString(0, 16, disp_flame, OLED_8X16);
    }
    OLED_Update();
}

/* ========== 主函数 ========== */
int main(void)
{
    uint8_t flame_det;
    float flame_int;
    char flame_buf[20];
    float mq4_ppm;
    char mq4_buf[20];
    uint8_t dht11_temp, dht11_humi;
    char dht11_buf[20];
    float max6675_temp;
    char max6675_buf[20];

    uint8_t update_flame = 0, update_mq4 = 0, update_dht11 = 0, update_max6675 = 0;
    static float last_ppm = -999.0f, last_flame_int = -999.0f, last_max6675 = -999.0f;
    static uint8_t last_flame_det = 0xFF, last_dht11_temp = 0xFF, last_dht11_humi = 0xFF;

    Delay_Init();
    SPI_OLED_Init();
    OLED_Init();

    MQ4_Init();
    Flame_Init();
    DHT11_Init();
    MAX6675_Init();
    ESP32_USART_Init();

    // IIC OLED 清屏（不再显示静态标题，全部内容动态刷新）
    OLED_Clear();
    OLED_Update();

    // SPI OLED 标题
    SPI_OLED_Clear();
    SPI_OLED_ShowString_8x16(0, 0, "TC:");
    SPI_OLED_ShowString_8x16(0, 16, "MQ-4:");
    SPI_OLED_ShowString_8x16(0, 32, "状态:安全");
    SPI_OLED_Update();

    // MQ-4 预热倒计时
    for (int8_t cd = 20; cd >= 0; cd--)
    {
        char cd_buf[16];
        SPI_OLED_Clear();
        SPI_OLED_ShowString_8x16(0, 0, "TC:");
        SPI_OLED_ShowString_8x16(0, 16, "MQ-4:");
        SPI_OLED_ShowChinese(0, 32, "状");
        SPI_OLED_ShowChinese(16, 32, "态");
        SPI_OLED_ShowChar_8x16(32, 32, ':');
        SPI_OLED_Update();
        sprintf(cd_buf, "预热:%2ds", cd);
        OLED_ShowString(24, 0, cd_buf, OLED_8X16);
        OLED_Update();
        Delay(1000);
    }
    OLED_Clear();
    OLED_Update();
    MQ4_Calibrate();

    sprintf(disp_dht11, "---");
    sprintf(disp_flame, "---");

    while (1)
    {
        // 串口接收
        static char rx_buf[128];
        static uint8_t idx = 0;
        while (USART_GetFlagStatus(ESP32_USART, USART_FLAG_RXNE) == SET)
        {
            char c = USART_ReceiveData(ESP32_USART);
            if (c == '\n')
            {
                rx_buf[idx] = '\0';
                if (idx > 0) ProcessReceivedJson(rx_buf);
                idx = 0;
            }
            else if (c == '\r') continue;
            else if (idx < sizeof(rx_buf)-1) rx_buf[idx++] = c;
        }

        // 临时消息计时
        if (show_temp_msg)
        {
            if (msg_remaining_loops > 0)
                msg_remaining_loops--;
            else
                show_temp_msg = 0;
        }

        // DHT11
        static uint32_t dht11_timer = 0;
        if (dht11_timer == 0)
        {
            if (DHT11_ReadData(&dht11_humi, &dht11_temp))
            {
                if (last_dht11_temp == 0xFF || dht11_temp != last_dht11_temp || dht11_humi != last_dht11_humi)
                {
                    last_dht11_temp = dht11_temp;
                    last_dht11_humi = dht11_humi;
                    update_dht11 = 1;
                    sprintf(dht11_buf, "温:%dC 湿:%d%%", dht11_temp, dht11_humi);
                }
            }
            else
            {
                if (last_dht11_temp != 0xFE)
                {
                    last_dht11_temp = 0xFE;
                    update_dht11 = 1;
                    sprintf(dht11_buf, "温湿度错误");
                }
            }
            dht11_timer = 10;
        }
        dht11_timer--;
        if (update_dht11)
        {
            strcpy(disp_dht11, dht11_buf);
            update_dht11 = 0;
        }

        // 火焰
        flame_det = Flame_IsDetected();
        flame_int = Flame_GetIntensity();
        if (last_flame_det == 0xFF || flame_det != last_flame_det || fabs(flame_int - last_flame_int) > 50.0f)
        {
            last_flame_det = flame_det;
            last_flame_int = flame_int;
            update_flame = 1;
            if (flame_det)
                sprintf(flame_buf, "火灾! %dmV", (int)flame_int);
            else
                sprintf(flame_buf, "安全   %dmV", (int)flame_int);
        }
        if (update_flame)
        {
            strcpy(disp_flame, flame_buf);
            update_flame = 0;
        }

        // MAX6675
        max6675_temp = MAX6675_ReadTemperature();
        if (max6675_temp > -273.0f)
        {
            if (last_max6675 < -900.0f || fabs(max6675_temp - last_max6675) > 0.5f)
            {
                last_max6675 = max6675_temp;
                update_max6675 = 1;
                sprintf(max6675_buf, "%.1fC", max6675_temp);
            }
        }
        else
        {
            if (last_max6675 > -273.0f || last_max6675 < -900.0f)
            {
                last_max6675 = -999.0f;
                update_max6675 = 1;
                sprintf(max6675_buf, "断开");
            }
        }
        if (update_max6675)
        {
            uint8_t p, c;
            for (p = 0; p < 2; p++)
                for (c = 48; c < 128; c++) SPI_OLED_GRAM[p][c] = 0x00;
            SPI_OLED_ShowString_8x16(48, 0, max6675_buf);
            update_max6675 = 0;
        }

        // MQ-4
        mq4_ppm = MQ4_GetPPM();
        if (last_ppm < -900.0f || fabs(mq4_ppm - last_ppm) > 0.5f)
        {
            last_ppm = mq4_ppm;
            update_mq4 = 1;
            sprintf(mq4_buf, "%.1fppm", mq4_ppm);
        }
        if (update_mq4)
        {
            uint8_t p, c;
            for (p = 2; p < 4; p++)
                for (c = 48; c < 128; c++) SPI_OLED_GRAM[p][c] = 0x00;
            SPI_OLED_ShowString_8x16(48, 16, mq4_buf);
            update_mq4 = 0;
        }

        // 警报判断与 SPI OLED 第三行显示
        uint8_t alert_fire      = flame_det;
        uint8_t alert_gas       = (mq4_ppm > g_threshold_mq4_high);
        uint8_t alert_temp_high = (dht11_temp > g_threshold_temp_high);
        uint8_t alert_humi_high = (dht11_humi > g_threshold_humi_high);
        uint8_t alert_tc_high   = (max6675_temp > g_threshold_tc_high);

        for (uint8_t p = 4; p < 6; p++)
            for (uint8_t c = 0; c < 128; c++) SPI_OLED_GRAM[p][c] = 0x00;

        SPI_OLED_ShowChinese(0, 32, "状");
        SPI_OLED_ShowChinese(16, 32, "态");
        SPI_OLED_ShowChar_8x16(32, 32, ':');

        uint8_t x_offset = 40;
        if (alert_fire) {
            SPI_OLED_ShowChinese(x_offset, 32, "火"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "警");
        } else if (alert_gas) {
            SPI_OLED_ShowChinese(x_offset, 32, "燃"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "气"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "泄"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "漏");
        } else if (alert_temp_high) {
            SPI_OLED_ShowChinese(x_offset, 32, "温"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "度"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "过"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "高");
        } else if (alert_humi_high) {
            SPI_OLED_ShowChinese(x_offset, 32, "湿"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "度"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "过"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "高");
        } else if (alert_tc_high) {
            SPI_OLED_ShowChar_8x16(x_offset, 32, 'T'); x_offset += 8;
            SPI_OLED_ShowChar_8x16(x_offset, 32, 'C'); x_offset += 8;
            SPI_OLED_ShowChinese(x_offset, 32, "超"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "限");
        } else {
            SPI_OLED_ShowChinese(x_offset, 32, "安"); x_offset += 16;
            SPI_OLED_ShowChinese(x_offset, 32, "全");
        }
        SPI_OLED_Update();

        // 构建 JSON 并发送
        char json_buf[512];
        sprintf(json_buf,
            "{"
            "\"mq4\":%.1f,"
            "\"flame\":%d,"
            "\"flame_mv\":%d,"
            "\"temp\":%d,"
            "\"humi\":%d,"
            "\"tc\":%.1f,"
            "\"alert\":{"
                "\"fire\":%s,"
                "\"gas\":%s,"
                "\"temp_high\":%s,"
                "\"humi_high\":%s,"
                "\"tc_high\":%s"
            "}"
            "}\r\n",
            mq4_ppm,
            flame_det, (int)flame_int,
            dht11_temp, dht11_humi, max6675_temp,
            alert_fire ? "true" : "false",
            alert_gas ? "true" : "false",
            alert_temp_high ? "true" : "false",
            alert_humi_high ? "true" : "false",
            alert_tc_high ? "true" : "false"
        );
        My_USART_SendString(ESP32_USART, json_buf);

        UpdateIICOLED();
        SPI_OLED_Update();

        Delay(200);
    }
}

