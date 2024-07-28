/** @file InstantTask.h
 @brief Asynchronous Cooperative Tasks based on stackless Coroutines for C++11
        No dependencies from outside of InstantRTOS, just copy the headers!
        There is no "build step" needed, everything fits into headers!

(c) see https://github.com/olvap80/InstantRTOS

Tasks are "special coroutines" targeting cooperative multithreading 
and asynchronous execution, please see samples below for illustration.
Initially created as a lightweight multitasking framework for Arduino.

 @code

 @endcode

Please note that no "scheduler", no "event loop" etc are needed,
unless you intentionally need/wish to use them, for example
one can (but is not obligated to) use InstantScheduler.h 
TODO: also notes and sample with InstantScheduler

NOTE: On why "native C++20 coroutines" are not suitable for embedded, see:
      https://probablydance.com/2021/10/31/c-coroutines-do-not-spark-joy/
      (remember InstantTask works in C++11 which is default for Arduino))

The main difference from InstantCoroutine.h is that Cooperative Tasks
from InstantTask.h return "thenable" once resumed and their "yielded result"
does NOT go to the caller of the "resume" as "return value"
(as "simple" Coroutines do in InstantCoroutine.h), 
INSTEAD the "yielded value" is passed to callback via "thenable".
This allows handy tricks with asynchronous code and "awaits" leading to
natural and linear cooperative control flow, and predictable behavior.
Please see below for details:

- Coroutine from InstantCoroutine.h resumes when called with (), 
    and each such call via () ALWAYS returns the next value coroutine "yields"
    (such coroutine executes only in the context of the resumer).
- Task is a coroutine that "yields" value(s) to a callback, and can
    "suspend" self while "awaiting" to other ("Thenable") thing 
    (such "other Thenable being awaited" is not necessary to be a coroutine!).
    Resuming Task does not mean "yielded value" goes to the resumer,
    once call completes. Instead yielded value goes to the callback provided.
    Thus resume operation returns "Thenable" to be used for receiving
    "yielded value" (result).

Coroutines and Cooperative Tasks are controlled in different ways:
- Resuming of a Coroutine from InstantCoroutine.h is intended to be done
    from "outside", when "external code" drives that coroutine 
    by resuming it "explicitly", "manually" (so Coroutine acts more like 
    iterator/generator, and using it "like a thread" is only an option).
    Coroutine also can "schedule self" from "inside" for the future resume
    but this is not a convenient way for those coroutines returning values. 
- Tasks here are intended to be used as lightweight "non-preemptive threads"
    Cooperative Task is able to schedule self for future as it is intended
    by internal logic (Cooperative Tasks act more like "threads", and all the
    design of Cooperative Tasks is to help using them for such purpose).
    Yielded result is provided to Task's "Then(...)" callback.
    "Await" operation for the task can be seen as "schedule me in the future"!

Cooperative Task has much more instruments to control how is can be
suspended and scheduled:
- Coroutine from InstantCoroutine.h suspend themselves only via "yield"
    operation. This means those coroutines, that yield values (those that
    are not "void coroutineResultType" have to yield something on each suspend
    operation, and cannot pause themselves to "wait for something" without
    "yielding" some value, even if "there is nothing to yield so far")
- Cooperative Tasks can suspend themselves both on "yield" and "await"
    operations. When Cooperative Task "awaits" for something,
    it does not have to always produce some value to caller.
    This flexibility comes with the fact Cooperative Task
    always returns "Thenable" once resumed, and all yields
    go to the callback previously provided to "Then(...)".
    Yield means
        "send yielded value to the callback previously set by the resumer"
        (once yield happened, "someone else" has to explicitly resume that task,
         and task will continue execution from the context of resumer)
    Await means
        "continue running me once value (or completion) arrives (resolves)"
        (once await starts control returns to the resumer,
         with "thenable" as a return value, future yielded output will go
         to that thenable;
         once awaited value/completion arrives/resolves,
         the Cooperative Task continues from the context of the "resolver")

Coroutines from the InstantCoroutine.h and Cooperative Tasks here 
are different by the way parameters are passed:
- Coroutine from InstantCoroutine.h has both state fields and 
    optional resume parameters (those specified in CoroutineBegin and
    then passed to () on each resume call)
- Cooperative Task has only state, but () resume call does not receive
    any parameters (you have to manually update state when some
    "extra" parameters are needed)


NOTE: InstantTask.h is configurable for interrupt (thread) safety.
      It is always safe to use the same object from the same thread.
      (different objects used from different threads will work as well).
      It is safe to use the same object from different threads/interrupts
      only if that interrupt (thread) safety is configured, see below



# DESIGN NOTES (JUST SKIP THAT if not interested in internal details)):

There is a general problem: starting "something" that leads to resolving of
"thenable" BEFORE "previous" "something else" resolves that same "thenable"
(resolving "thenable" twice when waited only once)!
The "right solution" is to use thenable "only once" (to start different
"something" leading to "resolved thenable" with "different thenable"),
but this works nice only with dynamic memory allocation (and gc is nice).
(TODO: how this problem of "resuming already resumed stuff" also apply to C++ coroutines?)
In our case with our Task we have "the same Task" with "the same thenable",
this means we can "resume it while it is already resumed" or resume it while
it is "awaiting for something else" (resume Task before yield from Task).
General solution for this is something like "typestate" in language, on resume
"type changes", the only C++ mitigation available for this problem so far
is to force user for mandatory calling of *.Then on resuming Task by using
[[nodiscard]] attribute for resuming API
(thus it is not possible to ignore result of resume).

Scenarios considered:

- Scenario_1. Task T1 is resumed by the "resumer" R0 and immediately "Yields" 
    to own "then callback" (without returning to "resumer" R0),
    now "then callback" executes in T1 before "return to R0",
    this can lead to recursion (!) if "then callback" resumes the same task T1
    or if "then callback" resumes some other task T2 and T2 then resumes
    the same task T1 via Scenario_1.
    Solution: track ProtectFromRecursion (we are in then callback) state and  
    check from nested recursive call to return thenable through callback
    instead of resuming. The ResumedByImmediateCallback state will be processed
    by T1 on return from callback.
    (this works because "resume" for Task does not mean "run immediately",
    it means "ensure task resumes, and provide Then operation, to be
    resolved once Task yields")

- Scenario_2. Task T1 is resumed by R0 and then "Awaits" for T2, this means,
    control returns to resumer R0, but R0 cannot resume T1 again until
    "then callback" is issued (trying to resume will clash with pending
    "await" being done from the Task T1 "awaiting for T2").
    All "Tasks" resumed shall be immediately "awaited", all future
    resumes can happen only after "then callback" (after "await"!)
    Mitigation is [[nodiscard]] to force R0 use returned Thenable for
    continuing execution!
    (Is is assumed the next attempt to resume will happen from "then callback"
     attached to [[nodiscard]] Thenable, once task T1 yields,
     and since we go "await" here, then Scenario_1 is not applicable)
    
- Scenario_3. "Awaiting" for value (or for completion!), here we following two parts:
    Scenario_3a. Value arrives from "someone else" to "resuming then callback"
        later - not a problem, because T1 already returned to R0 (no recursion),
        and "awaited" value exists for sure in the call stack of the caller
        at the moment "resuming then callback" happens!
    Scenario_3b. Value to process (!) arrives to "then callback" immediately
        from the API of "call to be awaited"
        (we did not exit current operator() yet!),
        this means T1 is "resumed" recursively,
        so one shall process "arrived variable" in recursive call, 
        but then return back here to prevent recursion!
        (Note:  case of continuing/resuming from other thread 
                the such case not considered a recursion,
                but we must be aware of that case)
        ALLOW ONLY ASSIGN TO FIELD IN RECURSION, then follow Scenario_1!
    Scenario_3c. Completion arrives, no value is provided, handle as Scenario_1!




Portable and lightweight stackless cooperative tasks fpr C++11 and above 
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

#ifndef InstantTask_INCLUDED_H
#define InstantTask_INCLUDED_H

//Tasks are based on the same idea as coroutines, and reuses some suff
#include "InstantCoroutine.h"
//Thenable is used to "yield" value (or completion) from the task
#include "InstantThenable.h"
//Delegate is needed to create compact lambda for "await" 
#include "InstantDelegate.h"


//______________________________________________________________________________
// Configurable error handling and interrupt safety

/* Common configuration to be included only if available
   (you can separate file and/or configure individually
    or just skip that to stick with defaults) */
#if defined(__has_include) && __has_include("InstantRTOS.Config.h")
#   include "InstantRTOS.Config.h"
#endif


#ifndef InstantTask_Panic
#   ifdef InstantRTOS_Panic
#       define InstantTask_Panic() InstantRTOS_Panic('T')  
#   else
#       define InstantTask_Panic() /* you can customize here! */ do{}while(true)
#   endif
#endif

#ifndef InstantTask_EnterCritical
#   if defined(InstantRTOS_EnterCritical) && !defined(InstantTask_SuppressEnterCritical)
#       define InstantTask_EnterCritical InstantRTOS_EnterCritical
#       define InstantTask_LeaveCritical InstantRTOS_LeaveCritical
#       if defined(InstantRTOS_MutexObjectType)
#           define InstantTask_MutexObjectType InstantRTOS_MutexObjectType
#           define InstantTask_MutexObjectVariable InstantRTOS_MutexObjectVariable
#       endif
#   else
#       define InstantTask_EnterCritical
#       define InstantTask_LeaveCritical
#       define InstantTask_MutexObject
#   endif
#endif


//______________________________________________________________________________
// Handle C++ versions (just skip to "Classes for handling tasks" below))

#if defined(__cplusplus)
#   if __cplusplus >= 201703L
#       if __cplusplus >= 202002L
#           define TaskNodiscard(explainWhy) [[nodiscard(explainWhy)]]
#       else
#           define TaskNodiscard(explainWhy) [[nodiscard]]
#       endif
#   else
#       ifdef __GNUC__
#           define TaskNodiscard(explainWhy) __attribute__((warn_unused_result))
#       elif defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#           if _MSVC_LANG >= 202002L
#               define TaskNodiscard(explainWhy) [[nodiscard(explainWhy)]]
#           else
#               define TaskNodiscard(explainWhy) [[nodiscard]]
#           endif
#       endif
#   endif
#endif

#if !defined(TaskNodiscard)
#       define TaskNodiscard(explainWhy)
#endif



//______________________________________________________________________________
// Task public API 

/// Define a class for Cooperative Task (can be instantiated multiple times))
/** Cooperative Task setup shall follow in {}, see sample above.
 * Use TaskBegin(), TaskEnd() inside those {} to delimit task body (task code).
 * Use TaskYield(...) to yield to callback provided to our *.Then 
 * Use TaskAwaitCompletion(...), TaskAwaitField(...),
 * inside the task body (task code) to await for other "Thenable"
 * Has also two methods (inherited from CoroutineBase):
 *  -   Finished() returns true if coroutine is finished, false otherwise 
 *  -   operator bool() returns true if coroutine is still not finished
 * The operator() is created by nesting TaskBegin macro call, see below
 * NOTE: one can place fields (and constructors!) inside {}, see sample above
 *       this is the best approach to have additional state for Task
 * NOTE: all fields are private by default!
 * REMEMBER: return cannot be used inside Cooperative Task body,
 *           (use TaskYield or TaskStop instead of return)!
 *           also be aware that break can be used only in nested loop!
 * NOTE: Cooperative Tasks work perfectly with InstantScheduler.h ))
 *       (but one can use own approach for scheduling,
 *        use callbacks for schedule, etc) */
#define TaskDefine(CooperativeTaskName) \
    class CooperativeTaskName: \
        public CoroutineBase /* every task is also a Coroutine! */


/// Cooperative Task body code follows this macro (initially suspended!)
/** Macro adds operator() that resumes the Task and returns "our thenable", 
 * to be used in "await" operations by the resumer (by "them", on resume
 * one provides callback to Then(...) for receiving "completion").
 * It is not specified when exactly callback provided to Then(...) is called:
 * immediately after operator() or "some time later". One must ensure Task
 * instance is alive as long as callback provided to Then(...) is issued.
 * When callback resumes the same Task - recursion is automatically prevented
 * (this is known as "Symmetric transfer")
 * If "someone else" resumes again Task already running - "Panic" is issued.
 * REMEMBER: Setup Then(...) callback to find the moment when task yields,
 *           or use "Await" macros inside other task to be sure resume
 *           happens only in "right moments" (to not resume 
 *           Task doing "Await" on something else)
 * REMEMBER: corresponding TaskEnd() macro must mark the end of the task body!
 * REMEMBER: return cannot be used inside Cooperative Task body,
 *           (use TaskYield or TaskStop instead of return)!
 *           also be aware that break can be used only in nested loop!
 * NOTE: do not declare local variables inside Cooperative Task body
 *       (actually do not declare local variable crossing TaskYield!)
 *       use LifetimeManager and LifetimeManagerScope from InstantMemory.h
 *       once RAII like lifetime scoping is needed */
#define TaskBegin(TypeYieldedByTask) \
    public: \
        /* Returns "Thenable" part so resume call can be "awaited", see below */ \
        TaskNodiscard("Every resume operation shall be Then-ed, do not resume before task yields") \
        Thenable<TypeYieldedByTask>& operator()(){ return Internal_ResumeTask(); } \
        \
        /* Start task and ignore output (cannot obtain ) */ \
        /* One shall not try to resume such task again */ \
        void StartAndExplicitlyIgnore(){ Internal_ResumeTask(); } \
        \
    TASK_INTERNAL_BEGIN(TypeYieldedByTask)


/// Yield via own Thenable (Task is also Thenable)
/** Remember state and suspend execution
 *  (the next call to operator() will resume from the same place)
 *  REMEMBER: one cannot place TaskYield inside switch statement
 *  NOTE: one can specify return values for resumable cooperative tasks,
 *        so yielding value will go to "Then(...)" callback */
#define TaskYield(...) \
    TASK_INTERNAL_YIELD_FROM( \
        (CppCoroutine_State::Initial + (COROUTINE_PLACE_COUNTER - CppCoroutineState_COUNTER_START)), \
        __VA_ARGS__ );


/// Await for thenable event (thenable without parameters) to happen
/** One awaits for thenable to resolve, just pass thenable expression as macro parameter
    @code

    @endcode
    REMEMBER: all code below is "continued" after thenable is resolved,
              but Task returns immediately to the resumer,
              this resumer cannot resume the same Task again!
              That caller shall use .Then(...) API to detect the moment when 
              current task yields (if call caller is also a task it can also 
              use "await" but not on out thenable) */
#define TaskAwaitForCompletion(...) \
    do{ \
        using DelegateType = Delegate<void()>; \
        using ThisClassType = InstantTaskDetails::RemoveReference<decltype(*this)>::type; \
        using ConvertToSimpleSignatureType = \
            InstantTaskDetails::ConvertToSimpleSignature<ThisClassType>; \
        TASK_INTERNAL_AWAIT( \
            (CppCoroutine_State::Initial + (COROUTINE_PLACE_COUNTER - CppCoroutineState_COUNTER_START)), \
            DelegateType::From(this).Bind<&ConvertToSimpleSignatureType::apply>(), \
            __VA_ARGS__); \
    } while (false)

/// Await for result from Thenable and write it to Task field
/** One awaits for thenable to resolve,
    just pass field name and thenable expression as macro parameter
    @code

    @endcode
    REMEMBER: all code below is "continued" after thenable is resolved,
              but Task returns immediately to the resumer,
              this resumer cannot resume the same Task again!
              That caller shall use .Then(...) API to detect the moment when 
              current task yields (if call caller is also a task it can also 
              use "await" but not on out thenable) */
#define TaskAwaitToField(fieldName, /*awaitedExpression*/ ...) \
    do{ \
        /* NOTE: field type must match exactly result of "their" thenable */ \
        using FieldType = decltype(this->fieldName); \
        using DelegateType = Delegate<void(const FieldType& arrivedValue)>; \
        using ThisClassType = InstantTaskDetails::RemoveReference<decltype(*this)>::type; \
        using TheirResultToFieldAndResumeType = \
            InstantTaskDetails::TheirResultToFieldAndResume< \
                ThisClassType, FieldType, &ThisClassType::fieldName \
            >; \
        TASK_INTERNAL_AWAIT( \
            (CppCoroutine_State::Initial + (COROUTINE_PLACE_COUNTER - CppCoroutineState_COUNTER_START)), \
            DelegateType::From(this).Bind<&TheirResultToFieldAndResumeType::apply>(), \
            __VA_ARGS__); \
    } while (false)

///Special case that inverts callback to "awaitable"  
#define TaskAwaitForResume(codeThatSchedulesResume) \
    TODO

///TODO
#define TaskAwaitForCallback(callbackSetup, callbackSignature, /*callback code*/...) \


/// The "the last return" from the Cooperative Task
/** Once stopped it will be not resumable again.
 * "Panic" is issued when trying to resume stopped Cooperative Tasks.
 * Use LifetimeManager from InstantMemory.h 
 * if you wish to have "restartable" Cooperative Tasks */
#define TaskStop(theLastRVal) \
    do { \
        cppCoroutine_State.current = CppCoroutine_State::Final; \
        /* TODO: how to the last yield here */; \
    } while (false)

/// End of the Cooperative Task body (corresponds to TaskBegin() from the above)
/** Remember, one shall not reach TaskEnd(),
 * use TaskStop(...) mark Cooperative Task as "finished forever" */
#define TaskEnd() \
                default: InstantTask_Panic(); \
            } \
        } \
    private:


//TODO:  and cppfork/microfork/forkpp/cppgo and keep captured stuff
//       to run new "thread" inline with variables captured ()



//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################



//______________________________________________________________________________
// (needs to be declared in advance, just skip this section))


/// Hold execution state for Cooperative Task (internal stuff, just skip it)
class CppTask_HandleRecursionBase{
public:
    /// Test if Task can execute/continue execution
    /** Called by operator() to ensure one can continue current call,
     * and update state according to decision being made,
     * see Scenario_1 and Scenario_3b above 
     * (Note: Scenario_2 is covered by [[nodiscard]], and is not related here) */
    bool CppTask_HandleRecursion_CanExecuteHere(){
        bool callerShallContinueExecution = true; // the most expected case!
        
        /* test in interrupt safe environment
           since Task execution can continue in some other thread */
        {InstantTask_EnterCritical
            if( CppTask_AdditionalState::ReadyToResume != cppTask_AdditionalState ){
                // Resuming/continuing a task that does "something else"
                
                if( CppTask_AdditionalState::ProtectFromRecursion == cppTask_AdditionalState ){
                    /* This task was already running and now yields (or awaits!)
                    We are resuming from "then callback" called recursively! */
                    cppTask_AdditionalState = CppTask_AdditionalState::ResumedByImmediateCallback;
                    /* One shall return to callers to prevent recursion
                    (Direct caller shall use Then(...) to wait for continuing)
                    In this way cooperative tasks can resume each other without recursion! */
                    callerShallContinueExecution = false;
                }
                else{
                    /* Resuming a task that is already running, definitely an error!
                       Please ensure you subscribe to .Then callback or "await" it
                       before resuming your tasks!
                       (one shall not resume manually again before Thenable resolves) */
                    InstantTask_Panic();
                }
            }
            else{
                //sign that this Task now executes normally
                cppTask_AdditionalState = CppTask_AdditionalState::Busy;
                //and leave callerShallContinueExecution as true
            }
        InstantTask_LeaveCritical}

        return callerShallContinueExecution;
    }

    /// Helper API to be called from TaskAwaitToValue macro
    template<class OtherThenableT, class FunctorT>
    bool CppTask_AwaitValue_Suspends(
        FunctorT&& function,
        OtherThenableT&& otherThenable
    ){
        CppTask_EnterOperation_ProtectFromRecursion();

        // attach to "their" thenable (this can execute right now!)
        otherThenable.Then(function);

        //we are on embedded platform, let's assume there are no exceptions allowed

        return CppTask_LeaveOperation_DoesTaskStaySuspended();
    }

protected:
    CppTask_HandleRecursionBase() = default;
    
    /// Mark this task as going to call something leading to potential recursive call
    /** Called indirectly by operator() via CppTask_Yield_IssueThenCallback_Suspends
     * to start area where potential recursion can happen due to issued code,
     * works in pair with CppTask_LeaveOperation_DoesTaskStaySuspended(),
     * see Scenario_1 above for context*/
    void CppTask_EnterOperation_ProtectFromRecursion(){
        InstantTask_EnterCritical
            // Sign we are going to some callback (like "then callback")
            cppTask_AdditionalState = CppTask_AdditionalState::ProtectFromRecursion;
        InstantTask_LeaveCritical
    }

    /// Determine task status once callback operation is done
    /** Counterpart for CppTask_EnterOperation_ProtectFromRecursion 
     * to be called indirectly by operator(),
     * @returns true when operator() of Cooperative Task shall "suspend"
     *          (suspending here means coroutine returns to caller),
     *          false when operator() shall continue execution */
    bool CppTask_LeaveOperation_DoesTaskStaySuspended(){
        bool taskStaysSuspended = true; //the most probable result

        {InstantTask_EnterCritical
            // Check we were not resumed again in that "then callback"
            if( CppTask_AdditionalState::ResumedByImmediateCallback != cppTask_AdditionalState ){
                cppTask_AdditionalState = CppTask_AdditionalState::ReadyToResume;
                /* caller macro shall "suspend", to wait for "yet other resumer"
                   (leave taskStaysSuspended as true while ReadyToResume,
                    thus both yield and await return to resumer,
                    and resumer shall subscribe to returned Thenable),
                    
                    in case of "await" this is also ReadyToResume internal state,
                    this means "resume while await" (unless prevented by [[nodiscard]])
                    will not be detected immediately, but let's stay simple,
                    to allow the same operator() be reused by "Await" macro)) */
            }
            else{
                //means ResumedByImmediateCallback happened

                //we continue running since "then callback" resumed us again
                cppTask_AdditionalState = CppTask_AdditionalState::Busy;
                
                //caller macro shall continue execution without suspending
                taskStaysSuspended = false;
            }
        InstantTask_LeaveCritical}
        
        return taskStaysSuspended;
    }


    /// Keep track of nesting calls and prevent recursion (allow symmetric )
    enum class CppTask_AdditionalState: unsigned char{
        ReadyToResume, ///< Ready to be resumed by outside world by calling operator()
        Busy,  ///< Doing the work (executes operator() right now)
        ProtectFromRecursion, ///< Sign we are doing potential resuming callback
                              ///< from the operator(), still Busy, call in progress
        ResumedByImmediateCallback ///< Sign we are resuming right now
    } cppTask_AdditionalState = CppTask_AdditionalState::ReadyToResume;
};


/// Base for all "awaitable" ("thenable") tasks
template<class TypeYieldedByTask>
class TaskAdditionalState: public CppTask_HandleRecursionBase{
public:
    /* Remember, we hide "operator()" from Thenable 
     (it is intended for call only by this task in yield)
     SomeTask::operator()" is different and means "resume task",
     please also note that one can access "out " */
    ThenableToResolve<TypeYieldedByTask> CppTask_thenable;

    /// Helper API to be called from TaskYield macro to issue "then callback"
    bool CppTask_Yield_IssueThenCallback_Suspends(const TypeYieldedByTask& result){
        CppTask_EnterOperation_ProtectFromRecursion();

        /* "Their" "then callback" is executed by "our Task" here!
        This is "thread/interrupt safe", and so "no other guards are needed".
        That "external" "then callback" can resume our Task immediately
        here leading to recursion (reentering the same "our Task" again),
        in this case "our Task" entry point will detect recursive resume
        by checking against ProtectFromRecursion in "our" operator(),
        then it will flag back with ResumedByImmediateCallback and return,
        thus caller of CppTask_Yield_IssueThenCallback_Suspends ("our Task")
        will continue execution without entering recursion */
        
        CppTask_thenable(result); // here we call Thenable<TypeYieldedByTask>::operator()

        //we are on embedded platform, let's assume there are no exceptions allowed

        return CppTask_LeaveOperation_DoesTaskStaySuspended();
    }
};

/// Base for all "awaitable" ("thenable") tasks (specialization for void)
template<>
class TaskAdditionalState<void>: public CppTask_HandleRecursionBase{
public:
    /* Remember, we hide "operator()" from Thenable 
     (it is intended for call only by this task in yield)
     SomeTask::operator()" is different and means "resume task",
     please also note that one can access "out " */
    ThenableToResolve<void> CppTask_thenable;

    /// Helper API to be called from TaskYield macro to issue "then callback"
    bool CppTask_Yield_IssueThenCallback_Suspends(){
        CppTask_EnterOperation_ProtectFromRecursion();

        /* "Their" "then callback" is executed by "our Task" here!
           This is "thread/interrupt safe", and so no guards are needed.
           That "external" "then callback" can resume our Task recursively
           after that "our Task" will detect recursive resume 
           by checking against ProtectFromRecursion,
           then it will flag back with ResumedByImmediateCallback and return */
        CppTask_thenable();

        //we are on embedded platform, let's assume there are no exceptions allowed

        return CppTask_LeaveOperation_DoesTaskStaySuspended();
    }
};


namespace InstantTaskDetails{
    //keep the promise to not use standard headers))
    template<class T> struct RemoveReference { typedef T type; };
    template<class T> struct RemoveReference<T&> { typedef T type; };
    template<class T> struct RemoveReference<T&&> { typedef T type; };

    ///Generate "" API to be called by "their" Thenable
    template<class TaskClass>
    struct ConvertToSimpleSignature{
        /// API to write class member   
        static void apply(TaskClass& self){
            //resume coroutine from await, let it decide what happens next
            self.Internal_ResumeTask_AfterAwait();
        }
    };

    ///Generate API to be called by "their" Thenable for storing result to field
    template<class TaskClass, class ExpectedResultType, ExpectedResultType TaskClass::*field>
    struct TheirResultToFieldAndResume{
        /// API to write class member and resume Task
        static void apply(TaskClass& self, const ExpectedResultType& theirResult){
            (self.*field) = theirResult;
            //resume coroutine from await, let it decide what happens next
            self.Internal_ResumeTask_AfterAwait();
        }
    };
}

#define TASK_INTERNAL_BEGIN(TypeYieldedByTask) \
        /* NOTE: more checks are possible to track and assert right usage, */ \
        /*       but skip for now in favour of simplicity and compactness */ \
        void Internal_ResumeTask_AfterAwait(){ Internal_ResumeTask(); } \
    private: \
        TaskAdditionalState<TypeYieldedByTask> cppTask_State; \
        \
        /* One shall not try to resume such task again */ \
        Thenable<TypeYieldedByTask>& Internal_ResumeTask(){ \
            static constexpr CppCoroutine_State::Holder \
                CppCoroutineState_COUNTER_START = COROUTINE_PLACE_COUNTER; \
            \
            /* Handle possible recursion first */ \
            /* See Scenario_1 and Scenario_3b from above */ \
            /* NOTE: here ReadyToResume is also checked and Busy is set! */ \
            /*       (for the simplicity both resume and await go this way) */ \
            if( !cppTask_State.CppTask_HandleRecursion_CanExecuteHere() ){ \
                /* Return through all callers back to the same task (this task!) */ \
                /* Resuming callers shall use .Then to catch moment when task yields */ \
                return cppTask_State.CppTask_thenable; \
            } \
            \
            /* Will jump to the previous saved position (or start from Initial) */\
            switch( cppCoroutine_State.current ){ \
                /* The very first time coroutine/task runs */\
                case CppCoroutine_State::Initial:;

///Helper macro to form state saving/resume point for yield operation
#define TASK_INTERNAL_YIELD_FROM(cppCoroutine_place_id, /*yielded return expression*/ ...) \
    do{ \
        /* Split state saving and suspend intentionally to allow */ \
        /* resume from the right point when CppTask_Yield_IssueThenCallback_Suspends */ \
        /* decides what to do (resume can happen even from interrupt in some configurations!).*/ \
        /* Remember state to resume for the same cppCoroutine_place_id below */ \
        COROUTINE_REMEMBER_STATE(cppCoroutine_place_id) \
        /* Call "our" "then callback" that "someone else" (resumer) */\
        /* uses to "await" for "yield" from this task */\
        /* See Scenario_1 above for example */ \
        if( cppTask_State.CppTask_Yield_IssueThenCallback_Suspends(__VA_ARGS__) ){ \
            /* That "then callback" did not resume this task, wait for resume by someone else! */\
            /* Even if "someone else" resumes this coroutine, this has no effect here */ \
            /* as that "someone else" will resume from the point of "remember" */ \
            /* (this means from the point BELOW!) */ \
            COROUTINE_INTERNAL_SUSPEND(cppCoroutine_place_id, cppTask_State.CppTask_thenable) \
        } \
        /* else means: continue execution of this task as it was resumed by "then callback" */\
        /*             saved state is not used, next yield will just rewrite it */ \
    } while (false)


///Helper macro to form state saving/resume point
#define TASK_INTERNAL_AWAIT(\
            cppCoroutine_place_id, function,  /*awaitedExpression*/ ...) \
    do{ \
        /* Split state saving and suspend intentionally to allow */ \
        /* resume from the right point when CppTask_AwaitValue_Suspends */ \
        /* decides what to do (resume can happen even from interrupt in some configurations!).*/ \
        /* Remember state to resume for the same cppCoroutine_place_id below */ \
        COROUTINE_REMEMBER_STATE(cppCoroutine_place_id) \
        \
        /* Tie to "their thenable" in awaitedExpression !!!*/\
        if( this->cppTask_State.CppTask_AwaitValue_Suspends(function, __VA_ARGS__) ){ \
            /* That "then callback" did not resume this task, wait for resume by someone else! */\
            /* Even if "someone else" resumes this coroutine, this has no effect here */ \
            /* as that "someone else" will resume from the point of "remember" */ \
            /* (this means from the point BELOW!) */ \
            COROUTINE_INTERNAL_SUSPEND(cppCoroutine_place_id, this->cppTask_State.CppTask_thenable) \
        } \
        /* else means: continue execution of this task as if it was resumed by "their thenable" */\
        /*             saved state is not used, next yield will rewrite it */ \
    } while (false)

//TODO: move implementations here

#endif
