/*
 * Nordic nRF52840 BLE Heart Rate Collector
 *
 * Copyright (c) 2019 Terence M. Darwen - tmdarwen.com
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Module Description: Basically, a mediator between the BLE code and the
 * Adafruit Display.
 */

#include "Mediator.h"
#include "HRQueue.h"
#include "AdafruitDisplay.h"
#include "HeartImage.h"

// The position of the heart image on the scanning screen.
#define SCANNING_IMAGE_UPPER_LEFT_X        35
#define SCANNING_IMAGE_UPPER_LEFT_Y         0
#define SCANNING_TEXT_UPPER_LEFT_X         12
#define SCANNING_TEXT_UPPER_LEFT_Y         56

// Adafruit screen positions used to perform the animation on the heart image
// displayed during scanning for the a heart rate monitor.
#define ANIMATION_FILL_RECT_UPPER_LEFT_X   45
#define ANIMATION_FILL_RECT_UPPER_LEFT_Y   15
#define ANIMATION_FILL_RECT_WIDTH          37
#define ANIMATION_FILL_RECT_HEIGHT         21
#define ANIMATION_FILL_WIDTH_INCREMENT      5
#define ANIMATION_TOTAL_FRAMES             12
#define ANIMATION_FRAME_ZERO                0
#define ANIMATION_FRAMES_JUST_HEART         4

// The size of the Adafruit screen
#define DISPLAY_PIXEL_WIDTH               128
#define DISPLAY_PIXEL_HEIGHT               64

// The font size used on the Adafruit screen
#define DISPLAY_PIXEL_FONT_SIZE             1

// Keeps track of the current state of the HR Collector
static enum HRMediatorState MediatorState = STARTUP;

// Stores the current battery level percent of the HR Collector
static uint8_t BatteryLevel = 0;

// Non-public function prototypes
void HRMediatorDisplayScanningAnimation();
void HRMediatorDisplayHeartRateInfo();

void HRMediatorInit()
{
    AdafruitDisplayInit(DISPLAY_PIXEL_WIDTH, DISPLAY_PIXEL_HEIGHT, DISPLAY_PIXEL_FONT_SIZE);
}

void HRMediatorSetState(enum HRMediatorState State)
{
    MediatorState = State;
}

void HRMediatorPushHeartRate(uint16_t HR)
{
    HRQueuePushBack(HR);
}

void HRMediatorUpdate()
{
    if(MediatorState == STARTUP || MediatorState == SCANNING)
    {        
        HRMediatorDisplayScanningAnimation();
    }
    else if(MediatorState == CONNECTED)
    {
        HRMediatorDisplayHeartRateInfo();
    }
}

void HRMediatorSetBatteryLevel(uint8_t Level)
{
    BatteryLevel = Level;
}

void HRMediatorDisplayScanningAnimation()
{
    static uint32_t callCounter = 0;
    static uint32_t fillUpperLeftX = 0;
    static uint32_t fillWidth = 0;  

    uint32_t counterMod = callCounter % ANIMATION_TOTAL_FRAMES;
    ++callCounter;

    if(counterMod == ANIMATION_FRAME_ZERO) 
    {
        fillUpperLeftX = ANIMATION_FILL_RECT_UPPER_LEFT_X;
        fillWidth = 0;
    }
    else if(counterMod < ANIMATION_FRAMES_JUST_HEART) 
    {
      // No need to redraw, just return
      return;
    }
    else if(counterMod == ANIMATION_FRAMES_JUST_HEART)
    {
        fillUpperLeftX = ANIMATION_FILL_RECT_UPPER_LEFT_X;
        fillWidth = ANIMATION_FILL_RECT_WIDTH;
    }
    else
    {
        fillUpperLeftX += ANIMATION_FILL_WIDTH_INCREMENT;
        fillWidth -= ANIMATION_FILL_WIDTH_INCREMENT;
    }

    AdafruitDisplayClearScreen();
    AdafruitDisplayDrawBitmap(SCANNING_IMAGE_UPPER_LEFT_X, SCANNING_IMAGE_UPPER_LEFT_Y, 
                              HeartImage, HeartImageWidth, HeartImageHeight);
    AdafruitDisplaySetCursor(SCANNING_TEXT_UPPER_LEFT_X, SCANNING_TEXT_UPPER_LEFT_Y);
    AdafruitDisplayFillRect(fillUpperLeftX, ANIMATION_FILL_RECT_UPPER_LEFT_Y, 
                            fillWidth, ANIMATION_FILL_RECT_HEIGHT);
    AdafruitDisplayString("Scanning for HRM");
    AdafruitDisplayUpdateScreen();
}

void HRMediatorDisplayHeartRateInfo()
{
    static uint32_t prevHRQueueIndex = 0; 
    
    // Only refresh the display when we have new HR data
    if(prevHRQueueIndex == HRQueueGetCurrentIndex())
    {
        return;
    }
    prevHRQueueIndex = HRQueueGetCurrentIndex();


    AdafruitDisplayClearScreen();
    AdafruitDisplaySetCursor(0, 0);
    AdafruitDisplayString("Heart Rate:");

    AdafruitDisplaySetCursor(70, 0);

    uint8_t MostRecentHRQueueIndex = HRQueueGetCurrentIndex() ? HRQueueGetCurrentIndex() - 1 : HR_QUEUE_MAX_SIZE - 1;

    if(HRQueueGet(MostRecentHRQueueIndex) < 100)
    {
        uint8_t digit1 = HRQueueGet(MostRecentHRQueueIndex) % 10;
        uint8_t digit2 = HRQueueGet(MostRecentHRQueueIndex) / 10;
        AdafruitDisplayWrite(48 + digit2);
        AdafruitDisplayWrite(48 + digit1);
    }
    else
    {
        uint8_t digit1 = HRQueueGet(MostRecentHRQueueIndex) % 10;
        uint8_t digit2 = (HRQueueGet(MostRecentHRQueueIndex) - 100) / 10;
        uint8_t digit3 = HRQueueGet(MostRecentHRQueueIndex) / 100;
        AdafruitDisplayWrite(48 + digit3);
        AdafruitDisplayWrite(48 + digit2);
        AdafruitDisplayWrite(48 + digit1);
    }

    AdafruitDisplaySetCursor(0, 10);

    uint8_t digit1 = BatteryLevel % 10;
    uint8_t digit2 = BatteryLevel / 10;
    AdafruitDisplayString("Battery:");
    AdafruitDisplayWrite(48 + digit2);
    AdafruitDisplayWrite(48 + digit1);
    AdafruitDisplayWrite('%');

    AdafruitDisplaySetCursor(55, 10);

    uint8_t i = 0;
    uint8_t CurrentHRIndex = 0; //HRQueueIndex;
    if(HRQueueGetQueueSize() == HR_QUEUE_MAX_SIZE)
    {
      CurrentHRIndex = HRQueueGetCurrentIndex();
      if(CurrentHRIndex == HR_QUEUE_MAX_SIZE) CurrentHRIndex = 0;
    }
    uint8_t XPos = 0;
    for(i = 0; i < HRQueueGetQueueSize(); ++i)
    {                
        if(CurrentHRIndex == HR_QUEUE_MAX_SIZE) CurrentHRIndex = 0;

        // We got about 40 pixels in height to draw lines (pixels 24-to-64).
        // I consider min HR to be 50 and max HR to be 140.  This is 90 
        // different possibilities that need to be mapped to 40 pixels.
        // So first find the percentage position of the HR:
        float HRPercent = ((float)(HRQueueGet(CurrentHRIndex) - 50)) / 90.0f;

        // Then calculate pixel height.
        uint8_t PixelHeight = (uint8_t)(40.0f * HRPercent + 0.5f);
        uint8_t YPos = 64 - PixelHeight;
        AdafruitDisplayWriteLine(XPos, 64, XPos, YPos);
        XPos += 2;
        ++CurrentHRIndex;
    }                        

    AdafruitDisplayUpdateScreen();
}