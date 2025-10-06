/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * @Hardwares: M5Core + Unit ?
 * @Platform Version: Arduino M5Stack Board Manager v2.1.3
 * @Dependent Library:
 * M5Stack@^0.4.6: https://github.com/m5stack/M5Stack
 * Adafruit_TCS34725: https://github.com/adafruit/Adafruit_TCS34725
 */

#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <WiFi.h>
#include <ArduinoOSCWiFi.h>

#define SENSOR1
// #define SENSOR2

struct Color {
  int r, g, b;
  int note;
};

Color colorTable[] =
{
  {86, 75, 103, 1},
  {70, 119, 69, 2},   
  {126, 65, 80, 3},   
  {107, 87, 67, 4}, 
  {93, 107, 54, 5}, 
  {37, 80, 148, 6}, 
  {46, 98, 120, 7}, 
  {57, 87, 119, 8},
  {45, 111, 109, 9},
  {115, 73, 79, 10},
  {43, 95, 125, 11},
  {78, 72, 115, 12},
  {25, 88, 154, 13},
  {80, 89, 93, 14},
  {56, 121, 83, 15}, 
  {35, 96, 134, 16}, 
  {69, 73, 123, 17}, 
  {71, 109, 79, 18}, 
  {33, 106, 128, 19},
  {29, 91, 146, 20},
  {71, 106, 83, 21},
  {114, 64, 89, 22},
  {32, 96, 138, 23},
  {45, 111, 108, 24}
};

int note;
int note2;

int count = sizeof(colorTable) / sizeof(colorTable[0]);

// ここから変更
String ssid = "L305";
String pw = "4dp_1450";

#ifdef SENSOR1
const IPAddress ip(192, 168, 0, 3);
extern const int oscPort = 4444;
#endif

#ifdef SENSOR2
const IPAddress ip(192, 168, 0, 4);
extern const int oscPort = 4000;
#endif
const IPAddress gateway(192, 168, 0, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns(192, 168, 0, 1);


const char* oscHost = "192.168.0.20";
// ここまで変更

unsigned long wifiMillis = 0;
int wifiReconnectCount = 0;

#define commonAnode true  // set to false if using a common cathode LED.
byte gammatable[256];  // our RGB -> eye-recognized gamma color

static uint16_t color16(uint16_t r, uint16_t g, uint16_t b)
{
    uint16_t _color;
    _color = (uint16_t)(r & 0xF8) << 8;
    _color |= (uint16_t)(g & 0xFC) << 3;
    _color |= (uint16_t)(b & 0xF8) >> 3;
    return _color;
}

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

Adafruit_TCS34725 tcs2 = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

const int switchPin = 12;
const int switchPin2 = 14;

int button1State = 0;
int button2State = 0;

int button1Flag = 0;
int button2Flag = 0;

int twoButtonStateFlag = 0;

void setup()
{
  Serial.begin(115200);

  #ifdef ESP_PLATFORM
    WiFi.disconnect(true, true);  // disable wifi, erase ap info
    delay(1000);
    WiFi.mode(WIFI_STA);
  #endif

  WiFi.config(ip, gateway, subnet, dns);
  WiFi.begin(ssid.c_str(), pw.c_str());

  unsigned long time = millis();
  Serial.print("Wifi Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
    if (millis() > time + 5000)
    {
        Serial.println("retry");
        WiFi.disconnect();
        WiFi.reconnect();
        time = millis();
    }
  }
  Serial.println("WiFiOK");

  Wire.begin(21, 22); // カラーセンサー1台目のピン
  Wire1.begin(32, 33); // カラーセンサー2台目のピン

  pinMode(switchPin, INPUT_PULLUP);
  pinMode(switchPin2, INPUT_PULLUP);

  Serial.println("Color View Test!");
  while (!tcs.begin())
  {  // 如果color unit未能初始化
    Serial.println("No TCS34725 found ... check your connections");
    delay(1000);
  }
  while (!tcs2.begin(TCS34725_ADDRESS, &Wire1))
  {
    Serial.println("2台目のカラーセンサーと接続できていません");
    delay(1000);
  }
  tcs.setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);  // Sets theintegration time for the
                                                            
  tcs.setGain(TCS34725_GAIN_4X);                           // Adjusts the gain on the TCS34725.

  tcs2.setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);
  tcs2.setGain(TCS34725_GAIN_4X);

  Serial.println("Setup OK.");
}

void loop()
{
  button1State = digitalRead(switchPin); // 現状のボタンの状態を記録
  button2State = digitalRead(switchPin2); // 現状のボタンの状態を記録
  Serial.println(button1State);

  if (WiFi.status() != WL_CONNECTED && millis() - wifiMillis >= 1000)
  {
    WiFi.disconnect();
    WiFi.reconnect();
    wifiMillis = millis();
    Serial.println("reconnect");
    wifiReconnectCount++;

    if (wifiReconnectCount > 10)
    {
      ESP.restart();
    }
  }

  uint16_t clear, red, green, blue;
  uint16_t clear2, red2, green2, blue2;
  tcs.getRawData(&red, &green, &blue,
                   &clear);  // Reads the raw red, green, blue and clear channel

  tcs2.getRawData(&red2, &green2, &blue2,
                   &clear2);  // Reads the raw red, green, blue and clear channel

  // Figure out some basic hex code for visualization.  生成对应的十六进制代码
  uint32_t sum = clear;
  uint32_t sum2 = clear2;
  float r, g, b;
  float r2, g2, b2;
  r = red;
  r /= sum;
  g = green;
  g /= sum;
  b = blue;
  b /= sum;
  r *= 255;
  g *= 255;
  b *= 255;

  r2 = red2;
  r2 /= sum2;
  g2 = green2;
  g2 /= sum2;
  b2 = blue2;
  b2 /= sum2;
  r2 *= 255;
  g2 *= 255;
  b2 *= 255;
  uint16_t _color = color16((int)r, (int)g, (int)b);
  uint16_t _color2 = color16((int)r2, (int)g2, (int)b2);

  int R = (int)r;
  int G = (int)g;
  int B = (int)b;

  int R2 = (int)r2;
  int G2 = (int)g2;
  int B2 = (int)b2;

  // ==============================
  // 最も近い色を探す
  // ==============================
  int index = 0;
  long bestDist = 999999;

  for (int i = 0; i < count; i++) {
    long dist = sq(R - colorTable[i].r) +
                sq(G - colorTable[i].g) +
                sq(B - colorTable[i].b);
    if (dist < bestDist) {
      bestDist = dist;
      index = i;
    }
  }
  // ==============================
  // 判定結果を出力
  // ==============================
  // Serial.printf("Norm R:%d G:%d B:%d -> ", R, G, B);
  if (bestDist < 4000) { // 誤差許容(数字を小さくすれば誤差は小さくなる)
    // Serial.printf("Matched Note %d\n", colorTable[index].note);
  } else {
    Serial.println("No match");
  }



  // 2台目の識別プログラム
  // ==============================
  // 最も近い色を探す
  // ==============================
  int index2 = 0;
  long bestDist2 = 999999;

  for (int i2 = 0; i2 < count; i2++) {
    long dist2 = sq(R2 - colorTable[i2].r) +
                sq(G2 - colorTable[i2].g) +
                sq(B2 - colorTable[i2].b);
    if (dist2 < bestDist2) {
      bestDist2 = dist2;
      index2 = i2;
    }
  }
  // ==============================
  // 判定結果を出力
  // ==============================
  // Serial.printf("Norm R2:%d G2:%d B2:%d -> ", R2, G2, B2);
  if (bestDist2 < 4000) { // 誤差許容(数字を小さくすれば誤差は小さくなる)
    // Serial.printf("Matched Note %d\n", colorTable[index2].note);
  } else {
    Serial.println("No match");
  }

  if (button1State == LOW)
  {
    if(button1Flag == 0)
    {
      Serial.println("スイッチ1押されました");
      Serial.printf("Normalized R: %.0f  G: %.0f  B: %.0f\n", r, g, b);
      Serial.printf("Matched Note %d\n", colorTable[index].note);

      // Serial.println("ON");
      note = colorTable[index].note;
      OscWiFi.send(oscHost, oscPort, "/note", note);

      button1Flag = 1;
    }
  }else
  if(button1Flag == 1)
  {
    if(button1Flag == 1)
    {
      OscWiFi.send(oscHost, oscPort, "/off", note);
    }
    button1Flag = 0;
    Serial.println("音を切りました");
  }

  if (button2State == LOW)
  {
    if(button2Flag == 0)
    {
      Serial.println("スイッチ2押されました");
      Serial.printf("Matched Note %d\n", colorTable[index2].note);
      // Serial.println("===== Color Data2 =====");
      // Serial.printf("Clear: %d  R: %d  G: %d  B: %d\n", clear2, red2, green2, blue2);
      Serial.printf("Normalized R: %.0f  G: %.0f  B: %.0f\n", r2, g2, b2);
      // Serial.printf("16bit Color: 0x%04X\n", _color2);
      // Serial.println("========================");

      note2 = colorTable[index2].note;
      OscWiFi.send(oscHost, oscPort, "/note", note2);

      button2Flag = 1;
    }
  }else if(button2Flag == 1)
  {
    if(button2Flag == 1)
    {
      OscWiFi.send(oscHost, oscPort, "/off", note2);
    }
    button2Flag = 0;
    Serial.println("音を切りました");
  }
  delay(10);
}
