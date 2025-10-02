/***************************************************
  名前: ソレノイドラム
  基板: Arduino MEGA
****************************************************/
#include <FastLED.h>

// オートの切り替え
#define AUTOBTN_PIN 30
#define STARTRECBTN_PIN 32

#define MAIN_LED_NUM 4
#define SOLENOID_LED_NUM 4

bool isManualMode = false;

//本体
constexpr int MainLEDPins[] = {36, 37, 38, 39};

//ソレノイド
constexpr int SolenoidLEDPins[] = {40, 41, 42, 43};

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

int brightness = 0;
int brightnessStep = 4;
int ledBrightNum = 4;

CRGB leds[MAIN_LED_NUM];
CRGB leds2[MAIN_LED_NUM];
CRGB leds3[MAIN_LED_NUM];
CRGB leds4[MAIN_LED_NUM];
CRGB* mainLEDs[] = {leds, leds2, leds3, leds4};

CRGB leds5[SOLENOID_LED_NUM];
CRGB leds6[SOLENOID_LED_NUM];
CRGB leds7[SOLENOID_LED_NUM];
CRGB leds8[SOLENOID_LED_NUM];
CRGB* solenoidLEDs[] = {leds5, leds6, leds7, leds8};

#define DELAYVAL 100

const int Solenoids[] = {26,27,28,29};
const int solenoidNum = sizeof(Solenoids)/sizeof(Solenoids[0]);

const int rhythmbtns[solenoidNum] = {4,5,6,7};
int rhythmbtnsStates[solenoidNum] = {0, 0};
int rhythmbtnsLastStates[solenoidNum] = {0, 0};

const int timingSampleMaxNum = 100;
int timingCounts[solenoidNum] = {0, 0};
unsigned long timing[solenoidNum][timingSampleMaxNum] = {};

unsigned long startTime = 0;
unsigned long recStartTime = 0;
bool isRec = false;
bool isStart = false;

unsigned long lastPressBtnTime = 0;
unsigned long playWaitTime = 0;
unsigned long playLoopEndTime = 0;
unsigned long waitStartTime = 0;
bool isWaiting = false;

/* ------------------------------------------------------------ */
/* 初期化 */
/* ------------------------------------------------------------ */
void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < sizeof(Solenoids)/sizeof(Solenoids[0]); i++)
  {
    pinMode(Solenoids[i], OUTPUT);
  }
  for (int i = 0; i < sizeof(rhythmbtns)/sizeof(rhythmbtns[0]); i++)
  {
    pinMode(rhythmbtns[i], INPUT_PULLUP);
  }
  pinMode(STARTRECBTN_PIN, INPUT_PULLUP);
  pinMode(AUTOBTN_PIN, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, MainLEDPins[0], COLOR_ORDER>(mainLEDs[0], MAIN_LED_NUM).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, MainLEDPins[1], COLOR_ORDER>(mainLEDs[1], MAIN_LED_NUM).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, MainLEDPins[2], COLOR_ORDER>(mainLEDs[2], MAIN_LED_NUM).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, MainLEDPins[3], COLOR_ORDER>(mainLEDs[3], MAIN_LED_NUM).setCorrection(TypicalLEDStrip);
  
  FastLED.addLeds<LED_TYPE, SolenoidLEDPins[0], COLOR_ORDER>(solenoidLEDs[0], SOLENOID_LED_NUM).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, SolenoidLEDPins[1], COLOR_ORDER>(solenoidLEDs[1], SOLENOID_LED_NUM).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, SolenoidLEDPins[2], COLOR_ORDER>(solenoidLEDs[2], SOLENOID_LED_NUM).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, SolenoidLEDPins[3], COLOR_ORDER>(solenoidLEDs[3], SOLENOID_LED_NUM).setCorrection(TypicalLEDStrip);
}

/* ------------------------------------------------------------ */
/* メインループ */
/* ------------------------------------------------------------ */
void loop()
{
  // 記録モード
  if (digitalRead(STARTRECBTN_PIN) == HIGH && !isRec) {
    recStartTime = millis();
    for (int i = 0; i < solenoidNum; i++) {
      for (int j = 0; j < timingSampleMaxNum; j++) {
        timing[i][j] = 0;
      }
    }
    for (int i = 0; i < solenoidNum; i++) {
      timingCounts[i] = 0;
    }
    isRec = true;
    isStart = false;
    Serial.println("REC start");
  }
  // 再生モード
  else if (digitalRead(STARTRECBTN_PIN) == LOW && !isStart) {
    for (int i = 0; i < solenoidNum; i++) {
      for (int j = 0; j < timingSampleMaxNum; j++) {
        Serial.print(String(timing[i][j])+",");
      }
      Serial.println();
    }
    startTime = millis();
    for (int i = 0; i < solenoidNum; i++) {
      timingCounts[i] = 0;
    }
    isRec = false;
    isStart = true;
    Serial.println("Play start");
    playWaitTime = millis() - lastPressBtnTime;
    delay(playWaitTime);
  }

  // マニュアルモード
  if (digitalRead(AUTOBTN_PIN) == LOW) {
    isManualMode = true;

    for (int i = 0; i < sizeof(rhythmbtns)/sizeof(rhythmbtns[0]); i++) {
      if (digitalRead(rhythmbtns[i]) == LOW) {
        delay(10);
        if (digitalRead(rhythmbtns[i]) == LOW) {
          rhythmbtnsStates[i] = LOW;
          lastPressBtnTime = millis();
        }
      }
      else {
        rhythmbtnsStates[i] = HIGH;
      }
      
      if (rhythmbtnsStates[i] == LOW && rhythmbtnsLastStates[i] == HIGH) {
        digitalWrite(Solenoids[i], HIGH);
        delay(15);
        digitalWrite(Solenoids[i], LOW);
      }
      rhythmbtnsLastStates[i] = rhythmbtnsStates[i];
    }
  }
  else {
    if (isManualMode) {
      // 切り替わったときに実行する
      for (int i = 0; i < sizeof(rhythmbtns)/sizeof(rhythmbtns[0]); i++) {
        rhythmbtnsStates[i] = 0;
        rhythmbtnsLastStates[i] = 0;
      }

      isManualMode = false;
    }

    // 記録中
    if (isRec) {
      for (int i = 0; i < sizeof(rhythmbtns)/sizeof(rhythmbtns[0]); i++) {
        if (digitalRead(rhythmbtns[i]) == LOW) {
          delay(10);
          if (digitalRead(rhythmbtns[i]) == LOW) {
            rhythmbtnsStates[i] = LOW;
            lastPressBtnTime = millis();
          }
        }
        else {
          rhythmbtnsStates[i] = HIGH;
        }
        
        if (rhythmbtnsStates[i] == LOW && rhythmbtnsLastStates[i] == HIGH) {
          if (timingCounts[i] < timingSampleMaxNum) {
            timing[i][timingCounts[i]] = millis() - recStartTime;
            // Serial.println(timing[i][timingCounts[i]]);
            Serial.println(rhythmbtns[i]);
            timingCounts[i]++;
            Serial.println(timingCounts[i]);
            digitalWrite(Solenoids[i], HIGH);
            delay(15);
            digitalWrite(Solenoids[i], LOW);
          }
        }
        rhythmbtnsLastStates[i] = rhythmbtnsStates[i];
      }
    }

    // 再生中
    if (isStart) {
      for (int i = 0; i < solenoidNum; i++) {
        unsigned long currentPlayTime = millis() - startTime;
        unsigned long nextTiming = timing[i][timingCounts[i]];
        
        if (nextTiming != 0 && nextTiming <= currentPlayTime) {
          Serial.println("Hit");
          Serial.println(currentPlayTime);
          digitalWrite(Solenoids[i], HIGH);
          timingCounts[i]++;
        }

        // ソレノイドを100ms後にOFFにする処理
        if (timingCounts[i] > 0) {
          unsigned long lastHitTime = timing[i][timingCounts[i]-1];
          unsigned long timeSinceLastHit = currentPlayTime - lastHitTime;
          
          if (timeSinceLastHit >= 15) {
            digitalWrite(Solenoids[i], LOW);
          }
        }
      }

      int zeroCount = 0;
      for (int i = 0; i < solenoidNum; i++) {
        if (timing[i][timingCounts[i]] == 0) {
          zeroCount++;
        }
      }
      if (zeroCount == solenoidNum) {
        if (!isWaiting) {
          waitStartTime = millis();
          isWaiting = true;

        } else if (millis() - waitStartTime >= playWaitTime) {
          for (int i = 0; i < solenoidNum; i++) {
            timingCounts[i] = 0;
            // delay(15);
            digitalWrite(Solenoids[i], LOW);
          }
          startTime = millis();
          isWaiting = false;
        }
      }
    }
  }

  brightness += brightnessStep;
  if (brightness >= 255 || brightness <= 0)
  {
    brightnessStep *= -1;
    // 光る個数をランダムにする
    // ledBrightNum = random(0, MAIN_LED_NUM);
    ledBrightNum = random(1, MAIN_LED_NUM + 1);
    // すべて消灯する
    // for (int i = 0; i < sizeof(mainLEDs)/sizeof(mainLEDs[0]); i++) {
    //   for (int j = 0; j < MAIN_LED_NUM; j++) {
    //     mainLEDs[i][j] = CRGB(0, 0, 0);
    //   }
    // }
    // for (int i = 0; i < sizeof(solenoidLEDs)/sizeof(solenoidLEDs[0]); i++) {
    //   for (int j = 0; j < SOLENOID_LED_NUM; j++) {
    //     solenoidLEDs[i][j] = CRGB(0, 0, 0);
    //   }
    // }
  }
  uint8_t r, g, b;
  HSVtoRGB(240, 1, brightness / 255.0, r, g, b);

  // 本体用
  // for (int i = 0; i < sizeof(mainLEDs)/sizeof(mainLEDs[0]); i++) {
  //   for (int j = 0; j < random(0, ledBrightNum); j++) {
  //     mainLEDs[i][j] = CRGB(r, g, b);
  //   }
  // }

  for (int i = 0; i < sizeof(mainLEDs)/sizeof(mainLEDs[0]); i++) {
    for (int j = 0; j < ledBrightNum; j++) {
      mainLEDs[i][j] = CRGB(r, g, b);
    }
  }
  // ソレノイド用
  // for (int i = 0; i < sizeof(solenoidLEDs)/sizeof(solenoidLEDs[0]); i++) {
  //   for (int j = 0; j < random(0, ledBrightNum); j++) {
  //     solenoidLEDs[i][j] = CRGB(r, g, b);
  //   }
  // }
  for (int i = 0; i < sizeof(solenoidLEDs)/sizeof(solenoidLEDs[0]); i++) {
    for (int j = 0; j < ledBrightNum; j++) {
      solenoidLEDs[i][j] = CRGB(r, g, b);
    }
  }
  FastLED.show();
  
}

/*
--------------------------------------------------
HSV値をRGB値に変換する関数
--------------------------------------------------
*/
void HSVtoRGB(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
  float c = v * s;
  float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
  float m = v - c;
  float r1, g1, b1;

  if (h < 60) {
    r1 = c; g1 = x; b1 = 0;
  } else if (h < 120) {
    r1 = x; g1 = c; b1 = 0;
  } else if (h < 180) {
    r1 = 0; g1 = c; b1 = x;
  } else if (h < 240) {
    r1 = 0; g1 = x; b1 = c;
  } else if (h < 300) {
    r1 = x; g1 = 0; b1 = c;
  } else {
    r1 = c; g1 = 0; b1 = x;
  }

  r = (uint8_t)((r1 + m) * 255);
  g = (uint8_t)((g1 + m) * 255);
  b = (uint8_t)((b1 + m) * 255);
}
