#include "Level.h"
#include "Memory.h"
#include "Entity.h"

namespace soko
{
    inline iv3
    DirToUnitOffset(Direction dir)
    {
        iv3 result = {};
        switch (dir)
        {
        case Direction_North: { result = IV3(0, 1, 0); } break;
        case Direction_South: { result = IV3(0, -1, 0); } break;
        case Direction_West:  { result = IV3(-1, 0, 0); } break;
        case Direction_East:  { result = IV3(1, 0, 0); } break;
        case Direction_Up:    { result = IV3(0, 0, 1); } break;
        case Direction_Down:  { result = IV3(0, 0, -1); } break;
            INVALID_DEFAULT_CASE;
        }
        return result;
    }
    inline v3
    WorldToRH(v3 v)
    {
        v3 result;
        result.x = v.x;
        result.y = v.z;
        result.z = -v.y;
        return result;
    }

    inline v3
    RHToWorld(v3 v)
    {
        v3 result;
        result.x = v.x;
        result.y = -v.z;
        result.z = v.y;
        return result;
    }

    inline iv3
    GetChunkCoord(i32 x, i32 y, i32 z)
    {
        iv3 result;
        result.x = x >> CHUNK_BIT_SHIFT;
        result.y = y >> CHUNK_BIT_SHIFT;
        result.z = z >> CHUNK_BIT_SHIFT;
        return result;
    }

    inline iv3
    GetChunkCoord(iv3 tile)
    {
        return GetChunkCoord(tile.x, tile.y, tile.z);
    }

    inline uv3
    GetTileCoordInChunk(i32 x, i32 y, i32 z)
    {
        uv3 result;
        result.x = x & CHUNK_BIT_MASK;
        result.y = y & CHUNK_BIT_MASK;
        result.z = z & CHUNK_BIT_MASK;
        return result;
    }

    inline uv3
    GetTileCoordInChunk(iv3 tile)
    {
        return GetTileCoordInChunk(tile.x, tile.y, tile.z);
    }

    inline ChunkPos
    GetChunkPos(iv3 tile)
    {
        ChunkPos result;
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        result = {c, t};
        return result;
    }

    inline iv3
    WorldTileFromChunkTile(iv3 chunk, uv3 chunkTile)
    {
        // TODO: Fast implementation
        iv3 result = chunk * CHUNK_DIM;
        result.x += (i32)chunkTile.x;
        result.y += (i32)chunkTile.y;
        result.z += (i32)chunkTile.z;
        return result;
    }

    internal Level*
    CreateLevel(AB::MemoryArena* arena)
    {
        Level* result = 0;
        Level* level = PUSH_STRUCT(arena, Level);
        if (level)
        {
            level->sessionArena = arena;
            level->entityCount = 1;
            result = level;
        }
        return result;
    }

    inline Chunk*
    GetChunk(Level* level, i32 x, i32 y, i32 z)
    {
        Chunk* result = 0;

        if ((x >= LEVEL_MIN_DIM_CHUNKS) &&
            (x <= LEVEL_MAX_DIM_CHUNKS) &&
            (y >= LEVEL_MIN_DIM_CHUNKS) &&
            (y <= LEVEL_MAX_DIM_CHUNKS) &&
            (z >= LEVEL_MIN_DIM_CHUNKS) &&
            (z <= LEVEL_MAX_DIM_CHUNKS))
        {
            // TODO: Better hash
            u32 hashMask = LEVEL_CHUNK_TABLE_SIZE - 1;
            u32 hash = (Abs(x) * 7 + Abs(y) * Abs(13) + Abs(z) * 23) & hashMask;

            for (u32 offset = 0; offset < LEVEL_CHUNK_TABLE_SIZE; offset++)
            {
                u32 index = (hash + offset) & hashMask;

                Chunk* chunk = level->chunkTable[index];
                if (chunk &&
                    chunk->coord.x == x &&
                    chunk->coord.y == y &&
                    chunk->coord.z == z)
                {
                    result = chunk;
                    break;
                }
            }
        }
        return result;
    }

    inline Chunk*
    GetChunk(Level* level, iv3 coord)
    {
        Chunk* result = GetChunk(level, coord.x, coord.y, coord.z);
        return result;
    }

    inline u32
    GetChunkEntityMapIndex(uv3 tileInChunk)
    {
        SOKO_ASSERT(tileInChunk.x < CHUNK_DIM &&
                    tileInChunk.y < CHUNK_DIM &&
                    tileInChunk.z < CHUNK_DIM);
        u32 index = tileInChunk.z * CHUNK_DIM * CHUNK_DIM + tileInChunk.y * CHUNK_DIM + tileInChunk.x;
        return index;
    }

    struct EntityMapIterator
    {
        u16 at;
        u8 isHead;
        union
        {
            ChunkEntityMapBlock* block;
            ChunkEntityMapResidentBlock* head;
            void* _rawPtr;
        };
    };

    inline Entity*
    YieldEntityFromTile(Chunk* chunk, uv3 tileInChunk, EntityMapIterator* it)
    {
        Entity* result = 0;

        if (!it->_rawPtr)
        {
            u32 index = GetChunkEntityMapIndex(tileInChunk);
            auto head = chunk->entityMap + index;
            if (head->at)
            {
                result = head->entities[0];
                it->isHead = 1;
                it->head = head;
                it->at = 1;
            }
        }
        else
        {
            if (it->isHead)
            {
                auto head = it->head;
                if (it->at < head->at)
                {
                    result = head->entities[it->at];
                    it->at++;
                }
                else
                {
                    it->isHead = 0;
                    it->block = head->next;
                    if (it->block)
                    {
                        it->at = 1;
                        result = it->block->entities[0];
                    }
                }
            }
            else
            {
                if (it->at < it->block->at)
                {
                    result = it->block->entities[it->at];
                    it->at++;
                }
                else
                {
                    it->block = it->block->next;
                    if (it->block)
                    {
                        it->at = 1;
                        result = it->block->entities[0];
                    }
                }
            }
        }

        return result;
    }

    inline Entity*
    YieldEntityFromTile(Level* level, iv3 tile, EntityMapIterator* at)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        Chunk* chunk = GetChunk(level, c);
        return YieldEntityFromTile(chunk, t, at);
    }

    inline ChunkEntityMapBlock*
    AllocEntityMapBlock(Level* level, AB::MemoryArena* arena)
    {
        ChunkEntityMapBlock* block = 0;
        if (level->freeChunkEntityMapBlocks)
        {
            auto next = level->freeChunkEntityMapBlocks->next;
            block = level->freeChunkEntityMapBlocks;
            level->freeChunkEntityMapBlocks = next;
            level->freeChunkEntityMapBlockCount++;
        }
        else
        {
            block = PUSH_STRUCT(arena, ChunkEntityMapBlock);
        }

        if (block)
        {
            level->chunkEntityMapBlockCount++;
            block->at = 0;
            block->next = 0;
        }

        return block;
    }

    internal bool
    RegisterEntityInTile(Chunk* chunk, Entity* entity, uv3 tileInChunk)
    {
        SOKO_STATIC_ASSERT(IsPowerOfTwo(CHUNK_ENTITY_MAP_SIZE));
        AB::MemoryArena* arena = chunk->level->sessionArena;

        bool result = false;

        u32 index = GetChunkEntityMapIndex(tileInChunk);

        auto head = chunk->entityMap + index;
        SOKO_ASSERT(head->at <= CHUNK_ENTITY_MAP_RESIDENT_BLOCK_SIZE);
        if (head->at < CHUNK_ENTITY_MAP_RESIDENT_BLOCK_SIZE)
        {
            head->entities[head->at] = entity;
            head->at++;
            result = true;
        }
        else
        {
            if (!head->next)
            {
                auto newBlock = AllocEntityMapBlock(chunk->level, arena);
                if (newBlock)
                {
                    head->next = newBlock;
                    newBlock->entities[newBlock->at] = entity;
                    newBlock->at++;
                    result = true;
                }
            }
            else
            {
                auto block = head->next;
                SOKO_ASSERT(block->at <= CHUNK_ENTITY_MAP_BLOCK_SIZE);
                if (block->at < CHUNK_ENTITY_MAP_BLOCK_SIZE)
                {
                    block->entities[block->at] = entity;
                    block->at++;
                    result = true;
                }
                else
                {
                    auto newBlock = AllocEntityMapBlock(chunk->level, arena);
                    if (newBlock)
                    {
                        head->next = newBlock;
                        newBlock->entities[newBlock->at] = entity;
                        newBlock->at++;
                        newBlock->next = block;
                        result = true;
                    }
                }
            }
        }
        return result;
    }


    inline bool
    RegisterEntityInTile(Chunk* chunk, Entity* entity)
    {
        uv3 t = GetTileCoordInChunk(entity->pos);
        return RegisterEntityInTile(chunk, entity, t);
    }

    inline bool
    RegisterEntityInTile(Level* level, Entity* entity, iv3 tile)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        Chunk* chunk = GetChunk(level, c);
        return RegisterEntityInTile(chunk, entity, t);
    }

    inline bool
    RegisterEntityInTile(Level* level, Entity* e)
    {
        iv3 c = GetChunkCoord(e->pos);
        Chunk* chunk = GetChunk(level, c);
        return RegisterEntityInTile(chunk, e);
    }

    internal bool
    UnregisterEntityInTile(Chunk* chunk, uv3 tileInChunk, Entity* e)
    {
        u32 index = GetChunkEntityMapIndex(tileInChunk);

        bool found = false;
        auto head = chunk->entityMap + index;
        for (u32 index = 0; index < head->at; index++)
        {
            if (head->entities[index] == e)
            {
                found = true;
                Entity* newPtr = 0;
                if (head->next && head->next->at)
                {
                    newPtr = head->next->entities[head->next->at - 1];
                    head->next->at--;
                }
                if (newPtr)
                {
                    head->entities[index] = newPtr;
                }
                else
                {
                    head->entities[index] = head->entities[head->at - 1];
                    head->at--;
                }
                break;
            }
        }

        if (!found)
        {
            auto block = head->next;
            while (block)
            {
                for (u32 index = 0; index < block->at; index++)
                {
                    if (block->entities[index] == e)
                    {
                        found = true;
                        Entity* newPtr = 0;
                        newPtr = head->next->entities[head->next->at - 1];
                        head->next->at--;
                        block->entities[index] = newPtr;
                        goto loopEnd;
                    }
                }
                block = block->next;
            }
        loopEnd:;
        }

        if (found)
        {
            auto block = head->next;
            if (block && !block->at)
            {
                head->next = block->next;
                block->next = chunk->level->freeChunkEntityMapBlocks;
                chunk->level->freeChunkEntityMapBlocks = block;
                chunk->level->freeChunkEntityMapBlockCount++;
                chunk->level->chunkEntityMapBlockCount--;
            }
        }
        return found;
    }

    internal bool
    UnregisterEntityInTile(Level* level, iv3 tile, Entity* e)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        Chunk* chunk = GetChunk(level, c);
        return UnregisterEntityInTile(chunk, t, e);
    }

    internal void
    UnregisterEntityInTile(Chunk* chunk, Entity* entity)
    {
        uv3 t = GetTileCoordInChunk(entity->pos);
        bool result = UnregisterEntityInTile(chunk, t, entity);
        SOKO_ASSERT(result);
    }

    inline void
    UnregisterEntityInTile(Level* level, Entity* e)
    {
        iv3 c = GetChunkCoord(e->pos);
        Chunk* chunk = GetChunk(level, c);
        UnregisterEntityInTile(chunk, e);
    }

    inline void
    SetTileInChunkInternal(Chunk* chunk, u32 x, u32 y, u32 z, TileValue value);

    inline Chunk*
    AddChunk(Level* level, i32 x, i32 y, i32 z)
    {
        Chunk* result = 0;

        SOKO_ASSERT(x >= LEVEL_MIN_DIM_CHUNKS && x <= LEVEL_MAX_DIM_CHUNKS);
        SOKO_ASSERT(y >= LEVEL_MIN_DIM_CHUNKS && y <= LEVEL_MAX_DIM_CHUNKS);
        SOKO_ASSERT(z >= LEVEL_MIN_DIM_CHUNKS && z <= LEVEL_MAX_DIM_CHUNKS);

        // TODO: Better hash
        u32 hashMask = LEVEL_CHUNK_TABLE_SIZE - 1;
        u32 hash = (Abs(x) * 7 + Abs(y) * Abs(13) + Abs(z) * 23) & hashMask;

        for (u32 offset = 0; offset < LEVEL_CHUNK_TABLE_SIZE; offset++)
        {
            u32 index = (hash + offset) & hashMask;

            Chunk* chunk = level->chunkTable[index];
            if (!chunk)
            {
                Chunk* newChunk = PUSH_STRUCT(level->sessionArena, Chunk);
                if (newChunk)
                {

                    newChunk->level = level;
                    newChunk->coord = IV3(x, y, z);
                    SET_ARRAY(Tile, CHUNK_TILE_COUNT, newChunk->tiles, (i32)TileValue_Empty);

                    ChunkMesh mesh = {};
                    if (GenChunkMesh(level, newChunk, &mesh))
                    {
                        // NOTE: Empty chunk should not allocate any memory for mesh
                        SOKO_ASSERT(!mesh.blockCount);
                        LoadedChunkMesh loadedMesh = RendererLoadChunkMesh(&mesh);
                        if (loadedMesh.gpuHandle)
                        {
                            newChunk->loadedMesh = loadedMesh;
                            newChunk->mesh = mesh;
                            level->chunkTable[index] = newChunk;
                            level->loadedChunksCount++;
                            result = newChunk;
                            break;
                        }
                    }
                    if (!result)
                    {
                        INVALID_CODE_PATH;
                        // TODO: Free chunk memory in editor
                        // In game this never should happen!!!
                    }
                }
            }
        }

        return result;
    }

    inline Chunk*
    AddChunk(Level* level, iv3 coord)
    {
        Chunk* result = AddChunk(level, coord.x, coord.y, coord.z);
        return result;
    }

    inline const Tile*
    GetTilePointerInChunkInternal(Chunk* chunk, u32 x, u32 y, u32 z)
    {
        // TODO: Actial check?
        SOKO_ASSERT(x < CHUNK_DIM);
        SOKO_ASSERT(y < CHUNK_DIM);
        SOKO_ASSERT(z < CHUNK_DIM);

        Tile* tile = 0;
        tile = chunk->tiles + (z * CHUNK_DIM * CHUNK_DIM + y * CHUNK_DIM + x);

        return tile;
    }

    inline const Tile*
    GetTileInChunk(Chunk* chunk, u32 x, u32 y, u32 z)
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
    GetTileInChunk(Chunk* chunk, uv3 tileInChunk)
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
    TileIsTerrain(Chunk* chunk, uv3 tileInChunk)
    {
        bool result = true;
        const Tile* tile = GetTileInChunk(chunk, tileInChunk);
        result = TileIsTerrain(tile);
        return result;
    }

    inline bool
    TileIsTerrain(Level* level, iv3 tile)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        Chunk* chunk = GetChunk(level, c);
        bool result = TileIsTerrain(chunk, t);
        return result;
    }

    enum TileCheckFlag : u32
    {
        TileCheck_Terrain = (1 << 0),
        TileCheck_Entities = (1 << 1)
    };

    inline bool
    CheckTile(Chunk* chunk, uv3 tileInChunk, u32 flags = TileCheck_Terrain | TileCheck_Entities)
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
                Entity* e = YieldEntityFromTile(chunk, tileInChunk, &it);
                if (!e) break;
                if (EntityCollisionChecks[e->behavior.type](chunk->level, e))
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
    CheckTile(Level* level, iv3 tile, u32 flags = TileCheck_Terrain | TileCheck_Entities)
    {
        iv3 c = GetChunkCoord(tile);
        uv3 t = GetTileCoordInChunk(tile);
        Chunk* chunk = GetChunk(level, c);
        return CheckTile(chunk, t, flags);
    }

    inline bool
    CanMove(Level* level,  iv3 tile)
    {
        iv3 groundTile = tile + DirToUnitOffset(Direction_Down);
        bool result = CheckTile(level, tile) && TileIsTerrain(level, groundTile);
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

#pragma pack(push, 1)

    struct SerializedTile
    {
        TileValue value;
    };

    struct SerializedChunk
    {
        iv3 coord;
        SerializedTile tiles[CHUNK_DIM * CHUNK_DIM * CHUNK_DIM];
        u32 filledTileCount;
    };

#pragma pack(pop)

    // TODO: Write to file directly, without temp buffer?
    internal bool
    SaveLevel(const Level* level, const wchar_t* filename, AB::MemoryArena* arena)
    {
        bool result = false;

        uptr headerSize = sizeof(AB::AABLevelHeader);
        uptr chunksSize = sizeof(SerializedChunk) * level->loadedChunksCount;
        uptr entitiesSize = CalcSerializedEntitiesSize(level);

        uptr bufferSize = headerSize + chunksSize + entitiesSize;

        byte* buffer = (byte*)PUSH_SIZE(arena, bufferSize);
        if (buffer)
        {
            auto header = (AB::AABLevelHeader*)buffer;
            header->magicValue = AB::AAB_FILE_MAGIC_VALUE;
            header->version = AB::AAB_LEVEL_FILE_VERSION;
            // TODO: Is that should be size of data placed after header?
            header->assetSize = bufferSize - sizeof(AB::AABLevelHeader);
            header->assetType = AB::AAB_FILE_TYPE_LEVEL;
            header->chunkCount = level->loadedChunksCount;
            header->chunkMeshBlockCount = level->globalChunkMeshBlockCount;
            header->firstChunkOffset = sizeof(AB::AABLevelHeader);
            SOKO_ASSERT(level->spawnerID);
            header->spawnerID = level->spawnerID;
            //header->entityCount = level->entityCount - 1; // Null entity is not considered
            header->firstEntityOffset = headerSize + chunksSize;

            auto chunks = (SerializedChunk*)(buffer + header->firstChunkOffset);

            u32 chunksWritten = 0;
            for (u32 i = 0; i < LEVEL_CHUNK_TABLE_SIZE; i++)
            {
                Chunk* chunk = level->chunkTable[i];
                if (chunk)
                {
                    chunksWritten++;
                    SOKO_ASSERT(chunksWritten <= header->chunkCount);
                    chunks[chunksWritten - 1].coord = chunk->coord;
                    chunks[chunksWritten - 1].filledTileCount = chunk->filledTileCount;
                    for (u32 tileInd = 0;
                         tileInd < (CHUNK_DIM * CHUNK_DIM * CHUNK_DIM);
                         tileInd++)
                    {
                        chunks[chunksWritten - 1].tiles[tileInd].value = chunk->tiles[tileInd].value;
                    }
                }
            }

            SOKO_ASSERT(chunksWritten == level->loadedChunksCount);

            void* entities = buffer + headerSize + chunksSize;
            // TODO: Checking
            header->entityCount = SerializeEntititiesToBuffer(level, entities, entitiesSize);

            SOKO_ASSERT(bufferSize <= 0xffffffff);
            result = DebugWriteFile(filename, buffer, (u32)bufferSize);
        }
        return result;
    }

    internal bool
    GetLevelMetaInfo(const wchar_t* filename, LevelMetaInfo* outInfo)
    {
        bool result = 0;
        u32 fileSize = DebugGetFileSize(filename);
        if (fileSize)
        {
            AABLevelHeader header;
            u32 headerResult = DebugReadFile(&header, sizeof(AABLevelHeader), filename);
            if (headerResult == sizeof(AABLevelHeader) &&
                header.magicValue == AAB_FILE_MAGIC_VALUE &&
                header.version == AAB_LEVEL_FILE_VERSION &&
                header.assetType == AAB_FILE_TYPE_LEVEL &&
                header.chunkCount &&
                header.chunkMeshBlockCount)
            {
                result = 1;
                if (outInfo)
                {
                    ZERO_STRUCT(LevelMetaInfo, outInfo);
                    outInfo->chunkCount = header.chunkCount;
                    outInfo->chunkMeshBlockCount = header.chunkMeshBlockCount;
                }
            }
        }
        return result;
    }

    inline uptr
    CalcLevelArenaSize(const LevelMetaInfo* info, uptr sizeForEntities)
    {
        uptr result = 0;
        u64 levelSize = sizeof(Level) + sizeof(Chunk) * info->chunkCount;
        uptr chunkMeshDataSize = sizeof(ChunkMeshVertexBlock) * info->chunkMeshBlockCount;
        SOKO_ASSERT(levelSize <= AB::UPTR_MAX);
        // NOTE: Adding some random value just in case
        uptr safetyPad = 128 + alignof(ChunkMeshVertexBlock) * info->chunkMeshBlockCount;
        result = (uptr)levelSize + chunkMeshDataSize + safetyPad + sizeForEntities;
        return result;
    }

    inline void
    RemeshChunk(Level* level, Chunk* chunk)
    {
        bool result = GenChunkMesh(level, chunk, &chunk->mesh);
        SOKO_ASSERT(result);
        chunk->loadedMesh.quadCount = RendererReloadChunkMesh(&chunk->mesh, chunk->loadedMesh.gpuHandle);
        chunk->dirty = false;
    }

    inline bool
    LoadChunks(AB::MemoryArena* levelArena, Level* loadedLevel,
               SerializedChunk* chunks, u32 chunkCount)
    {
        bool result = false;
        for (u32 chunkIdx = 0; chunkIdx < chunkCount; chunkIdx++)
        {
            SerializedChunk* sChunk = chunks + chunkIdx;
            Chunk* newChunk = AddChunk(loadedLevel, sChunk->coord);
            newChunk->filledTileCount = sChunk->filledTileCount;
            if (newChunk)
            {
                for (u32 z = 0; z < CHUNK_DIM; z++)
                {
                    for (u32 y = 0; y < CHUNK_DIM; y++)
                    {
                        for (u32 x = 0; x < CHUNK_DIM; x++)
                        {
                            u32 tileIdx =
                                z * CHUNK_DIM * CHUNK_DIM +
                                y * CHUNK_DIM +
                                x;
                            newChunk->tiles[tileIdx].value = sChunk->tiles[tileIdx].value;
                        }
                    }
                }
                // TODO: Store meshes on the cpu and load on the gpu only when necessary
                RemeshChunk(loadedLevel, newChunk);
                result = true;
            }
        }
        return result;
    }

    inline bool
    LoadEntities(Level* loadedLevel, SerializedEntityV2* entities, u32 entityCount)
    {
        bool result = true;
        for (u32 idx = 0; idx < entityCount; idx++)
        {
            SerializedEntityV2* sEntity = entities + idx;
            u32 id = AddSerializedEntity(loadedLevel, sEntity);
            if (id != sEntity->id)
            {
                result = false;
                break;
            }
        }
        return result;
    }

    internal Level*
    LoadLevel(const wchar_t* filename, AB::MemoryArena* levelArena, AB::MemoryArena* tempArena)
    {
        Level* result = 0;
        u32 fileSize = DebugGetFileSize(filename);
        if (fileSize)
        {
            AB::AABLevelHeader header;
            u32 headerResult = DebugReadFile(&header, sizeof(AB::AABLevelHeader), filename);
            if (headerResult == sizeof(AB::AABLevelHeader) &&
                header.magicValue == AB::AAB_FILE_MAGIC_VALUE &&
                header.version == AB::AAB_LEVEL_FILE_VERSION &&
                header.assetType == AB::AAB_FILE_TYPE_LEVEL)
            {
                void* fileBuffer = PUSH_SIZE(tempArena, fileSize);
                if (fileBuffer)
                {
                    u32 bytesRead = DebugReadFile(fileBuffer, fileSize, filename);
                    if (bytesRead == fileSize)
                    {
                        Level* loadedLevel = CreateLevel(levelArena);
                        if (loadedLevel)
                        {
                            SOKO_ASSERT(header.spawnerID);
                            loadedLevel->spawnerID = header.spawnerID;
                            auto chunks = (SerializedChunk*)((byte*)fileBuffer + header.firstChunkOffset);
                            if (LoadChunks(levelArena, loadedLevel, chunks, header.chunkCount))
                            {
                                auto entities = (SerializedEntityV2*)((byte*)fileBuffer + header.firstEntityOffset);
                                if (LoadEntities(loadedLevel, entities, header.entityCount))
                                {
                                    SOKO_ASSERT(loadedLevel->loadedChunksCount == header.chunkCount);
                                    SOKO_ASSERT(loadedLevel->entityCount == header.entityCount + 1);
                                    result = loadedLevel;
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    internal bool
    GenTestLevel(AB::MemoryArena* tempArena)
    {
        BeginTemporaryMemory(tempArena, true);
        bool result = 0;
        Level* level = CreateLevel(tempArena);
        for (i32 chunkX = LEVEL_MIN_DIM_CHUNKS; chunkX <= LEVEL_MAX_DIM_CHUNKS; chunkX++)
        {
            for (i32 chunkY = LEVEL_MIN_DIM_CHUNKS; chunkY <= LEVEL_MAX_DIM_CHUNKS; chunkY++)
            {
                Chunk* chunk = AddChunk(level, chunkX, chunkY, 0);
                SOKO_ASSERT(chunk);

                for (u32 x = 0; x < CHUNK_DIM; x++)
                {
                    for (u32 y = 0; y < CHUNK_DIM; y++)
                    {
                        SetTileInChunkInternal(chunk, x, y, 0, TileValue_Wall);

                        //if (chunkX == 0 && chunkY == 0)
                        {

                            if ((x == 0) || (x == CHUNK_DIM - 1) ||
                                (y == 0) || (y == CHUNK_DIM - 1) ||
                                (x == 20 && y == 20) ||
                                (x == 20 && y == 21) ||
                                (x == 21 && y == 21))
                            {
                                i32 z = 1;
                                if ((x == 20 && y == 21) ||
                                    (x == 21 && y == 21))
                                {
                                    z = 2;
                                }
                                if (y == 15 || x == 15)
                                {
                                    SetTileInChunkInternal(chunk, x, y, z, TileValue_Empty);
                                }
                                else
                                {
                                    SetTileInChunkInternal(chunk, x, y, z, TileValue_Wall);
                                }
                            }
                        }
                    }
                }
                ChunkMesh mesh = {};
                bool meshResult = GenChunkMesh(level, chunk, &mesh);
                SOKO_ASSERT(meshResult);
                chunk->mesh = mesh;
                //level->globalChunkMeshBlockCount += mesh.blockCount;
            }
        }

        Entity entity1 = {};
        entity1.type = EntityType_Block;
        entity1.flags = EntityFlag_Collides | EntityFlag_Movable;
        entity1.pos = IV3(5, 7, 1);
        entity1.movementSpeed = 5.0f;
        entity1.mesh = EntityMesh_Cube;
        entity1.material = EntityMaterial_Block;

        AddEntity(level, entity1);
        //AddEntity(playerLevel)

        Entity spawner = {};
        spawner.type = EntityType_Spawner;
        spawner.flags = 0;
        spawner.pos = IV3(10, 10, 1);
        spawner.movementSpeed = 0.0f;
        spawner.mesh = EntityMesh_Plate;
        spawner.material = EntityMaterial_RedPlate;

        u32 spawnerID = AddEntity(level, spawner);
        SOKO_ASSERT(spawnerID);
        level->spawnerID = spawnerID;
        //AddEntity(playerLevel)


        Entity entity2 = {};
        entity2.type = EntityType_Block;
        entity2.flags = EntityFlag_Collides | EntityFlag_Movable;
        entity2.pos = IV3(5, 8, 1);
        entity2.mesh = EntityMesh_Cube;
        entity2.movementSpeed = 5.0f;
        entity2.material = EntityMaterial_Block;

        AddEntity(level, entity2);

        Entity entity3 = {};
        entity3.type = EntityType_Block;
        entity3.flags = EntityFlag_Collides | EntityFlag_Movable;
        entity3.pos = IV3(5, 9, 1);
        entity3.mesh = EntityMesh_Cube;
        entity3.material = EntityMaterial_Block;
        entity3.movementSpeed = 5.0f;

        AddEntity(level, entity3);

        Entity plate = {};
        plate.type = EntityType_Plate;
        plate.flags = 0;
        plate.pos = IV3(10, 9, 1);
        plate.mesh = EntityMesh_Plate;
        plate.material = EntityMaterial_RedPlate;


        AddEntity(level, plate);

        Entity portal1 = {};
        portal1.type = EntityType_Portal;
        portal1.flags = 0;
        portal1.pos = IV3(12, 12, 1);
        portal1.mesh = EntityMesh_Portal;
        portal1.material = EntityMaterial_Portal;
        portal1.portalDirection = Direction_North;

        Entity* portal1Entity = GetEntity(level, AddEntity(level, portal1));

        Entity portal2 = {};
        portal2.type = EntityType_Portal;
        portal2.flags = 0;
        portal2.pos = IV3(17, 17, 1);
        portal2.mesh = EntityMesh_Portal;
        portal2.material = EntityMaterial_Portal;
        portal2.portalDirection = Direction_West;

        Entity* portal2Entity = GetEntity(level, AddEntity(level, portal2));

        portal1Entity->bindedPortalID = portal2Entity->id;
        portal2Entity->bindedPortalID = portal1Entity->id;

        AddEntity(level, EntityType_Spikes, IV3(15, 15, 1), 0.0f,
                  EntityMesh_Spikes, EntityMaterial_Spikes);
        Entity* button = GetEntity(level, AddEntity(level, EntityType_Button, IV3(4, 4, 1), 0.0f,
                                                    EntityMesh_Button, EntityMaterial_Button));
#if 0
        // TODO: Entity custom behavior
        button->updateProc = [](Level* level, Entity* entity, void* data) {
            GameState* gameState = (GameState*)data;
            AddEntity(level, ENTITY_TYPE_BLOCK, IV3(4, 5, 1),
                      EntityMesh_Cube, EntityMaterial_Block,
                      gameState->memoryArena);
        };
        button->updateProcData = (void*)gameState;
#endif

        if(SaveLevel(level, L"testLevel.aab", tempArena))
        {
            result = 1;
        }
        EndTemporaryMemory(tempArena);
        return result;
    }
}
