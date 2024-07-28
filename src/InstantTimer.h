/** @file InstantTimer.h
 @brief Simple timing classes to track timings in platform independent way

Simple platform independent handy time counting, without any dependencies,
straight forward implementation to use directly from program loop,
follows absolute minimalism to be cheap by memory and functionality.
(Note: consider InstantScheduler.h when you need some more complex scheduling))


The InstantTimer.h implements following:

    - SimpleTimer is resettable "one shot" timer 
                    that avoids burden of manual interval counting
                    (timer can be scheduled again later)
    - PeriodicTimer is autoreset "multi shot" timer to generate
                    periodic events in fixed interval

Ideal for single treaded environment, see sample below for details.
 @code
    SimpleTimer timer_led;
    bool ledState = false;

    #define LED_ON  digitalWrite(LED_BUILTIN, HIGH)
    #define LED_OFF digitalWrite(LED_BUILTIN, LOW)

    PeriodicTimer periodic_timer_signal;
    #define SIGNAL_PIN 4
    bool signalState = false;

    void setup() {
        Serial.begin(115200);

        pinMode(LED_BUILTIN, OUTPUT);
        pinMode(SIGNAL_PIN, OUTPUT);
        
        auto us = micros();
        timer_led.Start(us, 1000000);
        periodic_timer_signal.StartPeriod(us, 500);
    }

    void loop() {
        auto us = micros();
        if( timer_led.Discover(us) ){
            if(!ledState){
                LED_ON;
            }
            else{
                LED_OFF;
            }
            ledState = !ledState;
            //need to "charge" simple timer every time 
            timer_led.Start(us, 1000000);
        }
        if( !ledState ){
            Serial.println(F("1"));
        }
        else{
            Serial.println(F("0"));
        }

        //PeriodicTimer is "recharged" automatically 
        if( periodic_timer_signal.Discover(us) ){
            digitalWrite(SIGNAL_PIN, signalState);
            signalState = !signalState;
        }
    }
 @endcode


NOTE: InstantTimer.h is intended to be used from the same thread
      (see sample with timer_led.Discover above)

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


#ifndef InstantTimer_INCLUDED_H
#define InstantTimer_INCLUDED_H


#ifndef InstantTimer_Ticks_Type
    ///Type to be used for storing time measurements and time calculations
    /** This shall be the type returned by your time measurement API.
     * It is assumed that time grows continuously with unsigned overflow.
     * It is assumed that arithmetic is unsigned (two's complement)
     * https://stackoverflow.com/a/18195756/4336953 */ 
    #define InstantTimer_Ticks_Type unsigned long
#endif


///Simplest timer to be scheduled directly from the code
/** Hide tick counting logic behind straight forward interface,
 * You choose units and API to work with the physical timer,
 * and provide measurements to Start() and Discover() API.
 * The only assumption is that SimpleTimer::Ticks size must match
 * your units of time measurements.
 * REMEMBER: it is user's responsibility to call Discover() API frequent enough
 *           to make timer work with desired precision */
class SimpleTimer{
public:
    /// Initial timer
    /** Initially timer does not count (IsPending() returns false),
     *  One has to call Start to make it counting */
    SimpleTimer() = default;


    /// The time measurement unit for the SimpleTimer
    /** Can be anything, milliseconds, microseconds seconds,
     * meaning is defined by the user.
     * Actually SimpleTimer does not measure time,
     * user has to provide measurements!
     * This shall be the type returned by your time measurement API.
     * It is assumed that time grows continuously with unsigned overflow.
     * It is assumed that arithmetic is unsigned (two's complement)
     * https://stackoverflow.com/a/18195756/4336953 */
    using Ticks = InstantTimer_Ticks_Type;

    /// The maximum ticks amount SimpleTimer is able to wait
    static constexpr Ticks DeltaMax = ~Ticks(0) / 2;


    /// Mark timer as "pending" and remember next trigger moment
    /** Remembers "expected" time for the timer to trigger,
     * from now all calls to IsPending() return true,
     * until delta ticks expires. */
    void Start(Ticks currentTicks, Ticks delta){
        expectedAbsoluteTicks = currentTicks + delta;
        //start counting 
        isPending = true;
    }

    /// Update and check for the "edge" when timer triggers
    /** Returns true only when delta ticks expires since call to Start API
     * this means new "expiration state" detected ("discovered"),
     * then all other subsequent calls will return false again.
     * Use IsPending() API to check if timer still counting.
     * Call Start() to "recharge" the timer again.
     *
     * It is assumed that Discover() is called more often then DeltaMax,
     * (actually more often then expected time precision!)
     * thus we will not miss the moment once timer expires! */
    bool Discover(Ticks currentTicks){
        if( isPending ){
            //compare unsigned values assuming two's complement
            if( (currentTicks - expectedAbsoluteTicks) > DeltaMax ){
                //current time is still before the desired time
                return false;
            }
            // We have just detected time passed
            isPending = false;
            //trigger detection is the only moment when true is returned
            return true;
        }
        return false;
    }

    /// Test timer is pending without altering state (time is not checked!)
    /** Reads existing state (as calculated by the most recent Discover() call)
     * @returns true only between Start and expiration moment,
     *          false is returned before Start() of after expire.
     * 
     * Remember one must periodically call Discover() API to detect expiration!
     * Discover() returns true only as "edge" of expiration moment is detected,
     * After Discover() has detected expiration, IsPending() always returns false
     * till Start() is called to "recharge" */
    bool IsPending() const {
        return isPending;
    }

    /// Cancel timer without expiration (IsPending() becomes false)
    /** Once Cancel-ed, Discover() API will have no effect 
     * and return false (as a sign that "nothing more was discovered"),
     * but all subsequent calls to IsPending() will return false from now!
     * Call Start to "recharge" timer again. */
    void Cancel(){
        isPending = false;
    } 


    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    void Start(OtherTicks currentTicks, Ticks delta) = delete;

    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    bool Discover(OtherTicks currentTicks) = delete;

private:
    /// True if timer is between Start and expire
    bool isPending = false;
    ///Time when Timer will trigger/expire (Discover() API shall detect expiration)
    /** In this way we remember only the single value to compare with! */
    Ticks expectedAbsoluteTicks = 0;
};


/// Simplest periodic timer
/** REMEMBER: it is user's responsibility to call Discover() API frequent enough
 *            to make timer work with desired precision */
class PeriodicTimer{
public:
    /// Initial timer
    /** Initially timer is not generating any periods,
     *  One has to call StartPeriod to make it counting */
    PeriodicTimer() = default;

    
    /// The time measurement unit for the PeriodicTimer
    /** Can be anything, milliseconds, microseconds seconds,
     * meaning is defined by the user.
     * Actually PeriodicTimer does not measure time,
     * user has to provide measurements!
     * This shall be the type returned by your time measurement API.
     * It is assumed that time grows continuously with unsigned overflow.
     * It is assumed that arithmetic is unsigned (two's complement)
     * https://stackoverflow.com/a/18195756/4336953 */
    using Ticks = InstantTimer_Ticks_Type;


    /// The maximum ticks amount PeriodicTimer is able to wait
    static constexpr Ticks PeriodMax = ~Ticks(0) / 2;


    /// Start timing periods and remember the next period moment
    /** Remembers "expected" time for the timer to detect next period,
     * Use periodic call to Discover() to detect next period moment */
    void StartPeriod(Ticks currentTicks, Ticks period){
        generationPeriod = period;
        nextPeriodAbsoluteTicks = currentTicks + period;
    }


    /// Update state and check for the edge as new period started
    /** Returns true only when period ticks expires since call to Start API,
     * then all other the subsequent calls return false.
     * This call updates PeriodicTimer state, subsequent call see that modification,
     * remember to Discover() more often then corresponding period! */
    bool Discover(Ticks currentTicks){
        if( generationPeriod ){
            //compare unsigned values assuming two's complement
            if( (currentTicks - nextPeriodAbsoluteTicks) > PeriodMax ){
                //current time is still before the desired time
                return false;
            }
            //Time passed

            //next time in the future 
            //(not from the currentTicks but from expected ticks!)
            nextPeriodAbsoluteTicks += generationPeriod;

            //the only moment when true is returned
            return true;
        }
        return false;
    }

    /// Test timer is generating periods
    bool IsActive() const{
        return generationPeriod != 0;
    }

    /// Cause timer to stop generating periods
    void Deactivate(){
        generationPeriod = 0;
    }

    /// Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    void StartPeriod(OtherTicks currentTicks, Ticks delta) = delete;

    /// Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    bool Discover(OtherTicks currentTicks) = delete;

public:
    /// True when period generation is active
    Ticks generationPeriod = 0;
    /// Time when Discover() shall detect next period
    Ticks nextPeriodAbsoluteTicks = 0;
};

#endif
