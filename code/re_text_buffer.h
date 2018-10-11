#pragma once

static constexpr int32 DEFAULT_BUFFER_SIZE = 65536;

// TODO(Naor): Maybe find a better name for this
struct text_buffer
{
    char* Buffer;
    char* BufferEnd;
    char* BufferPointer;
    int32 Size;
    
    inline void Initialize(int32 BufferSize = DEFAULT_BUFFER_SIZE)
    {
        if(BufferSize < DEFAULT_BUFFER_SIZE)
        {
            BufferSize = DEFAULT_BUFFER_SIZE;
        }
        
        // TODO(Naor): We need a better structure for this, 
        // the memory might overflow.
        Buffer = (char*)Platform.AllocateMemory(BufferSize);
        BufferEnd = Buffer;
        BufferPointer = Buffer;
        Size = 0;
    }
    
    std::string GetWord()
    {
        std::string Result;
        // TODO(Naor): Change BufferEnd to BufferPointer after
        // we set the BufferPointer to the appropriate place.
        char* Scan = BufferEnd;
        
        // Find the beginning of the word
        while(Scan != Buffer && *(Scan-1) != ' ' && *(Scan-1) != '\r')
        {
            --Scan;
        }
        
        Result.reserve(BufferEnd - Scan);
        while(Scan != BufferEnd)
        {
            Result.push_back(*Scan++);
        }
        
        return Result;
    }
    
    inline void AddAutoComplete()
    {
        std::string NewWord = GetWord();
        
        AutoComplete.Add(NewWord);
    }
};