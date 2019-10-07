#pragma once
#include "Platform.h"

namespace soko
{
    constant i32 CHUNK_BIT_SHIFT = 5;
    constant i32 CHUNK_BIT_MASK = (1 << CHUNK_BIT_SHIFT) - 1;
    constant i32 CHUNK_DIM = 1 << CHUNK_BIT_SHIFT;
    constant i32 CHUNK_TILE_COUNT = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

    constant i32 LEVEL_INVALID_COORD = 0x7fffffff;
    constant i32 LEVEL_MAX_DIM = 128;
    constant i32 LEVEL_MIN_DIM = -127;

    constant u32 LEVEL_FULL_DIM_CHUNKS = (LEVEL_MAX_DIM * 2) / CHUNK_DIM; // 8
    constant i32 LEVEL_MAX_DIM_CHUNKS = LEVEL_FULL_DIM_CHUNKS / 2;
    constant i32 LEVEL_MIN_DIM_CHUNKS = -LEVEL_MAX_DIM_CHUNKS + 1;
    constant u32 LEVEL_ENTITY_TABLE_SIZE = 1024;
    constant f32 LEVEL_TILE_SIZE = 1.0f;
}