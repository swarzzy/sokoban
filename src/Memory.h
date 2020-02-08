#pragma once
#include "Platform.h"
#include <cstring>

// TODO: Temporary solution.
// Resolve asserts mess and use normal asserts
#include <assert.h>
#define ASSERT assert


#define KILOBYTES(kb) ((kb) * 1024llu)
#define MEGABYTES(mb) ((mb) * 1024llu * 1024llu)

#define Memcpy(dest, src, size) memcpy((dest), (src), (size))
#define Memset(dest, val, size) memset((dest), (val), (size))
// TODO: Get rid of this mess
#define COPY_ARRAY(type, elem_count, dest, src) memcpy(dest, src, sizeof(type) * elem_count)
#define COPY_STRUCT(type, dest, src) memcpy(dest, src, sizeof(type))
#define COPY_BYTES(numBytes, dest, src) memcpy(dest, src, numBytes)
#define SET_ARRAY(type, elem_count, dest, val) memset(dest, val, sizeof(type) * elem_count)
#define ZERO_STRUCT(type, dest) memset(dest, 0, sizeof(type))
#define ZERO_ARRAY(type, count, dest) memset(dest, 0, sizeof(type) * count)
#define ZERO_SIZE(size, dest) memset(dest, 0, size)
#define ZERO_FROM_MEMBER(type, member, dest) memset(((byte*)dest + offsetof(type, member)), 0, sizeof(type) - offsetof(type, member))

#define PUSH_STRUCT(arena, type) (type*)PushSize(arena, sizeof(type), alignof(type))
#define PUSH_ARRAY(arena, type, count) (type*)PushSize(arena, sizeof(type) * count, alignof(type))
#define PUSH_SIZE(arena, size) PushSize(arena, size, 0)

namespace AB
{
    constant u64 DEFAULT_ALIGMENT = 16;

    struct alignas(DEFAULT_ALIGMENT) MemoryArena
    {
        void* begin;
        uptr offset;
        uptr size;
        uptr free;
        b32 isTemporary;
        i32 tempCount;
    };

    enum MemoryArenaFlags : u32
    {
        MemoryArenaFlag_None = 0,
        MemoryArenaFlag_ClearTemp = (1 << 0)
    };

    struct TempMemory
    {
        MemoryArena* arena;
        uptr offset;
    };

    inline TempMemory
    BeginTemporaryMemory(MemoryArena* arena, MemoryArenaFlags flags = MemoryArenaFlag_None)
    {
        ASSERT(arena->isTemporary);

        arena->tempCount++;
        return TempMemory { arena, arena->offset };
    }

    inline void EndTemporaryMemory(TempMemory* frame)
    {
        if (frame->arena)
        {
            auto arena = frame->arena;
            if (frame->offset < arena->offset)
            {
                uptr markOffset = frame->offset;
                arena->free += arena->offset - frame->offset;
                arena->offset = frame->offset;
            }
            arena->tempCount--;
            ASSERT(arena->tempCount >= 0);
            *frame = {};
        }
    }

    struct ScopedTempMemory
    {
        TempMemory frame;
        static inline ScopedTempMemory Make(MemoryArena* arena, MemoryArenaFlags flags = MemoryArenaFlag_None) { return ScopedTempMemory { BeginTemporaryMemory(arena, flags)}; }
        // TODO: Remove
        static inline ScopedTempMemory make(TempMemory frame) { return ScopedTempMemory {frame.arena, frame.offset}; }
        ~ScopedTempMemory() { EndTemporaryMemory(&this->frame); }
    };

    inline bool CheckTempArena(const MemoryArena* arena)
    {
        ASSERT(arena->isTemporary);
        bool result = (arena->tempCount == 0) && (arena->offset == 0);
        return result;
    }

    inline uptr CalculatePadding(uptr offset, uptr alignment)
    {
        uptr padding = 0;
        auto alignmentMask = alignment - 1;
        if (offset & alignmentMask)
        {
            padding = alignment - (offset & alignmentMask);
        }
        return padding;
    }

    inline void* PushSizeInternal(MemoryArena* arena, uptr size, uptr aligment)
    {
        uptr padding = 0;
        uptr useAligment = 0;
        uptr currentAddress = (uptr)arena->begin + arena->offset;

        if (aligment == 0)
        {
            useAligment = DEFAULT_ALIGMENT;
        }
        else
        {
            useAligment = aligment;
        }

        if (arena->offset % useAligment != 0)
        {
            padding = CalculatePadding(currentAddress, useAligment);
        }

        // TODO: Grow!
        ASSERT(size + padding <= arena->free);
        uptr nextAdress = (uptr)((byte*)arena->begin + arena->offset + padding);
        ASSERT(nextAdress % useAligment == 0);
        arena->offset += size + padding;
        arena->free -= size + padding;

        return (void*)nextAdress;
    }

    inline void* PushSize(MemoryArena* arena, uptr size, u32 flags = MemoryArenaFlag_ClearTemp, uptr aligment = 0)
    {
        void* mem = PushSizeInternal(arena, size, aligment);
        if (arena->isTemporary && (flags & MemoryArenaFlag_ClearTemp))
        {
            Memset(mem, 0, size);
        }
        return mem;
    }

    inline MemoryArena* AllocateSubArena(MemoryArena* arena, uptr size, bool isTemporary)
    {
        MemoryArena* result = nullptr;
        uptr chunkSize = size + sizeof(arena);
        void* chunk = PushSize(arena, chunkSize, DEFAULT_ALIGMENT);
        if (chunk)
        {
            MemoryArena header = {};
            header.free = size;
            header.size = size;
            header.isTemporary = isTemporary;
            header.begin = (void*)((byte*)chunk + sizeof(MemoryArena));
            COPY_STRUCT(MemoryArena, chunk, &header);
            result = (MemoryArena*)chunk;
        }
        return result;
    }
}

namespace soko
{
    typedef AB::MemoryArena MemoryArena;
}
