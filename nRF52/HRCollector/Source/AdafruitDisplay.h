/*!
 * @file Adafruit_SSD1306.h
 *
 * This is part of for Adafruit's SSD1306 library for monochrome
 * OLED displays: http://www.adafruit.com/category/63_98
 *
 * These displays use I2C or SPI to communicate. I2C requires 2 pins
 * (SCL+SDA) and optionally a RESET pin. SPI requires 4 pins (MOSI, SCK,
 * select, data/command) and optionally a reset pin. Hardware SPI or
 * 'bitbang' software SPI are both supported.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * BSD license, all text above, and the splash screen header file,
 * must be included in any redistribution.
 *
 * Updated by Terence M. Darwen (tmdarwen.com) to work with the Nordic 
 * nRF52840.
 */

#ifndef AdafruitDisplay
#define AdafruitDisplay

#include <stdint.h>

void AdafruitDisplayInit(uint16_t screenWidth, uint16_t screenHeight, uint8_t textSize);
void AdafruitDisplayDeInit();

// Change text size
void AdafruitSetTextSize(uint8_t textSize);
  
// Clear the screen
void AdafruitDisplayClearScreen();

// Display the current drawn screen
void AdafruitDisplayUpdateScreen();

// Display the current drawn screen
void AdafruitDisplayWrite(uint8_t c);

// Display the given string
void AdafruitDisplayString(const char* string);

// Draw pixel at the given (x,y) position
void AdafruitDisplayDrawPixel(int16_t x, int16_t y);

// Draw a line from point (x1,y1) to point (x2,y2)
void AdafruitDisplayWriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

// Set the cursor at the given (x,y) position
void AdafruitDisplaySetCursor(int16_t x, int16_t y);

// Draw the given bitmap data at the given (x,y) position
void AdafruitDisplayDrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);

// Draw a fill rect at the given location (x,y) and the given width/height (w,h)
void AdafruitDisplayFillRect(int16_t x, int16_t y, int16_t w, int16_t h);

#endif
