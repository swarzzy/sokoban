#pragma once
#include "Platform.h"
#include <cstring>

#define KILOBYTES(kb) ((kb) * 1024llu)
#define MEGABYTES(mb) ((mb) * 1024llu * 1024llu)

#define COPY_ARRAY(type, elem_count, dest, src) memcpy(dest, src, sizeof(type) * elem_count)
#define COPY_SCALAR(type, dest, src) memcpy(dest, src, sizeof(type))
#define COPY_BYTES(numBytes, dest, src) memcpy(dest, src, numBytes)
#define SET_ARRAY(type, elem_count, dest, val) memset(dest, val, sizeof(type) * elem_count)
#define ZERO_STRUCT(type, dest) memset(dest, 0, sizeof(type))
#define ZERO_ARRAY(type, count, dest) memset(dest, 0, sizeof(type) * count)
#define ZERO_FROM_MEMBER(type, member, dest) memset(((byte*)dest + offsetof(type, member)), 0, sizeof(type) - offsetof(type, member))

#define PUSH_STRUCT(arena, type) (type*)PushSize(arena, sizeof(type), alignof(type))
#define PUSH_ARRAY(arena, type, count) (type*)PushSize(arena, sizeof(type) * count, alignof(type))
#define PUSH_SIZE(arena, size) PushSize(arena, size, 0)
namespace AB
{

    // Using 16 byte aligment as default as malloc does (according to specs)
    // There are crashes when using 8 byte aligment and optimizations are enabled
    static const u64 DEFAULT_ALIGMENT = 16;//alignof(std::max_align_t);

    struct MemoryArena
    {
        uptr free;
        uptr offset;
        uptr size;
        void* stackMark;
        void* begin;
        // TODO: Set paddind propperly on 32-bit machines
        // So that arena memory aligned at 16 bytes boundary
        uptr _pad;
    };

    inline uptr CalculatePadding(uptr offset, uptr aligment)
    {
        // TODO: Calculate padding properly
        // TODO: @Important: enable asserts here
        //AB_CORE_ASSERT(aligment, "Aligment is zero.");
        // TODO: IMPORTANT: FIXME: Fix the padding it is not correct.
#if 1
        u64 padding = (aligment - offset % aligment) % aligment;
        return padding;
#else
        uptr mul = (offset / aligment) + 1;
        uptr aligned = mul * aligment;
        uptr padding = aligned - offset;
        return padding;
#endif
    }

    inline void BeginTemporaryMemory(MemoryArena* arena)
    {
        arena->stackMark = (void*)((byte*)arena->begin + arena->offset);
    }

    inline void EndTemporaryMemory(MemoryArena* arena)
    {
        // TODO: @Important: enable asserts here
        //AB_CORE_ASSERT(arena->stackMark,
        //             "Calling EndTemporaryMemory without BeginTemporaryMemory");
        uptr markOffset = (uptr)arena->stackMark - (uptr)arena->begin;
        arena->free += arena->offset - markOffset;
        arena->offset = markOffset;
        arena->stackMark = nullptr;
    }

    inline void* PushSize(MemoryArena* arena, uptr size, uptr aligment = 0)
    {
        uptr nextAdress = 0;
        uptr padding = 0;
        uptr useAligment = 0;
        uptr currentOffset = arena->offset;
        uptr currentAddress = (uptr)arena->begin + currentOffset;

        if (aligment == 0)
        {
            useAligment = DEFAULT_ALIGMENT;
        }
        else
        {
            useAligment = aligment;
        }

        if (currentOffset % useAligment != 0)
        {
            padding = CalculatePadding(currentAddress, useAligment);
        }

        if (size + padding <= arena->free)
        {
            // TODO: Why +1 is here?
            arena->offset += size + padding + 1;
            arena->free -= size + padding + 1;
            nextAdress = currentAddress + padding;
            // TODO: Padding!!!!!!
            //AB_CORE_ASSERT(nextAdress % useAligment == 0, "Wrong aligment");
        }

        return (void*)nextAdress;
    }

    inline MemoryArena* AllocateSubArena(MemoryArena* arena, uptr size)
    {
        MemoryArena* result = nullptr;
        uptr chunkSize = size + sizeof(arena);
        void* chunk = PushSize(arena, chunkSize, DEFAULT_ALIGMENT);
        if (chunk)
        {
            MemoryArena header = {};
            header.free = size;
            header.offset = 0;
            header.stackMark = nullptr;
            header.size = size;
            header.begin = (void*)((byte*)chunk + sizeof(MemoryArena));
            COPY_SCALAR(MemoryArena, chunk, &header);
            result = (MemoryArena*)chunk;
        }
        return result;
    }
}
