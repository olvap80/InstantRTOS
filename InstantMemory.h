/** @file InstantMemory.h
    @brief Simple deterministic memory management utilities suitable for real time
    can be used for dynamic memory allocations on Arduino and similar platforms.

    (c) see https://github.com/olvap80/InstantRTOS

    Zero dependencies, works instantly by copy pasting to your project...
    Inspired by memory management toolset available in various RTOSes,
    and now available in C++ :)

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

            manualManagement.Create("Some parameter", 42); //lifetime started
            ...
            manualManagement->SomeAction(); //continues
            ...
            manualManagement.Destroy(); //ended
            
            ...
            useAsSingleton.Singleton().DoSomethingElse(); //create if not exists
        }
    @endcode

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

/* Keep promise to not use any standard libraries by default, 
Enable defines below to allow standard library instead of own implementation */
//#define InstantRTOS_USE_STDLIB
#ifdef InstantRTOS_USE_STDLIB

#   if __has_include(<cstddef>)
#       include <cstddef>
        //remember uintptr_t is optional per https://en.cppreference.com/w/cpp/types/integer
        using std::size_t;
#   else
#       include <stddef.h>
#   endif

#   if __has_include(<cstdint>)
#       include <cstdint>
        using std::uintptr_t;
#   else
#       include <stdint.h>
#   endif

#   define INSTANTMEMORY_SIZE_T size_t
#   define INSTANTMEMORY_UINTPTR_T uintptr_t

#   if __has_include(<new>)
    //this header is present even on avr
#   include <new>
#else
#   include <new.h>
#endif

//hack to replace class for the case when custom placement new is not needed
#define InstantMemoryPlaceholderHelper(ptr) ptr

#else

#   define INSTANTMEMORY_SIZE_T unsigned
#   define INSTANTMEMORY_UINTPTR_T unsigned

static_assert(
    sizeof(INSTANTMEMORY_SIZE_T) == sizeof( sizeof(INSTANTMEMORY_SIZE_T) ),
    "The INSTANTMEMORY_SIZE_T shall have the same size as the result of sizeof"
);
static_assert(
    sizeof(INSTANTMEMORY_UINTPTR_T) >= sizeof( void* ),
    "The INSTANTMEMORY_UINTPTR_T shall be large enough to hold pointer"
);

/// Helper class to allow custom placement new without conflicts 
class InstantMemoryPlaceholderHelper{
public:
    InstantMemoryPlaceholderHelper(void *placeForAllocation) : ptr(placeForAllocation) {} 
private:
    void *ptr;
    friend void* operator new(INSTANTMEMORY_SIZE_T, InstantMemoryPlaceholderHelper place) noexcept; 
};

/// own placement new implementation 
/** see https://en.cppreference.com/w/cpp/memory/new/operator_new */
inline void* operator new(INSTANTMEMORY_SIZE_T, InstantMemoryPlaceholderHelper place) noexcept{
    return place.ptr;
}

#endif


//______________________________________________________________________________
// Configurable error handling

#ifndef InstantMemory_Panic
#   ifdef InstantRTOS_Panic
#       define InstantMemory_Panic() InstantRTOS_Panic('M')  
#   else
#       define InstantMemory_Panic() /* you can customize here! */ do{}while(true)
#   endif
#endif

#ifndef InstantMemory_EnterCritical
#   ifdef InstantMemory_EnterCritical
#       define InstantMemory_EnterCritical InstantRTOS_EnterCritical
#       define InstantMemory_LeaveCritical InstantRTOS_LeaveCritical
#   else
#       define InstantMemory_EnterCritical
#       define InstantMemory_LeaveCritical
#   endif
#endif

//______________________________________________________________________________
// Classes for memory operations


///Optimized base for BlockPool (used for all kinds of BlockPools below)
/** All various BlockPools reuse the same implementation
 *  to save program space being spent on allocation/deallocation logic.
 *  Use BlockPool or SmartAllocator (TBD) below
 *  (NOTE: single implementation to not duplicate code for each type) */
class BlockPoolBase{
public:
    //all the copying is banned
    constexpr BlockPoolBase(const BlockPoolBase&) = delete;
    BlockPoolBase& operator =(const BlockPoolBase&) = delete;

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

    ///"Raw" allocate of any free block, returned null if no more blocks available
    /** Obtain raw uninitialized bytes */
    void* AllocateRaw();


    /// "Raw" memory deallocation, panic if not allocated here
    /** Can "panic" if memory is not owned by that BlockPoolBase,
     * Free raw bytes, do not issue any destructors! */
    static void FreeRaw(void* memoryPreviouslyAllocatedByBlockPool);

    /// "Raw" deallocation with destructor, panic if not allocated here
    template<class TBeingPlacedWhileAllocating>
    static void Free(TBeingPlacedWhileAllocating* correspondingAllocatedObject);
    
    /// Use this for determining minimum alignment requirements
    union Metadata{
        BlockPoolBase* owner; ///<Internal usage
        ByteType* next; ///<Internal usage
        SizeType s; ///<Internal usage (for alignment purposes)
    };

protected:
    /// @Make gcc happy with C++ 11
    constexpr BlockPoolBase() = delete;

    ///Initialize BlockPool logic
    /** One must be sure memoryArea contains at least
     *  entireBlockSizeUsed*totalBlocksAvailable bytes!
     *  Metadata is added to the end of */
    BlockPoolBase(
        ByteType* memoryArea, ///< BlockPoolBase will place allocated blocks here,
                              ///< there shall be space enough to hold
                              ///< entireBlockSizeUsed*totalBlocksAvailable bytes
        SizeType customBlockSizeUsed, ///Custom part in the block
        SizeType entireBlockSizeUsed, ///<Entire block size (as calculated by EntireBlockSize)
        SizeType totalBlocksAvailable ///<Total number of full blocks reserved
    );

    /// Calculate size of the block together with helper information (Metadata)
    /** The EntireBlockSize takes into account place for Metadata 
     * and ensures Metadata alignment does not violate requestedAlignment.
     * It is assumed that customBlockSizeRequested is already of
     * size compatible with requestedAlignment,
     * and that both requestedAlignment and sizeof(Metadata) are power of two */
    static constexpr SizeType EntireBlockSize(
        SizeType customBlockSizeRequested,
        SizeType requestedAlignment
    );


    template<SizeType N>
    struct IsPowerOfTwo {
        static constexpr bool value = 0 != ((N >= 1) & !(N & (N - 1)));
    };
    static_assert(
        IsPowerOfTwo<sizeof(Metadata)>::value,
        "sizeof(Metadata) shall be power of two"
    );
    
private:
    /// Constant to mark BlockPoolBase instances for debugging purposes
    static constexpr SizeType MarkToTest = 24991;
    /// Mark BlockPoolBase instances for debugging purposes
    const SizeType mark = MarkToTest;

    /// Custom pa
    const SizeType customBlockSize;
    const SizeType entireBlockSize;

    /// Maximum number of blocks available for allocation
    const SizeType totalBlocks;
    /// How much blocks were allocated so far
    SizeType blocksAllocated = 0;

    /// Pointer to the "custom memory" of the first free block
    ByteType* firstFree;

    /// One must include metadata without spoiling alignment
    static constexpr SizeType entireAlignedBlockSize(
        SizeType customBlockSizeRequested,
        SizeType alignmentWithMetadata
    );
};


///Simple pool to allocate fixed size blocks
/** Allow allocation of raw pointer to fixed size memory blocks */
template<
    BlockPoolBase::SizeType SingleBlockSizeRequested,
    BlockPoolBase::SizeType TotalNumBlocks,
    class AlignAsType = BlockPoolBase::Metadata
>
class BlockPool: public BlockPoolBase{
    static_assert(
        SingleBlockSizeRequested >= 2,
        "Too small blocks are not supported by BlockPool");
    static_assert(
        SingleBlockSizeRequested % alignof(AlignAsType) == 0,
        "BlockPool: ensure your blocks are of proper size allowing proper alignment"
    );

public:
    /// Create ready to use BlockPool
    constexpr BlockPool();
    
    ///Allocation with simultaneous construction
    template<class TBeingPlacedWhileAllocating, class... Args>
    TBeingPlacedWhileAllocating* Allocate(Args&&... args);

private:
    static constexpr SizeType entireBlockSize =
        EntireBlockSize(SingleBlockSizeRequested, alignof(AlignAsType));
    static_assert(
        IsPowerOfTwo<alignof(AlignAsType)>::value,
        "alignof(AlignAsType) shall be power of two"
    );

    /// Actual memory for blocks allocated as a single chunk
    /** Allocation info is not intermixed with allocated bytes,
     * thus there are no problems with alignments and strict aliasing */
    alignas(AlignAsType) ByteType memoryForBlocks[
        entireBlockSize*TotalNumBlocks
    ];
};




///Allow instance of T to be constructed "in place"
/** Wraps constructable items and manage their lifetime explicitly,
 *  used to handle those types that do not follow .setup()/.setup() pattern,
 *  NOTE: "doing important stuff" in constructor is "not recommended", since
 *        there is no way to report error, but still there are such types :)
 *  NOTE: No thread safety in favor of simplicity here! :) */
template<class T>
class LifetimeManager{
public:
    //all the copying is banned
    constexpr LifetimeManager(const LifetimeManager&) = delete;
    LifetimeManager& operator =(const LifetimeManager&) = delete;

    /// Initialize empty instance
    LifetimeManager() = default;

    ///Create corresponding item, or panic if already exists
    /** Always returns valid reference (if returns at all)) */
    template<class... Args>
    T& Create(Args&&... args){
        if( !exists ){
            exists = true;
            /* Create new item explicitly.
               Does what std::forward by exploiting reference collapsing */
            return *new( InstantMemoryPlaceholderHelper(placeInMemory) )
                                        T( reinterpret_cast<Args&&>(args)... );
        }
        InstantMemory_Panic();
    }

    ///Access existing item or create the new one if not exists
    /** Use Create or operator-> if you do not need singleton
     * NOTE: No thread safety in favor of simplicity here! */
    template<class... Args>
    T& Singleton(Args&&... args){
        if( exists ){
            return reinterpret_cast<T*>(placeInMemory);
        }
        exists = true;
        return *new( InstantMemoryPlaceholderHelper(placeInMemory) )
                                        T( reinterpret_cast<Args&&>(args)... );
    }
    
    ///Destroy corresponding item if it existed
    void Destroy(){
        if( exists ){
            exists = false;
            //call destructor explicitly
            reinterpret_cast<T*>(placeInMemory)->~T();
        }
    }

    ///Destroy corresponding item if it existed, panic if not
    void DestroyOrPanic(){
        if( exists ){
            exists = false;
            ///call destructor explicitly
            reinterpret_cast<T*>(placeInMemory)->~T();
        }
        else{
            InstantMemory_Panic();
        }
    }

    ///Check item exists
    explicit operator bool() const {
        return exists;
    }

    ///Access to wrapped value
    /** Use reference from Create to avoid extra check */
    T* operator->(){
        if( exists ){
            return reinterpret_cast<T*>(placeInMemory);
        }
        InstantMemory_Panic();
    }

    ///Access to wrapped value
    /** Use reference from Create to avoid extra check */
    T& operator*(){
        if( exists ){
            return *reinterpret_cast<T*>(placeInMemory);
        }
        InstantMemory_Panic();
    }

private:
    ///Actual location of that object
    /** See also https://en.cppreference.com/w/cpp/language/new#Placement_new */
    alignas(T) BlockPoolBase::ByteType placeInMemory[ sizeof(T) ];

    bool exists = false;
};


/// TODO: SmartAllocator not yet ready! implement later
#if 0

///Specialization defines value of expected instance count for SmartAllocator
/** Sample usage
    @code
    @endcode
*/ 
template<class ClassToDetermineExpectedCount>
class SmartAllocatorExpectedCount;

///Smart pool associated to type to allocate fixed size blocks
/** Uses SmartAllocatorExpectedCount to determine associated BlockPool capacity
 * nested Allocate API returns smart pointer that keeps allocated instance alive */
template<class T>
class SmartAllocator{
public:
    ///Internal pointer that keeps T instance alive
    /** NOTE: Ptr is compatible with ??? */ 
    class Ptr{};

    template<class... Args>
    Ptr Allocate(Args... args);

private:

};

#endif



//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################

/// One must include metadata without spoiling alignment
inline constexpr BlockPoolBase::SizeType BlockPoolBase::entireAlignedBlockSize(
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

inline constexpr BlockPoolBase::SizeType BlockPoolBase::BlockSize() const{
    return customBlockSize;
}

inline constexpr BlockPoolBase::SizeType BlockPoolBase::TotalBlocks() const{
    return totalBlocks;
}

inline constexpr BlockPoolBase::SizeType BlockPoolBase::BlocksAllocated() const{
    return blocksAllocated;
}


inline void* BlockPoolBase::AllocateRaw(){
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
}

inline void BlockPoolBase::FreeRaw(void* memoryPreviouslyAllocatedByBlockPool){
    if( nullptr != memoryPreviouslyAllocatedByBlockPool ){
        ByteType* ptr = reinterpret_cast<ByteType*>(memoryPreviouslyAllocatedByBlockPool);
        
        auto metadata = reinterpret_cast<Metadata*>(ptr - sizeof(Metadata));
        BlockPoolBase* owner = metadata->owner;
        if( owner->mark == MarkToTest ){
            // valid block, almost for sure))
            metadata->next = owner->firstFree;
            owner->firstFree = ptr;
        }
        else{
            // someone wants to free invalid block
            InstantMemory_Panic();
        }
    }    
}

template<class TBeingPlacedWhileAllocating>
inline void BlockPoolBase::Free(TBeingPlacedWhileAllocating* correspondingAllocatedObject)
{
    if( nullptr == correspondingAllocatedObject ){
        return;
    }
    correspondingAllocatedObject->~TBeingPlacedWhileAllocating();
    FreeRaw( correspondingAllocatedObject );
}



inline BlockPoolBase::BlockPoolBase(
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

inline constexpr BlockPoolBase::SizeType BlockPoolBase::EntireBlockSize(
    SizeType customBlockSizeRequested,
    SizeType requestedAlignment
){
    return entireAlignedBlockSize(
        customBlockSizeRequested,
        sizeof(Metadata) > requestedAlignment
            ? sizeof(Metadata)
            : requestedAlignment
    );
}

template<
    BlockPoolBase::SizeType SingleBlockSize,
    BlockPoolBase::SizeType TotalNumBlocks,
    class AlignAsType
>
constexpr BlockPool<SingleBlockSize, TotalNumBlocks, AlignAsType>::BlockPool()
:   BlockPoolBase(
        memoryForBlocks, SingleBlockSize, entireBlockSize, TotalNumBlocks
    ){}

template<
    BlockPoolBase::SizeType SingleBlockSize,
    BlockPoolBase::SizeType TotalNumBlocks,
    class AlignAsType
>
template<class TBeingPlacedWhileAllocating, class... Args>
TBeingPlacedWhileAllocating* 
BlockPool<SingleBlockSize, TotalNumBlocks, AlignAsType>::Allocate(Args&&... args){
    //see https://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c
    static_assert(sizeof(TBeingPlacedWhileAllocating) <= SingleBlockSize, "Cannot create item that does not fit into block");
    auto rawMemory = AllocateRaw();
    if( rawMemory ){
        //see https://en.cppreference.com/w/cpp/memory/new/operator_new
        return new( InstantMemoryPlaceholderHelper(rawMemory) )
                    TBeingPlacedWhileAllocating(static_cast<Args&&>(args)...);
    }
    return nullptr;
}



#endif
