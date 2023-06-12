/** @file InstantDebounce.h 
    @brief General debouncing (depends only on InstantTimer.h/InstantScheduler.h)

    Simple demo for debouncing button to control LED and beep))
    @code
        // Pin where button is connected (as INPUT_PULLUP)
        #define BUTTON_PIN 4
        // INPUT_PULLUP makes pressed button value LOW
        bool IsButtonPressed(){
            return digitalRead(BUTTON_PIN) == LOW;
        }
        // assume we use micros for debouncing
        SimpleDebounce button(50000, false);

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

            //Note: timer_led is initially expired, so we will enter under if
            
            beep_timer.StartPeriod(micros(), 500);
        }

        void loop() {
            auto us = micros();
            if( button.Update(us, IsButtonPressed()) ){
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
                if( beep_timer.CheckForEdge(us) ){
                    digitalWrite(BEEP_PIN, beepState);
                    beepState = !beepState;
                }
            }
        }
    @endcode

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

//activate SimpleDebounce feature only if InstantTimer dependency is present 
#if __has_include("InstantTimer.h")
#include "InstantTimer.h"

/// Debounce single digital value (to be used in continuous loop)
class SimpleDebounce{
public:
    /// Units being used for time measurements
    /** It is yous choice to provide milliseconds or microseconds,
     * just remember to be consistent for the same SimpleDebounce instance,
     * requirements shall be the same as for SimpleTimer::Ticks type */
    using Ticks = SimpleTimer::Ticks;

    /// Setup initial instance
    SimpleDebounce(
        Ticks pinDebounceTicks, ///< How long to wait before accepting a new value
        bool initialVal ///< Value considered to be initial from start
    )
        : pinDebounce(pinDebounceTicks)
        , currentDebouncedVal(initialVal)
    {}

    /// Current value considered to be filtered (debounced)
    bool Value(){
        return currentDebouncedVal;
    }

    /// Take the new time and value into account
    /** Called with current value on each iteration,
     * \returns true if new (different) value is detected due to debounce
     *  (returned value can be used to detect state change) */
    bool Update(
        Ticks time, ///< Current time in the same units as in constructor
        bool val
    ){
        bool sameValue = (val == currentDebouncedVal);
        
        if( simpleTimer.CheckForEdge(time) ){
            // We were in debounce state, and debouncing timer just expired!
            if( !sameValue ){
                //new debounced value was detected
                currentDebouncedVal = val;
                return true; //DIFFERENT VALUE for sure!
            }
            //else means button was released before debounce time!
        }
        else{
            //here we are either counting debounce time or already debounced

            // Using != for booleans as logical XOR ))
            if( sameValue != simpleTimer.IsExpired() ){
                /* here != means one of:
                1. same value while counting debounce interval
                                            -> chattering continues
                2. different value when previous was debounced
                                            -> need to debounce again
                BOTH cases mean we start timing from scratch */
                simpleTimer.Start(time, pinDebounce);
            }
        }
        
        return false; //value did not change (yet)
    }

private:
    /// Timed to be used for debounce (remember it is initially expired!)
    SimpleTimer simpleTimer;
    /// Timeout used to charge simpleTimer for debounce
    const Ticks pinDebounce;

    /// Value being already debounced (considered as being current)
    bool currentDebouncedVal = false;
};

#endif

//activate DebounceAction feature only if InstantScheduler dependency is present 
#if __has_include("InstantScheduler.h") && __has_include("InstantDelegate.h")
#include "InstantScheduler.h"
#include "InstantDelegate.h"

/// Debounce single digital value (to be used with Scheduler)
class DebounceAction{
public:
    /// Callback used to check for button status
    using Checker = Delegate< bool() >;
    
    /// Units being used for time measurements 
    using Ticks = Scheduler::Ticks;

    /// Callback invoked once status changes
};


/// Keep track of the button with the help of Scheduler
class ButtonAction{
public:
    /// Callback used to check for button status
    using Checker = Delegate< bool() >;
    
    /// Units being used for time measurements 
    using Ticks = Scheduler::Ticks;

    ///
    enum Event{
        Pressed,
        Released,
    };

    ///
    ButtonAction(
        const Checker& testForValue,
        Ticks checkInterval,
        unsigned numCheckIntervalsToDebounce = 1
    );



public:

};



#endif

#endif