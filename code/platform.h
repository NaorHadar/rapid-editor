#pragma once

#define PLATFORM_ALLOCATE_MEMORY(name) void* name(size_t Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

// TODO(Naor): Search where, when and how casey use this!
#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void* Block)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

struct platform_api
{
    platform_allocate_memory* AllocateMemory;
    platform_deallocate_memory* DeallocateMemory;
};
extern platform_api Platform;