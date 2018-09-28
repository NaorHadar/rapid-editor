#include <gl3w.c>
#include <SDL.h>
#include <iostream>
#include <cstdint>

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using bool32 = int32_t;

static bool Running;
static bool ShouldRender;

static uint32 Vao;
static uint32 Vbo;
static uint32 ShaderProgram;

void InitializeSimpleTriangle()
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
    
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
}

void Render()
{
    glClearColor(0.4f, 0.2f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindVertexArray(Vao);
    glUseProgram(ShaderProgram);
    
    float RandomValue[3] = {
        (rand() % 255) / 255.0f,
        (rand() % 255) / 255.0f,
        (rand() % 255) / 255.0f
    };
    
    glUniform3fv(glGetUniformLocation(ShaderProgram, "RandomValue"), 1, RandomValue);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
}

// TODO(Naor): Fix/Change every f/printf to make it really output!
int main(int argc, char* argv[])
{
    SDL_Window* Window;
    SDL_GLContext GLContext;
    
    // TODO(Naor): Validate this with if
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Something went wrong while initializing SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    // Create an application window with the following settings:
    Window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        640,                               // width, in pixels
        480,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
        );
    
    GLContext = SDL_GL_CreateContext(Window);
    SDL_GL_MakeCurrent(Window, GLContext);
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    if (Window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }
    
    
    if (gl3wInit()) {
        fprintf(stderr, "failed to initialize OpenGL\n");
        return 1;
    }
    if (!gl3wIsSupported(3, 3)) {
        fprintf(stderr, "OpenGL 3.3 not supported\n");
        return 1;
    }
    
    InitializeSimpleTriangle();
    
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