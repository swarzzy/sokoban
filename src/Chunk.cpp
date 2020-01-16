#include "Chunk.h"
namespace soko
{
    inline u32
    GetChunkEntityTableIndex(uv3 tile)
    {
        SOKO_ASSERT(tile.x < CHUNK_DIM);
        SOKO_ASSERT(tile.y < CHUNK_DIM);
        SOKO_ASSERT(tile.z < CHUNK_DIM);
        u32 result = tile.z * CHUNK_DIM * CHUNK_DIM + tile.y * CHUNK_DIM + tile.x;
        SOKO_ASSERT(result < CHUNK_TILE_COUNT);
        return result;
    }

    inline Entity*
    YieldEntityFromTile(Chunk* chunk, uv3 tileInChunk, EntityMapIterator* it)
    {
        Entity* result = 0;

        if (!it->block && it->index == 0)
        {
            u32 index = GetChunkEntityTableIndex(tileInChunk);
            ChunkEntityBlock* head = chunk->entityTable[index];
            it->block = head;
            it->index = 0;
        }
        if (it->block)
        {
            SOKO_ASSERT(!(it->index > it->block->at));

            if (it->index == it->block->at)
            {
                it->block = it->block->next;
                if (it->block)
                {
                    it->index = 0;
                }
                else
                {
                    it->index = 0xffffffff;
                }
            }

            if (it->block)
            {
                result = it->block->entities[it->index];
                it->index++;
            }
        }

        return result;
    }

    inline const
    Entity* YieldEntityFromTile(const Chunk* chunk, uv3 tileInChunk, EntityMapIterator* at)
    {
        return YieldEntityFromTile((Chunk*)chunk, tileInChunk, at);
    }

    inline Entity*
    YieldEntityFromTile(Level* level, iv3 tile, EntityMapIterator* at)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        Chunk* chunk = GetChunk(level, c);
        return YieldEntityFromTile(chunk, t, at);
    }

    inline const Entity*
    YieldEntityFromTile(const Level* level, iv3 tile, EntityMapIterator* at)
    {
        return YieldEntityFromTile((Level*)level, tile, at);
    }

    // TODO: These two functions downhere are the same.
    // Maybe make a generic function to _get stuff from freelist or allocate_
    // ____GENERIC_FREELISTS____
    inline ChunkEntityBlock*
    GetChunkEntityBlock(Level* level, AB::MemoryArena* arena)
    {
        ChunkEntityBlock* block = 0;
        if (level->freeChunkEntityBlocks)
        {
            auto next = level->freeChunkEntityBlocks->next;
            block = level->freeChunkEntityBlocks;
            level->freeChunkEntityBlocks = next;
            ZERO_ARRAY(Entity*, ArrayCount(block->entities), block->entities);
        }
        else
        {
            block = PUSH_STRUCT(arena, ChunkEntityBlock);
        }

        if (block)
        {
            level->chunkEntityBlockCount++;
            block->at = 0;
            block->next = 0;
        }

        return block;
    }

    internal bool
    RegisterEntityInTileInternal(Chunk* chunk, Entity* entity, uv3 tileInChunk)
    {
        Level* level = chunk->level;
        AB::MemoryArena* arena = level->sessionArena;
        bool result = false;

        u32 index = GetChunkEntityTableIndex(tileInChunk);
        ChunkEntityBlock* block = chunk->entityTable[index];

        if (!block || block->at == ArrayCount(block->entities))
        {
            ChunkEntityBlock* newBlock = GetChunkEntityBlock(level, arena);
            if (newBlock)
            {
                newBlock->next = block;
                newBlock->at = 0;
                chunk->entityTable[index] = newBlock;
                block = newBlock;
            }
            else
            {
                block = 0;
            }
        }

        if (block)
        {
            SOKO_ASSERT(block->at < ArrayCount(block->entities));
            block->entities[block->at] = entity;
            block->at++;
            result = true;
        }
        return result;
    }

    EntityArrayBlock* GetChunkEntityArrayBlock(Chunk* chunk)
    {
        EntityArrayBlock* result = 0;
        Level* level = chunk->level;
        if (level->chunkEntityArrayBlockFreeList)
        {
            result = level->chunkEntityArrayBlockFreeList;
            level->chunkEntityArrayBlockFreeList = result->next;
            level->chunkEntityArrayBlockCount--;
            result->at = 0;
            result->next = 0;
        }
        if (!result)
        {
            // NOTE: Assumed mem cleared to zero
            result = PUSH_STRUCT(level->sessionArena, EntityArrayBlock);
            SOKO_ASSERT(result);
        }
        return result;
    }

    void PutEntityInChunkArray(Chunk* chunk, Entity* entity)
    {
        auto block = chunk->entityArray;
        if (!block)
        {
            block = GetChunkEntityArrayBlock(chunk);
            chunk->entityArray = block;
        }
        else if (block->at >= ArrayCount(block->entities))
        {
            auto newBlock = GetChunkEntityArrayBlock(chunk);
            newBlock->next = block;
            chunk->entityArray = newBlock;
            block = newBlock;
        }

        SOKO_ASSERT(block->at < ArrayCount(block->entities));
        u32 index = block->at;
        block->entities[index] = entity;
        block->at++;

        SOKO_ASSERT(!entity->chunkEntityArrayIndex.block);
        SOKO_ASSERT(!entity->chunkEntityArrayIndex.index);

        entity->chunkEntityArrayIndex.block = block;
        entity->chunkEntityArrayIndex.index = index;
    }

    void RemoveEntityFromChunkArray(Chunk* chunk, Entity* entity)
    {
        auto block = entity->chunkEntityArrayIndex.block;
        auto index = entity->chunkEntityArrayIndex.index;

        SOKO_ASSERT(block->entities[index] == entity);
        SOKO_ASSERT(chunk->entityArray->at);

        // TODO: Maybe not zero this for speed purposes?
        entity->chunkEntityArrayIndex.block = 0;
        entity->chunkEntityArrayIndex.index = 0;

        auto lastEntity = chunk->entityArray->entities[chunk->entityArray->at - 1];
        if (lastEntity != entity)
        {
            block->entities[index] = lastEntity;
            lastEntity->chunkEntityArrayIndex.block = block;
            lastEntity->chunkEntityArrayIndex.index = index;
            chunk->entityArray->at--;
        }
        else
        {
            block->at--;
        }

        if (chunk->entityArray->at == 0)
        {
            auto freeBlock = chunk->entityArray;
            chunk->entityArray = freeBlock->next;

            auto level = chunk->level;

            freeBlock->next = level->chunkEntityArrayBlockFreeList;
            level->chunkEntityArrayBlockFreeList = freeBlock;
            level->chunkEntityArrayBlockCount++;
        }
    }

    // TODO: Check that an entity was registered in the tile only once
    internal bool
    UnregisterEntityInTileInternal(Chunk* chunk, uv3 tileInChunk, Entity* e)
    {
        bool result = false;
        u32 index = GetChunkEntityTableIndex(tileInChunk);

        ChunkEntityBlock* head = chunk->entityTable[index];
        ChunkEntityBlock* block = head;
        if (block)
        {
            while (block)
            {
                for (u32 i = 0; i < block->at; i++)
                {
                    Entity* be = block->entities[i];
                    if (be == e)
                    {
                        bool result = true;
                        if (!((block == head) && (i == head->at - 1)))
                        {
                            block->entities[i] = head->entities[head->at - 1];
                            head->at--;
                        }
                        else
                        {
                            block->at--;
                        }
                        if (!head->at)
                        {
                            chunk->entityTable[index] = head->next;
                            head->next = chunk->level->freeChunkEntityBlocks;
                            chunk->level->freeChunkEntityBlocks = head;
                            chunk->level->chunkEntityBlockCount--;
                        }
                        goto end;
                    }
                }
                block = block->next;
            }
        }
    end:;
        return result;
    }

    inline void
    RegisterEntityInTile(Level* level, Entity* e)
    {
        iv3 c = GetChunkCoord(e->pos);
        uv3 t = GetTileCoordInChunk(e->pos);
        Chunk* chunk = GetChunk(level, c);
        RegisterEntityInTileInternal(chunk, e, t);
        PutEntityInChunkArray(chunk, e);
    }

    inline void
    UnregisterEntityInTile(Level* level, Entity* e)
    {
        iv3 c = GetChunkCoord(e->pos);
        uv3 t = GetTileCoordInChunk(e->pos);
        Chunk* chunk = GetChunk(level, c);
        UnregisterEntityInTileInternal(chunk, t, e);
        RemoveEntityFromChunkArray(chunk, e);
    }

    inline void
    SetTileInChunkInternal(Chunk* chunk, u32 x, u32 y, u32 z, TileValue value);

    inline const Tile*
    GetTilePointerInChunkInternal(const Chunk* chunk, u32 x, u32 y, u32 z)
    {
        // TODO: Actial check?
        SOKO_ASSERT(x < CHUNK_DIM);
        SOKO_ASSERT(y < CHUNK_DIM);
        SOKO_ASSERT(z < CHUNK_DIM);

        const Tile* tile = 0;
        tile = chunk->tiles + (z * CHUNK_DIM * CHUNK_DIM + y * CHUNK_DIM + x);

        return tile;
    }

    inline const Tile*
    GetTileInChunk(const Chunk* chunk, u32 x, u32 y, u32 z)
    {
        const Tile* result = 0;
        if (chunk &&
            x < CHUNK_DIM &&
            y < CHUNK_DIM &&
            z < CHUNK_DIM)
        {
            const Tile* tile = GetTilePointerInChunkInternal(chunk, x, y, z);
            if (tile)
            {
                result = tile;
            }
        }
        return result;
    }

    inline const Tile*
    GetTileInChunk(const Chunk* chunk, uv3 tileInChunk)
    {
        return GetTileInChunk(chunk, tileInChunk.x, tileInChunk.y, tileInChunk.z);
    }

    inline const Tile*
    GetTilePointerInternal(Level* level, i32 x, i32 y, i32 z)
    {
        const Tile* result = 0;

#if 0
        if ((x >= LEVEL_MIN_DIM && x <= LEVEL_MAX_DIM) &&
            (y >= LEVEL_MIN_DIM && y <= LEVEL_MAX_DIM) &&
            (z >= LEVEL_MIN_DIM && z <= LEVEL_MAX_DIM))
#endif

            iv3 c = GetChunkCoord(x, y, z);
        uv3 t = GetTileCoordInChunk(x, y, z);

        Chunk* chunk = GetChunk(level, c.x, c.y, c.z);

        if (chunk)
        {
            result = GetTilePointerInChunkInternal(chunk, t.x, t.y, t.z);
        }
        return result;
    }

    inline const Tile*
    GetTile(Level* level, i32 x, i32 y, i32 z)
    {
        const Tile* result = 0;
        // TODO: @Speed: Stop returninig pointers and dereferencing them here
        // This code needs cleanup
        const Tile* tile = GetTilePointerInternal(level, x, y, z);
        if (tile)
        {
            result = tile;
        }
        return result;
    }

    inline const Tile*
    GetTile(Level* level, iv3 coord)
    {
        const Tile* result = GetTile(level, coord.x, coord.y, coord.z);
        return result;
    }

    inline bool
    TileIsTerrain(Tile tile)
    {
        bool result = (tile.value != TileValue_Empty);
        return result;
    }

    inline bool
    TileIsTerrain(const Tile* t)
    {
        bool result = !t || TileIsTerrain(*t);
        return result;
    }

    inline bool
    TileIsTerrain(const Chunk* chunk, uv3 tileInChunk)
    {
        bool result = true;
        const Tile* tile = GetTileInChunk(chunk, tileInChunk);
        result = TileIsTerrain(tile);
        return result;
    }

    inline bool
    TileIsTerrain(const Level* level, iv3 tile)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        const Chunk* chunk = GetChunk(level, c);
        bool result = TileIsTerrain(chunk, t);
        return result;
    }

    enum TileCheckFlag : u32
    {
        TileCheck_Terrain = (1 << 0),
        TileCheck_Entities = (1 << 1)
    };

    inline bool
    CheckTile(const Chunk* chunk, uv3 tileInChunk, u32 flags = TileCheck_Terrain | TileCheck_Entities, const Entity* forEntity = 0)
    {
        bool result = true;
        const Tile* tile = GetTileInChunk(chunk, tileInChunk);

        bool occupiedByTerrain = TileIsTerrain(tile);
        bool occupiedByEntities = false;

        if (flags & TileCheck_Entities)
        {
            EntityMapIterator it = {};
            while (true)
            {
                const Entity* e = YieldEntityFromTile(chunk, tileInChunk, &it);
                if (!e) break;
                if ((e != forEntity) && EntityCollides(chunk->level, e))
                {
                    occupiedByEntities = true;
                    break;
                }
            }
        }

        if (flags == TileCheck_Terrain)
        {
            result = !occupiedByTerrain;
        }
        else if (flags == TileCheck_Entities)
        {
            result = !occupiedByEntities;
        }
        else if (flags == (TileCheck_Entities | TileCheck_Terrain))
        {
            result = !occupiedByTerrain && !occupiedByEntities;
        }

        return result;
    }

    inline bool
    CheckTile(const Level* level, iv3 tile, u32 flags = TileCheck_Terrain | TileCheck_Entities, const Entity* forEntity = 0)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        const Chunk* chunk = GetChunk(level, c);
        return CheckTile(chunk, t, flags, forEntity);
    }

    inline bool
    CanMove(const Level* level, iv3 tile, const Entity* e)
    {
        bool result = true;
        if (e)
        {
            iv3 testTile = tile;
            iv3 groundTile = testTile + DirToUnitOffset(Direction_Down);
            bool tileFree = CheckTile(level, testTile, TileCheck_Terrain | TileCheck_Entities, e);
            bool groundFree = TileIsTerrain(level, groundTile);
            result = result && tileFree && groundFree;
        }
        else
        {
            iv3 testTile = tile;
            iv3 groundTile = testTile + DirToUnitOffset(Direction_Down);
            bool tileFree = CheckTile(level, testTile, TileCheck_Terrain | TileCheck_Entities);
            bool groundFree = TileIsTerrain(level, groundTile);
            result = tileFree && groundFree;
        }
        return result;
    }

    inline bool
    TerrainIsSuitableForMovement(Level* level,  iv3 tile)
    {
        iv3 groundTile = tile + DirToUnitOffset(Direction_Down);
        bool result = !TileIsTerrain(level, tile) && TileIsTerrain(level, groundTile);
        return result;
    }

    inline void
    SetTileInChunkInternal(Chunk* chunk, u32 x, u32 y, u32 z, TileValue value)
    {
        SOKO_ASSERT(x < CHUNK_DIM);
        SOKO_ASSERT(y < CHUNK_DIM);
        SOKO_ASSERT(z < CHUNK_DIM);

        Tile* tile = chunk->tiles + (z * CHUNK_DIM * CHUNK_DIM + y * CHUNK_DIM + x);
        if (tile->value != value)
        {
            if (!TileIsTerrain(*tile) && TileIsTerrain(Tile{value}))
            {
                chunk->filledTileCount++;
            }
            else if (TileIsTerrain(*tile) && !TileIsTerrain(Tile{value}))
            {
                SOKO_ASSERT(chunk->filledTileCount > 0);
                chunk->filledTileCount--;
            }

            tile->value = value;
            chunk->dirty = true;
        }
    }

    inline void
    SetTileInChunk(Chunk* chunk, u32 x, u32 y, u32 z, TileValue value)
    {
        if (x < CHUNK_DIM &&
            y < CHUNK_DIM &&
            z < CHUNK_DIM)
        {
            SetTileInChunkInternal(chunk, x, y, z, value);
        }
    }

    inline void
    SetTileInChunk(Chunk* chunk, uv3 tile, TileValue value)
    {
        SetTileInChunkInternal(chunk, tile.x, tile.y, tile.z, value);
    }

    inline void
    SetTile(Level* level, i32 x, i32 y, i32 z, TileValue value)
    {
        // TODO: Real check instead of asserts
        SOKO_ASSERT(x >= LEVEL_MIN_DIM && x <= LEVEL_MAX_DIM);
        SOKO_ASSERT(y >= LEVEL_MIN_DIM && y <= LEVEL_MAX_DIM);
        SOKO_ASSERT(z >= LEVEL_MIN_DIM && z <= LEVEL_MAX_DIM);

        iv3 c = GetChunkCoord(x, y, z);
        uv3 t = GetTileCoordInChunk(x, y, z);

        Chunk* chunk = GetChunk(level, c.x, c.y, c.z);

        if (chunk)
        {
            SetTileInChunkInternal(chunk, t.x, t.y, t.z, value);
        }
    }

    inline void
    SetTile(Level* level, iv3 tile, TileValue value)
    {
        SetTile(level, tile.x, tile.y, tile.z, value);
    }
}
