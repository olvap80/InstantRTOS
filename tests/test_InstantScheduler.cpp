/** @file tests/test_InstantScheduler.cpp
    @brief Unit tests for core scheduling and multicast listening semantics
*/

#include "InstantScheduler.h"
#include "InstantCallback.h"
#include "doctest/doctest.h"
#include <vector>

TEST_CASE("InstantScheduler: ScheduleAfter same-time ordering and ScheduleBefore precedence"){
    Scheduler sch;
    sch.Start(0);

    std::vector<int> order;

    ActionNode a,b,c,d;
    //CallbackFrom used to ensure lambda lifetime is managed
    a.OnNext(CallbackFrom<1>( [&]{ order.push_back(1); }) );
    b.OnNext(CallbackFrom<1>( [&]{ order.push_back(2); }) );
    c.OnNext(CallbackFrom<1>( [&]{ order.push_back(3); }) );
    d.OnNext(CallbackFrom<1>( [&]{ order.push_back(0); }) );

    // Schedule three with same relative delay (0) using ScheduleAfter
    a.ScheduleAfter(sch, 0);
    b.ScheduleAfter(sch, 0);
    c.ScheduleAfter(sch, 0);

    // Insert d before all items with same time
    d.ScheduleBefore(sch, 0);

    // Execute all at time 0
    CHECK( sch.ExecuteAll(0) );

    // Expect d (0) first, then a,b,c in insertion order
    REQUIRE( order.size() == 4 );
    CHECK( order[0] == 0 );
    CHECK( order[1] == 1 );
    CHECK( order[2] == 2 );
    CHECK( order[3] == 3 );
}

TEST_CASE("InstantScheduler: Periodic self-cancel executes once"){
    Scheduler sch; sch.Start(100);
    int calls = 0;
    ActionNode periodic;
    periodic.OnNext(CallbackFrom<1>( [&]{
        ++calls;
        // Cancel inside callback; also zero period is handled by Cancel semantics
        periodic.Cancel();
    } )).ScheduleAfter(sch, 10, 10); // would repeat every 10 if not canceled

    CHECK( sch.ExecuteAll(109) == false );
    CHECK( sch.ExecuteAll(110) == true );
    CHECK( calls == 1 );
    // Future times should not execute again
    CHECK( sch.ExecuteAll(120) == false );
    CHECK( sch.ExecuteAll(130) == false );
}

TEST_CASE("InstantScheduler: "){
    Scheduler sch;
    sch.Start(100);

    int calls = 0;
    ActionNode periodic;

    periodic.OnNext(CallbackFrom<1>( [&]{
        ++calls;
        // Cancel inside callback; also zero period is handled by Cancel semantics
        //periodic.Cancel();
    } )).ScheduleAfter(sch, 10, 10); // would repeat every 10 if not canceled
/*
    CHECK( sch.ExecuteAll(109) == false );
    CHECK( calls == 0 );
    CHECK( sch.ExecuteAll(110) == true );
    CHECK( calls == 1 );
    CHECK( sch.ExecuteAll(111) == false );
    CHECK( calls == 1 );

    CHECK( sch.ExecuteAll(119) == false );
    CHECK( calls == 1 );
    CHECK( sch.ExecuteAll(120) == true );
    CHECK( calls == 2 );
    CHECK( sch.ExecuteAll(121) == false );
    CHECK( calls == 2 );

    CHECK( sch.ExecuteAll(129) == false );
    CHECK( calls == 2 );
    CHECK( sch.ExecuteAll(130) == true );
    CHECK( calls == 3 );
    CHECK( sch.ExecuteAll(131) == false );
    CHECK( calls == 3 );*/
}

TEST_CASE("InstantScheduler: ListenOnce vs ListenSubscribe and Cancel while listening"){
    MulticastToActions multi;
    Scheduler sch; sch.Start(0);

    int onceCalls = 0;
    int subCalls = 0;
    int cancelCalls = 0;

    ActionNode onceNode, subNode, cancelNode;
    // onceNode should fire only once
    onceNode.OnNext(CallbackFrom<1>([&](){ ++onceCalls; }));
    // subscriber: re-subscription will be manual between calls
    subNode.OnNext(CallbackFrom<1>([&](){ ++subCalls; }));
    cancelNode.OnNext(CallbackFrom<1>([&](){ ++cancelCalls; }));

    onceNode.ListenOnce(multi);
    subNode.ListenSubscribe(multi);
    cancelNode.ListenSubscribe(multi);

    // Cancel the cancelNode before any multicast execution
    cancelNode.Cancel();
    CHECK( !cancelNode.IsListening() );

    multi(); // first round
    CHECK( onceCalls == 1 );
    CHECK( subCalls == 1 );
    CHECK( cancelCalls == 0 );

    // Rearm subscriber callback (required for another invocation)
    subNode.OnNext(CallbackFrom<1>([&](){ ++subCalls; }));
    multi(); // second round
    CHECK( onceCalls == 1 );
    CHECK( subCalls == 2 );

    // Cancel subscription after it fired twice
    subNode.Cancel();
    CHECK( !subNode.IsListening() );
    multi();
    CHECK( subCalls == 2 );
}

TEST_CASE("InstantScheduler: Action schedules itself again inside callback"){
    Scheduler sch; sch.Start(0);
    int calls = 0;
    ActionNode node;

    // First callback
    node.OnNext(CallbackFrom<1>([&](){ ++calls; })).ScheduleAfter(sch, 5);

    CHECK( sch.ExecuteAll(4) == false );
    CHECK( sch.ExecuteAll(5) == true ); // first
    CHECK( calls == 1 );

    // Manually reattach and reschedule
    node.OnNext(CallbackFrom<1>([&](){ ++calls; })).ScheduleAfter(sch,5);
    CHECK( sch.ExecuteAll(10) == true ); // second
    CHECK( calls == 2 );
    // Third
    node.OnNext(CallbackFrom<1>([&](){ ++calls; })).ScheduleAfter(sch,5);
    CHECK( sch.ExecuteAll(15) == true ); // third
    CHECK( calls == 3 );

    CHECK( sch.ExecuteAll(20) == false );
}

TEST_CASE("InstantScheduler: Overflow ordering near wrap boundary"){
    Scheduler sch; sch.Start(0);
    using T = Scheduler::Ticks;
    std::vector<int> order;
    ActionNode first, second;
    // near wrap; schedule first slightly earlier (wrap logic handles ordering)
    T base = std::numeric_limits<T>::max() - 10;
    first.OnNext(CallbackFrom<1>([&](){ order.push_back(1); }));
    second.OnNext(CallbackFrom<1>([&](){ order.push_back(2); }));

    first.ScheduleAfter(sch, base); // absolute time = base
    second.ScheduleAfter(sch, base + 5); // absolute time = base+5 wraps maybe

    // Move time to just before first
    CHECK( sch.ExecuteAll(base - 1) == false );
    CHECK( order.empty() );

    // Execute at first time
    CHECK( sch.ExecuteAll(base) == true );
    CHECK( order.size() == 1 );
    CHECK( order[0] == 1 );

    // Execute at second time
    CHECK( sch.ExecuteAll(base + 5) == true );
    CHECK( order.size() == 2 );
    CHECK( order[1] == 2 );
}
