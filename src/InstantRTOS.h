/** @file InstantRTOS.h 
 @brief Cumulative include with all the files

Include all InstantRTOS headers at once with configuration applied.
NOTE: all the components are (relatively) independent of each other
      and are designed to be separable (and operable independently) 

See https://github.com/olvap80/InstantRTOS#features-implemented-so-far 
for explanation why each component is needed :)

See TBD for tutorial

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


#include "InstantRTOS.Config.h"


// General utility headers _____________________________________________________

#include "InstantCoroutine.h"
#include "InstantDelegate.h"
//#include "InstantTask.h"


// Timing, intervals and scheduling ____________________________________________

#include "InstantScheduler.h"
#include "InstantTimer.h"


// Memory and queueing _________________________________________________________

#include "InstantMemory.h"
#include "InstantQueue.h"


// Other handy utility stuff ___________________________________________________

#include "InstantDebounce.h"
#include "InstantSignals.h"
