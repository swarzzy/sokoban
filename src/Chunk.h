#pragma once
#include "Platform.h"

namespace soko
{
    struct Entity;

    enum [reflect seq_enum] TileValue : u8
    {
        TileValue_Empty = 0,
        TileValue_Wall,
        TileValue_Stone,
        TileValue_Grass
    };

    struct ChunkPos
    {
        iv3 chunk;
        uv3 tile;
    };

    struct Tile
    {
        TileValue value;
    };

    struct ChunkEntityBlock
    {
        u32 at;
        Entity* entities[8];
        ChunkEntityBlock* next;
    };

    struct EntityArrayBlock
    {
        u32 at;
        // NOTE: Size must be power of two
        Entity* entities[16];
        EntityArrayBlock* next;
    };

    struct EntityMapIterator
    {
        const ChunkEntityBlock* block;
        u32 index;
    };

    struct Chunk
    {
        Level* level;
        b32 dirty;
        iv3 coord;
        u32 filledTileCount;
        Tile tiles[CHUNK_TILE_COUNT];
        LoadedChunkMesh loadedMesh;
        ChunkMesh mesh;
        ChunkEntityBlock* entityTable[CHUNK_TILE_COUNT];

        Chunk* nextInHash;
        // TODO: Acelleration structure for traversing entities
        // in chunk sequentially
        EntityArrayBlock* entityArray;
    };

    inline bool CanMove(const Level* level, iv3 tile, const Entity* e);
    inline Chunk* AddChunk(Level* level, i32 x, i32 y, i32 z);
    inline Chunk* AddChunk(Level* level, iv3 coord);
    inline void SetTileInChunk(Chunk* chunk, u32 x, u32 y, u32 z, TileValue value);
}
