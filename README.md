# [InstantRTOS](https://github.com/olvap80/InstantRTOS) simple lightweight RTOS + utility for embedded platforms
Header-only, minimalistic real time operating system, with fast and handy utility classes, without any dependencies.
Easy to use, simple and intuitive.
Here word "Instant" stands to the ability for using InstantRTOS parts and patterns immediately (even if you have another RTOS running, you can run InstantRTOS from that RTOS))

# Features
- Written in C++ 11, suitable to work even on small embedded platforms, like Arduino (yes Arduino actually uses C++! and yes, it is possible to write RTOS in C++).
- No dependencies (even no standard headers needed) by default.
- Only standard C++ (does not depend on any platform specifics).
- Dynamic memory is not required ("heavy" new/delete, malloc/free are not required).
- Each file contains usage samples, every API is documented with doxygen.
- Easy to integrate with any platform (see samples in corresponding files!)
- You can take away parts you need (like efficient [Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) and [Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h)) and use them separately without RTOS, and even in areas not related to embedded ([Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) and [Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) will work perfectly even on desktop)).


# Frequently Asked Questions

## Why another RTOS?
This is fun)) The idea is to create a RTOS made of lightweight parts that can be easily moved around.
Also I like header-only libraries, so simple header-only RTOS is definitely my best option...


## Is InstantRTOS ready to use?
Development is still in progress but some universal parts like
[Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h), 
[Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h)
and RTOS components like
[Scheduler](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h),
simplest ["primitive" platform independent imers](https://github.com/olvap80/InstantRTOS/blob/main/InstantTimer.h)
can be immediately used independently of other parts.
Just copy the parts you need directly into your project)).


## Why not &lt;programming_langulage_name&gt; for RTOS instead of C++?
Googling in the area of embedded programming leads to answers in C and it is more easy to integrate from C++ than from any other language.

Embedded is still around C (and C++), any "exotic" languages are hard to integrate with "plain old C"))
Those "exotic" languages usually have big runtime, non deterministic garbage collection, 
or are not popular enough to have a compiler (and wrappers) for every platform (and instructions on "how to setup them" are not as easy as for "classics").  

**You can use C header files (and libraries) of your platform directly from C++ AS IS**, without all those pains of "foreign function interfaces", "marshaling", etc.!
Embedded systems, usually ship with C headers, rewriting data structures enumarations and function prototypes from them in a frapper for &lt;exotic_programming_langulage&gt; is pain!
Supporting such integration libraries to be in sync with original C code is pain
(unlsess someone else is doing that for you on regular basis, but they do not! and even then their wrappers stay mostly undocumented,
so you still have to dig original C cenrtic documentation and C code)


## Then why C++, why not C?
Think of C++ as "advanced C" for a moment))
"Plain" C requires too much boilerplate code aroud simple things. 
All those "OOP in C stuff", when every "embedded guru" invents own "the right way" and fancy conventions to do all the boilerplate are **not fun** 
(and "true man on embedded use bare C and assembler" is a myth!)
 
C++ integrates with C shoothly, but was "not welcome" by for embedded due prejudice caused by some space and time consuming language/runtime features.
But once those features are cut off, C++ is as efficient as C by code execution time and by binary size (and much easier to develop!).

The most famous embedded platform known for using C++ is Arduino, and C++ fits perfectly even into requirements for their simplest board/CPU (yep, C++ copliles even on AVR).
Remember: all the fears around C++ are well known, but they are easy to google and are easy to find solutions :)


## Why Coroutines in C++11? What about C++20 coroutines?
Because of [THIS](https://probablydance.com/2021/10/31/c-coroutines-do-not-spark-joy/),
requiring dynamic memory allocation for the thing with known (at the moment of creation) size is too drastic
for small embedded platforms.

In contrast the simplest [Coroutine in InstantRTOS](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) 
requires only sizeof(short) for holding own state and no dynamic memory allocation.

InstantRTOS implements low-memory, fast-switching statckless coroutines to do cooperative multitasking efficienly on any platform.
Coroutines are nice structural replacement for finite state machines (FSMs), since natural flow control statements are used instead of the mess of state/event transitions.


## Where is HAL? What about registers and peripherals?
The main components of InstantRTOS do not depend on any hardware/platform/CPU specifics at all, here is why:

Delegates(https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h)
and Coroutines(https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h)
are pure standard C++11, they do not depend on any platform at all.
These can be used both on embedded platforms (like Arduino) and on desktop (Windows, Linux).
Delegates(https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h)
and Coroutines(https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) in InstantRTOS are resource friendly and avoid heap usage. 

The idea of [Scheduler](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h)
and [timers](https://github.com/olvap80/InstantRTOS/blob/main/InstantTimer.h) is straight forward: just calling Sheduler from infinite loop with updated time value
(the most RTOS actually do the same, some of them provide an API to detect "idle" state, and it is still up to you to invent behavior for this)).

[Scheduler](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) takes care of accounting time, you shall only provide new time measurements!
It is up to you to choose time measurement units (seconds, milliseconds, some other hardware "ticks"),
you can even have multiple schedulers using different time units if you need.


## How to take CPU into account? 
InstantRTOS is CPU independent but you can use all your CPU fuatures with it!
You can post to queues and schedule tasks from interrupts when InstantRTOS_EnterCritical and InstantRTOS_LeaveCritical are defined.
Those macro are also applicable when two RTOSes (InstantRTOS and some other) coexist together. 
Those macro are the only CPU dependency, but you do not need them when you do not use "advanced features" described above.

## What about configuring InstantRTOS per my needs?
See TBD document for tutorial on details.

## What about saving battery and minimizing power consumption?
It looks like calling scheduler from infinite loop is not a very efficient way to save battery in any OS.
After a call to Scheduler::ExecuteOne or Scheduler::ExecuteAll it is possible to query Scheduler(https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) for the next schedule time with
```cpp
/// Obtain when next event is going to happen
/** \returns true if there is next time moment known
 *           false if there is no scheduled moment at all */
bool Scheduler::HasNextTicks(Ticks* writeTo) const;
```
and then (depending on your needs) put your device into Deep Sleep or Light Sleep until that time is reached.
Once next schedule time is known, one can apply own unique efficient strategy for power saving, depending on wait time needed.

Design note: it would be irrational to embed all the possible Deep Sleep or Light Sleep strategies into the RTOS 
(and other RTOS'es also do not)), so it is possible to use Scheduler::HasNextTicks to make your own decision. 

## Hey, why PascalCase?
To make InstantRTOS code not look like any other "coding style applicable for embedded", seriously:
- There will be no name clashes with other RTOS and their types, defines and macros!
- InstantRTOS API call insertions will be visible and distinct from other API calls...
- All those "k" prefixes and "_S" suffixes are useless!

# Features implemented so far
## General utility headers

- [InstantDelegate.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) - Fast deterministic delegates for invoking callbacks, suitable for real time operation (no heap allocation at all)

- [InstantCoroutine.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) - Simple minimalistic coroutines, suitable for all various platforms (like Arduino!) for the case when native C++ coroutines are too heavyweight (or when co_yield and stuff does not work)). Works starting from C++11 (so this can be considered as a nice coroutine implementation for Arduino, as Arduino uses C++11 by default)). NOTE: Coroutine behaves as functor and is perfectly compatible with [InstantDelegate.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) (one can resume coroutines using [delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h), also [InstantScheduler.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) can be used to schedule coroutines)

## Timing, intervals and scheduling

- [InstantScheduler.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) - The simplest possible portable scheduler suitable for embedded platforms like Arduino (actually only standard C++ is required).

- [InstantTimer.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantTimer.h) - Simple timing classes to track timings in platform independent way (this is the most "primitive" and "basic" approach, use it only when [InstantScheduler.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) does not fit due to some reason)

## Memory and queueing

- InstantMemory.h (TODO) - Simple deterministic memory management utilities suitable for real time can be used for fast and deterministic memory allocations on Arduino and similar platforms.

- InstantQueue.h (in progress) - Simple deterministic queues suitable for real time TBD.

## Other handy utility stuff

- [InstantDebounce.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDebounce.h) (in progress) - General debouncing

- InstantSignals.h (in progress) - Handle hardware signals being mapped to memory

# Future plans
TODO (still being invented)
