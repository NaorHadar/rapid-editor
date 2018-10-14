#pragma once

// NOTE(Naor): We call this file rapid_math.h only because
// it creates problems calling it math.h, probably because something
// is trying to include "math.h" and it includes this file..


// TODO(Naor): Maybe later on, we will create class to some of the types in here (m4, vec2, etc..)

namespace math
{
    // NOTE(Naor): Thanks to GLM.
    inline m4 OrthographicProjection(float Left, float Right, float Bottom, float Top)
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
    
    inline m4 Mat4Identity()
    {
        m4 Result = {};
        
        Result.E[0][0] = 1.0f;
        Result.E[1][1] = 1.0f;
        Result.E[2][2] = 1.0f;
        Result.E[3][3] = 1.0f;
        
        return Result;
    }
    
    inline void Translate(m4& Mat, const vec2& Pos)
    {
        Mat.E[3][0] += Pos.X;
        Mat.E[3][1] += Pos.Y;
    }
    
    inline void Scale(m4& Mat, const vec2& Scale)
    {
        Mat.E[0][0] = Scale.X;
        Mat.E[1][1] = Scale.Y;
    }
}