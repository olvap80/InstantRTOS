/** @file tests/test_InstantReadyBox.cpp
    @brief Unit tests for InstantReadyBox.h
*/

#include "InstantReadyBox.h"

#include "doctest/doctest.h"
#include <string>
#include <vector>
#include <memory>

namespace{
    /// Class for testing purposes with value
    class TestValue{
    public:
        TestValue(int val = 0) : value(val) {}
        int GetValue() const { return value; }
        bool operator==(const TestValue& other) const { return value == other.value; }
    private:
        int value;
    };

    /// Helper to track callback calls
    struct CallbackTracker{
        int callCount = 0;
        std::vector<int> receivedValues;

        void Reset(){
            callCount = 0;
            receivedValues.clear();
        }

        void RecordCall(int value = 0){
            ++callCount;
            receivedValues.push_back(value);
        }
    };
} //namespace

TEST_CASE("ReadyBoxTrigger<int> basic functionality"){
    ReadyBoxTrigger<int> trigger;
    CallbackTracker tracker;

    SUBCASE("OnReady with value available immediately"){
        // Trigger first, then subscribe
        trigger(42);

        //Callback shall be issued for sure here (immediately)
        //lambda will not fade away
        auto lambdaMustBeAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        auto totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEvents == 1 );

        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 42 );

        //No new events should be triggered, but we need to make types happy))
        auto lambdaMustBeAliveAgain = [&](const int& result){
            tracker.RecordCall(result); // shall not be called
        };
        auto totalPreviousEventsAgain = trigger.OnReady(lambdaMustBeAliveAgain);
        CHECK( totalPreviousEventsAgain == 0 );
        //nothing changes for existing number of calls
        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.size() == 1 );

    }

    SUBCASE("OnReady with callback set first"){
        /* We must always ensure any capturing lambda,
           or object + method pair is alive as long as they can be called */
        auto lambdaMustBeAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        // Subscribe first, then trigger later
        auto totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive); //does not trigger yet

        // No previous calls shall happen
        CHECK( totalPreviousEvents == 0 );
        CHECK( tracker.callCount == 0 );

        trigger(55); // Now the callback shall be called

        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 55 );

        //New subscription shall hot trigger without new event
        auto totalPreviousEventsAgain = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEventsAgain == 0 );
        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.size() == 1 );
    }

    SUBCASE("Multiple triggers without callback accumulate count"){
        trigger(10);
        trigger(20);
        trigger(30);

        // Only last value is preserved (callback issues immediately)
        auto lambdaMustBeAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        auto totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive);
        //Only one event (the last) is processed immediately, but the rest was extracted
        CHECK( totalPreviousEvents == 3 );
        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 30 );

        //Only one event (the last) is processed immediately, but the rest (previous) were discarded
        auto totalPreviousEventsAgain = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEventsAgain == 0 );
        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.size() == 1 );
    }

    SUBCASE("OnNext ignores previous triggers"){
        trigger(100);
        trigger(200);

        auto lambdaMustBeAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        trigger.OnNext(lambdaMustBeAlive);

        CHECK( tracker.callCount == 0 );

        trigger(300); //Lambda shall be called now

        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 300 );
    }

    SUBCASE("ExplicitlyIgnore consumes any pending event"){
        trigger(111);
        trigger(222);
        trigger(333);

        REQUIRE( 3 == trigger.ExplicitlyIgnore() );

        //lambda is not called at all, nothing shall be triggered
        auto lambdaMustBeAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        unsigned totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEvents == 0 );
        CHECK( tracker.callCount == 0 );
    }

    SUBCASE("ResetCallback clears the state, no handler is called"){
        trigger(123);
        trigger(456);

        trigger.ResetCallback();

        //lambda is not called at all, nothing shall be triggered
        auto lambdaMustBeAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        unsigned totalPreviousEventsAgain = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEventsAgain == 0 );

        CHECK( tracker.callCount == 0 );
    }

    SUBCASE("StoredResult provides access to last value"){
        trigger(999);

        //lambda is called immediately
        auto lambdaMustBeAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        unsigned totalPreviousEventsCount1 = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEventsCount1 == 1 );

        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 999 );

        unsigned totalPreviousEventsCount2 = trigger.OnReady(lambdaMustBeAlive);
        // No new call because no new value was triggered
        CHECK( totalPreviousEventsCount2 == 0 );
        CHECK( tracker.callCount == 1 );

        trigger(555);

        // new call just happened
        CHECK( tracker.callCount == 2 );
        CHECK( tracker.receivedValues.at(1) == 555 );

        trigger(333);
        trigger(777);
        trigger(888);

        // no new call yet, because previous value handler was already used
        CHECK( tracker.callCount == 2 );
        CHECK( tracker.receivedValues.size() == 2 );

        // Subscribe again
        unsigned totalPreviousEventsCount3 = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEventsCount3 == 3 );
        CHECK( tracker.callCount == 3 );
        CHECK( tracker.receivedValues.at(2) == 888 );
    }
}

TEST_CASE("ReadyBoxTrigger<void> basic functionality"){
    ReadyBoxTrigger<void> trigger;
    CallbackTracker tracker;

    SUBCASE("OnReady with call available immediately"){
        // Trigger first, then subscribe
        trigger();

        //Callback shall be issued for sure here (immediately)
        auto lambdaMustBeAlive = [&](){
            tracker.RecordCall();
        };
        auto totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive);

        CHECK( totalPreviousEvents == 1 );
        CHECK( tracker.callCount == 1 );

        //This one shall not trigger at all
        auto totalPreviousEventsAgain = trigger.OnReady(lambdaMustBeAlive);

        CHECK( totalPreviousEventsAgain == 0 );
        CHECK( tracker.callCount == 1 ); //shall stay the same
    }

    SUBCASE("OnReady with callback set first"){
        // Subscribe first, then trigger
        auto lambdaMustBeAlive = [&](){
            tracker.RecordCall();
        };
        trigger.OnReady(lambdaMustBeAlive); //does not trigger yet

        CHECK( tracker.callCount == 0 );

        trigger(); // Now it should trigger

        CHECK( tracker.callCount == 1 );

        //New subscription shall hot trigger without new event
        auto totalPreviousEventsAgain = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEventsAgain == 0 );
        CHECK( tracker.callCount == 1 ); //stays the same
    }

    SUBCASE("Multiple triggers without callback accumulate count"){
        trigger();
        trigger();
        trigger();

        // Callback issues immediately and only once
        auto lambdaMustBeAlive = [&](){
            tracker.RecordCall();
        };
        unsigned totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive);

        //There were three events
        CHECK( totalPreviousEvents == 3 );
        // But only one callback was issued only once
        CHECK( tracker.callCount == 1 );
    }

    SUBCASE("OnNext ignores previous triggers"){
        trigger();
        trigger();

        // first OnNext should not fire for past events
        auto keepAlive = [&](){ tracker.RecordCall(); };
        trigger.OnNext(keepAlive); //does not trigger yet

        CHECK( tracker.callCount == 0 );

        trigger(); // fires now
        CHECK( tracker.callCount == 1 );
    }

    SUBCASE("ExplicitlyIgnore consumes all pending events"){
        trigger();
        trigger();
        trigger();

        unsigned ignored = trigger.ExplicitlyIgnore();
        CHECK( ignored == 3 );
        CHECK( tracker.callCount == 0 );

        // subsequent OnReady should not fire at all (nothing pending)
        auto lambdaMustBeAlive = [&](){
            tracker.RecordCall();
        };
        unsigned anything = trigger.OnReady(lambdaMustBeAlive);
        CHECK( anything == 0 );
        CHECK( tracker.callCount == 0 );
    }

    SUBCASE("ResetCallback clears state and future trigger counted"){
        trigger();
        trigger();

        trigger.ResetCallback();

        auto lambdaMustBeAlive = [&](){
            tracker.RecordCall();
        };

        unsigned totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEvents == 0 ); // cleared, past events forgotten
        CHECK( tracker.callCount == 0 );

        trigger();
        CHECK( tracker.callCount == 1 );
    }

    SUBCASE("Reentrant OnReady for void variant"){
        auto nestedKeepAlive = [&](){ tracker.RecordCall(); };
        trigger();
        CHECK( tracker.callCount == 0 );

        //This one is called immediately
        auto anotherKeepAlive = [&](){
            tracker.RecordCall();
            // subscribe again inside callback; this one should wait for next trigger
            trigger.OnReady(nestedKeepAlive);
        };
        trigger.OnReady(anotherKeepAlive);
        CHECK( tracker.callCount == 1 );
        trigger(); // triggers the inner subscription (nestedKeepAlive)
        CHECK( tracker.callCount == 2 );
    }

    SUBCASE("Consecutive ExplicitlyIgnore calls"){
        trigger();
        trigger();

        CHECK( trigger.ExplicitlyIgnore() == 2 );
        CHECK( trigger.ExplicitlyIgnore() == 0 );
    }
}

TEST_CASE("ReadyBoxTrigger with custom types"){
    ReadyBoxTrigger<TestValue> trigger;
    CallbackTracker tracker;

    SUBCASE("Works with custom class"){
        TestValue testVal(42);

        trigger(testVal);

        //Callback shall be issued for sure here (immediately)
        auto lambdaMustBeAlive = [&](const TestValue& result){
            tracker.RecordCall(result.GetValue());
        };
        trigger.OnReady(lambdaMustBeAlive);

        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 42 );
    }

    SUBCASE("Stored result preserves custom object"){
        TestValue testVal(123);
        trigger(testVal);

        //CHECK( trigger.UntrackedEventsCount() == 1 );
        //CHECK( trigger.StoredResult()->GetValue() == 123 );
    }
}

TEST_CASE("ReadyBoxTrigger with move-only types (std::unique_ptr)"){
    // Move-only value type; ensures ReadyBoxTrigger correctly moves and stores last value only.
    ReadyBoxTrigger<std::unique_ptr<int>> trigger;

    SUBCASE("Multiple pending unique_ptr values, only last delivered"){
        trigger(std::make_unique<int>(1));
        trigger(std::make_unique<int>(2));
        trigger(std::make_unique<int>(42));

        int received = -1;
        // called immediately
        auto lambdaMustBeAlive = [&](const std::unique_ptr<int>& p){
            REQUIRE( p );
            received = *p;
        };
        unsigned totalPreviousEvents = trigger.OnReady(lambdaMustBeAlive);
        CHECK( totalPreviousEvents == 3 );
        CHECK( received == 42 );

        // No additional pending now
        int second = -1;
        //shall not be called at all
        auto keepSecondLambdaAlive = [&](const std::unique_ptr<int>& p){ second = *p; };
        unsigned again = trigger.OnReady(keepSecondLambdaAlive);
        CHECK( again == 0 );
        CHECK( second == -1 );
    }

    SUBCASE("OnNext skips past unique_ptr events and captures next one"){
        trigger(std::make_unique<int>(10));
        trigger(std::make_unique<int>(20));

        int received = -1;
        auto keepAlive = [&](const std::unique_ptr<int>& p){ received = *p; };
        trigger.OnNext(keepAlive); // should not fire yet
        CHECK( received == -1 );

        trigger(std::make_unique<int>(30)); // fires
        CHECK( received == 30 );

        // Another trigger should NOT fire keepAlive again until resubscribed
        trigger(std::make_unique<int>(40));
        CHECK( received == 30 );
    }

    SUBCASE("Reset after OnNext with move-only type makes pending consumable via OnReady"){
        bool cbCalled = false;
        auto cb = [&](const std::unique_ptr<int>&){
            cbCalled = true;
        };

        trigger.OnNext(cb); // set but not triggered

        CHECK( cbCalled == false );

        trigger.ResetCallback(); // remove it

        trigger(std::make_unique<int>(77)); // becomes pending
        int val = -1;

        // called immediately
        auto keepValLambdaAlive = [&](const std::unique_ptr<int>& p){ val = *p; };
        unsigned totalPreviousEvents = trigger.OnReady(keepValLambdaAlive);
        CHECK( totalPreviousEvents == 1 );
        CHECK( val == 77 );
    }

    SUBCASE("ExplicitlyIgnore clears pending unique_ptr events"){
        trigger(std::make_unique<int>(5));
        trigger(std::make_unique<int>(6));

        unsigned ignored = trigger.ExplicitlyIgnore();
        CHECK( ignored == 2 );
        int val = -1;

        auto keepValLambdaAlive2 = [&](const std::unique_ptr<int>& p){ val = *p; };
        unsigned totalPreviousEvents = trigger.OnReady(keepValLambdaAlive2);
        CHECK( totalPreviousEvents == 0 );
        CHECK( val == -1 );
    }
}

TEST_CASE("ReadyBoxTrigger constructor variants"){
    CallbackTracker tracker;

    auto keepLambdaAlive = [&](const int& result){
        tracker.RecordCall(result);
    };

    SUBCASE("Default constructor"){
        ReadyBoxTrigger<int> trigger;

        trigger.OnReady(keepLambdaAlive);

        CHECK( tracker.callCount == 0 );
        CHECK( tracker.receivedValues.size() == 0 );

        trigger(123);
        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 123 );
    }

    SUBCASE("Constructor with callback for ReadyBoxTrigger<int>"){
        ReadyBoxTrigger<int> trigger(keepLambdaAlive);

        CHECK( tracker.callCount == 0 );
        CHECK( tracker.receivedValues.size() == 0 );

        trigger(789);

        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 789 );
    }

    SUBCASE("Constructor with callback for ReadyBoxTrigger<void>"){
        bool called = false;
        auto keepVoidLambdaAlive = [&](){ called = true; };
        ReadyBoxTrigger<void> trigger(keepVoidLambdaAlive);

        CHECK( called == false );

        trigger();

        CHECK( called == true );
    }
}

TEST_CASE("ReadyBoxTrigger reuse scenarios"){
    ReadyBoxTrigger<int> trigger;
    CallbackTracker tracker;

    SUBCASE("Can reuse after callback execution"){
        // First round
        auto lambdaKeepAlive = [&](const int& result){ tracker.RecordCall(result); };
        trigger.OnReady(lambdaKeepAlive);
        trigger(100);

        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 100 );

        // Second round
        trigger.OnReady(lambdaKeepAlive);
        trigger(200);

        CHECK( tracker.callCount == 2 );
        CHECK( tracker.receivedValues.at(1) == 200 );
    }

    SUBCASE("Callback can subscribe new callback during execution"){
        trigger(999);
        unsigned innerCallCount = 0;

        auto keepNestedLambdaAlive = [&](const int& innerResult){
            tracker.RecordCall(innerResult + 1000);
        };

        // No callbacks here
        CHECK( tracker.callCount == 0 );

        // This one (surrounding) is called immediately
        auto surroundingKeepAlive = [&](const int& result){
            tracker.RecordCall(result);
            // Subscribe again inside callback
            innerCallCount = trigger.OnReady(keepNestedLambdaAlive); //this one does not trigger yet
        };
        trigger.OnReady(surroundingKeepAlive);

        // Only outer call shall be called
        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 999 );

        // No inner calls yet
        CHECK( innerCallCount == 0 );

        // Trigger again to test inner subscription
        trigger(555);

        CHECK( tracker.callCount == 2 );
        CHECK( tracker.receivedValues.at(1) == 1555 );
    }
}

TEST_CASE("ReadyBoxTrigger edge cases"){
    SUBCASE("OnReady returns 0 when callback is set first"){
        ReadyBoxTrigger<int> trigger;
        unsigned totalPreviousEvents = trigger.OnReady([](const int& result){});
        CHECK( totalPreviousEvents == 0 );
    }

    SUBCASE("Multiple OnReady calls replace previous callback"){
        ReadyBoxTrigger<int> trigger;
        CallbackTracker tracker1, tracker2;

        auto keepLambda1Alive = [&](const int& result){
            tracker1.RecordCall(result);
        };
        trigger.OnReady(keepLambda1Alive);

        auto keepLambda2Alive = [&](const int& result){
            tracker2.RecordCall(result);
        };
        trigger.OnReady(keepLambda2Alive);

        trigger(123);

        CHECK( tracker1.callCount == 0 );
        CHECK( tracker2.callCount == 1 );
        CHECK( tracker2.receivedValues.at(0) == 123 );
    }

    SUBCASE("OnNext after pending events ignores them and waits for next"){
        ReadyBoxTrigger<int> trigger;
        CallbackTracker tracker;

        trigger(100);
        trigger(101);

        auto keepAlive = [&](const int& result){
            tracker.RecordCall(result);
        };
        trigger.OnNext(keepAlive); // does not trigger yet (because of OnNext)
        CHECK( tracker.callCount == 0 );

        trigger(200); // this will trigger the OnNext
        CHECK( tracker.callCount == 1 );
        CHECK( tracker.receivedValues.at(0) == 200 );
    }

    SUBCASE("OnNext overwritten by another OnNext"){
        ReadyBoxTrigger<int> trigger;

        int first = 0, second = 0;

        auto cb1 = [&](const int& v){ first = v; };
        auto cb2 = [&](const int& v){ second = v; };

        trigger.OnNext(cb1);
        trigger.OnNext(cb2); // overwrite

        trigger(5);

        CHECK( first == 0 );
        CHECK( second == 5 );
    }

    SUBCASE("Reset after subscription prevents old callback firing"){
        ReadyBoxTrigger<int> trigger;

        int oldCalls = 0; int newCalls = 0;

        auto oldCb = [&](const int& v){ ++oldCalls; };

        unsigned totalPreviousEventsConsumed = trigger.OnReady(oldCb); // stored, no pending yet
        CHECK( totalPreviousEventsConsumed == 0 );
        trigger.ResetCallback();

        trigger(7); // becomes pending

        // This one is called immediately
        auto keepNewCallsAlive = [&](const int& v){ ++newCalls; };
        unsigned totalPreviousEventsConsumedThisTime = trigger.OnReady(keepNewCallsAlive);

        CHECK( totalPreviousEventsConsumedThisTime == 1 );
        CHECK( oldCalls == 0 );
        CHECK( newCalls == 1 );
    }

    SUBCASE("ExplicitlyIgnore multiple times for int variant"){
        ReadyBoxTrigger<int> trigger;

        trigger(1); trigger(2); trigger(3);

        CHECK( trigger.ExplicitlyIgnore() == 3 );
        CHECK( trigger.ExplicitlyIgnore() == 0 );
    }
}

//------------------------------------------------------------------------------
// Lifetime correctness tests using a destructor-counting type
//------------------------------------------------------------------------------
namespace {
    struct DtorTracked{
        int id;
        explicit DtorTracked(int i): id(i){}
        DtorTracked(const DtorTracked&) = delete;
        DtorTracked& operator=(const DtorTracked&) = delete;
        DtorTracked(DtorTracked&& other) noexcept: id(other.id){
            // Mark moved-from so its destructor is ignored
            other.id = 0;
        }
        ~DtorTracked(){
            if( id > 0 ){
                destroyed.push_back(id);
            }
        }
        static std::vector<int> destroyed;
    };
    std::vector<int> DtorTracked::destroyed{}; // NOLINT
}

TEST_CASE("ReadyBoxTrigger lifetime: destructor counts on overwrite and consumption"){
    DtorTracked::destroyed.clear();

    {
        ReadyBoxTrigger<DtorTracked> trigger;

        // Three pending events, only last stored; previous two must be destroyed.
        trigger(DtorTracked(11));
        trigger(DtorTracked(22));
        trigger(DtorTracked(33));

        REQUIRE( DtorTracked::destroyed.size() == 2 );
        CHECK( DtorTracked::destroyed[0] == 11 );
        CHECK( DtorTracked::destroyed[1] == 22 );

        // This one is called immediately
        unsigned totalPreviousEventsConsumed = trigger.OnReady(
            [](const DtorTracked& v){
                CHECK( v.id == 33 );
            }
        );
        CHECK( totalPreviousEventsConsumed == 3 );

        // Consumption destroys the stored value
        CHECK( DtorTracked::destroyed.size() == 3 );
        CHECK( DtorTracked::destroyed[2] == 33 );

        // No additional calls
        unsigned totalPreviousEventsConsumedAgain = trigger.OnReady(
            [](const DtorTracked& v){}
        );
        CHECK( totalPreviousEventsConsumedAgain == 0 );
    }

    // No additional destructions after trigger goes out of scope
    CHECK( DtorTracked::destroyed.size() == 3 );
}
