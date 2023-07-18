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

#define InstantRTOS_TryAllowCritical

//TODO: move that logic to InstantTryCPU.h ))
#ifdef InstantRTOS_TryAllowCritical
#   if __has_include(<util/atomic.h>)
#       include <util/atomic.h>
#       define InstantRTOS_EnterCritical ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#       define InstantRTOS_LeaveCritical }
#   elif defined(xt_rsil)
#       define InstantRTOS_EnterCritical { uint32_t saved_iterrupts = xt_rsil(15)
#       define InstantRTOS_LeaveCritical xt_wsr_ps(saved_iterrupts)}
#   elif defined(interrupts)
        /* It is not the best option to always enable interrupts (from interrupt)
           but here we have the last hope to do something meaningful */
#   else
    //TODO: write your own here ))
#   endif

#endif


// TODO: Arduino specific error processing


// Add all InstantRTOS headers at once
#include "InstantRTOS.h"


#endif
