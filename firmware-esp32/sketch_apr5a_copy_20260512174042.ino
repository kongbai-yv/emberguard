#include <Wire.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <U8g2lib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

// 凭据不写进仓库：复制 secrets.example.h 为 secrets.h 并填写真实值
#include "secrets.h"

// ========== WiFi 配置 ==========

// ========== EMQX Cloud MQTT 配置 ==========

const int mqtt_port = 8883;

const char* pub_topic = "sensor/data";
const char* sub_topic = "threshold/set";

// ========== 引脚定义 ==========
#define BUZZER_PIN      25
#define RELAY_FAN1      32
#define RELAY_PUMP      33
#define RELAY_LIGHT     26
#define RELAY_FAN2      27

// ========== 0.91 寸 IIC OLED 配置 ==========
#define I2C_SCL         22
#define I2C_SDA         21
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, I2C_SCL, I2C_SDA);

// ========== NTP 时间配置 ==========
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800);

// ========== 串口通信（与STM32） ==========
#define STM32_RX 16
#define STM32_TX 18
#define STM32_SERIAL Serial2

// ========== 从STM32接收的原始数据 ==========
float stmMq4 = 0.0;
int   stmFlame = 0;
int   stmFlameMV = 0;
float stmTemp = 0.0;
float stmHumi = 0.0;
float stmTc = 0.0;

// 其他报警由 ESP32 本地阈值计算
bool alertFire = false;
bool alertGas = false;
bool alertTempHigh = false;
bool alertHumiHigh = false;
bool alertTcHigh = false;

// ========== 本地阈值变量 ==========
float dhtTempThreshold = 35.0;
float humiThreshold = 60.0;
float mq4Threshold = 400.0;
float pt100Threshold = 100.0;

// ========== 蜂鸣器模式 ==========
enum BuzzerMode { BUZZER_OFF, BUZZER_FAST, BUZZER_SLOW };
BuzzerMode currentBuzzerMode = BUZZER_OFF;
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

// 定时任务
unsigned long lastStatusPrint = 0;
const unsigned long statusPrintInterval = 5000;
unsigned long lastMqttPublish = 0;
const unsigned long mqttPublishInterval = 5000;

// MQTT 客户端
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// ========== BMP280 气压传感器 ==========
Adafruit_BMP280 bmp;
bool bmpOk = false;

// ========== 天气预警（基于气压骤降） ==========
bool stormWarning = false;              // 暴雨/台风预警标志
float lastPressureForTrend = 0.0;       // 上一次比较的气压值 (hPa)
unsigned long lastTrendCheckTime = 0;   // 上次检测时间
const unsigned long TREND_CHECK_INTERVAL = 300000; // 5分钟 = 300000 ms
const float PRESSURE_DROP_THRESHOLD = 1.0; // 5分钟内下降超过1.0 hPa 触发预警

// ========== 函数声明 ==========
void selfTest();
void handleSTM32Data();
void handleSerialCommand();
void updateAlertsFromLocalThresholds();
void updateRelaysAndBuzzer();
void printStatusSummary();
void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishSensorData();
void updateOLED();
void checkWeatherTrend();  // 新增：检测气压趋势并更新 stormWarning

// ========== 初始化 ==========
void setup() {
  Serial.begin(115200);
  STM32_SERIAL.begin(115200, SERIAL_8N1, STM32_RX, STM32_TX);

  Wire.begin(I2C_SDA, I2C_SCL);
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.clearBuffer();

  // 初始化 BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("⚠️ BMP280 未找到 (地址0x76)，尝试 0x77...");
    if (!bmp.begin(0x77)) {
      Serial.println("❌ BMP280 初始化失败，天气预警功能禁用");
      bmpOk = false;
    } else {
      bmpOk = true;
      Serial.println("✅ BMP280 初始化成功 (地址 0x77)");
    }
  } else {
    bmpOk = true;
    Serial.println("✅ BMP280 初始化成功 (地址 0x76)");
  }
  if (bmpOk) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
    // 记录初始气压值
    lastPressureForTrend = bmp.readPressure() / 100.0F;
  }

  connectWiFi();
  connectMQTT();
  timeClient.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);

  pinMode(RELAY_FAN1, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_LIGHT, OUTPUT);
  pinMode(RELAY_FAN2, OUTPUT);
  digitalWrite(RELAY_FAN1, LOW);
  digitalWrite(RELAY_PUMP, LOW);
  digitalWrite(RELAY_LIGHT, LOW);
  digitalWrite(RELAY_FAN2, LOW);

  selfTest();
  Serial.println("✅ ESP32 自检完成，天气预警功能已启用。");
}

void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();
  timeClient.update();

  handleSerialCommand();
  handleSTM32Data();
  updateAlertsFromLocalThresholds();
  updateRelaysAndBuzzer();

  // 定期检查气压趋势（每5分钟）
  if (bmpOk) {
    unsigned long now = millis();
    if (now - lastTrendCheckTime >= TREND_CHECK_INTERVAL) {
      checkWeatherTrend();
      lastTrendCheckTime = now;
    }
  }

  // 蜂鸣器逻辑
  unsigned long now = millis();
  unsigned long interval = 0;
  switch (currentBuzzerMode) {
    case BUZZER_FAST: interval = 200; break;
    case BUZZER_SLOW: interval = 800; break;
    default: interval = 0; break;
  }
  if (interval > 0) {
    if (now - lastBuzzerToggle >= interval) {
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? LOW : HIGH);
      lastBuzzerToggle = now;
    }
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
  }

  updateOLED();

  if (now - lastMqttPublish > mqttPublishInterval) {
    publishSensorData();
    lastMqttPublish = now;
  }
  if (now - lastStatusPrint > statusPrintInterval) {
    printStatusSummary();
    lastStatusPrint = now;
  }
  delay(50);
}

// 检测气压变化趋势，更新风暴预警标志
void checkWeatherTrend() {
  float currentPressure = bmp.readPressure() / 100.0F;
  float pressureChange = currentPressure - lastPressureForTrend;  // 正值=上升，负值=下降
  Serial.printf("🌬️ 气压检测: 5分钟内变化 = %.2f hPa (当前=%.2f, 上次=%.2f)\n",
                pressureChange, currentPressure, lastPressureForTrend);

  if (pressureChange <= -PRESSURE_DROP_THRESHOLD) {
    if (!stormWarning) {
      stormWarning = true;
      Serial.println("⚠️ 触发暴雨/台风预警！气压骤降！");
    }
  } else {
    if (stormWarning) {
      stormWarning = false;
      Serial.println("✅ 气压已稳定或回升，解除预警");
    }
  }
  // 更新记录值
  lastPressureForTrend = currentPressure;
}

void updateAlertsFromLocalThresholds() {
  alertFire = (stmFlame == 1);
  alertGas = (stmMq4 > mq4Threshold);
  alertTempHigh = (stmTemp > dhtTempThreshold);
  alertHumiHigh = (stmHumi > humiThreshold);
  alertTcHigh = (stmTc > pt100Threshold);
}

void handleSTM32Data() {
  if (STM32_SERIAL.available()) {
    String jsonStr = STM32_SERIAL.readStringUntil('\n');
    jsonStr.trim();
    if (jsonStr.length() == 0) return;
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, jsonStr);
    if (err) return;
    if (doc.containsKey("mq4")) stmMq4 = doc["mq4"];
    if (doc.containsKey("flame")) stmFlame = doc["flame"];
    if (doc.containsKey("flame_mv")) stmFlameMV = doc["flame_mv"];
    if (doc.containsKey("temp")) stmTemp = doc["temp"];
    if (doc.containsKey("humi")) stmHumi = doc["humi"];
    if (doc.containsKey("tc")) stmTc = doc["tc"];
  }
}

void updateRelaysAndBuzzer() {
  if (alertFire) {
    currentBuzzerMode = BUZZER_FAST;
    digitalWrite(RELAY_PUMP, HIGH);
    digitalWrite(RELAY_FAN1, LOW);
    digitalWrite(RELAY_FAN2, LOW);
    digitalWrite(RELAY_LIGHT, LOW);
  }
  else if (alertGas) {
    currentBuzzerMode = BUZZER_FAST;
    digitalWrite(RELAY_FAN1, HIGH);
    digitalWrite(RELAY_FAN2, HIGH);
    digitalWrite(RELAY_PUMP, LOW);
    digitalWrite(RELAY_LIGHT, LOW);
  }
  else if (alertTempHigh || alertTcHigh) {
    currentBuzzerMode = BUZZER_SLOW;
    digitalWrite(RELAY_FAN1, HIGH);
    digitalWrite(RELAY_FAN2, HIGH);
    digitalWrite(RELAY_PUMP, LOW);
    digitalWrite(RELAY_LIGHT, LOW);
  }
  else {
    currentBuzzerMode = BUZZER_OFF;
    digitalWrite(RELAY_PUMP, LOW);
    digitalWrite(RELAY_FAN1, LOW);
    digitalWrite(RELAY_FAN2, LOW);
    digitalWrite(RELAY_LIGHT, LOW);
  }
}

void handleSerialCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, cmd);
    if (err) {
      Serial.println("❌ 无效的 JSON 格式");
      return;
    }
    bool updated = false;
    if (doc.containsKey("temp_high")) { dhtTempThreshold = doc["temp_high"]; Serial.printf("✅ 温度上限已更新为: %.1f\n", dhtTempThreshold); updated = true; }
    if (doc.containsKey("humi_high")) { humiThreshold = doc["humi_high"]; Serial.printf("✅ 湿度上限已更新为: %.1f\n", humiThreshold); updated = true; }
    if (doc.containsKey("mq4_high")) { mq4Threshold = doc["mq4_high"]; Serial.printf("✅ 燃气上限已更新为: %.1f\n", mq4Threshold); updated = true; }
    if (doc.containsKey("tc_high")) { pt100Threshold = doc["tc_high"]; Serial.printf("✅ PT100 上限已更新为: %.1f\n", pt100Threshold); updated = true; }
    if (!updated) Serial.println("❌ 未识别的字段");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  Serial.print("📥 收到 MQTT 指令: ");
  Serial.println(message);
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err) {
    Serial.println("❌ JSON 解析失败");
    return;
  }
  bool updated = false;
  if (doc.containsKey("temp_high")) { dhtTempThreshold = doc["temp_high"]; Serial.printf("✅ 温度上限已更新为: %.1f\n", dhtTempThreshold); updated = true; }
  if (doc.containsKey("humi_high")) { humiThreshold = doc["humi_high"]; Serial.printf("✅ 湿度上限已更新为: %.1f\n", humiThreshold); updated = true; }
  if (doc.containsKey("mq4_high")) { mq4Threshold = doc["mq4_high"]; Serial.printf("✅ 燃气上限已更新为: %.1f\n", mq4Threshold); updated = true; }
  if (doc.containsKey("tc_high")) { pt100Threshold = doc["tc_high"]; Serial.printf("✅ PT100 上限已更新为: %.1f\n", pt100Threshold); updated = true; }
  if (!updated) Serial.println("⚠️ 未包含可识别的阈值字段");
}

void publishSensorData() {
  StaticJsonDocument<512> doc;
  doc["mq4"] = stmMq4;
  doc["flame"] = stmFlame;
  doc["flame_mv"] = stmFlameMV;
  doc["temp"] = stmTemp;
  doc["humi"] = stmHumi;
  doc["tc"] = stmTc;

  // 添加气压和海拔
  if (bmpOk) {
    float pressure = bmp.readPressure() / 100.0F;
    float altitude = bmp.readAltitude(1013.25);
    doc["pressure"] = pressure;
    doc["altitude"] = altitude;
  } else {
    doc["pressure"] = nullptr;
    doc["altitude"] = nullptr;
  }

  // 添加风暴预警标志
  doc["storm_warning"] = stormWarning;

  JsonObject alert = doc.createNestedObject("alert");
  alert["fire"] = alertFire;
  alert["gas"] = alertGas;
  alert["temp_high"] = alertTempHigh;
  alert["humi_high"] = alertHumiHigh;
  alert["tc_high"] = alertTcHigh;

  char buffer[512];
  serializeJson(doc, buffer);
  if (mqttClient.publish(pub_topic, buffer)) {
    Serial.print("📤 MQTT 发布成功: ");
    Serial.println(buffer);
  } else {
    Serial.println("❌ MQTT 发布失败");
  }
}
void updateOLED() {
  u8g2.clearBuffer();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);   // 16x16中文字体

  // ========== 报警优先级（火焰 > 燃气 > 超温 > 湿度过高 > 风暴预警） ==========
  String alarmText = "";
  if (alertFire) alarmText = "火焰报警";
  else if (alertGas) alarmText = "燃气泄漏";
  else if (alertTempHigh) alarmText = "温度过高";
  else if (alertTcHigh) alarmText = "PT100超温";
  else if (alertHumiHigh) alarmText = "湿度过高";
  else if (stormWarning) alarmText = "⛈️ 暴雨预警";

  if (alarmText != "") {
    // 显示报警（居中单行）
    int w = u8g2.getUTF8Width(alarmText.c_str());
    int x = (128 - w) / 2;
    if (x < 0) x = 0;
    u8g2.setCursor(x, 24);
    u8g2.print(alarmText);
  } else {
    // 无报警：轮换显示时间 或 气压/海拔（仅当 BMP280 可用）
    static unsigned long lastSwitch = 0;
    static int mode = 0;   // 0: 时间, 1: 气压海拔
    unsigned long now = millis();

    // 如果 BMP280 不可用，强制只显示时间，不切换
    if (!bmpOk) {
      mode = 0;
    } else {
      if (now - lastSwitch >= 3000) {   // 每 3 秒切换一次
        mode = 1 - mode;
        lastSwitch = now;
      }
    }

    if (mode == 0) {
      // 显示时间（居中单行）
      String timeStr = "时间: " + timeClient.getFormattedTime();
      int w = u8g2.getUTF8Width(timeStr.c_str());
      int x = (128 - w) / 2;
      if (x < 0) x = 0;
      u8g2.setCursor(x, 24);
      u8g2.print(timeStr);
    } else {
      // 显示气压和海拔（两行，各居中）
      if (bmpOk) {
        float pressure = bmp.readPressure() / 100.0F;      // hPa
        float altitude = bmp.readAltitude(1013.25);        // 米
        String line1 = "气压: " + String(pressure, 1) + " hPa";
        String line2 = "海拔: " + String(altitude, 1) + " m";

        int w1 = u8g2.getUTF8Width(line1.c_str());
        int x1 = (128 - w1) / 2;
        if (x1 < 0) x1 = 0;
        u8g2.setCursor(x1, 16);    // 第一行 y=16
        u8g2.print(line1);

        int w2 = u8g2.getUTF8Width(line2.c_str());
        int x2 = (128 - w2) / 2;
        if (x2 < 0) x2 = 0;
        u8g2.setCursor(x2, 32);    // 第二行 y=32
        u8g2.print(line2);
      } else {
        // BMP280 故障时显示提示（不会进入此分支，因为上面已强制 mode=0，但保留防御）
        u8g2.setCursor(0, 24);
        u8g2.print("气压传感器错误");
      }
    }
  }
  u8g2.sendBuffer();
}

void selfTest() {
  Serial.println("🔧 开始自检...");
  digitalWrite(BUZZER_PIN, LOW); delay(200);
  digitalWrite(BUZZER_PIN, HIGH); delay(200);
  digitalWrite(BUZZER_PIN, LOW); delay(200);
  digitalWrite(BUZZER_PIN, HIGH);
  int relays[] = {RELAY_FAN1, RELAY_PUMP, RELAY_LIGHT, RELAY_FAN2};
  for (int i = 0; i < 4; i++) {
    Serial.print("🔌 继电器 "); Serial.print(i+1); Serial.println(" 吸合");
    digitalWrite(relays[i], HIGH);
    delay(1000);
    digitalWrite(relays[i], LOW);
    Serial.print("🔌 继电器 "); Serial.print(i+1); Serial.println(" 断开");
    delay(1000);
  }
  Serial.println("🔧 自检结束。");
}

void connectWiFi() {
  Serial.print("📡 正在连接 WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("✅ WiFi 已连接，名称: ");
    Serial.println(WiFi.SSID());
    Serial.print("   IP 地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("❌ WiFi 连接失败");
  }
}

void connectMQTT() {
  espClient.setInsecure();
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  Serial.print("📡 正在连接 MQTT Broker: ");
  Serial.println(mqtt_broker);
  while (!mqttClient.connected()) {
    String clientId = "ESP32Client-" + String(WiFi.macAddress());
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("✅ MQTT 已连接");
      mqttClient.subscribe(sub_topic);
      Serial.print("📩 已订阅主题: ");
      Serial.println(sub_topic);
    } else {
      Serial.print("❌ MQTT 连接失败, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" 5秒后重试...");
      delay(5000);
    }
  }
}

void printStatusSummary() {
  Serial.println("========== 📊 状态摘要 ==========");
  Serial.print("🌡️ 环境温度(DHT): "); Serial.print(stmTemp, 1); Serial.println(" ℃");
  Serial.print("💧 环境湿度: "); Serial.print(stmHumi, 1); Serial.println(" %");
  Serial.print("🔥 PT100温度: "); Serial.print(stmTc, 1); Serial.println(" ℃");
  Serial.print("🔥 燃气浓度: "); Serial.print(stmMq4, 1); Serial.println(" ppm");
  Serial.print("🔥 火焰检测: "); Serial.println(alertFire ? "⚠️ 有火焰" : "✅ 正常");
  Serial.print("🌡️ 温度过高报警: "); Serial.println(alertTempHigh ? "⚠️ 是" : "✅ 否");
  Serial.print("💧 湿度过高报警: "); Serial.println(alertHumiHigh ? "⚠️ 是" : "✅ 否");
  Serial.print("🔥 PT100超温: "); Serial.println(alertTcHigh ? "⚠️ 是" : "✅ 否");

  if (bmpOk) {
    float pressure = bmp.readPressure() / 100.0F;
    float altitude = bmp.readAltitude(1013.25);
    Serial.print("⏲️ 大气压: "); Serial.print(pressure, 1); Serial.println(" hPa");
    Serial.print("⛰️ 估算海拔: "); Serial.print(altitude, 1); Serial.println(" m");
    Serial.print("🌦️ 风暴预警: "); Serial.println(stormWarning ? "⚠️ 激活" : "✅ 无");
  } else {
    Serial.println("⚠️ BMP280 不可用");
  }

  Serial.print("🔔 蜂鸣器模式: ");
  switch (currentBuzzerMode) {
    case BUZZER_FAST: Serial.println("急促响"); break;
    case BUZZER_SLOW: Serial.println("缓慢响"); break;
    default: Serial.println("关闭"); break;
  }
  Serial.print("⚙️ 继电器状态: ");
  Serial.print("风扇1="); Serial.print(digitalRead(RELAY_FAN1) ? "开" : "关");
  Serial.print(" 水泵="); Serial.print(digitalRead(RELAY_PUMP) ? "开" : "关");
  Serial.print(" 灯="); Serial.print(digitalRead(RELAY_LIGHT) ? "开" : "关");
  Serial.print(" 风扇2="); Serial.println(digitalRead(RELAY_FAN2) ? "开" : "关");
  Serial.println("==================================");
}