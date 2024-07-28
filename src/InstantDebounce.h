/** @file InstantDebounce.h
    @brief General debouncing independent of CPU and platform
           can be used for debouncing button states or any other chattering value

There are two approaches for debouncing:

    - use DebounceAction or ButtonAction detecting new debounced value
        with the help of the scheduler (the preferred way!)

    - "Discover" debounced value with SimpleDebounce 
        by checking for debounced state "manually" in a loop
        (the "straight forward" way of iterative checks without Scheduler
        this time value in ticks/milliseconds/microseconds has to be provided
        "manually" by the user)
    

Both approaches are covered in samples below.

Simple demo for debouncing button with "Discover" approach control LED and beep))
 @code
    // Pin where button is connected (as INPUT_PULLUP)
    #define BUTTON_PIN 2
    // INPUT_PULLUP makes pressed button value LOW
    bool IsButtonPressed(){
        return digitalRead(BUTTON_PIN) == LOW;
    }
    // assume we use micros for debouncing
    SimpleDebounce button(false, 50000);

    #define LED_ON  digitalWrite(LED_BUILTIN, HIGH)
    #define LED_OFF digitalWrite(LED_BUILTIN, LOW)

    //Pin with beeper
    #define BEEP_PIN 7

    namespace{
        // For demo purposes generate frequency directly from the code
        PeriodicTimer beep_timer;
        // Used to toggle value on BEEP_PIN
        bool beepState = false;
    }

    void setup() {
        Serial.begin(115200);
        // remember SimpleTimer is initially "expired", 
        //so we will immediately enter condition

        pinMode(BUTTON_PIN, INPUT_PULLUP);
        
        pinMode(LED_BUILTIN, OUTPUT);
        pinMode(BEEP_PIN, OUTPUT);

        beep_timer.StartPeriod(micros(), 500);
    }

    void loop() {
        auto us = micros();
        if( button.Discover(us, IsButtonPressed()) ){
            Serial.print(F("Button state just changed to:"));

            if( button.Value() ){
                Serial.println(F("PRESSED"));
                LED_ON;
            }
            else{
                Serial.println(F("RELEASED"));
                LED_OFF;
            }
        }
        //else means button debounced state did not change

        // checked on each iteration
        if( button.Value() ){
            //use timer to simulate beep (for demo purposes))
            if( beep_timer.Discover(us) ){
                digitalWrite(BEEP_PIN, beepState);
                beepState = !beepState;
            }
        }
    }
 @endcode


NOTE: both InstantTimer.h and InstantScheduler.h are also platform independent
      InstantDebounce.h will automatically include existing one :) 

NOTE: InstantDebounce.h class instances inherit interrupt (thread) safety 
      from corresponding InstantScheduler.h file settings.
      Option with InstantTimer.h (SimpleDebounce) is intended to be used
      from the same thread (see sample with button.Discover above)

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


#ifndef InstantDebounce_INCLUDED_H
#define InstantDebounce_INCLUDED_H

//activate DebounceAction feature only if InstantScheduler dependency is present 
#if __has_include("InstantScheduler.h") && __has_include("InstantDelegate.h")
// The simplest possible portable scheduler suitable for embedded 
// platforms like Arduino (actually only standard C++ is required)
#include "InstantScheduler.h"
// Fast deterministic delegates for invoking callbacks,
// suitable for real time operation (no heap allocation at all)
#include "InstantDelegate.h"


/// Common debouncing code for reuse by DebounceAction and ButtonAction
class DebounceBase{
public:
    /// Callback used to check for button status
    using RawValueChecker = Delegate< bool() >;

    /// Units being used for time measurements 
    using Ticks = Scheduler::Ticks;

    /// Type to hold number of checks before considering value as debounced
    using IntervalCount = unsigned char;

    /// Setup initial instance parameters
    DebounceBase(
        bool initialValue, ///< Value considered to be initial from start
        Ticks checkIntervalTicks, ///< Interval between checks for raw value while debounce
        IntervalCount totalIntervals ///< How many intervals before debounce
    )
        : currentDebouncedVal(initialValue)
        , checkInterval(checkIntervalTicks)
        , successCountExpected(totalIntervals)
        {} 

    /// Current value considered to be filtered (debounced)
    bool Value() const{
        return successCountCurrent >= successCountExpected;
    }

    /// Remove from the corresponding scheduler (and so prevent state detection)
    void Cancel(){
        action.Cancel();
    }
    
protected:
    /// Set DebounceBase into listening mode according to available settings
    void Schedule(
        Scheduler& targetScheduler,
        const RawValueChecker& rawValueChecker,
        const ActionNode::Callback& callback
    ){
        checker = rawValueChecker;
        action.Set(callback).ScheduleAfter(targetScheduler, checkInterval);
    }

    /// To be called from derived class
    bool Discover(){
        if( checker() != currentDebouncedVal ){
            if( ++successCountCurrent >= successCountExpected ){
                // new value is the opposite of previous
                currentDebouncedVal = !currentDebouncedVal;
                return true; // Sign we have new value
            }
        }
        else{
            successCountCurrent = 0;
        }
        return false;
    }

    
private:
    /// Internal action to cooperate with the scheduler
    ActionNode action;

    /// The callback being used to read current raw (not debounced) value
    RawValueChecker checker{ []() -> bool { return false; } };

    /// Value being already debounced (considered as being current)
    bool currentDebouncedVal = false;

    /// Interval between checks for raw value while debounce
    const Ticks checkInterval;
    /// Desired number of changed values before accepting a new one!
    const IntervalCount successCountExpected;
    /// Number of changed values accumulated so far
    IntervalCount successCountCurrent = 0;
    
};



/// Debounce single digital value (to be used with Scheduler)
class DebounceAction: public DebounceBase{
public:
    using DebounceBase::DebounceBase;


    /// Callback invoked once status changes
    using Callback = Delegate< void() >;

    /// Setup callback to be issued once true value is debounced
    DebounceAction& OnTrue(const Callback& callbackOnTrue){
        onTrue = callbackOnTrue;
        return *this;
    }

    /// Setup callback to be issued once false value is debounced
    DebounceAction& OnFalse(const Callback& callbackOnFalse){
        onTrue = callbackOnFalse;
        return *this;
    }

    /// Set DebounceBase into listening mode according to available settings
    void Schedule(
        Scheduler& targetScheduler,
        const RawValueChecker& checker
    ){
        DebounceBase::Schedule(
            targetScheduler,
            checker,
            ActionNode::Callback::From(this).Bind<&DebounceAction::ActionHandler>()
        );
    }


private:
    /// Called once true value is debounced
    Callback onTrue{ [](){} };
    /// Called once false value is debounced
    Callback onFalse{ [](){} };

    /// Callback being issued bu the scheduler on each check interval
    void ActionHandler(){
        if( Discover() ){
            // New value discovered!
            if( Value() ){
                onTrue();
            }
            else{
                onFalse();
            }
        }
    }
};


/// Keep track of the button with the help of Scheduler
class ButtonAction: public DebounceBase{
public:
    /// Callback used to check for button status
    using Checker = Delegate< bool() >;
    
    /// Units being used for time measurements 
    using Ticks = Scheduler::Ticks;

    ///
    ButtonAction(
        const Checker& testForValue,
        Ticks checkInterval,
        unsigned numCheckIntervalsToDebounce = 1
    );



private:
    void ActionHandler(){

    }
};



#endif


//activate SimpleDebounce feature only if InstantTimer dependency is present 
#if __has_include("InstantTimer.h")
// Simple timing classes to track timings in platform independent way
#include "InstantTimer.h"

/// Debounce single digital value (to be used in continuous loop)
class SimpleDebounce{
public:
    /// Units being used for time measurements
    /** It is yous choice to provide milliseconds or microseconds,
     * just remember to be consistent for the same SimpleDebounce instance,
     * requirements shall be the same as for SimpleTimer::Ticks type */
    using Ticks = SimpleTimer::Ticks;

    /// Setup initial instance parameters
    constexpr SimpleDebounce(
        bool initialValue, ///< Value considered to be initial from start
        Ticks debounceTicks ///< How long to wait before accepting a new value
    )
        : currentDebouncedVal(initialValue)
        , debounceInterval(debounceTicks)
    {}

    /// Current value considered to be filtered (debounced)
    constexpr bool Value() const {
        return currentDebouncedVal;
    }

    /// Take the new time and value into account
    /** Called with current value on each iteration,
     * @returns true if new (different) value is detected ("discovered")
     *          after the debounce cycle was performed
     *  (returned value can be used to detect state change) */
    bool Discover(
        Ticks time, ///< Current time is measured in the same units 
                    ///< as units in the constructor
        bool val ///< Current value corresponding to time
    ){
        if( simpleTimer.IsPending() ){
            //Debouncing is in progress right now
            if( val == currentDebouncedVal ){
                /* Value is the same as before debounce again, 
                   chattering continues, value is unstable */
                simpleTimer.Cancel();
            }
            else if( simpleTimer.Discover(time) ){
                // We were in debounce state, and debouncing timer just expired!
                currentDebouncedVal = !currentDebouncedVal;
                return true; //We have detected DIFFERENT VALUE for sure!
            }
        }
        else if( val != currentDebouncedVal ){
            //new value detected, start debouncing
            simpleTimer.Start(time, debounceInterval);
        }

        return false;
    }

    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    bool Discover(
        OtherTicks time, ///< Current time in the same units as in constructor
        bool val
    ) = delete;

private:
    /// Value being already debounced (considered as being current)
    bool currentDebouncedVal = false;

    /// Timed to be used for debounce (remember it is initially expired!)
    SimpleTimer simpleTimer;
    /// Timeout used to charge simpleTimer for debounce
    const Ticks debounceInterval;
};

#endif

#endif