/*!
 * @file Adafruit_SSD1306.cpp
 *
 * @mainpage Arduino library for monochrome OLEDs based on SSD1306 drivers.
 *
 * @section intro_sec Introduction
 *
 * This is documentation for Adafruit's SSD1306 library for monochrome
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
 * @section dependencies Dependencies
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit-GFX-Library">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * @section license License
 *
 * BSD license, all text above, and the splash screen included below,
 * must be included in any redistribution.
 *
 * Updated by Terence M. Darwen (tmdarwen.com) to work with the Nordic 
 * nRF52840.
 */

#include "AdafruitDisplay.h"
#include "glcdfont.h"

#include "app_util_platform.h"
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log_ctrl.h"

// The pin numbers from the nRF52 to the SSD1306
#define SSD1306_DATA_PIN     26
#define SSD1306_CLOCK_PIN    27
#define SSD1306_DC_PIN       28
#define SSD1306_RESET_PIN    29
#define SSD1306_CS_PIN       30

// SSD1306 Commands
#define SSD1306_MEMORYMODE          0x20 ///< See datasheet
#define SSD1306_COLUMNADDR          0x21 ///< See datasheet
#define SSD1306_PAGEADDR            0x22 ///< See datasheet
#define SSD1306_SETCONTRAST         0x81 ///< See datasheet
#define SSD1306_CHARGEPUMP          0x8D ///< See datasheet
#define SSD1306_SEGREMAP            0xA0 ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_NORMALDISPLAY       0xA6 ///< See datasheet
#define SSD1306_SETMULTIPLEX        0xA8 ///< See datasheet
#define SSD1306_DISPLAYOFF          0xAE ///< See datasheet
#define SSD1306_DISPLAYON           0xAF ///< See datasheet
#define SSD1306_COMSCANDEC          0xC8 ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET    0xD3 ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5 ///< See datasheet
#define SSD1306_SETPRECHARGE        0xD9 ///< See datasheet
#define SSD1306_SETCOMPINS          0xDA ///< See datasheet
#define SSD1306_SETVCOMDETECT       0xDB ///< See datasheet
#define SSD1306_SETSTARTLINE        0x40 ///< See datasheet
#define SSD1306_DEACTIVATE_SCROLL   0x2E ///< Stop scroll


uint16_t screenWidth_ = 0;
uint16_t screenHeight_ = 0;    

int16_t cursorX_ = 0;       ///< x location to start print()ing text
int16_t cursorY_ = 0;       ///< y location to start print()ing text

uint8_t textSize_ = 1;

uint8_t* buffer;

#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }

void AdafruitDisplayCommand1(uint8_t c);
void AdafruitDisplayCommandList(const uint8_t *c, uint8_t n);
void AdafruitDisplayDrawChar(int16_t x, int16_t y, unsigned char c, uint8_t size);
void AdafruitDisplayFillRect(int16_t x, int16_t y, int16_t w, int16_t h);
void AdafruitDisplaySPIWrite(uint8_t d);

void AdafruitDisplayInit(uint16_t screenWidth, uint16_t screenHeight, uint8_t textSize)
{
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    textSize_ = textSize;

    nrf_gpio_cfg_output(SSD1306_DATA_PIN);
    nrf_gpio_cfg_output(SSD1306_CLOCK_PIN);
    nrf_gpio_cfg_output(SSD1306_DC_PIN);
    nrf_gpio_cfg_output(SSD1306_RESET_PIN);
    nrf_gpio_cfg_output(SSD1306_CS_PIN);

    buffer = (uint8_t *)malloc(screenWidth_ * ((screenHeight_ + 7) / 8));

    AdafruitDisplayClearScreen();

    nrf_gpio_pin_set(SSD1306_CS_PIN);      // Device deselect
    nrf_gpio_pin_clear(SSD1306_CLOCK_PIN); // Set the clock low

    // Reset SSSD1306
    nrf_gpio_pin_set(SSD1306_RESET_PIN);    
    nrf_delay_ms(1);                        // VDD goes high at start, pause for 1 ms
    nrf_gpio_pin_clear(SSD1306_RESET_PIN);  // Bring reset low
    nrf_delay_ms(10);                       // Wait 10 ms
    nrf_gpio_pin_set(SSD1306_RESET_PIN);    // Bring out of reset

    nrf_gpio_pin_clear(SSD1306_CS_PIN);

    // Init sequence
    static const uint8_t init1[] = {
       SSD1306_DISPLAYOFF,                   // 0xAE
       SSD1306_SETDISPLAYCLOCKDIV,           // 0xD5
       0x80,                                 // the suggested ratio 0x80
       SSD1306_SETMULTIPLEX };               // 0xA8
    AdafruitDisplayCommandList(init1, sizeof(init1));
    AdafruitDisplayCommand1(screenHeight_ - 1);

    static const uint8_t init2[] = {
      SSD1306_SETDISPLAYOFFSET,             // 0xD3
      0x0,                                  // no offset
      SSD1306_SETSTARTLINE | 0x0,           // line #0
      SSD1306_CHARGEPUMP };                 // 0x8D
    AdafruitDisplayCommandList(init2, sizeof(init2));

    AdafruitDisplayCommand1(0x14);

    static const uint8_t init3[] = {
      SSD1306_MEMORYMODE,                   // 0x20
      0x00,                                 // 0x0 act like ks0108
      SSD1306_SEGREMAP | 0x1,
      SSD1306_COMSCANDEC };
    AdafruitDisplayCommandList(init3, sizeof(init3));

    static const uint8_t init4b[] = {
      SSD1306_SETCOMPINS,                 // 0xDA
      0x12,
      SSD1306_SETCONTRAST };              // 0x81
    AdafruitDisplayCommandList(init4b, sizeof(init4b));
    AdafruitDisplayCommand1(0x9F);

    AdafruitDisplayCommand1(SSD1306_SETPRECHARGE); // 0xd9
    AdafruitDisplayCommand1(0xF1);
    static const uint8_t init5[] = {
      SSD1306_SETVCOMDETECT,               // 0xDB
      0x40,
      SSD1306_DISPLAYALLON_RESUME,         // 0xA4
      SSD1306_NORMALDISPLAY,               // 0xA6
      SSD1306_DEACTIVATE_SCROLL,
      SSD1306_DISPLAYON };                 // Main screen turn on
    AdafruitDisplayCommandList(init5, sizeof(init5)); 
}

void AdafruitSetTextSize(uint8_t textSize)
{
    textSize_ = textSize;
}

void AdafruitDisplayDeInit()
{
    free(buffer);
}
  
// Clear the screen
void AdafruitDisplayClearScreen()
{
    memset(buffer, 0, screenWidth_ * ((screenHeight_ + 7) / 8));
}

// Display the current drawn screen
void AdafruitDisplayUpdateScreen()
{
    static const uint8_t dlist1[] = {
      SSD1306_PAGEADDR,
      0,                         // Page start address
      0xFF,                      // Page end (not really, but works here)
      SSD1306_COLUMNADDR,
      0 };                       // Column start address
    AdafruitDisplayCommandList(dlist1, sizeof(dlist1));
    AdafruitDisplayCommand1(screenWidth_ - 1); // Column end address

    uint16_t count = screenWidth_ * ((screenHeight_ + 7) / 8);
    uint8_t *ptr   = buffer;

    nrf_gpio_pin_set(SSD1306_DC_PIN);
    while(count--) AdafruitDisplaySPIWrite(*ptr++);
}

// Draw pixel at the given (x,y) position
void AdafruitDisplayDrawPixel(int16_t x, int16_t y)
{
    if((x >= 0) && (x < screenWidth_) && (y >= 0) && (y < screenHeight_))
    {
        uint32_t BufferIndex = x + (y / 8) * screenWidth_;
        buffer[BufferIndex] |=  (1 << (y & 7));
    }
}

// Set the cursor at the given (x,y) position
void AdafruitDisplaySetCursor(int16_t x, int16_t y)
{ 
    cursorX_ = x;
    cursorY_ = y;
}

// Draw the given bitmap data at the given (x,y) position
void AdafruitDisplayDrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h)
{
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j=0; j<h; j++, y++) 
    {
        for(int16_t i=0; i<w; i++ )
        {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            if(byte & 0x80) AdafruitDisplayDrawPixel(x+i, y);
        }
    }
}

// Draw a line from point (x1,y1) to point (x2,y2)
void AdafruitDisplayWriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) 
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for (; x0<=x1; x0++) 
    {
        if (steep) 
        {
            AdafruitDisplayDrawPixel(y0, x0);
        }
        else
        {
            AdafruitDisplayDrawPixel(x0, y0);
        }
        err -= dy;
        if (err < 0) 
        {
            y0 += ystep;
            err += dx;
        }
    }
}

void AdafruitDisplayDrawFastVLine(int16_t x, int16_t y, int16_t h)
{
    AdafruitDisplayWriteLine(x, y, x, y+h-1);
}

void AdafruitDisplayWrite(uint8_t c)
{
    if(c == '\n') // Newline?
    {                        
        cursorX_  = 0;                     // Reset x to zero,
        cursorY_ += textSize_ * 8;          // advance y one line
    } 
    else if(c != '\r') // Ignore carriage returns
    {                 
        if(((cursorX_ + textSize_ * 6) > screenWidth_))  // Off right?
        {
            cursorX_  = 0;                 // Reset x to zero,
            cursorY_ += textSize_ * 8;      // advance y one line
        }
        AdafruitDisplayDrawChar(cursorX_, cursorY_, c, textSize_);
        cursorX_ += textSize_ * 6;          // Advance x one char
    }
}

void AdafruitDisplayDrawChar(int16_t x, int16_t y, unsigned char c, uint8_t size)
{
    // If it's not on the screen, we don't need to draw it
    if((x >= screenWidth_) || (y >= screenHeight_) || ((x + 6 * size - 1) < 0) ||  ((y + 8 * size - 1) < 0))
    {
        return;
    }

    for(int8_t i = 0; i < 5; i++)   // Char bitmap = 5 columns
    { 
        uint8_t line = font[c * 5 + i];
        for(int8_t j = 0; j < 8; j++, line >>= 1)
        {
            if(line & 1) 
            {
                if(size == 1)
                {
                    AdafruitDisplayDrawPixel(x+i, y+j);
                }
                else
                {
                    AdafruitDisplayFillRect(x+i*size, y+j*size, size, size);
                }
            } 
        }
    }
}

void AdafruitDisplayString(const char* string)
{
    unsigned int i = 0;
    while(string[i])
    {
        AdafruitDisplayWrite(string[i]);
        ++i;
    }
}

void AdafruitDisplayFillRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    for (int16_t i = x; i < x + w; i++)
    {
        AdafruitDisplayDrawFastVLine(i, y, h);
    }
}


// SPI transaction/selection must be performed in calling function.
void AdafruitDisplaySPIWrite(uint8_t d) 
{
    for(uint8_t bit = 0x80; bit; bit >>= 1)
    {
        nrf_gpio_pin_write(SSD1306_DATA_PIN, d & bit);
        nrf_gpio_pin_set(SSD1306_CLOCK_PIN);
        nrf_gpio_pin_clear(SSD1306_CLOCK_PIN);
    }
}

// Issue single command to SSD1306.
// Because command calls are often grouped, SPI transaction and selection
// must be started/ended in calling function for efficiency.
// This is a private function, not exposed (see ssd1306_command() instead).
void AdafruitDisplayCommand1(uint8_t c)
{
    nrf_gpio_pin_clear(SSD1306_DC_PIN);
    AdafruitDisplaySPIWrite(c);
}

// Issue list of commands to SSSD1306
void AdafruitDisplayCommandList(const uint8_t *c, uint8_t n)
{
    nrf_gpio_pin_clear(SSD1306_DC_PIN);
    while(n--)
    {
        AdafruitDisplaySPIWrite(*c);
        ++c;
    }
}
