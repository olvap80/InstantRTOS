# [InstantRTOS](https://github.com/olvap80/InstantRTOS) simple lightweight RTOS + utility for embedded platforms
Header-only, minimalistic real time operating system, with fast and handy utility classes, without any dependencies.
Easy to use, simple and intuitive.
Here word "Instant" stands to the ability for using InstantRTOS parts and patterns immediately (even if you have another RTOS running, you can run InstantRTOS from that RTOS))

# Benefits (and goals)
- RTOS is written in C++ 11 (yes, it is possible to write RTOS in C++!), with type safety and all the compile time stuff (constexpr), lambda and fast efficient delegates, RAII (constructors and destructors).
- Suitable to work even on small embedded platforms, like Arduino (yes Arduino actually uses C++!).
- No dependencies (even no standard headers needed) by default.
- Only standard C++ (does not depend on any platform specifics).
- Dynamic memory is not required ("heavy" new/delete, malloc/free are not required).
- Each file contains usage samples, every API is documented with doxygen.
- Easy to integrate with any platform (see samples in corresponding files!)
- You can take away parts you need (like efficient [Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) and [Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h)) and use them separately without RTOS, and even in areas not related to embedded ([Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) and [Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) will work perfectly even on desktop)).

Jump to [Frequently Asked Questions](https://github.com/olvap80/InstantRTOS/blob/main/README.md#frequently-asked-questions) to see the motivation behind design decisions for InstantRTOS.


# Features implemented so far
## General utility headers

- [InstantDelegate.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) - Fast deterministic delegates for invoking callbacks, suitable for real time operation (no heap allocation at all).
The approach used by [InstantDelegate.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) is absolutely platform independent, only standard C++ is required, this implements fast delegate system for functions, functors and member function pointers, more lightweight then std::function, suitable for embedded platforms.

- [InstantCoroutine.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) - Simple minimalistic coroutines, suitable for all various platforms (like Arduino!) for the case when native C++ coroutines are too heavyweight (or when co_yield and stuff does not work)). Works starting from C++11 (so this can be considered as a nice coroutine implementation for Arduino, as Arduino uses C++11 by default)). NOTE: Coroutine behaves as functor and is perfectly compatible with [InstantDelegate.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h) (one can resume coroutines using [delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h), also [InstantScheduler.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) can be used to schedule coroutines)

## Timing, intervals and scheduling

- [InstantScheduler.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) - The simplest possible portable scheduler suitable for embedded platforms like Arduino (actually only standard C++ is required).

- [InstantTimer.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantTimer.h) - Simple timing classes to track timings in platform independent way (this is the most "primitive" and "basic" approach, use it only when [InstantScheduler.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) does not fit due to some reason)

## Memory and queueing

- [InstantMemory.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantMemory.h) - Simple deterministic memory management utilities (block pools, lifetime management, TBD) suitable for real time, can be used for fast and deterministic memory allocations on Arduino and similar platforms.

- InstantQueue.h (in progress) - Simple deterministic queues suitable for real time TBD.

## Other handy utility stuff

- [InstantDebounce.h](https://github.com/olvap80/InstantRTOS/blob/main/InstantDebounce.h) (in progress) - General debouncing

- InstantSignals.h (in progress) - Handle hardware signals being mapped to memory


# Frequently Asked Questions

## Why another RTOS?
This is fun)) The idea is to create a RTOS in C++ being made of lightweight parts that can be easily moved around.
Also I like header-only libraries, so simple header-only RTOS is definitely my best option...


## Is InstantRTOS ready to use?
(For the list of ready stuff see [below](#features-implemented-so-far))

Development is still in progress but some universal parts like
[Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h), 
[Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h)
and RTOS components like
[Scheduler](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h),
simplest ["primitive" platform independent imers](https://github.com/olvap80/InstantRTOS/blob/main/InstantTimer.h)
can be immediately used independently of other parts.
Just copy the parts you need directly into your project)).


## Why not &lt;programming_langulage_name&gt; for RTOS instead of C++?
Googling in the area of embedded programming leads to answers in C and it is more easy to integrate from C++, than from any other language.

Embedded is still around C (and C++), any "exotic" languages are hard to integrate with "plain old C"))
Those "exotic" languages usually have big runtime, non deterministic garbage collection, 
or are not popular enough to have a compiler (and wrappers) for every platform (and instructions on "how to setup them" are not as easy as for "classics").  

**You can use C header files (and libraries) of your platform directly from C++ AS IS**, without all those pains of "foreign function interfaces", "marshaling", etc.!
Embedded systems, usually ship with C headers, rewriting data structures enumerations and function prototypes from them in a frapper for &lt;exotic_programming_langulage&gt; is pain!
Supporting such integration libraries to be in sync with original C code is pain
(unless someone else is doing that for you on regular basis, but they do not! and even then their wrappers stay mostly undocumented,
so you still have to dig original C centric documentation and C code)


## Then why C++, why not C?
Think of C++ as "advanced C" for a moment)) RAII is the must-have feature!
"Plain" C requires too much boilerplate code around simple things. 
All those "OOP in C stuff", when every "embedded guru" invents own "the right way" and fancy conventions to do all the boilerplate are **not fun** 
(and "true man on embedded use bare C and assembler" is a myth!)
 
C++ integrates with C smoothly, but was "not welcome" by for embedded due prejudice caused by some space and time consuming language/runtime features.
But once those features are cut off, C++ is as efficient as C by code execution time and by binary size (and much easier to develop!).

The most famous embedded platform known for using C++ is Arduino, and C++ fits perfectly even into requirements for their simplest board/CPU (yep, C++ compiles even on AVR).
Remember: all the fears around C++ are well known, but they are easy to google and are easy to find solutions :)


## Why Coroutines in C++11? What about C++20 coroutines?
Because of [THIS](https://probablydance.com/2021/10/31/c-coroutines-do-not-spark-joy/),
requiring dynamic memory allocation for the thing with known (at the moment of creation) size is too drastic
for small embedded platforms.

In contrast the simplest [Coroutine in InstantRTOS](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) 
requires only sizeof(short) for holding its own state and no dynamic memory allocation.

InstantRTOS implements low-memory, fast-switching stackless coroutines to do cooperative multitasking efficiently on any platform.
Coroutines are nice structural replacement for finite state machines (FSMs), since natural flow control statements are used instead of the mess of state/event transitions.


## What about preemption of tasks?
Multiple "threads of execution" that "run simultaneously and share resources" is the feature leading us to mutexes and semaphores, CAS and memory barriers, (and race conditions, deadlocks, priority inversion)...
this complicates programming a lot (and hurts CPU performance!) 
Despite the fact that RTOS do their best to help you dealing with all that stuff, there is a synchronization overhead due to shared resources.
Also, usually there are more "threads of execution" then there physical CPUs available,
but context switches for preempting of some “thread” by “some other thread” on the same CPU are bad (more time to switch, misses in CPU cache, space needed for multiple thread stacks and storing CPU contexts).
The exact way of doing CPU context switching between threads is not portable between different platforms at all, each kind of CPU has own fancy way to do this!

So regarding preemption: "not yet, but some day in the future as an additional module supporting some platforms"...
Let's stick with Coroutines for now))

## But what about cases when preemption is really needed?
That is why "not yet" instead of "never"))

The main question here is **when that preemption is really needed?** 

"Using preemption is cool"... but that is why all those "Active Objects, communicating with each other by asynchronous message passing" and "run to completion" were invented: to overcome synchronization troubles caused by preemption on "cool projects"! "Active Objects" are kind of *workaround*, where queues are used *instead* of "traditional" resource protecting mechanisms...

But look: once "each Active Object handles one event at a time" and "runs to completion" this looks like... single threaded execution on single CPU!
With single physical CPU (like AVR on Arduino) our "Active Object tasks" definitely do not run "in parallel" (they have to "switch" from one to another, either preemptively or cooperatively).
Thus doing "true" preemption of multiple tasks with CPU context switching leads to... CPU time and memory space wasted!

According the to above, **using of non-preemptive scheduling will avoid the overhead of synchronization needed to protect shared resources!**
Nice and suitable option for embedded devices is to avoid synchronization complexity and preemption overheads by using cooperative multitasking to have smaller memory requirements and less CPU load!

The only **real case when task preemption is really needed**, is to let "_the more important task to interrupt the less important task_",
and this leads us to the next section about priorities.


## Then what about task priorities?
Priorities are needed to make "more critical tasks" able "to execute in time" regardless of what "less critical task" is doing "right now"!
Here "regardless of what they do" means even doing ```for(;;){}``` by "less critical task" shall be preempted (when needed) using CPU context switch.
And "more critical" usually means "a shorter cycle duration results in a higher job priority"
as suggested by the Rate-monotonic scheduling algorithm.

Naturally in the real case there are shared resources, pending queues (with multiple items waiting in them), etc.
and even those (beloved by "cool projects" and CS academists) "Active Objects" with their queues of pending messages are not "periodic tasks with unique periods" as it is required by the mathematically proven Rate-monotonic scheduling approach!
They are also not a collection of independent jobs as it is required by Earliest deadline first scheduling!...
According to the above the perfect mathematically proven optimal scheduling theorems do not work for the real case!

Naturally for the real systems those "Rate-monotonic scheduling" or "Earliest deadline first scheduling" approaches are still considered at design stage,
as _**the only way** to "reason theoretically" about establishing a rational schedule approach_.
This "theoretical approach" is used together with practical extensive use of watchdogs/timeouts/asserts and followed by extensive testing to detect all the stuff that does not fit with "ideal theory"!
Extensive testing also helps to detect those "banned, but still possible" issues with ```for(;;){}``` (usually such "unwanted" loops do not look that simple!).
NOTE: ```for(;;){}``` and long computation is still considered "bad", even for preemptive systems, and watchdogs/timeouts are used to ensure such computations do not cause the system to miss critical deadlines!
"High priority long computation" is a problem, because it delays lower priority tasks and "low priority long computation" is still a problem when it is done under critical section (priority inversion) or when high priority task waits for lower priority "Active Object" to process the request (obvious design error but still "accidentally" possible in complex systems).

Now let's look at **cooperative multitasking**, when each task voluntarily gives up control.
Here the word "voluntarily" **also means** "*task suspends while waiting for something to happen*", and, (not) surprisingly, it is a **natural and desired condition** for any embedded application to make all tasks (even those, that are preemptive) "waiting for something to happen" instead of continuously running))! Now we can consider there are only two states for the cooperative task (coroutine): resumed (executing/running) and suspended (voluntarily gives up control/waits).

For cooperative multitasking we can treat any execution between "resume" and "suspend" as "mutex locking everything" or "global task lock".
Once the cooperative task (coroutine) is resumed, we can consider it owns that "global task lock", and once it "yields to scheduler" we can consider it as releasing this "global task lock".
The shortest is the time coroutine is being executed (in resumed state), the less time that "global task lock" is "locked" and the more it looks like... preemptive system...
And then all the approaches used for preemptive systems can also apply: "theoretical approach" (RMS or EDF) and "practical approach" (timeouts, watchdogs and extensive testing))!

Naturally, doing ```for(;;){}``` will stall entire cooperative system, but for preemptive system doing ```for(;;){}``` under mutex has similar drastic effects to tasks messing with that mutex.
So approach with preemption allows other "not involved" tasks to "still do something", bit one still needs to detect "wrong thing is going to happen" condition with timeouts, watchdogs and extensive testing in both approaches.
On the other hand cooperative multitasking has obvious advantages: under our imaginary "global task lock" we do not have to use any mutexes at all (and we are not wasting memory and CPU time for those mutexes!), we own that imaginary "global task lock" by default once we are executing our code, there are no "deadlocks" between cooperative tasks, no need to use "manual" mutexes to protect resources being shared between cooperative tasks, no need to use "atomic operations", CAS and "memory barriers", etc.

Natural approach for our cooperative system is to use scheduling based on the "Earliest deadline first"
to resume "the most urgent task earlier", this looks like "using dynamic priorities" but there are no "task priorities" at all (you can easily introduce some prioritization, see below!)
we just resume tasks in their "straightforward order")).
In our case we will simplify approach from the above even more: cooperative tasks (coroutines) are _resumed in the order as they were scheduled and run "till the next yield"_.
This is "a little bit different" than "task shall be completed till this deadline time", but still enough "theoretical ground" to start with, before proceeding _in practice_ with timeouts, watchdogs, asserts and extensive testing)).
(I know perfect CS academist will cry bloody tears while seeing those "simplifications", but such similar "simplifications" is what real life RTOS/embedded system developers actually do, 
... and that is why timeouts/watchdogs/asserts/extensive are *always* needed to refine theoretically built constructions
... and that is why "it is a **natural and desired condition** for any embedded application to make all tasks (even those, that are preemptive) "waiting for something to happen" instead of continuously running"!!))

## More important and less important tasks (priorities for cooperative world)
TBD on cooperative multitasking, watchdogs, timeouts *planning and multiple schedulers* and on workarounds!

TODO: problem "too much time for sequential execution of every coroutine for some more important tasks" (do we really need multiple schedulers? maybe one is enough just because tasks insers self to it on the right place?)
"short enough resume" vs "tasks in time" and "task importance" 
tasks are enqueued, main difference from EDF!
(those tasks that have more important deadlines vs those scheduled for "specific time", execution loops)


howto with multiple schedulers
design note: maybe one "default global sheduler"?

## Coroutine vs FSM

TODO: active object and switch(message) criticism, state machine criticism, state machines vs flowcharts (state switching the same as goto, etc)

## Where is HAL? What about registers and peripherals?
The main components of InstantRTOS do not depend on any hardware/platform/CPU specifics at all, here is why:

[Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h)
and [Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h)
are pure standard C++11, they do not depend on any platform at all.
These can be used both on embedded platforms (like Arduino) and on desktop (Windows, Linux).
[Delegates](https://github.com/olvap80/InstantRTOS/blob/main/InstantDelegate.h)
and [Coroutines](https://github.com/olvap80/InstantRTOS/blob/main/InstantCoroutine.h) in InstantRTOS are resource friendly and avoid heap usage. 

The idea of [Scheduler](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h)
and [timers](https://github.com/olvap80/InstantRTOS/blob/main/InstantTimer.h) is straight forward: just calling Sheduler from infinite loop with updated time value
(the most RTOS actually do the same, some of them provide an API to detect "idle" state, and it is still up to you to invent behavior for this)).

[Scheduler](https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) takes care of accounting time, you shall only provide new time measurements!
It is up to you to choose time measurement units (seconds, milliseconds, some other hardware "ticks"),
you can even have multiple schedulers using different time units if you need.


## How to take CPU into account? 
InstantRTOS is CPU independent but you can use all your CPU features with it!
You can post to queues and schedule tasks from interrupts when InstantRTOS_EnterCritical and InstantRTOS_LeaveCritical are defined.
Those macros are also applicable when two RTOSes (InstantRTOS and some other) coexist together. 
Those macros are the only CPU dependency, but you do not need them when you do not use "advanced features" described above.

## What about configuring InstantRTOS per my needs?
See TBD document for tutorial on details.

## What about saving battery and minimizing power consumption?
It looks like calling a scheduler from an infinite loop is not a very efficient way to save battery in any OS.
After a call to Scheduler::ExecuteOne or Scheduler::ExecuteAll it is possible to query Scheduler(https://github.com/olvap80/InstantRTOS/blob/main/InstantScheduler.h) for the next schedule time with
```cpp
/// Obtain when next event is going to happen
/** \returns true if there is next time moment known
 *           false if there is no scheduled moment at all */
bool Scheduler::HasNextTicks(Ticks* writeTo) const;
```
and then (depending on your needs) put your device into Deep Sleep or Light Sleep until that time is reached.
Once next schedule time is known, one can apply own unique efficient strategy for power saving, depending on the wait time needed.

Design note: it would be irrational to embed all the possible Deep Sleep or Light Sleep strategies into the RTOS 
(and other RTOSes also do not)), so it was mede possible to use Scheduler::HasNextTicks to make your own decision. 

## Hey, why PascalCase?
To make InstantRTOS code not look like any other "coding style applicable for embedded", seriously:
- There will be no name clashes with other RTOS and their types, defines and macros!
- InstantRTOS API call insertions will be visible and distinct from other API calls...
- All those "k" prefixes and "_S" suffixes are useless! C++ compilers (and IDE) are smart enough to not call enum instead of function or assign structure to int))


# Future plans
All existing parts are designed to have a stable interface.
It is intended that future changes will not break any existing functionality.

Plans so far
- Complete queues and their variations
- Add preemption for some platforms (and stuff around)
- ...
- TODO (still being invented)
