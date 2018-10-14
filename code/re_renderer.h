#pragma once

// TODO(Naor): Add other primitive types,
// add font renderer in here.
namespace renderer
{
    static uint32 QuadVao;
    static uint32 QuadVbo;
    static uint32 QuadShader;
    
    inline void UpdateProjection(const m4& Projection)
    {
        glUseProgram(QuadShader);
        glUniformMatrix4fv(glGetUniformLocation(QuadShader, "Projection"), 1, GL_FALSE, Projection.E[0]);
    }
    
    static void Initialize(const m4& Projection)
    {
        const char* VertexSource = 
            "#version 330 core\n"
            "layout (location = 0) in vec2 Position;\n"
            "uniform mat4 Projection;\n"
            "uniform mat4 Model;\n"
            "void main()\n"
            "{\n"
            "gl_Position = Projection * Model * vec4(Position, 0.0, 1.0);\n"
            "}\n";
        const char* FragmentSource = 
            "#version 330 core\n"
            "uniform vec4 InColor;\n"
            "out vec4 OutColor;\n"
            "void main()\n"
            "{\n"
            "OutColor = InColor;\n"
            "}\n";
        
        QuadShader = CompileShaderProgram(VertexSource, FragmentSource);
        
        UpdateProjection(Projection);
        
        glGenVertexArrays(1, &QuadVao);
        glGenBuffers(1, &QuadVbo);
        
        // TODO(Naor): What values we need to put here?
        // the values are in the range of the orthograhic projection, meaning
        // 0-800 and 0-600, use the max height of the font for now and
        // find a max width of the font and make a quad out of it.
        static float Vertices[] = 
        {
            
            0.0f, 0.0f, 
            1.0f, 0.0f,
            0.0f, 1.0f,
            
            0.0f, 1.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
        };
        
        glBindVertexArray(QuadVao);
        glBindBuffer(GL_ARRAY_BUFFER, QuadVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
        glEnableVertexAttribArray(0);
    }
    
    static void DrawQuad(const vec2& Pos, const vec2& Scale, const vec4& Color)
    {
        glUseProgram(QuadShader);
        
        m4 Model = math::Mat4Identity();
        math::Translate(Model, Pos);
        math::Scale(Model, Scale);
        
        // TODO(Naor): Store the location instead of querying it every
        // draw..
        int32 ModelLocation = glGetUniformLocation(QuadShader, "Model");
        int32 InColorLocation = glGetUniformLocation(QuadShader, "InColor");
        
        glUniformMatrix4fv(ModelLocation, 1, false, Model.E[0]);
        glUniform4fv(InColorLocation, 1, &Color.E[0]);
        
        glBindVertexArray(QuadVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}