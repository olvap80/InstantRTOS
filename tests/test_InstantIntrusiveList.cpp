/** @file tests/test_InstantIntrusiveList.cpp
    @brief Unit tests for InstantIntrusiveList.h
*/

#include <exception>
/// Custom exception for testing InstantIntrusiveListPanic
class TestInstantIntrusiveListException: public std::exception{
    const char* what() const noexcept override{
        return "TestInstantIntrusiveListException";
    }
};
//Header will see this definition
#define InstantIntrusiveListPanic() throw TestInstantIntrusiveListException()
#include "InstantIntrusiveList.h"

#include "doctest/doctest.h"
#include <initializer_list>

namespace{
    /// Class for testing purposes
    class MyTestItem: public IntrusiveList<MyTestItem>::Node{
    public:
        /// Constructor for testing purposes
        MyTestItem(int value) : storedValue(value){}

        /// Method for testing purposes
        int Value(){
            return storedValue;
        }
    private:
        int storedValue;
    };

} //namespace

TEST_CASE("InstantIntrusiveList"){
    // put your main code here, to run repeatedly:
    IntrusiveList<MyTestItem> il;

    MyTestItem ti1(11);
    MyTestItem ti2(22);
    MyTestItem ti3(33);

    il.InsertAtFront(&ti2);
    il.InsertAtFront(&ti1);
    il.InsertAtBack(&ti3);
    {
        static constexpr auto expected = {11, 22, 33};

        int i = 0;
        auto expected_it = expected.begin();
        for(auto& item : il){
            CHECK(item.Value() == *expected_it);
            ++i;
            ++expected_it;
        }
        CHECK( i == expected.size() );
    }


    MyTestItem ti4(444);
    ti2.InsertPrevChainElement(&ti4);
    {
        static constexpr auto expected = {11, 444, 22, 33};

        int i = 0;
        auto expected_it = expected.begin();
        for(auto& item : il){
            CHECK(item.Value() == *expected_it);
            ++i;
            ++expected_it;
        }
        CHECK( i == expected.size() );
    }


    ti2.InsertNextChainElement(&ti4);
    {
        static constexpr auto expected = {11, 22, 444, 33};

        int i = 0;
        auto expected_it = expected.begin();
        for(auto& item : il){
            CHECK(item.Value() == *expected_it);
            ++i;
            ++expected_it;
        }
        CHECK( i == expected.size() );
    }


    ti4.RemoveFromChain();
    {
        static constexpr auto expected = {11, 22, 33};

        int i = 0;
        auto expected_it = expected.begin();
        for(auto& item : il){
            CHECK(item.Value() == *expected_it);
            ++i;
            ++expected_it;
        }
        CHECK( i == expected.size() );
    }


    il.RemoveAtFront();
    {
        static constexpr auto expected = {22, 33};

        int i = 0;
        auto expected_it = expected.begin();
        for(auto& item : il){
            CHECK(item.Value() == *expected_it);
            ++i;
            ++expected_it;
        }
        CHECK( i == expected.size() );
    }


    il.RemoveAtEnd();
    {
        static constexpr auto expected = {22};

        int i = 0;
        auto expected_it = expected.begin();
        for(auto& item : il){
            CHECK(item.Value() == *expected_it);
            ++i;
            ++expected_it;
        }
        CHECK( i == expected.size() );
    }


    ti2.RemoveFromChain();
    CHECK( il.IsEmpty() );
}
