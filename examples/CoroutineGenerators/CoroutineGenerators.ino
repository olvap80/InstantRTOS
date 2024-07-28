#include "InstantCoroutine.h"

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

//Create functor class named Range for range producing coroutine
//(yes, it is possible for a coroutine to be template, it is just
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

    //NOTE: next iteration will start a new range again from scratch
    //      but sequenceOfSquares will continue where is was stopped previously
    delay( 2000 );
}
