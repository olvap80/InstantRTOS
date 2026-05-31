/** @file InstantReadyBox.h
    @brief Simplest "awaitable" to be used as "issue that callback when ready"
           Allow separated in time moments of call and obtaining result.
           Arduino friendly, fast and memory-efficient
           substitute for "subscriptions"/"futures/promise", for embedded systems.

ReadyBox works as "one shot per each OnReady", this means that one needs to
explicitly charge ("await" again on "consumer side") if more events are needed,
by "subscribing" callback to OnReady.

Invocation (producer side) and OnReady (consumer side) are in pair:
 -  invocation of ReadyBoxTrigger::operator() will place the result
    (and call the callback if it was already set)
 -  subscribing to ReadyBox::OnReady will remember the callback
    if there is no result yet, or will call the callback immediately
    if the result is already available.

NOTE: Before callback is called, it is removed from the ReadyBox
      and corresponding result is also removed.
      (callback receives the copy of the result as a parameter,
      so it is safe to make a new subscription with ReadyBox::OnReady)

ReadyBox is intended to also catch "happened (before)" event for those cases
once event source can produce some event before we are ready to process it.
This can be seen as a reusable "subscriptions with memory",
or a lightweight "waitable queue" with only one item, such that has a
"slot"/"holder" for the result of "something calculated somewhere",
or for accounting the fact that "something happened".
It is suitable for InstantTask.h to allow "awaiting" for events without
the need to "block" on OS kernel API calls.

 @code
    //actual readyTrigger (place is allocated statically)
    ReadyBoxTrigger<int> readyTrigger;
    ...

    //producer
    readyTrigger(42); //call operator() to provide the value to consumer
    ...

    //consumer
    readyTrigger.OnReady([](const int& result){
        Serial.print(F("Result is: "));
        Serial.println(result);
        // if needed we can .OnReady again here and reuse the same readyTrigger
    });
 @endcode
Here moments of the call and of the subscribing for the consumer
can be separated in time, and happen in any order.

NOTE: it is also safe to subscribe a new callback while previous is in progress!
      that new callback will wait for the new readiness,
      this is useful in some scenarios.

One can treat this as a reusable "functional event" to tie producer and consumer,
simplest form of the "queue" primitive with space for only one item,
or a lightweight and reusable "future/promise" to be resolved,
being very memory-efficient and suitable for embedded systems.
NOTE: This is NOT a Promise/A+ implementation: there is no chaining of "JS like thenables",
    and one cannot "return" another ReadyBox or attach multiple callbacks (no "thenable chaining").
    Use InstantTask.h to await multiple ReadyBox instances in sequence.)


Implementation is optimized for usage in embedded systems.

Q: what if invoked from the interrupt
A: there is configurable (optional) protection!
   there is optional support for subscribing and invoking from
   separated threads/interrupts

NOTE: InstantReadyBox.h is configurable for interrupt (thread) safety.
      It is always safe to use the same object from the same thread.
      (different objects used from different threads will work as well).
      It is safe to use the same object from different threads/interrupts
      only if that interrupt (thread) safety is configured, see below


MIT License

Copyright (c) 2023-2025 Pavlo M, see https://github.com/olvap80/InstantRTOS

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

#ifndef InstantReadyBox_INCLUDED_H
#define InstantReadyBox_INCLUDED_H

//Delegates (compact lambda callbacks) are used for attaching "code" to ReadyBox
#include "InstantDelegate.h"
//ReadyBox stores arrived result in "LifetimeManager" allocator member
#include "InstantMemory.h"


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

#ifndef InstantReadyBox_EnterCritical
#   if defined(InstantRTOS_EnterCritical) && !defined(InstantReadyBox_SuppressEnterCritical)
        //we have access from interrupts and/or multithreading
#       define InstantReadyBox_EnterCritical InstantRTOS_EnterCritical
#       define InstantReadyBox_LeaveCritical InstantRTOS_LeaveCritical
#       if defined(InstantRTOS_MutexObjectType)
#           define InstantReadyBox_MutexObjectType InstantRTOS_MutexObjectType
#           define InstantReadyBox_MutexObjectVariable InstantRTOS_MutexObjectVariable
#       endif
#   else
#       if 1
            //no interrupts/multithreading, so optimize it out where possible
#           define InstantReadyBox_NoMultithreadingProtection
            //do not change in this branch, activate next branch to tune manually
#           define InstantReadyBox_EnterCritical
#           define InstantReadyBox_LeaveCritical
#       else
            //place for custom implementation for InstantReadyBox.h
            //(usually not needed to get here, is mutually )
#           define InstantReadyBox_EnterCritical
#           define InstantReadyBox_LeaveCritical
#       endif
#   endif
#endif


//______________________________________________________________________________
// Public API

//TODO: ReadyBoxWhenAll, ReadyBoxWhenAny helpers (expression templates?)
//TODO: Use auto on global scope to construct ReadyBox chains via expression templates!!!


template<class T>
class ReadyBoxTrigger;
template<>
class ReadyBoxTrigger<void>;


/** Consumer part of ReadyBox for "issue the callback handler once ready".
 Here moments of call and obtaining result can be separated in time,
 this means derived "ReadyBoxTrigger" (see below)
 can be called before "awaiting listener" (handler)
 is "subscribed" with the help of .OnReady(...) API here.
 Actually this is just a slot to hold a result of "something calculated",
 or the fact that "something happened"
 There is no predefined behavior or rule on when calculation starts,
 the only rule is that attached callback (consumer) is called once
 "calculation completes" (producer provides value).
 The "Calculated value" (result) is preserved if consumer is not yet attached,
 thus the order of attaching does not matter!
 Producer just calls operator() in ReadyBoxTrigger to provide value to consumer.
 (This is NOT a Promise/A+; it simply ties producer and consumer.
 There is no returning of another ReadyBox; use InstantTask.h to await
 multiple ReadyBox objects in sequence.)

 NOTE:  There are no limitation on the number of ReadyBoxTrigger::operator() calls
        (there will be as many callback calls
        as many times is the operator() was is called,
        but StoredResult will remember only the last one)

 TaskAwait from InstantTask.h accepts ReadyBox as a point for resuming
 because we have .OnReady method here */
template<class T>
class ReadyBox: private Delegate< void(const T& result) >{
public:
    // cannot copy such ReadyBox (as there is no "state sharing" for it!)
    ReadyBox(const ReadyBox& other) = delete;
    ReadyBox& operator=(const ReadyBox& other) = delete;

    /// The simple signature corresponding to ReadyBox
    using Signature = void(const T& result);
    /// Type for callback (handler) to be passed to OnReady
    using Callback = Delegate< Signature >;

    /// Original template type (type of stored value, if any)
    using ResultType = T;
    /// Type of the argument received by the callback
    using ArgumentType = const T&;


    /// Setup initial ReadyBox to work
    ReadyBox();

    /// Setup with already attached handler
    ReadyBox(const Callback& eventCallbackHandler);


    /** Setup a new callback (handler) to execute on ReadyBoxTrigger::operator().
     Callback will execute immediately if
     derived ReadyBoxTrigger was previously called,
     and the result is already available in corresponding ReadyBox.
     In the case if no result is available yet, the callback will be
     stored/remembered and will be called once ReadyBoxTrigger::operator()
     is issued the next time.
     (callback is issued only one time if there were multiple previous calls,
     also callback is removed after being called,
     this means one must subscribe with OnReady again).
     The callback will receive the last known (stored) result as a parameter,
     ("result" is the stuff that was passed to ReadyBoxTrigger::operator()).
     When executing the callback, corresponding result is already
     moved out from the ReadyBox instance (ReadyBox can be reused again!),
     but a copy is still available for the original callback being executed.
     Meanwhile callback can register itself (or another callback) again
     to be called later on the next ReadyBoxTrigger::operator() call.
     @returns   Number of previously unhandled (pending) calls consumed now.
                non-zero value means eventCallbackHandler was immediately called.
                0 => none (eventCallbackHandler stored for future
                            and there were no pending calls/values to issue it with).
                1 => exactly one prior call to operator() existed
                            (the one that just triggered eventCallbackHandler).
                >1 => several; only the last value is delivered to eventCallbackHandler.
                (only the result of the last call is passed to the callback).

     NOTE:  It is guaranteed that setup for a new callback
            will not conflict with those currently being processed.
     NOTE:  It is guaranteed that code is never called from within critical section
            (this allows to avoid any deadlocks, but be careful with subscription order).
     NOTE:  ReadyBox can be passed only by reference,
            and cannot exist separated from caller (ReadyBoxTrigger)
     NOTE:  There is no way to return more events for chaining here
            (this is not a promise/A+ style) */
    unsigned OnReady(const Callback& eventCallbackHandler);

    /** Setup new callback to execute only on new operator() call (forget the past).
     Callback will execute only once new operator() is called,
     previous calls on operator() have no effect on it,
     (callback is issued only one time if there are multiple calls!
     this means one must subscribe with OnNext again).
     NOTE: see notes above for OnReady */
    void OnNext(const Callback& eventCallback);


    /** Explicitly ignore any pending events once (consume without user callback).
     Attach "do nothing" callback
     @note  such "do nothing" callback will "eat" the result
            of ReadyBoxTrigger::operator() call as with OnReady call,
     @returns number of events consumed (ignored) */
    unsigned ExplicitlyIgnore();

    /// Reset to initial state (will silently start counting from zero again)
    void ResetCallback();

private:
    friend class ReadyBoxTrigger<T>;

    /** One can create only ReadyBoxTrigger instances.
     The ReadyBox class can be passed only by reference,
     and cannot exist separated from the caller (ReadyBoxTrigger) */
    ~ReadyBox() = default;

    /// Just a placeholder to mark "unsubscribed" ReadyBox
    static void markerForUnsubscribedBox(
        const Callback*, const T&
    ){}

    /// Special helper for ExplicitlyIgnore
    static void doNothing(const T&){}

    /// Store the value for the time when there is no subscription
    LifetimeManager<T> storedResult;

#if !defined(InstantReadyBox_NoMultithreadingProtection) && defined(InstantReadyBox_MutexObjectType)
    InstantReadyBox_MutexObjectType InstantReadyBox_MutexObjectVariable;
#endif
};

/// Invocable ReadyBox to allow issuing corresponding callback
template<class T>
class ReadyBoxTrigger: public ReadyBox<T>{
    using Base = ReadyBox<T>;
public:
    //inherit the constructors "as is"
    using Base::Base;

    /// Type for the callback (handler) to be passed to OnReady
    using Callback = typename ReadyBox<T>::Callback;

    /** Invoke the callback (or remember the result).
     Allow calling ReadyBox "event" with const T& result argument,
     this turns that event to "callable thing" (functor)!
     Prefer direct call of that event (the fastest way to work),
     one can wrap reference to ReadyBox in Delegate (Callback),
     but remember ReadyBox shall live as long as there are references.
     REMEMBER:  the operator() will extract and issue handler if it is present
                and remember the fact of call if there is no callback/handler.
                Once more results/events are needed,
                one has to "subscribe again" after callback (handler) happened.

     NOTE:  uniform call sticks with Delegate< void(const T& result),
            there is no sense to do any "perfect forwarding" )).

     NOTE:  ReadyBoxTrigger<...>::operator() guarantees that by explicit copying
            callback is invoked outside of any additional critical section
            (this allows to avoid any deadlocks
            and to allow subscribing again directly from that callback). */
    void operator()(const T& result);

    /// Invoke with rvalue to support move-only types (stored via move when no subscriber)
    void operator()(T&& result);
};


/** Callable Event that remembers the fact of calls before callback is attached.
 Special case of the ReadyBox<void> with void as type
 It is possible to call ReadyBox<void> before the callback is attached,
 call counts is accumulated and can be obtained once OnReady is called,
 OnReady(...) or OnNext(...) API can be used to tie with arrived calls */
template<>
class ReadyBox<void>: private Delegate<void()>{
public:
    // cannot copy such ReadyBox (as there is no "state sharing" for it!)
    ReadyBox(const ReadyBox& other) = delete;
    ReadyBox& operator=(const ReadyBox& other) = delete;

    /// The simple signature corresponding to ReadyBox
    using Signature = void();
    /// Type for callback (handler) to be passed to OnReady
    using Callback = Delegate< Signature >;

    /// Type of the argument received by the callback
    using ArgumentType = void;


    /// Setup initial ReadyBox to work
    ReadyBox();

    /// Setup already attached event
    ReadyBox(const Callback& eventCallback);


    /** Setup new callback (handler) to execute on (and after) operator() call.
     Callback will execute immediately if
     derived ReadyBoxTrigger was previously called,
     and only one time even if there were multiple previous calls.
     In the case if no calls to operator() yet, the callback will be
     stored/remembered and will be called once ReadyBoxTrigger::operator()
     is issued the next time.
     (callback is issued only one time if there were multiple previous calls,
     also callback is removed after being called,
     this means one must subscribe with OnReady again).
     @returns   Number of previously unhandled (pending) calls consumed now.
                non-zero value means eventCallbackHandler was immediately called.
                0 => none (eventCallbackHandler stored for future
                    and there were no pending calls/values to issue it with).
                1 => exactly one prior call to operator() existed
                    (the one that just triggered eventCallbackHandler).
                >1 => several; only the last value is delivered to eventCallbackHandler.
                (only the result of the last call is passed to the callback).

     NOTE:  It is guaranteed that setup for a new callback
            will not conflict with those currently being processed.
     NOTE:  It is guaranteed that code is never called from within critical section
            (this allows to avoid any deadlocks, but be careful with subscription order).
     NOTE:  ReadyBox can be passed only by reference,
            and cannot exist separated from caller (ReadyBoxTrigger)
     NOTE:  There is no way to return more events for chaining here
            (this is not a promise/A+ style) */
    unsigned OnReady(const Callback& eventCallbackHandler);

    /** Setup new callback to execute only on new operator() call (forget the past).
     Callback will execute only once new operator() is called,
     previous calls on operator() have no effect on it,
     (callback is issued only one time if there are multiple calls!
     this means one must subscribe with OnNext again).
     NOTE: see notes above for OnReady */
    void OnNext(const Callback& eventCallback);


    /** Explicitly ignore that ReadyBox for one time.
     Attach "do nothing" callback
     NOTE:  such "do nothing" callback will "eat" result as with OnReady.
            is case if we already have stored result (were called)
     @returns number of events consumed (how many were ignored) */
    unsigned ExplicitlyIgnore();

    /// Reset to initial state (will silently count again)
    void ResetCallback();


    /** Release (move out) callback (if any) without executing it, leaves ReadyBox empty.
     Useful to "take away" callback to execute it later,
     or to move it to another ReadyBox instance.
     NOTE: if there were pending calls, they will be lost!
     @returns   the callback that was attached
                (empty callback if none was attached,
                continues to track pending calls in such case) */
    InstantDelegateNodiscard("One must use (call) returned value of TakeCallback to be consistent with intended usage")
    Callback TakeCallback();

private:
    friend class ReadyBoxTrigger<void>;

    /** One can create only ReadyBoxTrigger instances.
     ReadyBox can be passed only by reference,
     and cannot exist separated from the caller (ReadyBoxTrigger) */
    ~ReadyBox() = default;

    /// Just a placeholder to mark "unsubscribed" ReadyBox (and do nothing)
    static void markerForUnsubscribedBox(const Callback*){}

    /// Special helper for ExplicitlyIgnore
    static void doNothing(){}

#if !defined(InstantReadyBox_NoMultithreadingProtection) && defined(InstantReadyBox_MutexObjectType)
    InstantReadyBox_MutexObjectType InstantReadyBox_MutexObjectVariable;
#endif
};

/// Invocable ReadyBox to allow issuing corresponding callback
template<>
class ReadyBoxTrigger<void>: public ReadyBox<void>{
    using Base = ReadyBox<void>;
public:
    //inherit constructors from base as is
    using Base::Base;

    /// Type for callback (handler) to be passed to OnReady
    using Callback = typename ReadyBox<void>::Callback;

    /** Invoke the callback (or remember result).
     Allow calling ReadyBox "event" with const T& result argument,
     this turns that event to "callable thing" (functor)!
     Prefer direct call of that event (the fastest way to work),
     one can wrap reference to ReadyBox in Delegate (Callback),
     but remember ReadyBox shall live as long as there are references.

     REMEMBER:  the operator() will extract and issue handler if it is present
                or remember the fact of call if there is no callback/handler.
                Once more results/events are needed,
                one has to "subscribe again" after callback (handler) happened.

     NOTE:  ReadyBoxTrigger<...>::operator() guarantees that by explicit copying
            callback is invoked outside of any additional critical section
            (this allows to avoid any deadlocks
            and to allow subscribing again directly from that callback) */
    void operator()();
};


#ifdef InstantReadyBox_NoMultithreadingProtection
//NOTE: that warranty still works if InstantReadyBox_MutexObject expends to nothing
static_assert(
    sizeof( ReadyBox<void> ) == sizeof( Delegate<void()> ),
    "There is a warranty ReadyBox<void> costs as corresponding delegate!!!"
);
//NOTE: that warranty still works if InstantReadyBox_MutexObject expends to nothing
static_assert(
    sizeof( ReadyBoxTrigger<void> ) == sizeof( Delegate<void()> ),
    "There is a warranty ReadyBox<void> costs as corresponding delegate!!!"
);
#endif

/* LAYOUT CONTRACT: ReadyBox<void> privately derives from Delegate<void()>
   and repurposes Delegate::state.untrackedEventsCount (a union member
   overlapping the callee pointer) to count untracked events.
   The asserts below guard this dependency so that any refactoring
   of Delegate::State that would break ReadyBox<void> is caught. */
static_assert(
    sizeof( Delegate<void()> )
        == sizeof( void(* const *)(const Delegate<void()>*) ) + sizeof(void*),
    "ReadyBox<void> relies on Delegate being exactly two pointers "
    "(caller + union), layout has changed!"
);
static_assert(
    sizeof(unsigned) <= sizeof(void*),
    "ReadyBox<void> stores event count in a pointer-sized union member, "
    "unsigned must fit within pointer size"
);



//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################



template<class T>
ReadyBox<T>::ReadyBox() : Callback(markerForUnsubscribedBox) {
    //Constructor will also do Callback::state.untrackedEventsCount = 0;
}

template<class T>
ReadyBox<T>::ReadyBox(const Callback& eventCallbackHandler)
    : Callback(eventCallbackHandler) {}


template<class T>
unsigned ReadyBox<T>::OnReady(const Callback& eventCallbackHandler){
    unsigned previousUntrackedCount = 0;
#ifdef InstantReadyBox_NoMultithreadingProtection
    //check there was a result waiting for that callback
    if( !storedResult ){
        //store to wait for future call
        Callback::operator=(eventCallbackHandler);
    }
    else{
        // call the handler one time as the sign there were other calls

        /* Make own stack copy by move (what if T is move-only?),
           this copy will allow the callback
           to do "other OnReady" in the same place (this instance is reused) */
        T saveStoredResultToOwnCopy{static_cast<T&&>(*storedResult)};

        /* "moved out value" in storedResult is not needed any more
            (this also ensures eventCallbackHandler can attach other callback
            via OnReady withput causing it to immediately fire) */
        storedResult.DestroyOrPanic();

        // At least one event is just tracked ))
        previousUntrackedCount = Callback::state.untrackedEventsCount;
        Callback::state.untrackedEventsCount = 0;

        /* Just execute callback,
            any other callback can overwrite it without problems
            NOTE: forwarding with static_cast<T&&> is useless for now */
        eventCallbackHandler( saveStoredResultToOwnCopy );
    }
#else
    LifetimeManager<T> saveStoredResultToOwnCopy;

    {InstantReadyBox_EnterCritical
        //check there was a result waiting for that callback
        if( !storedResult ){
            //store to wait for future call
            Callback::operator=(eventCallbackHandler);
        }
        else{
            // call the handler one time as the sign there were other calls

            /* Make own stack copy to let the callback
            to do "other OnReady" in the same place (this instance is reused) */
            saveStoredResultToOwnCopy.Emplace(static_cast<T&&>(*storedResult));

            /* Value in storedResult is not needed any more
            (this also ensures eventCallback can attach other callback
                via OnReady withput causing it to immediately fire) */
            storedResult.DestroyOrPanic();

            // At least one event is just tracked ))
            previousUntrackedCount = Callback::state.untrackedEventsCount;
            Callback::state.untrackedEventsCount = 0;
        }
    InstantReadyBox_LeaveCritical}

    if( saveStoredResultToOwnCopy ){
        eventCallbackHandler( *saveStoredResultToOwnCopy );
    }
#endif
    return previousUntrackedCount;
}

template<class T>
void ReadyBox<T>::OnNext(const Callback& eventCallback){
    InstantReadyBox_EnterCritical
    storedResult.Destroy(); //we are not interested any more
    Callback::operator=(eventCallback);
    InstantReadyBox_LeaveCritical
}


template<class T>
unsigned ReadyBox<T>::ExplicitlyIgnore(){
    return OnReady(doNothing);
}


template<class T>
void ReadyBox<T>::ResetCallback(){
    InstantReadyBox_EnterCritical
    Callback::state.correspondingCaller = markerForUnsubscribedBox;
    Callback::state.untrackedEventsCount = 0;
    storedResult.Destroy();
    InstantReadyBox_LeaveCritical
}


template<class T>
void ReadyBoxTrigger<T>::operator()(const T& result){
#ifdef InstantReadyBox_NoMultithreadingProtection
    if(     Callback::state.correspondingCaller
        !=  ReadyBox<T>::markerForUnsubscribedBox
    ){
        //We have custom handler

        //separate copy to allow new subscription inside callback (handler)
        Delegate< void(const T& result) > copy{ *(Callback*)this };

        //mark as "no handler" (start from scratch)
        Callback::state.correspondingCaller = ReadyBox<T>::markerForUnsubscribedBox;
        Callback::state.untrackedEventsCount = 0;

        //copy will be able to subscribe again
        copy(result);
    }
    else{
        //No custom handler: store the latest result
        ++Callback::state.untrackedEventsCount;
        ReadyBox<T>::storedResult.Force(result);
    }
#else
    //Nothing happens if there is no subscription found below
    Delegate< void(const T& result) > copy{ [](const T& result) {} };

    {InstantReadyBox_EnterCritical
        if(     Callback::state.correspondingCaller
            !=  ReadyBox<T>::markerForUnsubscribedBox
        ){
            //We have custom handler

            copy = *static_cast<Callback*>(this);

            Callback::state.correspondingCaller = ReadyBox<T>::markerForUnsubscribedBox;
            Callback::state.untrackedEventsCount = 0;
        }
        else{
            ++Callback::state.untrackedEventsCount;
            ReadyBox<T>::storedResult.Force(result);
        }
    InstantReadyBox_LeaveCritical}

    //copy is executed outside of "critical section" to avoid deadlocks
    copy(result);
#endif
}

// Added overload to support move-only types (e.g., std::unique_ptr).
// Rvalue events are forwarded and stored/moved without requiring a copy.
template<class T>
void ReadyBoxTrigger<T>::operator()(T&& result){
#ifdef InstantReadyBox_NoMultithreadingProtection
    if(     Callback::state.correspondingCaller
        !=  ReadyBox<T>::markerForUnsubscribedBox
    ){
        //We have custom handler

        //separate onstack copy to allow new subscription inside callback (handler)
        Delegate< void(const T& result) > ownCopyOfCallback{ *(Callback*)this };

        Callback::state.correspondingCaller = ReadyBox<T>::markerForUnsubscribedBox;
        Callback::state.untrackedEventsCount = 0;

        //copy will be able to subscribe again
        ownCopyOfCallback(result); // pass as lvalue reference (handler sees const T&)
    }
    else{
        //No custom handler: store the latest result (move)
        ++Callback::state.untrackedEventsCount;
        ReadyBox<T>::storedResult.Force(static_cast<T&&>(result));
    }
#else
    //Nothing happens if there is no subscription found below
    Delegate< void(const T& result) > ownCopyOfCallback{ [](const T&){ } };

    {InstantReadyBox_EnterCritical
        if(     Callback::state.correspondingCaller
            !=  ReadyBox<T>::markerForUnsubscribedBox
        ){
            //We have custom handler
            ownCopyOfCallback = *static_cast<Callback*>(this);

            Callback::state.correspondingCaller = ReadyBox<T>::markerForUnsubscribedBox;
            Callback::state.untrackedEventsCount = 0;
        }
        else{
            //No custom handler: store the latest result (move)
            ++Callback::state.untrackedEventsCount;
            ReadyBox<T>::storedResult.Force(static_cast<T&&>(result));
        }
    InstantReadyBox_LeaveCritical}

    //copy is executed outside of "critical section" to avoid deadlocks
    ownCopyOfCallback(result);
#endif
}


inline ReadyBox<void>::ReadyBox() : Callback(markerForUnsubscribedBox) {
    Callback::state.untrackedEventsCount = 0;
}

inline ReadyBox<void>::ReadyBox(const Callback& eventCallback)
    : Callback(eventCallback) {}


inline unsigned ReadyBox<void>::OnReady(const Callback& eventCallbackHandler){
    unsigned previousUntrackedCount = 0;
#ifdef InstantReadyBox_NoMultithreadingProtection
    if(
            // either we had other custom handler (just overwrite it)
            Callback::state.correspondingCaller != markerForUnsubscribedBox
            // or we had no untracked events without a handler
        ||  !Callback::state.untrackedEventsCount
    ){
        //did not happen before: store to wait for future call
        Callback::operator=(eventCallbackHandler);
    }
    else{
        // call the handler one time as the sign there were other calls

        // At least one event is just tracked ))
        previousUntrackedCount = Callback::state.untrackedEventsCount;
        Callback::state.untrackedEventsCount = 0;

        /* Just execute callback,
        any other callback can overwrite it without problems
        NOTE: forwarding with static_cast<T&&> is useless for now */
        eventCallbackHandler();
    }
#else
    bool runNow = false;
    {InstantReadyBox_EnterCritical
        //check there was a result waiting for that callback
        if(
                // either we had other custom handler (just overwrite it)
                Callback::state.correspondingCaller != markerForUnsubscribedBox
                // or we had no untracked events without a handler
            ||  !Callback::state.untrackedEventsCount
        ){
            //store to wait for future call
            Callback::operator=(eventCallbackHandler);
        }
        else{
            // call the handler one time as the sign there were other calls

            // At least one event is just tracked ))
            previousUntrackedCount = Callback::state.untrackedEventsCount;
            Callback::state.untrackedEventsCount = 0;

            runNow = true;
        }
    InstantReadyBox_LeaveCritical}

    if( runNow ){
        eventCallbackHandler();
    }
#endif
    return previousUntrackedCount;
}

inline void ReadyBox<void>::OnNext(const Callback& eventCallback){
    InstantReadyBox_EnterCritical
    Callback::operator=(eventCallback);
    InstantReadyBox_LeaveCritical
}


inline unsigned ReadyBox<void>::ExplicitlyIgnore(){
    return OnReady(doNothing);
}

inline void ReadyBox<void>::ResetCallback(){
    InstantReadyBox_EnterCritical
    Callback::state.correspondingCaller = markerForUnsubscribedBox;
    Callback::state.untrackedEventsCount = 0;
    InstantReadyBox_LeaveCritical
}

inline ReadyBox<void>::Callback ReadyBox<void>::TakeCallback(){
#ifdef InstantReadyBox_NoMultithreadingProtection
    if( Callback::state.correspondingCaller != markerForUnsubscribedBox ){
        /* copy delegate to return (on-stack copy)
           Copy with markerForUnsubscribedBox will do nothing*/
        Delegate<void()> copyOfCallback{ *(Callback*)this };

        if( Callback::state.correspondingCaller != markerForUnsubscribedBox ){
            // we had something to return
            Callback::state.correspondingCaller = markerForUnsubscribedBox;
            /* we do not reset untrackedEventsCount here,
               as with existing "non markerForUnsubscribedBox" callbacks
               it is already 0 */
        }
        else{
            // callback was empty, count as untracked event
            ++Callback::state.untrackedEventsCount;
        }

        return copyOfCallback;
    }
    return Delegate<void()>{ []() {} }; // empty delegate
#else
    Delegate<void()> copyOfCallback{ []() {} }; // must be initialized

    {InstantReadyBox_EnterCritical
        if( Callback::state.correspondingCaller != markerForUnsubscribedBox ){
            if( Callback::state.correspondingCaller != markerForUnsubscribedBox ){
                // we have something to return
                copyOfCallback = *static_cast<Callback*>(this);

                Callback::state.correspondingCaller = markerForUnsubscribedBox;
                /* we do not reset untrackedEventsCount here,
                as with existing "non markerForUnsubscribedBox" callbacks
                it is already 0 */
            }
            else{
                // callback was empty, count as untracked event
                ++Callback::state.untrackedEventsCount;
            }
        }
    InstantReadyBox_LeaveCritical}

    return copyOfCallback;
#endif
}


inline void ReadyBoxTrigger<void>::operator()(){
#ifdef InstantReadyBox_NoMultithreadingProtection
    if(     Callback::state.correspondingCaller
        !=  ReadyBox<void>::markerForUnsubscribedBox
    ){
        //There a is callback ready for execution

        //separate copy to allow new subscription inside callback (handler)
        Delegate<void()> copyOfCallback{ *(Callback*)this };

        //Prepare for next round
        Callback::state.correspondingCaller = ReadyBox<void>::markerForUnsubscribedBox;
        Callback::state.untrackedEventsCount = 0;

        copyOfCallback(); //Callback can do new subscription here
    }
    else{
        ++Callback::state.untrackedEventsCount;
    }
#else
    //separate copy to allow new subscription inside callback (handler)
    Delegate< void() > copyOfCallback{ []() {} };

    {InstantReadyBox_EnterCritical
        if(     Callback::state.correspondingCaller
            !=  ReadyBox<void>::markerForUnsubscribedBox
        ){
            //There a is callback ready for execution
            copyOfCallback = *static_cast<Callback*>(this);

            //Prepare for next round
            Callback::state.correspondingCaller = markerForUnsubscribedBox;
            Callback::state.untrackedEventsCount = 0;
        }
        else{
            ++Callback::state.untrackedEventsCount;
        }
    InstantReadyBox_LeaveCritical}

    //copy is executed outside of "critical section" to avoid deadlocks
    copyOfCallback(); //Callback can do new subscription here
#endif
}

#endif
