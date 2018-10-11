#pragma once

// NOTE(Naor): We call this file rapid_math.h only because
// it creates problems calling it math.h, probably because something
// is trying to include "math.h" and it includes this file..

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
