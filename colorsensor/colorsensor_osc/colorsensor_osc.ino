#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <WiFi.h>
#include <ArduinoOSCWiFi.h>

#define SENSOR1
// #define SENSOR2

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

bool tcs1_ok = false;
bool tcs2_ok = false;

struct Color {
  int r, g, b;
  int note;
};

Color colorTable[] = {
  {112, 65, 89, 44},
  {123, 64, 80, 45},
  {114, 72, 78, 49},
  {105, 89, 66, 50},
  {103, 91, 64, 51},
  {94, 107, 53, 52},
  {66, 110, 82, 53},
  {71, 105, 83, 54},
  {71, 109, 79, 55},
  {70, 118, 69, 56},
  {56, 122, 82, 57},
  {44, 112, 108, 58},
  {46, 109, 107, 59},
  {56, 93, 113, 60},
  {47, 97, 119, 61},
  {34, 97, 126, 62},
  {49, 90, 113, 63},
  {43, 95, 124, 64},
  {34, 95, 134, 65},
  {32, 95, 137, 66},
  {25, 88, 153, 67},
  {28, 91, 146, 68},
  {25, 82, 160, 69},
  {45, 83, 128, 70},
  {53, 92, 117, 71},
  {36, 81, 147, 72},
  {76, 67, 114, 73},
  {68, 65, 124, 75},
  {56, 78, 120, 76},
  {48, 74, 131, 77},
  {53, 84, 125, 78},
  {76, 83, 94, 80},
  {72, 71, 110, 82}
};
int count = sizeof(colorTable) / sizeof(colorTable[0]);

Adafruit_TCS34725 tcs1 = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Adafruit_TCS34725 tcs2 = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

const int button1Pin = 12;
const int button2Pin = 14;

bool button1Flag = false;
bool button2Flag = false;

int note1;
int note2;


unsigned long wifiMillis = 0;
int wifiReconnectCount = 0;

// #define commonAnode true  // set to false if using a common cathode LED.
// byte gammatable[256];  // our RGB -> eye-recognized gamma color

// static uint16_t color16(uint16_t r, uint16_t g, uint16_t b)
// {
//     uint16_t _color;
//     _color = (uint16_t)(r & 0xF8) << 8;
//     _color |= (uint16_t)(g & 0xFC) << 3;
//     _color |= (uint16_t)(b & 0xF8) >> 3;
//     return _color;
// }

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

  setupColorSensors();

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);


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
  Serial.println("ColorSensor OK");
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

  // uint16_t r1, g1, b1, c1, r2, g2, b2, c2;
  // tcs1.getRawData(&r1, &g1, &b1, &c1);  // Reads the raw red, green, blue and clear channel
  // tcs2.getRawData(&r2, &g2, &b2, &c2);  // Reads the raw red, green, blue and clear channel

  // int R, G, B, R2, G2, B2;
  // normalizeColor(r1, g1, b1, c1, R, G, B);
  // normalizeColor(r2, g2, b2, c2, R2, G2, B2);


  if (button1State == LOW && !button1Flag)
  {
    delay(5);
    if (digitalRead(button1Pin) == LOW)
    {
      uint16_t r, g, b, c;
      tcs1.getRawData(&r, &g, &b, &c);
      int R, G, B;
      normalizeColor(r, g, b, c, R, G, B);

      int idx = findColor(R, G, B);

      if(idx >= 0)
      {
        note1 = colorTable[idx].note;
        OscWiFi.send(oscHost, oscPort, "/note", note1);
        button1Flag = true;
      }
    }
  } 

  if (button1State == HIGH && button1Flag)
  {
    delay(5);
    if(digitalRead(button1Pin) == HIGH)
    {
      OscWiFi.send(oscHost, oscPort, "/off", note1);
      button1Flag = false;
    }
  }

  if (button2State == LOW && !button2Flag)
  {
    delay(5);
    if (digitalRead(button2Pin) == LOW)
    {
      uint16_t r, g, b, c;
      tcs2.getRawData(&r, &g, &b, &c);
      int R, G, B;
      normalizeColor(r, g, b, c, R, G, B);

      int idx = findColor(R, G, B);

      if(idx >= 0)
      {
        note2 = colorTable[idx].note;
        OscWiFi.send(oscHost, oscPort, "/note", note2);
        button2Flag = true;
      }

    }
  } 

  if (button2State == HIGH && button2Flag)
  {
    delay(5);
    if(digitalRead(button2Pin) == HIGH)
    {
      OscWiFi.send(oscHost, oscPort, "/off", note2);
      button2Flag = false;
    }
  }

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
