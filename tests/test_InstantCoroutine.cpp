/** @file tests/test_InstantCoroutine.cpp
  * @brief Unit tests for InstantCoroutine.h
  */

#include <exception>
/// Custom exception for testing InstantCoroutine_Panic
class TestInstantCoroutinePanicException: public std::exception{
    const char* what() const noexcept override{
        return "TestInstantCoroutinePanicException";
    }
};
//Header will see this definition
#define InstantCoroutine_Panic() throw TestInstantCoroutinePanicException()
#include "InstantCoroutine.h"

#include "doctest/doctest.h"
#include <cctype>
#include <string>


namespace{

    // Create functor class named SequenceOfSquares for a coroutine 
    // producing squares starting from 0 
    CoroutineDefine( SequenceOfSquares ) {
        //coroutine variables (state persisting between CoroutineYield calls)
        int i = 0;

        //specify body of the coroutine, int here is the type yielded
        CoroutineBegin(int)
            for ( ;; ++i){
                CoroutineYield( i*i );
            }
        CoroutineEnd()
    };

} //namespace

TEST_CASE("InstantCoroutine normal usage SequenceOfSquares"){
    //Instantiate squares generator
    SequenceOfSquares sequenceOfSquares;

    //first 10 squares
    for(int i = 0; i < 10; ++i){
        auto x = sequenceOfSquares();
        CHECK( x == i*i );

        //Such coroutine "never ends" 
        CHECK( sequenceOfSquares );
    }

    //next 10 squares
    for(int i = 10; i < 20; ++i){
        auto x = sequenceOfSquares();
        CHECK( x == i*i );

        //Such coroutine "never ends" 
        CHECK( sequenceOfSquares );
    }
}


namespace{

    //Create functor class named Range for range producing coroutine
    //(yes, it is possible for a coroutine to be a template, it is just
    // a class that can be instantiated multiple times!)
    template<class T>
    CoroutineDefine( Range ) {
        T current, last;
    public:
        ///Constructor to establish initial coroutine state
        Range(T beginFrom, T endWith) : current(beginFrom), last(endWith) {}

        //body of the coroutine that uses this state
        CoroutineBegin(T)
            for(; current < last; ++current){
                CoroutineYield( current );
            }
            CoroutineStop(last);
        CoroutineEnd()
    };

} //namespace

TEST_CASE("InstantCoroutine normal usage Range"){
    //Instantiate squares generator
    Range<int8_t> range(10, 20);

    SUBCASE("first 10 starting from 10"){
        for(int i = 10; i < 20; ++i){
            auto x = range();
            CHECK( x == i );
        }
        //the last value is returned by CoroutineStop
        CHECK( range() == 20 );

        //Such coroutine that reached CoroutineStop shall be marked as "ended"
        CHECK( !range );
        //Once "ended" such coroutine will rise an exception if called
        CHECK_THROWS_AS(
            [&]{ range(); }(),
            TestInstantCoroutinePanicException
        );
    }
    SUBCASE("Range as iterator"){
        int pos = 10;
        while( range ){
            auto x = range();
            CHECK( x == pos++ );
        }
        /*the last value returned by the coroutine was 20
          (that was what CoroutineStop returns) */
        CHECK( pos == 21 );

        //Such coroutine that reached CoroutineStop shall be marked as "ended"
        CHECK( !range );
        //Once "ended" such coroutine will rise an exception if called
        CHECK_THROWS_AS(
            [&]{ range(); }(),
            TestInstantCoroutinePanicException
        );
    }
}


TEST_CASE("InstantCoroutine two simultaneously"){
    //Range and SequenceOfSquares shall go in sync
    Range<int> range(0, 17);
    SequenceOfSquares sequenceOfSquares;
    while( range ){
        auto x = range();
        CHECK( x*x == sequenceOfSquares() );
    }
}


namespace{
//Additional sample inspired by https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html

    CoroutineDefine( Parser ) {
    public:
        std::string accumulator;

        CoroutineBegin(void, int c)
            for (;;) {
                if( std::isalpha(c) ){
                    do {
                        accumulator.push_back(char(c));
                        CoroutineYield(); //get next char here
                    } while( std::isalpha(c) );
                    accumulator += "[WORD DETECTED]";
                }
                accumulator.push_back(char(c));
                accumulator += "[PUNCT DETECTED]";
                CoroutineYield(); //wait for more chars
            }
        CoroutineEnd()
    } parser;

    CoroutineDefine( Decompressor ) {
        unsigned len = 0;
        CoroutineBegin(void, int c)
            for (;;) {
                if( c == EOF ){
                    CoroutineStop();
                }
                if( c == 0xFF ) { // sign we met repetition
                    CoroutineYield(); // wait for length
                    len = c; //remember c is coroutine parameter here
                    CoroutineYield(); // wait for repeated char to c
                    while (len--){
                        parser(c);
                    }
                }
                else{
                    parser(c);
                }
                CoroutineYield(); //wait for more chars
            }
        CoroutineEnd()
    } decompressor;

} //namespace
 
TEST_CASE("InstantCoroutine normal usage Parser"){
    static const unsigned char testArray[] = "abc def \xFF\x07r d\xFF\x03Ref";

    for(int position = 0; ; ++position ){
        CHECK( decompressor );

        unsigned char current = testArray[position];
        if( !current ){
            decompressor(EOF);
            break;
        }
        else{
            decompressor(current & 0xFF);
        }
    }

    CHECK(
        parser.accumulator ==
        "abc[WORD DETECTED] [PUNCT DETECTED]def[WORD DETECTED] "
        "[PUNCT DETECTED]rrrrrrr[WORD DETECTED] [PUNCT DETECTED]dRRRef"
    );

    //Decompressor did CoroutineStop, so it is marked as "ended"
    CHECK( !decompressor );
    //Once "ended" such coroutine will rise an exception if called
    CHECK_THROWS_AS(
        [&]{ decompressor('a'); }(),
        TestInstantCoroutinePanicException
    );
}
