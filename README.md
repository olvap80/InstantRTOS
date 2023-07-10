# InstantRTOS
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

