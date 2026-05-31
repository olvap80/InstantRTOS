/** @file tests/test_InstantTimer.cpp
    @brief  Unit tests for InstantTimer.h
*/

#include "InstantTimer.h"
#include "doctest/doctest.h"
#include <limits>

TEST_CASE("InstantTimer: SimpleTimer") {
    //Remember, we provide a time, timer just tracks it
    SimpleTimer simpleTimer;

    //Never expired before starting
    CHECK( !simpleTimer.IsPending() );


    SUBCASE("Normal expiration") {
        /*We provide a time, timer just tracks it,
          test starting from "some moment" */
        simpleTimer.Start(SimpleTimer::Ticks(10000), 1000);
        //Never expired after starting
        CHECK( simpleTimer.IsPending() );

        //All values before expiration time do not trigger
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(10010)) );
        CHECK( simpleTimer.IsPending() );
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(10100)) );
        CHECK( simpleTimer.IsPending() );
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(10999)) );
        CHECK( simpleTimer.IsPending() );

        //Expiration time triggers, timer triggering shall be discovered
        CHECK( simpleTimer.Discover(SimpleTimer::Ticks(11000)) );
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );

        //Timer expires only once
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(11000)) );
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(11001)) );
        CHECK( !simpleTimer.IsPending() );

        //Timer expires only once
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(12001)) );
        CHECK( !simpleTimer.IsPending() );

        //But can be started again

        simpleTimer.Start(SimpleTimer::Ticks(20000), 1000);
        //Never expired after starting
        CHECK( simpleTimer.IsPending() );

        //All values before expiration time do not trigger
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(20010)) );
        CHECK( simpleTimer.IsPending() );
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(20100)) );
        CHECK( simpleTimer.IsPending() );
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(20999)) );
        CHECK( simpleTimer.IsPending() );

        //Time after expiration also triggers
        CHECK( simpleTimer.Discover(SimpleTimer::Ticks(21500)) );
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );

        //Timer expires only once
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(21501)) );
        CHECK( !simpleTimer.IsPending() );

        //Also can cross unsigned bounds once scheduled near boundary
        constexpr SimpleTimer::Ticks newStartFrom
            = std::numeric_limits<SimpleTimer::Ticks>::max() - 500;

        simpleTimer.Start(newStartFrom, 1000);
        CHECK( simpleTimer.IsPending() );

        CHECK( !simpleTimer.Discover(newStartFrom + 1) );
        CHECK( simpleTimer.IsPending() );

        CHECK( !simpleTimer.Discover(newStartFrom + 999) );
        CHECK( simpleTimer.IsPending() );

        //Wrapping around the boundary is not a problem
        CHECK( simpleTimer.Discover(newStartFrom + 1000) );
        CHECK( !simpleTimer.IsPending() );

        CHECK( !simpleTimer.Discover(newStartFrom + 1001) );
        CHECK( !simpleTimer.IsPending() );
    }
    SUBCASE("Start again (restart) without expiration") {
        //We provide a time, timer just tracks it
        simpleTimer.Start(SimpleTimer::Ticks(0), 1000);
        //Never expired after starting
        CHECK( simpleTimer.IsPending() );

        //We provide a time, timer just tracks it
        simpleTimer.Start(SimpleTimer::Ticks(1000), 1000);
        //Never expired after starting
        CHECK( simpleTimer.IsPending() );

        //All values before expiration time do not trigger
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(1010)) );
        CHECK( simpleTimer.IsPending() );
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(1100)) );
        CHECK( simpleTimer.IsPending() );
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(1999)) );
        CHECK( simpleTimer.IsPending() );

        //Expiration time triggers, timer triggering shall be discovered
        CHECK( simpleTimer.Discover(SimpleTimer::Ticks(2000)) );
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );

        //Timer expires only once
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(11001)) );
        CHECK( !simpleTimer.IsPending() );
    }
    SUBCASE("Corner case - 0 time") {
        simpleTimer.Start(SimpleTimer::Ticks(0), 0);

        //Never expired after starting
        CHECK( simpleTimer.IsPending() );

        //Discovering the time triggers the timer
        CHECK( simpleTimer.Discover(SimpleTimer::Ticks(0)) );
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );
    }
    SUBCASE("Cancel") {
        simpleTimer.Start(SimpleTimer::Ticks(0), 0);
        //Never expired after starting
        CHECK( simpleTimer.IsPending() );

        //Force expiration
        simpleTimer.Cancel();
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );

        //Timer expires only once
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(1)) );
    }
    SUBCASE("Cancel twice is idempotent and restart works") {
        simpleTimer.Start(SimpleTimer::Ticks(100), 50);
        CHECK( simpleTimer.IsPending() );
        simpleTimer.Cancel();
        CHECK( !simpleTimer.IsPending() );

        // Second cancel has no effect
        simpleTimer.Cancel();
        CHECK( !simpleTimer.IsPending() );

        // Discover after cancel does not trigger
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(1000)) );

        // Can be started again and will expire
        simpleTimer.Start(SimpleTimer::Ticks(2000), 10);
        CHECK( simpleTimer.IsPending() );
        CHECK( simpleTimer.Discover(SimpleTimer::Ticks(2010)) );
        CHECK( !simpleTimer.IsPending() );
    }
    SUBCASE("Cancel after expiration") {
        simpleTimer.Start(SimpleTimer::Ticks(0), 0);
        //Never expired after starting
        CHECK( simpleTimer.IsPending() );

        //Discovering the time triggers the timer
        CHECK( simpleTimer.Discover(SimpleTimer::Ticks(0)) );
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );

        //Force expiration
        simpleTimer.Cancel();
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );

        //Timer expires only once
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(1)) );
    }
    SUBCASE("Is never 'Discovered' without starting") {
        //Discovering the time triggers the timer
        CHECK( !simpleTimer.Discover(SimpleTimer::Ticks(0)) );
        //After expiration, timer is not pending
        CHECK( !simpleTimer.IsPending() );
    }
}

TEST_CASE("PeriodicTimer"){
    PeriodicTimer periodicTimer;
    REQUIRE( !periodicTimer.IsActive() );
    REQUIRE( !periodicTimer.Discover(PeriodicTimer::Ticks(0)) );

    SUBCASE("Normal periods") {
        periodicTimer.StartPeriod(SimpleTimer::Ticks(10000), 1000);

        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10010)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10500)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10999)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(11000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11001)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11200)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11999)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(12000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(12000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(12001)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(12999)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(13100)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(13999)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(14000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(14001)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(14999)) );
    }
    SUBCASE("Wrap-around across max boundary") {
        //All values are measured from here
        const auto nearMax = std::numeric_limits<PeriodicTimer::Ticks>::max() - 50;

        periodicTimer.StartPeriod(nearMax, 100);

        // Before first period (shall not trigger, as not yet reached)
        CHECK( !periodicTimer.Discover(nearMax + 10) );

        SUBCASE("Straight crossing the boundary directly to the period") {
            // Cross boundary directly to the period
            CHECK( periodicTimer.Discover(nearMax + 100) );
        }
        SUBCASE("Crossing the boundary in steps") {
            //Just crossed the boundary (shall not trigger, as not yet reached)
            CHECK( !periodicTimer.Discover(nearMax + 50) );
            CHECK( !periodicTimer.Discover(nearMax + 51) );

            // Should detect period at nearMax+100 (wraps)
            CHECK( periodicTimer.Discover(nearMax + 100) );

            // Next period is at +200
            CHECK( !periodicTimer.Discover(nearMax + 150) ); //not yet
            CHECK( periodicTimer.Discover(nearMax + 200) ); //right on time
            CHECK( !periodicTimer.Discover(nearMax + 201) ); //just after
        }
    }
    SUBCASE("Catch-up when current jumps far ahead"){
        periodicTimer.StartPeriod(PeriodicTimer::Ticks(0), 100);

        // Jump far ahead beyond multiple periods
        const PeriodicTimer::Ticks now = 1000;

        for(int i = 0; i < 10; ++i){
            // Should detect all missed periods one by one
            CHECK( periodicTimer.Discover(now) );
        }

        // All periods detected, next call shall not trigger
        CHECK( !periodicTimer.Discover(now) );

        // Next time in future should trigger again
        CHECK( periodicTimer.Discover(now + 100) );
    }
    SUBCASE("Reactivate after Deactivate and period=1 behavior"){
        periodicTimer.StartPeriod(PeriodicTimer::Ticks(0), 100);
        periodicTimer.Deactivate(); // deactivate right away

        CHECK( !periodicTimer.IsActive() );
        CHECK( !periodicTimer.Discover(PeriodicTimer::Ticks(100)) );

        // Reactivate with period 1
        periodicTimer.StartPeriod(PeriodicTimer::Ticks(1000), 1);
        CHECK( periodicTimer.IsActive() );

        CHECK( !periodicTimer.Discover(PeriodicTimer::Ticks(1000)) ); // not yet
        CHECK( periodicTimer.Discover(PeriodicTimer::Ticks(1001)) ); // now
        CHECK( !periodicTimer.Discover(PeriodicTimer::Ticks(1001)) ); // already done
        CHECK( periodicTimer.Discover(PeriodicTimer::Ticks(1002)) ); // again
    }
    SUBCASE("Changing periods") {
        periodicTimer.StartPeriod(SimpleTimer::Ticks(10000), 1000);

        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10010)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10500)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10999)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(11000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11001)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11200)) );

        periodicTimer.StartPeriod(SimpleTimer::Ticks(11500), 2000);
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11999)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(12000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(12500)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(13499)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(13500)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(13999)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(14999)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(15499)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(15510)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(17499)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(17500)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(17501)) );

    }
    SUBCASE("Deactivate") {
        periodicTimer.StartPeriod(SimpleTimer::Ticks(10000), 1000);

        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10010)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10500)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(10999)) );

        CHECK( periodicTimer.Discover(SimpleTimer::Ticks(11000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11001)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11200)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11999)) );

        periodicTimer.Deactivate();
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(12000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(13000)) );
    }
    SUBCASE("Deactivate immediately after starting") {
        periodicTimer.StartPeriod(SimpleTimer::Ticks(10000), 1000);
        periodicTimer.Deactivate();

        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11001)) );
    }
    SUBCASE("Periods with 0 period are already \"deactivated\" (no period)") {
        periodicTimer.StartPeriod(SimpleTimer::Ticks(10000), 0);
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(11000)) );
        CHECK( !periodicTimer.Discover(SimpleTimer::Ticks(21001)) );
    }

}
