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

#include "HRQueue.h"

static uint16_t g_HRQueue[HR_QUEUE_MAX_SIZE];
static uint16_t g_HRQueueIndex = 0;
static uint16_t g_HRQueueSize = 0;

uint16_t HRQueueGetQueueSize()
{
    return g_HRQueueSize;
}

uint16_t HRQueueGetCurrentIndex()
{
    return g_HRQueueIndex;
}

void HRQueuePushBack(uint16_t HRValue)
{    
    // Add new value
    g_HRQueue[g_HRQueueIndex] = HRValue;

    // Increment the queue index, if we hit the max size, wrap around
    ++g_HRQueueIndex;
    if(g_HRQueueIndex == HR_QUEUE_MAX_SIZE)
    {
        g_HRQueueIndex = 0;
    }

    // Increment the queue size unless it's at the max
    if(g_HRQueueSize < HR_QUEUE_MAX_SIZE)
    {
        ++g_HRQueueSize;
    }
}

uint16_t HRQueueGet(uint16_t HRQueueIndex)
{    
    return g_HRQueue[HRQueueIndex];
}