# 焰哨 · EmberGuard

**基于 STM32 + ESP32 双芯片联动的火灾预警检测系统**
*A dual-MCU fire early-warning and detection system built on STM32 and ESP32.*

> STM32F103 负责前端采集、阈值判断与本地显示（双 OLED）；ESP32 负责联网、云端上报与联动控制（继电器 / 蜂鸣器 / NTP 校时）；Web 仪表盘与移动端 App 通过 MQTT over TLS 实时查看火情、可燃气体与环境趋势，并可反向下发阈值。

---

## 1. 架构 / Architecture

```
 传感器 + 双 OLED             UART 115200 8N1           WiFi + MQTT/TLS
 STM32F103C8  ────────────────►  ESP32  ────────────────►  EMQX Cloud
 采集 / 阈值判断 / 显示          联网 / 组包 / 联动            │
                                 继电器×4 · 蜂鸣器 · NTP       ├──► Web 仪表盘
                                 AHT20 + BMP280                └──► 移动端 App
                                                                   │
                                     ▲  threshold/set  ◄───────────┘
```

| 环节 | 说明 |
|---|---|
| 主控 | STM32F103C8（Cortex-M3，64 KB Flash / 20 KB RAM），标准外设库（非 HAL） |
| 联网 | ESP32（Arduino 框架），WiFi + MQTT over TLS 8883 |
| 板间 | STM32 `USART1` ↔ ESP32 `Serial2`，**115200 8N1**，共地；STM32 PA9/PA10 ↔ ESP32 GPIO16/GPIO18 |
| 云端 | EMQX Cloud，TLS 8883；网页端用 WSS 8084 |
| 主题 | 上行 `sensor/data`（约 5 秒一包），下行 `threshold/set` |
| 显示 | STM32：1.3" SPI OLED + 0.91" I2C OLED；ESP32：0.91" I2C OLED（U8g2） |

## 2. 仓库结构 / Repository layout

```
emberguard/
├─ firmware-stm32/                  STM32 固件（Keil 工程 EmberGuard.uvprojx）
│  ├─ user/main.c                   主逻辑：采集、阈值判断、OLED 刷新、串口上报
│  ├─ user/stm32f10x_it.c           中断向量
│  ├─ bsp_*.c / DHT11.* / MAX6675.* 板级驱动（MQ-4、火焰、DHT11、热电偶）
│  ├─ my_lib/                       delay / 软件 I2C / SPI / USART / OLED / 字库
│  └─ std_periph_driver/            ST 标准外设库
├─ firmware-esp32/
│  └─ emberguard_esp32/
│     ├─ emberguard_esp32.ino       串口收数 → MQTT 上报；AHT20 + BMP280；继电器与蜂鸣器
│     └─ secrets.example.h          凭据模板（复制为 secrets.h 使用）
├─ web-monitor/                     Web 仪表盘（Vite + React + Tailwind）
│  └─ legacy/                       早期单文件版本（monitor.html、test-mqtt.html），仅存档
└─ mobile-app/                      移动端（uni-app，MQTT over TLS/WSS）
```

## 3. 检测逻辑 / How detection works

| 信号 | 传感器 | 判定 |
|---|---|---|
| 火焰 | 火焰传感器（模拟量 + 数字量） | 光强超过阈值立即触发 |
| 可燃气体 | MQ-4 | 浓度超 `mq4_high` 触发（上电预热 20 s 后标定基线） |
| 异常升温 | MAX6675 热电偶 / DHT11 / ESP32 BMP280 | 温度或升温速率超阈值 |
| 环境 | 温湿度、气压 | 用于趋势展示与误报抑制 |

触发后：本地蜂鸣器 + OLED 告警提示，ESP32 立即上报 `sensor/data`，云端可下发 `threshold/set` 调整阈值。

## 4. 关键设计 / Design notes

- **双 OLED 分工**：1.3" SPI 屏刷主数据（温度、湿度、气体、火焰），0.91" I2C 屏刷状态行，避免单屏信息拥挤；两屏按各自总线独立初始化。
- **局部重绘**：主循环按"数值变化量 + 变化区域"决定刷新范围，只在数值区（y=48~127）清屏重画，减少 SPI 全屏刷新带来的撕裂与闪烁。
- **传感器失效降级**：任一传感器读数为无效值（NaN / 超量程 / 通信超时）时，界面显示占位符而非残留旧值，避免"假数据"误导判断。
- **MQ-4 预热与标定**：MQ-4 上电需预热，代码在 20 s 预热结束后读取基线并计算相对浓度，规避上电初期的高值误报。
- **凭据分离**：仓库不含任何密钥，`secrets.example.h` 提供模板，真实凭据写入被 `.gitignore` 忽略的 `secrets.h`。
- **移动端 MQTT**：uni-app 无成熟 MQTT over TLS 客户端，改为手写 MQTT 3.1.1 报文（`CONNECT` 固定头 `0x10`、`connectFlags = 0xC2`：username + password + clean session），直接跑在 WebSocket 之上。

## 5. 快速开始 / Quick start

### 5.1 凭据配置（必做）

仓库里**不含任何密钥**。使用前：

```bash
# ESP32
cp firmware-esp32/emberguard_esp32/secrets.example.h \
   firmware-esp32/emberguard_esp32/secrets.h
# 然后编辑 secrets.h 填入 WiFi 与 MQTT 凭据；secrets.h 已被 .gitignore 忽略
```

Web 与移动端把 `YOUR_BROKER_HOST` / `YOUR_MQTT_USER` / `YOUR_PASSWORD` 换成自己的值（建议只改本地，不要提交）。

### 5.2 烧录

1. **STM32**：Keil 打开 `firmware-stm32/EmberGuard.uvprojx`，编译下载
2. **ESP32**：Arduino IDE 打开 `firmware-esp32/emberguard_esp32/emberguard_esp32.ino`，安装 `PubSubClient`、`ArduinoJson`、`U8g2`、`NTPClient`、`Adafruit AHTX0`、`Adafruit BMP280` 后上传
3. **接线**：STM32 `PA9`(TX) → ESP32 `GPIO16`，`PA10`(RX) ← ESP32 `GPIO18`，两板共地
4. **看数据**：`cd web-monitor && npm install && npm run dev`；移动端用 HBuilderX 运行或打包

### 5.3 常见问题

- 串口全是乱码 → 两端波特率不一致（必须都是 115200）
- ESP32 能连 WiFi 但收不到数据 → 检查 TX/RX 是否接反、是否共地
- 网页连不上 → 浏览器要求 `wss://`，端口用 8084

## 6. 隐私说明 / Privacy

公开版本中已移除：WiFi 名称与密码、MQTT 主机/用户名/密码、机构品牌字样。本地开发请使用 `secrets.h` 或自己的配置文件，并确认 `.gitignore` 生效。

Credentials are stripped from this public copy. Keep your own values in `secrets.h` or local config files and verify `.gitignore` before committing.

## 7. 硬件清单 / Hardware

**已接入主程序**：STM32F103C8 最小系统板、ESP32 开发板、DHT11、MQ-4、火焰传感器、MAX6675 + K 型热电偶、0.96"~1.3" SPI OLED、0.91" I2C OLED、继电器模块 ×4（风机 ×2、水泵、照明）、蜂鸣器、ESP32 侧 AHT20 + BMP280。

**驱动已提供，主程序未接入**：BMP180（`bsp_bmp180.c`，与 BMP280 功能重叠，未编译进工程）、HC-SR04 超声波（`bsp_hcsr04.c`）、SU-03T 离线语音模块。如需启用，需在 `main.c` 中补充初始化与调用。

## 8. 已知限制 / Known limitations

- **串口收数采用轮询**：STM32 主循环带 `Delay(200)`，115200 下 200 ms 内可能累积约 2.3 KB 数据，而 USART 数据寄存器仅 1 字节深，高速或突发场景会丢包并置 ORE 标志。后续改为 DMA + 空闲中断环形缓冲。
- **阈值在两处定义**：STM32（35 / 75 / 500 / 50）与 ESP32（35 / 60 / 400 / 100）各有一套默认值，未统一，容易出现"两端判断不一致"。后续由云端下发统一。
- **未启用独立看门狗（IWDG）**：程序跑飞后无法自恢复。
- **上报使用 `sprintf` 拼 JSON**：`char json_buf[512]` 在 20 KB RAM 上偏大，浮点格式化也会显著增加 Flash 占用。后续改用轻量手写序列化。

## 9. 路线图 / Roadmap

- [ ] STM32 串口改用 DMA + 空闲中断 + 环形缓冲，接收端做长度与 CRC 校验
- [ ] 阈值由云端统一下发，本地仅缓存
- [ ] 增加 IWDG 与异常复位计数
- [ ] Web 仪表盘移除无关的 3D 机柜场景，改为传感器拓扑图
- [ ] 补充接线图与实物照片

## 10. 参与 / Contributing

欢迎提 Issue 或 PR。提交前请确认没有把 `secrets.h`、真实 Broker 地址或设备凭据带进提交。

## 11. 许可 / License

MIT，见 [LICENSE](LICENSE)。
