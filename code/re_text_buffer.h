#pragma once

static constexpr int32 DEFAULT_BUFFER_SIZE = 65536;

// TODO(Naor): Maybe find a better name for this
struct text_buffer
{
    // TODO(Naor): Change the buffers, they may overflow at some point.
    char* Buffer;
    char* BufferEnd;
    // NOTE(Naor): LastBufferPointer is used for knowning where was the pointer
    // at, if we open this text_buffer with another panel, it will set the pointer
    // to the last set pointer (LastBufferPointer)
    char* LastBufferPointer;
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
        LastBufferPointer = Buffer;
        Size = 0;
    }
    
    std::string GetWord()
    {
        std::string Result;
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
        
        g_AutoComplete.Add(NewWord);
    }
};