/** @file InstantIntrusiveList.h
    @brief Zero overhead intrusive bidirectional (double linked) list (chain)
           no dependencies at all (does not depend even on standard headers)

Suitable for embedded platforms like Arduino, no dynamic memory usage at all
(minimalistic implementation to chain existing items into iterable list)

Compatible with range based for.

Example Usage (generate sequences):
 @code
    class MyTestItem: public IntrusiveList<MyTestItem>::Node{
    public:
        /// Constructor for testing purposes
        MyTestItem(int value) : storedValue(value){}

        /// Method for testing purposes
        void Print(){
            Serial.println(storedValue);
        }
    private:
        int storedValue;
    };

    void setup() {
        Serial.begin(9600);
    }

    void loop() {

        Serial.println(F("\n------------- Iteration -------------"));

        // put your main code here, to run repeatedly:
        IntrusiveList<MyTestItem> il;

        MyTestItem ti1(11);
        MyTestItem ti2(22);
        MyTestItem ti3(33);

        il.InsertAtFront(&ti2);
        il.InsertAtFront(&ti1);
        il.InsertAtBack(&ti3);
        for(auto& item : il){
            item.Print();
        }
        Serial.println(F("----"));

        MyTestItem ti4(444);
        ti2.InsertPrevChainElement(&ti4);
        for(auto& item : il){
            item.Print();
        }
        Serial.println(F("----"));

        ti2.InsertNextChainElement(&ti4);
        for(auto& item : il){
            item.Print();
        }
        Serial.println(F("----"));

        ti4.RemoveFromChain();
        for(auto& item : il){
            item.Print();
        }
        Serial.println(F("----"));

        il.RemoveAtFront();
        for(auto& item : il){
            item.Print();
        }
        Serial.println(F("----"));

        il.RemoveAtEnd();
        for(auto& item : il){
            item.Print();
        }
        Serial.println(F("----"));

        ti2.RemoveFromChain();
        for(auto& item : il){
            item.Print();
        }

        delay(14200);
    }
 @endcode

NOTE: chains and intrusive lists are not threadsafe/interrupt safe
      different chains/lists can be used from different threads without problems.

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

#ifndef InstantIntrusiveList_INCLUDED_H
#define InstantIntrusiveList_INCLUDED_H

//______________________________________________________________________________
// Configurable error handling

/* Common configuration to be included only if available
   (you can separate file and/or configure individually
    or just skip that to stick with defaults) */
#if defined(__has_include) && __has_include("InstantRTOS.Config.h")
#   include "InstantRTOS.Config.h"
#endif

#ifndef InstantIntrusiveListPanic
    ///Special case to handle coroutine in unexpected state
    /** NOTE: coroutine used after completion also falls here */
#   define InstantIntrusiveListPanic() /* TODO: your custom action here! */ for(;;){}
#endif


//______________________________________________________________________________
// Public API

/// Element for building circular chain of items
/** Single element (item/node/link) of a circular doubly linked list
 * The treatment of such element depends of usage context,
 * see IntrusiveList below as one of the usage variants.
 * REMEMBER: Collections with ChainElement are NOT a "standard containers".
 *           usual "standard" ownership rules do not apply!
 * NOTE: long method names prevent mixing with derived class methods,
 *       so one can derive from ChainElement without clashing names :) */
class ChainElement{
protected:
    //ban copying (cannot chain moveable elements with pointers!)
    constexpr ChainElement(const ChainElement&) = delete;
    ChainElement& operator=(const ChainElement&) = delete;

    /// By default ChainElement is "chained to self"  
    ChainElement() = default;
    /// Destructor asserts we shall remove item from chain first
    ~ChainElement();
public:

    ///Test if this ChainElement is not chained to other ChainElements
    bool IsChainElementSingle() const;

    /// Obtain next ChainElement in the chain (the item following this one)
    ChainElement* NextChainElement();
    /// Obtain previous ChainElement in the chain (the item preceding this one)
    ChainElement* PrevChainElement();

    /// Obtain next ChainElement in the chain (the item following this one)
    const ChainElement* NextChainElement() const;
    /// Obtain previous ChainElement in the chain (the item preceding this one)
    const ChainElement* PrevChainElement() const;

    ///Insert ChainElement to be NEXT for this one (after this)
    /** Remove argument ChainElement from any other chain, then
     *  insert that argument AFTER this ChainElement (so it will be next
     *  for this and from now our NextChainElement will point to it) */
    void InsertNextChainElement(ChainElement* chainElementBeingInserted);

    ///Insert ChainElement to be PREVIOUS for this one (before this)
    /** Remove argument ChainElement from any other chain, then
     *  insert that argument BEFORE this ChainElement (so it will be previous
     *  for this and from now our PrevChainElement will point to it) */
    void InsertPrevChainElement(ChainElement* chainElementBeingInserted);

    ///Remove item from existing chain and cycle within own chain only
    void RemoveFromChain();

private:
    ChainElement* next = this;
    ChainElement* prev = this;

    /// Internal method to remove from other chain
    void removeFromOtherChainOnly();
};


/// Common base for all intrusive lists
/** The simplest possible implementation for chaining the items (list nodes!)
 * "allocated somewhere else" (!) to not use any allocations at all
 * REMEMBER: items are never copied into that list, instead they are just 
 *           nodes chained into double linked list via IntrusiveList::Node base!
 * REMEMBER: iterations pretend to be compatible with the standard library
 *           iteration conventions (begin/end, etc),
 *           BUT operations on the list use own intrusive approach! */
template <class ItemType>
class IntrusiveList{
public:
    //ban copying (this ensures pointers are valid)
    constexpr IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    /// Defaults to empty list
    IntrusiveList() = default;


    /// The base class for all IntrusiveList items
    /** Hold chain infomation, derive potential item from this class
     *  to make it chainable into the intrusive linked list
     *  NOTE: long method names prevent mixing with class methods */
    class Node: public ChainElement{
    protected:
        //There is no way to create node then from derived class
        Node() = default;
        ~Node() = default;

    public:

        /// Next Node in the chain of list items
        Node* NextListNode();
        /// Previous Node in the chain of list items
        Node* PrevListNode();

        /// Next Node in the chain of list items
        const Node* NextListNode() const;
        /// Previous Node in the chain of list items
        const Node* PrevListNode() const;

        ///Access to entire list item (derived class)
        ItemType* CastToListNode();
        ///Access to entire list item (derived class)
        const ItemType* CastToListNode() const;
    };


    /// Test list is empty
    /** Returns true if there are no elements in the list, false otherwise
     *  NOTE: count is not maintained intentionally to allow adding and removing
     *        items via operations available from ChainElement */ 
    bool IsEmpty() const;


    /// Inserts a nodeToBeInserted at the start of the list
    /** That node will be the first one in the sequence */
    void InsertAtFront(ItemType* nodeToBeInserted);

    /// Inserts a nodeToBeInserted at the end of the list
    /** That node will be the last valid node one in the sequence */
    void InsertAtBack(ItemType* nodeToBeInserted);

    /// Removes a node from the start of the list (if any)
    /** First Node is removed from the chain of elements,
     *  but not deleted (IntrusiveList does not manage lifetime).
     *  @returns pointer to the removed node, or nullptr if nothing to remove */
    ItemType* RemoveAtFront();

    /// Removes a node from the end of the list (if any)
    /** The last Node is removed from the chain of elements,
     *  but not deleted (IntrusiveList does not manage lifetime).
     *  @returns pointer the to removed node, or nullptr if nothing to remove */
    ItemType* RemoveAtEnd();


    ///Iterator pretending to expose API following standard (and range based for support)
    /** Provide operations typically expected from the standard iterators,
     * to make iteration compatible with range based for. 
     * This base allows reusing the same algorithms for iterations 
     * REMEMBER: iterations pretend to be compatible with the standard library
     *           iteration conventions (begin/end, etc),
     *           BUT operations on the list use own intrusive approach!
     * REMEMBER: removing item currently pointed by iterator invalidates that iterator */
    template<
        class NodeType, class ListNodeType,
        class RawNodeType, class RawListNodeType //to make const_iterator comparable with iterator
    >
    class IteratorSupportedOperations{
    public:
        /// Make iterator pointing Node
        IteratorSupportedOperations(NodeType* nodeToWrap) : currentNode(nodeToWrap) {}

        /// Move to next position
        IteratorSupportedOperations& operator++(){
            currentNode = currentNode->NextListNode();
            return *this;
        }

        /// Move to previous position
        IteratorSupportedOperations& operator--(){
            currentNode = currentNode->PrevListNode();
            return *this; 
        }

        /// Move to next position (postfix)
        IteratorSupportedOperations operator++(int){
            IteratorSupportedOperations res = *this;
            currentNode = currentNode->NextListNode();
            return res;
        }
        /// Move to previous position (postfix)
        IteratorSupportedOperations operator--(int){
            IteratorSupportedOperations res = *this;
            currentNode = currentNode->PrevListNode();
            return res;
        }

        /// Access Node members
        ListNodeType* operator->() const{
            return currentNode->CastToListNode();
        }

        /// Access Node
        ListNodeType& operator*() const{
            return *currentNode->CastToListNode();
        }

        /// Check two iterators reference to the same item 
        bool operator==(
            const IteratorSupportedOperations<
                RawNodeType, RawListNodeType, RawNodeType, RawListNodeType
            >&other
        ) const {
            return this->operator->() == other.operator->();
        }
        /// Check two iterators are referencing different items
        bool operator!=(
            const IteratorSupportedOperations<
                RawNodeType, RawListNodeType, RawNodeType, RawListNodeType
            >&other
        ) const {
            return this->operator->() != other.operator->();
        }

        /// Check two iterators reference to the same item 
        bool operator==(
            const IteratorSupportedOperations<
                const RawNodeType, const RawListNodeType, RawNodeType, RawListNodeType
            >&other
        ) const {
            return this->operator->() == other.operator->();
        }
        /// Check two iterators are referencing different items
        bool operator!=(
            const IteratorSupportedOperations<
                const RawNodeType, const RawListNodeType, RawNodeType, RawListNodeType
            >&other
        ) const {
            return this->operator->() != other.operator->();
        }

    private:
        /// The Node pointed by iterator
        NodeType* currentNode;
    };
    
    /// Iterator partially compatible standard
    using iterator = IteratorSupportedOperations<Node, ItemType, Node, ItemType>;

    /// Iterator partially compatible standard
    using const_iterator = IteratorSupportedOperations<const Node, const ItemType, Node, ItemType>;


    //TODO: reverse_iterator, const_reverse_iterator


    ///Iterator to the beginning of the sequence
    /** Make IntrusiveList compatible with range-based for,
     * https://en.cppreference.com/w/cpp/language/range-for */
    iterator begin();
    ///Iterator to the end of the sequence
    /** Make IntrusiveList compatible with range-based for,
     * https://en.cppreference.com/w/cpp/language/range-for
     * (remember: do not dereference end iterator!) */
    iterator end();

    ///Iterator to the beginning of the const sequence
    /** Make IntrusiveList compatible with range-based for,
     * https://en.cppreference.com/w/cpp/language/range-for */
    const_iterator begin() const;
    ///Iterator to the end of the const sequence
    /** Make IntrusiveList compatible with range-based for,
     * https://en.cppreference.com/w/cpp/language/range-for */
    const_iterator end() const;

    ///Const iterator to the beginning of the sequence
    const_iterator cbegin() const;
    ///Const iterator to the end of the sequence
    const_iterator cend() const;

private:
    /// Special node to not contain any data
    class RootNode: public Node{
    public:
        RootNode() = default;
        ~RootNode() = default;
    };
    ///The head of the chain of linked nodes
    /** Also serves as "end" of forward and reverse sequences */
    RootNode listHead;
};




//______________________________________________________________________________
//##############################################################################
/*==============================================================================
*  Implementation details follow                                               *
*=============================================================================*/
//##############################################################################




inline ChainElement::~ChainElement(){
    //assertion to ensure enclosing chain is not broken
    if( !IsChainElementSingle() ){
        /* Destroying before removed from the chain
           is likely an error (because those chain elements
           usually have additional logic) */
        InstantIntrusiveListPanic();
    }
}

inline bool ChainElement::IsChainElementSingle() const{
    //Chain is always circle, so enough to check one member
    return next == this;
}

inline ChainElement* ChainElement::NextChainElement() {
    return next;
}

inline ChainElement* ChainElement::PrevChainElement() {
    return prev;
}

inline const ChainElement* ChainElement::NextChainElement() const {
    return next;
}

inline const ChainElement* ChainElement::PrevChainElement() const {
    return prev;
}

inline void ChainElement::InsertNextChainElement(ChainElement* chainElementBeingInserted){
    /*reuse existing InsertPrevChainElement
        (inserting after this means inserting before next of this) */
    next->InsertPrevChainElement(chainElementBeingInserted);
}

inline void ChainElement::InsertPrevChainElement(ChainElement* chainElementBeingInserted){
    /* check ensure we do not break current chain by
        inserting the same item in the same place (before self) */
    if( chainElementBeingInserted != this ){
        //safely remove that item from any previous chains 
        chainElementBeingInserted->removeFromOtherChainOnly();

        //insert between previous and this one
        prev->next = chainElementBeingInserted;
        chainElementBeingInserted->prev = prev;
        chainElementBeingInserted->next = this;
        prev = chainElementBeingInserted;
    }
}

inline void ChainElement::RemoveFromChain(){
    removeFromOtherChainOnly();

    /* this ensures ChainElement is now in "own" chain
        and other RemoveFromChain call will work correctly */
    prev = next = this;
}


inline void ChainElement::removeFromOtherChainOnly(){
    prev->next = next;
    next->prev = prev;
}



template <class ItemType>
inline typename IntrusiveList<ItemType>::Node* IntrusiveList<ItemType>::Node::NextListNode() {
    return static_cast<Node*>(NextChainElement());
}

template <class ItemType>
inline typename IntrusiveList<ItemType>::Node* IntrusiveList<ItemType>::Node::PrevListNode() {
    return static_cast<Node*>(PrevChainElement());
}


template <class ItemType>
inline const typename IntrusiveList<ItemType>::Node* IntrusiveList<ItemType>::Node::NextListNode() const {
    return static_cast<const Node*>(NextChainElement());
}
/// Previous Node in the chain of list items
template <class ItemType>
inline const typename IntrusiveList<ItemType>::Node* IntrusiveList<ItemType>::Node::PrevListNode() const {
    return static_cast<Node*>(PrevChainElement());
}

template <class ItemType>
inline ItemType* IntrusiveList<ItemType>::Node::CastToListNode(){
        return static_cast<ItemType*>(this);
}
template <class ItemType>
inline const ItemType* IntrusiveList<ItemType>::Node::CastToListNode() const{
        return static_cast<const ItemType*>(this);
}

template <class ItemType>
inline bool IntrusiveList<ItemType>::IsEmpty() const{
    return listHead.IsChainElementSingle();
}

template <class ItemType>
inline void IntrusiveList<ItemType>::InsertAtFront(ItemType* nodeToBeInserted){
    listHead.InsertNextChainElement(nodeToBeInserted);
}

template <class ItemType>
inline void IntrusiveList<ItemType>::InsertAtBack(ItemType* nodeToBeInserted){
    listHead.InsertPrevChainElement(nodeToBeInserted);
}

template <class ItemType>
inline ItemType* IntrusiveList<ItemType>::RemoveAtFront(){
    if( !listHead.IsChainElementSingle() ){
        Node* res = listHead.NextListNode();
        res->RemoveFromChain();
        return res->CastToListNode();
    }
    return nullptr;
}

template <class ItemType>
inline ItemType* IntrusiveList<ItemType>::RemoveAtEnd(){
    if( !listHead.IsChainElementSingle() ){
        Node* res = listHead.PrevListNode();
        res->RemoveFromChain();
        return res->CastToListNode();
    }
    return nullptr;
}


template <class ItemType>
inline typename IntrusiveList<ItemType>::iterator IntrusiveList<ItemType>::begin(){
    return listHead.NextListNode();
}
template <class ItemType>
inline typename IntrusiveList<ItemType>::iterator IntrusiveList<ItemType>::end(){
    return &listHead;
}

template <class ItemType>
inline typename IntrusiveList<ItemType>::const_iterator IntrusiveList<ItemType>::begin() const {
    return listHead.NextListNode();
}
template <class ItemType>
inline typename IntrusiveList<ItemType>::const_iterator IntrusiveList<ItemType>::end() const {
    return &listHead;
}

template <class ItemType>
inline typename IntrusiveList<ItemType>::const_iterator IntrusiveList<ItemType>::cbegin() const {
    return listHead.NextListNode();
}
template <class ItemType>
inline typename IntrusiveList<ItemType>::const_iterator IntrusiveList<ItemType>::cend() const {
    return &listHead;
}

#endif
