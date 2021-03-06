#pragma once
#include "Platform.h"
#include "FreeList.h"

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

    struct ChunkEntityBlock : public LinkedBlock<ChunkEntityBlock>
    {
        u32 at;
        Entity* entities[8];
    };

    struct EntityArrayBlock : public LinkedBlock<EntityArrayBlock>
    {
        u32 at;
        Entity* entities[16];
    };

    SOKO_STATIC_ASSERT(IsPowerOfTwo(ArrayCount(DeclMember(EntityArrayBlock, entities))));

    struct ChunkEntityArrayIter
    {
        EntityArrayBlock* block;
        u32 at;
        Entity* ptr;

        inline ChunkEntityArrayIter& operator++()
            {
                if (this->at == (this->block->at - 1))
                {
                    this->at = 0;
                    this->block = this->block->next;
                }
                else
                {
                    this->at++;
                }

                if (this->block)
                {
                    this->ptr = this->block->entities[this->at];
                }

                return *this;
            }

        bool operator!=(ChunkEntityArrayIter const& other) const
            {
                // TODO: Not a valid comparsion. Used only for foreach loop
                return this->block != other.block;
            }

        Entity& operator*()
            {
                return *ptr;
            }
    };

    inline ChunkEntityArrayIter begin(EntityArrayBlock* array)
    {
        ChunkEntityArrayIter iter = {};
        if (array)
        {
            iter.block = array;
            iter.ptr = array->entities[0];
        }
        return iter;
    }

    inline ChunkEntityArrayIter end(EntityArrayBlock* array)
    {
        ChunkEntityArrayIter iter = {};
        return iter;
    }

    struct ChunkEntityArrayIndex
    {
        EntityArrayBlock* block;
        u32 index;
    };

    SOKO_STATIC_ASSERT(IsPowerOfTwo(ArrayCount(DeclMember(EntityArrayBlock, entities))));

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
