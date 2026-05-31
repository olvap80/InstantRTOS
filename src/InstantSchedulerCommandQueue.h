/** @file InstantSchedulerCommandQueue.h
 @brief Fixed-capacity command queue for cross-context Scheduler control.

Design intent:
- Scheduler internals (intrusive lists) are mutated only from owner context.
- ISR/foreign threads post lightweight commands into queue.
- Owner context drains queue and applies commands to Scheduler/ActionNode.

This keeps the original Scheduler implementation untouched while providing
safe(ish) usage pattern for mixed contexts with minimal critical sections.

MIT License

Copyright (c) 2026 Pavlo M, see https://github.com/olvap80/InstantRTOS
*/

#ifndef InstantSchedulerCommandQueue_INCLUDED_H
#define InstantSchedulerCommandQueue_INCLUDED_H

#include "InstantScheduler.h"
#include <stddef.h>


//______________________________________________________________________________
// Configurable error handling and critical section hooks

#if defined(__has_include)
#   if __has_include("InstantRTOS.Config.h")
#       include "InstantRTOS.Config.h"
#   endif
#endif

#ifndef InstantSchedulerCommandQueue_Panic
#   ifdef InstantRTOS_Panic
#       define InstantSchedulerCommandQueue_Panic() InstantRTOS_Panic('Q')
#   else
#       define InstantSchedulerCommandQueue_Panic() do{}while(true)
#   endif
#endif

#ifndef InstantSchedulerCommandQueue_EnterCritical
#   if defined(InstantScheduler_EnterCritical)
#       define InstantSchedulerCommandQueue_EnterCritical InstantScheduler_EnterCritical
#       define InstantSchedulerCommandQueue_LeaveCritical InstantScheduler_LeaveCritical
#   else
#       define InstantSchedulerCommandQueue_EnterCritical
#       define InstantSchedulerCommandQueue_LeaveCritical
#   endif
#endif


//______________________________________________________________________________
// Public API

/// Queue command describing a deferred Scheduler/ActionNode operation.
struct SchedulerCommand{
    enum Type : unsigned char{
        ScheduleAfter,
        ScheduleBefore,
        Cancel,
        ResetCallback
    };

    Type type = Cancel;
    ActionNode* action = nullptr;
    Scheduler* scheduler = nullptr;
    Scheduler::Ticks ticksToWaitFirstTime = 0;
    Scheduler::Ticks periodTicks = 0;
};


/** Single queue instance used as a bridge between contexts.
 Typical usage:
 - producer contexts call Post...
 - owner context periodically calls Drain... and then Execute...
 NOTE:  this queue is fixed-capacity and non-blocking.
        Post... returns false when queue is full.  */
template<size_t Capacity>
class SchedulerCommandQueue{
public:
    // copying banned
    constexpr SchedulerCommandQueue(const SchedulerCommandQueue&) = delete;
    SchedulerCommandQueue& operator=(const SchedulerCommandQueue&) = delete;

    SchedulerCommandQueue() = default;

    /// Number of pending commands currently buffered.
    unsigned Pending() const {
        unsigned pending = 0;
        {
            InstantSchedulerCommandQueue_EnterCritical
            pending = count;
            InstantSchedulerCommandQueue_LeaveCritical
        }
        return pending;
    }

    /// Queue capacity known at compile time.
    static constexpr size_t MaxCapacity(){ return Capacity; }

    bool PostScheduleAfter(
        ActionNode& action,
        Scheduler& targetScheduler,
        Scheduler::Ticks ticksToWaitFirstTime,
        Scheduler::Ticks periodTicks = 0
    ){
        SchedulerCommand command;
        command.type = SchedulerCommand::ScheduleAfter;
        command.action = &action;
        command.scheduler = &targetScheduler;
        command.ticksToWaitFirstTime = ticksToWaitFirstTime;
        command.periodTicks = periodTicks;
        return post(command);
    }

    bool PostScheduleBefore(
        ActionNode& action,
        Scheduler& targetScheduler,
        Scheduler::Ticks ticksToWaitFirstTime,
        Scheduler::Ticks periodTicks = 0
    ){
        SchedulerCommand command;
        command.type = SchedulerCommand::ScheduleBefore;
        command.action = &action;
        command.scheduler = &targetScheduler;
        command.ticksToWaitFirstTime = ticksToWaitFirstTime;
        command.periodTicks = periodTicks;
        return post(command);
    }

    bool PostCancel(ActionNode& action){
        SchedulerCommand command;
        command.type = SchedulerCommand::Cancel;
        command.action = &action;
        return post(command);
    }

    bool PostResetCallback(ActionNode& action){
        SchedulerCommand command;
        command.type = SchedulerCommand::ResetCallback;
        command.action = &action;
        return post(command);
    }

    /** Apply one queued command in owner context.
     Returns true if one command was consumed and applied. */
    bool DrainOne(){
        SchedulerCommand command;
        if( !pop(command) ){
            return false;
        }
        apply(command);
        return true;
    }

    /** Apply up to maxToDrain commands in owner context.
     Returns number of consumed commands. */
    unsigned DrainMany(unsigned maxToDrain){
        unsigned applied = 0;
        while( applied < maxToDrain ){
            if( !DrainOne() ){
                break;
            }
            ++applied;
        }
        return applied;
    }

    /// Apply all currently queued commands in owner context.
    unsigned DrainAll(){
        return DrainMany(~0u);
    }

private:
    SchedulerCommand ring[Capacity > 0 ? Capacity : 1];
    unsigned head = 0;
    unsigned tail = 0;
    unsigned count = 0;

    bool post(const SchedulerCommand& command){
        if( command.action == nullptr ){
            InstantSchedulerCommandQueue_Panic();
            return false;
        }
        if(
            (command.type == SchedulerCommand::ScheduleAfter
                || command.type == SchedulerCommand::ScheduleBefore)
            && command.scheduler == nullptr
        ){
            InstantSchedulerCommandQueue_Panic();
            return false;
        }

        bool posted = false;
        {
            InstantSchedulerCommandQueue_EnterCritical

            if( count < Capacity ){
                ring[tail] = command;
                ++tail;
                if( tail >= Capacity ){
                    tail = 0;
                }
                ++count;
                posted = true;
            }

            InstantSchedulerCommandQueue_LeaveCritical
        }
        return posted;
    }

    bool pop(SchedulerCommand& outCommand){
        bool hasItem = false;
        {
            InstantSchedulerCommandQueue_EnterCritical

            if( count > 0 ){
                outCommand = ring[head];
                ++head;
                if( head >= Capacity ){
                    head = 0;
                }
                --count;
                hasItem = true;
            }

            InstantSchedulerCommandQueue_LeaveCritical
        }
        return hasItem;
    }

    static void apply(const SchedulerCommand& command){
        switch( command.type ){
            case SchedulerCommand::ScheduleAfter:
                command.action->ScheduleAfter(
                    *command.scheduler,
                    command.ticksToWaitFirstTime,
                    command.periodTicks
                );
                break;

            case SchedulerCommand::ScheduleBefore:
                command.action->ScheduleBefore(
                    *command.scheduler,
                    command.ticksToWaitFirstTime,
                    command.periodTicks
                );
                break;

            case SchedulerCommand::Cancel:
                command.action->Cancel();
                break;

            case SchedulerCommand::ResetCallback:
                command.action->ResetCallback();
                break;
        }
    }
};


/** Convenience helper for owner-loop style scheduling.
 Recommended owner loop sequence:
 1) Drain queued commands from ISR/foreign contexts.
 2) Execute due actions with current time. */
template<size_t Capacity>
inline bool SchedulerOwnerStep(
    Scheduler& scheduler,
    SchedulerCommandQueue<Capacity>& commandQueue,
    Scheduler::Ticks currentTicks
){
    commandQueue.DrainAll();
    return scheduler.ExecuteAll(currentTicks);
}

#endif
