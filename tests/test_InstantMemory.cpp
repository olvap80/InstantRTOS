/** @file tests/test_InstantMemory.cpp
    @brief Unit tests for InstantMemory.h
*/

#include <exception>
/// Custom exception for testing InstantMemoryPanic
class TestInstantMemoryException: public std::exception{
    const char* what() const noexcept override{
        return "TestInstantMemoryException";
    }
};
//Header will see this definition
#define InstantMemory_Panic() throw TestInstantMemoryException()
#include "InstantMemory.h"
#include "doctest/doctest.h"
#include <tuple>


namespace{
    int instancesOfSomeClass = 0;
    class SomeClass{
    public:
        SomeClass(int value): value(value){
            ++instancesOfSomeClass;
        }
        ~SomeClass(){
            --instancesOfSomeClass;
        }
        int Value() const { return value; }
    private:
        int value;
    };
} //namespace


TEST_CASE("InstantMemory BlockPool (allocation/deallocation)"){
    constexpr MemoryBlocks::SizeType NumBlocks = 10;
    BlockPool<sizeof(SomeClass), NumBlocks> blocks;

    CHECK(blocks.BlockSize() == sizeof(SomeClass));
    REQUIRE(blocks.TotalBlocks() == NumBlocks);
    REQUIRE(blocks.BlocksAllocated() == 0);

    SUBCASE("BlockPool simple"){
        auto p1 = blocks.MakePtr<SomeClass>(42);
        CHECK(instancesOfSomeClass == 1);
        CHECK(blocks.BlocksAllocated() == 1);
        CHECK(p1->Value() == 42);

        auto p2 = blocks.MakePtr<SomeClass>(43);
        CHECK(instancesOfSomeClass == 2);
        CHECK(blocks.BlocksAllocated() == 2);
        CHECK(p2->Value() == 43);

        MemoryBlocks::Free(p1);
        CHECK(instancesOfSomeClass == 1);
        CHECK(blocks.BlocksAllocated() == 1);

        auto p3 = blocks.MakePtr<SomeClass>(44);
        CHECK(instancesOfSomeClass == 2);
        CHECK(blocks.BlocksAllocated() == 2);
        CHECK(p3->Value() == 44);

        MemoryBlocks::Free(p2);
        MemoryBlocks::Free(p3);
        CHECK(instancesOfSomeClass == 0);
        CHECK(blocks.BlocksAllocated() == 0);

        //======================================================================
        auto p4 = blocks.MakePtr<SomeClass>(45);
        CHECK(instancesOfSomeClass == 1);
        CHECK(blocks.BlocksAllocated() == 1);
        CHECK(p4->Value() == 45);

        { //check allocation limit
            constexpr int MaxAllocationsLeft = NumBlocks - 1;

            SomeClass* ptrs[MaxAllocationsLeft];
            //allocate all blocks
            for(MemoryBlocks::SizeType i = 0; i < MaxAllocationsLeft; ++i){
                ptrs[i] = blocks.MakePtr<SomeClass>(i);
                CHECK(instancesOfSomeClass == i + 2);
                CHECK(blocks.BlocksAllocated() == i + 2);
            }
            //no more space
            CHECK(blocks.BlocksAllocated() == NumBlocks);
            CHECK(blocks.AllocateRaw() == nullptr);
            CHECK_THROWS_AS(blocks.MakePtr<SomeClass>(NumBlocks), TestInstantMemoryException);

            //free all blocks in straight order
            for(MemoryBlocks::SizeType i = 0; i < MaxAllocationsLeft; ++i){
                MemoryBlocks::Free(ptrs[i]);
                CHECK(instancesOfSomeClass == MaxAllocationsLeft - i);
                CHECK(blocks.BlocksAllocated() == MaxAllocationsLeft - i);
            }
            //endure only p4 is there
            CHECK(blocks.BlocksAllocated() == 1);
            for(MemoryBlocks::SizeType i = 0; i < MaxAllocationsLeft; ++i){
                ptrs[i] = blocks.MakePtr<SomeClass>(i);
                CHECK(instancesOfSomeClass == i + 2);
                CHECK(blocks.BlocksAllocated() == i + 2);
            }
            //no more space
            CHECK(blocks.BlocksAllocated() == NumBlocks);
            CHECK(blocks.AllocateRaw() == nullptr);
            CHECK_THROWS_AS(blocks.MakePtr<SomeClass>(NumBlocks), TestInstantMemoryException);

            //free all blocks in reverse order
            for(MemoryBlocks::SizeType i = 0; i < MaxAllocationsLeft; ++i){
                MemoryBlocks::Free(ptrs[MaxAllocationsLeft - i - 1]);
                CHECK(instancesOfSomeClass == MaxAllocationsLeft - i);
                CHECK(blocks.BlocksAllocated() == MaxAllocationsLeft - i);
            }
        }

        //ensure only p4 is there
        CHECK(blocks.BlocksAllocated() == 1);
        MemoryBlocks::Free(p4);
        CHECK(instancesOfSomeClass == 0);
        CHECK(blocks.BlocksAllocated() == 0);
    }
}


namespace{
    //Assume all tests execute in the single thread sequentially
    int instancesOfAllocationTest = 0;

    class AllocationTest{
    public:
        AllocationTest(){++instancesOfAllocationTest;}
        AllocationTest(char cInit) :c(cInit) {++instancesOfAllocationTest;}
        AllocationTest(char cInit, int iInit, double dInit)
            : c(cInit), i(iInit), d(dInit) {++instancesOfAllocationTest;}

        ~AllocationTest(){
            //Prevent optimization of unused variables
            volatile char* p = reinterpret_cast<volatile char*>(&c);
            std::memset(const_cast<char*>(p), 0, sizeof(c));
            p = reinterpret_cast<volatile char*>(&i);
            std::memset(const_cast<char*>(p), 0, sizeof(i));
            p = reinterpret_cast<volatile char*>(&d);
            std::memset(const_cast<char*>(p), 0, sizeof(d));

            //decrement instance counter
            --instancesOfAllocationTest;

            //mark as "unused" (for debugging purposes, eliminated by optimization in release))
            c = '_';
            i = 0;
            d = -1.1;
        }

        std::tuple<char, int, double> State() const{
            return std::make_tuple(c, i, d);
        }

    private:
        char c = 'A';
        int i = 42;
        double d = 42.42;
    };
} //namespace


TEST_CASE("InstantMemory LifetimeManager and LifetimeManagerScope"){
    LifetimeManager<AllocationTest> lifetimeManager;
    REQUIRE(instancesOfAllocationTest == 0);

    auto& ref = lifetimeManager.Emplace('R');
    CHECK(lifetimeManager);
    CHECK(instancesOfAllocationTest == 1);
    CHECK(ref.State() == std::make_tuple('R', 42, 42.42));

    CHECK(lifetimeManager->State() == std::make_tuple('R', 42, 42.42));
    CHECK((*lifetimeManager).State() == std::make_tuple('R', 42, 42.42));

    lifetimeManager.Destroy();
    REQUIRE(!lifetimeManager);
    REQUIRE(instancesOfAllocationTest == 0); //object is destroyed

    //destroying again should not panic
    lifetimeManager.Destroy();

    LifetimeManagerScope(lifetimeManager, 'S', 1233, 12.34){
        CHECK(lifetimeManager);
        CHECK(lifetimeManager->State() == std::make_tuple('S', 1233, 12.34));
        CHECK((*lifetimeManager).State() == std::make_tuple('S', 1233, 12.34));
        CHECK(instancesOfAllocationTest == 1);
    }
    REQUIRE(!lifetimeManager);
    REQUIRE(instancesOfAllocationTest == 0); //object is automatically destroyed

    SUBCASE("DestroyOrPanic panics when already destroyed"){
        CHECK_THROWS_AS(lifetimeManager.DestroyOrPanic(), TestInstantMemoryException);
    }
    SUBCASE("Emplace again and Force"){
        lifetimeManager.Emplace('T', 12345, 12.345);
        REQUIRE(lifetimeManager);
        CHECK(instancesOfAllocationTest == 1);
        CHECK(lifetimeManager->State() == std::make_tuple('T', 12345, 12.345));
        CHECK((*lifetimeManager).State() == std::make_tuple('T', 12345, 12.345));

        CHECK_THROWS_AS(lifetimeManager.Emplace('U', 12, 1.2), TestInstantMemoryException);
        REQUIRE(lifetimeManager);
        REQUIRE(instancesOfAllocationTest == 1);
        //old object is still there
        CHECK(lifetimeManager->State() == std::make_tuple('T', 12345, 12.345));

        lifetimeManager.Force('U', 122, 1.23);
        REQUIRE(lifetimeManager);
        CHECK(instancesOfAllocationTest == 1);
        CHECK(lifetimeManager->State() == std::make_tuple('U', 122, 1.23));

        lifetimeManager.DestroyOrPanic();
        REQUIRE(instancesOfAllocationTest == 0);

        //cannot access destroyed object
        CHECK_THROWS_AS(lifetimeManager->State(), TestInstantMemoryException);
        CHECK_THROWS_AS(*lifetimeManager, TestInstantMemoryException);
    }

    SUBCASE("Force on empty constructs new instance"){
        CHECK(!lifetimeManager);
        lifetimeManager.Force('F', 321, 3.21);
        CHECK(lifetimeManager);

        CHECK(instancesOfAllocationTest == 1);
        CHECK(lifetimeManager->State() == std::make_tuple('F', 321, 3.21));
        lifetimeManager.Destroy();

        CHECK(!lifetimeManager);
        CHECK(instancesOfAllocationTest == 0);
    }

}

namespace{
    class AllocationTestSingleton: public AllocationTest{
    public:
        using AllocationTest::AllocationTest;
        AllocationTestSingleton()
            : AllocationTest('A', 42, 42.42) {}
    };
}


TEST_CASE("InstantMemory InstantSingleton shall return the same instance"){
    {
    InstantSingleton<AllocationTestSingleton> useAsSingleton;
        REQUIRE(instancesOfAllocationTest == 0);

        //For the first time, a new instance is created
        CHECK(useAsSingleton->State() == std::make_tuple('A', 42, 42.42));
        CHECK(instancesOfAllocationTest == 1);

        //Again should return the same instance
        CHECK(useAsSingleton->State() == std::make_tuple('A', 42, 42.42));
        CHECK(instancesOfAllocationTest == 1);

        //and again
        CHECK(useAsSingleton->State() == std::make_tuple('A', 42, 42.42));
        CHECK(instancesOfAllocationTest == 1);

        //force destroy the instance
        useAsSingleton.Destroy();
        CHECK(instancesOfAllocationTest == 0);

        //should create a new instance after destroy
        CHECK(useAsSingleton->State() == std::make_tuple('A', 42, 42.42));
        CHECK(instancesOfAllocationTest == 1);

        //should return the same instance
        CHECK(useAsSingleton->State() == std::make_tuple('A', 42, 42.42));
        CHECK(instancesOfAllocationTest == 1);

        //and again
        CHECK(useAsSingleton->State() == std::make_tuple('A', 42, 42.42));
        CHECK(instancesOfAllocationTest == 1);
    }
    //destroyed instance once LifetimeManager goes out of scope
    CHECK(instancesOfAllocationTest == 0);
}


TEST_CASE("InstantMemory BlockPool extended"){
    constexpr MemoryBlocks::SizeType NumBlocks = 5;
    BlockPool<sizeof(AllocationTest), NumBlocks> blocks;

    CHECK(blocks.BlockSize() == sizeof(AllocationTest));
    REQUIRE(blocks.TotalBlocks() == NumBlocks);
    REQUIRE(blocks.BlocksAllocated() == 0);

    AllocationTest* p1 = blocks.MakePtr<AllocationTest>();
    CHECK(instancesOfAllocationTest == 1);
    CHECK(blocks.BlocksAllocated() == 1);
    AllocationTest* p2 = blocks.MakePtr<AllocationTest>('B');
    CHECK(instancesOfAllocationTest == 2);
    CHECK(blocks.BlocksAllocated() == 2);
    //auto also turns to pointer
    auto p3 = blocks.MakePtr<AllocationTest>('C', 30, 3.3);
    CHECK(instancesOfAllocationTest == 3);
    CHECK(blocks.BlocksAllocated() == 3);
    auto p4 = blocks.MakePtr<AllocationTest>('D', 40, 4.4);
    CHECK(instancesOfAllocationTest == 4);
    CHECK(blocks.BlocksAllocated() == 4);
    auto p5 = blocks.MakePtr<AllocationTest>('E', 45, 4.5);
    CHECK(instancesOfAllocationTest == 5);
    CHECK(blocks.BlocksAllocated() == 5);

    //this one will return nullptr because there is no more space
    // since we have only 5 blocks in blockPool
    CHECK(blocks.AllocateRaw() == nullptr);
    CHECK_THROWS_AS(blocks.MakePtr<AllocationTest>('F', 50, 5.5), TestInstantMemoryException);

    //verify that all items are in place and did not overlap (have correct values)
    CHECK(p1->State() == std::make_tuple('A', 42, 42.42));
    CHECK(p2->State() == std::make_tuple('B', 42, 42.42));
    CHECK(p3->State() == std::make_tuple('C', 30, 3.3));
    CHECK(p4->State() == std::make_tuple('D', 40, 4.4));
    CHECK(p5->State() == std::make_tuple('E', 45, 4.5));

    CHECK(instancesOfAllocationTest == 5);

    //Now free some items and allocate again
    MemoryBlocks::Free(p2);
    CHECK(instancesOfAllocationTest == 4);
    MemoryBlocks::Free(p4);
    CHECK(instancesOfAllocationTest == 3);
    CHECK(blocks.BlocksAllocated() == 3);

    //verify that other items are untouched
    CHECK(p1->State() == std::make_tuple('A', 42, 42.42));
    CHECK(p3->State() == std::make_tuple('C', 30, 3.3));
    CHECK(p5->State() == std::make_tuple('E', 45, 4.5));


    //those new items will take place of previously freed
    auto p7 = blocks.MakePtr<AllocationTest>('G', 50, 5.5);
    CHECK(instancesOfAllocationTest == 4);
    auto p8 = blocks.MakePtr<AllocationTest>('H');
    CHECK(instancesOfAllocationTest == 5);
    CHECK(blocks.BlocksAllocated() == 5);

    //this one will return nullptr because there is no more space
    CHECK(blocks.AllocateRaw() == nullptr);
    CHECK_THROWS_AS(blocks.MakePtr<AllocationTest>('I', 50, 5.5), TestInstantMemoryException);

    //verify that all items are in place and did not overlap (have correct values)
    CHECK(p1->State() == std::make_tuple('A', 42, 42.42));
    CHECK(p3->State() == std::make_tuple('C', 30, 3.3));
    CHECK(p5->State() == std::make_tuple('E', 45, 4.5));
    CHECK(p7->State() == std::make_tuple('G', 50, 5.5));
    CHECK(p8->State() == std::make_tuple('H', 42, 42.42));


    MemoryBlocks::Free(p7);
    CHECK(instancesOfAllocationTest == 4);
    CHECK(blocks.BlocksAllocated() == 4);
    auto p9 = blocks.MakePtr<AllocationTest>('I', 50, 5.5);
    CHECK(instancesOfAllocationTest == 5);
    CHECK(blocks.BlocksAllocated() == 5);

    //verify that all items are in place and did not overlap (have correct values)
    CHECK(p1->State() == std::make_tuple('A', 42, 42.42));
    CHECK(p3->State() == std::make_tuple('C', 30, 3.3));
    CHECK(p5->State() == std::make_tuple('E', 45, 4.5));
    CHECK(p8->State() == std::make_tuple('H', 42, 42.42));
    CHECK(p9->State() == std::make_tuple('I', 50, 5.5));


    //this one will panic because there is no more space
    CHECK_THROWS_AS(blocks.MakePtr<AllocationTest>('J', 50, 5.5), TestInstantMemoryException);
    CHECK(blocks.BlocksAllocated() == 5);

    MemoryBlocks::Free(p3);
    CHECK(instancesOfAllocationTest == 4);
    CHECK(blocks.BlocksAllocated() == 4);
    MemoryBlocks::Free(p1);
    CHECK(instancesOfAllocationTest == 3);
    CHECK(blocks.BlocksAllocated() == 3);
    MemoryBlocks::Free(p5);
    CHECK(instancesOfAllocationTest == 2);
    CHECK(blocks.BlocksAllocated() == 2);
    MemoryBlocks::Free(p9);
    CHECK(instancesOfAllocationTest == 1);
    CHECK(blocks.BlocksAllocated() == 1);
    //remember p7 was already destructed and freed before
    MemoryBlocks::Free(p8);
    CHECK(instancesOfAllocationTest == 0);
    CHECK(blocks.BlocksAllocated() == 0);


    //go allocating again

    p1 = blocks.MakePtr<AllocationTest>('K');
    CHECK(instancesOfAllocationTest == 1);
    CHECK(blocks.BlocksAllocated() == 1);

    auto r1 = blocks.AllocateRaw(); //this if not initialized
    CHECK(instancesOfAllocationTest == 1); //no change
    CHECK(blocks.BlocksAllocated() == 2); //one more block allocated

    p2 = blocks.MakePtr<AllocationTest>('L', 700, 7.7);
    CHECK(instancesOfAllocationTest == 2); //one more instance
    CHECK(blocks.BlocksAllocated() == 3); //one more block allocated

    auto r2 = blocks.AllocateRaw();
    CHECK(instancesOfAllocationTest == 2); //no change
    CHECK(blocks.BlocksAllocated() == 4); //one more block allocated
    auto r3 = blocks.AllocateRaw();
    CHECK(instancesOfAllocationTest == 2); //no change
    CHECK(blocks.BlocksAllocated() == 5); //one more block allocated
    auto r4 = blocks.AllocateRaw();
    CHECK(instancesOfAllocationTest == 2); //no change
    CHECK(blocks.BlocksAllocated() == 5); //one more block allocated

    //verify p1, p2 have correct values
    CHECK(p1->State() == std::make_tuple('K', 42, 42.42));
    CHECK(p2->State() == std::make_tuple('L', 700, 7.7));
    CHECK(blocks.BlocksAllocated() == 5); //one more block allocated

    //then free in reverse order
    MemoryBlocks::Free(p1);
    CHECK(instancesOfAllocationTest == 1);
    CHECK(blocks.BlocksAllocated() == 4);
    CHECK(p2->State() == std::make_tuple('L', 700, 7.7));
    MemoryBlocks::FreeRaw(r1);
    CHECK(instancesOfAllocationTest == 1);
    CHECK(blocks.BlocksAllocated() == 3);
    CHECK(p2->State() == std::make_tuple('L', 700, 7.7));
    MemoryBlocks::Free(p2);
    CHECK(instancesOfAllocationTest == 0);
    CHECK(blocks.BlocksAllocated() == 2);
    MemoryBlocks::FreeRaw(r3);
    CHECK(instancesOfAllocationTest == 0);
    CHECK(blocks.BlocksAllocated() == 1);
    MemoryBlocks::FreeRaw(r2);
    CHECK(instancesOfAllocationTest == 0);
    CHECK(blocks.BlocksAllocated() == 0);
}

TEST_CASE("InstantMemory UniqueBlock basic and move semantics"){
    // Use small pool (need at least 2 blocks for move-assignment swap test)
    constexpr MemoryBlocks::SizeType NumBlocks = 4;
    BlockPool<sizeof(AllocationTest), NumBlocks> pool;

    CHECK(instancesOfAllocationTest == 0);

    SUBCASE("RAII allocation and destruction"){
        {
            UniqueBlock<AllocationTest> u1(pool, 'X', 900, 9.0);
            CHECK(instancesOfAllocationTest == 1);
            //test content is still there as expected
            CHECK(u1->State() == std::make_tuple('X', 900, 9.0));
        }
        // Destructor should have freed that block
        CHECK(instancesOfAllocationTest == 0);
        // and pool shall be empty again
        CHECK(pool.BlocksAllocated() == 0);
    }

    SUBCASE("Move construction transfers ownership (source becomes empty)"){
        {
            UniqueBlock<AllocationTest> original(pool, 'M', 100, 1.0);
            CHECK(instancesOfAllocationTest == 1);
            auto* addrOriginal = &(*original);

            // Move construction transfers ownership
            UniqueBlock<AllocationTest> movedHere(std::move(original));

            // instance count unchanged (still one live object)
            CHECK(instancesOfAllocationTest == 1);
            // movedHere now points to original allocation
            CHECK(&(*movedHere) == addrOriginal);
            // let movedHere go out of scope to free
            // (We intentionally do not dereference original after move)

            //There shall be no references to original after move
            CHECK(!original);
        }
        // Destructor should have freed that block
        CHECK(instancesOfAllocationTest == 0);
        // and pool shall be empty again
        CHECK(pool.BlocksAllocated() == 0);
    }

    SUBCASE("Move assignment"){
        {
            // Create two distinct allocations with different states
            UniqueBlock<AllocationTest> b(pool, 'B', 20, 2.2);
            CHECK(instancesOfAllocationTest == 1);
            auto* oldAddrB = &(*b);
            CHECK(b->State() == std::make_tuple('B', 20, 2.2));

            auto makeTemporary = [&]{
                UniqueBlock<AllocationTest> a(pool, 'A', 10, 1.1);
                CHECK(instancesOfAllocationTest == 2);
                auto* addrA = &(*a);
                CHECK(a->State() == std::make_tuple('A', 10, 1.1));
                CHECK(b->State() == std::make_tuple('B', 20, 2.2));
                return a;
            };

            b = makeTemporary();

            // Temporary object shall be destroyed here, b takes its place
            CHECK(instancesOfAllocationTest == 1);
            // B points to new location
            CHECK(&(*b) != oldAddrB);
            CHECK(b->State() == std::make_tuple('A', 10, 1.1));
        }
        // Destructor should have freed that block
        CHECK(instancesOfAllocationTest == 0);
        // and pool shall be empty again
        CHECK(pool.BlocksAllocated() == 0);
    }
}
