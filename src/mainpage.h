#include <Arduino.h>
#include "DS1307.h"
#include <ESP8266WiFi.h>

#ifndef MainPage_h
#define MainPage_h

#ifdef __AVR__
 #include <avr/pgmspace.h>
#elif defined(ESP8266)
 #include <pgmspace.h>
#else
 #define PROGMEM
#endif

class MainPage {
public:
  MainPage(){}

  String getPage(const char* version, uint8_t hour, uint8_t minute,
    uint8_t second, uint8_t month, uint8_t day, uint8_t dayOfWeekOriginal,
    uint16_t year, int8_t timezone) {
    String result = "<html><head><meta charset='UTF-8'><title>Firmware version: ";
    result = result + version;
    result = result + "</title></head><body><font face='sans-serif'><div><h2>Parameters:</h2>Время: ";
    result = result + hour;
    result = result + ":";
    result = result + minute;
    result = result + ":";
    result = result + second;
    result = result + " ";
    result = result + year;
    result = result + "-";
    result = result + month;
    result = result + "-";
    result = result + day;
    result = result + ", ";
    switch (dayOfWeekOriginal) {
  		case MON: {
  				result = result + "monday";
  			}
  			break;
  		case TUE: {
  				result = result + "tuesday";
  			}
  			break;
  		case WED: {
  				result = result + "wednesday";
  			}
  			break;
  		case THU: {
  				result = result + "thursday";
  			}
  			break;
  		case FRI: {
  				result = result + "friday";
  			}
  			break;
  		case SAT: {
  				result = result + "saturday";
  			}
  			break;
  		case SUN: {
          result = result + "sunday";
  			}
  			break;
  	}

    if (timezone > 0) {
      result = result + ", UTC+";
    } else {
      result = result + ", UTC";
    }
    result = result + timezone;

    result = result + "<br><br></div><div><h2>Firmware update:</h2>";

    result = result + "Current version: ";
    result = result + version;
    result = result + "<form method='POST' action='/update' enctype='multipart/form-data' onsubmit='window.open(window.location.origin, '_self');'>Select firmware file (*.bin): <input type='file' name='update'><input type='submit' accept='.bin' value='Update firmware'></form></div></font></body></html>";
    return result;
  };

  String getRefresh(IPAddress addr, uint8_t sec = 10) {
    String result = "<!DOCTYPE html><html><title>Reload page</title><meta http-equiv='refresh' content='";
    result = result + sec;
    result = result + ";url=http://";
    result = result + addr;
    result = result + "' /><body onload='document.location.replace('";
    result = result + "'http://";
    result = result + addr;
    result = result + "')'></body></html>";

    return result;
  }
};

// extern MainPage;

#else

#endif
