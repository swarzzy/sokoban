#pragma once
#include "Platform.h"
// TODO: Intrinsics file
#include "hypermath.h"

namespace soko
{
    constant char* DEFAULT_LEVEL_NAME = "pbrTest04.aab";
    constant wchar_t* DEFAULT_LEVEL_NAME_W = L"pbrTest04.aab";


    constant i32 CHUNK_BIT_SHIFT = 5;
    constant i32 CHUNK_BIT_MASK = (1 << CHUNK_BIT_SHIFT) - 1;
    constant i32 CHUNK_DIM = 1 << CHUNK_BIT_SHIFT;
    constant i32 CHUNK_TILE_COUNT = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

    constant i32 LEVEL_INVALID_COORD = 0x7fffffff;
    // TODO: Is That correct
    constant i32 LEVEL_MAX_DIM = 160;
    constant i32 LEVEL_MIN_DIM = -96;

    constant u32 LEVEL_FULL_DIM_CHUNKS = (LEVEL_MAX_DIM + Abs(LEVEL_MIN_DIM)) / CHUNK_DIM; // 8
    constant i32 LEVEL_MAX_DIM_CHUNKS = LEVEL_FULL_DIM_CHUNKS / 2;
    constant i32 LEVEL_MIN_DIM_CHUNKS = -LEVEL_MAX_DIM_CHUNKS + 1;
    constant u32 LEVEL_ENTITY_TABLE_SIZE = 1024;
    constant f32 LEVEL_TILE_SIZE = 1.0f;
    constant f32 LEVEL_TILE_RADIUS = LEVEL_TILE_SIZE / 2.0f;
    constant u32 LEVEL_CHUNK_TABLE_SIZE = NextPowerOfTwo(LEVEL_FULL_DIM_CHUNKS * LEVEL_FULL_DIM_CHUNKS * LEVEL_FULL_DIM_CHUNKS);

    SOKO_STATIC_ASSERT(IsPowerOfTwo(LEVEL_CHUNK_TABLE_SIZE));
}
