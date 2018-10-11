#pragma once
#include <algorithm>
#include <string>
#include <stack>
#include "memory.h"

// TODO(Naor): For now, this will only support english letters, without digits or underscores,
// fix it that it will support them.
static constexpr int NUM_OF_AUTO_COMPLETE_CHARS = 26;
static constexpr int MAX_NUM_OF_WORDS_TO_AUTO_COMPLETE = 16384;

struct auto_complete_node
{
    auto_complete_node* Characters[NUM_OF_AUTO_COMPLETE_CHARS];
    // TODO(Naor): Change the data structure
    std::vector<std::string> Words;
};

// TODO(Naor): Make the algorithm remember the word used in a list,
// and use it more often. (For example if there is A and AB, and the autocomplete
// returns A, but the user uses AB (after another tab), the autocomplete will return AB first
// instead of A. ****(Maybe give a timer when the word was last used, and use the latest one)
// TODO(Naor): Implement the remove functionality.
struct auto_complete_tree
{
    auto_complete_node* Root;
    // TODO(Naor): Change the structure to be dynamically allocated so
    // if the user go above the max, it will allocate more memory.
    fill_memory_static<auto_complete_node> Memory;
    
    void Initialize()
    {
        Memory.Initialize(MAX_NUM_OF_WORDS_TO_AUTO_COMPLETE);
        Root = Memory.New();
    }
    
    void Add(const std::string& Word)
    {
        std::string FixedWord = ToLower(Word);
        std::stack<auto_complete_node*> NodeTrace;
        NodeTrace.push(Root);
        
        auto_complete_node* Current = Root;
        for(char Char : FixedWord)
        {
            // If any character in the string is outside the english 
            // letters, the function will do nothing.
            if(Char < 'a' || Char > 'z')
            {
                return;
            }
            
            int32 Index = (int32)(Char - 'a');
            if(!Current->Characters[Index])
            {
                Current->Characters[Index] = Memory.New();
            }
            Current = Current->Characters[Index];
            
            NodeTrace.push(Current);
        }
        
        auto_complete_node* LastNode = NodeTrace.top();
        for(const std::string Duplicate : LastNode->Words)
        {
            // If the tree contains that word, skip the insertion.
            if(Duplicate == Word)
                return;
        }
        
        while(!NodeTrace.empty())
        {
            NodeTrace.top()->Words.push_back(Word);
            NodeTrace.pop();
        }
    }
    
    std::vector<std::string> Get(const std::string& PartialWord)
    {
        if(PartialWord == "")
        {
            return {};
        }
        
        std::string FixedWord = ToLower(PartialWord);
        auto_complete_node* Current = Root;
        
        for(char Char : FixedWord)
        {
            int32 Index = (int32)(Char - 'a');
            if(Char < 'a' || Char > 'z' || !Current->Characters[Index])
            {
                return {};
            }
            
            Current = Current->Characters[Index];
        }
        
        return Current->Words;
    }
};

