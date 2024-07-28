/* @file InstantCallback
* @brief Show how to turn C++ lambda into a "plain C" callback
*/
#include "InstantCallback.h"

void invoke_simple_callback(
    unsigned (*simpleFunctionPointer)(unsigned arg)
){
    Serial.println(F("\nENTER invoke_simple_callback"));
    unsigned res = simpleFunctionPointer(1000);
    Serial.print(F("res=")); Serial.println(res);
    Serial.println(F("LEAVE invoke_simple_callback\n"));
}

void invoke_multiple(
    unsigned (*simpleFunctionPointer)(unsigned arg)
){
    Serial.println(F("\nENTER invoke_multiple"));
    unsigned res = simpleFunctionPointer(2000);
    Serial.print(F("res=")); Serial.println(res);
    res = simpleFunctionPointer(3000);
    Serial.print(F("res=")); Serial.println(res);
    res = simpleFunctionPointer(4000);
    Serial.print(F("res=")); Serial.println(res);
    Serial.println(F("LEAVE invoke_multiple\n"));
}

/// Class for demo purposes (illustrate how lifetime is managed)
struct Wrap{
    unsigned val = 0;

    Wrap(){
        Serial.println(F("Wrap Default constructor"));
    }
    Wrap(unsigned initialVal): val(initialVal){
        Serial.print(F("Wrap Constructor for")); Println();
    }
    ~Wrap(){
        Serial.print(F("Wrap Destructor for")); Println();
    }

    Wrap(const Wrap& other): val(other.val){
        Serial.print(F("Wrap Copy for")); Println();
    }
    Wrap(Wrap&& other): val(other.val){
        other.val += 10000; //mark "other" as "moved from"
        Serial.print(F("Wrap Move from "));
        Serial.print((unsigned)&other);
        Serial.print(F(" to "));
        Println();
    }
    Wrap& operator=(const Wrap& other){
        val = other.val; 
        Serial.print(F("Wrap Assignment for")); Println();
        return *this;
    }
    Wrap& operator=(Wrap&& other){
        val = other.val;
        other.val += 20000; //mark "other" as "moved from"
        Serial.print(F("Wrap Move assignment for")); Println();
        return *this;
    }
    void Println(){
        Serial.print(F(" val=")); Serial.print(val);
        Serial.print(F(" at ")); Serial.println((unsigned)this);
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println(F("Start ==================================="));
}

void loop() {
    Serial.println(F("\nIteration ==============================="));
    
    unsigned var = rand() & 0xF;
    Wrap wrap = rand() & 0xFF;
    //demo for "single shot" callback
    invoke_simple_callback(CallbackFrom<1>(
        [=](unsigned arg){
            Serial.print(F("Lambda-1 called var=")); Serial.print(var);
            Serial.print(F(", wrap=")); Serial.print(wrap.val);
            Serial.print(F(", arg=")); Serial.println(arg);
            return var + wrap.val + arg;
        }
    ));

    //refresh captured variables for new values
    var = rand() & 0xF;
    wrap.val = rand() & 0xFF;
    //demo for multi shot callback
    invoke_multiple(CallbackFrom<1>(
        [=](
            CallbackExtendLifetime& lifetime,
            unsigned arg
        ){
            if( 4000 == arg ){
                //this will free memory after lambda exits
                lifetime.Dispose();
            }
            Serial.print(F("Lambda-2 called var=")); Serial.print(var);
            Serial.print(F(", wrap=")); Serial.print(wrap.val);
            Serial.print(F(", arg=")); Serial.println(arg);
            return var + wrap.val + arg;
        }
    ));

    delay(1000);
}
