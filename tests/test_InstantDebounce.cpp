/** @file tests/test_InstantDebounce.cpp
    @brief Unit tests for InstantDebounce.h
*/

#include "InstantDebounce.h"
#include "doctest/doctest.h"

TEST_CASE("InstantDebounce: SimpleDebounce") {
    // assume we use millis for debouncing
    SimpleDebounce button(false, 50);

    //Initial state is false
    CHECK(button.Value() == false);

    SUBCASE("Debouncing when no chatter") {
        button.Discover(SimpleDebounce::Ticks(1000), true);
        CHECK( !button.Value() ); //still false, counts
        button.Discover(SimpleDebounce::Ticks(1003), true);
        CHECK( !button.Value() ); //still false, counts
        button.Discover(SimpleDebounce::Ticks(1049), true);
        CHECK( !button.Value() ); //still false, counts
        
        button.Discover(SimpleDebounce::Ticks(1050), true);
        CHECK( button.Value() ); //debounced, now true

        button.Discover(SimpleDebounce::Ticks(1051), true);
        CHECK( button.Value() ); //still true

        button.Discover(SimpleDebounce::Ticks(1100), false);
        CHECK( button.Value() ); //still true, counts
        button.Discover(SimpleDebounce::Ticks(1125), false);
        CHECK( button.Value() ); //still true, counts
        button.Discover(SimpleDebounce::Ticks(1149), false);
        CHECK( button.Value() ); //still true, counts

        button.Discover(SimpleDebounce::Ticks(1150), false);
        CHECK( !button.Value() ); //debounced, now false

        button.Discover(SimpleDebounce::Ticks(1151), false);
        CHECK( !button.Value() ); //still false
    }
    SUBCASE("Debouncing when single spike") {
        //chatter that causes timer to start, 
        button.Discover(SimpleDebounce::Ticks(1152), true);
        CHECK( !button.Value() ); //still false
        button.Discover(SimpleDebounce::Ticks(1153), false); //timer counts from scratch
        CHECK( !button.Value() ); //still false

        button.Discover(SimpleDebounce::Ticks(1175), false); //timer starts counting
        CHECK( !button.Value() ); //still false

        //Assume there were no other measurements, timer should expire
        button.Discover(SimpleDebounce::Ticks(1203), false);
        CHECK( !button.Value() ); //still false

        /*Assume there were no other measurements, timer should expire
          unless we continue pressing and releasing again */
        button.Discover(SimpleDebounce::Ticks(1303), true);
        CHECK( !button.Value() ); //still false
    }

    SUBCASE("Debouncing with chatter") {
        button.Discover(SimpleDebounce::Ticks(1000), true);
        CHECK( !button.Value() ); //still false, counts
        button.Discover(SimpleDebounce::Ticks(1003), true);
        CHECK( !button.Value() ); //still false, counts
        button.Discover(SimpleDebounce::Ticks(1049), true);
        CHECK( !button.Value() ); //still false, counts

        //chatter in the last moment
        button.Discover(SimpleDebounce::Ticks(1050), false);
        CHECK( !button.Value() ); //still false, counts from start
        button.Discover(SimpleDebounce::Ticks(1051), true);
        CHECK( !button.Value() ); //still false, continues counting
        button.Discover(SimpleDebounce::Ticks(1090), true);
        CHECK( !button.Value() ); //still false, continues counting
        button.Discover(SimpleDebounce::Ticks(1100), true);
        CHECK( !button.Value() ); //still false, continues counting
        button.Discover(SimpleDebounce::Ticks(1101), true);
        CHECK( button.Value() ); //debounced!



        button.Discover(SimpleDebounce::Ticks(1151), true);
        CHECK( button.Value() ); //still true

        //go back to false
        button.Discover(SimpleDebounce::Ticks(1200), false);
        CHECK( button.Value() ); //still true, counts
        button.Discover(SimpleDebounce::Ticks(1225), false);
        CHECK( button.Value() ); //still true, counts
        button.Discover(SimpleDebounce::Ticks(1249), false);
        CHECK( button.Value() ); //still true, counts

        //chatter in the last moment
        button.Discover(SimpleDebounce::Ticks(1250), true);
        CHECK( button.Value() ); //still false, counts from start
        button.Discover(SimpleDebounce::Ticks(1251), false);
        CHECK( button.Value() ); //still false, continues counting
        button.Discover(SimpleDebounce::Ticks(1290), false);
        CHECK( button.Value() ); //still false, continues counting
        button.Discover(SimpleDebounce::Ticks(1300), false);
        CHECK( button.Value() ); //still false, continues counting
        button.Discover(SimpleDebounce::Ticks(1301), false);
        CHECK( !button.Value() ); //debounced!
    }
}
