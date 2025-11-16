/*
  Use this code to read IR commands from the remote
*/

// #include <Arduino.h>
// #include <IRremote.hpp>

// #define IR_RECEIVE_PIN 28

// void setup()
// {
//   Serial.begin(9600);
//   IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
// }

// void loop()
// {
//   if (IrReceiver.decode())
//   {
//     IrReceiver.printIRResultRawFormatted(&Serial, true);
//     Serial.println(F("---"));
//     IrReceiver.resume();
//   }
// }

/* END */


/*
  Main code
*/

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <DHT.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <IRremote.hpp>

#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define CYAN        0x07FF
#define YELLOW      0xFFE0
#define WHITE       0xFFFF

#define YP A2
#define XM A3
#define YM 8
#define XP 9

#define TS_MINX 907
#define TS_MAXX 136
#define TS_MINY 942
#define TS_MAXY 139
#define MINPRESSURE 200
#define MAXPRESSURE 1000

#define DHTPIN 22
#define DHTTYPE DHT11

#define RTC_SCLK 27  // CLK
#define RTC_IO   26  // DAT
#define RTC_CE   25  // RST

#define IR_PIN 23

#define X_OFFSET          180

#define TIME_X            30
#define TIME_Y            20
#define TIME_SIZE         2

#define TEMP_X            30
#define TEMP_Y            TIME_Y+30
#define TEMP_SIZE         2

#define HUM_X             30
#define HUM_Y             TEMP_Y+30
#define HUM_SIZE          2

#define AC_STATUS_X       30
#define AC_STATUS_Y       HUM_Y+30
#define AC_STATUS_SIZE    2

#define AC_BTN_X          10
#define AC_BTN_Y          AC_STATUS_Y+60
#define AC_BTN_W          80
#define AC_BTN_H          60
#define AC_BTN_TEXT_X     AC_BTN_X + 5
#define AC_BTN_TEXT_Y     AC_BTN_Y + 20
#define AC_BTN_TEXT_SIZE  2

#define FAN_BTN_X         AC_BTN_X + AC_BTN_W + 20
#define FAN_BTN_Y         AC_BTN_Y
#define FAN_BTN_W         AC_BTN_W
#define FAN_BTN_H         AC_BTN_H    
#define FAN_BTN_TEXT_X    FAN_BTN_X + 5
#define FAN_BTN_TEXT_Y    FAN_BTN_Y + 20
#define FAN_BTN_TEXT_SIZE 2

#define NUM_RETRIES       3
#define AUTO_TEMP_OFFSET  0

#define AC_STATUS_ON      0
#define AC_STATUS_OFF     1
#define AC_STATUS_AUTO    2

#define FAN_STATUS_ON     0
#define FAN_STATUS_OFF    1
#define FAN_STATUS_AUTO   2

#define CONFIG_BTN_X         FAN_BTN_X + FAN_BTN_W + 20
#define CONFIG_BTN_Y         FAN_BTN_Y
#define CONFIG_BTN_W         AC_BTN_W
#define CONFIG_BTN_H         AC_BTN_H
#define CONFIG_BTN_TEXT_X    CONFIG_BTN_X + 5
#define CONFIG_BTN_TEXT_Y    CONFIG_BTN_Y + 20
#define CONFIG_BTN_TEXT_SIZE 2

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
DHT dht(DHTPIN, DHTTYPE);
ThreeWire myWire(RTC_IO, RTC_SCLK, RTC_CE); 
RtcDS1302<ThreeWire> Rtc(myWire);

int pixel_x, pixel_y;
uint8_t acMode = AC_STATUS_AUTO; // 0=ON, 1=OFF, 2=AUTO
uint8_t fanMode = FAN_STATUS_AUTO; // 0=ON, 1=OFF, 2=AUTO
bool is_ac_on = false;
unsigned long lastUpdate = 0;
bool is_synced = false;

unsigned long lastTouchTime = 0;
const unsigned long SCREEN_TIMEOUT = 60000;
bool isDisplaySleeping = false;

float temp;
float hum;
RtcDateTime now;

uint16_t ON_rawData[60] = {  // REPLACE THESE WITH YOUR CODES
  8750, 4050,
  550, 1500, 500, 550, 500, 500, 550, 500,
  500, 1550, 500, 500, 500, 500, 550, 500,
  500, 500, 550, 500, 450, 550, 500, 550,
  500, 500, 500, 1550, 500, 500, 550, 500,
  500, 1550, 500, 1500, 550, 1500, 500, 1550,
  500, 500, 500, 1550, 550, 500, 500, 500,
  500, 550, 500, 1550, 500, 1500, 500, 1550,
  500
};
uint8_t ON_rawDataLength = 60;

uint16_t OFF_rawData[60] = {  // REPLACE THESE WITH YOUR CODES
  8700, 4100,
  550, 1550, 500, 500, 500, 500, 550, 500,
  500, 1550, 500, 500, 550, 500, 500, 500,
  500, 1550, 500, 1500, 550, 500, 550, 500,
  500, 500, 500, 550, 500, 500, 500, 500,
  550, 500, 500, 500, 500, 500, 550, 500,
  500, 500, 500, 1550, 500, 500, 550, 1500,
  550, 500, 500, 500, 500, 500, 550, 1550,
  500
};
uint8_t OFF_rawDataLength = 60;

uint8_t AUTO_MIN_TEMP = 21 + AUTO_TEMP_OFFSET, 
        AUTO_MAX_TEMP = 22 + AUTO_TEMP_OFFSET,
        AUTO_MIN_HOUR = 6,
        AUTO_MAX_HOUR = 20,
        AUTO_MIN_HUM = 30;

bool inConfigMode = false;
uint8_t selectedConfigIndex = 0;

const char* configLabels[] = {
  "Min Temp", "Max Temp", "Min Hour", "Max Hour", "Min Hum"
};
const int NUM_CONFIGS = 5;

void syncClock();
void drawScreen();
void drawACButton();
void drawFanButton();
void drawConfigButton();
void updateSensors();
void turnACOn();
void turnACOff();
void turnFanOn();
void turnFanOff();
void sleepDisplay();
void wakeDisplay();
void adjustConfigValue(int delta);
void handleConfigTouch(int x, int y);
void drawConfigMenu();

void setup()
{
  Serial.begin(9600);
  pinMode(IR_PIN, OUTPUT);
  dht.begin();

  IrSender.begin(IR_PIN);

  if (is_synced)
  {
    Rtc.Begin();

    if (!Rtc.GetIsRunning())
    {
      Serial.println("RTC not running, starting now...");
      Rtc.SetIsRunning(true);
    }
  }
  else
  {
    syncClock();
  }
  
  uint16_t ID = tft.readID();
  tft.begin(ID);
  tft.setRotation(3);
  tft.fillScreen(BLACK);

  drawScreen();
}

void loop()
{
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  bool touched = (p.z > MINPRESSURE && p.z < MAXPRESSURE);

  if (touched)
  {
    if (isDisplaySleeping)
    {
      wakeDisplay();
    }

    lastTouchTime = millis();

    pixel_x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    pixel_y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);

    if (inConfigMode)
    {
      handleConfigTouch(pixel_x, pixel_y);
      delay(200);
      return;
    }

    if (pixel_x > AC_BTN_X && pixel_x < (AC_BTN_X + AC_BTN_W) &&
        pixel_y > AC_BTN_Y && pixel_y < (AC_BTN_Y + AC_BTN_H))
    {
      acMode = (acMode + 1) % 3;
      drawACButton();
      delay(300);
    }
    else if (pixel_x > FAN_BTN_X && pixel_x < (FAN_BTN_X + FAN_BTN_W) &&
            pixel_y > FAN_BTN_Y && pixel_y < (FAN_BTN_Y + FAN_BTN_H))
    {
      fanMode = (fanMode + 1) % 3;
      drawFanButton();
      delay(300);
    }
    else if (pixel_x > CONFIG_BTN_X && pixel_x < (CONFIG_BTN_X + CONFIG_BTN_W) &&
            pixel_y > CONFIG_BTN_Y && pixel_y < (CONFIG_BTN_Y + CONFIG_BTN_H))
    {
      inConfigMode = true;
      drawConfigMenu();
      delay(300);
    }
  }

  if (!isDisplaySleeping && (millis() - lastTouchTime > SCREEN_TIMEOUT))
  {
    sleepDisplay();
  }

  temp = dht.readTemperature();
  hum  = dht.readHumidity();
  now  = Rtc.GetDateTime();

  if (!isDisplaySleeping && !inConfigMode && millis() - lastUpdate > 2000)
  {
    updateSensors();
    lastUpdate = millis();
  }

  if (acMode == AC_STATUS_AUTO && (now.Hour() >= AUTO_MIN_HOUR && now.Hour() <= AUTO_MAX_HOUR))
  {
    if ((temp >= AUTO_MAX_TEMP || hum <= AUTO_MIN_HUM) && is_ac_on)
    {
      turnACOff();
    }

    if (temp <= AUTO_MIN_TEMP && !is_ac_on)
    {
      turnACOn();
    }
  }

  if (is_ac_on && (now.Hour() < AUTO_MIN_HOUR || now.Hour() > AUTO_MAX_HOUR))
  {
    turnACOff();
    delay(1000);
  }
}

void sleepDisplay()
{
  tft.fillScreen(BLACK);
  isDisplaySleeping = true;
  Serial.println("Display sleeping...");
}

void wakeDisplay()
{
  tft.fillScreen(BLACK);
  drawScreen();
  isDisplaySleeping = false;
  Serial.println("Display awake!");
}

void drawScreen()
{
  tft.fillScreen(BLACK);

  updateSensors();
  drawACButton();
  drawFanButton();
  drawConfigButton();
}

void updateSensors()
{
  int16_t x1, y1;
  uint16_t w, h;

  tft.setTextSize(TIME_SIZE);
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u", now.Hour(), now.Minute());

  tft.getTextBounds("Time: 00:00", TIME_X+X_OFFSET, TIME_Y, &x1, &y1, &w, &h);
  tft.fillRect(x1, y1, w, h, BLACK);          // clear exact area
  tft.setCursor(TIME_X, TIME_Y);
  tft.setTextColor(CYAN);
  tft.print("Time: ");
  tft.setCursor(TIME_X + X_OFFSET, TIME_Y);
  tft.print(timeBuf);

  tft.setTextSize(TEMP_SIZE);
  char tempBuf[8];
  if (isnan(temp)) strcpy(tempBuf, "--");
  else dtostrf(temp, 4, 1, tempBuf);          // float → string safely

  tft.getTextBounds("Temperature: 00.0C", TEMP_X+X_OFFSET, TEMP_Y, &x1, &y1, &w, &h);
  tft.fillRect(x1, y1, w, h, BLACK);          // clear exact area

  tft.setCursor(TEMP_X, TEMP_Y);
  tft.setTextColor(YELLOW);
  tft.print("Temperature: ");
  tft.setCursor(TEMP_X + X_OFFSET, TEMP_Y);
  tft.print(tempBuf);
  tft.print("C");

  tft.setTextSize(HUM_SIZE);
  char humBuf[6];
  if (isnan(hum)) strcpy(humBuf, "--");
  else snprintf(humBuf, sizeof(humBuf), "%d", (int)hum);

  tft.getTextBounds("Humidity: 100%", HUM_X+X_OFFSET, HUM_Y, &x1, &y1, &w, &h);
  tft.fillRect(x1, y1, w, h, BLACK);          // clear exact area

  tft.setCursor(HUM_X, HUM_Y);
  tft.setTextColor(CYAN);
  tft.print("Humidity: ");
  tft.setCursor(HUM_X + X_OFFSET, HUM_Y);
  tft.print(humBuf);
  tft.print("%");

  tft.setTextSize(AC_STATUS_SIZE);
  char statusBuf[4];
  if (is_ac_on == false)
    snprintf(statusBuf, sizeof(statusBuf), "OFF");
  else
    snprintf(statusBuf, sizeof(statusBuf), "ON");

  tft.getTextBounds("AC Status: OFF", AC_STATUS_X+X_OFFSET, AC_STATUS_Y, &x1, &y1, &w, &h);
  tft.fillRect(x1, y1, w, h, BLACK);          // clear exact area

  tft.setCursor(AC_STATUS_X, AC_STATUS_Y);
  tft.setTextColor(YELLOW);
  tft.print("AC Status: ");
  tft.setCursor(AC_STATUS_X + X_OFFSET, AC_STATUS_Y);
  tft.print(statusBuf);
}

void drawACButton()
{
  String label;
  uint16_t color;

  if (acMode == AC_STATUS_ON)
  {
    label = "ON";
    color = GREEN;
    
    turnACOn();
  }
  else if (acMode == AC_STATUS_OFF)
  {
    label = "OFF";
    color = RED;
    
    turnACOff();
  }
  else
  {
    label = "AUTO";
    color = BLUE;
  }

  tft.fillRect(AC_BTN_X, AC_BTN_Y, AC_BTN_W, AC_BTN_H, color);
  tft.drawRect(AC_BTN_X, AC_BTN_Y, AC_BTN_W, AC_BTN_H, WHITE);

  tft.setTextColor(WHITE);
  tft.setTextSize(AC_BTN_TEXT_SIZE);
  tft.setCursor(AC_BTN_TEXT_X, AC_BTN_TEXT_Y-40);
  tft.print("AC");
  tft.setCursor(AC_BTN_TEXT_X, AC_BTN_TEXT_Y);
  tft.print(label);
}

void drawFanButton()
{
  String label;
  uint16_t color;

  if (fanMode == FAN_STATUS_ON)
  {
    label = "ON";
    color = GREEN;
    
    turnFanOn();
  }
  else if (fanMode == FAN_STATUS_OFF)
  {
    label = "OFF";
    color = RED;
    
    turnFanOff();
  }
  else
  {
    label = "AUTO";
    color = BLUE;
  }

  tft.fillRect(FAN_BTN_X, FAN_BTN_Y, FAN_BTN_W, FAN_BTN_H, color);
  tft.drawRect(FAN_BTN_X, FAN_BTN_Y, FAN_BTN_W, FAN_BTN_H, WHITE);

  tft.setTextColor(WHITE);
  tft.setTextSize(FAN_BTN_TEXT_SIZE);
  tft.setCursor(FAN_BTN_TEXT_X, FAN_BTN_TEXT_Y-40);
  tft.print("FAN");
  tft.setCursor(FAN_BTN_TEXT_X, FAN_BTN_TEXT_Y);
  tft.print(label);
}

void drawConfigButton()
{
  tft.fillRect(CONFIG_BTN_X, CONFIG_BTN_Y, CONFIG_BTN_W, CONFIG_BTN_H, YELLOW);
  tft.drawRect(CONFIG_BTN_X, CONFIG_BTN_Y, CONFIG_BTN_W, CONFIG_BTN_H, WHITE);
  
  tft.setTextColor(BLACK);
  tft.setTextSize(CONFIG_BTN_TEXT_SIZE);
  tft.setCursor(CONFIG_BTN_TEXT_X, CONFIG_BTN_TEXT_Y - 40);
  tft.print("CONFIG");
  tft.setCursor(CONFIG_BTN_TEXT_X, CONFIG_BTN_TEXT_Y);
  tft.print("MENU");
}

void drawConfigMenu()
{
  tft.fillScreen(BLACK);
  tft.setTextSize(2);

  for (int i = 0; i < NUM_CONFIGS; i++)
  {
    int y = 10 + i * 30;
    tft.setCursor(20, y);
    tft.setTextColor(i == selectedConfigIndex ? YELLOW : WHITE);
    tft.print(configLabels[i]);
    tft.setCursor(190, y);

    switch (i)
    {
      case 0: tft.print(AUTO_MIN_TEMP); break;
      case 1: tft.print(AUTO_MAX_TEMP); break;
      case 2: tft.print(AUTO_MIN_HOUR); break;
      case 3: tft.print(AUTO_MAX_HOUR); break;
      case 4: tft.print(AUTO_MIN_HUM); break;
    }
  }

  int btnY = 190;
  tft.fillRect(30, btnY, 60, 40, BLUE);  // UP
  tft.setTextColor(WHITE);
  tft.setCursor(55, btnY + 10);
  tft.print("+");

  tft.fillRect(110, btnY, 60, 40, BLUE); // DOWN
  tft.setCursor(135, btnY + 10);
  tft.print("-");

  tft.fillRect(190, btnY, 100, 40, GREEN); // BACK
  tft.setCursor(210, btnY + 10);
  tft.print("BACK");
}

void handleConfigTouch(int x, int y)
{
    int btnY = 190;
    int btnH = 40;

    if (y > btnY && y < btnY + btnH)
    {
        if (x > 30 && x < 90) // UP
        {
            adjustConfigValue(1);
        }
        else if (x > 110 && x < 170) // DOWN
        {
            adjustConfigValue(-1);
        }
        else if (x > 190 && x < 290) // BACK
        {
            inConfigMode = false;
            drawScreen();
        }
    }
    else
    {
        for (int i = 0; i < NUM_CONFIGS; i++)
        {
            int yTop = 10 + i * 30;
            if (y > yTop && y < yTop + 25)
            {
                selectedConfigIndex = i;
                drawConfigMenu();
                break;
            }
        }
    }
}

void adjustConfigValue(int delta)
{
  switch (selectedConfigIndex)
  {
    case 0: AUTO_MIN_TEMP = constrain(AUTO_MIN_TEMP + delta, 10, 35); break;
    case 1: AUTO_MAX_TEMP = constrain(AUTO_MAX_TEMP + delta, 10, 35); break;
    case 2: AUTO_MIN_HOUR = constrain(AUTO_MIN_HOUR + delta, 0, 23); break;
    case 3: AUTO_MAX_HOUR = constrain(AUTO_MAX_HOUR + delta, 0, 23); break;
    case 4: AUTO_MIN_HUM = constrain(AUTO_MIN_HUM + delta, 10, 90); break;
  }
  drawConfigMenu();
}

void printDateTime(const RtcDateTime& dt)
{
  char datestring[26];

  snprintf_P(datestring, 
    countof(datestring),
    PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
    dt.Month(),
    dt.Day(),
    dt.Year(),
    dt.Hour(),
    dt.Minute(),
    dt.Second() );

  Serial.print(datestring);
}

void syncClock()
{
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) 
  {
    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) 
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled) 
  {
    Serial.println("RTC is newer than compile time.");
    Rtc.SetDateTime(compiled);
  }
  else if (now == compiled) 
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
}

void turnACOn()
{
  for (int i = 0; i < NUM_RETRIES; i++)
  {
    IrSender.sendRaw(ON_rawData, ON_rawDataLength, 38);
    delay(100);
  }

  is_ac_on = true;
  Serial.println("AC should turn on");
}

void turnACOff()
{
  for (int i = 0; i < 3; i++)
  {
    IrSender.sendRaw(OFF_rawData, OFF_rawDataLength, 38);
    delay(100);
  }

  is_ac_on = false;
  Serial.println("AC should turn off");
}

void turnFanOn()
{
  Serial.println("Fan should turn on");
}

void turnFanOff()
{
  Serial.println("Fan should turn off");
}

/* END */