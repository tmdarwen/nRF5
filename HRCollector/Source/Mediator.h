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

#pragma once

#include <stdint.h>

// The possible different states for the HR Collector
enum HRMediatorState
{
    STARTUP,
    SCANNING,
    CONNECTED
};

// Performs necessary initialization of the Adafruit display, etc.
void HRMediatorInit();

// Update the current state of the HR Collector
void HRMediatorSetState(enum HRMediatorState State);

// Add the latest heart rate value to the
void HRMediatorPushHeartRate(uint16_t HRValue);

// Update the current battery level percentage
void HRMediatorSetBatteryLevel(uint8_t BatteryLevel);

// Call perdioically to update the Adafruit display, etc
void HRMediatorUpdate();