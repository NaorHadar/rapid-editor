#pragma once
#include <vector>

// NOTE(Naor): This struct will add and free member of constant size, and have the ability
// to avoid empty gaps in memory.
template<typename type>
struct fill_memory_static
{
    type* Block;
    size_t Size;
    type* End;
    std::vector<type*> EmptyGaps;
    
    // NOTE(Naor): SizeInit is number of elements in the block
    inline void Initialize(size_t SizeInit)
    {
        Block = (type*)Platform.AllocateMemory(SizeInit * sizeof(type));
        Size = 0;
        End = Block;
    }
    
    inline type* New()
    {
        Size++;
        
        type* StorePointer;
        if(!EmptyGaps.empty())
        {
            StorePointer = EmptyGaps.back();
            EmptyGaps.pop_back();
        }
        else
        {
            StorePointer = End++;
        }
        
        return StorePointer;
    }
    
    // NOTE(Naor): This will COPY the element into the block (using copy-ctor
    // if there is one).
    inline type* Add(type Element)
    {
        Size++;
        
        type* StorePointer = New();
        *StorePointer = Element;
        
        return StorePointer;
    }
    
    // NOTE(Naor): This will NOT delete the element given, it will
    // only mark it as "empty" so we can use it later on.
    inline void Remove(type* ToRemove)
    {
        // If we remove something from the end of the block, move the end pointer.
        if(ToRemove == (End-1))
        {
            End--;
        }
        
        EmptyGaps.push_back(ToRemove);
        Size--;
    }
};

// TODO(Naor): This should be a memory structure that will expand dynamically.
template<typename type>
struct fill_memory_dynamic
{
    
};