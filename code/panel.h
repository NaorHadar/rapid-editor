#pragma once

#include "text_buffer.h"

// TODO(Naor): Currently, we treat a panel as the entire window,
// add a viewport to each panel and render only to it.
// (We can use glScisssor and glClear to clear only part of the window)
struct panel
{
    text_buffer* TextBuffer;
    
    // TODO(Naor): For now we use KMOD from SDL, maybe move
    // that to our own enum
    void KeyPressed(char CharPressed, int32 Mod = KMOD_NONE)
    {
        if(CharPressed == SDLK_BACKSPACE)
        {
            if(TextBuffer->Size > 0)
            {
                *(--TextBuffer->BufferEnd) = 0;
                TextBuffer->Size--;
            }
        }
        else
        {
            
            if(CharPressed == ' ' || CharPressed == '\r')
            {
                TextBuffer->AddAutoComplete();
            }
            
            // TODO(Naor): Handle capslock
            if(Mod & KMOD_SHIFT)
            {
                if(CharPressed >= 'a' && CharPressed <= 'z')
                {
                    // TODO(Naor): Make this compile time number.
                    CharPressed -= 'a' - 'A'; 
                }
                else if(CharPressed >= 'A' && CharPressed <= 'Z')
                {
                    // TODO(Naor): Make this compile time number.
                    CharPressed += 'a' - 'A'; 
                }
            }
            
            // TODO(Naor): This might overflow!
            *TextBuffer->BufferEnd++ = CharPressed;
            TextBuffer->Size++;
        }
    }
    
    void GetAutoComplete()
    {
        if(RequestNewAutoComplete)
        {
            // TODO(Naor): Move this to a utility function
            char* Scan = TextBuffer->BufferEnd;
            while(Scan != TextBuffer->Buffer && *(Scan-1) != ' ' && *(Scan-1) != '\r')
            {
                --Scan;
            }
            
            LastSearchedWord = "";
            LastSearchedWord.reserve(TextBuffer->BufferEnd - Scan);
            while(Scan != TextBuffer->BufferEnd)
            {
                LastSearchedWord.push_back(*Scan++);
            }
            
            LastCompletedWords = AutoComplete.Get(LastSearchedWord);
            LastCompletedWordsIndex = 0;
        }
        
        // We want to check again if the list is not empty
        if(!LastCompletedWords.empty())
        {
            std::string NextWord = LastCompletedWords[LastCompletedWordsIndex];
            size_t StartIndex = LastSearchedWord.size();
            
            for(size_t NextWordIndex = StartIndex;
                NextWordIndex < NextWord.size(); 
                ++NextWordIndex)
            {
                ++TextBuffer->Size;
                *TextBuffer->BufferEnd++ = NextWord[NextWordIndex];
            }
            
            LastCompletedWordsIndex++;
            if(LastCompletedWordsIndex >= LastCompletedWords.size())
            {
                LastCompletedWordsIndex = 0;
            }
        }
    }
    
    void Render()
    {
        glClearColor(0.047f, 0.047f, 0.047f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(ShaderProgram);
        // NOTE(Naor): This might not be neccessary, Texture0 should be
        // active by default.
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(Vao);
        
        float X = 0.0f;
        float Y = (float)DrawableHeight - MaxFontHeight;
        const char* Scan = TextBuffer->Buffer;
        while(*Scan)
        {
            // TODO(Naor): Check if this is ok for other platforms
            if(*Scan == '\r')
            {
                X = 0.0f;
                Y -= MaxFontHeight;
                Scan++;
                continue;
            }
            
            character Char = Characters[*Scan++];
            
            float XPos = X + Char.Bearing.X; // * Scale
            float YPos = Y - (Char.Size.Y - Char.Bearing.Y); // * Scale
            
            float Width = (float)Char.Size.X; // * Scale
            float Height = (float)Char.Size.Y; // * Scale
            
            float Vertices[6][4] = {
                {XPos, YPos + Height, 0.0f, 0.0f},
                {XPos, YPos, 0.0f, 1.0f},
                {XPos + Width, YPos, 1.0f, 1.0f},
                
                {XPos, YPos + Height, 0.0f, 0.0f},
                {XPos + Width, YPos, 1.0f, 1.0f},
                {XPos + Width, YPos + Height, 1.0f, 0.0f},
            };
            
            glBindTexture(GL_TEXTURE_2D, Char.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, Vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            X += (Char.Advance >> 6); // * Scale
        }
    }
};