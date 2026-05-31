/** @file tests/test_InstantDelegateEx.cpp
    @brief Unit tests for InstantDelegate.h - corner cases:
           multiple inheritance, virtual dispatch, virtual base classes,
           diamond inheritance and equality semantics across these hierarchies.
*/

#include "InstantDelegate.h"
#include "doctest/doctest.h"
#include <cstdint>


namespace{
    using std::int32_t;

    //=========================================================================
    // Classes for multiple inheritance tests (non-virtual)
    // MIBaseB occupies a nonzero offset inside MIDerived - key stress test:
    // the delegate must store and restore the correct subobject pointer.

    struct MIBaseA{
        int32_t a_val = 0;
        int32_t method_a(int32_t x) const {
            return a_val + x + 1000;
        }
    };

    struct MIBaseB{
        int32_t b_val = 0;
        int32_t method_b(int32_t x) const {
            return b_val + x + 2000;
        }
    };

    struct MIDerived : MIBaseA, MIBaseB{
        int32_t method_d(int32_t x) const {
            return a_val + b_val + x + 3000;
        }
        int32_t method_e(int32_t x) const {
            return a_val + b_val + x + 4000;
        }
    };

    //=========================================================================
    // Classes for virtual method dispatch tests

    struct VBase{
        virtual int32_t vmethod(int32_t x) const { return x + 1000; }
        virtual ~VBase() = default;
    };

    struct VDerived : VBase{
        int32_t extra = 0;
        int32_t vmethod(int32_t x) const override { return x + 2000 + extra; }
    };

    //=========================================================================
    // Classes for virtual inheritance tests

    struct VIBase{
        int32_t vib_val = 0;
        int32_t vib_method(int32_t x) const { return vib_val + x + 5000; }
    };

    struct VIDerived : virtual VIBase{
        int32_t vid_val = 0;
        int32_t vid_method(int32_t x) const { return vib_val + vid_val + x + 6000; }
    };

    //=========================================================================
    // Classes for diamond virtual inheritance tests

    struct DiamondA{
        int32_t a_val = 0;
        int32_t a_method(int32_t x) const { return a_val + x + 7000; }
    };

    struct DiamondB : virtual DiamondA{
        int32_t b_val = 0;
    };

    struct DiamondC : virtual DiamondA{
        int32_t c_val = 0;
    };

    // DiamondD has exactly one shared DiamondA subobject
    struct DiamondD : DiamondB, DiamondC{
        int32_t d_method(int32_t x) const { return a_val + b_val + c_val + x + 8000; }
    };

    //=========================================================================
    // Functor with virtual base - exercises MakeCallerForFunctor with
    // a non-trivial object layout (vtable pointer for virtual base access)

    struct FunctorWithVBase : virtual VIBase{
        // Constructor needed: virtual base makes this a non-aggregate,
        // so explicit construction is required for rvalue temporaries
        explicit FunctorWithVBase(int32_t v = 0){ vib_val = v; }
        int32_t operator()(int32_t x) const { return vib_val + x + 9000; }
    };

} //namespace


TEST_CASE("InstantDelegate multiple inheritance - second base at nonzero offset"){
    // MIBaseB subobject sits at a nonzero offset inside MIDerived.
    // A pointer-adjustment bug would make bRef alias aRef storage,
    // reading a_val instead of b_val and producing wrong numeric results.
    // Binding via the base class reference ensures the correct subobject
    // pointer is stored in (and later restored from) the delegate state.
    using D = Delegate<int32_t(int32_t)>;

    MIDerived obj;
    obj.a_val = 10;
    obj.b_val = 20;

    // Bind to first base method via base reference (sanity check, offset 0)
    MIBaseA& aRef = obj;
    CHECK( 1015 == D::From(aRef).Bind<&MIBaseA::method_a>()(5) );

    // Bind to second base method via base reference (nonzero offset, key test)
    MIBaseB& bRef = obj;
    CHECK( 2025 == D::From(bRef).Bind<&MIBaseB::method_b>()(5) );

    // Bind to derived own method directly via derived reference
    CHECK( 3035 == D::From(obj).Bind<&MIDerived::method_d>()(5) );

    // Repeat via pointer path for From(&obj) symmetry
    MIBaseA* aPtr = &obj;
    CHECK( 1015 == D::From(aPtr).Bind<&MIBaseA::method_a>()(5) );

    MIBaseB* bPtr = &obj;
    CHECK( 2025 == D::From(bPtr).Bind<&MIBaseB::method_b>()(5) );

    CHECK( 3035 == D::From(&obj).Bind<&MIDerived::method_d>()(5) );
}


TEST_CASE("InstantDelegate multiple inheritance - const object BoundDelegateBuilder"){
    // Exercises the BoundDelegateBuilder<const C> specialisation.
    // For a const object only the const-method Bind overloads are available;
    // the specialisation strips the const qualifier to form the method pointer
    // type, so &MIBaseA::method_a still resolves correctly.
    using D = Delegate<int32_t(int32_t)>;

    MIDerived tmp;
    tmp.a_val = 100;
    tmp.b_val = 200;
    const MIDerived& constObj = tmp;

    const MIBaseA& constARef = constObj;
    CHECK( 1105 == D::From(constARef).Bind<&MIBaseA::method_a>()(5) );

    const MIBaseB& constBRef = constObj;
    CHECK( 2205 == D::From(constBRef).Bind<&MIBaseB::method_b>()(5) );

    CHECK( 3305 == D::From(constObj).Bind<&MIDerived::method_d>()(5) );

    // Also via const pointer
    const MIBaseA* constAPtr = &constObj;
    CHECK( 1105 == D::From(constAPtr).Bind<&MIBaseA::method_a>()(5) );
}


TEST_CASE("InstantDelegate virtual method dispatch via delegate"){
    // A delegate storing VBase* still performs virtual dispatch:
    // (VBase_ptr->*&VBase::vmethod)(args) routes through the vtable and
    // calls VDerived::vmethod, correctly accessing VDerived::extra.
    // This verifies virtual dispatch is not suppressed by the delegate layer.
    using D = Delegate<int32_t(int32_t)>;

    VDerived vd;
    vd.extra = 50;

    // Direct dispatch - delegate stores VDerived*, calls override directly
    CHECK( 2055 == D::From(vd).Bind<&VDerived::vmethod>()(5) );

    // Virtual dispatch via base pointer - delegate stores VBase*, vtable call
    VBase* basePtr = &vd;
    CHECK( 2055 == D::From(basePtr).Bind<&VBase::vmethod>()(5) );

    // Virtual dispatch via base reference - delegate stores VBase*, vtable call
    VBase& baseRef = vd;
    CHECK( 2055 == D::From(baseRef).Bind<&VBase::vmethod>()(5) );

    // Confirm base-only object returns base implementation (no override)
    VBase baseOnly;
    CHECK( 1005 == D::From(baseOnly).Bind<&VBase::vmethod>()(5) );
}


TEST_CASE("InstantDelegate virtual inheritance"){
    // Virtual base means VIBase subobject is addressed through a vptr offset.
    // Binding via a VIBase& reference stores the adjusted VIBase* directly,
    // so the delegate call reaches vib_val without additional indirection.
    using D = Delegate<int32_t(int32_t)>;

    VIDerived vid;
    vid.vib_val = 10;
    vid.vid_val = 20;

    // Bind to virtual base method via base reference (stores VIBase*)
    VIBase& vibRef = vid;
    CHECK( 5015 == D::From(vibRef).Bind<&VIBase::vib_method>()(5) );

    // Bind to derived own method (stores VIDerived*)
    CHECK( 6035 == D::From(vid).Bind<&VIDerived::vid_method>()(5) );

    // Pointer paths
    VIBase* vibPtr = &vid;
    CHECK( 5015 == D::From(vibPtr).Bind<&VIBase::vib_method>()(5) );
    CHECK( 6035 == D::From(&vid).Bind<&VIDerived::vid_method>()(5) );
}


TEST_CASE("InstantDelegate diamond virtual inheritance - single shared base"){
    // DiamondD has exactly one DiamondA subobject (virtual base shared by B and C).
    // Obtaining DiamondA& from a DiamondD object is unambiguous;
    // the delegate stores that adjusted pointer and reaches the single a_val.
    // d_method accesses a_val, b_val and c_val through proper offsets.
    using D = Delegate<int32_t(int32_t)>;

    DiamondD dd;
    dd.a_val = 10;
    dd.b_val = 20;
    dd.c_val = 30;

    // Bind to the single shared virtual base (stores DiamondA*)
    DiamondA& aRef = dd;    // unambiguous: only one DiamondA subobject
    CHECK( 7015 == D::From(aRef).Bind<&DiamondA::a_method>()(5) );

    // Bind to derived own method (accesses a_val, b_val, c_val)
    CHECK( 8065 == D::From(dd).Bind<&DiamondD::d_method>()(5) );

    // Pointer paths
    DiamondA* aPtr = &dd;
    CHECK( 7015 == D::From(aPtr).Bind<&DiamondA::a_method>()(5) );
    CHECK( 8065 == D::From(&dd).Bind<&DiamondD::d_method>()(5) );
}


TEST_CASE("InstantDelegate functor with virtual base"){
    // MakeCallerForFunctor stores void* to the functor and casts it back to
    // FunctorWithVBase*.  Even though FunctorWithVBase contains a vptr slot
    // for its virtual base, the cast is valid because the stored address is
    // the original FunctorWithVBase* - the delegate never slices the object.
    using MyCallback = Delegate<int32_t(int32_t)>;

    FunctorWithVBase f(100);

    // Normal functor-reference path (lvalue, uses Delegate(Functor& functor) ctor)
    MyCallback d(f);
    CHECK( 9105 == d(5) );

    // Unstorable path: pass a temporary rvalue so Functor deduces as a
    // non-reference type (Functor& deduction from lvalue would give Functor=T&
    // which makes MakeCallerForFunctor<T&> attempt a pointer-to-reference)
    CHECK( 9105 == MyCallback::Unstorable(FunctorWithVBase{100})(5) );
}


TEST_CASE("InstantDelegate inheritance equality semantics"){
    // cmpTo() performs byte-wise comparison of the two-word delegate state.
    // Different method pointer -> different caller function -> not equal.
    // Different object address -> not equal even for the same method.
    // Copy, or From(ref) vs From(&obj) for the same address -> equal.
    using D = Delegate<int32_t(int32_t)>;

    MIDerived obj1, obj2;
    obj1.a_val = 10;  obj1.b_val = 20;
    obj2.a_val = 10;  obj2.b_val = 20;

    auto d_d_obj1      = D::From(obj1).Bind<&MIDerived::method_d>();
    auto d_d_obj1_copy = d_d_obj1;
    CHECK( d_d_obj1 == d_d_obj1_copy );    // copy equals original

    auto d_e_obj1 = D::From(obj1).Bind<&MIDerived::method_e>();
    CHECK( d_d_obj1 != d_e_obj1 );         // same object, different method -> different caller ptr

    // From(ref) and From(&obj) store the same address -> equal
    auto d_d_obj1_ptr = D::From(&obj1).Bind<&MIDerived::method_d>();
    CHECK( d_d_obj1 == d_d_obj1_ptr );

    auto d_d_obj2 = D::From(obj2).Bind<&MIDerived::method_d>();
    CHECK( d_d_obj1 != d_d_obj2 );         // same method, different objects

    // All method-bound delegates are truthy (caller != callSimpleCase)
    CHECK( d_d_obj1 );
    CHECK( d_e_obj1 );
    CHECK( d_d_obj2 );

    // Base-subobject delegates for the same subobject compare equal
    MIBaseA& aRef1 = obj1;
    auto d_a     = D::From(aRef1).Bind<&MIBaseA::method_a>();
    auto d_a_ptr = D::From(&aRef1).Bind<&MIBaseA::method_a>();
    CHECK( d_a == d_a_ptr );               // reference vs pointer to same subobject

    // Subobjects of different base types use different callers -> not equal
    MIBaseB& bRef1 = obj1;
    auto d_b = D::From(bRef1).Bind<&MIBaseB::method_b>();
    CHECK( d_a != d_b );                   // different subobject types, different callers
}
