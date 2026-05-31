/** @file InstantArduino.h 
 @brief Common Arduino centric include
        (and a nice sample on how InstantRTOS is integrated)

Include this file to have "every supported stuff" on Arduino.
Edit defaults to anything you like/need for your project. 

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

// Main include file for the Arduino SDK (this also makes vscode hints happy)
#include "Arduino.h"


//TODO: Arduino specific error processing

// Add all InstantRTOS headers at once
#include "InstantRTOS.h"

/*

/// Debounce single digital pin
class ArduinoDebounce{
public:
    /// Units being used for time measurements 
    using Milliseconds = SimpleTimer::Ticks;

    /// 
    ArduinoDebounce(
        uint8_t pinToWatch, ///Pin 
        Milliseconds pinDebounceMilliseconds = 50
    );

    ///
    void Start(uint8_t mode){
        digitalRead(pin);
    }



private:
    uint8_t pin;

    static_assert(
        sizeof(Milliseconds) == sizeof(decltype(millis()))
    );
};
*/

#endif
