#pragma once

static uint32 CompileShaderProgram(const char* VertexSource, const char* FragmentSource)
{
    uint32 VertexShader = glCreateShader(GL_VERTEX_SHADER);
    uint32 FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(VertexShader, 1, &VertexSource, NULL);
    glCompileShader(VertexShader);
    glShaderSource(FragmentShader, 1, &FragmentSource, NULL);
    glCompileShader(FragmentShader);
    
    uint32 Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    glLinkProgram(Program);
    
    int32 Success;
    glGetProgramiv(Program, GL_LINK_STATUS, &Success);
    if(!Success) {
        char Buffer[256];
        glGetProgramInfoLog(Program, 512, NULL, Buffer);
        OutputDebugString(Buffer);
    }
    
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    
    return Program;
}
