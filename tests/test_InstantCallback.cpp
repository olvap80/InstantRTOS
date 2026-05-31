/** @file tests/test_InstantCallback.cpp
    @brief Unit tests for InstantCallback.h
*/

#include <exception>
/// Custom exception for testing InstantCallback_Panic
class TestInstantCallbackPanicException: public std::exception{
    const char* what() const noexcept override{
        return "TestInstantCallbackPanicException";
    }
};
//Header will see this definition
#define InstantCallback_Panic() throw TestInstantCallbackPanicException()
#include "InstantCallback.h"


#include "doctest/doctest.h"
#include <tuple>
#include <type_traits>

unsigned invoke_simple_callback(
    unsigned (*simpleFunctionPointer)(unsigned arg)
){
    return simpleFunctionPointer(1000);
}

std::tuple<unsigned, unsigned, unsigned> invoke_multiple(
    unsigned (*simpleFunctionPointer)(unsigned arg)
){
    return {
        simpleFunctionPointer(2000),
        simpleFunctionPointer(3000),
        simpleFunctionPointer(4000)
    };
}

TEST_CASE("InstantCallback normal usage"){
    unsigned capture1 = rand() & 0xFF;
    unsigned capture2 = rand() & 0x3FF;
    SUBCASE("single shot"){
        //demo for "single shot" callback
        auto res = invoke_simple_callback(CallbackFrom<1>(
            [=](unsigned arg){
                return capture1*2 + capture2 + arg;
            }
        ));
        CHECK(res == capture1*2 + capture2 + 1000);
    }
    SUBCASE("multiple shot"){
        //demo for "multiple shot" callback
        auto res = invoke_multiple(CallbackFrom<1>(
            [=](
                CallbackExtendLifetime& lifetime,
                unsigned arg
            ){
                if( 4000 == arg ){
                    //this will free memory after lambda exits
                    lifetime.Dispose();
                }
                return capture1*3 + capture2 + arg;
            }
        ));
        CHECK(std::get<0>(res) == capture1*3 + capture2 + 2000);
        CHECK(std::get<1>(res) == capture1*3 + capture2 + 3000);
        CHECK(std::get<2>(res) == capture1*3 + capture2 + 4000);
    }
}

TEST_CASE("CallbackFrom allocation and InstantCallback_Panic"){
    SUBCASE("Iterative CallbackFrom function pointer shall not cause panic"){
        //this is to test if the memory is correctly freed
        //when the lambda is destroyed
        for(int i=0; i<100; i++){
            auto res = invoke_simple_callback(CallbackFrom<1>(
                [](unsigned arg){ //notice no capture here, this shall be a simple function pointer
                    return arg;
                }
            ));
            CHECK(res == 1000);
        }
    }

    SUBCASE("Iterative CallbackFrom with the same lambda shall not cause panic"){
        //this is to test if the memory is correctly freed
        //when the lambda is destroyed
        for(int i=0; i<100; i++){
            auto res = invoke_simple_callback(CallbackFrom<1>(
                [=](unsigned arg){
                    return arg+i;
                }
            ));
            CHECK(res == 1000 + i);
        }
    }
    SUBCASE("Sequential CallbackFrom with different lambda shall not cause panic"){
        auto cb1 = CallbackFrom<1>([=](unsigned arg){ return arg; });

        //assert that such type is "C style" unsigned(*)(unsigned)
        static_assert(
            std::is_same<decltype(cb1), unsigned(*)(unsigned)>::value,
            "cb1 should be of type unsigned(*)(unsigned)"
        );

        //this works because each lambda has different type and memory
        auto cb2 = CallbackFrom<1>([=](unsigned arg){ return arg; });
        auto cb3 = CallbackFrom<1>([=](unsigned arg){ return arg; });

        //but types of "C style callback" are the same
        static_assert(
            std::is_same<decltype(cb1), decltype(cb2)>::value,
            "cb1 and cb2 should have the same types"
        );
        static_assert(
            std::is_same<decltype(cb2), decltype(cb3)>::value,
            "cb2 and cb3 should have the same types"
        );

        //assert that the memory is correctly freed
        auto res = invoke_simple_callback(cb1);
        CHECK(res == 1000);
        res = invoke_simple_callback(cb2);
        CHECK(res == 1000);
        res = invoke_simple_callback(cb3);
        CHECK(res == 1000);

        #if defined(__cplusplus) && __cplusplus >= 202002L || defined(_MSVC_LANG) && _MSVC_LANG >= 202002L
            //NOTE: a lambda can only appear in an unevaluated context with '/std:c++20' or later

            //assert other kinds of callbacks map to C style callbacks correctly
            static_assert(
                std::is_same<
                    decltype(CallbackFrom<1>([=](unsigned arg){})),
                    void(*)(unsigned)
                >::value,
                "cb1 should be of type void(*)(unsigned)"
            );
            static_assert(
                std::is_same<
                    decltype(CallbackFrom<1>([=](unsigned arg, double arg2){})),
                    void(*)(unsigned, double)
                >::value,
                "cb1 should be of type void(*)(unsigned, double)"
            );
            static_assert(
                std::is_same<
                    decltype(CallbackFrom<1>([=](unsigned arg, double arg2, char arg3){})),
                    void(*)(unsigned, double, char)
                >::value,
                "cb1 should be of type void(*)(unsigned, double, char)"
            );
        #endif
    }
    SUBCASE("Allocating extra lambda call shall cause panic (one item allowed)"){
        constexpr int NumCallbacksAllowed = 1;
        int iterations = 0;
        CHECK_THROWS_AS(
            [&]{
                constexpr int NumCallbacksAllowedNoCapture = 1; //make compiler happy

                for( ; iterations < NumCallbacksAllowedNoCapture+1; ++iterations){
                    (void)CallbackFrom<NumCallbacksAllowedNoCapture>(
                        [=](unsigned arg){
                            return arg;
                        }
                    );
                }
            }(),
            TestInstantCallbackPanicException
        );
        CHECK(iterations == NumCallbacksAllowed);
    }
    SUBCASE("Allocating extra lambda call shall cause panic (ten items allowed)"){
        constexpr int NumCallbacksAllowed = 10;
        int iterations = 0;
        CHECK_THROWS_AS(
            [&]{
                constexpr int NumCallbacksAllowedNoCapture = 10; //make compiler happy

                for( ; iterations < NumCallbacksAllowedNoCapture+1; ++iterations){
                    (void)CallbackFrom<NumCallbacksAllowedNoCapture>(
                        [=](unsigned arg){
                            (void)arg;
                        }
                    );
                }
            }(),
            TestInstantCallbackPanicException
        );
        CHECK(iterations == NumCallbacksAllowed);
    }
}

TEST_CASE("InstantCallback reservedCount=2 allows two simultaneous single-shot callbacks"){
    // Allocate two callbacks without invoking to ensure both slots are held simultaneously.
    constexpr int base1 = 10;
    constexpr int base2 = 20;
    constexpr int base3 = 30;

    auto makeCallback = [](int base) {
        //On this point the same lambda will be used
        return CallbackFrom<2>(
            [=](unsigned arg) -> int {
                return base + (int)arg;
            }
        );
    };

    auto cb1 = makeCallback(base1);
    auto cb2 = makeCallback(base2);

    static_assert(
        std::is_same<decltype(cb1), decltype(cb2)>::value,
        "cb1 and cb2 should have the same types"
    );

    static_assert(
        std::is_same<decltype(cb1), int(*)(unsigned)>::value,
        "cb1 and cb2 should be of type int(*)(unsigned)"
    );

    // Both should be callable now (order reversed to ensure independence)
    CHECK(cb2(100) == base2 + 100); // frees one slot
    CHECK(cb1(50)  == base1 + 50);  // frees second slot

    // After both were invoked (single-shot), both slots are free again; allocate a third
    auto cb3 = makeCallback(base3);
    CHECK(cb3(7) == base3 + 7); // frees one slot

    // Allocate two new callbacks again in reverse order to test reuse
    auto cb4 = makeCallback(base2 + 1000);
    auto cb5 = makeCallback(base1 + 2000);
    CHECK(cb4(1) == base2 + 1000 + 1);
    CHECK(cb5(2) == base1 + 2000 + 2);
}

namespace {
    class AdditionalTag {};
}

TEST_CASE("InstantCallback multi-shot lifetime management with CallbackExtendLifetime"){
    SUBCASE("Persistent multi-shot without Dispose panics on second allocation (reservedCount=1)"){
        int iterations = 0;
        CHECK_THROWS_AS(
            [&]{
                for( ; iterations < 2; ++iterations ){
                    // Same lambda expression site => same lambda type => same pool
                    auto cb = CallbackFrom<1>(
                        [=](CallbackExtendLifetime& lifetime, unsigned arg)->unsigned{
                            // Intentionally never calling lifetime.Dispose(); keeps slot allocated
                            (void)lifetime; // silence unused warning
                            return arg + 1;
                        }
                    );
                    // Even invoking it doesn't free because we never Dispose
                    unsigned r = cb(10);
                    CHECK(r == 11);
                }
            }(),
            TestInstantCallbackPanicException
        );
        CHECK(iterations == 1); // second allocation failed because first never disposed
    }

    SUBCASE("Early Dispose frees slot allowing re-allocation (reservedCount=1)"){
        // First allocation: lambda disposes itself on first call

        auto makeCallback = [](unsigned disposeArg) {
            return CallbackFrom<1>(
                [=](CallbackExtendLifetime& lifetime, unsigned arg)->unsigned{
                    if( arg == disposeArg ){
                        lifetime.Dispose(); // free slot after this call
                    }
                    return arg + 10;
                }
            );
        };

        auto cb1 = makeCallback(111);
        CHECK( cb1(111) == 121 ); // triggers Dispose (111 + 10)

        // Second allocation with the SAME lambda expression site (same type) must succeed now
        auto cb2 = makeCallback(222);
        CHECK( cb2(222) == 232 ); // also disposes

        // Third allocation again (slot reused twice already) to ensure reuse works repeatedly
        auto cb3 = makeCallback(333);
        CHECK( cb3(333) == 343 );
    }

    SUBCASE("Two persistent multi-shot occupy both slots; third allocation panics (reservedCount=2)"){
        int iterations = 0;
        CHECK_THROWS_AS(
            [&]{
                for( ; iterations < 3; ++iterations ){
                    auto cb = CallbackFrom<2>(
                        [=](CallbackExtendLifetime& lifetime, unsigned arg)->unsigned{
                            (void)lifetime; // never Dispose => persistent
                            return arg + 7 + iterations; // use iterations to differ return
                        }
                    );
                    unsigned expect = 100 + 7 + iterations;
                    CHECK( cb(100) == expect );
                    // Still persistent after call
                }
            }(),
            TestInstantCallbackPanicException
        );
        CHECK(iterations == 2); // third allocation failed
    }

    SUBCASE("Dispose one of two persistent; can allocate new third (reservedCount=2)"){
        // Allocate first (will stay persistent until a specific arg)
        auto makeCallback = [](unsigned disposeArg) {
            return CallbackFrom<2>(
                [=](CallbackExtendLifetime& lifetime, unsigned arg)->unsigned{
                    if( arg == disposeArg ){
                        lifetime.Dispose(); // free slot after this call
                    }
                    // just to differ return values (this also checks memory usage)
                    return arg + (disposeArg / 10);
                }
            );
        };
        auto cb1 = makeCallback(500);
        CHECK( cb1(100) == 150 ); // not disposed yet, slot still used

        // Allocate second persistent
        auto cb2 = makeCallback(600);
        CHECK( cb2(150) == 210 ); // not disposed yet, slot still used

        // Free first by calling with disposing arg
        CHECK( cb1(500) == 550 ); // now cb1 disposed; one slot free

        // Allocate new third callback (should reuse freed slot)
        auto cb3 = makeCallback(700);
        CHECK( cb3(1230) == 1300 ); // not disposed yet

        // Dispose second and third now
        CHECK( cb2(600) == 660 ); // dispose second
        CHECK( cb3(700) == 770 ); // dispose third

        // After disposing both, we can allocate two fresh again without panic
        auto cb4 = makeCallback(420);
        auto cb5 = makeCallback(430);
        CHECK( cb4(420) == 462 ); // disposes
        CHECK( cb5(430) == 473 ); // disposes
    }

    SUBCASE("Different lambda expression sites have independent pools; same lambda type conflicts when persistent"){
        /*NOTE: any way only temporaries shall be supported by current CallbackFrom (this is intended)
                entire test is artificial, supporting only exotic corner cases,
                in normal scenarios only unique lambda literals shall be used (always unique) */

        unsigned numCallsA = 0;
        unsigned numCallsB = 0;

        // Create a named lambda (expression site A) so we can reuse its type exactly via decltype.
        auto persistentLambdaTypeA = [&](CallbackExtendLifetime& lifetime, unsigned arg) -> unsigned{
            (void)lifetime; // never Dispose => persistent
            ++numCallsA;
            return arg + 1; // distinguish result
        };
        using PersistentAType = decltype(persistentLambdaTypeA);

        // Create another instance of the SAME lambda type (PersistentAType) => cannot allocate (pool full)
        PersistentAType anotherInstanceSameType1 = persistentLambdaTypeA; // copy construct new callable of same type
        PersistentAType anotherInstanceSameType2 = persistentLambdaTypeA; // copy construct new callable of same type

        // Allocate first persistent of type A (reservedCount=1 for this type's pool)
        auto cbTypeA1 = CallbackFrom<1>(static_cast<PersistentAType&&>(persistentLambdaTypeA));

        // Allocate a different persistent lambda (expression site B) => different type => its own pool
        auto cbTypeB1 = CallbackFrom<1>(
            [&](CallbackExtendLifetime& lifetime, unsigned arg)->unsigned{
                (void)lifetime; // persistent
                ++numCallsB;
                return arg + 2; // different result
            }
        );

        // Both callbacks callable; neither frees its slot (no Dispose)
        CHECK( cbTypeA1(10) == 11 );
        CHECK( cbTypeB1(10) == 12 );

        //Trying to allocate another of type A (same type as first) must fail (pool full)
        CHECK_THROWS_AS(
            (void)CallbackFrom<1>(static_cast<PersistentAType&&>(anotherInstanceSameType1)),
            TestInstantCallbackPanicException
        );

        //Trying to allocate another but with tag shall succeed
        CHECK_NOTHROW(
            (void)CallbackFrom<1, AdditionalTag>(static_cast<PersistentAType&&>(anotherInstanceSameType2))
        );

        // Check call counts
        CHECK(numCallsA == 1);
        CHECK(numCallsB == 1);
    }
}

