#pragma once

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

