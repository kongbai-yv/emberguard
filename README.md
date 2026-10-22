# 焰哨 · EmberGuard

**基于 STM32 + ESP32 双芯片联动的火灾预警检测系统**
*A dual-MCU fire early-warning and detection system built on STM32 and ESP32.*

> STM32F103 负责前端采集与本地显示，ESP32 负责联网、云端上报与联动控制，PC 网页与手机 App 实时查看火情、可燃气体与环境趋势。
> STM32F103 handles sensing and local display; ESP32 handles networking, cloud reporting and actuator control; a web dashboard and a mobile app show live status.

---

## 1. 架构 / Architecture

```
 传感器 + OLED                 UART 115200 8N1            WiFi + MQTT/TLS
 STM32F103C8  ────────────────►  ESP32  ────────────────►  EMQX Cloud
 采集 / 报警 / 显示              联网 / 组包 / 联动            │
                                 继电器 · 蜂鸣器 · NTP         ├──► Web 仪表盘
                                                              └──► 移动端 App
                                     ▲  threshold/set  ◄───────────────┘
```

| 环节 | 说明 |
|---|---|
| 主控 | STM32F103C8（Cortex-M3，64 KB Flash / 20 KB RAM） |
| 联网 | ESP32（Arduino 框架） |
| 板间 | STM32 `USART1` ↔ ESP32 `Serial2`，**115200 8N1**，共地 |
| 引脚 | ESP32 RX = GPIO16，TX = GPIO18 |
| 云端 | EMQX Cloud，TLS 8883；网页端用 WSS 8084 |
| 主题 | 上行 `sensor/data`（约 5 秒一包），下行 `threshold/set` |

## 2. 仓库结构 / Repository layout

```
emberguard/
├─ firmware-stm32/   STM32 固件（Keil 工程 template.uvprojx）
│  ├─ user/main.c    主逻辑：采集、阈值判断、串口上报
│  ├─ App/           DHT11 / MQ-4 燃气 / 火焰 / BMP180 / MAX6675 / HC-SR04 / OLED(SPI)
│  └─ my_lib/        delay / I2C / SPI / UART 等底层封装
├─ firmware-esp32/   ESP32 固件（Arduino sketch）
│  ├─ *.ino          串口收数 → MQTT 上报；AHT20 + BMP280；继电器与蜂鸣器
│  └─ secrets.example.h  凭据模板
├─ web-monitor/      Web 仪表盘（Vite + React + Tailwind）
├─ mobile-app/       移动端（uni-app，MQTT over TLS/WSS）
└─ docs/             架构与接线说明
```

## 3. 告警逻辑 / How detection works

| 信号 | 传感器 | 判定 |
|---|---|---|
| 火焰 | 火焰传感器（模拟量 + 数字量） | 光强超过阈值立即触发 |
| 可燃气体 | MQ-4 | 浓度超 `mq4_high` 触发 |
| 异常升温 | MAX6675 热电偶 / DHT11 / BMP280 | 温度或升温速率超阈值 |
| 环境 | 温湿度、气压 | 用于趋势与误报抑制 |

触发后：本地蜂鸣器 + OLED 提示，ESP32 立即上报 `sensor/data`，云端可下发 `threshold/set` 调整阈值。

## 4. 快速开始 / Quick start

### 4.1 凭据配置（必做）

仓库里**不含任何密钥**。使用前：

```bash
# ESP32
cp firmware-esp32/secrets.example.h firmware-esp32/secrets.h
# 然后编辑 secrets.h 填入 WiFi 与 MQTT 凭据；secrets.h 已被 .gitignore 忽略
```

Web 与移动端把 `YOUR_BROKER_HOST` / `YOUR_MQTT_USER` / `YOUR_PASSWORD` 换成自己的值（建议本地改动，不要提交）。

### 4.2 烧录

1. **STM32**：Keil 打开 `firmware-stm32/template.uvprojx`，编译下载
2. **ESP32**：Arduino IDE 打开 `firmware-esp32/*.ino`，装好所需库后上传
3. **接线**：STM32 `PA9`(TX) → ESP32 `GPIO16`，`PA10`(RX) ← ESP32 `GPIO18`，两板共地
4. **看数据**：`cd web-monitor && npm install && npm run dev`；移动端用 HBuilderX 运行或打包

### 4.3 常见问题

- 串口全是乱码 → 两端波特率不一致（必须都是 115200）
- ESP32 能连 WiFi 但收不到数据 → 检查 TX/RX 是否接反、是否共地
- 网页连不上 → 浏览器要求 `wss://`，端口用 8084

## 5. 隐私说明 / Privacy

公开版本中已移除：WiFi 名称与密码、MQTT 主机/用户名/密码、机构品牌字样。本地开发请使用 `secrets.h` 或自己的配置文件，并确认 `.gitignore` 生效。

Credentials are stripped from this public copy. Keep your own values in `secrets.h` or local config files and verify `.gitignore` before committing.

## 6. 硬件清单 / Hardware

STM32F103C8 最小系统板、ESP32 开发板、DHT11、MQ-4、火焰传感器、BMP280（或 BMP180）、MAX6675 + K 型热电偶、HC-SR04、0.96" SPI OLED、0.91" I2C OLED、继电器模块 ×4（风机 ×2、水泵、照明）、蜂鸣器、SU-03T 离线语音模块（可选）。

## 7. 参与 / Contributing

欢迎提 Issue 或 PR。提交前请确认没有把 `secrets.h`、真实 Broker 地址或设备凭据带进提交。

## 8. 许可 / License

未指定许可证；如需开源请补充 `LICENSE`（例如 MIT）。
