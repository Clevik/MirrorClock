#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Timer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <mainpage.h>

#ifdef __AVR__
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#else
  #define PROGMEM
#endif

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
  #include <SPI.h>
#endif


/*
  DEFINE
*/

// Remove this line and inserted WiFi settings below
#include "personalconfig.h"

#define VER "1.1"

// WiFi settings
#ifndef STASSID
  #define STASSID "SSID"
  #define STAPSK  "PASSWORD"
#endif

// Timezone
#ifndef DEFAULTTIMEZONE
  #define DEFAULTTIMEZONE 0
#endif

// Set time when boot. Uses build time as default.
//#define SETTIME

// Run demonstration mode. Watch faces do change every 30 seconds.
//#define DEMOMODE


// OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

uint8_t displayWidth;
uint8_t displayHeight;

// ICONS
// All set:
// u8g2_font_open_iconic_all_1x_t
// Partial sets:
// u8g2_font_open_iconic_www_1x_t
// u8g2_font_open_iconic_www_4x_t
// u8g2_font_open_iconic_weather_2x_t
// u8g2_font_open_iconic_embedded_2x_t

uint16_t icons[13]  = {
  0x0045,   // Broken chain, open_iconic_www_1x     - WiFi disconnected
  0x0048,   // WiFi, open_iconic_www_1x             - WiFi connected
  0x004E,   // Globe, open_iconic_www_1x            - Update data from internet in progress (get weather data)
  0x004F,   // Chain, open_iconic_www_1x            - Connection to WiFi in progress (change with chain animation)
  0x0051,   // Another WiFi, open_iconic_www_1x     - WiFi Connected
  0x0053,   // Two way arrows, open_iconic_www_1x   - Local server get request/transfer data
  0x0040,   // Cloudy, open_iconic_weather_2x|open_iconic_weather_2x2
  0x0041,   // Partial cloudy, open_iconic_weather_2x|open_iconic_weather_2x2
  0x0042,   // Moonlight, open_iconic_weather_2x|open_iconic_weather_2x2
  0x0043,   // Rain,  open_iconic_weather_2x|open_iconic_weather_2x2
  0x0045,   // Sunshine, open_iconic_weather_2x|open_iconic_weather_2x2
  0x0043,   // Upload, open_iconic_www_4x_t         - Update firmware OTA
  0x004E    // Power icon, open_iconic_embedded_2x  - Reboot ESP
};

// Time
tmElements_t localTime;
bool timeCorrect;
int8_t timezone = DEFAULTTIMEZONE;

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *dayName [7] = {
  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

// Design config
bool firmwareUpdateOTA = false;
bool transferData = false;
bool rebooting = false;
bool uploadingError = false;

const uint8_t totalWatchFaces = 6;
uint8_t currentWatchFace = 6;

uint8_t clockCenterX = 31;
uint8_t clockCenterY = 31;
uint8_t clockRad = 23;
uint8_t uploadingErrorCode = 0;

HTTPUploadStatus uploadStatus;

// Other valiables
enum align {
  left,
  center,
  right
};

// Timers
Timer updateCurrentTimeTimer;
Timer displayCurrentTimeTimer;
#ifdef DEMOMODE
Timer changeWatchFaceTimer;
#endif

// Network
const char *ssid = STASSID;
const char *password = STAPSK;
ESP8266WebServer server(80);
MainPage page;

/*
  Set time functions
*/


#ifdef SETTIME

bool getTime(const char *str) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) {
    return false;
  }

  localTime.Hour = Hour;
  localTime.Minute = Min;
  localTime.Second = Sec;
  return true;
}

bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) {
    return false;
  }

  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) {
      break;
    }
  }

  if (monthIndex >= 12) {
    return false;
  }

  localTime.Day = Day;
  localTime.Month = monthIndex + 1;
  localTime.Year = CalendarYrToTm(Year);
  return true;
}

bool getDayOfWeek(const char *str) {
  int dayOfWeek;

  if (sscanf(str, "%d", &dayOfWeek) != 1) {
    return false;
  }

  if (dayOfWeek < 1 || dayOfWeek > 7) {
    return false;
  }

  localTime.Wday = dayOfWeek;
  return true;
}

#endif


/*
  Time functions
*/


void updateCurrentTime() {
  timeCorrect = RTC.read(localTime);
}


/*
  Display functions
*/


void drawMark(int h) {
  if (((localTime.Second % 5) == 0) && (h * 5 == localTime.Second)) {
      return;
  }

  float x1, y1, x2, y2;
  
  h = h * 30;
  h = h + 270;
  
  x1 = (clockRad - 1) * cos(h * 0.0175);
  y1 = (clockRad - 1) * sin(h * 0.0175);
  x2 = (clockRad - 5) * cos(h * 0.0175);
  y2 = (clockRad - 5) * sin(h * 0.0175);
  
  u8g2.drawLine(x1 + clockCenterX, y1 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
}

void drawSec(int s) {
  if ((s % 5) == 0) {
    return;
  }

  float x1, y1, x2, y2;

  s = s * 6;
  s = s + 270;
  
  x1 = (clockRad - 1) * cos(s * 0.0175);
  y1 = (clockRad - 1) * sin(s * 0.0175);
  x2 = (clockRad - 5) * cos(s * 0.0175);
  y2 = (clockRad - 5) * sin(s * 0.0175);

  u8g2.drawLine(x1 + clockCenterX, y1 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
}

void drawMin(int m) {
  float x1, y1, x2, y2, x3, y3, x4, y4;

  m = m * 6;
  m = m + 270;
  
  x1 = (clockRad - 5) * cos(m * 0.0175);
  y1 = (clockRad - 5) * sin(m * 0.0175);
  x2 = (clockRad - 27) * cos(m * 0.0175);
  y2 = (clockRad - 27) * sin(m * 0.0175);
  x3 = (clockRad - 20) * cos((m + 8) * 0.0175);
  y3 = (clockRad - 20) * sin((m + 8) * 0.0175);
  x4 = (clockRad - 20) * cos((m - 8) * 0.0175);
  y4 = (clockRad - 20) * sin((m - 8) * 0.0175);
  
  u8g2.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
  u8g2.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
  u8g2.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
  u8g2.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);
}

void drawHour(int h, int m) {
  float x1, y1, x2, y2, x3, y3, x4, y4;

  h = (h * 30) + (m / 2);
  h = h + 270;
  
  x1 = (clockRad - 10) * cos(h * 0.0175);
  y1 = (clockRad - 10) * sin(h * 0.0175);
  x2 = (clockRad - 27) * cos(h * 0.0175);
  y2 = (clockRad - 27) * sin(h * 0.0175);
  x3 = (clockRad - 22) * cos((h + 12) * 0.0175);
  y3 = (clockRad - 22) * sin((h + 12) * 0.0175);
  x4 = (clockRad - 22) * cos((h - 12) * 0.0175);
  y4 = (clockRad - 22) * sin((h - 12) * 0.0175);
  
  u8g2.drawLine(x1 + clockCenterX, y1 + clockCenterY, x3 + clockCenterX, y3 + clockCenterY);
  u8g2.drawLine(x3 + clockCenterX, y3 + clockCenterY, x2 + clockCenterX, y2 + clockCenterY);
  u8g2.drawLine(x2 + clockCenterX, y2 + clockCenterY, x4 + clockCenterX, y4 + clockCenterY);
  u8g2.drawLine(x4 + clockCenterX, y4 + clockCenterY, x1 + clockCenterX, y1 + clockCenterY);
}

void drawWatchFace() {
  // Draw Clockface
  for (int i=0; i<2; i++) {
    u8g2.drawCircle(clockCenterX, clockCenterY, clockRad-i);
  }

  for (int i=0; i<3; i++) {
    u8g2.drawCircle(clockCenterX, clockCenterY, i);
  }
  
  // Draw a small mark for every hour
  for (int i=0; i<12; i++) {
    drawMark(i);
  }
}

void drawEmptyCentralBlock(uint8_t center, uint8_t width, uint8_t top, uint8_t height, uint8_t corner = 0) {
  u8g2.setColorIndex(0);
  if (corner == 0) {
    u8g2.drawBox(center - width / 2, top, width, height);
  } else {
    u8g2.drawRBox(center - width / 2, top, width, height, 3);
  }

  u8g2.setColorIndex(1);
  if (corner == 0) {
    u8g2.drawFrame(center - width / 2, top, width, height);
  } else {
    u8g2.drawRFrame(center - width / 2, top, width, height, 3);
  }
}

void drawCentralBlock(uint8_t center, uint8_t width, uint8_t top, uint8_t height, uint8_t corner = 0) {
  if (corner == 0) {
    u8g2.drawFrame(center - width / 2, top, width, height);
  } else {
    u8g2.drawRFrame(center - width / 2, top, width, height, 3);
  }
}

void drawCentralLines(uint8_t x, uint8_t y1, uint8_t y2, uint8_t y3) {
  u8g2.drawLine(x, 16, x, y1 - 2);
  u8g2.drawLine(x, y1 + 2, x, y2 - 2);
  u8g2.drawLine(x, y2 + 2, x, y3 - 2);
}

void drawHorisontalLines(uint8_t y1, uint8_t y2, uint8_t y3) {
  u8g2.drawLine(0, y1, displayWidth, y1);
  u8g2.drawLine(0, y2, displayWidth, y2);
  u8g2.drawLine(0, y3, displayWidth, y3);
}

void drawText(const char *c, uint8_t y, align a, uint8_t offset = 0) {
  uint8_t start = 0;

  if (a == left) {
    start = 0;
  } else if (a == center) {
    uint8_t strWidth = u8g2.getStrWidth(c);

    if (strWidth == 0 || strWidth > displayWidth) {
      start = 0;
    } else {
      start = (displayWidth - strWidth) / 2;
    }
  } else if (a == right) {
    uint8_t strWidth = u8g2.getStrWidth(c);

    if (strWidth == 0 || strWidth > displayWidth) {
      start = 0;
    } else {
      start = displayWidth - strWidth;
    }
  }
  
  u8g2.drawStr(start + offset, y, c);
}

void drawSeconds(uint8_t s, uint8_t y) {
  uint8_t firstSecond = 0;
  if (s < 30) {
    firstSecond = s + 30;
  } else {
    firstSecond = s - 30;
  }

  uint8_t stepSecond = firstSecond;
  float pixelsInOneSecond = displayWidth / 60;
  char STRING1[] = "  ";
  uint8_t delta = 5; // Need for font size correction

  for(size_t i = 0; i < 70; i++) {
    stepSecond = firstSecond + i;
    while(stepSecond >= 60) {
      stepSecond = stepSecond - 60;
    }

    if (stepSecond % 15 == 0) {
      u8g2.drawLine(i * pixelsInOneSecond, y - 2, i * pixelsInOneSecond, y);
      sprintf(STRING1, "%d", stepSecond);
      if (stepSecond == 0) {
        delta = 2;
      } else if (stepSecond == 15) {
        delta = 4;
      } else {
        delta = 5;
      }
      drawText(STRING1, y - 4, left, i * pixelsInOneSecond - delta);
    }
  }
}

void drawMinutes(uint8_t m, uint8_t y) {
  uint8_t firstMinute = 0;
  if (m < 30) {
    firstMinute = m + 30;
  } else {
    firstMinute = m - 30;
  }

  uint8_t stepMinute = firstMinute;
  float pixelsInOneMinute = displayWidth / 60;
  char STRING1[] = "  ";
  uint8_t delta = 5; // Need for font size correction

  for(size_t i = 0; i < 70; i++) {
    stepMinute = firstMinute + i;
    while(stepMinute >= 60) {
      stepMinute = stepMinute - 60;
    }

    if (stepMinute % 5 == 0) {
      u8g2.drawLine(i * pixelsInOneMinute, y - 2, i * pixelsInOneMinute, y);
    }

    if (stepMinute % 15 == 0) {
      sprintf(STRING1, "%d", stepMinute);
      if (stepMinute == 0) {
        delta = 2;
      } else if (stepMinute == 15) {
        delta = 4;
      } else {
        delta = 5;
      }
      drawText(STRING1, y - 4, left, i * pixelsInOneMinute - delta);
    }
  }
}

void drawHours(uint8_t h, uint8_t y) {
  uint8_t firstHour = 0;
  if (h < 12) {
    firstHour = h + 12;
  } else {
    firstHour = h - 12;
  }

  uint8_t stepHour = firstHour;
  float pixelsInOneHour = displayWidth / 24;
  char STRING1[] = "  ";
  uint8_t delta = 5; // Need for font size correction

  for(size_t i = 0; i < 28; i++) {
    stepHour = firstHour + i;
    while(stepHour >= 24) {
      stepHour = stepHour - 24;
    }

    u8g2.drawLine(i * pixelsInOneHour, y - 2, i * pixelsInOneHour, y);

    if (stepHour % 3 == 0) {
      sprintf(STRING1, "%d", stepHour);
      if (stepHour <= 10) {
        delta = 2;
      } else {
        delta = 4;
      }
      drawText(STRING1, y - 4, left, i * pixelsInOneHour - delta);
    }
  }
}

void drawCurrentTimeInBlock(uint8_t center, uint8_t width, uint8_t y1, uint8_t y2, uint8_t y3, uint8_t h, uint8_t m, uint8_t s) {
  char HOUR[] = "  ";
  sprintf(HOUR, "%02d", h);

  char MINUTE[] = "  ";
  sprintf(MINUTE, "%02d", m);

  char SECOND[] = "  ";
  sprintf(SECOND, "%02d", s);

  u8g2.drawStr(center - width / 2 + 1, y1, HOUR);
  u8g2.drawStr(center - width / 2 + 1, y2, MINUTE);
  u8g2.drawStr(center - width / 2 + 1, y3, SECOND);
}

void drawTopBar(const char *c, const uint8_t *font, uint8_t y) {
  u8g2.setFont(font);
  drawText(c, y, left);

  u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
  if (transferData){
    u8g2.drawGlyph(displayWidth - 8, y, icons[5]);
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      u8g2.drawGlyph(displayWidth - 8, y, icons[1]);
    } else {
      u8g2.drawGlyph(displayWidth - 8, y, icons[0]);
    }
  }
}

void drawModeOne() {
  char STRING1[] = "                ";
  sprintf(STRING1, "%s %02d %04d, %s", monthName[localTime.Month - 1], localTime.Day, tmYearToCalendar(localTime.Year), dayName[localTime.Wday]);

  char STRING2[] = "     ";
  sprintf(STRING2, "%02d:%02d", localTime.Hour, localTime.Minute);

  char STRING3[] = "  ";
  sprintf(STRING3, "%02d", localTime.Second);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar(STRING1, u8g2_font_7x14B_tf, 10);

  u8g2.setFont(u8g2_font_logisoso22_tn);
  drawText(STRING2, 38, center);
  u8g2.setFont(u8g2_font_logisoso16_tf);
  drawText(STRING3, 63, center);

  u8g2.sendBuffer();
}

void drawModeTwo() {
  clockCenterX = displayWidth / 2;
  clockCenterY = 39;

  char STRING1[] = "                ";
  sprintf(STRING1, "%s %02d %04d, %s", monthName[localTime.Month - 1], localTime.Day, tmYearToCalendar(localTime.Year), dayName[localTime.Wday]);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar(STRING1, u8g2_font_7x14B_tf, 10);

  drawWatchFace();
  drawSec(localTime.Second);
  drawMin(localTime.Minute);
  drawHour(localTime.Hour, localTime.Minute);

  u8g2.sendBuffer();
}

void drawModeThree() {
  clockCenterX = displayWidth - clockRad - 1;
  clockCenterY = 39;

  char STRING1[] = "                ";
  sprintf(STRING1, "%s %02d %04d, %s", monthName[localTime.Month - 1], localTime.Day, tmYearToCalendar(localTime.Year), dayName[localTime.Wday]);

  char STRING2[] = "     ";
  sprintf(STRING2, "%02d:%02d", localTime.Hour, localTime.Minute);

  char STRING3[] = "  ";
  sprintf(STRING3, "%02d", localTime.Second);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar(STRING1, u8g2_font_7x14B_tf, 10);

  u8g2.setFont(u8g2_font_logisoso22_tn);
  drawText(STRING2, 38, left);
  u8g2.setFont(u8g2_font_logisoso16_tf);
  drawText(STRING3, 63, left, 25);
  
  drawWatchFace();
  drawSec(localTime.Second);
  drawMin(localTime.Minute);
  drawHour(localTime.Hour, localTime.Minute);

  u8g2.sendBuffer();
}

void drawModeFour() {
  char STRING1[] = "                ";
  sprintf(STRING1, "%s %02d %04d, %s", monthName[localTime.Month - 1], localTime.Day, tmYearToCalendar(localTime.Year), dayName[localTime.Wday]);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar(STRING1, u8g2_font_7x14B_tf, 10);

  uint8_t line1Y = 30;
  uint8_t line2Y = 47;
  uint8_t line3Y = 63;
  drawCentralLines(displayWidth / 2, line1Y, line2Y, line3Y);
  drawHorisontalLines(line1Y, line2Y, line3Y);

  u8g2.setFont(u8g2_font_haxrcorp4089_tn);
  drawSeconds(localTime.Second, line3Y);
  u8g2.setFont(u8g2_font_glasstown_nbp_tn);
  drawMinutes(localTime.Minute, line2Y);
  drawHours(localTime.Hour, line1Y);

  u8g2.sendBuffer();
}

void drawModeFive() {
  char STRING1[] = "                ";
  sprintf(STRING1, "%s %02d %04d, %s", monthName[localTime.Month - 1], localTime.Day, tmYearToCalendar(localTime.Year), dayName[localTime.Wday]);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar(STRING1, u8g2_font_7x14B_tf, 10);

  uint8_t line1Y = 31;
  uint8_t line2Y = 46;
  uint8_t line3Y = 60;
  drawHorisontalLines(line1Y, line2Y, line3Y);

  u8g2.setFont(u8g2_font_haxrcorp4089_tn);
  drawSeconds(localTime.Second, line3Y);
  drawMinutes(localTime.Minute, line2Y);
  drawHours(localTime.Hour, line1Y);

  drawCentralBlock(displayWidth / 2, 14, 16, displayHeight - 16, 3);

  u8g2.sendBuffer();
}

void drawModeSix() {
  char STRING1[] = "                ";
  sprintf(STRING1, "%s %02d %04d, %s", monthName[localTime.Month - 1], localTime.Day, tmYearToCalendar(localTime.Year), dayName[localTime.Wday]);

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar(STRING1, u8g2_font_7x14B_tf, 10);

  uint8_t line1Y = 31;
  uint8_t line2Y = 46;
  uint8_t line3Y = 60;
  drawHorisontalLines(line1Y, line2Y, line3Y);

  u8g2.setFont(u8g2_font_haxrcorp4089_tn);
  drawSeconds(localTime.Second, line3Y);
  drawMinutes(localTime.Minute, line2Y);
  drawHours(localTime.Hour, line1Y);

  drawEmptyCentralBlock(displayWidth / 2, 19, 16, displayHeight - 16, 3);

  u8g2.setFont(u8g2_font_profont12_tn);
  drawCurrentTimeInBlock(displayWidth / 2, 13, line1Y - 3, line2Y - 3, line3Y - 3, localTime.Hour, localTime.Minute, localTime.Second);

  u8g2.sendBuffer();
}

void drawFirmwareUpdateMode() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar("Mirror Clock", u8g2_font_7x14B_tf, 10);
  u8g2.setFont(u8g2_font_7x14B_tf);
  char VERSION[] = "Version: 0.0";
  sprintf(VERSION, "Version: %s", VER);
  drawText(VERSION, 26, left);
  drawText("Firmware update", 42, left);
  
  switch (uploadStatus) {
    case UPLOAD_FILE_START:
      drawText("Upload begin", 58, left);
      break;

    case UPLOAD_FILE_WRITE:
      drawText("Uploading", 58, left);
      break;

    case UPLOAD_FILE_END:
      drawText("Upload end", 58, left);
      break;

    case UPLOAD_FILE_ABORTED:
      drawText("Upload abort", 58, left);
      break;
  
    default:
      break;
  }

  u8g2.setFont(u8g2_font_open_iconic_www_2x_t);
  u8g2.drawGlyph(displayWidth - 16, displayHeight, icons[11]);

  u8g2.sendBuffer();
}

void drawRebootingMode() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar("Mirror Clock", u8g2_font_7x14B_tf, 10);
  u8g2.setFont(u8g2_font_7x14B_tf);
  char VERSION[] = "Version: 0.0";
  sprintf(VERSION, "Version: %s", VER);
  drawText(VERSION, 26, left);
  drawText("Firmware update", 42, left);
  drawText("Rebooting...", 58, left);

  u8g2.setFont(u8g2_font_open_iconic_embedded_2x_t);
  u8g2.drawGlyph(displayWidth - 16, displayHeight, icons[12]);

  u8g2.sendBuffer();
}

void drawFWErrorMode() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);

  drawTopBar("Mirror Clock", u8g2_font_7x14B_tf, 10);
  u8g2.setFont(u8g2_font_7x14B_tf);
  drawText("Firmware update", 26, left);
  drawText("End with error", 42, left);
  
  switch (uploadingErrorCode) {
    case 1:
      drawText("FW is too BIG", 58, left);
      break;
  
    case 2:
      drawText("FW wrong siz", 58, left);
      break;
  
    case 3:
      drawText("Error uploading", 58, left);
      break;

    case 4:
      drawText("Upload aborted", 58, left);
      break;
      
    default:
      break;
  }

  u8g2.sendBuffer();
}

void displayCurrentTime() {
  if (firmwareUpdateOTA) {
    drawFirmwareUpdateMode();
    return;
  }

  if (rebooting) {
    drawRebootingMode();
    return;
  }

  if (uploadingError) {
    drawFWErrorMode();
    return;
  }

  if (!timeCorrect) {
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setFontDirection(0);
    u8g2.setFont(u8g2_font_7x14B_tf);

    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      drawText("DS1307 is stopped", 10, center);
      drawText("Run the SetTime", 26, center);
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      drawText("DS1307 read error", 10, center);
      drawText("Check circuitry", 26, center);
    }

    u8g2.sendBuffer();

    return;
  }

  switch (currentWatchFace)
  {
    case 1:
      drawModeOne();
      break;

    case 2:
      drawModeTwo();
      break;

    case 3:
      drawModeThree();
      break;

    case 4:
      drawModeFour();
      break;

    case 5:
      drawModeFive();
      break;

    case 6:
      drawModeSix();
      break;
  
    default:
      drawModeOne();
      break;
  }
}

#ifdef DEMOMODE

void changeWatchFace() {
  currentWatchFace++;
  
  if (currentWatchFace > totalWatchFaces || currentWatchFace < 1) {
    currentWatchFace = 1;
  }
}

#endif


/*
  Core functions
*/


void setup() {
  Serial.begin(9600);
  Serial.println("Mirror clock starting");

  // OLED initialize
  u8g2.begin();

  displayHeight = u8g2.getDisplayHeight();
  displayWidth = u8g2.getDisplayWidth();

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);
  u8g2.setFont(u8g2_font_7x14B_tf);
  drawText("Mirror Clock", 10, center);

  char VERSION[] = "Version: 0.0";
  sprintf(VERSION, "Version: %s", VER);
  drawText(VERSION, 26, center);
  drawText("(C) Clevik", 42, center);
  
  #ifdef DEMOMODE
  u8g2.setFont(u8g2_font_smart_patrol_nbp_tf);
  drawText("Demo mode", 63, right);
  #endif

  u8g2.sendBuffer();

  // Update time
  updateCurrentTime();

  delay(1000);

  #ifdef SETTIME

  bool parse=false;
  bool config=false;
  Serial.println("Set time");

  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setFontDirection(0);
  u8g2.setFont(u8g2_font_7x14B_tf);
  drawText("Set time", 10, center);
  u8g2.sendBuffer();

  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__) && getDayOfWeek("3")) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(localTime)) {
      config = true;
    }
  }
  
  delay(500);

  if (parse && config) {
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    Serial.print(", Date=");
    Serial.println(__DATE__);

    drawText("DS1307 configured", 26, center);
    u8g2.sendBuffer();

  } else if (parse) {
    Serial.println("DS1307 Communication Error :-{");
    Serial.println("Please check your circuitry");

    drawText("Communication Err", 26, center);
    drawText("Check circuitry", 42, center);
    u8g2.sendBuffer();
  } else {
    Serial.print("Could not parse info from the compiler, Time=\"");
    Serial.print(__TIME__);
    Serial.print("\", Date=\"");
    Serial.print(__DATE__);
    Serial.println("\"");
    
    drawText("Could not parse", 26, center);
    drawText("DATE and TIME", 42, center);
    u8g2.sendBuffer();
  }

  delay (3000);

  #endif

  // Timers initialize
  updateCurrentTimeTimer.every(500, updateCurrentTime);
  displayCurrentTimeTimer.every(500, displayCurrentTime);
  
  #ifdef DEMOMODE
  changeWatchFaceTimer.every(30000, changeWatchFace);
  #endif

  // WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
  u8g2.drawGlyph(displayWidth - 8, 8, icons[2]);
  u8g2.drawGlyph(displayWidth - 8, 58, icons[0]); // 0 broken chain, 3 - chain
  u8g2.setFont(u8g2_font_7x14B_tf);
  drawText("Mirror Clock", 10, left);
  drawText("Configuring WiFi", 26, left);
  drawText("WiFi:", 42, left);
  drawText(ssid, 42, left, 36);
  drawText("Attempt 1/10", 58, left);
  u8g2.sendBuffer();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint8_t attempt = 1;
  char STRING1[] = "             ";

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    sprintf(STRING1, "Attempt %d/10", (int)(attempt / 2) + 1);
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
    u8g2.drawGlyph(displayWidth - 8, 8, icons[2]);
    if (attempt %2 == 0) {
      u8g2.drawGlyph(displayWidth - 8, 58, icons[0]);
    } else {
      u8g2.drawGlyph(displayWidth - 8, 58, icons[3]);
    }
    
    u8g2.setFont(u8g2_font_7x14B_tf);
    drawText("Mirror Clock", 10, left);
    drawText("Configuring WiFi", 26, left);
    drawText("WiFi:", 42, left);
    drawText(ssid, 42, left, 36);
    drawText(STRING1, 58, left);
    u8g2.sendBuffer();

    attempt = attempt + 1;
    if (attempt > 20) {
      drawText("WiFi not connected", 58, left);
      delay(3000);
      break;
    }

    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
    u8g2.drawGlyph(displayWidth - 8, 8, icons[2]);
    u8g2.setFont(u8g2_font_7x14B_tf);
    drawText("Mirror Clock", 10, left);
    drawText("Configuring WiFi", 26, left);
    drawText("WiFi:", 42, left);
    drawText(ssid, 42, left, 36);
    drawText("WiFi connected", 58, left);
    u8g2.sendBuffer();

    delay(2000);
  }

  server.on("/", HTTP_GET, [](){
    transferData = true;
    displayCurrentTime();
    Serial.println("HTTP /");
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String response;

    if (rebooting) {
      response = page.getRefresh(WiFi.localIP());
    } else {
      response = page.getPage(VER, localTime.Hour, localTime.Minute, localTime.Second, localTime.Month, localTime.Day, localTime.Wday, tmYearToCalendar(localTime.Year), timezone);
    }

    server.send(200, "text/html", response);
    transferData = false;
  });

  server.on("/update", HTTP_GET, [](){
    transferData = true;
    displayCurrentTime();
    Serial.println("HTTP /update");
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    if (rebooting) {
      String response = page.getRefresh(WiFi.localIP());
      server.send(200, "text/html", response);
    } else {
      server.send(200, "text/plain", (Update.hasError())?"Last update FAIL":"Last update OK");
    }
    
    transferData = false;
  });

  server.on("/update", HTTP_POST, [](){
    transferData = true;
    displayCurrentTime();
    Serial.println("HTTP /update");
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    if (rebooting) {
      String response = page.getRefresh(WiFi.localIP());
      server.send(200, "text/html", response);
    } else {
      server.send(200, "text/plain", (Update.hasError())?"Last update FAIL":"Last update OK");
    }
    
    transferData = false;
    rebooting = true;
    displayCurrentTime();
    ESP.restart();
  },[](){
    ESP.wdtDisable();
    transferData = true;
    firmwareUpdateOTA = true;
    uploadingError = false;

    displayCurrentTime();

    HTTPUpload& upload = server.upload();
    uploadStatus = upload.status;

    if (uploadStatus == UPLOAD_FILE_START) {
      ESP.wdtEnable(1000);
      Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

      if (!Update.begin(maxSketchSpace)) { // start with max available size
        Update.printError(Serial);
        uploadingError = true;
        uploadingErrorCode = 1;
        displayCurrentTime();
        delay(3000);
      }
    } else if(uploadStatus == UPLOAD_FILE_WRITE) {
      ESP.wdtEnable(1000);
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize){
        Update.printError(Serial);
        uploadingError = true;
        uploadingErrorCode = 2;
        displayCurrentTime();
        delay(3000);
      }
    } else if (uploadStatus == UPLOAD_FILE_END) {
      ESP.wdtEnable(1000);
      transferData = false;
      firmwareUpdateOTA = false;

      if (Update.end(true)) { // true to set the size to the current progress
        Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
        uploadingError = false;
        rebooting = true;
        server.stop();
      } else {
        Update.printError(Serial);
        uploadingError = true;
        uploadingErrorCode = 3;
        displayCurrentTime();
        delay(3000);
      }

      Serial.setDebugOutput(false);
    } else if (uploadStatus == UPLOAD_FILE_ABORTED) {
      ESP.wdtEnable(1000);
      transferData = false;
      firmwareUpdateOTA = false;

      Serial.println("Upload aborted");
      uploadingError = true;
      uploadingErrorCode = 4;
      displayCurrentTime();
      delay(3000);
    }
    yield();
  });

  server.begin();
}

void loop(void) {
  updateCurrentTimeTimer.update();
  displayCurrentTimeTimer.update();
  server.handleClient();

  #ifdef DEMOMODE
  changeWatchFaceTimer.update();
  #endif
}