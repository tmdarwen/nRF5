Nordic nRF52840 Basic Project Template for Segger Embedded Studio
=================================================================

**Description**

This is a basic project template for the [Nordic nRF52840 Development Board](https://www.mouser.com/new/nordic-semiconductor/nordic-nRF52840-dev-kit/) using [Segger Embedded Studio](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/Segger-Embedded-Studio) and the [nRF5 SDK](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK).

After loading the project in Embedded Studio, you'll need to update the "Global Macros" setting to add the NRF SDK location.  The global macros setting can be found by selecting the "Tools" pulldown menu, clcking "Building" and you'll see "Global Macros" under the "Build" section.  As an example, if you installed the nRF5 SDK at "C:\Nordic\nRF5SDK" you'll want to set the global macro as "NRFSDK="C:\Nordic\nRF5SDK".  After doing so, you should be able to build and run the basic project template.

The basic project template program simply displays "Hello World(n)" to a terminal program such as [PuTTY](https://www.putty.org/), where n starts at 0 and increments each time "Hello World" is printed.  "Hello World" will print once at startup and everytime a key is pressed in the terminal program.

 

**Licensing**

Aside from specific Nordic code, the MIT License applies to all code authored by Terence M. Darwen within this project:

*Copyright (c) 2020 Terence M. Darwen - tmdarwen.com*

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
