/** @file InstantCoroutine.h
    @brief Simple minimalistic coroutines suitable for all various platforms (like Arduino)
    when native C++ coroutines are too heavyweight (or co_yield and stuff does not work)
    platform independent (does not depend on CPU, only standard C++ is required!)

    (c) see https://github.com/olvap80/InstantRTOS

    Initially created as lightweight coroutines for Arduino (2 bytes of state per coroutine).
    Lightweight coroutines are implemented as easy to use C++ functor classes.
    Use coroutines instead of writing explicit finite state machines.

    Implemented as stackless coroutines, own coroutine state is only two bytes,
    one can add any custom state variables that turn to class fields.

    Example Usage (generate sequences):
    @code

        #include "InstantCoroutine.h"

        //Create functor class named SequenceOfSquares for coroutine producing squares
        CoroutineDefine( SequenceOfSquares ) {
            //coroutine variables (state persisting between CoroutineYield calls)
            int i = 0;

            //specify body of the coroutine
            CoroutineBegin(int)
                for ( ;; ++i){
                    CoroutineYield( i*i );
                }
            CoroutineEnd()
        };

        //Create functor class named Range for range producing coroutine
        template<class T>
        CoroutineDefine( Range ) {
            T current, last;
        public:
            ///Constructor to establish initial coroutine state
            Range(T beginFrom, T endWith)
                : current(beginFrom), last(endWith) {}

            //body of the coroutine that uses this state
            CoroutineBegin(int)
                for(; current < last; ++current){
                    CoroutineYield( current );
                }
                CoroutineStop(last);
            CoroutineEnd()
        };

        //Instantiate squares generator
        SequenceOfSquares sequenceOfSquares;

        void setup() {
            Serial.begin(9600);
        }

        void loop() {
            //establish new range on each iteration
            Range<int8_t> range(10, 20);

            while( range ){
                //"call" to coroutine resumes is till CoroutineYield or CoroutineStop
                auto i = range();
                auto x = sequenceOfSquares();

                Serial.print( i );
                Serial.print( ':' );
                Serial.println( x );
                delay( 200 );
            }
            Serial.println(F("Iteration finished"));

            //NOTE: next iteration will start range from scratch
            //      but sequenceOfSquares will continue where is was stopped previously
            delay( 2000 );
        }

    @endcode

    
    NOTE: CoroutineYield will not function correctly within a switch block,
          do not place CoroutineYield inside switch.
    NOTE: It is strongly recommended to turn on all compiler warnings in preferences!
          Remember: all the variables and state shall be outside of coroutine body,
                    just declare them before coroutine body
                    (between CoroutineDefine( Range ) { and CoroutineBegin), 
                    they will turn to class fields!

    The main idea is inspired by https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
    and https://forum.arduino.cc/t/arduino-coroutines-are-they-useful/36819
    coroutine state is saved as class fields (see sample below).

    This can serve as C++ coroutines for embedded platforms (like Arduino)

    On why "native C++20 coroutines" are not suitable for embedded, see:
    https://probablydance.com/2021/10/31/c-coroutines-do-not-spark-joy/
    (remember InstantCoroutine works in C++11 which is default for Arduino))

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

#ifndef InstantCoroutine_INCLUDED_H
#define InstantCoroutine_INCLUDED_H


//NOTE: coroutine used after completion also falls here
#ifndef InstantCoroutine_Panic
#   ifdef InstantCoroutine_Panic
#       define InstantCoroutine_Panic() InstantRTOS_Panic("C")  
#   else
#       define InstantCoroutine_Panic() /* you can customize here! */ do{}while(true)
#   endif
#endif


///Define class for coroutine
/** Class body shall follow in {}, see sample above
 * NOTE: all fields are private by default!
 * NOTE: one can place fields (and constructors!) inside {}, see sample above */
#define CoroutineDefine(coroutineClassName) \
    class coroutineClassName: public CppCoroutineBase<COROUTINE_PLACE_COUNTER>

/// Coroutine body follows this macro (coroutine is initially suspended!)
/** One shall specify return type or void as the first parameter,
 * other parameters can optionally specify resume parameters for the coroutine
 * (they have to be passed to operator() in coroutine on each resume).
 * REMEMBER: CoroutineEnd must end the coroutine body
 * NOTE: do not declare local variables inside body */
#define CoroutineBegin(coroutineResultType, ...) \
    public: \
        /* Just call coroutine object with () to resume the coroutine */ \
        coroutineResultType operator()(__VA_ARGS__) { \
            switch( cppCoroutineState ){ \
                case CppCoroutineStateInitial:;

/// Yield to caller (with value if coroutineResultType was not void)
/** Remember state and suspend execution
 *  (next call to operator() will resume from the same place)
 *  NOTE: one cannot place CoroutineYield inside switch statement
 *  NOTE: one can specify return values for resumable coroutines */
#define CoroutineYield(...) \
    COROUTINE_YIELD_FROM(COROUTINE_PLACE_COUNTER, __VA_ARGS__)

///The "the last return" from the coroutine
/** Once stopped it will be not resumable again */
#define CoroutineStop(lastRVal) \
    do { cppCoroutineState = CppCoroutineStateFinal; return lastRVal; } while (false)

/// End coroutine body (corresponds to CoroutineBegin)
#define CoroutineEnd() \
                default: InstantCoroutine_Panic(); \
            } \
        } \
    public:



//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################



///Common base for all coroutine classes
/** NOTE: copying is not banned, copy will duplicate entire state
 *        let uses decide if he wants lifetime to be copied */
template<int stateStartsFrom>
class CppCoroutineBase{
    public:
        ///Test coroutine did not finish (CoroutineStop was NOT called)
        operator bool() const {
            return !Finished();
        }

        ///Test coroutine finished (CoroutineStop was called)
        bool Finished() const {
            return CppCoroutineStateFinal == cppCoroutineState;
        }

    protected:
        ///Type for state
        /** Assume our __LINE__ will never go beyond */
        using CppCoroutineStateHolder = short;
        ///Maximum state for static_assert ))
        /** Compute assuming two's complement */
        static const CppCoroutineStateHolder CppCoroutineStateHolderMax =
            ((1 << (sizeof(CppCoroutineStateHolder)*8 - 2)) - 1) * 2 + 1;

        ///Always start from this state
        static const CppCoroutineStateHolder CppCoroutineStateInitial = stateStartsFrom;
        ///The final state we cannot continue from
        static const CppCoroutineStateHolder CppCoroutineStateFinal = CppCoroutineStateInitial-1;

        ///State where that coroutine is paused/stopped
        CppCoroutineStateHolder cppCoroutineState = CppCoroutineStateInitial;

        ///For use from derived classes only
        CppCoroutineBase() = default;
};


///Ensure macro argument is expanded before being used
/** See https://gcc.gnu.org/onlinedocs/cpp/Argument-Prescan.html
 * and https://gcc.gnu.org/onlinedocs/cppinternals/Macro-Expansion.html */
#define COROUTINE_EXPANd_MACRO(macro) macro

///Helper macro to form
#define COROUTINE_YIELD_FROM(cppCoroutine_place_id, ...) \
    do{ \
        static_assert(cppCoroutine_place_id > CppCoroutineStateInitial, "COROUTINE_PLACE_COUNTER is broken"); \
        static_assert(cppCoroutine_place_id < CppCoroutineStateHolderMax, "Mapping lines to state is broken"); \
        cppCoroutineState = cppCoroutine_place_id; \
        return __VA_ARGS__; \
        case cppCoroutine_place_id:; \
    } while (false)


//define macro to obtain sequential numbers (optimizes switch statement a lot!)
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__) || defined(__IAR_SYSTEMS_ICC__)
    // see https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
    // and https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
    // and https://nanxiao.me/en/__counter__-macro-in-gcc-clang/
    // and https://wwwfiles.iar.com/arm/webic/doc/EWARM_DevelopmentGuide.ENU.pdf

#   define COROUTINE_PLACE_COUNTER COROUTINE_EXPANd_MACRO(__COUNTER__)
#else
#   define COROUTINE_PLACE_COUNTER COROUTINE_EXPANd_MACRO(__LINE__)
#endif

#endif
