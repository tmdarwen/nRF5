/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */

#include "Adafruit_GFX.h"
#include "glcdfont.h"

#include <cstdlib>

#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }


Adafruit_GFX::Adafruit_GFX(uint16_t screenWidth, uint16_t screenHeight, uint8_t textSize) :
    screenWidth_(screenWidth), screenHeight_(screenHeight), textSize_(textSize) { }

void Adafruit_GFX::SetCursor(int16_t x, int16_t y)
{ 
    cursorX_ = x;
    cursorY_ = y;
}

void Adafruit_GFX::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h)
{
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    for(int16_t j=0; j<h; j++, y++) 
    {
        for(int16_t i=0; i<w; i++ )
        {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            if(byte & 0x80) DrawPixel(x+i, y);
        }
    }
}

void Adafruit_GFX::DrawFastVLine(int16_t x, int16_t y, int16_t h)
{
    WriteLine(x, y, x, y+h-1);
}

void Adafruit_GFX::WriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
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
            DrawPixel(y0, x0);
        }
        else
        {
            DrawPixel(x0, y0);
        }
        err -= dy;
        if (err < 0) 
        {
            y0 += ystep;
            err += dx;
        }
    }
}

size_t Adafruit_GFX::Write(uint8_t c)
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
        DrawChar(cursorX_, cursorY_, c, textSize_);
        cursorX_ += textSize_ * 6;          // Advance x one char
    }

    return 1;
}

void Adafruit_GFX::DrawChar(int16_t x, int16_t y, unsigned char c, uint8_t size)
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
                    DrawPixel(x+i, y+j);
                }
                else
                {
                    FillRect(x+i*size, y+j*size, size, size);
                }
            } 
        }
    }
}

void Adafruit_GFX::FillRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    for (int16_t i = x; i < x + w; i++)
    {
        DrawFastVLine(i, y, h);
    }
}