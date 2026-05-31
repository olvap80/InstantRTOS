/** @file InstantMemory.h
    @brief Simple deterministic memory management utilities suitable for real time
    can be used for dynamic memory allocations on Arduino and similar platforms.

(c) see https://github.com/olvap80/InstantRTOS

Zero dependencies, works instantly by copy pasting to your project...
Inspired by memory management tool set available in various RTOSes,
and now available in pure C++ :)

The BlockPool is intended to be allocated statically and then used
for memory allocation.

Sample usage
 @code
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
 @endcode

The LifetimeManager can be used to explicitly allocate/deallocate single
instance (manage object lifetime manually), see sample:

 @code
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
 @endcode

There is a LifetimeManagerScope macro to allow RAII for LifetimeManager,
so that you do not need to call Emplace and Destroy manually.
The LifetimeManagerScope does not create variable on stack, thus it is
ideal option to work in pair with InstantCoroutine.h allowing object
life time management and coroutine nesting, see sample below:
 @code
    #include "InstantCoroutine.h"
    #include "InstantMemory.h"

    CoroutineDefine( SequenceOfSquares ) {
        int i = 0;
        CoroutineBegin(int)
            for( ;; ++i ){
                CoroutineYield( i*i );
            }
        CoroutineEnd()
    };
    CoroutineDefine( SequenceOfCubes ) {
        int i = 0;
        CoroutineBegin(int)
            for( ;; ++i ){
                CoroutineYield( i*i*i );
            }
        CoroutineEnd()
    };

    template<class T>
    CoroutineDefine( Range ) {
        T current, last;
    public:
        Range(T beginFrom, T endWith) : current(beginFrom), last(endWith) {}

        CoroutineBegin(T)
            for(; current < last; ++current){
                CoroutineYield( current );
            }
            CoroutineStop(last);
        CoroutineEnd()
    };

    CoroutineDefine( UseOtherCoroutines ) {
        LifetimeManager< Range<int8_t> > range;
        LifetimeManager<SequenceOfSquares> sequenceOfSquares;
        LifetimeManager<SequenceOfCubes> sequenceOfCubes;

        CoroutineBegin(void)
            for( ;; ){
                Serial.println(F("------ ITERATION STARTED -----"));
                Serial.println(F("Printing squares:"));
                LifetimeManagerScope(sequenceOfSquares){
                    LifetimeManagerScope(range, 0, 10){
                        while( *range ){
                            Serial.print( (*range)() );
                            Serial.print( ':' );
                            Serial.println( (*sequenceOfSquares)() );
                            CoroutineYield();
                        }
                    }
                }

                Serial.println(F("Printing cubes:"));
                CoroutineYield();

                LifetimeManagerScope(sequenceOfCubes){
                    LifetimeManagerScope(range, 0, 15){
                        while( *range ){
                            Serial.print( (*range)() );
                            Serial.print( ':' );
                            Serial.println( (*sequenceOfCubes)() );
                            CoroutineYield();
                        }
                    }
                }
            }
        CoroutineEnd()
    };

    //The coroutine using other coroutines
    UseOtherCoroutines useOtherCoroutines;

    void setup() {
        Serial.begin(9600);
    }

    void loop() {
        useOtherCoroutines();
        delay( 200 );
    }
 @endcode
Above we overcome Coroutine limitations with LifetimeManagerScope,
The code "looks like this synchronous"
despite being internally split into states with CoroutineYield() calls.


NOTE: InstantMemory.h is configurable for interrupt (thread) safety.
      It is always safe to use the same object from the same thread.
      (different objects used from different threads will work as well).
      It is safe to use the same object from different threads/interrupts
      only if that interrupt (thread) safety is configured, see below


Deterministic memory allocation ald lifetime management suitable for real time
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

#ifndef InstantMemory_INCLUDED_H
#define InstantMemory_INCLUDED_H


//______________________________________________________________________________
// Portable configuration (just skip to "Classes for memory operations" below))

/* Keep our promise to not use any standard libraries by default,
   but types from those header still must have */
#if defined(InstantRTOS_USE_STDLIB) || defined(__has_include)

#   if __has_include(<cstddef>)
#       include <cstddef>
        using std::size_t;
#       define INSTANTMEMORY_SIZE_T size_t
#   else
#       include <stddef.h>
#       define INSTANTMEMORY_SIZE_T size_t
#   endif

#   if __has_include(<cstdint>)
#       include <cstdint>
        //remember uintptr_t is optional per https://en.cppreference.com/w/cpp/types/integer
        using std::uintptr_t;
#       define INSTANTMEMORY_UINTPTR_T uintptr_t
#   else
#       include <stdint.h>
#       define INSTANTMEMORY_UINTPTR_T uintptr_t
#   endif

#   if __has_include(<new>)
        //this header is present even on avr
#       include <new>
        //hack to replace class for the case when custom placement new is not needed
#       define InstantMemoryPlaceholderHelper(ptr) ptr
#   else
#       include <new.h>
        //hack to replace class for the case when custom placement new is not needed
#       define InstantMemoryPlaceholderHelper(ptr) ptr
#   endif

#endif


#if !defined(INSTANTMEMORY_SIZE_T)
#   define INSTANTMEMORY_SIZE_T unsigned
    static_assert(
        sizeof(INSTANTMEMORY_SIZE_T) == sizeof( sizeof(INSTANTMEMORY_SIZE_T) ),
        "The INSTANTMEMORY_SIZE_T shall have the same size as the result of sizeof"
    );
#endif
#if !defined(INSTANTMEMORY_UINTPTR_T)
#   define INSTANTMEMORY_UINTPTR_T unsigned
    static_assert(
        sizeof(INSTANTMEMORY_UINTPTR_T) >= sizeof( void* ),
        "The INSTANTMEMORY_UINTPTR_T shall be large enough to hold pointer"
    );
#endif

#if !defined(InstantMemoryPlaceholderHelper)
    /// Helper class to allow custom placement new without conflicts
    class InstantMemoryPlaceholderHelper{
    public:
        InstantMemoryPlaceholderHelper(void *placeForAllocation) : ptr(placeForAllocation) {}
    private:
        void *ptr;
        friend void* operator new(INSTANTMEMORY_SIZE_T, InstantMemoryPlaceholderHelper place) noexcept;
    };

    /** own placement new implementation.
     see https://en.cppreference.com/w/cpp/memory/new/operator_new */
    inline void* operator new(INSTANTMEMORY_SIZE_T, InstantMemoryPlaceholderHelper place) noexcept{
        return place.ptr;
    }
    /// Make compiler happy with explicit empty delete operator
    inline void operator delete(void*, InstantMemoryPlaceholderHelper place) noexcept{
        //do nothing, as we do not allocate memory
    }
#endif


//______________________________________________________________________________
// Configurable error handling and interrupt safety

/* Common configuration to be included only if available
   (you can separate file and/or configure individually
    or just skip that to stick with defaults) */
#if defined(__has_include)
#   if __has_include("InstantRTOS.Config.h")
#       include "InstantRTOS.Config.h"
#   endif
#endif

#ifndef InstantMemory_Panic
#   ifdef InstantRTOS_Panic
#       define InstantMemory_Panic() InstantRTOS_Panic('M')
#   else
#       define InstantMemory_Panic() /* you can customize here! */ do{}while(true)
#   endif
#endif


#ifndef InstantMemory_EnterCritical
#   if defined(InstantRTOS_EnterCritical) && !defined(InstantMemory_SuppressEnterCritical)
        //we have access from interrupts and/or multithreading
#       define InstantMemory_EnterCritical InstantRTOS_EnterCritical
#       define InstantMemory_LeaveCritical InstantRTOS_LeaveCritical
#       if defined(InstantRTOS_MutexObjectType)
#           define InstantMemory_MutexObjectType InstantRTOS_MutexObjectType
#           define InstantMemory_MutexObjectVariable InstantRTOS_MutexObjectVariable
#       endif
#   else
#       if 1
            //no interrupts/multithreading, so optimize it out where possible
#           define InstantMemory_NoMultithreadingProtection
            //do not change in this branch, activate next branch to tune manually
#           define InstantMemory_EnterCritical
#           define InstantMemory_LeaveCritical
#       else
            //place for custom implementation for InstantMemory.h
            //(usually not needed to get here, is mutually )
#           define InstantMemory_EnterCritical
#           define InstantMemory_LeaveCritical
#       endif
#   endif
#endif

//Uncomment below to allow checking for memory access in -> and * operators
//#define InstantMemory_CheckAccess

//Uncomment below to enable singleton thread safety
//#define InstantMemory_SingletonThreadsafe


//______________________________________________________________________________
// Classes for memory operations - BlockPool and variations


/** Optimized base used for all kinds of BlockPools below.
 All various BlockPools reuse the same implementation
 to save program space being spent on allocation/deallocation logic.
 Use BlockPool, SharedAllocator, or SmartAllocator (TBD) below,
 instead of this class.
 (NOTE: single implementation to not duplicate code for each type) */
class MemoryBlocks{
public:
    //all the copying is banned
    constexpr MemoryBlocks(const MemoryBlocks&) = delete;
    MemoryBlocks& operator =(const MemoryBlocks&) = delete;

    //NOTE: for actual allocation API see corresponding derived classes


    ///Type to represent memory sizes
    using SizeType = INSTANTMEMORY_SIZE_T;
    ///Aliased "simplest" type to use for "raw" memory
    using ByteType = unsigned char;


    /// How much bytes are available in custom part of block
    constexpr SizeType BlockSize() const;
    /// Maximum number of blocks available for allocation
    constexpr SizeType TotalBlocks() const;
    /// How many blocks are allocated so far
    constexpr SizeType BlocksAllocated() const;

    /** "Raw" allocate of any free block, returns null when no more blocks.
     Obtain raw uninitialized bytes (entire block), no constructor is called.
     One can use Make* API from derived classes for allocation
     nullptr is just ignored here */
    void* AllocateRaw();

    /** Free raw bytes, do not issue any destructors!
     Free entire block as "just bytes" without initialization,
     pointer must come from AllocateRaw/MakePtr of a MemoryBlocks instance */
    static void FreeRaw(void* memoryPreviouslyAllocatedByBlockPool);


    /** "Raw" deallocation with destructor, panic if not allocated here.
     Free block and call destructor of TBeingPlacedWhileAllocating,
     pointer must come from AllocateRaw/MakePtr of a MemoryBlocks instance.
     One can use Make* API from derived classes for allocation
     nullptr is just ignored here */
    template<class TBeingPlacedWhileAllocating>
    static void Free(TBeingPlacedWhileAllocating* correspondingAllocatedObject);

    /** Metadata to be placed at the beginning of each block.
     Aso use this for determining minimum alignment requirements.
     Always goes just before custom part of the block,
     appending more metadata.
     It is up to derived class to ensure with EntireBlockSize(...)
     that entireBlockSizeUsed is large enough to hold metadata */
    class Metadata{
    private:
        friend class MemoryBlocks; ///<The only one who can access internals
        union{
            ByteType* next; ///<When free, points to the next free block
            MemoryBlocks* owner; ///<As allocated, points to MemoryBlocks
        };
    };

protected:
    /// @Make gcc happy with C++ 11
    constexpr MemoryBlocks() = delete;

    /** Initialize MemoryBlocks logic.
     One must be sure memoryArea contains at least
     entireBlockSizeUsed*totalBlocksAvailable bytes!
     Metadata is added to the end of */
    MemoryBlocks(
        ByteType* memoryArea, ///< MemoryBlocks will place allocated blocks here,
                              ///< there shall be space enough to hold
                              ///< entireBlockSizeUsed*totalBlocksAvailable bytes
        SizeType customBlockSizeUsed, ///Custom part in the block (without metadata)
        SizeType entireBlockSizeUsed, ///<Entire block size (as calculated by EntireBlockSize)
        SizeType totalBlocksAvailable ///<Total number of full blocks reserved
    );

    /// Helper to use in assertions for type sizes and alignments
    template<SizeType N>
    struct IsPowerOfTwo{
        static constexpr bool value = (N > 0) && !(N & (N - 1)); };
    static_assert(  IsPowerOfTwo<sizeof(Metadata)>::value,
                    "sizeof(Metadata) shall be power of two" );

    /** Calculate size of the block together with helper information (Metadata).
     The EntireBlockSize takes into account place for Metadata
     and ensures Metadata alignment does not violate requestedAlignment.
     It is assumed that customBlockSizeRequested is already of
     size compatible with requestedAlignment,
     and that both requestedAlignment and sizeof(Metadata) are power of two.
     Use BlockPool for allocation instead of manual memory management */
    static constexpr SizeType EntireBlockSize(
        SizeType customBlockSizeRequested, SizeType requestedAlignment);

private:
    /// "Magic" constant to mark MemoryBlocks instances for debugging purposes
    static constexpr SizeType MarkToTest = 24991;
    static_assert(
        sizeof(SizeType) >= 2,
        "SizeType shall be at least 2 bytes"
    );
    /// Mark MemoryBlocks instances for debugging/error detection purposes
    /* This is not a 100% warranty, just a way to increase detection chances.
       "Magic" constant here is checked at free time to
       identify invalid usage or double frees.
       No need to be bulletproof here, just use BlockPool/UniqueBlock below :)
       instead of any manual memory management */
    const SizeType mark = MarkToTest;

    /// Custom part of the block (without metadata)
    const SizeType customBlockSize;
    /// Entire block size (as calculated by EntireBlockSize)
    const SizeType entireBlockSize;

    /// Maximum number of blocks available for allocation
    const SizeType totalBlocks;
    /// How much blocks were allocated so far
    SizeType blocksAllocated = 0;

    /// Pointer to the "custom memory" of the first free block
    ByteType* firstFree;

    /// One must include metadata without spoiling alignment (internal API)
    static constexpr SizeType entireAlignedBlockSize(
        SizeType customBlockSizeRequested, ///< At least this size is provided
        SizeType alignmentWithMetadata ///< Metadata and alignment total requirements
                                       ///< at least this size before the custom part
    );

    #if !defined(InstantMemory_NoMultithreadingProtection) && defined(InstantMemory_MutexObjectType)
        InstantMemory_MutexObjectType InstantMemory_MutexObjectVariable;
    #endif
};


/** Simple pool to allocate fixed size blocks.
 Allow allocation of raw pointer to fixed size memory blocks */
template<
    MemoryBlocks::SizeType SingleBlockSizeRequested,
    MemoryBlocks::SizeType TotalNumBlocks,
    class AlignAsType = MemoryBlocks::Metadata
>
class BlockPool: public MemoryBlocks{
    static_assert(
        SingleBlockSizeRequested % alignof(AlignAsType) == 0,
        "MemoryBlocks: ensure your blocks are of proper size allowing proper alignment"
    );
public:
    /// Create ready to use BlockPool
    constexpr BlockPool();

    /** Allocation with simultaneous construction (and static check for size)).
     Use MakePtr to allocate and construct object of TBeingPlacedWhileAllocating
     in the block, panic if block is not available */
    template<class TBeingPlacedWhileAllocating, class... Args>
    TBeingPlacedWhileAllocating* MakePtr(Args&&... args);

private:
    static constexpr MemoryBlocks::SizeType entireBlockSize =
        MemoryBlocks::EntireBlockSize(SingleBlockSizeRequested, alignof(AlignAsType));

    static_assert(
        MemoryBlocks::IsPowerOfTwo<alignof(AlignAsType)>::value,
        "alignof(AlignAsType) shall be power of two"
    );

    /** Actual memory for blocks allocated as a single chunk.
     Allocation info is not intermixed with allocated bytes,
     thus there are no problems with alignments and strict aliasing */
    alignas(AlignAsType) MemoryBlocks::ByteType memoryForBlocks[
        entireBlockSize*TotalNumBlocks
    ];
};

/// RAII to enclose unique allocation from corresponding MemoryBlocks
template<class TBeingPlacedWhileAllocating>
class UniqueBlock{
public:
    /// Initially empty
    UniqueBlock() : ptr(nullptr) {}

    /// RAII for unique allocation in MemoryBlocks
    template<
        MemoryBlocks::SizeType SingleBlockSizeRequested,
        MemoryBlocks::SizeType TotalNumBlocks,
        class AlignAsType = MemoryBlocks::Metadata,

        class... Args
    >
    UniqueBlock(
        BlockPool<
            SingleBlockSizeRequested,
            TotalNumBlocks,
            AlignAsType
        >& pool, ///< BlockPool to allocate from
        Args&&... args ///< Arguments to be forwarded to TBeingPlacedWhileAllocating constructor
    ): ptr(pool.MakePtr<TBeingPlacedWhileAllocating>(std::forward<Args>(args)...))
    {}

    // Cannot be copied, only moved
    UniqueBlock(const UniqueBlock&) = delete;
    UniqueBlock& operator=(const UniqueBlock&) = delete;

    /// Move constructor (transfer ownership)
    UniqueBlock(UniqueBlock&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr; // take ownership
    }
    /// Move assignment operator (transfer ownership)
    UniqueBlock& operator=(UniqueBlock&& other) noexcept {
        // Swap pointers (other’s destructor will do the rest, this is intended)
        TBeingPlacedWhileAllocating* temp = ptr; //keep promise: no dependencies
        ptr = other.ptr;
        other.ptr = temp; //other instance will soon go away and complete the cleanup
        return *this;
    }

    /// Destructor to free the allocated memory
    ~UniqueBlock() {
        MemoryBlocks::Free(ptr);  //nullptr is just ignored by Free
    }

    /// Access to the allocated object
    TBeingPlacedWhileAllocating* operator->() {
#ifdef InstantMemory_CheckAccess
        if( !ptr ){
            InstantMemory_Panic(); // panic if not allocated
        }
#endif
        return ptr;
    }

    /// Access to the allocated object
    TBeingPlacedWhileAllocating& operator*() {
#ifdef InstantMemory_CheckAccess
        if( !ptr ){
            InstantMemory_Panic(); // panic if not allocated
        }
#endif
        return *ptr;
    }

    /// Conversion to bool for easy checking
    explicit operator bool() const {
        return ptr != nullptr;
    }

private:
    /// Pointer to the allocated object
    TBeingPlacedWhileAllocating* ptr;
};

//______________________________________________________________________________
// Classes for memory operations - LifetimeManager

/** Allow instance of T to be constructed "in place".
 Wraps constructable items and manage their lifetime explicitly,
 used to handle those types that do not follow .setup()/.setup() pattern,
 NOTE: "doing important stuff" in constructor is "not recommended", since
        there is no way to report error, but still there are such types :)
 NOTE: No thread safety in favor of simplicity and speed here! :) */
template<class T> class InstantSingleton; // forward

template<class T>
class LifetimeManager{
public:
    //all the copying is banned
    constexpr LifetimeManager(const LifetimeManager&) = delete;
    LifetimeManager& operator =(const LifetimeManager&) = delete;

    /// Initialize empty instance
    LifetimeManager() = default;

    /// Cleanup all the stuff if it is still there
    ~LifetimeManager();

    /** Create corresponding item inside, or panic if already exists.
     Item is created by forwarding parameters,
     Once previous item was already present issue panic.
     Always returns a valid reference (if returns at all)) */
    template<class... Args>
    T& Emplace(Args&&... args);

    /** Create corresponding item inside, or replace if already exists.
     Item is always created by forwarding parameters.
     Nonexisting item will be created.
     Any existing previous item is properly destructed first
     before placing (forcing) the new one.
     Always returns a valid reference (if returns at all)) */
    template<class... Args>
    T& Force(Args&&... args);


    ///Destroy corresponding item if it existed
    void Destroy();

    ///Destroy corresponding item if it existed, panic if not
    void DestroyOrPanic();

    ///Check item exists (there is something stored here)
    explicit operator bool() const;

    /** Access to wrapped value.
     Use reference from Emplace to avoid extra check */
    T* operator->();

    /** Access to wrapped value.
     Use reference from Emplace to avoid extra check */
    T& operator*();

private:
    /** Actual location of that object.
     See also https://en.cppreference.com/w/cpp/language/new#Placement_new */
    alignas(T) MemoryBlocks::ByteType placeInMemory[ sizeof(T) ];

    bool exists = false;

    template<class> friend class InstantSingleton; // allow InstantSingleton access
};


/// Specialized access pattern on top of LifetimeManager
template<class T>
class InstantSingleton: private LifetimeManager<T>{
    using Base = LifetimeManager<T>;
public:
    using Base::Base;

    /** Access to wrapped value as singleton.
     Use reference from Emplace to avoid extra check */
    T* operator->(){
        return access();
    }

    /** Access to wrapped value as singleton.
     Use reference from Emplace to avoid extra check */
    T& operator*(){
        return *access();
    }

    ///Manual destroy is still allowed for exotic deinitialization cases
    void Destroy();

private:
    /** Access existing item or create the new one if not exists.
     Use Emplace or operator-> if you do not need singleton
     NOTE:  No thread safety by default in favor of simplicity here,
            unless InstantMemory_SingletonThreadsafe is defined! */
    T* access();

    #if !defined(InstantMemory_NoMultithreadingProtection) && defined(InstantMemory_MutexObjectType)
        #ifdef InstantMemory_SingletonThreadsafe
            /* Extra stuff on some platforms,
               that is why "off" by default */
            InstantMemory_MutexObjectType InstantMemory_MutexObjectVariable;
        #endif
    #endif
};

/** Keep track of LifetimeManager activation without creating stack instance.
 This is kind of "RAII like" scope for T from LifetimeManager<T>
 Especially useful with InstantCoroutine.h and InstantTask.h
 to make RAII like scopes (guard like behavior),
 but without introducing any local variables crossing CoroutineYield()!
 Use LifetimeManagerScope to handle Emplace and Destroy automatically.
 @code
    LifetimeManager<SomeClass> someLifetimeManager;
    ... //SomeClass does not exist here
    LifetimeManagerScope(someLifetimeManager){
        //SomeClass created (emplaced) and exists here
        ...
        someLifetimeManager->SomeAPI();
        ...
    }
    //SomeClass does not exist here (destroyed)
 @endcode
 One can use lifetimeManager inside of {} that follow LifetimeManagerScope
 and LifetimeManagerScope takes care to Destroy() once leaving that block
 REMEMBER:  break, continue and return shall not be used inside block!
            this also does not work with exceptions! */
#define LifetimeManagerScope(lifetimeManager, ...) \
    for( lifetimeManager.Emplace(__VA_ARGS__); lifetimeManager ; lifetimeManager.Destroy() )

//TODO: ArenaAllocator!!!

/// TODO: SmartAllocator not yet ready! implement later
#if 0

class SharedAllocator{
public:
    /// RAII to enclose unique allocation from corresponding MemoryBlocks
    ///TODO: two strategies of place for counter
    template<class TBeingPlacedWhileAllocating>
    class SharedAllocation{
    public:
    private:
    };

    /// RAII for shared allocation in MemoryBlocks
    template<class TBeingPlacedWhileAllocating, class... Args>
    SharedAllocation<TBeingPlacedWhileAllocating> AllocateShared(Args&&... args);
};

/** Specialization defines value of expected instance count for SmartAllocator.
 Sample usage
 @code
 @endcode
 */
template<class ClassToDetermineExpectedCount>
class SmartAllocatorExpectedCount;

/** Smart pool associated to type to allocate fixed size blocks.
 Uses SmartAllocatorExpectedCount to determine associated MemoryBlocks capacity
 nested MakeShared API returns smart pointer that keeps allocated instance alive */
template<class T>
class SmartAllocator{
public:
    /** Internal pointer that keeps T instance alive.
     NOTE: Ptr is compatible with ??? */
    class Ptr{};

    template<class... Args>
    Ptr MakeShared(Args... args);

private:

};

#endif



//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################


//______________________________________________________________________________
// Implementing MemoryBlocks

inline constexpr MemoryBlocks::SizeType MemoryBlocks::entireAlignedBlockSize(
    SizeType customBlockSizeRequested,
    SizeType alignmentWithMetadata
){
    /* Using 2*alignmentWithMetadata - 1
        because after rounding and multiplying at least one alignment must be included */
    return
        (
            (customBlockSizeRequested + 2*alignmentWithMetadata - 1)
            / alignmentWithMetadata
        ) * alignmentWithMetadata;
}

inline constexpr MemoryBlocks::SizeType MemoryBlocks::BlockSize() const{
    return customBlockSize;
}

inline constexpr MemoryBlocks::SizeType MemoryBlocks::TotalBlocks() const{
    return totalBlocks;
}

inline constexpr MemoryBlocks::SizeType MemoryBlocks::BlocksAllocated() const{
    return blocksAllocated;
}


inline void* MemoryBlocks::AllocateRaw(){
#ifdef InstantMemory_NoMultithreadingProtection
    ByteType* res = firstFree;
    if( res ){
        auto metadata = reinterpret_cast<Metadata*>(res - sizeof(Metadata));

        firstFree = metadata->next;
        metadata->owner = this; // future free will use this information

        ++blocksAllocated;
        return res;
    }
    //allow caller to scream for error in the place of call
    return nullptr;
#else
    ByteType* res = nullptr;
    {InstantMemory_EnterCritical
        res = firstFree;
        if( res ){
            auto metadata = reinterpret_cast<Metadata*>(res - sizeof(Metadata));

            firstFree = metadata->next;
            metadata->owner = this; // future free will use this information

            ++blocksAllocated;
        }
    InstantMemory_LeaveCritical}

    //allow caller to scream for error in the place of call
    return res;
#endif
}

inline void MemoryBlocks::FreeRaw(void* memoryPreviouslyAllocatedByBlockPool){
    InstantMemory_EnterCritical
    if( nullptr != memoryPreviouslyAllocatedByBlockPool ){
        ByteType* ptr = reinterpret_cast<ByteType*>(memoryPreviouslyAllocatedByBlockPool);

        auto metadata = reinterpret_cast<Metadata*>(ptr - sizeof(Metadata));
        MemoryBlocks* owner = metadata->owner;
        // This condition is not 100% warranty, just a way to increase detection chances.
        if( owner->mark == MarkToTest ){
            // valid block, almost for sure))
            //This will overwrite overlapping owner field, making mark invalid
            metadata->next = owner->firstFree; // Link the freed block
            owner->firstFree = ptr; // Update the first free block
            --owner->blocksAllocated;
            // Note: that block will be used next time keeping cache hot
        }
        else{
            // someone wants to free invalid block (or double free)
            InstantMemory_Panic();
        }
    }
    InstantMemory_LeaveCritical
}

/** "Raw" deallocation with destructor, panic if not allocated here.
 Free block and call destructor of TBeingPlacedWhileAllocating,
 panic if block was not allocated by this MemoryBlocks.
 One can use Make* API from derived classes for allocation,
 nullptr is just ignored here */
template<class TBeingPlacedWhileAllocating>
void MemoryBlocks::Free(TBeingPlacedWhileAllocating* correspondingAllocatedObject)
{
    /* This part is responsibility of the caller to ensure that
       correspondingAllocatedObject is deleted only once,
       and so no thread safety is needed here. */
    if( correspondingAllocatedObject ){
        //call destructor explicitly
        correspondingAllocatedObject->~TBeingPlacedWhileAllocating();

        // Go thread safe part to free raw memory
        FreeRaw( correspondingAllocatedObject );
    }
}



inline MemoryBlocks::MemoryBlocks(
    ByteType* memoryArea,
    SizeType customBlockSizeUsed,
    SizeType entireBlockSizeUsed,
    SizeType totalBlocksAvailable
) :
    customBlockSize(customBlockSizeUsed),
    entireBlockSize(entireBlockSizeUsed),
    totalBlocks(totalBlocksAvailable)
{
    // The first block to be allocated, pointer to custom memory
    firstFree = memoryArea + (entireBlockSizeUsed - customBlockSizeUsed);

    ByteType* currentBlock = firstFree;
    // walk all blocks one by one except the last one
    for(SizeType i = totalBlocksAvailable; i > 1; --i){
        ByteType* nextBlock = currentBlock + entireBlockSizeUsed;
        reinterpret_cast<Metadata*>(
            currentBlock - sizeof(Metadata)
        )->next = nextBlock;
        currentBlock = nextBlock;
    }

    //the last one block has no next block
    reinterpret_cast<Metadata*>(
        currentBlock - sizeof(Metadata)
    )->next = nullptr;
}

inline constexpr MemoryBlocks::SizeType MemoryBlocks::EntireBlockSize(
    SizeType customBlockSizeRequested,
    SizeType requestedAlignment
){
    return entireAlignedBlockSize(
        customBlockSizeRequested,
        sizeof(Metadata) > requestedAlignment
            ? sizeof(Metadata) //At least Metadata!
            : requestedAlignment
    );
}


//______________________________________________________________________________
// Implementing SimpleBlockPool

template<
    MemoryBlocks::SizeType SingleBlockSizeRequested,
    MemoryBlocks::SizeType TotalNumBlocks,
    class AlignAsType
>
constexpr BlockPool<SingleBlockSizeRequested, TotalNumBlocks, AlignAsType>
::BlockPool()
:   MemoryBlocks(
        memoryForBlocks,
        SingleBlockSizeRequested,
        entireBlockSize,
        TotalNumBlocks
    )
{}

template<
    MemoryBlocks::SizeType SingleBlockSizeRequested,
    MemoryBlocks::SizeType TotalNumBlocks,
    class AlignAsType
>
template<class TBeingPlacedWhileAllocating, class... Args>
TBeingPlacedWhileAllocating*
BlockPool<SingleBlockSizeRequested, TotalNumBlocks, AlignAsType>::MakePtr(Args&&... args){
    //see https://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c
    static_assert(
        sizeof(TBeingPlacedWhileAllocating) <= SingleBlockSizeRequested,
         "Cannot create item that does not fit into block"
    );
    auto rawMemory = AllocateRaw();
    if( rawMemory ){
        //see https://en.cppreference.com/w/cpp/memory/new/operator_new
        return new( InstantMemoryPlaceholderHelper(rawMemory) )
                    TBeingPlacedWhileAllocating(static_cast<Args&&>(args)...);
    }
    InstantMemory_Panic();
}

//______________________________________________________________________________
// Implementing LifetimeManager

template<class T>
LifetimeManager<T>::~LifetimeManager(){
    Destroy();
}


template<class T>
template<class... Args>
T& LifetimeManager<T>::Emplace(Args&&... args){
    if( !exists ){
        exists = true;
        /* Create new item explicitly.
            Does what std::forward by exploiting reference collapsing */
        return *new( InstantMemoryPlaceholderHelper(placeInMemory) )
                                    T( static_cast<Args&&>(args)... );
    }
    InstantMemory_Panic();
}

template<class T>
template<class... Args>
T& LifetimeManager<T>::Force(Args&&... args){
    if( exists ){
        // destroy old existing instance
        reinterpret_cast<T*>(placeInMemory)->~T();
    }
    /* Always create a new item explicitly.
        Does what std::forward by exploiting reference collapsing */
    auto ptr = new(
        InstantMemoryPlaceholderHelper(placeInMemory)
    )
        T( static_cast<Args&&>(args)... ); //keep promise: no dependencies
    exists = true; //only after is constructed
    return *ptr;
}


//______________________________________________________________________________
// Implementing instantSingleton: specialized access pattern

template<class T>
void InstantSingleton<T>::Destroy(){
#ifndef InstantMemory_SingletonThreadsafe
    Base::Destroy();
#else
    {InstantMemory_EnterCritical
        Base::Destroy();
    InstantMemory_LeaveCritical}
#endif
}

template<class T>
T* InstantSingleton<T>::access(){
#ifndef InstantMemory_SingletonThreadsafe
    if( Base::exists ){
        return reinterpret_cast<T*>(Base::placeInMemory);
    }
    auto ptr = new(
        InstantMemoryPlaceholderHelper(Base::placeInMemory)
    ) T();
    Base::exists = true;
    return ptr;
#else
    {InstantMemory_EnterCritical
        if( !Base::exists ){
            new(
                InstantMemoryPlaceholderHelper(Base::placeInMemory)
            ) T(); //
            Base::exists = true;
        }
    InstantMemory_LeaveCritical}
    return reinterpret_cast<T*>(Base::placeInMemory);
#endif
}



template<class T>
void LifetimeManager<T>::Destroy(){
    if( exists ){
        exists = false;
        //call destructor explicitly
        reinterpret_cast<T*>(placeInMemory)->~T();
    }
}

template<class T>
void LifetimeManager<T>::DestroyOrPanic(){
    if( exists ){
        exists = false;
        ///call destructor explicitly
        reinterpret_cast<T*>(placeInMemory)->~T();
    }
    else{
        InstantMemory_Panic();
    }
}


template<class T>
LifetimeManager<T>::operator bool() const {
    return exists;
}


template<class T>
T* LifetimeManager<T>::operator->(){
    if( exists ){
        return reinterpret_cast<T*>(placeInMemory);
    }
    InstantMemory_Panic();
}

template<class T>
T& LifetimeManager<T>::operator*(){
    if( exists ){
        return *reinterpret_cast<T*>(placeInMemory);
    }
    InstantMemory_Panic();
}



#endif
