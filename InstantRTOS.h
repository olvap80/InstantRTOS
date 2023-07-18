/** @file InstantRTOS.h 
    @brief Common include with all files

    See TBD for tutorial
    See 

    See https://github.com/olvap80/InstantRTOS#features-implemented-so-far 
    for explanation why each component is needed :)


    There are following "configurations" for InstantRTOS
    (InstantRTOS works perfectly without them if you do not use corresponding
     feature, so you do not need to always define them)

    REMEMBER: using only cooperative threads without involving interrupts and/or
              without coexisting with other RTOS 
              means you do not have to configure any Enter/LeaveCritical stuff,
              just leave corresponding macros undefined :)
    
    # general configurations:

    ## To allow queueing and scheduling to work with interrupts (or other RTOS)
    
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

    InstantScheduler_EnterCritical
    InstantScheduler_LeaveCritical
    InstantQueue_EnterCritical
    InstantQueue_LeaveCritical
    InstantMemory_EnterCritical
    InstantMemory_LeaveCritical

    InstantCoroutine_Panic
    InstantMemory_Panic
    InstantQueue_Panic

    
*/

// General utility headers _____________________________________________________

#include "InstantCoroutine.h"
#include "InstantDelegate.h"


// Timing, intervals and scheduling ____________________________________________

#include "InstantScheduler.h"
#include "InstantTimer.h"


// Memory and queueing _________________________________________________________

#include "InstantMemory.h"
#include "InstantQueue.h"


// Other handy utility stuff ___________________________________________________

#include "InstantDebounce.h"
#include "InstantSignals.h"
