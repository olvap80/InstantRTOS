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
    SUBCASE("Iterative CallbackFrom with the same lambda shall not cause panic"){
        //this is to test if the memory is correctly freed
        //when the lambda is destroyed
        for(int i=0; i<100; i++){
            auto res = invoke_simple_callback(CallbackFrom<1>(
                [=](unsigned arg){
                    return arg;
                }
            ));
            CHECK(res == 1000);
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

