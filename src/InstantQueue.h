/** @file InstantQueue.h
 @brief Simple deterministic queues suitable for real time
        can be used for dynamic memory allocations on Arduino and similar platforms.

(c) see https://github.com/olvap80/InstantRTOS

Zero dependencies, works instantly by copy-pasting to your project...
Inspired by memory management toolset available in various RTOSes,
but now in C++  :)


TODO: advantages of heterogenous queues (linear memory usage)

NOTE: InstantQueue.h is configurable for interrupt (thread) safety.
      It is always safe to use the same object from the same thread.
      (different objects used from different threads will work as well).
      It is safe to use the same object from different threads/interrupts
      only if that interrupt (thread) safety is configured, see below

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

#ifndef InstantQueue_INCLUDED_H
#define InstantQueue_INCLUDED_H

//______________________________________________________________________________
// Configurable error handling and interrupt safety

/* Common configuration to be included only if available
   (you can separate file and/or configure individually
    or just skip that to stick with defaults) */
#if defined(__has_include)
#   if __has_include("InstantRTOS.Config.h")
#       include "InstantRTOS.Config.h"
#   endif
#endif

#ifndef InstantQueue_Panic
#   ifdef InstantRTOS_Panic
#       define InstantQueue_Panic() InstantRTOS_Panic('Q')
#   else
#       define InstantQueue_Panic() /* you can customize here! */ do{}while(true)
#   endif
#endif

//This will be used in the future
#ifndef InstantQueue_EnterCritical
#   if defined(InstantRTOS_EnterCritical) && !defined(InstantQueue_SuppressEnterCritical)
#       define InstantQueue_EnterCritical InstantRTOS_EnterCritical
#       define InstantQueue_LeaveCritical InstantRTOS_LeaveCritical
#       if defined(InstantRTOS_MutexObjectType)
#           define InstantQueue_MutexObjectType InstantRTOS_MutexObjectType
#           define InstantQueue_MutexObjectVariable InstantRTOS_MutexObjectVariable
#       endif
#   else
#       define InstantQueue_EnterCritical
#       define InstantQueue_LeaveCritical
#   endif
#endif

//______________________________________________________________________________
// Public API

/** Measure sizes requested for the queue,
Intentionally using signed type to allow "special values":
- Positive values are counts (requested or bytes).
- Zero means "no items" or "no bytes" or "no more items/bytes" available
  and does not mean error (it is up to the user if zero is acceptable in specific context),
- Negative values means error and are up to the user to handle */
using InstantQueueSSize = int;
/** Measure sizes available in the queue
(using signed type avoids UB and handles internal calculations properly) */
using InstantQueueSize = unsigned; //TODO: use size_t and ssize_t if available, see InstantMemory.h for example
static_assert(
    sizeof(InstantQueueSSize) == sizeof(InstantQueueSize),
    "InstantQueueSSize and InstantQueueSize must be of the same size");

//TODO: assert for callback signatures, they shall operate on InstantQueueSSize

/** Universal queue of heterogenous items (of different size!).
 All the supported variants are here for now.
 Derive from this base privately and open only the supported variants
 with the help of using-declarations, or just use the supported variants directly.
 @tparam worstAlignment */
template<
    InstantQueueSSize worstAlignment = sizeof(void*)
>
class FlexibleQueue{
public:
    /** Please content without acquiring locks/critical sections.
    This will work only if called in a context where no other thread/task can interfere.
    Or from the interrupt context assuming everyone else uses *Locked API versions */
    template<class Functor>
    InstantQueueSSize PlaceNoLocks(
        InstantQueueSSize bytesRequested, ///< How many bytes are needed at most
        const Functor& placer ///< placer retuns number of bytes really written
    ){
        //Allocate place in the queue
        void* bytesLocation = nullptr; //TODO
        InstantQueueSSize spaceReallyAvailable = 0; //TODO

        //if cannot allocate bytesRequested return false

        /*Invoke placer
        (placer shall handle if bytesRequested is greater than spaceReallyAvailable
         passing a value of spaceReallyAvailable less than bytesRequested
         is a normal way to indicate "failed to find enough space")
         TODO: is this right strategy? */
        auto bytesReallyPlaced = placer(bytesLocation, spaceReallyAvailable);
        static_assert(
            sizeof(bytesReallyPlaced) == sizeof(InstantQueueSSize),
            "Placer must return InstantQueueSSize"
        );

        /* Assume two's complement and that unsigned overflow is well defined (modulo 2^N),
        so if placer returns negative value, it means error, and if it returns value greater than spaceReallyAvailable, it means error as well */
        if( static_cast<InstantQueueSize>(bytesReallyPlaced) < static_cast<InstantQueueSize>(spaceReallyAvailable) ){
            //Normal and expected case, item was placed, we can "commit" it
            //TODO": "Commit" bytesReallyPlaced

        }

        /* Assume caller will handle error cases (negative value or value greater than spaceReallyAvailable) */
        return bytesReallyPlaced; //item was placed
    }

    /// Extract what we have so far
    template<class Functor>
    bool Extract(
        const Functor& extractor
    ){
        //Find place in the queue
        void* bytesLocation = nullptr; //TODO

        //return false; //no items were extracted at all

        //if there is no bytesRequested return false

        auto bytesReallyExtracted = extractor(bytesLocation/*, totalBytesAvailableToExtract*/);
        //panic if bytesReallyExtracted > totalBytesAvailableToExtract

        return true; // item was extracted
    }

private:
    /* We always operate on "solid" (unfragmented) items ap once,
    using pages mapped twice would be a nice trick, but not always possible,
    so we will just use linear buffer and leave fragment at the end unused
    BBBBBBBBBBFFFFFFFFFFF
    ^.........^
    g.........p

    BBBBBBBBBBBBBBBBBBBFF
    ^..................^
    g..................p
    FFFFFFBBBBBBBBFFFFFFF
    ......g.......p
    BBBFFFBBBBBBBBBBBBBUU
    ^.....^............^
    g.....p............u
    where:
    B - bytes occupied by items
    F - free bytes
    g(get) - start of the gap
    p(put) - end of the gap
    u(unused) - unused fragment at the end
    */
    InstantQueueSSize total;
    InstantQueueSSize used;
};


/// Queue holding items of the same type
template<class Item>
class SimpleQueue{
public:
    ///
    void Put(const Item& itemToCopy);

    ///
    void Put(const Item&& itemToCopy);

    ///
    template <class... Args>
    void Emplace(Args&&... args);

    ///
    bool HasPendingItems();

    ///
    Item& PendingItem();

    ///
    template<class Functor>
    bool ExtractIfAvailable(const Functor& functor){
        if( PendingItem() ){
            functor();
            return true;
        }
        return false;
    }
};

/// SimpleQueue with allocated storage
template<class Item, unsigned NumItems>
class SimpleQueueContainer{
public:
};


/** Queue containing runnable items (lambda also can be passed here!).
 Q: do we need signature here?
 A: nope, seems to be overcomplication for exotic cases
 TODO: cooperation with Delegate */
class ExecutableQueue{
public:

    ///
    bool HasPendingItems();

    ///Run item
    //TODO

    template<class Functor>
    bool RunIfAvailable(const Functor& functor){
        if( HasPendingItems() ){
            functor();
            return true;
        }
        return false;
    }

};


/// General queue holding items of different sizes
class HeterogenousQueueBase{
public:
    using CountType = unsigned;

    bool Put(void* rawMemory, CountType bytes);
    //bool

    //get?
};

/// Queue holding items of different sizes derived from common base
template<class ItemBase>
class HeterogenousQueue{

};


//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################



#endif
