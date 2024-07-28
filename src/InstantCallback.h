/** @file InstantCallback.h
 @brief Turn callable (lambda) into simple function pointer (callback),
        to allow using of C++ lambda via autogenerated "trampoline"
        when "plain function pointer"/callback is required,
        handy lambda as callback without any dynamic allocation!!!

(c) see https://github.com/olvap80/InstantRTOS

Uses deterministic block pool for turning capturing lambda into
a "simple function pointer"/callback via trampoline.
Function pointer/callback is required by many "C style" API and usually
there is no way to provide additional state with such callback.

No dependencies at all, can be used as separate file. 

Usage sample:
 @code
    //Some demo API
    void CallAfterMinute(void (*rawFunctionPointer)());
    ...

    std::string message = ...
    CallAfterMinute(CallbackFrom<1>(
        [=]{
            //this lambda is able to see captured message
            DoSomething(message);
        }
    ));
 @endcode

The reservedCount template parameter determines the maximum number of
simultaneous trampolines possible in that place for that kind of lambda.
In the sample above it means number of THE SAME(!) lambdas pending 
in CallAfterMinute simultaneously before corresponding lambda is executed).
Choose value for reservedCount depending of your expected usage
of specific lambda (reentrancy/multiple usages/multitasking).
Template produces unique pool of trampolines per unique lambda type.
CallbackFrom panics if number of reserved places is exceeded.
For Arduino UNO there are 15 bytes of code per each new reservedCount item
starting form 4bytes of RAM per additional lambda (when compiler inlines lambda) 

REMEMBER: one shall not use trampolines for lambdas without captures(!),
          since in standard C++ any non-capturing lambda is convertible to
          a "simple plain function pointer" (callback) with compatible signature
          and no trampolines/block pools are needed in that case!

Internally lambda is moved (!) into autogenerated block pool, specially reserved
for that kind of lambda. "Single shot" trampoline is created by default,
this means created callback/trampoline shall be used (called) only once.
Use CallbackExtendLifetime& extra parameter to control trampoline lifetime
manually from the lambda 
 @code
    //Some demo API
    void IterateItems(void (*rawFunctionPointer)(Item* item, bool isLastItem));
    ...

    std::string message = ...
    CallAfterMinute(CallbackFrom<1>(
        [=](
            CallbackExtendLifetime& lifetime,   // additional parameter
            Item& item, bool isLastItem         // original parameters
        ){
            if( isLastItem ){
                lifetime.Dispose(); //lambda exists till is exited
            }

            //this lambda is able to see captured message
            DoSomething(item, message);
        }
    ));
 @endcode
Here lambda deallocates self as soon as it is obvious that it was the last call
(here the "last call" means source shall not call the same trampoline any more!
calling the same trampoline again once is was "disposed" leads to undefined
behavior, so before calling to CallbackExtendLifetime::Dispose() 
one must be 100% sure current call is the last one)

NOTE: use ExecutableQueue from InstantQueue.h instead of sending 
      function pointers allocated via CallbackFrom!
      Prefer Delegate from InstantDelegate.h for the case of
      referencing existing objects (no need to guess reservedCount,
      unless it is your design choice to use "simple plain function pointer")).
      By the contrast The InstantCallback approach is the best design choice
      for situations when "plain simple function pointer" is required
      by some "3rd party" API that you do dot wish to change))


# Compare "single shot" vs CallbackExtendLifetime (more details)

The CallbackExtendLifetime for creating long life callbacks
and their contrast with "single shot" callback is illustrated below
 @code
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
 @endcode

NOTE: Above here callback is called immediately for demo purposes,
      in general case callback to lambda mapping is preserved.
      Remember that capturing by move via [=, wrap=static_cast<Wrap&&>(wrap)]
      will allow moving instead of copying to lambda in C++14,
      see https://en.cppreference.com/w/cpp/language/lambda ))

Here is corresponding output grouped and commented:
 @code
    ...
    Iteration ===============================
    Wrap Constructor for val=241 at 2298 //value to be captured created

    Wrap Copy for val=241 at 2296 //value is copied into lambda argument
    Wrap Move from 2296 to  val=241 at 460 //lambda is moved to allocation storage

    ENTER invoke_simple_callback
    Wrap Move from 460 to  val=241 at 2284 //lambda is moved to copy on stack before call
    Wrap Destructor for val=10241 at 460 //allocation storage is freed, one can reuse it
    Lambda-1 called var=7, wrap=241, arg=1000 //lambda is called (and can reuse allocation)
    Wrap Destructor for val=241 at 2284 //lambda copy is not needed (destructed)
    res=1248
    LEAVE invoke_simple_callback

    Wrap Destructor for val=10241 at 2296 //lambda argument is destructed

    Wrap Copy for val=42 at 2296 //value is copied into lambda argument
    Wrap Move from 2296 to  val=42 at 454 //lambda is moved to allocation storage


    ENTER invoke_multiple
    Lambda-2 called var=9, wrap=42, arg=2000 //lambda is called
    res=2051
    Lambda-2 called var=9, wrap=42, arg=3000 //lambda is called
    res=3051
    Lambda-2 called var=9, wrap=42, arg=4000 //here lambda will mark self for removal
    Wrap Destructor for val=42 at 454 //allocation storage is freed, one can reuse it
    res=4051
    LEAVE invoke_multiple

    Wrap Destructor for val=10042 at 2296 //lambda argument is destructed

    Wrap Destructor for val=42 at 2298 //value to be captured destructed
    ...
 @endcode


NOTE: InstantCallback.h is configurable for interrupt (thread) safety.
      It is always safe to use the same callback from the same thread,
      and no special actions are needed when CallbackFrom happens from 
      the same thread as following call to callback API
      (different callbacks used from different threads will work as well).
      It is safe to use the same callback from different threads/interrupts
      only if that interrupt (thread) safety is configured.


# Design Notes

This can be seen as static deterministic allocation (preallocated block pool),
Each lambda has unique type according to C++ standard,
    https://stackoverflow.com/questions/34596254/do-lambdas-have-different-types
so it is safe to specify reservedCount for each lambda individually
as lambda will be unique (reservedCount per each lambda, not per signature!).
For the case if "something else callable" needs to be mapped,
there is optional AdditionalOptionalTag to make unique allocation for
non-lambda types (exotic case, ignore it when you always use anonymous lambda).


Converting capturing lambdas to callbacks (simple functions, trampolines)
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

#ifndef InstantCallback_INCLUDED_H
#define InstantCallback_INCLUDED_H

//______________________________________________________________________________
// Configurable error handling and interrupt safety

/* Common configuration to be included only if available
   (you can separate file and/or configure individually
    or just skip that to stick with defaults) */
#if defined(__has_include) && __has_include("InstantRTOS.Config.h")
#   include "InstantRTOS.Config.h"
#endif

#ifndef InstantCallback_Panic
#   ifdef InstantRTOS_Panic
#       define InstantCallback_Panic() InstantRTOS_Panic('B')  
#   else
#       define InstantCallback_Panic() /* you can customize here! */ do{}while(true)
#   endif
#endif

#ifndef InstantCallback_EnterCritical
#   if defined(InstantRTOS_EnterCritical) && !defined(InstantCallback_SuppressEnterCritical)
#       define InstantCallback_EnterCritical InstantRTOS_EnterCritical
#       define InstantCallback_LeaveCritical InstantRTOS_LeaveCritical
#       if defined(InstantRTOS_MutexObjectType)
#           define InstantCallback_MutexObjectType InstantRTOS_MutexObjectType
#           define InstantCallback_MutexObjectVariable InstantRTOS_MutexObjectVariable
#       endif
#   else
#       define InstantCallback_EnterCritical
#       define InstantCallback_LeaveCritical
#   endif
#endif


//______________________________________________________________________________
// Handle C++ versions (just skip to "Classes for handling tasks" below))

#if defined(__cplusplus)
#   if __cplusplus >= 201703L
#       if __cplusplus >= 202002L
#           define InstantCallbackNodiscard(explainWhy) [[nodiscard(explainWhy)]]
#       else
#           define InstantCallbackNodiscard(explainWhy) [[nodiscard]]
#       endif
#   else
#       ifdef __GNUC__
#           define InstantCallbackNodiscard(explainWhy) __attribute__((warn_unused_result))
#       elif defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#           if _MSVC_LANG >= 202002L
#               define InstantCallbackNodiscard(explainWhy) [[nodiscard(explainWhy)]]
#           else
#               define InstantCallbackNodiscard(explainWhy) [[nodiscard]]
#           endif
#       endif
#   endif
#endif

#if !defined(InstantCallbackNodiscard)
#       define InstantCallbackNodiscard(explainWhy)
#endif


//______________________________________________________________________________
// Some "must have" internal stuff before declaring API (just skip that!!!))

namespace InternalTrampolineDetails{
    /// Meta function to extract types and code from LambdaType::operator()
    template<class MethodSignature>
    struct ConvertSignature;

    /// Signature for trampoline obtained directly from lambda
    template<class LambdaType>
    using SimpleSignatureFromLambda = typename
        ConvertSignature<decltype(&LambdaType::operator())>::SimpleSignature;
}


//______________________________________________________________________________
// Manage trampoline creation and their lifetime


///Turn lambda to function pointer by storing lambda in block pool
/** Create "single shot" trampoline by default
(trampoline deallocates automatically after first call, 
 thus freeing one item reservedCount)
For usage sample see explanations at beginning of the file.
NOTE: AdditionalOptionalTag can be used for "nonunique" types, 
      when "something else" then lambda is passed to CallbackFrom
      in several different places.
In your usual case you pass only reservedCount to CallbackFrom  
See also https://en.cppreference.com/w/cpp/language/function_template
         section Template argument deduction */
template<unsigned reservedCount, class AdditionalOptionalTag = void, class LambdaType>
InstantCallbackNodiscard(
    "One shall use and call result of CallbackFrom or else memory is lost forever"
)
auto CallbackFrom(LambdaType&& lambda) -> 
    InternalTrampolineDetails::SimpleSignatureFromLambda<LambdaType>*;



/// Extra parameter to lambda change default allocation behavior in CallbackFrom
/** By default trampolines are "single shot", they "deallocate" as soon 
as are called, so it is possible to subscribe only to those API that call lambda
For usage sample see explanations at beginning of the file.
See CallbackFrom */
class CallbackExtendLifetime{
public:
    /// No way to copy (is passed by reference to lambda)
    CallbackExtendLifetime(const CallbackExtendLifetime&) = delete;
    /// No way to copy (is passed by reference to lambda)
    CallbackExtendLifetime& operator=(const CallbackExtendLifetime&) = delete;

    /// Cause trampoline API to be freed
    /** Call from inside of lambda to cause "this lambda" to untie from callback,
    (this means resource limited by reservedCount can be reused again,
     and this also means calling lambda must make sure that the old caller 
     will not issue the same trampoline any more) */
    void Dispose();

    ///Test corresponding trampoline is disposed (freed)
    bool IsDisposed() const;

protected:
    /// Created by corresponding apply
    CallbackExtendLifetime(){}

private:
    /// True if lambda wands to dispose (free trampoline) 
    bool isDisposed = false;
};


//TODO: combinations with thenable??
//TODO: what about object + method with delegate? NO, use lambda as unique!


//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################


//______________________________________________________________________________
// Get to placement new


/* Keep promise to not use any standard libraries by default, 
   but types from those header still must have */
#if defined(InstantRTOS_USE_STDLIB) || defined(__has_include)

#   if __has_include(<cstddef>)
#       include <cstddef>
        using std::size_t;
#       define INSTANTTRAMPOLINE_SIZE_T size_t
#   else
#       include <stddef.h>
#       define INSTANTTRAMPOLINE_SIZE_T size_t
#   endif

#   if __has_include(<cstdint>)
#       include <cstdint>
        //remember uintptr_t is optional per https://en.cppreference.com/w/cpp/types/integer
        using std::uintptr_t;
#       define INSTANTTRAMPOLINE_UINTPTR_T uintptr_t
#   else
#       include <stdint.h>
#       define INSTANTTRAMPOLINE_UINTPTR_T uintptr_t
#   endif

#   if __has_include(<new>)
        //this header is present even on avr
#       include <new>
        //hack to replace class for the case when custom placement new is not needed
#       define InstantCallbackPlaceholderHelper(ptr) ptr
#   else
#       include <new.h>
        //hack to replace class for the case when custom placement new is not needed
#       define InstantCallbackPlaceholderHelper(ptr) ptr
#   endif

#endif


#if !defined(INSTANTTRAMPOLINE_SIZE_T)
#   define INSTANTTRAMPOLINE_SIZE_T unsigned
    static_assert(
        sizeof(INSTANTTRAMPOLINE_SIZE_T) == sizeof( sizeof(INSTANTTRAMPOLINE_SIZE_T) ),
        "The INSTANTTRAMPOLINE_SIZE_T shall have the same size as the result of sizeof"
    );
#endif
#if !defined(INSTANTTRAMPOLINE_UINTPTR_T)
#   define INSTANTTRAMPOLINE_UINTPTR_T unsigned
    static_assert(
        sizeof(INSTANTTRAMPOLINE_UINTPTR_T) >= sizeof( void* ),
        "The INSTANTTRAMPOLINE_UINTPTR_T shall be large enough to hold pointer"
    );
#endif

#if !defined(InstantCallbackPlaceholderHelper)
    /// Helper class to allow custom placement new without conflicts 
    class InstantCallbackPlaceholderHelper{
    public:
        InstantCallbackPlaceholderHelper(void *placeForAllocation) : ptr(placeForAllocation) {} 
    private:
        void *ptr;
        friend void* operator new(INSTANTTRAMPOLINE_SIZE_T, InstantCallbackPlaceholderHelper place) noexcept; 
    };

    /// own placement new implementation 
    /** see https://en.cppreference.com/w/cpp/memory/new/operator_new */
    inline void* operator new(INSTANTTRAMPOLINE_SIZE_T, InstantCallbackPlaceholderHelper place) noexcept{
        return place.ptr;
    }
#endif


//______________________________________________________________________________
// Actual trampoline implementation

inline void CallbackExtendLifetime::Dispose(){
    isDisposed = true; //called by "the same thread", no protection needed
}

inline bool CallbackExtendLifetime::IsDisposed() const{
    return isDisposed; //called by "the same thread", no protection needed
}


namespace InternalTrampolineDetails{

    template<class Res, class LambdaType, class... Args>
    struct ConvertSignature<Res(LambdaType::*)(Args...)>{
        using SimpleSignature = Res(Args...);
    };
    template<class Res, class LambdaType, class... Args>
    struct ConvertSignature<Res(LambdaType::*)(Args...) const>{
        using SimpleSignature = Res(Args...);
    };

    template<class Res, class LambdaType, class... Args>
    struct ConvertSignature<Res(LambdaType::*)(CallbackExtendLifetime&, Args...)>{
        using SimpleSignature = Res(Args...);
    };
    template<class Res, class LambdaType, class... Args>
    struct ConvertSignature<Res(LambdaType::*)(CallbackExtendLifetime&, Args...) const>{
        using SimpleSignature = Res(Args...);
    };


    /// Access for apply for creating CallbackExtendLifetime 
    class CallbackExtendLifetimeImpl: public CallbackExtendLifetime{
    public:
        /// Make constructor visible for apply
        CallbackExtendLifetimeImpl(){}
    };

    /// General node for lambda allocation (chain of nodes is generated in LambdaListNodeGenerator)
    template<class LambdaType, class AdditionalOptionalTag, void (*freeMe)(void*)>
    struct LambdaListNode{
        /// Signature for corresponding "simple function pointer"
        using SimpleSignature = SimpleSignatureFromLambda<LambdaType>;

        /// Create general allocation node
        LambdaListNode(
            SimpleSignature* individualTrampolineGenerated,
            LambdaListNode* nextNode
        )
            : individualTrampoline(individualTrampolineGenerated), next(nextNode) {}
        
        /// Destructor to do nothing (assume there is no lambda)
        ~LambdaListNode(){}

        union{
            /// Hold corresponding "simple function pointer"/callback when is free
            SimpleSignature* individualTrampoline;
            
            /// Lambda being wrapped when is allocated
            LambdaType lambda;
        };

        /// Link to next node for lambda
        LambdaListNode* next;

        /// Only the one who instantiated or is aware how to free that node
        void Free(){
            freeMe(this);
        }
    };


    /// Meta function to obtain code for LambdaType::operator()
    template<class LambdaListNodeType, class MethodSignature>
    struct ObtainSimpleFunction;

    /// Specialization to obtain code for LambdaType::operator()
    template<class LambdaListNodeType, class Res, class LambdaType, class... Args>
    struct ObtainSimpleFunction<LambdaListNodeType, Res(LambdaType::*)(Args...)>{
        template<LambdaListNodeType* obj>
        struct For{
            static Res apply(Args... args){
                // copy is needed to allow recursive allocation
                auto copyOfLambda = static_cast<LambdaType&&>(obj->lambda);
                // corresponding lambda moved from but still needs to be destructed
                obj->lambda.~LambdaType();
                // remember simple function again, to be used next time
                obj->individualTrampoline = apply;
                // free corresponding node
                obj->Free();
                //call the copy
                return copyOfLambda(static_cast<Args&&>(args)...);
            }
        };
    };
    /// Specialization to obtain code for LambdaType::operator() const
    template<class LambdaListNodeType, class Res, class LambdaType, class... Args>
    struct ObtainSimpleFunction<LambdaListNodeType, Res(LambdaType::*)(Args...) const>:
        public ObtainSimpleFunction<LambdaListNodeType, Res(LambdaType::*)(Args...)> //reuse existing
    {};

    /// Specialization to obtain code for lambda that controls own lifetime
    template<class LambdaListNodeType, class Res, class LambdaType, class... Args>
    struct ObtainSimpleFunction<LambdaListNodeType, Res(LambdaType::*)(CallbackExtendLifetime&, Args...)>{
        template<LambdaListNodeType* obj>
        struct For{
            static Res apply(Args... args){
                CallbackExtendLifetimeImpl extendLifetime;
                Res res = obj->lambda(extendLifetime, static_cast<Args&&>(args)...);
                
                if( extendLifetime.IsDisposed() ){
                    // lambda explicitly schedules disposal
                    obj->lambda.~LambdaType();
                    // remember simple function again, to be used next time
                    obj->individualTrampoline = apply;
                    // free corresponding node
                    obj->Free();
                }

                return res;
            }
        };
    };
    /// Specialization to obtain code for void lambda that controls own lifetime
    template<class LambdaListNodeType, class LambdaType, class... Args>
    struct ObtainSimpleFunction<LambdaListNodeType, void(LambdaType::*)(CallbackExtendLifetime&, Args...)>{
        template<LambdaListNodeType* obj>
        struct For{
            static void apply(Args... args){
                CallbackExtendLifetimeImpl extendLifetime;
                obj->lambda(extendLifetime, static_cast<Args&&>(args)...);
                
                if( extendLifetime.IsDisposed() ){
                    // lambda explicitly schedules disposal
                    obj->lambda.~LambdaType();
                    // remember simple function again, to be used next time
                    obj->individualTrampoline = apply;
                    // free corresponding node
                    obj->Free();
                }
            }
        };
    };
    /// Specialization to obtain code for lambda that controls own lifetime
    template<class LambdaListNodeType, class Res, class LambdaType, class... Args>
    struct ObtainSimpleFunction<LambdaListNodeType, Res(LambdaType::*)(CallbackExtendLifetime&, Args...) const>:
        public ObtainSimpleFunction<LambdaListNodeType, Res(LambdaType::*)(CallbackExtendLifetime&, Args...)>
    {};

    /// Meta function to obtain code for LambdaType
    template<class LambdaListNodeType, class LambdaType, LambdaListNodeType* obj>
    using ObtainSimpleFunctionFor = typename
        ObtainSimpleFunction<LambdaListNodeType, decltype(&LambdaType::operator())>::template For<obj>;


    /// Meta function to generate linked list of LambdaListNode and corresponding trampolines
    template<class LambdaType, class AdditionalOptionalTag, void (*freeMe)(void*), unsigned reservedCount>
    struct LambdaListNodeGenerator;

    /// Generate source for individual caller (all except first)
    template<class LambdaType, class AdditionalOptionalTag, void (*freeMe)(void*), unsigned reservedCount>
    struct LambdaListNodeGenerator{
        /// Node to store corresponding lambda
        static LambdaListNode<LambdaType, AdditionalOptionalTag, freeMe> node;
    };
    /// Generate source for individual caller (the first one)
    template<class LambdaType, class AdditionalOptionalTag, void (*freeMe)(void*)>
    struct LambdaListNodeGenerator<LambdaType, AdditionalOptionalTag, freeMe, 1>{
        /// Node to store corresponding lambda
        static LambdaListNode<LambdaType, AdditionalOptionalTag, freeMe> node;
    };

    //The last node storing corresponding lambda is nas no "next" node
    template<class LambdaType, class AdditionalOptionalTag, void (*freeMe)(void*)>
    LambdaListNode<LambdaType, AdditionalOptionalTag, freeMe>
    LambdaListNodeGenerator<LambdaType, AdditionalOptionalTag, freeMe, 1>::node{
        &ObtainSimpleFunctionFor<
            LambdaListNode<LambdaType, AdditionalOptionalTag, freeMe>,
            LambdaType,
            &LambdaListNodeGenerator<LambdaType, AdditionalOptionalTag, freeMe, 1>::node
        >::apply,
        nullptr // no "next" node
    };
    //All the rest of the nodes reference 
    template<class LambdaType, class AdditionalOptionalTag, void (*freeMe)(void*), unsigned reservedCount>
    LambdaListNode<LambdaType, AdditionalOptionalTag, freeMe>
    LambdaListNodeGenerator<LambdaType, AdditionalOptionalTag, freeMe, reservedCount>::node{
        &ObtainSimpleFunctionFor<
            LambdaListNode<LambdaType, AdditionalOptionalTag, freeMe>,
            LambdaType,
            &LambdaListNodeGenerator<LambdaType, AdditionalOptionalTag, freeMe, 1>::node
        >::apply,
        &LambdaListNodeGenerator<LambdaType, AdditionalOptionalTag, freeMe, reservedCount-1>::node
    };

    //keep the promise to not use standard headers))
    template<class T> struct RemoveReference { typedef T type; };
    template<class T> struct RemoveReference<T&> { typedef T type; };
    template<class T> struct RemoveReference<T&&> { typedef T type; };

    /// Allocator encapsulating  
    template<unsigned reservedCount, class AdditionalOptionalTag, class LambdaType>
    class Allocator{
    public:
        /// Freeing node (to be passed as template parameter)
        static void FreeNode(void* rawNode);

        /// Cut off references to ger original lambda type 
        using OriginalLambda = typename InternalTrampolineDetails::RemoveReference<LambdaType>::type;

        /// Type of nodes used for allocation 
        using Node = LambdaListNode<LambdaType, AdditionalOptionalTag, FreeNode>;
        /// Signature for corresponding "simple function pointer"
        using SimpleSignature = typename Node::SimpleSignature;


        /// Allocate item that forwards from simple function to lambda
        template<class UsedLambda>
        static SimpleSignature* Allocate(UsedLambda&& lambda){
            if( !first ){
                InstantCallback_Panic();
            }
            SimpleSignature* res;
            {InstantCallback_EnterCritical
                // extract pointer from union before wiping out with lambda
                res = first->individualTrampoline;
                // now it is possible to store lambda here using move
                new( InstantCallbackPlaceholderHelper(&first->lambda) )
                                    OriginalLambda(
                                        //forward here onto copy
                                        static_cast<UsedLambda&&>(lambda)
                                    );
                // step over allocated item 
                first = first->next;
            InstantCallback_LeaveCritical}
            return res;
        }

    private:
        /// Generator to use for obtaining linked list of nodes 
        using Generator = LambdaListNodeGenerator<LambdaType, AdditionalOptionalTag, FreeNode, reservedCount>;

        /// Head of the list
        static Node* first;

#if defined(InstantCallback_MutexObjectType)
        // additional static member is needed for EnterCritical/LeaveCritical
        static InstantCallback_MutexObjectType InstantCallback_MutexObjectVariable;
#endif
    };

    template<unsigned reservedCount, class AdditionalOptionalTag, class LambdaType>
    typename Allocator<reservedCount, AdditionalOptionalTag, LambdaType>::Node* 
        Allocator<reservedCount, AdditionalOptionalTag, LambdaType>::first =
            &Allocator<reservedCount, AdditionalOptionalTag, LambdaType>::Generator::node;

    template<unsigned reservedCount, class AdditionalOptionalTag, class LambdaType>
    inline void Allocator<reservedCount, AdditionalOptionalTag, LambdaType>::FreeNode(void* rawNode){
        InstantCallback_EnterCritical
            auto node = static_cast<Node*>(rawNode);
            node->next = first;
            first = node;
        InstantCallback_LeaveCritical
    }

#if defined(InstantCallback_MutexObjectType)
    // additional static member is needed for EnterCritical/LeaveCritical
    template<unsigned reservedCount, class AdditionalOptionalTag, class LambdaType>
    InstantCallback_MutexObjectType 
    Allocator<reservedCount, AdditionalOptionalTag, LambdaType>::
                                            InstantCallback_MutexObjectVariable;
#endif
}


template<unsigned reservedCount, class AdditionalOptionalTag, class LambdaType>
InstantCallbackNodiscard(
    "One shall use and call result of CallbackFrom or else memory is lost forever"
)
auto CallbackFrom(LambdaType&& lambda) -> 
    InternalTrampolineDetails::SimpleSignatureFromLambda<LambdaType>*
{
    using OriginalLambda = typename InternalTrampolineDetails::RemoveReference<LambdaType>::type;
    
    // lambda will be moved to internal block pool
    return InternalTrampolineDetails::Allocator<
        reservedCount, AdditionalOptionalTag, OriginalLambda
    >::Allocate(
        //forward here
        static_cast<LambdaType&&>(lambda)
    );
}

#endif