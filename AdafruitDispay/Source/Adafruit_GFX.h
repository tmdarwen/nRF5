#ifndef _ADAFRUIT_GFX_H
#define _ADAFRUIT_GFX_H

#include <stdint.h>
#include <stddef.h>

/// A generic graphics superclass that can handle all sorts of drawing. 
/// At a minimum you can subclass and provide drawPixel().
class Adafruit_GFX
{
    public:
        Adafruit_GFX(uint16_t screenWidth, uint16_t screenHeight, uint8_t textSize=1);

        void SetCursor(int16_t x, int16_t y);

        void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);

        virtual void DrawPixel(int16_t x, int16_t y) = 0;

        void DrawFastVLine(int16_t x, int16_t y, int16_t h);
        
        void WriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

        size_t Write(uint8_t c);

        void DrawChar(int16_t x, int16_t y, unsigned char c, uint8_t size);

        void FillRect(int16_t x, int16_t y, int16_t w, int16_t h);

    protected:
        uint16_t screenWidth_;
        uint16_t screenHeight_;    

        int16_t cursorX_;       ///< x location to start print()ing text
        int16_t cursorY_;       ///< y location to start print()ing text

        uint8_t textSize_;

};

#endif