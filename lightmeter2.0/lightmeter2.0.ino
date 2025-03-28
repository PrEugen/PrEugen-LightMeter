#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BH1750.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_DC                 11
#define OLED_CS                 12
#define OLED_CLK                8 //10
#define OLED_MOSI               9 //9
#define OLED_RESET              10 //13
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

BH1750 lightMeter;

#define DomeMultiplier          2.17
#define MeteringButtonPin       2     // 测光
#define PlusButtonPin           6     //加
#define MinusButtonPin          5     //减
#define ModeButtonPin           4     //AS
#define MenuButtonPin           7     //菜单
#define MeteringModeButtonPin   0
//#define PowerButtonPin          2

// 修改后的参数范围
#define MaxISOIndex             20  // ISO最高到1600
#define MaxApertureIndex        29  // f/1.1到f/32
#define MaxTimeIndex            80
#define MaxNDIndex              13
#define MaxFlashMeteringTime    5000

float   lux;
boolean Overflow = 0;
float   ISOND;
boolean ISOmode = 0;
boolean NDmode = 0;

boolean PlusButtonState;
boolean MinusButtonState;
boolean MeteringButtonState;
boolean ModeButtonState;
boolean MenuButtonState;
boolean MeteringModeButtonState;

boolean ISOMenu = false;
boolean NDMenu = false;
boolean mainScreen = false;

// EEPROM地址
#define ISOIndexAddr        1
#define apertureIndexAddr   2
#define modeIndexAddr       3
#define T_expIndexAddr      4
#define meteringModeAddr    5
#define ndIndexAddr         6

#define defaultApertureIndex 12
#define defaultISOIndex      11
#define defaultModeIndex     0
#define defaultT_expIndex    19

uint8_t ISOIndex =          EEPROM.read(ISOIndexAddr);
uint8_t apertureIndex =     EEPROM.read(apertureIndexAddr);
uint8_t T_expIndex =        EEPROM.read(T_expIndexAddr);
uint8_t modeIndex =         EEPROM.read(modeIndexAddr);
uint8_t meteringMode =      EEPROM.read(meteringModeAddr);
uint8_t ndIndex =           EEPROM.read(ndIndexAddr);

int battVolts;
#define batteryInterval 10000
double lastBatteryTime = 0;

#include "lightmeter.h"

void setup() {
  pinMode(PlusButtonPin, INPUT_PULLUP);
  pinMode(MinusButtonPin, INPUT_PULLUP);
  pinMode(MeteringButtonPin, INPUT_PULLUP);
  pinMode(ModeButtonPin, INPUT_PULLUP);
  pinMode(MenuButtonPin, INPUT_PULLUP);
  pinMode(MeteringModeButtonPin, INPUT_PULLUP);

  //Serial.begin(115200);

  battVolts = getBandgap();

  Wire.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE_2);
  //lightMeter.begin(BH1750::ONE_TIME_LOW_RES_MODE); // for low resolution but 16ms light measurement time.

  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.setTextColor(WHITE);
  display.clearDisplay();

  // 初始化检查
  if (apertureIndex > MaxApertureIndex) apertureIndex = 12;
  if (ISOIndex > MaxISOIndex) ISOIndex = 11;
  if (T_expIndex > MaxTimeIndex) T_expIndex = 19;
  if (modeIndex > 1) modeIndex = 0;
  if (meteringMode > 1) meteringMode = 0;
  if (ndIndex > MaxNDIndex) ndIndex = 0;

  lux = getLux();
  refresh();
}

void loop() {
  if (millis() >= lastBatteryTime + batteryInterval) {
    lastBatteryTime = millis();
    battVolts = getBandgap();
  }

  readButtons();
  menu();

  if (MeteringButtonState == 0) {
    SaveSettings();
    lux = 0;
    refresh();

    if (meteringMode == 0) {
      lightMeter.configure(BH1750::ONE_TIME_HIGH_RES_MODE_2);
      lux = getLux();
      if (Overflow == 1) {
        delay(10);
        getLux();
      }
      refresh();
      delay(200);
    } else {
      lightMeter.configure(BH1750::CONTINUOUS_LOW_RES_MODE);

      unsigned long startTime = millis();
      uint16_t currentLux = 0;
      lux = 0;

      while (true) {
        // check max flash metering time
        if (startTime + MaxFlashMeteringTime < millis()) {
          break;
        }

        currentLux = getLux();
        delay(16);

        if (currentLux > lux) {
          lux = currentLux;
        }
      }

      refresh();
    }
  }
}
