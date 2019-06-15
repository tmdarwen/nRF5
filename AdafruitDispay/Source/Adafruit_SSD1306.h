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
 */

#ifndef _Adafruit_SSD1306_H_
#define _Adafruit_SSD1306_H_

#include "Adafruit_GFX.h"

class Adafruit_SSD1306 : public Adafruit_GFX
{
    public:
        Adafruit_SSD1306(uint16_t screenWidth, uint16_t screenHeight, uint8_t textSize=1);
        ~Adafruit_SSD1306();
  
        // Clear the screen
        void Clear();

        // Display the current drawn screen
        void Display();

        // Draw pixel at the given (x,y) position
        void DrawPixel(int16_t x, int16_t y);
  
    private:
        // The buffer holding the screen's current contents
        uint8_t* buffer;
      
        // Issue single command to SSD1306.
        // Because command calls are often grouped, SPI transaction and selection
        // must be started/ended in calling function for efficiency.
        // This is a private function, not exposed (see ssd1306_command() instead).
        void ssd1306_command1(uint8_t c);
  
        // Issue list of commands to SSSD1306
        void ssd1306_commandList(const uint8_t *c, uint8_t n);
  
        // SPI transaction/selection must be performed in calling function.
        void SPIWrite(uint8_t d);
};

#endif