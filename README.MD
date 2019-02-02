A clock in the mirror frame. Personal project.
In the project used Used OLED screen 128x64, ESP8266, DS1307.
Source files for ProjectIO.


## Features list

- [x] Real-time clock based on DS1307. The time is not reset when the power is turned off.
- [x] Support OLED screens based on the SSD1306 controller. Also supported other controllers via [U8G2](https://github.com/olikraus/U8g2_Arduino) library.
- [x] DS1307 failures description on the screen: wrong connection or not set time.

## WiFi and web page settings

- [x] Connect to the WiFi network.
- [x] WiFi status icon on the top line.
- [x] Update firmware from the local web page.
- [x] Indication on the screen when update firmware.
- [ ] Store settings in SPIFFS.
- [ ] Page with settings, control and time set.
- [ ] Change WiFi settings from the local web page.
- [ ] Selection of the face of the clock from the local web page.
- [ ] Set time from the local web page.
- [ ] Enable/disable time update from NTP server.
- [ ] Manual settings of NTP server address.

## Buttons and menu

- [ ] Support three touch buttons.
- [ ] On-screen menu.
- [ ] Selection of the face of the clock.
- [ ] Set time from the menu.
- [ ] WiFi access point mode.

_Note: AP mode can be turned on from a menu only, and don't working with WiFi client together._

- [ ] View WiFi settings.

## Improvements

- [ ] Light sensor support.
- [ ] Correct display contrast by light sensor state.
- [ ] Motion sensor support (for IDLE mode).
- [ ] IDLE mode: shut off the display or decrease contrast, when nobody is around.
- [ ] Night mode: turn off the display by schedule or if the light is off by night and the motion sensor detect anyone is close.

## Internet functions

- [ ] Update weather data from the internet.
- [ ] Show weather icons on the screen.
- [ ] Update time from NTP server.
