/** @file InstantSignals.h
    @brief Handle hardware signals being mapped to memory

This is general abstraction to operate and manipulate bits in memory 


MIT License

Copyright (c) 2023 Pavlo M, see https://github.com/olvap80/InstantArduino

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

#ifndef InstantSignals_INCLUDED_H
#define InstantSignals_INCLUDED_H

// маска, фільтрація сигналів (масовий антидрєбєзг)
/* ловлення сигналів на кожній ітерації
там де ловимо 1 складати по |
там, де ловимо 0 інвертувати і складати по |
(або викорситовувати "перевертальну" маску XOR)
*/

/// Simple bits(s) location in memory
/** Used to tie flags from multiple locations together,
 * This is actually address and mask to hold "coordinates of bits" */
class BitsLocation{
public:
    /// Fundamental type being referenced by address from BitsLocation
    using AddressableUnit = unsigned char;

    /// There is no way to create "unconnected"  BitsLocation
    BitsLocation() = delete;

    ///Tie with specific signal location
    BitsLocation(
        AddressableUnit* addressOfBits,    ///< Address where corresponding bits start
        AddressableUnit  maskToSelectBits  ///< Target bits to selected (1s means selected)
    ) : address(addressOfBits), mask(maskToSelectBits) {}

    /// Test there is at least one bit set to 1
    bool HasBitSet() const{
        return (*address & mask) != 0;
    }
    /// Test there are no bits set (all are 0)
    bool HasNoBitsSet() const{
        return (*address & mask) == 0;
    }
    /// Test all selected bits are set (all selected are 1)
    bool HasAllBitsSet() const{
        return (*address & mask) == mask;
    }

    /// Access selected bits (mask applies)
    AddressableUnit operator*() const{
        return *address & mask;
    }

    /// Actual address where signal is located
    AddressableUnit* Address() const{
        return address;
    }
    
    /// Mask being applied to selected bits
    AddressableUnit Mask() const{
        return mask;
    }

    /// Set to 1 all bits selected by the mask (nonatomic)
    void Set(){
        *address |= mask;
    }

    /// Nonatomic set target bits to specific value
    void Set(AddressableUnit newValue){
        *address = (*address & ~mask) | (newValue & mask);
    }

    /// Nonatomic set corresponding bits to ZEROES with regard to mask
    void Clear(){
        *address = (*address & ~mask);
    }

private:
    /// Actual address where signal is located
    AddressableUnit* address;
    
    /// Mask to apply to select target bits
    AddressableUnit mask;
};


class BitsDebounce{

};


/// 
class BitsAccumulator{
public:
    /// Fundamental type being referenced by SignalTracker
    /** The same type as BitsLocation */
    using AddressableUnit = BitsLocation::AddressableUnit;

    /// Update the latest value from the source
    void Refresh(){
        result |= *source ^ invert;
    }

    /// Clear what we have accumulated so far
    void Clear(){
        result = 0;
    }

    /// Obtain what is accumulated so far
    BitsLocation Result(){
        return BitsLocation(
            &result,
            source.Mask()
        );
    }

private:
    /// Real location where to obtain  
    BitsLocation source;

    /// Those bytes, that need to be inverted (it is assumed "active 1")
    AddressableUnit invert = 0;

    /// The accumulated result
    AddressableUnit result = 0;
};

#endif
