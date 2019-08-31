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
 * Module Description: A simple fixed size FIFO queue for storing heart rate 
 * values.
 */

#pragma once

#include <stdint.h>

// The max number of heart rate values that can be stored in the queue.  Once this 
// queue size is reached, the oldest value is removed when adding a new value.
#define HR_QUEUE_MAX_SIZE  64

// Returns the current number of HR values in the queue
uint16_t HRQueueGetQueueSize();

// Returns the current index the queue is at.  This is the index at which the next 
// added item will be added to.
uint16_t HRQueueGetCurrentIndex();

// Add a new heart rate value to the queue.
void HRQueuePushBack(uint16_t HRValue);

// Get the heart rate value at the given index of the queue.
uint16_t HRQueueGet(uint16_t HRQueueIndex);