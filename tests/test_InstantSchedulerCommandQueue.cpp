/** @file tests/test_InstantSchedulerCommandQueue.cpp
    @brief Unit tests for InstantSchedulerCommandQueue bridge
*/

#include "InstantSchedulerCommandQueue.h"
#include "InstantCallback.h"
#include "doctest/doctest.h"

TEST_CASE("InstantSchedulerCommandQueue: Post schedule and drain executes action"){
    Scheduler sch;
    sch.Start(100);

    SchedulerCommandQueue<8> queue;

    int calls = 0;
    ActionNode node;
    node.OnNext(CallbackFrom<1>([&](){ ++calls; }));

    CHECK( queue.PostScheduleAfter(node, sch, 10) );
    CHECK( queue.Pending() == 1 );

    CHECK( queue.DrainAll() == 1 );
    CHECK( queue.Pending() == 0 );

    CHECK( sch.ExecuteAll(109) == false );
    CHECK( calls == 0 );

    CHECK( sch.ExecuteAll(110) == true );
    CHECK( calls == 1 );
}

TEST_CASE("InstantSchedulerCommandQueue: Deferred cancel removes scheduled action"){
    Scheduler sch;
    sch.Start(0);

    SchedulerCommandQueue<8> queue;

    int calls = 0;
    ActionNode node;
    node.OnNext(CallbackFrom<1>([&](){ ++calls; })).ScheduleAfter(sch, 20);

    CHECK( queue.PostCancel(node) );
    CHECK( queue.DrainOne() );

    CHECK( sch.ExecuteAll(20) == false );
    CHECK( calls == 0 );
}

TEST_CASE("InstantSchedulerCommandQueue: Capacity and wraparound"){
    Scheduler sch;
    sch.Start(0);

    SchedulerCommandQueue<2> queue;

    ActionNode a;
    ActionNode b;
    ActionNode c;

    a.OnNext(CallbackFrom<1>([](){}));
    b.OnNext(CallbackFrom<1>([](){}));
    c.OnNext(CallbackFrom<1>([](){}));

    CHECK( queue.PostScheduleAfter(a, sch, 1) );
    CHECK( queue.PostScheduleAfter(b, sch, 2) );
    CHECK( queue.Pending() == 2 );

    // Full queue rejects new command
    CHECK( queue.PostScheduleAfter(c, sch, 3) == false );

    // Drain one, then post again to verify ring wraparound behavior
    CHECK( queue.DrainOne() );
    CHECK( queue.Pending() == 1 );

    CHECK( queue.PostScheduleAfter(c, sch, 3) );
    CHECK( queue.Pending() == 2 );

    CHECK( queue.DrainAll() == 2 );
    CHECK( queue.Pending() == 0 );

    CHECK( sch.ExecuteAll(1) == true );
    CHECK( sch.ExecuteAll(2) == true );
    CHECK( sch.ExecuteAll(3) == true );
}

TEST_CASE("InstantSchedulerCommandQueue: SchedulerOwnerStep drains and executes"){
    Scheduler sch;
    sch.Start(50);

    SchedulerCommandQueue<4> queue;

    int calls = 0;
    ActionNode node;
    node.OnNext(CallbackFrom<1>([&](){ ++calls; }));

    CHECK( queue.PostScheduleAfter(node, sch, 5) );

    CHECK( SchedulerOwnerStep(sch, queue, 54) == false );
    CHECK( calls == 0 );

    CHECK( SchedulerOwnerStep(sch, queue, 55) == true );
    CHECK( calls == 1 );
}
