# [InstantRTOS](https://github.com/olvap80/InstantRTOS)
Header-only, mimimalistic real time OS and handy utilities without dependencies.
Here "Instant" stands to the ability to use InstantRTOS parts and patterns immediately (even if you have another RTOS runnung, you can run InstantRTOS from that RTOS))

# Features
- Written in C++ 11, suitable to work even on small embedded platforms, like Arduino (yes Arduino actually uses C++! and yes, it it possible to write RTOS in C++)
- No dependencies (even no standard headers needed) by default
- Only standard C++ (does not depend on any platform specifics)
- Each file contains usage sample, every API is documented with doxygen
- Easy to integrate with any platform (see samples in corresponding files!)

# Frequently Asked Questions

## Why another RTOS?
This is fun)) The idea is to create RTOS made of ligtweight parts, that can be easy moved around.

## Why C++?
Because "true man on embedded use bare C and assembler" is a myth!

C requires too much boilerplate code aroud, 
C++ is unfameous for embedded because of some space and time consuming language/runtime features,
But once those features are cut off it is as efficient as C.

The most fameous platform known for using C++ is Arduino, and C++ fits perfectly even into their simplest board/CPU requirements.

## Is it ready to use?

Development is still in progress but some parts like Delegates, Coroutines, Scheduler can be used independently of other parts.
Just copy the parts you need directly into your project.

## Where is HAL? What about registers and periperals?

TODO

## What about battery

TODO

# Features
## General utility headers

- [InstantDelegate.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) - Fast deterministic delegates for invoking callbacks, suitable for real time operation (no heap allocation at all)

- [InstantCoroutine.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) - Simple minimalistic coroutines suitable for all various platforms (like Arduino!) for the case when native C++ coroutines are too heavyweight (or when co_yield and stuff does not work)).

## Timing, intervals and scheduling

- [InstantTimer.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantTimer.h) - Simple timing classes to track timings in platform independent way.

- [InstantScheduler.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) - The simplest possible portable scheduler suitable for embedded platforms like Arduino (actually only standard C++ is required).

## Memory and queueing

- InstantMemory.h (TODO) - Simple deterministic memory management utilities suitable for real time can be used for fast memory allocations on Arduino and similar platform.

- InstantQueue.h (in progress) - Simple deterministic queues suitable for real time can be used for dynamic memory allocations on Arduino and similar platforms.

## Other handy utility stuff

- [InstantDebounce.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDebounce.h) (in progress) - General debouncing

- InstantSignals.h (in progress) - Handle hardware signals being mapped to memory
