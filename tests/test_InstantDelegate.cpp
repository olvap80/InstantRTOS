/** @file tests/test_InstantDelegate.cpp
    @brief Unit tests for InstantDelegate.h
*/

#include "InstantDelegate.h"
#include "doctest/doctest.h"
#include <cstdint>


namespace{
    using std::int32_t;

    //shorthand to not repeat the same signature every time
    using MyCallback = Delegate<int32_t(int32_t)>;

    ///using callback by some custom API
    int32_t CustomAPI(const MyCallback& callbackNeeded, int32_t some_int_argument){
        return callbackNeeded(2 * some_int_argument);
    }

    ///"Plain" function with compatible signature
    int32_t ordinary_function_with_compatible_signature(int32_t val){
        return val + 84;
    }

    ///Some class with method to be called
    class SomeClass{
    public:
        /// Have some state in the object 
        SomeClass(int32_t val): some_private_field(val){}

        /// Method to be called via Delegate
        int32_t some_method(int32_t val){
            return val + 142 + some_private_field;
        }
    private:
        int32_t some_private_field;
    };

    /// callable thing having compatible operator() can also be referenced
    class SomeFunctor{
    public:
        /// Have some state in the functor
        SomeFunctor(int32_t val): some_private_field(val){}

        /// Make SomeFunctor callable
        int32_t operator()(int32_t val) {
            return (val + 342) / some_private_field;
        }
    private:
        int32_t some_private_field;
    };

} //namespace

TEST_CASE("InstantDelegate simple"){
    //various ways for creating the callback from lambdas, objects, methods
    
    //one can make callback directly from C++ lambda
    CHECK(
        2042 == CustomAPI(
                    MyCallback([](int val){ return val + 42;}),
                    1000
                )
    );

    //and from "simple" function (implicit conversion is supported)
    CHECK(
        4084 == CustomAPI(
                    ordinary_function_with_compatible_signature, 2000
                )
    );
    
    //also one can create Delegate for calling method of some object 
    SomeClass targetObject{10000};
    CHECK(
        16142 == CustomAPI(
                    MyCallback::From(targetObject).Bind<&SomeClass::some_method>(), 3000
                 )
    );


    SomeFunctor customCallable{2};
    CHECK( 4171 == CustomAPI(customCallable, 4000) );

    //assume CustomAPI does not store Delegate, so it is safe to use temporary
    CHECK( 4171 == CustomAPI(SomeFunctor{2}, 4000) );
}


namespace{
    ///Sample API for receiving "callable" Delegate as parameter
    int test(const Delegate<int(int)>& delegateToCall){
        return 20000 + delegateToCall(10);
    }

    ///Sample "unbound" function to be called via Delegate
    int function_to_pass_to_Delegate(int val){
        return val + 200;
    }

    ///Sample class (to demonstrate functor and method calls)
    class TestClass{
    public:
        TestClass(int addToVal): addTo(addToVal) {}

        ///Demo functor to be tied with delegate
        int operator()(int val) const {
            return val + addTo;
        }

        ///Demo method to be tied with delegate (object, method pair)
        int test_method(int val) const {
            return val + addTo + 1000;
        }
        int test_method2(int val) const {
            return val + addTo + 100;
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
        return t.valueGet() + 100 + val;
    }
    int function_to_bind_receivingPtr(class TestClass* t, int val){
        return t->valueGet() + 1000 + val;
    }
} //namespace


TEST_CASE("InstantDelegate advanced"){
    //demo of "something callable" being passed to test as Delegate
    TestClass demoCallable{-2};
    CHECK( 20008 == test(demoCallable) );

    //demo variable to be captured by lambda
    int captured = 1;
    //C++ "long lasting" lambda to be referenced by the Delegate
    auto lambdaAsVariable = [&](int val){
        return val + captured;
    };
    //one can easy call any functor/callable via delegate
    //assuming callable functor lives as long as delegate is called
    CHECK( 20011 == test(lambdaAsVariable) );

    //Function pointer can be called (implicit conversion, direct constructor is called)
    CHECK( 20210 == test(function_to_pass_to_Delegate) );


    //inline "non capturing" lambda can be explicitly forced to be Delegate
    CHECK(
        20052 == test(
                    Delegate<int(int)>([](int val){
                        return val + 42;
                    })
                )
    );
    

    captured = 4;
    //call with inlined lambda is also possible to force with Unstorable
    //(we can do this because we do not save that temporary!)
    CHECK(
        20014 ==    test(Delegate<int(int)>::Unstorable([&](int val){
                        return val + captured;
                    }))
    );

    //method pointer for existing object can be created
    const TestClass demoCallable5{5};
    //Note: direct call without delegate is demoCallable5.test_method(42);
    CHECK(
        21015 == test(
                    Delegate<int(int)>::From(demoCallable5).Bind<&TestClass::test_method>()
                )
    );

    //Ensure other method pointer can be created
    TestClass demoCallable6{6};
    CHECK(
        20116 == test(
                    Delegate<int(int)>::From(&demoCallable6).Bind<&TestClass::test_method2>()
                )
    );


    //Ensure method pointer can be created from function receiving reference
    TestClass demoCallable7{7};
    CHECK(
        20117 == test(
                    Delegate<int(int)>::From(demoCallable7)
                        .Bind<&function_to_bind_receivingRef>()
                )
    );
    //Ensure method pointer can be created from function receiving pointer
    TestClass demoCallable8{8};
    CHECK(
        21018 == test(
                    Delegate<int(int)>::From(&demoCallable8)
                        .Bind<&function_to_bind_receivingPtr>()
                )
    );
}
