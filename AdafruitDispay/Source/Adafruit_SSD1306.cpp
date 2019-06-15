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
 */

#include "Adafruit_SSD1306.h"

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
 
Adafruit_SSD1306::Adafruit_SSD1306(uint16_t screenWidth, uint16_t screenHeight, uint8_t textSize) : 
    Adafruit_GFX(screenWidth, screenHeight, textSize)
{      
    nrf_gpio_cfg_output(SSD1306_DATA_PIN);
    nrf_gpio_cfg_output(SSD1306_CLOCK_PIN);
    nrf_gpio_cfg_output(SSD1306_DC_PIN);
    nrf_gpio_cfg_output(SSD1306_RESET_PIN);
    nrf_gpio_cfg_output(SSD1306_CS_PIN);

    buffer = (uint8_t *)malloc(screenWidth_ * ((screenHeight_ + 7) / 8));

    Clear();

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
    ssd1306_commandList(init1, sizeof(init1));
    ssd1306_command1(screenHeight_ - 1);

    static const uint8_t init2[] = {
      SSD1306_SETDISPLAYOFFSET,             // 0xD3
      0x0,                                  // no offset
      SSD1306_SETSTARTLINE | 0x0,           // line #0
      SSD1306_CHARGEPUMP };                 // 0x8D
    ssd1306_commandList(init2, sizeof(init2));

    ssd1306_command1(0x14);

    static const uint8_t init3[] = {
      SSD1306_MEMORYMODE,                   // 0x20
      0x00,                                 // 0x0 act like ks0108
      SSD1306_SEGREMAP | 0x1,
      SSD1306_COMSCANDEC };
    ssd1306_commandList(init3, sizeof(init3));

    static const uint8_t init4b[] = {
      SSD1306_SETCOMPINS,                 // 0xDA
      0x12,
      SSD1306_SETCONTRAST };              // 0x81
    ssd1306_commandList(init4b, sizeof(init4b));
    ssd1306_command1(0x9F);

    ssd1306_command1(SSD1306_SETPRECHARGE); // 0xd9
    ssd1306_command1(0xF1);
    static const uint8_t init5[] = {
      SSD1306_SETVCOMDETECT,               // 0xDB
      0x40,
      SSD1306_DISPLAYALLON_RESUME,         // 0xA4
      SSD1306_NORMALDISPLAY,               // 0xA6
      SSD1306_DEACTIVATE_SCROLL,
      SSD1306_DISPLAYON };                 // Main screen turn on
    ssd1306_commandList(init5, sizeof(init5));
}

Adafruit_SSD1306::~Adafruit_SSD1306()
{
    free(buffer);
}

void Adafruit_SSD1306::Clear()
{
    memset(buffer, 0, screenWidth_ * ((screenHeight_ + 7) / 8));
}

void Adafruit_SSD1306::Display() 
{
    static const uint8_t dlist1[] = {
      SSD1306_PAGEADDR,
      0,                         // Page start address
      0xFF,                      // Page end (not really, but works here)
      SSD1306_COLUMNADDR,
      0 };                       // Column start address
    ssd1306_commandList(dlist1, sizeof(dlist1));
    ssd1306_command1(screenWidth_ - 1); // Column end address

    uint16_t count = screenWidth_ * ((screenHeight_ + 7) / 8);
    uint8_t *ptr   = buffer;

    nrf_gpio_pin_set(SSD1306_DC_PIN);
    while(count--) SPIWrite(*ptr++);
}


void Adafruit_SSD1306::DrawPixel(int16_t x, int16_t y)
{
    if((x >= 0) && (x < screenWidth_) && (y >= 0) && (y < screenHeight_))
    {
        uint32_t BufferIndex = x + (y / 8) * screenWidth_;
        buffer[BufferIndex] |=  (1 << (y & 7));
    }
}

// Issue single command to SSD1306.
// Because command calls are often grouped, SPI transaction and selection
// must be started/ended in calling function for efficiency.
// This is a private function, not exposed (see ssd1306_command() instead).
void Adafruit_SSD1306::ssd1306_command1(uint8_t c)
{
    nrf_gpio_pin_clear(SSD1306_DC_PIN);
    SPIWrite(c);
}

// Issue list of commands to SSSD1306
void Adafruit_SSD1306::ssd1306_commandList(const uint8_t *c, uint8_t n)
{
    nrf_gpio_pin_clear(SSD1306_DC_PIN);
    while(n--)
    {
        SPIWrite(*c);
        ++c;
    }
}

// SPI transaction/selection must be performed in calling function.
void Adafruit_SSD1306::SPIWrite(uint8_t d) 
{
    for(uint8_t bit = 0x80; bit; bit >>= 1)
    {
        nrf_gpio_pin_write(SSD1306_DATA_PIN, d & bit);
        nrf_gpio_pin_set(SSD1306_CLOCK_PIN);
        nrf_gpio_pin_clear(SSD1306_CLOCK_PIN);
    }
}
