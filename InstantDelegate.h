/** @file InstantDelegate.h
    @brief Fast deterministic memory pools with immediate allocation,
           suitable for real time (no heap allocation)

    (c) and license see https://github.com/olvap80/InstantRTOS

    Initially created as a lightweight delegates/lambdas for Arduino platform.
    Zero dependencies, by default does not depend even on standard headers!
    Cheap and fast replacement of std::function for Arduino and other platforms.
    Any "callable stuff" can be handled with InstantDelegate library :)
    Suitable for embedded real time applications requiring deterministic behavior.
    
    You can just copy this to your project, it dos not depend on InstantArduino.
    Short sample:
    @code
        //shorthand to not repeat the same signature every time
        using MyCallback = Delegate<int(int)>;

        //using callback by some custom API
        void CustomAPI(const MyCallback& callbackNeeded){
            auto myValue = callbackNeeded(some_int_argument)
            ...
        }
        ...

        //creating callback
        
        //one can make callback from C++ lambda
        CustomAPI( MyCallback([](int val){ return val + 42;}) );

        //and from "simple" function (implicit conversion supported)
        CustomAPI( ordinary_function_with_compatible_signature );
        
        //also one can create Delegate that calls method of some object 
        SomeClass targetObject{42};
        CustomAPI( MyCallback::Bind(targetObject).Method<&SomeClass::some_method>() );

        //and callable things having compatible operator() can also be referenced
        class SomeFunctor{
        public:
            ...
            int operator()(int val) {
                ...
            }
        };
        ...
        SomeFunctor customCallable;
        CustomAPI(customCallable);
    @endcode

    See additional usages covering other use cases in expanded sample below.

    All the Delegate instances with the same signature are "compatible" with each
    other, and can be copied/assigned, passed as parameters, regardless 
    of what kind of "callable" is actually referenced.

    NOTE: The Delegate instance created for C++ lambda
          will not copy that C++ lambda!
          It is responsibility of the developer to ensure
          Delegate is not called after original callable/lambda is destructed


    Additional sample, serves as a demo to cover more possible usages
    as a fast and compact C++ delegates for embedded platforms, like Arduino:
    @code
        ///Sample API for receiving "callable" Delegate as parameter
        void test(const Delegate<int(int)>& delegateToCall){
            Serial.println(F("ENTER test API being called with Delegate["));
            
            int res = delegateToCall(10);
            
            Serial.print(F("]LEAVE test API being called with Delegate"));
            Serial.println(res);
        }

        ///Sample "unbound" function to be called via Delegate
        int function_to_pass_to_Delegate(int val){
            Serial.println(F("function_to_pass_to_Delegate called"));
            return val + 2;
        }

        ///Sample class (to demonstrate functor and method calls)
        class TestClass{
        public:
            TestClass(int addToVal): addTo(addToVal) {}

            ///Demo functor to be tied with delegate
            int operator()(int val) const {
                Serial.println(F("functor operator() called"));
                return val + addTo;
            }

            ///Demo method to be tied with delegate (object, method pair)
            int test_method(int val) const {
                Serial.println(F("method_to_pass_to_Delegate called"));
                return val + addTo;
            }

            ///Demo method to demonstrate usage from free function
            int valueGet() const{
                return addTo;
            }

        private:
            int addTo;
        };

        //one can also create Delegate by binging objects to free functions 
        //(this may surprize you, but the word class is needed below,
        // to make such functions statically bindable at compile time)
        int function_to_bind_receivingRef(const class TestClass& t, int val){
            Serial.println(F("function_to_bind_receivingRef"));
            return t.valueGet() + 100 + val;
        }
        int function_to_bind_receivingPtr(class TestClass* t, int val){
            Serial.println(F("function_to_bind_receivingPtr"));
            return t->valueGet() + 1000 + val;
        }


        ///Setup once execution environment for Arduino board
        void setup() {
            Serial.begin(9600);
        }

        ///Arduino (event) loop
        void loop() {
            Serial.println(F("\n------------- Iteration -------------"));

            //demo of "something callable" being passed to test as Delegate
            TestClass demoCallable{-2};
            test(demoCallable);

            //demo variable to be captured by lambda
            int captured = 1;
            //C++ "long lasting" lambda to be referenced by the Delegate
            auto lambdaAsVariable = [&](int val){
                Serial.println(F("lambdaAsVariable"));
                return val + captured;
            };
            //one can easy call any functor/callable via delegate
            //assuming callable functor lives as long as delegate is called
            test(lambdaAsVariable);

            //Function pointer can be called (implicit conversion, direct constructor is called)
            test(function_to_pass_to_Delegate);

            //inline "non capturing" lambda can be explicitly forced to be Delegate
            test(
                Delegate<int(int)>([](int val){
                    Serial.println(F("inlined lambda"));
                    return val + 3;
                })
            );

            captured = 4;
            //call with inlined lambda is also possible to force with Unstorable
            //(we can do this because we do not save that temporary!)
            test(Delegate<int(int)>::Unstorable([&](int val){
                Serial.println(F("Temporary inlined_lambda (to be called by test, but not stored)"));
                return val + captured;
            }));

            //method pointer for existing object can be created
            const TestClass demoCallable5{5};
            //Note: direct call without delegate is demoCallable5.test_method(42);
            test(
                Delegate<int(int)>::Bind(demoCallable5).Method<&TestClass::test_method>()
            );
            //Ensure method pointer can be created with short syntax
            TestClass demoCallable6{6};
            test(
                Delegate<int(int)>::Bind(&demoCallable6).Method<&TestClass::test_method>()
            );

            //Ensure method pointer can be created from reference
            TestClass demoCallable7{7};
            test(
                Delegate<int(int)>::Bind(demoCallable7).Function<&function_to_bind_receivingRef>()
            );
            //Ensure method pointer can be created with short syntax
            TestClass demoCallable8{8};
            test(
                Delegate<int(int)>::Bind(&demoCallable8).Function<&function_to_bind_receivingPtr>()
            );

            delay(4200);
        }

    @endcode

    The Delegate class acts as the minimal "closure" for referring to
    "callable" items (lambda, functor or instance method) and can be treated
    as a "reference" to some "callable thing" existing "somewhere else".
    Here word "referring" means original content is not copied into that Delegate
    thus instance of the Delegate class acts as a "callable reference" implemented
    by wrapping  (object_pointer, function_pointer) pair.

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

#ifndef InstantDelegate_INCLUDED_H
#define InstantDelegate_INCLUDED_H

///Delegate (callable reference) to "callable thing" without ownership
template<class CallableSignature>
class Delegate;


///Delegate (callable reference) to "callable thing" without ownership
/** One specifies callable signature as a template parameter.
 * Delegate holds a function or reference to "something callable"
 * with the same signature as specified by template parameter.
 * Cheap solution to "funarg problem" (holds both function and pointer)
 * One must ensure referred stuff exists as long as Delegate is called */
template<class Res, class... Args>
class Delegate<Res(Args...)>{
public:
    ///It is not allowed to create empty Delegate (lambda references)
    /** See various ways to create Delegate below!
     *  When needed by design one can use explicit initialization
     *  to handle the case of calling the "unassigned" reference */
    constexpr Delegate() = delete;


    ///Issue/call Delegate with corresponding arguments
    /** Notice this call is the same for all kinds of Functor,
     *  and that signature is defined by Delegate template parameters */
    constexpr Res operator()(Args... args) const;


    ///Make Delegate from existing functor
    /** Be aware that the lifetime of functor shall not end
     *  before calls to Delegate are still made */
    template<class Functor>
    constexpr Delegate(Functor& functor);


    ///Signature for "simple" callback being passed to Delegate
    using SimpleCaseCallee = Res(Args...);

    ///Make Delegate from "ordinary function"
    /** Wraps function pointer, call to Delegate simply redirects call
     * and corresponding (Args...) parameters to that function. 
     * Here you can pass to simpleCaseCallee any variable */
    constexpr Delegate(SimpleCaseCallee* simpleCaseCallee);


    ///Force delegate reference from temporary functor (be aware what you do)
    /** NOTE: this is not for storage, only as parameters like [&](...){ lambda!
     *        Be aware: do not store permanently after API exits */
    template<class Functor>
    static constexpr Delegate Unstorable(Functor&& functor);


    ///Helper class to create Delegate instances, see Bind API below
    /** For usage in chained call, see sample below.
     * Ties methods/functions statically, see sample below */
    template<class C>
    class BoundDelegateBuilder;

    ///Easily create Delegate (closure) from instance and method pointer
    /** Allow method or function to be bound to instance reference
    Usage Sample (creating Delegate for wrapping method)
    @code
        Delegate<int(int)>::Bind(objectName).Method<&ClassName::method>()
        Delegate<int(int)>::Bind(objectName).Function<&function_to_bind_receivingRef>()
        Delegate<int(int)>::Bind(objectName).Function<&function_to_bind_receivingPtr>()
    @endcode */
    template<class C>
    static constexpr typename 
    Delegate::template BoundDelegateBuilder<C> Bind(C& object);

    ///Easily create Delegate (closure) from instance and method pointer
    /** Allow method or function to be bound to pointer
    Usage Sample (creating Delegate for wrapping method)
    @code
        Delegate<int(int)>::Bind(&objectName).Method<&ClassName::method>()
        Delegate<int(int)>::Bind(&objectName).Function<&function_to_bind_receivingRef>()
        Delegate<int(int)>::Bind(&objectName).Function<&function_to_bind_receivingPtr>()
    @endcode */
    template<class C>
    static constexpr typename
    Delegate::template BoundDelegateBuilder<C> Bind(C* objectPointer);

    ///Test delegate points to something (is "not null")
    constexpr explicit operator bool() const;
    ///Check this instance is "less" then other
    constexpr bool operator <(const Delegate& other) const;
    ///Check this instance is "greater" then other
    constexpr bool operator >(const Delegate& other) const;
    ///Check this instance is "equal" to other
    constexpr bool operator ==(const Delegate& other) const;
    ///Check this instance is "not equal" to other
    constexpr bool operator !=(const Delegate& other) const;

    //allow all the copying and assignments
    constexpr Delegate(const Delegate&) = default;
    constexpr Delegate(Delegate&&) = default;
    Delegate& operator =(const Delegate&) = default;
    Delegate& operator =(Delegate&&) = default;

private:
    ///Signature of the caller is the same for all instances with the same signature
    using InternalCallerType = Res(const Delegate* self, Args...);

    ///All callers have the same signature, regardless of what is called
    /** the operator(...) invokes correspondingCaller uniformly */
    InternalCallerType* correspondingCaller = nullptr;

    //union allows us to follow absolute minimalism
    union{
        ///Case of Functor reference - points to some object
        /** (turns to "this" for the method being called) */
        void* theCalleeAsObject;
        ///Case of Functor reference - points to some const object
        /** (turns to "this" for the method being called) */
        const void* theCalleeAsConstObject;

        ///Case of "simple function" to be called
        /** Make Delegate "universal", so that not only functor objects,
         * but also "simple" functions can be attached! */
        SimpleCaseCallee* simpleCaseCallee;
    };


    ///Internal constructor to tie with actual caller and object
    constexpr Delegate(InternalCallerType* caller, void* theCalleeAsObjectToUse);
    ///Internal constructor to tie with actual caller and const object
    constexpr Delegate(InternalCallerType* caller, const void* theCalleeAsObjectToUse);

    ///Handle the case when item to be called is simple function
    constexpr static Res callSimpleCase(const Delegate* self, Args... args);

    ///Compare to other delegate (to make <, >, ==, != possible)
    constexpr int cmpTo(const Delegate& other) const;


    //Declare template "meta functions" to generate call forwarders: 

    ///Template to obtain caller for original object
    /** static apply member is the function produced */
    template<class Functor>
    struct MakeCallerForFunctor;

    ///Template to obtain method caller for original object
    /** Static apply member is the function to produce method caller */
    template<class C, Res (C::*methodValueAsTemplateParameter)(Args...)>
    struct MakeCallerForMethod;

    ///Template to obtain method caller for original object
    /** Static apply member is the function to produce method caller */
    template<class C, Res (C::*methodValueAsTemplateParameter)(Args...) const>
    struct MakeCallerForConstMethod;

    ///Template to obtain function caller for original object (case of reference)
    /** Static apply member is the function to produce method caller */
    template<class C, Res (*functionValueAsTemplateParameter)(C&, Args...)>
    struct MakeCallerForFunctionWithReference;

    ///Template to obtain function caller for original object (case of reference)
    /** Static apply member is the function to produce method caller */
    template<class C, Res (*functionValueAsTemplateParameter)(const C&, Args...)>
    struct MakeCallerForFunctionWithConstReference;

    ///Template to obtain function caller for original object (case of pointer)
    /** Static apply member is the function to produce method caller */
    template<class C, Res (*functionValueAsTemplateParameter)(C*, Args...)>
    struct MakeCallerForFunctionWithPointer;

    ///Template to obtain function caller for original object (case of pointer)
    /** Static apply member is the function to produce method caller */
    template<class C, Res (*functionValueAsTemplateParameter)(const C*, Args...)>
    struct MakeCallerForFunctionWithConstPointer;
};


template<class Res, class... Args>
template<class C>
class Delegate<Res(Args...)>::BoundDelegateBuilder{
public:
    //NOTE: it is possible to call both nonconst and const methods on nonconst C

    ///Used by Delegate<Res(Args...)>::Bind(object) to start chained call
    constexpr BoundDelegateBuilder(C* instanceToCallMethodOn)
        : targetObject(instanceToCallMethodOn) {}

    ///Tie method to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object)
     * Place method address as template parameter,
     * use builder Method to create Delegate from class methods
     * (for the case you have ready to use method).
     * NOTE: method is tied statically at compile time */
    template<Res (C::*methodValueAsTemplateParameter)(Args...)>
    constexpr Delegate Method() const {
        return Delegate(
            MakeCallerForMethod<C, methodValueAsTemplateParameter>::apply,
            targetObject
        );
    }
    ///Tie const method to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object)
     * Place method address as template parameter,
     * use builder Method to create Delegate from class methods
     * (for the case you have ready to use method).
     * NOTE: method is tied statically at compile time */
    template<Res (C::*methodValueAsTemplateParameter)(Args...) const>
    constexpr Delegate Method() const {
        return Delegate(
            MakeCallerForConstMethod<C, methodValueAsTemplateParameter>::apply,
            targetObject
        );
    }

    ///Tie free function to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object) 
     * Place your function address as template parameter,
     * use builder Function to create Delegate from free functions
     * (for the case you do not want to change the original class).
     * NOTE: free function is tied statically at compile time */
    template<Res (*functionValueAsTemplateParameter)(C&, Args...)>
    constexpr Delegate Function() const {
        return Delegate(
            MakeCallerForFunctionWithReference<C, functionValueAsTemplateParameter>::apply,
            targetObject
        );
    }
    ///Tie free const function to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object) 
     * Place your function address as template parameter,
     * use builder Function to create Delegate from free functions
     * (for the case you do not want to change the original class).
     * NOTE: free function is tied statically at compile time */
    template<Res (*functionValueAsTemplateParameter)(const C&, Args...)>
    constexpr Delegate Function() const {
        return Delegate(
            MakeCallerForFunctionWithConstReference<C, functionValueAsTemplateParameter>::apply,
            targetObject
        );
    }

    ///Tie free function to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object) 
     * Place your function address as template parameter,
     * use builder Function to create Delegate from free functions
     * (for the case you do not want to change the original class).
     * NOTE: free function is tied statically at compile time */
    template<Res (*functionValueAsTemplateParameter)(C*, Args...)>
    constexpr Delegate Function() const {
        return Delegate(
            MakeCallerForFunctionWithPointer<C, functionValueAsTemplateParameter>::apply,
            targetObject
        );
    }
    ///Tie free const function to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object) 
     * Place your function address as template parameter,
     * use builder Function to create Delegate from free functions
     * (for the case you do not want to change the original class).
     * NOTE: free function is tied statically at compile time */
    template<Res (*functionValueAsTemplateParameter)(const C*, Args...)>
    constexpr Delegate Function() const {
        return Delegate(
            MakeCallerForFunctionWithConstPointer<C, functionValueAsTemplateParameter>::apply,
            targetObject
        );
    }

private:
    C* targetObject;
};


template<class Res, class... Args>
template<class C>
class Delegate<Res(Args...)>::BoundDelegateBuilder<const C>{
public:
    //NOTE: it is possible to call only const methods on const C

    ///Used by Delegate<Res(Args...)>::Bind(object) to start chained call
    constexpr BoundDelegateBuilder(const C* instanceToCallMethodOn)
        : targetObject(instanceToCallMethodOn) {}

    ///Tie const method to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object)
     * Place method address as template parameter,
     * use builder Method to create Delegate from class methods
     * (for the case you have ready to use method).
     * NOTE: method is tied statically at compile time */
    template<Res (C::*methodValueAsTemplateParameter)(Args...) const>
    constexpr Delegate Method() const {
        return Delegate(
            MakeCallerForConstMethod<const C, methodValueAsTemplateParameter>::apply,
            targetObject
        );
    }

    ///Tie const free function to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object) 
     * Place your function address as template parameter,
     * use builder Function to create Delegate from free functions
     * (for the case you do not want to change the original class).
     * NOTE: free function is tied statically at compile time */
    template<Res (*functionValueAsTemplateParameter)(const C&, Args...)>
    constexpr Delegate Function() const {
        return Delegate(
            MakeCallerForFunctionWithConstReference<const C, functionValueAsTemplateParameter>::apply,
            targetObject
        );
    }

    ///Tie const free function to call on object with that object into Delegate
    /** Chained call after Delegate<Res(Args...)>::Bind(object) 
     * Place your function address as template parameter,
     * use builder Function to create Delegate from free functions
     * (for the case you do not want to change the original class).
     * NOTE: free function is tied statically at compile time */
    template<Res (*functionValueAsTemplateParameter)(const C*, Args...)>
    constexpr Delegate Function() const {
        return Delegate(
            MakeCallerForFunctionWithConstPointer<const C, functionValueAsTemplateParameter>::apply,
            targetObject
        );
    }

private:
    const C* targetObject;
};

//TODO: what about multicast delegated? What about event sources with subscription, etc?

/* TODO: copyable lambdas
//TODO: default size depending on platform

template<unsigned size = 8, Res(Args...)>
class LambdaContainer{
public:
private:
};

template<class F, unsigned size = 8>
class Lambda{};


///Special container to create unique function and map it to lambda
//Allow association between global callback and lambda
#define CreateTrampoline()
*/



//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################


/* Keep promise to not use any standard libraries by default, 
Enable defines below to allow standard library instead of own implementation */
//#define INSTANTARDUINO_USE_STDLIB
#ifdef INSTANTARDUINO_USE_STDLIB
#   if __has_include(<cstring>)
#       include <cstring>
        using std::memcmp
#   else
        //remember, there is no <cstring> header for AVR
#       include <string.h>
#   endif
#endif

template<class Res, class... Args>
template<class Functor>
struct Delegate<Res(Args...)>::MakeCallerForFunctor{
    ///Extract C++ object from Delegate and forward arguments
    static constexpr Res apply(const Delegate* self, Args... args){
        return (*static_cast<Functor*>(self->theCalleeAsObject))( static_cast<Args&&>(args)... );
    }
};

template<class Res, class... Args>
template<class C, Res (C::*methodValueAsTemplateParameter)(Args...)>
struct Delegate<Res(Args...)>::MakeCallerForMethod{
    static constexpr Res apply(const Delegate* self, Args... args){
        return (static_cast<C*>(self->theCalleeAsObject)->*methodValueAsTemplateParameter)
                    (static_cast<Args&&>(args)...);
    }
};
template<class Res, class... Args>
template<class C, Res (C::*methodValueAsTemplateParameter)(Args...) const>
struct Delegate<Res(Args...)>::MakeCallerForConstMethod{
    static constexpr Res apply(const Delegate* self, Args... args){
        return (static_cast<const C*>(self->theCalleeAsConstObject)->*methodValueAsTemplateParameter)
                    (static_cast<Args&&>(args)...);
    }
};

template<class Res, class... Args>
template<class C, Res (*functionValueAsTemplateParameter)(C&, Args...)>
struct Delegate<Res(Args...)>::MakeCallerForFunctionWithReference{
    static constexpr Res apply(const Delegate* self, Args... args){
        return functionValueAsTemplateParameter(
            *static_cast<C*>(self->theCalleeAsObject),
            static_cast<Args&&>(args)...
        );
    }
};
template<class Res, class... Args>
template<class C, Res (*functionValueAsTemplateParameter)(const C&, Args...)>
struct Delegate<Res(Args...)>::MakeCallerForFunctionWithConstReference{
    static constexpr Res apply(const Delegate* self, Args... args){
        return functionValueAsTemplateParameter(
            *static_cast<const C*>(self->theCalleeAsConstObject),
            static_cast<Args&&>(args)...
        );
    }
};

template<class Res, class... Args>
template<class C, Res (*functionValueAsTemplateParameter)(C*, Args...)>
struct Delegate<Res(Args...)>::MakeCallerForFunctionWithPointer{
    static constexpr Res apply(const Delegate* self, Args... args){
        return functionValueAsTemplateParameter(
            static_cast<C*>(self->theCalleeAsObject),
            static_cast<Args&&>(args)...
        );
    }
};
template<class Res, class... Args>
template<class C, Res (*functionValueAsTemplateParameter)(const C*, Args...)>
struct Delegate<Res(Args...)>::MakeCallerForFunctionWithConstPointer{
    static constexpr Res apply(const Delegate* self, Args... args){
        return functionValueAsTemplateParameter(
            static_cast<const C*>(self->theCalleeAsConstObject),
            static_cast<Args&&>(args)...
        );
    }
};


template<class Res, class... Args>
constexpr Res Delegate<Res(Args...)>::operator()(Args... args) const {
    //Forward call arguments to the actual caller
    //Note: forwarding manually with static_cast<Args&&> since AVR has no std::forward defined
    return correspondingCaller(this, static_cast<Args&&>(args)...);
}


template<class Res, class... Args>
template<class Functor>
constexpr Delegate<Res(Args...)>::Delegate(Functor& functor)
    : Delegate(MakeCallerForFunctor<Functor>::apply, &functor) {}

template<class Res, class... Args>
constexpr Delegate<Res(Args...)>::Delegate(SimpleCaseCallee* simpleCaseCalleeFunction)
    : correspondingCaller(callSimpleCase), simpleCaseCallee(simpleCaseCalleeFunction) {}


template<class Res, class... Args>
template<class Functor>
constexpr Delegate<Res(Args...)>
    Delegate<Res(Args...)>::Unstorable(Functor&& functor)
{
    return Delegate(MakeCallerForFunctor<Functor>::apply, &functor);
}


template<class Res, class... Args>
constexpr Delegate<Res(Args...)>::Delegate(
    InternalCallerType* caller, void* theCalleeAsObjectToUse
)
    : correspondingCaller(caller), theCalleeAsObject(theCalleeAsObjectToUse){}

template<class Res, class... Args>
constexpr Delegate<Res(Args...)>::Delegate(
    InternalCallerType* caller, const void* theCalleeAsObjectToUse
)
    : correspondingCaller(caller), theCalleeAsConstObject(theCalleeAsObjectToUse){}


template<class Res, class... Args>
template<class C>
constexpr typename Delegate<Res(Args...)>::template BoundDelegateBuilder<C>
Delegate<Res(Args...)>::Bind(C& object){
    return BoundDelegateBuilder<C>(&object);
}

template<class Res, class... Args>
template<class C>
constexpr typename Delegate<Res(Args...)>::template BoundDelegateBuilder<C>
Delegate<Res(Args...)>::Bind(C* objectPointer){
    return BoundDelegateBuilder<C>(objectPointer);
}


template<class Res, class... Args>
constexpr Delegate<Res(Args...)>::operator bool() const {
    /* the only way to create Delegate around nullptr is passing
       null as simpleCaseCalleeFunction to constructor */
    return callSimpleCase != correspondingCaller || nullptr != simpleCaseCallee;
}

template<class Res, class... Args>
constexpr bool Delegate<Res(Args...)>::operator <(const Delegate& other) const{
    return cmpTo(other) < 0;
}

template<class Res, class... Args>
constexpr bool Delegate<Res(Args...)>::operator >(const Delegate& other) const{
    return cmpTo(other) > 0;
}

template<class Res, class... Args>
constexpr bool Delegate<Res(Args...)>::operator ==(const Delegate& other) const{
    return cmpTo(other) == 0;
}

template<class Res, class... Args>
constexpr bool Delegate<Res(Args...)>::operator !=(const Delegate& other) const{
    return cmpTo(other) != 0;
}


template<class Res, class... Args>
constexpr Res Delegate<Res(Args...)>::callSimpleCase(const Delegate* self, Args... args){
    return self->simpleCaseCallee(static_cast<Args&&>(args)...);
}


template<class Res, class... Args>
constexpr int Delegate<Res(Args...)>::cmpTo(const Delegate& other) const{
    static_assert(
        sizeof(Delegate::theCalleeAsObject) == sizeof(Delegate::simpleCaseCallee),
        "Comparison will not work if sizes are different"
    );
    static_assert(
        sizeof(Delegate::theCalleeAsObject) + sizeof(Delegate::simpleCaseCallee)
        == sizeof(Delegate),
        "Comparison will not work if sizes are different"
    );

#ifdef INSTANTARDUINO_USE_STDLIB
    static_assert(sizeof(int) > 1, "This function is not -mint8 compatible!");
    return memcmp(this, &other, sizeof(Delegate));
#else
    //Straight forward implementation, respect strict aliasing rules
    //see also https://stackoverflow.com/questions/25664848/unions-and-type-punning
    //and https://stackoverflow.com/questions/98650/what-is-the-strict-aliasing-rule/99010#99010
    //and also https://youtu.be/sCjZuvtJd-k
    const char* my = reinterpret_cast<const char*>(this);
    const char* their = reinterpret_cast<const char*>(&other);

    for(auto count = sizeof(Delegate); count > 0; --count){
        if( int diff = *my++ - *their++ ){
            return diff;
        }
    }
    return 0; //definitely both are equal
#endif
}



//TODO: reduce? map?

#endif
