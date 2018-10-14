#include <gl3w.c>
#include <SDL.h>
#include <windows.h>
#include <cstdint>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "re_types.h"
#include "re_math.h"
#include "re_platform.h"
#include "re_memory.h"
#include "re_opengl.h"
#include "re_utility.h"
#include "re_renderer.h"
#include "re_autocomplete.h"

// TODO(Naor): Move all these globals above #include "text_buffer.h" to
// a struct or another place. this is temporary place for them
// just to make everything work until I refactor the code.
static constexpr int32 MAX_CHARACTERS_TO_LOAD = 128;
static constexpr int32 MAX_TEXT_BUFFER = 1028;

static auto_complete_tree g_AutoComplete;
static bool g_RequestNewAutoComplete;
static std::vector<std::string> g_LastCompletedWords;
static int32 g_LastCompletedWordsIndex;
static std::string g_LastSearchedWord;

static uint32 g_Vao;
static uint32 g_Vbo;
static uint32 g_ShaderProgram;
static FT_Library g_FontLibrary;
static FT_Face g_FontFace;

static float g_MaxFontHeight;
static float g_MaxFontWidth;

static int32 g_DrawableWidth;
static int32 g_DrawableHeight;

static m4 g_Projection;

// TODO(Naor): Do we really want it to be unordered_map? 
// maybe we want to access it with an index because all the characters
// going to be known at runtime, we just convert them to an index
// and check if they are in the correct range.
static std::unordered_map<char, character> g_Characters;

#include "re_text_buffer.h"
#include "re_panel.h"

static bool g_Running;
static bool g_ShouldRender;

// NOTE(Naor): This is grabbed from the global scope (extern)
platform_api Platform;

static void InitializeSimpleText()
{
    int32 Error = FT_Init_FreeType(&g_FontLibrary);
    if(Error == 0)
    {
        // NOTE(Naor): If we already loaded the font to memory,
        // we can use FT_New_Memory_Face to create a new face object.
        
        // NOTE(Naor): To know how many faces a given font file contains,
        // set face_index to -1, then check the value of face->num_faces,
        // which indicates how many faces are embedded in the font file.
        Error = FT_New_Face(g_FontLibrary, "OpenSans-Regular.ttf", 0, &g_FontFace);
        if(Error == 0)
        {
            // NOTE(Naor): Or use FT_Set_Char_Size
            Error = FT_Set_Pixel_Sizes(
                g_FontFace,   /* handle to face object */
                0,      /* pixel_width           */
                18);   /* pixel_height          */
            
            
            // TODO(Naor): Make this into a single bitmap instead of every single
            // char into a texture itself.
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            for(uint32 Char = 0;
                Char < MAX_CHARACTERS_TO_LOAD;
                ++Char)
            {
                if(FT_Load_Char(g_FontFace, Char, FT_LOAD_RENDER) != 0)
                {
                    OutputDebugString("Failed loading char\n");
                    continue;
                }
                
                uint32 Texture;
                glGenTextures(1, &Texture);
                glBindTexture(GL_TEXTURE_2D, Texture);
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    g_FontFace->glyph->bitmap.width,
                    g_FontFace->glyph->bitmap.rows,
                    0, 
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    g_FontFace->glyph->bitmap.buffer
                    );
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                character Character;
                Character.TextureID = Texture;
                Character.Size = {static_cast<int32>(g_FontFace->glyph->bitmap.width), static_cast<int32>(g_FontFace->glyph->bitmap.rows)};
                Character.Bearing = {g_FontFace->glyph->bitmap_left, g_FontFace->glyph->bitmap_top};
                Character.Advance = g_FontFace->glyph->advance.x;
                
                if(static_cast<float>(Character.Size.Y) > g_MaxFontHeight)
                    g_MaxFontHeight = static_cast<float>(Character.Size.Y);
                if(static_cast<float>(Character.Size.X) > g_MaxFontWidth)
                    g_MaxFontWidth = static_cast<float>(Character.Size.X);
                
                g_Characters.insert(std::pair<char, character>((char)Char, Character));
            }
            
            // Loading the shaders
            const char* VertexSource = 
                "#version 330 core\n"
                "layout (location = 0) in vec4 Vertex;\n"
                "out vec2 TexCoords;\n"
                "\n"
                "uniform mat4 Projection;\n"
                "\n"
                "void main()\n"
                "{\n"
                "gl_Position = Projection * vec4(Vertex.xy, 0.0, 1.0);\n"
                "TexCoords = Vertex.zw;\n"
                "}\n";
            const char* FragmentSource = 
                "#version 330 core\n"
                "in vec2 TexCoords;\n"
                "out vec4 Color;\n"
                "\n"
                "uniform sampler2D Text;\n"
                "\n"
                "void main()\n"
                "{    \n"
                "vec4 Sampled = vec4(1.0, 1.0, 1.0, texture(Text, TexCoords).r);\n"
                "Color = vec4(0.56, 0.69, 0.5, 1.0) * Sampled;\n"
                "}  \n";
            
            g_ShaderProgram = CompileShaderProgram(VertexSource, FragmentSource);
            
            glUseProgram(g_ShaderProgram);
            g_Projection = math::OrthographicProjection(0.0f, static_cast<float>(g_DrawableWidth), 0.0f, static_cast<float>(g_DrawableHeight)); 
            glUniformMatrix4fv(glGetUniformLocation(g_ShaderProgram, "Projection"), 1, GL_FALSE, g_Projection.E[0]);
            
            // NOTE(Naor): We want to enable blending because we have 
            // to draw quads for the text.
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            glGenVertexArrays(1, &g_Vao);
            glGenBuffers(1, &g_Vbo);
            glBindVertexArray(g_Vao);
            glBindBuffer(GL_ARRAY_BUFFER, g_Vbo);
            // The quad
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glEnableVertexAttribArray(0);
        }
        else
        {
            OutputDebugString("Couldn't load the given font.");
        }
    }
    else
    {
        OutputDebugString("Couldn't initialize freetype.");
    }
    
    // TODO(Naor): Do we really want to tell FreeType that we are done with it?
    // maybe we would like to load fonts on the fly and reloading it would be a waste.
    // (We will probably wont be loading fonts that often so maybe we should free it)
    FT_Done_Face(g_FontFace);
}

// TODO(Naor): Those are really simple allocations for now,
// we need to align them and find a proper size based on the pages.
PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    void* Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    
    return Result;
}

PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    VirtualFree(Block, 0, MEM_RELEASE);
}

static void Win32InitializePlatform()
{
    Platform.AllocateMemory = Win32AllocateMemory;
    Platform.DeallocateMemory = Win32DeallocateMemory;
}

bool32 Win32InitializeWindowAndOpenGL(SDL_Window** Window)
{
    SDL_GLContext GLContext;
    
    // TODO(Naor): Validate this with if
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        char Buffer[256];
        sprintf(Buffer, "Something went wrong while initializing SDL: %s\n", SDL_GetError());
        OutputDebugString(Buffer);
        return 1;
    }
    
    // Create an application window with the following settings:
    *Window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        800,                               // width, in pixels
        600,                               // height, in pixels
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        );
    
    // TODO(Naor): This attribute MUST come before the OpenGL context creation, 
    // the attributes are set and only when we use CreateContext it is applied.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    GLContext = SDL_GL_CreateContext(*Window);
    SDL_GL_MakeCurrent(*Window, GLContext);
    
    
    if (*Window == NULL) {
        char Buffer[256];
        sprintf(Buffer, "Could not create window: %s\n", SDL_GetError());
        OutputDebugString(Buffer);
        return 1;
    }
    
    if (gl3wInit()) {
        OutputDebugString("failed to initialize OpenGL\n");
        return 1;
    }
    if (!gl3wIsSupported(3, 2)) {
        OutputDebugString("OpenGL 3.2 not supported\n");
        return 1;
    }
    
    SDL_GL_GetDrawableSize(*Window, &g_DrawableWidth, &g_DrawableHeight);
    
    return 0;
}

int main(int argc, char* argv[])
{
    Win32InitializePlatform();
    
    SDL_Window* Window;
    if(Win32InitializeWindowAndOpenGL(&Window) != 0)
    {
        // Something went wrong with initializing the window.
        return 1;
    }
    
    InitializeSimpleText();
    
    g_AutoComplete.Initialize();
    renderer::Initialize(g_Projection);
    
    std::vector<panel*> Panels;
    panel* CurrentPanel;
    
    // NOTE(Naor): For now, we create only one panel, so this is
    // enough.
    CurrentPanel = new panel;
    Panels.push_back(CurrentPanel);
    
    // NOTE(Naor): We also will create a simple text_buffer and assign
    // it to our panel, until we will add file loading and creation of 
    // new text buffers
    text_buffer* TextBuffer = new text_buffer;
    TextBuffer->Initialize();
    CurrentPanel->Attach(TextBuffer);
    
    g_Running = true;
    g_ShouldRender = true;
    while(g_Running)
    {
        SDL_Event Event;
        while (SDL_PollEvent(&Event)) 
        {
            g_ShouldRender = true;
            switch(Event.type)
            {
                case SDL_KEYDOWN:
                //case SDL_KEYUP:
                {
                    SDL_KeyboardEvent& Key = Event.key;
                    // If we press something other than tab, we need a
                    // new autocomplete list
                    if(Key.keysym.sym != SDLK_TAB)
                    {
                        g_RequestNewAutoComplete = true;
                    }
                    
                    if(Key.keysym.sym == SDLK_ESCAPE)
                    {
                        g_Running = false;
                    }
                    else
                    {
                        CurrentPanel->OtherKeyPress(Key.keysym.sym);
                    }
                }break;
                
                case SDL_TEXTINPUT:
                {
                    CurrentPanel->AppendText(Event.text.text);
                }break;
                
                case SDL_TEXTEDITING:
                {
                    SDL_TextEditingEvent& Edit = Event.edit;
                }break;
                
                case SDL_WINDOWEVENT:
                {
                    if(Event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        SDL_GL_GetDrawableSize(Window, &g_DrawableWidth, &g_DrawableHeight);
                        glViewport(0, 0, static_cast<int32>(g_DrawableWidth), static_cast<int32>(g_DrawableHeight));
                        
                        g_Projection = math::OrthographicProjection(0.0f, static_cast<float>(g_DrawableWidth), 0.0f, static_cast<float>(g_DrawableHeight));
                        renderer::UpdateProjection(g_Projection);
                        
                        // TODO(Naor): Move this into the renderer
                        glUseProgram(g_ShaderProgram);
                        glUniformMatrix4fv(glGetUniformLocation(g_ShaderProgram, "Projection"), 1, GL_FALSE, g_Projection.E[0]);
                    }
                }break;
                
                case SDL_QUIT:
                {
                    g_Running = false;
                }break;
            }
        }
        
        if(g_ShouldRender)
        {
            g_ShouldRender = false;
            
            // TODO(Naor): We might want to render everything instead of just the current panel,
            // for example, if two panels have the same text_buffer, and one panel is changing
            // the buffer, we want to rerender both of them. (maybe render based on the text_buffer)
            CurrentPanel->Render();
            
            OutputDebugString(CurrentPanel->TextBuffer->Buffer);
            OutputDebugString("\n------------------\n");
            
            SDL_GL_SwapWindow(Window);
        }
    }
    
    return 0;
}