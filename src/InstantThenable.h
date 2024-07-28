/** @file InstantThenable.h
    @brief Simplest "thenable" to be used as "issue that callback when ready"
           Allow separated in time moments of call and obtaining result.

Thenable is intended to catch "happened (before)" event for those cases when
event source can produce event before we are ready to process it.
This can be seen as "subscriptions with memory"
It is suitable for InstantTask.h to allow "awaiting" for events without
the need to "block" on OS kernel API calls.

One can treat this as "functional event" to tie producer and consumer.
(this is NOT a kind of "promise/A+", there is no "thenable chaining",
    use InstantTask.h to await for multiple "thenable"s in chain!!!)

Thenable works as "one shot per Then", this means that one needs to
explicitly "await" again for event by "subscribing" callback to Then.
NOTE: it is also safe to subscribe new callback while previous is in progress!

Implementation is optimized for usage in embedded systems.

Q: what if invoked from the interrupt
A: there is configurable (optional) protection!
   there is optional support for subscribing and invoking from 
   separated threads/interrupts

 @code
    TODO: sample
 @endcode

NOTE: InstantThenable.h is configurable for interrupt (thread) safety.
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

#ifndef InstantThenable_INCLUDED_H
#define InstantThenable_INCLUDED_H

//Delegates (compact lambda callbacks) are used for attaching "code" to Thenable 
#include "InstantDelegate.h"
//Thenable stores arrived result in "LifetimeManager" allocator member 
#include "InstantMemory.h"


//______________________________________________________________________________
// Configurable error handling and interrupt safety

/* Common configuration to be included only if available
   (you can separate file and/or configure individually
    or just skip that to stick with defaults) */
#if defined(__has_include) && __has_include("InstantRTOS.Config.h")
#   include "InstantRTOS.Config.h"
#endif

#ifndef InstantThenable_EnterCritical
#   if defined(InstantRTOS_EnterCritical) && !defined(InstantTask_SuppressEnterCritical)
        //we have access from interrupts and/or multithreading
#       define InstantThenable_EnterCritical InstantRTOS_EnterCritical
#       define InstantThenable_LeaveCritical InstantRTOS_LeaveCritical
#       if defined(InstantRTOS_MutexObjectType)
#           define InstantThenable_MutexObjectType InstantRTOS_MutexObjectType
#           define InstantThenable_MutexObjectVariable InstantRTOS_MutexObjectVariable
#       endif
#   else
#       if 1
            //no interrupts/multithreading, so optimize it out where possible
#           define InstantThenable_NoMultithreadingProtection
            //do not change in this branch, activate next branch to tune manually 
#           define InstantThenable_EnterCritical
#           define InstantThenable_LeaveCritical
#       else
            //place for custom implementation for InstantThenable.h
            //(usually not needed to get here, is mutually )
#           define InstantThenable_EnterCritical
#           define InstantThenable_LeaveCritical
#       endif
#   endif
#endif


//______________________________________________________________________________
// Public API

//TODO: ThenableWhenAll, ThenableWhenAll
//TODO: Use auto on global scope to construct thenable chains via expression templates!!!


template<class T>
class ThenableToResolve;
template<>
class ThenableToResolve<void>;


/// Simplest "thenable" to be used as "issue that callback handler once ready"
/** Here moments of call and obtaining result can be separated in time,
 * this means "thenable" can be called before "awaiting listener" (handler)
 * is "subscribed" with the help of .Then(...) API.
 * Actually this is just a slot to hold a result of "something calculated",
 * or the fact that "something happened"
 * There is no predefined behavior or rule on when calculation starts,
 * the only rule is that attached callback (consumer) is called once 
 * "calculation completes" (producer provides value).
 * "Calculated value" (result) is preserved if consumer is not yet attached,
 * thus the order of attaching does not matter!
 * Producer just calls operator() in ThenableToResolve to provide value to consumer. 
 * (this is NOT a kind "promise/A+", but just class to tie producer and
 *  consumer together, there is no "returning of other thenable",
 *  use InstantTask.h to await for multiple "thenable"s in chain! ))
 * NOTE: There are no limitation on the number of ThenableToResolve::operator() calls
 *       (there will be as many callback calls 
 *        as many times is the operator() was is called,
 *        but StoredResult will remember only the last one)
 * TaskAwait from InstantTask.h accepts Thenable as a point for resuming */
template<class T>
class Thenable: private Delegate< void(const T& result) >{
public:
    // cannot copy such Thenable (as there is no "state sharing" for it!)
    Thenable(const Thenable& other) = delete;
    Thenable& operator=(const Thenable& other) = delete;

    /// The simple signature corresponding to Thenable
    using Signature = void(const T& result);
    /// Type for callback (handler) to be passed to Then
    using Callback = Delegate< Signature >;

    /// Original template type (type of stored value, if any)
    using ResultType = T;
    /// Type of the argument received by the callback
    using ArgumentType = const T&;


    /// Setup initial Thenable to work 
    Thenable();

    /// Setup with already attached handler
    Thenable(const Callback& eventCallbackHandler);


    /// Setup new callback (handler) to execute on ThenableToResolve::operator()
    /** Callback will execute immediately if Thenable was previously called,
     *  and only one time even if there were multiple previous calls.
     *  After that such callback will execute once ThenableToResolve::operator()
     * NOTE: there is no way to return more events for chaining */
    void Then(const Callback& eventCallbackHandler);


    /// Setup new callback to execute only on operator() call
    /** Callback will execute only once operator() called,
     *  precious calls on operator() have no effect
     * NOTE: there is no way to return more events for chaining */
    void Set(const Callback& eventCallback);


    /// Explicitly ignore that thenable
    /** Attach "do nothing" callback
     *  NOTE: such "do nothing" callback will "eat" result as with then.
     *        is case if we already have stored result (were called) */
    void ExplicitlyIgnore();


    /// Obtain number of event that did not invoke eventCallback
    /** Can provide nonzero value only before Then(...) or Set(...),
     * or after ResetCallback() is called.
     * @returns number on times operator() without callback,
     *          always 0 when callback was set */
    unsigned UntrackedEventsCount() const;

    /// The last known stored value (when UntrackedEventsCount() != 0)
    /**  UntrackedEventsCount() != 0 means parameter of the latest ::operator()
     * is stored here as long as Then() or Set() was not called.
     * That value is passed to Then callback once provided  */
    LifetimeManager<T>& StoredResult();


    /// Reset to initial state (will silently count again)
    void ResetCallback();

private:
    friend class ThenableToResolve<T>;

    /// One can create only ThenableToResolve instances
    /** Thenable can be passed only by reference,
     * and cannot exist separated from caller */
    ~Thenable() = default;

    /// Just a placeholder to mark "unsubscribed" thenable
    static void markerForThenableWithoutSubscription(
        const Callback*, const T&
    ){}

    /// Special helper for ExplicitlyIgnore
    static void doNothing(const T&){}

    /// Store value for the time when there is no subscription
    LifetimeManager<T> storedResult;

#if !defined(InstantThenable_NoMultithreadingProtection) && defined(InstantThenable_MutexObjectType)
    InstantThenable_MutexObjectType InstantThenable_MutexObjectVariable;
#endif
};

/// Invocable thenable to allow issuing corresponding callback  
template<class T>
class ThenableToResolve: public Thenable<T>{
    using Base = Thenable<T>;
public:
    //inherit constructors from base as is
    using Base::Base;

    /// Type for callback (handler) to be passed to Then
    using Callback = typename Thenable<T>::Callback;

    /// Invoke the callback (or remember result)
    /** Allow calling Thenable "event" with const T& result argument,
       this turns that event to "callable thing" (functor)!
       Prefer direct call of that event (the fastest way to work),
       one can wrap reference to Thenable in Delegate (Callback),
       but remember Thenable shall live as long as there are references.

       REMEMBER: the operator() will extract and issue handler if it is present
                 and remember the fact of call if there is no callback/handler.
                 Once more results/events are needed,
                 one has to "subscribe again" after callback (handler) happened.

       NOTE: uniform call sticks with Delegate< void(const T& result),
             there is not sense to do any "perfect forwarding" )) */
    void operator()(const T& result);
};


/// Callable Event that remembers the fact of calls before callback is attached
/** Special case of Thenable<void> with void as type
 * It is possible to call Thenable<void> before callback is attached,
 * call counts is accumulated and can be obtained via UntrackedEventsCount(),
 * Then(...) or Set(...) API can be used to tie with arrived calls */
template<>
class Thenable<void>: private Delegate<void()>{
public:
    // cannot copy such Thenable (as there is no "state sharing" for it!)
    Thenable(const Thenable& other) = delete;
    Thenable& operator=(const Thenable& other) = delete;

    /// The simple signature corresponding to Thenable
    using Signature = void();
    /// Type for callback (handler) to be passed to Then
    using Callback = Delegate< Signature >;

    /// Type of the argument received by the callback
    using ArgumentType = void;


    /// Setup initial Thenable to work 
    Thenable();

    /// Setup already attached event
    Thenable(const Callback& eventCallback);


    /// Setup new callback (handler) to execute on (and after) operator() call
    /** Callback will execute immediately if Thenable was previously called,
     *  and only one time even if there were multiple previous calls.
     *  After that such callback will execute once operator()
     * NOTE: there is no way to return more events for chaining */
    void Then(const Callback& eventCallbackHandler);


    /// Setup new callback to execute only on operator() call
    /** Callback will execute only once operator() called,
     *  precious calls on operator() have no effect
     * NOTE: there is no way to return more events for chaining */
    void Set(const Callback& eventCallback);


    /// Explicitly ignore that thenable
    /** Attach "do nothing" callback
     *  NOTE: such "do nothing" callback will "eat" result as with then.
     *        is case if we already have stored result (were called) */
    void ExplicitlyIgnore();


    /// Obtain number of event that did not invoke eventCallback
    /** Can provide nonzero value only before Then(...) or Set(...),
     * or after ResetCallback() is called.
     * @returns number on times operator() without callback,
     *          always 0 when callback was set */
    unsigned UntrackedEventsCount() const;


    /// Reset to initial state (will silently count again)
    void ResetCallback();

private:
    friend class ThenableToResolve<void>;

    /// One can create only ThenableToResolve instances
    /** Thenable can be passed only by reference,
     * and cannot exist separated from caller */
    ~Thenable() = default;

    /// Just a placeholder to mark "unsubscribed" thenable
    static void markerForThenableWithoutSubscription(const Callback*){}

    /// Special helper for ExplicitlyIgnore
    static void doNothing(){}

#if !defined(InstantThenable_NoMultithreadingProtection) && defined(InstantThenable_MutexObjectType)
    InstantThenable_MutexObjectType InstantThenable_MutexObjectVariable;
#endif
};

/// Invocable thenable to allow issuing corresponding callback  
template<>
class ThenableToResolve<void>: public Thenable<void>{
    using Base = Thenable<void>;
public:
    //inherit constructors from base as is
    using Base::Base;

    /// Type for callback (handler) to be passed to Then
    using Callback = typename Thenable<void>::Callback;

    /// Invoke the callback (or remember result)
    /** Allow calling Thenable "event" with const T& result argument,
       this turns that event to "callable thing" (functor)!
       Prefer direct call of that event (the fastest way to work),
       one can wrap reference to Thenable in Delegate (Callback),
       but remember Thenable shall live as long as there are references.

       REMEMBER: the operator() will extract and issue handler if it is present
                 or remember the fact of call if there is no callback/handler.
                 Once more results/events are needed,
                 one has to "subscribe again" after callback (handler) happened.
    */
    void operator()();
};


#ifdef InstantThenable_NoMultithreadingProtection
//NOTE: that warranty still works if InstantThenable_MutexObject expends to nothing
static_assert(
    sizeof( Thenable<void> ) == sizeof( Delegate<void()> ),
    "There is a warranty Thenable<void> costs as corresponding delegate!!!"
);
//NOTE: that warranty still works if InstantThenable_MutexObject expends to nothing
static_assert(
    sizeof( ThenableToResolve<void> ) == sizeof( Delegate<void()> ),
    "There is a warranty Thenable<void> costs as corresponding delegate!!!"
);
#endif



//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################



template<class T>
Thenable<T>::Thenable() : Callback(markerForThenableWithoutSubscription) {
    Callback::state.untrackedEventsCount = 0;
}

template<class T>
Thenable<T>::Thenable(const Callback& eventCallbackHandler)
    : Callback(eventCallbackHandler) {}


template<class T>
void Thenable<T>::Then(const Callback& eventCallbackHandler){
#ifdef InstantThenable_NoMultithreadingProtection
    //check there was a result waiting for that callback
    if( !storedResult ){
        //store to wait for future call
        Callback::operator=(eventCallbackHandler);
    }
    else{
        // call the operator one time as the sign there were other calls
        
        /* Make own stack copy to let the callback 
            to do "other Then" in the same place (this instance is reused) */
        T copy{static_cast<T&&>(*storedResult)};

        /* Value in storedResult is not needed any more
            (this also ensures eventCallback can attach other callback
            via Then withput causing it to immediately fire) */
        storedResult.DestroyOrPanic();

        // At least one event is just tracked )) 
        --Callback::state.untrackedEventsCount;

        /* Just execute callback,
            any other callback can overwrite it without problems
            NOTE: forwarding with static_cast<T&&> is useless for now */
        eventCallbackHandler( copy );
    }
#else
    LifetimeManager<T> storedResultCopy;

    {InstantThenable_EnterCritical
        //check there was a result waiting for that callback
        if( !storedResult ){
            //store to wait for future call
            Callback::operator=(eventCallbackHandler);
        }
        else{
            // call the operator one time as the sign there were other calls
            
            /* Make own stack copy to let the callback 
            to do "other Then" in the same place (this instance is reused) */
            storedResultCopy.Emplace(static_cast<T&&>(*storedResult));

            /* Value in storedResult is not needed any more
            (this also ensures eventCallback can attach other callback
                via Then withput causing it to immediately fire) */
            storedResult.DestroyOrPanic();

            // At least one event is just tracked )) 
            --Callback::state.untrackedEventsCount;
        }
    InstantThenable_LeaveCritical}

    if( storedResultCopy ){
        eventCallback( storedResultCopy );
    }
#endif
}

template<class T>
void Thenable<T>::Set(const Callback& eventCallback){
    InstantThenable_EnterCritical
    storedResult.Destroy(); //we are not interested
    Callback::operator=(eventCallback);
    InstantThenable_LeaveCritical
}


template<class T>
void Thenable<T>::ExplicitlyIgnore(){
    Then(doNothing);
}

template<class T>
unsigned Thenable<T>::UntrackedEventsCount() const {
    if( Callback::state.correspondingCaller != markerForThenableWithoutSubscription ){
        return 0;
    }
    return Callback::state.untrackedEventsCount;
}

template<class T>
LifetimeManager<T>& Thenable<T>::StoredResult(){
    return storedResult;
}


template<class T>
void Thenable<T>::ResetCallback(){
    InstantThenable_EnterCritical
    Callback::state.correspondingCaller = markerForThenableWithoutSubscription;
    Callback::state.untrackedEventsCount = 0;
    storedResult.Destroy();
    InstantThenable_LeaveCritical
}


template<class T>
void ThenableToResolve<T>::operator()(const T& result){
#ifdef InstantThenable_NoMultithreadingProtection
    if(     Callback::state.correspondingCaller 
        !=  Thenable<T>::markerForThenableWithoutSubscription
    ){
        //separate copy to allow new subscription inside callback (handler)
        Delegate< void(const T& result) > copy{ *(Callback*)this };

        Callback::state.correspondingCaller = Thenable<T>::markerForThenableWithoutSubscription;
        Callback::state.untrackedEventsCount = 0;
        copy(result);
    }
    else{
        ++Callback::state.untrackedEventsCount;
        Thenable<T>::storedResult.Force(result);
    }
#else
    Delegate< void(const T& result) > copy{ [](const T& result) {} };

    {InstantThenable_EnterCritical
        if(     Callback::state.correspondingCaller 
            !=  Thenable<T>::markerForThenableWithoutSubscription
        ){
            copy = *static_cast<Callback*>(this);

            Callback::state.correspondingCaller = Thenable<T>::markerForThenableWithoutSubscription;
            Callback::state.untrackedEventsCount = 0;
        }
        else{
            ++Callback::state.untrackedEventsCount;
            Thenable<T>::storedResult.Force(result);
        }
    InstantThenable_LeaveCritical}

    //copy is executed outside of "critical section" to avoid deadlocks
    copy(result);
#endif
}


inline Thenable<void>::Thenable() : Callback(markerForThenableWithoutSubscription) {
    Callback::state.untrackedEventsCount = 0;
}

inline Thenable<void>::Thenable(const Callback& eventCallback)
    : Callback(eventCallback) {}


inline void Thenable<void>::Then(const Callback& eventCallbackHandler){
#ifdef InstantThenable_NoMultithreadingProtection
    if(
        !(  Callback::state.correspondingCaller != markerForThenableWithoutSubscription
            && Callback::state.untrackedEventsCount
        )
    ){
        //store to wait for future call
        Callback::operator=(eventCallbackHandler);
    }
    else{
        // call the operator one time as the sign there were other calls

        // At least one event is just tracked )) 
        --Callback::state.untrackedEventsCount;

        /* Just execute callback,
        any other callback can overwrite it without problems
        NOTE: forwarding with static_cast<T&&> is useless for now */
        eventCallbackHandler();
    }
#else
    bool runNow = false;
    {InstantThenable_EnterCritical
        //check there was a result waiting for that callback
        if(
            !(  Callback::state.correspondingCaller != markerForThenableWithoutSubscription
                && Callback::state.untrackedEventsCount
            )
        ){
            //store to wait for future call
            Callback::operator=(eventCallbackHandler);
        }
        else{
            // call the operator one time as the sign there were other calls

            // At least one event is just tracked )) 
            --Callback::state.untrackedEventsCount;

            runNow = true;
        }
    InstantThenable_LeaveCritical}
    
    if( runNow ){
        eventCallbackHandler();
    }
#endif
}

inline void Thenable<void>::Set(const Callback& eventCallback){
    InstantThenable_EnterCritical
    Callback::operator=(eventCallback);
    InstantThenable_LeaveCritical
}


inline void Thenable<void>::ExplicitlyIgnore(){
    Then(doNothing);
}

inline unsigned Thenable<void>::UntrackedEventsCount() const {
    if( Callback::state.correspondingCaller != markerForThenableWithoutSubscription ){
        return 0;
    }
    return Callback::state.untrackedEventsCount;
}

inline void Thenable<void>::ResetCallback(){
    InstantThenable_EnterCritical
    Callback::state.correspondingCaller = markerForThenableWithoutSubscription;
    Callback::state.untrackedEventsCount = 0;
    InstantThenable_LeaveCritical
}


inline void ThenableToResolve<void>::operator()(){
#ifdef InstantThenable_NoMultithreadingProtection
    if(     Callback::state.correspondingCaller 
        !=  Thenable<void>::markerForThenableWithoutSubscription
    ){
        //separate copy to allow new subscription inside callback (handler)
        Delegate<void()> copy{ *(Callback*)this };

        Callback::state.correspondingCaller = Thenable<void>::markerForThenableWithoutSubscription;
        Callback::state.untrackedEventsCount = 0;

        copy();
    }
    else{
        ++Callback::state.untrackedEventsCount;
    }
#else
    Delegate< void() > copy{ []() {} };

    {InstantThenable_EnterCritical
        if(     Callback::state.correspondingCaller 
            !=  Thenable<void>::markerForThenableWithoutSubscription
        ){
            copy = *static_cast<Callback*>(this);

            Callback::state.correspondingCaller = markerForThenableWithoutSubscription;
            Callback::state.untrackedEventsCount = 0;
        }
        else{
            ++Callback::state.untrackedEventsCount;
        }
    InstantThenable_LeaveCritical}

    //copy is executed outside of "critical section" to avoid deadlocks
    copy();
#endif
}

#endif
