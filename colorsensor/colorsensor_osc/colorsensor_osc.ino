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

struct Color {
  int r, g, b;
  int note;
};

Color colorTable[] =
{
  // {86, 75, 103, 1},
  {114, 64, 89, 44},
  {126, 65, 80, 45},
  {115, 73, 79, 49},
  {107, 87, 67, 50},
  {103, 93, 64, 51},
  {93, 107, 54, 52}, 
  {67, 110, 81, 53},
  {71, 106, 83, 54},
  {71, 109, 79, 55}, 
  {70, 119, 69, 56}, 
  {56, 121, 83, 57},
  {45, 111, 109, 58},
  {45, 111, 108, 59},
  {56, 95, 114, 60},
  {46, 98, 120, 61},
  {33, 106, 128, 62},
  {50, 100, 113, 63},
  {43, 95, 125, 64},
  {35, 96, 134, 65}, 
  {32, 96, 138, 66},
  {25, 88, 154, 67},
  {29, 91, 146, 68},
  {25, 82, 160, 69},
  {44, 91, 130, 70},
  {53, 92, 118, 71},
  {37, 80, 148, 72},
  {78, 72, 115, 73},
  {69, 73, 123, 75}, 
  {57, 87, 119, 76},
  {50, 83, 131, 77},
  {53, 85, 125, 78},
  {80, 89, 93, 80},
  {74, 79, 110, 82}

};

int note1;
int note2;

int count = sizeof(colorTable) / sizeof(colorTable[0]);

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

Adafruit_TCS34725 tcs1 = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

Adafruit_TCS34725 tcs2 = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

const int button1Pin = 12;
const int button2Pin = 14;

int button1Flag = 0;
int button2Flag = 0;

/* ---------------------------------------
 * 初期化
----------------------------------------*/
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
  wifiMillis = millis();
  Serial.println("WiFi OK");

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  setupColorSensors();

  Serial.println("Setup OK.");
}

void setupColorSensors()
{
  Wire.begin(21, 22);         // カラーセンサー1台目のピン
  Wire1.begin(32, 33);        // カラーセンサー2台目のピン

  Serial.println("Color View Test!");
  while (!tcs1.begin())
  {
    Serial.println("No TCS34725 found ... check your connections");
    delay(1000);
  }
  while (!tcs2.begin(TCS34725_ADDRESS, &Wire1))
  {
    Serial.println("2台目のカラーセンサーと接続できていません");
    delay(1000);
  }
  tcs1.setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);  // Sets theintegration time for the
                                                            
  tcs1.setGain(TCS34725_GAIN_4X);                           // Adjusts the gain on the TCS34725.

  tcs2.setIntegrationTime(TCS34725_INTEGRATIONTIME_154MS);
  tcs2.setGain(TCS34725_GAIN_4X);
}

/* ---------------------------------------
 * ループ
----------------------------------------*/
void loop()
{
  // if (WiFi.status() != WL_CONNECTED && millis() - wifiMillis >= 1000)
  // {
  //   WiFi.disconnect();
  //   WiFi.reconnect();
  //   wifiMillis = millis();
  //   // Serial.println("reconnect");
  //   wifiReconnectCount++;

  //   if (wifiReconnectCount > 10)
  //   {
  //     ESP.restart();
  //   }
  // }

  int button1State = digitalRead(button1Pin);
  int button2State = digitalRead(button2Pin);

  uint16_t r1, g1, b1, c1, r2, g2, b2, c2;
  tcs1.getRawData(&r1, &g1, &b1, &c1);  // Reads the raw red, green, blue and clear channel
  tcs2.getRawData(&r2, &g2, &b2, &c2);  // Reads the raw red, green, blue and clear channel

  int R, G, B, R2, G2, B2;
  normalizeColor(r1, g1, b1, c1, R, G, B);
  normalizeColor(r2, g2, b2, c2, R2, G2, B2);

  int idx1 = findColor(R, G, B);
  int idx2 = findColor(R2, G2, B2);

  if (button1State == LOW && button1Flag == 0 && idx1 >= 0)
  {
    note1 = colorTable[idx1].note;
    OscWiFi.send(oscHost, oscPort, "/note", note1);
    button1Flag = 1;
  } 
  else if (button1State == HIGH && button1Flag == 1)
  {
    OscWiFi.send(oscHost, oscPort, "/off", note1);
    button1Flag = 0;
  }

  if (button2State == LOW && button2Flag == 0 && idx2 >= 0)
  {
    note2 = colorTable[idx2].note;
    OscWiFi.send(oscHost, oscPort, "/note", note2);
    button2Flag = 1;
  } 
  else if (button2State == HIGH && button2Flag == 1)
  {
    OscWiFi.send(oscHost, oscPort, "/off", note2);
    button2Flag = 0;
  }
  // delay(5);
}

void normalizeColor(uint16_t rRaw, uint16_t gRaw, uint16_t bRaw, uint16_t cRaw, int &R, int &G, int &B)
{
  if (cRaw == 0) cRaw = 1;
  R = rRaw * 255.0 / cRaw;
  G = gRaw * 255.0 / cRaw;
  B = bRaw * 255.0 / cRaw;
}

int findColor(int R, int G, int B)
{
  int bestIndex = -1;
  long bestDist = LONG_MAX;

  for (int i = 0; i < count; i++)
  {
    long dist = sq(R - colorTable[i].r)
              + sq(G - colorTable[i].g)
              + sq(B - colorTable[i].b);
    if (dist < bestDist)
    {
      bestDist = dist;
      bestIndex = i;
    }
  }
  return (bestDist < 4000) ? bestIndex : -1;
}
