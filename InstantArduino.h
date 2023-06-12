/** @file InstantArduino.h 
    @brief Common Arduino centric include

    MIT License

    Copyright (c) 2023 Pavlo M, see https://github.com/olvap80/InstantRTOS

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef InstantArduino_INCLUDED_H
#define InstantArduino_INCLUDED_H

// Main include file for the Arduino SDK (makes vscode hints happy)
#include "Arduino.h"


// General utility headers _____________________________________________________

// Simple minimalistic coroutines suitable for all various platforms 
// (like Arduino!) for the case when native C++ coroutines are too heavyweight
// (or when co_yield and stuff does not work)
#include "InstantCoroutine.h"

// Fast deterministic delegates for invoking callbacks,
// suitable for real time operation (no heap allocation at all)
#include "InstantDelegate.h"


// Timing, intervals and scheduling ____________________________________________

// Simple timing classes to track timings in platform independent way
#include "InstantTimer.h"

// The simplest possible portable scheduler suitable for embedded 
// platforms like Arduino (actually only standard C++ is required)
//#include "InstantScheduler.h"


// Memory and queueing _________________________________________________________

// Simple deterministic memory management utilities suitable for real time
// can be used for dynamic memory allocations on Arduino and similar platform
//#include "InstantMemory.h"

// Simple deterministic queues suitable for real time
// can be used for dynamic memory allocations on Arduino and similar platforms.
//#include "InstantQueue.h"


// Other handy utility stuff ___________________________________________________

// General debouncing, see ArduinoDebounce below
#include "InstantDebounce.h"

//Handle hardware signals being mapped to memory
//#include "InstantSignals.h"





#endif
