nRF52 and Adafruit IMU 
======================

**Hardware**

-   [Nordic nRF52840 Development Board](https://www.mouser.com/new/nordic-semiconductor/nordic-nRF52840-dev-kit/)
-   [Adafruit NXP Precision 9DoF IMU Breakout Board](https://learn.adafruit.com/nxp-precision-9dof-breakout/overview)

 


**Building**

-   The projects use [Segger Embedded Studio](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/Segger-Embedded-Studio).

-   The projects use the [nRF5 SDK](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK). 

 


**Wiring**
-    NXP VIN to nRF52 5V
-    NXP GND to nRF52 GND
-    NXP SCL to nRF52 P0.27
-    NXP SDA to nRF52 P0.26

 

**Running**
After building the software and flashing the binary to the nRF52, open a serial session using a terminal program such as [PuTTY](https://www.putty.org/).  The COM port to use can be found in Device Manager (if using Windows).  The baud rate is 115,200.  If everything is setup correctly, you should see the IMU data displayed every half second.
     

 


**Licensing**

Aside from specific Nordic and Adafruit code, the MIT License applies to all code 
authored by Terence M. Darwen within this repo:

*Copyright (c) 2019-2020 Terence M. Darwen - tmdarwen.com*

*Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:*

*The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.*

*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*
