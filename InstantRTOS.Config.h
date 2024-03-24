/** @file InstantRTOS.Config.h 
 @brief General configuration to include before files being configured

There are following "configurations" for InstantRTOS
(InstantRTOS works perfectly without them if you do not use corresponding
    feature, so you do not need to always define them)

REMEMBER: using only cooperative threads without involving interrupts and/or
            without coexisting with other RTOS 
            means you do not have to configure any Enter/LeaveCritical stuff,
            just leave corresponding macros undefined :)


# general configurations:

## Work with interrupts (or other RTOS)

See InstantRTOS.Config.CPU.h for details

## To allow standard headers 

By default standard headers are not included (as promised), but if you use
them any way, you can enable them by simply defining InstantRTOS_USE_STDLIB
this will allow using available standard library features by InstantRTOS 
those like size_t, uintptr_t, memcmp, memcpy...
 @code
    #define InstantRTOS_USE_STDLIB
 @endcode


## To catch Panic (unrecoverable condition)

The InstantRTOS_Panic macro is intended to report "impossible" conditions,
those that are not intended to happen but do happen.
By default each module passes own letter to InstantRTOS_Panic, like
InstantRTOS_Panic('Q') for queues.
(You can easy find all those modules with searching for string
    InstantRTOS_Panic(' via Ctrl+Shift+F/Find in Files feature :))


# file specific configurations

One can tune InstantRTOS behavior on per file basis, for example 
once only your queues are used from interrupts but Scheduler is not,
then just define InstantQueue_EnterCritical and InstantQueue_LeaveCritical
but leave InstantScheduler_EnterCritical, InstantScheduler_LeaveCritical
empty. This will alter only the module where this behavior is needed.


FileNameWithoutExtension+_EnterCritical
FileNameWithoutExtension+_LeaveCritical
...
etc...

Also one can alter other InstantRTOS_Something stuff on per file basis

FileNameWithoutExtension+_Panic
etc...

TODO: https://stackoverflow.com/questions/36381932/c-decrementing-an-element-of-a-single-byte-volatile-array-is-not-atomic-why
        https://stackoverflow.com/questions/52784613/which-variable-types-sizes-are-atomic-on-stm32-microcontrollers/


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

#ifndef InstantRTOS_Config_INCLUDED_H
#define InstantRTOS_Config_INCLUDED_H


// Workaround if your compiler does not support __has_include
#ifndef __has_include
#   define __has_include(file) 1
#endif

// Uncomment below to allow identifiers for objects (handy for debugging)
#define InstantRTOS_AllowObjectIdentifiers

#ifdef InstantRTOS_AllowObjectIdentifiers
    // Uncomment below 
    //#define InstantRTOS_BanUnidentifiedObjects

    //Uncomment below to define your custom object name
//#   define InstantRTOS_ObjectNameType
#endif


// Uncomment below to allow InstantRTOS_Enter/LeaveCritical
//#define InstantRTOS_TryAllowCritical

/* Make the best effort to take CPU specifics into account
  InstantRTOS_TryAllowCritical etc are important here */
#include "InstantRTOS.Config.CPU.h"

/* Uncomment defines below to suppress using of InstantRTOS_Enter/LeaveCritical
   for specific module.
   This is needed if you have allowed InstantRTOS_TryAllowCritical above, 
   or have defined own InstantRTOS_TryAllowCritical, but want to suppress
   using of InstantRTOS_TryAllowCritical for specific file,
   because that specific file is not used from interrupts/multiple threads.
   (this is actually an optimization)*/
//#define InstantMemory_SuppressEnterCritical
//#define InstantQueue_SuppressEnterCritical
//#define InstantScheduler_SuppressEnterCritical
//#define InstantCallback_SuppressEnterCritical

#endif
