/** @file InstantTimer.h
    @brief Simple timing classes to track timings in platform independent way

    Simple platform independent time counting, without any dependencies,
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
        #define SIGNAL_PIN 7
        bool signalState = false;

        void setup() {
            Serial.begin(115200);
            
            pinMode(LED_BUILTIN, OUTPUT);
            pinMode(SIGNAL_PIN, OUTPUT);

            //Note: we need to set periodic_timer_signal only once
            periodic_timer_signal.StartPeriod(micros(), 500);
        }

        void loop() {
            auto us = micros();
            if( timer_led.Check(us)){
                // remember SimpleTimer is initially "expired", 
                //so we will immediately enter condition

                if(!ledState){
                    LED_ON;
                }
                else{
                    LED_OFF;
                }
                ledState = !ledState;
                timer_led.Start(us, 1000000);
            }
            if(!ledState){
                Serial.println(F("1"));
            }
            else{
                Serial.println(F("0"));
            }

            if( periodic_timer_signal.CheckForEdge(us) ){
                digitalWrite(SIGNAL_PIN, signalState);
                signalState = !signalState;
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


#ifndef InstantTimer_INCLUDED_H
#define InstantTimer_INCLUDED_H


#ifndef INSTANTTIMER_TICKS_TYPE
    ///Type to be used for storing time measurements and time calculations
    /** This shall be the type returned by your time measurement API.
     * It is assumed that time grows continuously with unsigned overflow.
     * It is assumed that arithmetic is unsigned (two's complement)
     * https://stackoverflow.com/a/18195756/4336953 */ 
    #define INSTANTTIMER_TICKS_TYPE unsigned long
#endif


///Simplest timer to be scheduled directly from the code
/** Hide tick counting logic behind straight forward interface,
 * You choose units and API to work with the physical timer,
 * and provide measurements to Start and Check API.
 * The only assumption is that SimpleTimer::Ticks size must match
 * your units of time measurements.
 * REMEMBER: it is user's responsibility to call Check API frequent enough
 *           to make timer work with desired precision */
class SimpleTimer{
public:
    /// Initial timer
    /** Initially timer is marked as already expired (does not count!),
     *  One has to call Start to make it counting (non expired) */
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
    using Ticks = INSTANTTIMER_TICKS_TYPE;

    /// The maximum ticks amount SimpleTimer is able to wait
    static constexpr Ticks DeltaMax = ~Ticks(0) / 2;


    /// Mark timer as "non expired" and remember expiration moment
    /** Remembers "expected" time for the timer to expire,
     * from now all calls to Check return false,
     * until delta ticks expires. */
    void Start(Ticks currentTicks, Ticks delta){
        expectedAbsoluteTicks = currentTicks + delta;
        //start counting 
        expired = false;
    }

    ///Check time expired (using currentTicks provided by the user)
    /** Turns true once delta ticks expires since call to Start API,
     * after expiration all the subsequent calls return true,
     * until Start is called again.
     * It is assumed that Check is called more often then DeltaMax,
     * thus we will not miss the moment once timer expires */
    bool Check(Ticks currentTicks){
        if( !expired ){
            //compare unsigned values assuming two's complement
            if( (currentTicks - expectedAbsoluteTicks) > DeltaMax ){
                //current time is still before the desired time
                return false;
            }
            //Time passed
            expired = true;
        }
        return true;
    }

    ///Check only for the edge as signal transitions to expired
    /** Turns to true only when delta ticks expires since call to Start API,
     * then all other the subsequent calls return false. */
    bool CheckForEdge(Ticks currentTicks){
        if( !expired ){
            //compare unsigned values assuming two's complement
            if( (currentTicks - expectedAbsoluteTicks) > DeltaMax ){
                //current time is still before the desired time
                return false;
            }
            //Time passed
            expired = true;
            //the only moment when true is returned
            return true;
        }
        return false;
    }

    /// Test timer is expired without altering state (time is not checked!)  
    bool IsExpired() const {
        return expired;
    }

    ///Force timer to be marked as expired
    /** Once marked as expired, Check API will return tur,
     * but CheckForEdge will not react */
    void ForceExpire(){
        expired = true;
    } 


    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    void Start(OtherTicks currentTicks, Ticks delta) = delete;

    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    bool Check(OtherTicks currentTicks) = delete;

    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    bool CheckForEdge(OtherTicks currentTicks, Ticks delta) = delete;

private:
    /// True if timer is expired (not waiting for the next)
    bool expired = true;
    ///Time when Check starts to return true
    /** In this way we remember only the single value to compare with */
    Ticks expectedAbsoluteTicks = 0;
};


/// Simplest periodic timer
/** REMEMBER: it is user's responsibility to call Check API frequent enough
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
    using Ticks = INSTANTTIMER_TICKS_TYPE;


    /// The maximum ticks amount PeriodicTimer is able to wait
    static constexpr Ticks PeriodMax = ~Ticks(0) / 2;


    /// Start timing periods and remember the next period moment
    /** Remembers "expected" time for the timer to detect next period,
     * Use periodic call to CheckForEdge to detect next period moment */
    void StartPeriod(Ticks currentTicks, Ticks period){
        generationPeriod = period;
        nextPeriodAbsoluteTicks = currentTicks + period;
    }


    ///Check only for the edge as signal transitions to expired (new period started)
    /** Turns to true only when period ticks expires since call to Start API,
     * then all other the subsequent calls return false. */
    bool CheckForEdge(Ticks currentTicks){
        if( generationPeriod ){
            //compare unsigned values assuming two's complement
            if( (currentTicks - nextPeriodAbsoluteTicks) > PeriodMax ){
                //current time is still before the desired time
                return false;
            }
            //Time passed

            //next time in the future 
            //(not from the currentTicks but from expected ticks)
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

    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    void StartPeriod(OtherTicks currentTicks, Ticks delta) = delete;

    ///Suppress any "incompatible" timings
    /** This forces using exact Ticks type */
    template<class OtherTicks>
    bool CheckForEdge(OtherTicks currentTicks, Ticks delta) = delete;

public:
    /// True when period generation is active
    Ticks generationPeriod = 0;
    ///Time when Check starts to return true
    Ticks nextPeriodAbsoluteTicks = 0;
};

#endif
