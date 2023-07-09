/** @file InstantScheduler.h
    @brief The simplest possible portable scheduler suitable for embedded 
           platforms like Arduino (actually only standard C++ is required).
           Hardware independent, clear algorithm, easy to integrate!

    See sample below for details.
    @code
    
    @endcode

    NOTE: you run that Scheduler manually, there can be as many schedulers as
          you like and each scheduler instance can have own units and timing!

    Additional sample of scheduling with coroutines:

    On choosing time unit and precision please see 
    https://forum.arduino.cc/t/how-fast-can-i-interrupt/25884
    and https://forum.arduino.cc/t/high-precision-timing-feasibility/177907
    and https://forum.arduino.cc/t/micros-vs-milis-accuracy-of-interrupt-measurements/288768

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

#ifndef InstantScheduler_INCLUDED_H
#define InstantScheduler_INCLUDED_H

#include "InstantDelegate.h"
#include "InstantIntrusiveList.h"

/*
    TODO: futures/promises? JS then?
            as callbacks?
*/

//______________________________________________________________________________
// Individual configuration (just leave default when works as expected))

/* Uncomment below to collect basic scheduler usage statistics
   (one can use that statistics to ensure schedule loop performs
    according to expected timings)) */
#define InstantScheduler_StatisticsCollection
// Uncomment below to allow floating average
#define InstantScheduler_StatisticsAverageCount 1000 


#ifndef InstantScheduler_Ticks_Type
    ///Type to be used for storing time measurements and time calculations
    /** This shall be the type returned by your time measurement API.
     * It is assumed that time grows continuously with unsigned overflow.
     * It is assumed that arithmetic is unsigned (two's complement)
     * https://stackoverflow.com/a/18195756/4336953 */ 
#   define InstantScheduler_Ticks_Type unsigned long
#endif

//This will be used in the future
#ifndef InstantScheduler_EnterCritical
#   ifdef InstantRTOS_EnterCritical
#       define InstantScheduler_EnterCritical InstantRTOS_EnterCritical
#       define InstantScheduler_LeaveCritical InstantRTOS_LeaveCritical
#   else
#       define InstantScheduler_EnterCritical
#       define InstantScheduler_LeaveCritical
#   endif
#endif


//______________________________________________________________________________
// The ActionNode - universal "schedulable thing"


// Forward declare items there ActionNode can be "scheduled" into
class Scheduler;
class MulticastToActions;


///Uniform action, simple "schedulable thing" for adding into scheduling chains
/** Set your callback/delegate to be scheduled here, and then
 * use corresponding methods to schedule/subscribe as needed.
 * The intention here is to reuse the same ActionNode for multiple purposes,
 * and to have all the API available directly.
 * REMEMBER: scheduled/subscribed ActionNode takes care to unschedule self from
 *           any previous Scheduler/MulticastToActions ... instance.
 *           (there is only one schedule/subscription at a time!) */
class ActionNode: private IntrusiveList<ActionNode>::Node{
public:
    //all the copying is banned (this ensures pointers are valid)
    constexpr ActionNode(const ActionNode&) = delete;
    ActionNode& operator =(const ActionNode&) = delete;


    //__________________________________________________________________________
    // Attaching actual callback/delegate

    /// Type of the callback being used by the scheduled items
    using Callback = EventCallback; //same as Delegate< void() >;


    /// Empty (do nothing) callback (use Set to assign callback later)
    ActionNode() = default;

    /// Wrap specified callback 
    ActionNode(const Callback& eventCallback);

    /// Set new callback to invoke once action is executed by the Scheduler
    /** Callback will execute only once Scheduler executes the callback called,
     *  previous schedules have no effect */
    ActionNode& Set(const Callback& eventCallback);

    /// Setup new callback to execute on (and after) schedule call
    /** Callback will execute immediately if ActionNode was previously called,
     *  and only one time even if there were multiple previous calls.
     *  After that such callback will execute on each schedule */
    ActionNode& Then(const Callback& eventCallback);


    //__________________________________________________________________________
    // Define timing units to be used for scheduling
    // (just leave default if works as expected))

    /// The time measurement unit for scheduling
    /** Can be anything you like, milliseconds, microseconds, seconds, 
     * 
     * This type is used for cyclic time measurement with overflowing counter
     * Infinitely cycling (continuously growing with overflow)
     * It is assumed that arithmetic is unsigned (two's complement)
     * https://stackoverflow.com/a/18195756/4336953 */
    using Ticks = InstantScheduler_Ticks_Type;

    /// The maximum ticks amount Scheduler is able to wait
    /** The maximum valid difference for comparison operations 
     * Time differences that go above then DeltaMax
     * cannot be compared with TicksIsLess API */
    static constexpr Ticks DeltaMax = ~Ticks(0) / 2;

    /// Provide < (less) operation for Ticks within limited range
    /** All intervals within DeltaMax are "ordered" and comparable.
     *  Possible overflow is already taken into account.
     * \returns true if operand1 goes before operand2
     *          (crossing unsigned ) */
    static bool TicksIsLess(const Ticks& op1, const Ticks& op2);


    //__________________________________________________________________________
    // Scheduling using time (meaning of the units is up to the scheduler)  
    // REMEMBER: Actually Scheduler does not measure time,
    //           user has to provide measurements!

    /// Schedule for execution "in next iteration"
    /** Actually synonym to ScheduleAfter(targetScheduler, 1)
     * Current ExecuteAll will NOT see that scheduled item,
     * because it is "in the future" from current time! */
    void ScheduleLater(Scheduler& targetScheduler);

    /// Schedule for execution "in same iteration"
    /** Actually synonym to ScheduleAfter(targetScheduler, 0)
     * Current ExecuteAll will see scheduled item,
     * because it has the same time as current! 
     * Q: can continuous ScheduleNow cause other actions to delay forever?
     * A: For ExecuteAll - YES, it will never end as time is the same, but
     *    for ExecuteOne - NOT if each ExecuteOne in
     *    next schedule will execute with new time, and so following
     *    ScheduleNow will delay further then current actions.
     *    To mitigate more, use several Scheduler instances 
     *    to call ExecuteAll for those time critical
     *    stuff that has to be executed in time,
     *    between ExecuteOne of those less critical items */ 
    void ScheduleNow(Scheduler& targetScheduler);


    ///Schedule for execution after all items of the same time
    /** Always removes ActionNode from current scheduler (if any).
     * Adds after all items with the same ticks in targetScheduler,
     * the ticksToWaitFirstTime is counted from current time of the target!
     * NOTE: remember tick units are "as in that scheduler"!*/
    void ScheduleAfter(
        Scheduler& targetScheduler,
        Ticks ticksToWaitFirstTime,
        Ticks periodTicks = 0
    );

    ///Schedule for execution later
    /** Adds before all items with the same ticks,
     * NOTE: remember tick units are "as in that scheduler"! */
    void ScheduleBefore(
        Scheduler& targetScheduler,
        Ticks ticksToWaitFirstTime,
        Ticks periodTicks = 0
    );

    /// Returns true if ActionNode is scheduled with Scheduler for execution
    bool IsScheduled() const;

    /// Absolute schedule time of next (pending) schedule
    /** Obtains value in Ticks units of corresponding scheduler
     * Valid only if IsScheduled gives true. */
    Ticks AbsoluteScheduleTime() const;

    /// Period to reschedule again after first schedule
    /** Obtains value in Ticks units of corresponding scheduler
     * Valid only if IsScheduled gives true.
     * Value of 0 means "non periodic" */
    Ticks PeriodTicksAgain() const;


    //__________________________________________________________________________
    // Listening to some other event/call source 

    /// Listen to MulticastToActions calls (removed on first call)
    /** Multiple ActionNode can be listening to the same MulticastToActions
     * simultaneously, and can be executed once that MulticastToActions
     * is called. Those, that were added with ListenOnce will be removed on call */
    void ListenOnce(MulticastToActions& multicastToAction);

    /// Listen to MulticastToActions calls (stay listening after call)
    /** Multiple ActionNode can be listening to the same MulticastToActions
     * simultaneously, and can be executed once that MulticastToActions
     * is called. Those, that were added with ListenSubscribe
     * stay listen as long as are not removed manually by Cancel */
    void ListenSubscribe(MulticastToActions& multicastToAction);

    /// Test ActionNode is listening to MulticastToActions instance
    /** Verify ActionNode is listening to MulticastToActions instance */
    bool IsListening() const;


    //__________________________________________________________________________
    // General scheduling/listening control

    /// Remove from the corresponding Scheduler/MulticastToActions
    /** Call to Cancel will prevent invocation from corresponding 
     * Scheduler/MulticastToActions */
    void Cancel();


    /// Reset to corresponding callback to initial state (will silently count again)
    /** Callback being set by Set(...) or Then(...) API is cleared,
     * you need to use Set(...) or Then(...) to keep track of the callback again */
    void ResetCallback();

    /// Delegate what will call EventSlot and Reset it ("unsubscribe")
    /** Use this API to create "single shot" subscriptions for this EventSlot,
     * such delegate will call EventSlot and ResetCallback(), 
     * so that EventSlot::Then() has to be called again
     * NOTE: REMEMBER TO OVERWRITE CallAndResetCallback EXISTING SUBSCRIPTION
     *       ON EventSlot Reset TO PREVENT "message from the past" */
    Callback MakeUnsubscribingCallback();

private:
    // Node must see ActionNode as derived from self
    friend class IntrusiveList<ActionNode>::Node;
    friend class IntrusiveList<ActionNode>;

    // Scheduler needs to run ActionNode instances
    friend class Scheduler;
    // MulticastToActions needs to run ActionNode instances
    friend class MulticastToActions;

    /// Code to be called once schedule time comes (does nothing by default)
    EventSlot eventSlot;

    ///Q: may be reversing methods will make scheduledWith unneeded? 
    //turn ActionNode::Cancel(); to Scheduler::Cancel(ActionNode?)
    ///A: no scheduledWith must be still present to check if scheduler OWNS ActionNode
    /// "Parent" scheduler to use for item item removal
    /** All methods are aware where action was scheduled previous time,
     * so rescheduling is handled automatically */
    Scheduler* scheduledWith = nullptr;

    union{
        // Used by Scheduler
        struct{
            /// Absolute schedule time as it is tracked by the corresponding scheduler  
            Ticks absoluteScheduleTime;
            /// Period to reschedule again after first schedule
            Ticks periodTicksAgain;
        } scheduleData;

        //special case for 
        bool multicastToActionsRemoveAfterCall;
    };


    /// Common action being performed when scheduling
    void prepareForNewSchedule(
        Scheduler& targetScheduler,
        Ticks ticksToWaitFirstTime,
        Ticks periodTicks
    );
    
    /// Place item to the right location in scheduler's queue
    void scheduleAfterFindPlace();

    /// Common setup for ListenOnce and ListenSubscribe
    void listenTo(MulticastToActions& multicastToAction, bool removeAfterCall);
};


//______________________________________________________________________________
// Classes used for scheduling/subscribing 


/// The simplest possible Scheduler for arranging actions in time
/** You shall invoke ExecuteAll or ExecuteOne frequent enough to
 * have desired precision.
 * Use HasNextTicks to find the time of next schedule! */
class Scheduler{
public:
    //all the copying is banned (this ensures pointers are valid)
    constexpr Scheduler(const Scheduler&) = delete;
    Scheduler& operator =(const Scheduler&) = delete;

    using Ticks = ActionNode::Ticks;

    /// Create initial empty Scheduler
    constexpr Scheduler() = default;

    /// Prepare initial time (so that all time intervals will start from it)
    /** This is the very first API to make schedule running!
     * All other API shall be called after this one,
     * TIME MUST BE KNOWN FIRST OF USAGE! */
    void Start(Ticks currentTicks);


    /// TODO: what about time going faster than tasks are really scheduled?

    /// Execute single pending item
    /** @return true if some item was executed */
    bool ExecuteOne(
        Ticks currentTicks ///< Current ticks that overflow
    );

    /// Execute all items that are pending so far
    /** @return true if at least one item was executed */
    bool ExecuteAll(
        Ticks currentTicks ///< Current ticks that overflow
    );
    

    /// Obtain when next event is going to happen
    /** \returns true if there are  */
    bool HasNextTicks(Ticks* writeTo) const;

#   ifdef InstantScheduler_StatisticsCollection
        /* Remember: scheduler does not measure time, instead
                    it only uses provided time to collect measurements
                    that is why set of API is limited */

        /// Worst case Delay between ExecuteOne API calls
        Ticks StatisticsDelayBetweenExecuteOneMax() const;

        /// Worst case Delay between ExecuteAll API calls
        Ticks StatisticsDelayBetweenExecuteAllMax() const;

#       ifdef InstantScheduler_StatisticsAverageCount
            /// Average case between ExecuteOne API calls 
            /** Delay of 0 is ignored here! 
             *  So ExecuteOne in single ExecuteAll are not counted here,
             *  Use StatisticsDelayBetweenExecuteAllAvg instead */
            Ticks StatisticsDelayBetweenExecuteOneAvg() const;

            /// Average case between ExecuteAll API calls 
            Ticks StatisticsDelayBetweenExecuteAllAvg() const;
#       endif

#   endif

private:
    /* ActionNode must be aware about corresponding owning Scheduler
    is will not be possible to transition between different Schedulers
    with correct item removal, etc */
    friend class ActionNode;

    /// Current absolute ticks as they arrived with Execute* API
    Ticks knownAbsoluteTicks = 0;

    /// The list of all items scheduled so far
    IntrusiveList<ActionNode> scheduledActions;

#   ifdef InstantScheduler_StatisticsCollection
        class MeasurementMonitor{
        public:
            /// Call this once measurement arrives
            void OnMeasurement(Ticks currentMeasurement);

            /// Obtain maximum known so far
            Ticks Max() const;

#           ifdef InstantScheduler_StatisticsAverageCount
                /// Obtain average known so far
                Ticks Average() const;
#           endif
        private:
            Ticks maxKnownValue = 0;

#           ifdef InstantScheduler_StatisticsAverageCount
                Ticks numMeasurements = 0; 
                Ticks accumulatedSoFar = 0;
#           endif
        };

        Ticks previousExecuteAllKnownAbsoluteTicks = 0;

        MeasurementMonitor statisticsDelayBetweenExecuteOne;
        MeasurementMonitor statisticsDelayBetweenExecuteAll;
#   endif
};


/// Serve as "multicast" collection of actions (translate one call to multiple)
class MulticastToActions{
public:
    /// Empty multicast
    MulticastToActions() = default;

    /// Execute all actions collected so far
    /** Those items added with ActionNode::ListenOnce are removed after execute,
     *  items added with ActionNode::ListenSubscribe stay for future calls 
     *  NOTE: one shall not call this from interrupt,
     *        this call is not reenterable! */
    void operator()();

private:
    // ActionNode places self to MulticastToActions instance
    friend class ActionNode;
    
    /// All actions to execute with operator()
    /** As long as one item is filled, the other one can be executed */
    IntrusiveList<ActionNode> actionsToExecute[2];

    /// Serve as index for actionsToExecute (trick to allow adding while executing)
    bool useFirst = false;
};


//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################



inline ActionNode::ActionNode(const Callback& eventCallback)
    : eventSlot(eventCallback) {}


inline ActionNode& ActionNode::Set(const Callback& eventCallback){
    eventSlot.Set(eventCallback);
    return *this;
}

inline ActionNode& ActionNode::Then(const Callback& eventCallback){
    eventSlot.Then(eventCallback);
    return *this;
}


inline bool ActionNode::TicksIsLess(const Ticks& op1, const Ticks& op2){
    /* Use the fact of unsigned integer two's complement arithmetic
    "wraps around", and so operand1 smaller then operand2
    will produce "very big value" )) */  
    return (op1 - op2) > DeltaMax;
}


inline void ActionNode::ScheduleLater(Scheduler& targetScheduler){
    ScheduleAfter(targetScheduler, 1);
}

inline void ActionNode::ScheduleNow(Scheduler& targetScheduler){
    ScheduleAfter(targetScheduler, 0);
}


inline void ActionNode::ScheduleAfter(
    Scheduler& targetScheduler,
    Ticks ticksToWaitFirstTime,
    Ticks periodTicks
){
    InstantScheduler_EnterCritical
    
    prepareForNewSchedule(
        targetScheduler,
        ticksToWaitFirstTime,
        periodTicks
    );

    scheduleAfterFindPlace();

    InstantScheduler_LeaveCritical
}

inline void ActionNode::ScheduleBefore(
    Scheduler& targetScheduler,
    Ticks ticksToWaitFirstTime,
    Ticks periodTicks
){
    InstantScheduler_EnterCritical

    prepareForNewSchedule(
        targetScheduler,
        ticksToWaitFirstTime,
        periodTicks
    );

    auto itr = targetScheduler.scheduledActions.begin();
    
    for( ; itr != targetScheduler.scheduledActions.end() ; ++itr){
        // if( itr->absoluteScheduleTime >= absoluteScheduleTime)
        if(
            !TicksIsLess(
                itr->scheduleData.absoluteScheduleTime,
                scheduleData.absoluteScheduleTime
            )
        ){
            /* We have found element with the same or greater time
                then this so we insert this just before that */
            break;
        }
    }

    //element found or end is reached - operation is the same:
    itr->InsertPrevChainElement(this);

    InstantScheduler_LeaveCritical
}


inline bool ActionNode::IsScheduled() const {
    return scheduledWith != nullptr;
}


inline ActionNode::Ticks ActionNode::AbsoluteScheduleTime() const{
    return scheduleData.absoluteScheduleTime;
}

inline ActionNode::Ticks ActionNode::PeriodTicksAgain() const{
    return scheduleData.periodTicksAgain;
}


void ActionNode::ListenOnce(MulticastToActions& multicastToAction){
    listenTo(multicastToAction, true);
}

void ActionNode::ListenSubscribe(MulticastToActions& multicastToAction){
    listenTo(multicastToAction, false);
}

void ActionNode::listenTo(
    MulticastToActions& multicastToAction,
    bool removeAfterCall
){
    InstantScheduler_EnterCritical

    scheduledWith = nullptr;
    multicastToAction.actionsToExecute[multicastToAction.useFirst].InsertAtBack(this);

    multicastToActionsRemoveAfterCall = removeAfterCall;

    InstantScheduler_LeaveCritical
}


bool ActionNode::IsListening() const{
    /* IsScheduled means we are not "listening" for sure,
       IsChainElementSingle also means we are not "listening" */ 
    return !(IsScheduled() || IsChainElementSingle());
}


inline void ActionNode::Cancel(){
    InstantScheduler_EnterCritical

    if( scheduledWith ){
        /* Remember: we have to remove from that chain manually
            and no custom/scheduling code shall run here,
            This is also the sign we wre not scheduled any more */ 
        RemoveFromChain();

        /* Item can cancel self while being processed,
            so we shall prevent own periodTicksAgain to reschedule it again */ 
        scheduleData.periodTicksAgain = 0;

        scheduledWith = nullptr;
    }

    InstantScheduler_LeaveCritical
}


inline void ActionNode::ResetCallback(){
    eventSlot.ResetCallback();
}


inline ActionNode::Callback ActionNode::MakeUnsubscribingCallback(){
    return eventSlot.MakeUnsubscribingCallback();
}


inline void ActionNode::prepareForNewSchedule(
    Scheduler& targetScheduler,
    Ticks ticksToWaitFirstTime,
    Ticks periodTicks
){
    /* Previous scheduler shall not work with this item any more
        remember: chain item is still not yet (re)moved here */

    //mark this as being scheduled with that new scheduler
    scheduledWith = &targetScheduler;
    scheduleData.absoluteScheduleTime = targetScheduler.knownAbsoluteTicks + ticksToWaitFirstTime;
    scheduleData.periodTicksAgain = periodTicks;
}

inline void ActionNode::scheduleAfterFindPlace(){
    auto itr = scheduledWith->scheduledActions.begin();
    
    for( ; itr != scheduledWith->scheduledActions.end() ; ++itr){
        // if( itr->absoluteScheduleTime > absoluteScheduleTime)
        if(
            TicksIsLess(
                scheduleData.absoluteScheduleTime,
                itr->scheduleData.absoluteScheduleTime
            )
        ){
            /* We have found element with the time GREATER then this
                so we insert this just before that! */
            break;
        }
    }

    //element found or end is reached - operation is the same:
    itr->InsertPrevChainElement(this);
}



inline void Scheduler::Start(Ticks currentTicks){
    knownAbsoluteTicks = currentTicks;

#   ifdef InstantScheduler_StatisticsCollection
        previousExecuteAllKnownAbsoluteTicks = currentTicks;
#   endif
}

inline bool Scheduler::ExecuteOne(
    Ticks currentTicks ///< Current ticks that overflow
){
    ///ActionNode we execute right now (if any)
    ActionNode* actionBeingExecutedNow = nullptr;
    {
        InstantScheduler_EnterCritical

#   ifdef InstantScheduler_StatisticsCollection
        statisticsDelayBetweenExecuteOne.OnMeasurement(currentTicks - knownAbsoluteTicks);
#   endif

        // executed action (if any) can schedule using new time 
        knownAbsoluteTicks = currentTicks;

        // always execute starting from list head
        auto actionToExecute = scheduledActions.begin();
        if(
                actionToExecute != scheduledActions.end()
                /* test the time for actionToExecute has come 
                (item time <= current time) same as !(current time < item time)
                REMEMBER: this works only if time difference is less then
                ActionNode::DeltaMax (with regard to overflow),
                so one shall call to Execute* API more frequently then
                Ticks difference of ActionNode::DeltaMax */
            &&  !ActionNode::TicksIsLess(
                    currentTicks,
                    actionToExecute->scheduleData.absoluteScheduleTime
                )
        ){
            //Currently running action
            actionBeingExecutedNow = actionToExecute.operator->();

            /* Do not use Cancel here to allow catching new period if any
               Thus just fix part of the values */
            actionBeingExecutedNow->RemoveFromChain();

            /* the actionBeingExecutedNow->scheduledWith = nullptr; 
               will happen later and only
               in the case if item is not scheduled to somewhere else */
        }

        InstantScheduler_LeaveCritical
    }

    //test we can execute current action
    if( actionBeingExecutedNow ){
        /*  Note: periodic item can cancel self here
                    (then periodTicksAgain turns 0) */
        actionBeingExecutedNow->eventSlot();

        {
            InstantScheduler_EnterCritical

            /* ensure item did not add self to somewhere else,
               (no scheduling or listening to something)
                in this case periodTicksAgain does not apply */
            if( actionBeingExecutedNow->IsChainElementSingle() ){
                //and there are periodic ticks 
                if( actionBeingExecutedNow->scheduleData.periodTicksAgain ){
                    // Determine the next time according to period
                    actionBeingExecutedNow->scheduleData.absoluteScheduleTime =
                        knownAbsoluteTicks + actionBeingExecutedNow->scheduleData.periodTicksAgain;

                    // Place item to the right location in scheduler's queue
                    actionBeingExecutedNow->scheduleAfterFindPlace();
                }
                else{
                    // means item already removed, complete with 
                    actionBeingExecutedNow->scheduledWith = nullptr;
                } 
            }
            //else means scheduledWith already points to somewhere else!

            InstantScheduler_LeaveCritical
        }

        return true; //item was executed

    }
    //else means there is no item to execute

    return false; //item was not executed
}

/// Execute all items that are pending so far
/** @return true if at least one item was executed */
inline bool Scheduler::ExecuteAll(
    Ticks currentTicks ///< Current ticks that overflow
){
#   ifdef InstantScheduler_StatisticsCollection
        statisticsDelayBetweenExecuteAll.OnMeasurement(currentTicks - previousExecuteAllKnownAbsoluteTicks);
        previousExecuteAllKnownAbsoluteTicks = currentTicks;
#   endif

    bool atLeastOneItemWasExecuted = false;
    while( ExecuteOne(currentTicks) ){
        atLeastOneItemWasExecuted = true;
    }
    return atLeastOneItemWasExecuted;
}
    

inline bool Scheduler::HasNextTicks(Ticks* writeTo) const{
    bool hasTicks = true;
    {
        InstantScheduler_EnterCritical
        
        // the first one is the nearest
        auto actionToExecute = scheduledActions.begin();
        if( actionToExecute != scheduledActions.end() ){
            *writeTo = actionToExecute->scheduleData.absoluteScheduleTime;
        }
        else{
            hasTicks = false;
        }
        InstantScheduler_LeaveCritical
    }
    return hasTicks;
}


#ifdef InstantScheduler_StatisticsCollection
    inline Scheduler::Ticks Scheduler::StatisticsDelayBetweenExecuteOneMax() const{
        return statisticsDelayBetweenExecuteOne.Max();
    }

    inline Scheduler::Ticks Scheduler::StatisticsDelayBetweenExecuteAllMax() const{
        return statisticsDelayBetweenExecuteAll.Max();
    }


    inline void Scheduler::MeasurementMonitor::OnMeasurement(Ticks currentMeasurement){
        if( currentMeasurement > maxKnownValue ){
            maxKnownValue = currentMeasurement;
        }
#       ifdef InstantScheduler_StatisticsAverageCount
            if( numMeasurements >= InstantScheduler_StatisticsAverageCount ){
                //enough measurements were accumulated
                accumulatedSoFar -= Average();
                /* Note: old values beyond InstantScheduler_StatisticsCount
                        gradually loose their weight in resulting average */
            }
            else{
                ++numMeasurements;
            }
            accumulatedSoFar += currentMeasurement;
#       endif
    }

#   ifdef InstantScheduler_StatisticsAverageCount
        inline Scheduler::Ticks Scheduler::MeasurementMonitor::Average() const{
            if( numMeasurements ){
                return accumulatedSoFar / numMeasurements;
            }
            return 0;
        }

        inline Scheduler::Ticks Scheduler::StatisticsDelayBetweenExecuteOneAvg() const{
            return statisticsDelayBetweenExecuteOne.Average();
        }

        inline Scheduler::Ticks Scheduler::StatisticsDelayBetweenExecuteAllAvg() const{
            return statisticsDelayBetweenExecuteAll.Average();
        }
#   endif

#endif


void MulticastToActions::operator()(){
    IntrusiveList<ActionNode>* actions;
    {
        InstantScheduler_EnterCritical
        actions = &actionsToExecute[useFirst]; //bool gives 0 or 1 as index 
        useFirst = !useFirst;
        InstantScheduler_LeaveCritical
    }

    // execute what we have accumulated so far
    for(;;){
        auto action = actions->begin(); 
        
        if( action == actions->end() ){
            break; //nothing to do with this list
        }
        
        //Item is removed but iterator stays valid))
        action->RemoveFromChain();
        
        action->eventSlot(); //Execute stuff (this can add action to somewhere)

        if(
                // action does not want to be autoremoved!
                !action->multicastToActionsRemoveAfterCall
            /* and that action did not add self 
                to somewhere else so far */
            &&  action->IsChainElementSingle()
        ){
            // add to list for next call
            InstantScheduler_EnterCritical
            actionsToExecute[useFirst].InsertAtBack(action.operator->());
            InstantScheduler_LeaveCritical
        }
    }
}

#endif
