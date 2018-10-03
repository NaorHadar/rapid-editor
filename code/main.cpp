#include <gl3w.c>
#include <SDL.h>
#include <iostream>
#include <cstdint>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using bool32 = int32_t;

struct ivec2
{
    int32 X;
    int32 Y;
};

struct vec4
{
    float X;
    float Y;
    float Z;
    float W;
};

struct m4
{
    float E[4][4];
};

struct character
{
    uint32 TextureID;
    ivec2 Size;
    ivec2 Bearing;
    uint32 Advance;
};

static bool Running;
static bool ShouldRender;

// Triangle stuff
static uint32 Vao;
static uint32 Vbo;
static uint32 ShaderProgram;

// Font stuff
static FT_Library FontLibrary;
static FT_Face FontFace;

// TODO(Naor): Do we really want it to be unordered_map? 
// maybe we want to access it with an index because all the characters
// going to be known at runtime, we just convert them to an index
// and check if they are in the correct range.
static std::unordered_map<char, character> Characters;

// NOTE(Naor): Thanks to GLM.
m4 OrthographicProjection(float Left, float Right, float Bottom, float Top)
{
    m4 Result = {};
    Result.E[3][3] = 1.0f;
    
    Result.E[0][0] = 2.0f / (Right - Left);
    Result.E[1][1] = 2.0f / (Top - Bottom);
    Result.E[2][2] = -1.0f;
    Result.E[3][0] = - (Right + Left) / (Right - Left);
    Result.E[3][1] = - (Top + Bottom) / (Top - Bottom);
    
    return Result;
}

static void CompileShaderProgram(const char* VertexSource, const char* FragmentSource)
{
    uint32 VertexShader = glCreateShader(GL_VERTEX_SHADER);
    uint32 FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(VertexShader, 1, &VertexSource, NULL);
    glCompileShader(VertexShader);
    glShaderSource(FragmentShader, 1, &FragmentSource, NULL);
    glCompileShader(FragmentShader);
    
    ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);
    glLinkProgram(ShaderProgram);
    
    int32 Success;
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if(!Success) {
        char Buffer[256];
        glGetProgramInfoLog(ShaderProgram, 512, NULL, Buffer);
        OutputDebugString(Buffer);
    }
    
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
}

static void InitializeSimpleTriangle()
{
    // NOTE(Naor): Setup the triangle buffer
    glGenVertexArrays(1, &Vao);
    glGenBuffers(1, &Vbo);
    
    glBindVertexArray(Vao);
    
    // NOTE(Naor): 1.0 is top for OpenGL
    static float Vertices[] = {
        0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, Vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    
    // NOTE(Naor): Setup the program for the triangle
    const char* VertexSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";
    const char* FragmentSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 RandomValue;"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(RandomValue, 1.0f);\n"
        "}\n\0";
    
    CompileShaderProgram(VertexSource, FragmentSource);
}

static void InitializeSimpleText()
{
    int32 Error = FT_Init_FreeType(&FontLibrary);
    if(Error == 0)
    {
        // NOTE(Naor): If we already loaded the font to memory,
        // we can use FT_New_Memory_Face to create a new face object.
        
        // NOTE(Naor): To know how many faces a given font file contains,
        // set face_index to -1, then check the value of face->num_faces,
        // which indicates how many faces are embedded in the font file.
        Error = FT_New_Face(FontLibrary, "C:/Windows/Fonts/Arial.ttf", 0, &FontFace);
        if(Error == 0)
        {
            // NOTE(Naor): Or use FT_Set_Char_Size
            Error = FT_Set_Pixel_Sizes(
                FontFace,   /* handle to face object */
                0,      /* pixel_width           */
                48 );   /* pixel_height          */
            
            
            // TODO(Naor): Make this into a single bitmap instead of every single
            // char into a texture itself.
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            for(uint32 Char = 0;
                Char < 128;
                ++Char)
            {
                if(FT_Load_Char(FontFace, Char, FT_LOAD_RENDER) != 0)
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
                    FontFace->glyph->bitmap.width,
                    FontFace->glyph->bitmap.rows,
                    0, 
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    FontFace->glyph->bitmap.buffer
                    );
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                
                character Character;
                Character.TextureID = Texture;
                Character.Size = {(int32)FontFace->glyph->bitmap.width, (int32)FontFace->glyph->bitmap.rows};
                Character.Bearing = {FontFace->glyph->bitmap_left, FontFace->glyph->bitmap_top};
                Character.Advance = FontFace->glyph->advance.x;
                
                Characters.insert(std::pair<char, character>((char)Char, Character));
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
                "Color = vec4(0.5, 0.6, 0.3, 1.0) * Sampled;\n"
                "}  \n";
            
            CompileShaderProgram(VertexSource, FragmentSource);
            
            glUseProgram(ShaderProgram);
            // TODO(Naor): Change the resolution based on the window size(?)
            m4 Projection = OrthographicProjection(0.0f, 800.0f, 0.0f, 600.0f);
            glUniformMatrix4fv(glGetUniformLocation(ShaderProgram, "Projection"), 1, GL_FALSE, Projection.E[0]);
            
            // NOTE(Naor): We want to enable blending because we have 
            // to draw quads for the text.
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            glGenVertexArrays(1, &Vao);
            glGenBuffers(1, &Vbo);
            glBindVertexArray(Vao);
            glBindBuffer(GL_ARRAY_BUFFER, Vbo);
            // The quad
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glEnableVertexAttribArray(0);
            
        }
        else
        {
            // TODO(Naor): Log error
            // Couldn't load the given font.
        }
    }
    else
    {
        // TODO(Naor): Log error
        // Couldn't initialize freetype.
    }
    
    // We are dont with the font.
    FT_Done_Face(FontFace);
    
    // TODO(Naor): Do we really want to tell FreeType that we are done with it?
    // maybe we would like to load fonts on the fly and reloading it would be a waste.
    // (We will probably wont be loading fonts that often so maybe we should free it)
}

static void Render()
{
    glClearColor(0.4f, 0.2f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
#if 0
    // Triangle rendering
    glBindVertexArray(Vao);
    glUseProgram(ShaderProgram);
    
    float RandomValue[3] = {
        (rand() % 255) / 255.0f,
        (rand() % 255) / 255.0f,
        (rand() % 255) / 255.0f
    };
    
    glUniform3fv(glGetUniformLocation(ShaderProgram, "RandomValue"), 1, RandomValue);
    glDrawArrays(GL_TRIANGLES, 0, 3);
#endif
    
    glUseProgram(ShaderProgram);
    // NOTE(Naor): This might not be neccessary, Texture0 should be
    // active by default.
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(Vao);
    
    float X = 100.0f;
    float Y = 100.0f;
    const char* TextToRender = "Hello world!";
    while(*TextToRender)
    {
        character Char = Characters[*TextToRender];
        
        float XPos = X + Char.Bearing.X; // * Scale
        float YPos = Y - (Char.Size.Y - Char.Bearing.Y); // * Scale
        
        float Width = 1.0f * Char.Size.X; // * Scale
        float Height = 1.0f * Char.Size.Y; // * Scale
        
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
        
        TextToRender++;
    }
}

int main(int argc, char* argv[])
{
    SDL_Window* Window;
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
    Window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        800,                               // width, in pixels
        600,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
        );
    
    
    
    // TODO(Naor): This attribute MUST come before the OpenGL context creation, 
    // the attributes are set and only when we use CreateContext it is applied.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    GLContext = SDL_GL_CreateContext(Window);
    SDL_GL_MakeCurrent(Window, GLContext);
    
    
    if (Window == NULL) {
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
    
    //InitializeSimpleTriangle();
    InitializeSimpleText();
    
    Running = true;
    ShouldRender = true;
    while(Running)
    {
        SDL_Event Event;
        while (SDL_PollEvent(&Event)) 
        {
            ShouldRender = true;
            switch(Event.type)
            {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    SDL_KeyboardEvent& key = Event.key;
                    if(key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    {
                        Running = false;
                    }
                    
                }break;
                
                case SDL_QUIT:
                {
                    Running = false;
                }break;
            }
        }
        
        if(ShouldRender)
        {
            ShouldRender = false;
            
            Render();
            
            SDL_GL_SwapWindow(Window);
        }
    }
    
    return 0;
}