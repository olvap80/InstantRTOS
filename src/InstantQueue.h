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
#if defined(__has_include) && __has_include("InstantRTOS.Config.h")
#   include "InstantRTOS.Config.h"
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

/// Universal queue of heterogenous items (of different size)
/**
 * @tparam lockedPlaceAllocation
 * @tparam placementUnderLock
 * @tparam lockedPlaceDeallocation
 * @tparam extractionUnderLock */ 
template<
    bool lockedPlaceAllocation = true,
    bool placementUnderLock = true,
    bool lockedPlaceDeallocation = true,
    bool extractionUnderLock = true
    //TODO: unsigned worstAlignment = sizeof ???
>
class UniversalQueue{
public:
    /// 
    using CountType = unsigned;
    
    /// Combine with ExtractionLoop in 
    static constexpr CountType BreakFlag = (~CountType(0) >> 1) + 1;
    static_assert(
        0 == (BreakFlag & (~CountType(0) >> 1)),
        "two's complement arithmetic is assumed"
    );

    /// Place content 
    template<class Functor>
    bool Place(
        CountType bytesRequested, ///< How many bytes are needed at most
        const Functor& placer ///< placer retuns number of bytes really written
    ){
        //Allocate place in the queue
        void* bytesLocation = nullptr; //TODO

        //if cannot allocate bytesRequested return false

        //Invoke placer
        auto bytesReallyPlaced = placer(bytesLocation);
        //panic if bytesReallyPlaced > bytesRequested

        //"Commit" bytesReallyPlaced

        return true; //item was placed
    }


    /// Extract one item we have so far
    template<class Functor>
    bool Extraction(
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

    /// try 
    template<class Functor>
    bool ExtractionLoop(CountType bytesRequested, const Functor& extractor){
        //return false; //no items were extracted at all 
        return true; // some items were extracted
    }
private:
    CountType total;
    CountType used; 
};


/// Queue holding items ot the same type
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


/// Queue containing runnable items (lambda also can be passed here!)
/**
 * Q: do we need signature here?
 * A: nope, seems to be overcomplication for exotic cases
 * TODO: cooperation with Delegate */
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
