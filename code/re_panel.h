#pragma once

#include "re_text_buffer.h"

// TODO(Naor): Currently, we treat a panel as the entire window,
// add a viewport to each panel and render only to it.
// (We can use glScisssor and glClear to clear only part of the window)
struct panel
{
    text_buffer* TextBuffer;
    // TODO(Naor): This might leak! we might want to introduce smart pointers at this point.
    // (Or notify everyone that is attached to text_buffer to update)
    char* BufferPointer;
    
    inline void Attach(text_buffer* Buffer)
    {
        TextBuffer = Buffer;
        BufferPointer = TextBuffer->LastBufferPointer;
    }
    
    // TODO(Naor): We need the current modifiers state so we can
    // manipulate the text more appropriately.
    void AppendText(char Text[32])
    {
        while(*Text)
        {
            if(*Text > 0 && *Text < 128)
            {
                if(*Text == ' ' || *Text == '\r')
                {
                    TextBuffer->AddAutoComplete();
                }
                
                *BufferPointer++ = *Text;
                ++TextBuffer->BufferEnd;
                
                if(BufferPointer != TextBuffer->BufferEnd)
                {
                    // TODO(Naor): This is really bad, find another solution for append
                    // text in the middle of the buffer (there is a solution in 
                    // Trello, implement it)
                    char* SwapPointer = TextBuffer->BufferEnd + 1;
                    while(SwapPointer - 1 != BufferPointer)
                    {
                        *SwapPointer-- = *(SwapPointer-1);
                    }
                }
                
                ++TextBuffer->Size;
            }
            ++Text;
        }
    }
    
    void OtherKeyPress(SDL_Keycode KeyCode)
    {
        switch(KeyCode)
        {
            case SDLK_TAB:
            {
                GetAutoComplete();
            }break;
            
            case SDLK_BACKSPACE:
            {
                if(TextBuffer->Size > 0 && BufferPointer != TextBuffer->Buffer)
                {
                    if(BufferPointer == TextBuffer->BufferEnd)
                    {
                        *(--BufferPointer) = 0;
                        TextBuffer->BufferEnd = BufferPointer;
                    }
                    else
                    {
                        // TODO(Naor): This is really bad, find another solution for append
                        // text in the middle of the buffer (there is a solution in 
                        // Trello, implement it)
                        char* SwapPointer = BufferPointer;
                        while(SwapPointer != TextBuffer->BufferEnd)
                        {
                            *SwapPointer++ = *(SwapPointer+1);
                        }
                    }
                    
                    --TextBuffer->Size;
                }
            }break;
            
            case SDLK_RETURN:
            {
                AppendText("\r");
            }break;
        }
    }
    
    void GetAutoComplete()
    {
        if(g_RequestNewAutoComplete)
        {
            // TODO(Naor): Move this to a utility function
            char* Scan = TextBuffer->BufferEnd;
            while(Scan != TextBuffer->Buffer && *(Scan-1) != ' ' && *(Scan-1) != '\r')
            {
                --Scan;
            }
            
            g_LastSearchedWord = "";
            g_LastSearchedWord.reserve(TextBuffer->BufferEnd - Scan);
            while(Scan != TextBuffer->BufferEnd)
            {
                g_LastSearchedWord.push_back(*Scan++);
            }
            
            g_LastCompletedWords = g_AutoComplete.Get(g_LastSearchedWord);
            g_LastCompletedWordsIndex = 0;
        }
        
        // We want to check again if the list is not empty
        if(!g_LastCompletedWords.empty())
        {
            std::string NextWord = g_LastCompletedWords[g_LastCompletedWordsIndex];
            size_t StartIndex = g_LastSearchedWord.size();
            
            int32 BufferEndAdvance = 0;
            for(size_t NextWordIndex = StartIndex;
                NextWordIndex < NextWord.size(); 
                ++NextWordIndex)
            {
                ++BufferEndAdvance;
                *BufferPointer++ = NextWord[NextWordIndex];
            }
            
            TextBuffer->BufferEnd += BufferEndAdvance;
            TextBuffer->Size += BufferEndAdvance;
            
            ++g_LastCompletedWordsIndex;
            if(g_LastCompletedWordsIndex >= g_LastCompletedWords.size())
            {
                g_LastCompletedWordsIndex = 0;
            }
        }
    }
    
    void Render()
    {
        glClearColor(0.047f, 0.047f, 0.047f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(g_ShaderProgram);
        // NOTE(Naor): This might not be neccessary, Texture0 should be
        // active by default.
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(g_Vao);
        
        float X = 0.0f;
        float Y = static_cast<float>(g_DrawableHeight) - g_MaxFontHeight;
        const char* Scan = TextBuffer->Buffer;
        while(*Scan)
        {
            // TODO(Naor): Check if this is ok for other platforms
            if(*Scan == '\r')
            {
                X = 0.0f;
                Y -= g_MaxFontHeight;
                Scan++;
                continue;
            }
            
            character Char = g_Characters[*Scan++];
            
            float XPos = X + Char.Bearing.X; // * Scale
            float YPos = Y - (Char.Size.Y - Char.Bearing.Y); // * Scale
            
            float Width = static_cast<float>(Char.Size.X); // * Scale
            float Height = static_cast<float>(Char.Size.Y); // * Scale
            
            float Vertices[6][4] = {
                {XPos, YPos + Height, 0.0f, 0.0f},
                {XPos, YPos, 0.0f, 1.0f},
                {XPos + Width, YPos, 1.0f, 1.0f},
                
                {XPos, YPos + Height, 0.0f, 0.0f},
                {XPos + Width, YPos, 1.0f, 1.0f},
                {XPos + Width, YPos + Height, 1.0f, 0.0f},
            };
            
            glBindTexture(GL_TEXTURE_2D, Char.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, g_Vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            X += (Char.Advance >> 6); // * Scale
        }
        
        if(Scan == BufferPointer)
        {
            renderer::DrawQuad(vec2{X, Y},
                               vec2{g_MaxFontWidth, g_MaxFontHeight},
                               vec4{1.0f, 1.0f, 0.6f, 1.0f});
        }
    }
};