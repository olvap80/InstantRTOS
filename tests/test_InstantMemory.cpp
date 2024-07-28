/** @file tests/test_InstantMemory.cpp
    @brief Unit tests for InstantMemory.h
*/

#include <exception>
/// Custom exception for testing InstantMemoryPanic
class TestInstantMemoryException: public std::exception{
    const char* what() const noexcept override{
        return "TestInstantIntrusiveListException";
    }
};
//Header will see this definition
#define InstantMemory_Panic() throw TestInstantMemoryException()
#include "InstantMemory.h"
#include "doctest/doctest.h"


#if 0
    //actual block pool (place is allocated statically)
    BlockPool<
        sizeof(SomeClass), //size of single block 
        10        //maximum number of blocks
    > blockPool;

    ...

    void loop() {
        ...

        auto ptr = blockPool.Allocate<SomeClass>();
        ...
        blockPool.Free(ptr);
        ...
    }


    //==============================================================================
    LifetimeManager<SomeClass> manualManagement;
    LifetimeManager<SomeOtherClass> useAsSingleton;

    ...

    void loop() {
        ...

        manualManagement.Emplace("Some parameter", 42); //lifetime started
        ...
        manualManagement->SomeAction(); //continues
        ...
        manualManagement.Destroy(); //ended
        
        ...
        useAsSingleton.Singleton().DoSomethingElse(); //create if not exists
    }

#endif

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

TEST_CASE("InstantMemory BlockPool"){
    constexpr CommonBlockPool::SizeType NumBlocks = 10; 
    BlockPool<sizeof(SomeClass), NumBlocks> blocks;

    CHECK(blocks.BlockSize() == sizeof(SomeClass));
    CHECK(blocks.TotalBlocks() == NumBlocks);
    CHECK(blocks.BlocksAllocated() == 0);

    SUBCASE("BlockPool simple"){
        auto p1 = blocks.MakePtr<SomeClass>(42);
        CHECK(instancesOfSomeClass == 1);
        CHECK(blocks.BlocksAllocated() == 1);
        CHECK(p1->Value() == 42);

        auto p2 = blocks.MakePtr<SomeClass>(43);
        CHECK(instancesOfSomeClass == 2);
        CHECK(blocks.BlocksAllocated() == 2);
        CHECK(p2->Value() == 43);

        blocks.Free(p1);
        CHECK(instancesOfSomeClass == 1);
        CHECK(blocks.BlocksAllocated() == 1);

        auto p3 = blocks.MakePtr<SomeClass>(44);
        CHECK(instancesOfSomeClass == 2);
        CHECK(blocks.BlocksAllocated() == 2);
        CHECK(p3->Value() == 44);

        blocks.Free(p2);
        blocks.Free(p3);
        CHECK(instancesOfSomeClass == 0);
        CHECK(blocks.BlocksAllocated() == 0);

        //======================================================================
        auto p4 = blocks.MakePtr<SomeClass>(45);
        CHECK(instancesOfSomeClass == 1);
        CHECK(blocks.BlocksAllocated() == 1);
        CHECK(p4->Value() == 45);

        {
            constexpr int MaxAllocationsLeft = NumBlocks - 1;
            
            SomeClass* ptrs[MaxAllocationsLeft];
            for(CommonBlockPool::SizeType i = 0; i < MaxAllocationsLeft; ++i){
                ptrs[i] = blocks.MakePtr<SomeClass>(i);
                CHECK(instancesOfSomeClass == i + 2);
                CHECK(blocks.BlocksAllocated() == i + 2);
            }

            CHECK(blocks.BlocksAllocated() == NumBlocks);
            CHECK(blocks.AllocateRaw() == nullptr);
            CHECK_THROWS_AS(blocks.MakePtr<SomeClass>(NumBlocks), TestInstantMemoryException);

            for(CommonBlockPool::SizeType i = 0; i < MaxAllocationsLeft; ++i){
                blocks.Free(ptrs[i]);
                CHECK(instancesOfSomeClass == MaxAllocationsLeft - i);
                CHECK(blocks.BlocksAllocated() == MaxAllocationsLeft - i);
            }

            CHECK(blocks.BlocksAllocated() == 1);
            for(CommonBlockPool::SizeType i = 0; i < MaxAllocationsLeft; ++i){
                ptrs[i] = blocks.MakePtr<SomeClass>(i);
                CHECK(instancesOfSomeClass == i + 2);
                CHECK(blocks.BlocksAllocated() == i + 2);
            }
        }
    }
}


#if 0
// Class to demonstrate using of the BlockPool
class AllocationTest{
public:
    AllocationTest(){
        Serial.print(F("CREATED:"));
        printState();
    }
    AllocationTest(char cInit) :c(cInit) {
        Serial.print(F("CREATED:"));
        printState();
    }
    AllocationTest(char cInit, int iInit, double dInit)
        :c(cInit), i(iInit), d(dInit)
    {
        Serial.print(F("CREATED:"));
        printState();
    }

    ~AllocationTest(){
        Serial.print(F("DESTROYING:"));
        printState();
        //mark as "unused" (for debugging))
        c = '_';
        i = 0;
        d = -1.1;
        Serial.print(F("DESTROYED:"));
        printState();
    }

    void printState() const{
        if( c != '_' ){
            Serial.print(F("AllocationTest instance at "));
            Serial.print(reinterpret_cast<unsigned>(this));
            Serial.print('/');
            Serial.print(c);
            Serial.print('/');
            Serial.print(i);
            Serial.print('/');
            Serial.println(d);
        }
        else{
            Serial.println(F("ALREADY DESTRUCTED"));
        }
    }

private:
    char c = 'A';
    int i = 42;
    double d = 42.42;
};

const unsigned n = sizeof(AllocationTest);

SimpleBlockPool<
    sizeof(AllocationTest),
    5
> blockPool;

LifetimeManager<AllocationTest> lifetimeManager;

void setup() {
    Serial.begin(9600);

    Serial.println(sizeof(AllocationTest));
    Serial.println(sizeof(float));
    Serial.println(sizeof(double));
    Serial.println(alignof(double));
    
    Serial.println(F("----------------------------------"));

    Serial.println(blockPool.BlockSize());
    Serial.println(blockPool.TotalBlocks());
    Serial.println(blockPool.BlocksAllocated());
}

void loop() {
    // put your main code here, to run repeatedly:
    Serial.println(F("\n------------- Iteraton -------------"));

    AllocationTest* p1 = blockPool.Allocate<AllocationTest>();
    AllocationTest* p2 = blockPool.Allocate<AllocationTest>('B');
    //auto also turns to pointer 
    auto p3 = blockPool.Allocate<AllocationTest>('C', 30, 3.3);
    auto p4 = blockPool.Allocate<AllocationTest>('D', 40, 4.4);
    auto p5 = blockPool.Allocate<AllocationTest>('E', 45, 4.5);
    
    //this one will return nullptr because there is no more space
    // since we have only 5 blocks in blockPool
    auto p6 = blockPool.Allocate<AllocationTest>('F');

    Serial.println(blockPool.BlocksAllocated());

    Serial.println(reinterpret_cast<unsigned>(p1));
    Serial.println(reinterpret_cast<unsigned>(p2));
    Serial.println(reinterpret_cast<unsigned>(p3));
    Serial.println(reinterpret_cast<unsigned>(p4));
    Serial.println(reinterpret_cast<unsigned>(p5));
    Serial.println(reinterpret_cast<unsigned>(p6));

    Serial.println(F("--Now free some items and allocate again--"));

    BlockPool::Free(p2);
    BlockPool::Free(p4);

    Serial.println(blockPool.BlocksAllocated());

    //those new items will take place of previ
    auto p7 = blockPool.Allocate<AllocationTest>('G', 50, 5.5);
    auto p8 = blockPool.Allocate<AllocationTest>('H');
    BlockPool::Free(p7);
    auto p9 = blockPool.Allocate<AllocationTest>('I', 50, 5.5);

    //this one will return nullptr because there is no more space
    auto p10 = blockPool.Allocate<AllocationTest>('J', 50, 5.5);
    Serial.println(reinterpret_cast<unsigned>(p10));

    Serial.println(blockPool.BlocksAllocated());
    Serial.println(F("-- Now free everyshing --"));
    BlockPool::Free(p3);
    BlockPool::Free(p1);
    Serial.println(blockPool.BlocksAllocated());
    BlockPool::Free(p5);
    BlockPool::Free(p9);
    //remember this was already destructed BlockPoolBase::Free(p7);
    BlockPool::Free(p8);

    Serial.println(F("-- Everyshing freed --"));
    Serial.println(blockPool.BlocksAllocated());
    Serial.println(F("-- Allocate again --"));

    p1 = blockPool.Allocate<AllocationTest>('K');
    auto r1 = blockPool.AllocateRaw();
    p2 = blockPool.Allocate<AllocationTest>('L', 700, 7.7);
    auto r2 = blockPool.AllocateRaw();
    auto r3 = blockPool.AllocateRaw();
    auto r4 = blockPool.AllocateRaw();

    Serial.println(blockPool.BlocksAllocated());

    Serial.println(reinterpret_cast<unsigned>(p1));
    Serial.println(reinterpret_cast<unsigned>(r1));
    Serial.println(reinterpret_cast<unsigned>(p2));
    Serial.println(reinterpret_cast<unsigned>(r2));
    Serial.println(reinterpret_cast<unsigned>(r3));
    Serial.println(reinterpret_cast<unsigned>(r4));

    Serial.println(blockPool.BlocksAllocated());

    BlockPool::Free(p1);
    BlockPool::FreeRaw(r1);
    BlockPool::Free(p2);
    BlockPool::FreeRaw(r3);
    BlockPool::FreeRaw(r2);

    Serial.println(blockPool.BlocksAllocated());

    Serial.println(F("-- Testing lifetimeManager 1 --"));

    auto& ref = lifetimeManager.Emplace('R');

    ref.printState();
    lifetimeManager->printState();
    (*lifetimeManager).printState();

    lifetimeManager.Destroy();

    //object is destroyed, but use ref for demo purposes
    ref.printState();

    Serial.println(F("-- Testing lifetimeManager 2 (LifetimeManagerScope) --"));

    LifetimeManagerScope(lifetimeManager, 'S', 1233, 12.34){
        
        lifetimeManager->printState();
        (*lifetimeManager).printState();

        lifetimeManager.Destroy();

        //ref is still valid, use it for demo purposes
        ref.printState();
    }
    delay(14200);
}
#endif