/** @file InstantRTOS.Config.CPU.h
 @brief Try to do "all the best" to use some CPU specifics


Those are needed ONLY if you need to interact with queueing and scheduling
from interrupts (or from threads of other multithreaded RTOS for those
rare and exotic cases when InstantRTOS coexists with some other RTOS
on the same device)


- InstantRTOS_EnterCritical - enter critical area (disable interrupts)
- InstantRTOS_LeaveCritical - revert interrupts back as before "Enter"

Sample for AVR (Like Arduino UNO or Leonardo)

OPTION1:
 @code
    #define InstantRTOS_EnterCritical { uint8_t oldSREG_value = SREG; cli();
    #define InstantRTOS_LeaveCritical SREG = oldSREG_value}
 @endcode
OPTION2:
 @code
    #define InstantRTOS_EnterCritical ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    #define InstantRTOS_LeaveCritical }
 @endcode

The InstantRTOS_MutexObjectType and InstantRTOS_MutexObjectVariable could be
useful on those plaforms, where there is no "disable interrupts" operation
to use but mutex is present. The InstantRTOS takes care to place macro
InstantRTOS_MutexObjectType and InstantRTOS_MutexObjectVariable in the right
place in the code, so it will be visible to Enter/LeaveCritical macros.
 @code
    TODO: sample here
    #define InstantRTOS_EnterCritical
    #define InstantRTOS_LeaveCritical
    #define InstantRTOS_MutexObjectType
    #define InstantRTOS_MutexObjectVariable
 @endcode

NOTE:   definitely you CANNOT obtain mutex from interrupt,
        sample above is only to illustrate concept
        for using InstantRTOS together with other with other (RT)OS
        where only mutex is available.

TODO: what about scoped macro?
TODO: move this to corresponding file



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

#ifndef InstantRTOS_Config_CPU_INCLUDED_H
#define InstantRTOS_Config_CPU_INCLUDED_H


//TODO: move that logic to  ))
#ifdef InstantRTOS_TryAllowCritical
#   if __has_include(<util/atomic.h>)
        //see also http://www.gammon.com.au/interrupts on interrupts on AVR
        //and https://arduino.stackexchange.com/questions/30968/how-do-interrupts-work-on-the-arduino-uno-and-similar-boards#comment60365_30969
#       include <util/atomic.h>
#       define InstantRTOS_EnterCritical ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
#       define InstantRTOS_LeaveCritical }
#       define InstantRTOS_MutexObject
#   elif defined(xt_rsil)
#       define InstantRTOS_EnterCritical { uint32_t saved_iterrupts = xt_rsil(15)
#       define InstantRTOS_LeaveCritical xt_wsr_ps(saved_iterrupts)}
#       define InstantRTOS_MutexObject
#   elif defined(interrupts)
        /* It is not the best option to always enable interrupts (from interrupt)
           but here we have the last hope to do something meaningful */
#   else
    //TODO: write your own here ))
#   endif

#endif

#endif