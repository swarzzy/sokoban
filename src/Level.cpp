#include "Level.h"
#include "Memory.h"
#include "Entity.h"

namespace soko
{
    inline v3i
    DirToUnitOffset(Direction dir)
    {
        v3i result = {};
        switch (dir)
        {
        case DIRECTION_NORTH: { result = V3I(0, 1, 0); } break;
        case DIRECTION_SOUTH: { result = V3I(0, -1, 0); } break;
        case DIRECTION_WEST:  { result = V3I(-1, 0, 0); } break;
        case DIRECTION_EAST:  { result = V3I(1, 0, 0); } break;
        case DIRECTION_UP:    { result = V3I(0, 0, 1); } break;
        case DIRECTION_DOWN:  { result = V3I(0, 0, -1); } break;
            INVALID_DEFAULT_CASE;
        }
        return result;
    }

    internal Level*
    CreateLevel(AB::MemoryArena* arena, u32 chunksNum)
    {
        SOKO_ASSERT(chunksNum <= LEVEL_FULL_DIM_CHUNKS * LEVEL_FULL_DIM_CHUNKS * LEVEL_FULL_DIM_CHUNKS);
        Level* result = 0;
        Level* level = PUSH_STRUCT(arena, Level);
        if (level)
        {
            level->chunkTable = PUSH_ARRAY(arena, Chunk, chunksNum);
            if (level->chunkTable)
            {
                level->chunkTableSize = chunksNum;
                level->sessionArena = arena;
                for (u32 i = 0; i < chunksNum; i++)
                {
                    Chunk* chunk = level->chunkTable + i;
                    chunk->coord = V3I(LEVEL_INVALID_COORD);
                    // NOTE: Reserving null entity
                    level->entityCount = 1;
                }
                result = level;
            }
        }
        return result;
    }

    inline Chunk*
    GetChunk(Level* level, i32 x, i32 y, i32 z)
    {
        Chunk* result = 0;

        SOKO_ASSERT(x >= LEVEL_MIN_DIM_CHUNKS && x <= LEVEL_MAX_DIM_CHUNKS);
        SOKO_ASSERT(y >= LEVEL_MIN_DIM_CHUNKS && y <= LEVEL_MAX_DIM_CHUNKS);
        SOKO_ASSERT(z >= LEVEL_MIN_DIM_CHUNKS && z <= LEVEL_MAX_DIM_CHUNKS);

        // TODO: Better hash
        u32 hash = (Abs(x) * 7 + Abs(y) * Abs(13) + Abs(z) * 23) % level->chunkTableSize;

        for (u32 offset = 0; offset < level->chunkTableSize; offset++)
        {
            u32 index = hash + offset;
            if (index > (level->chunkTableSize - 1))
            {
                index -= level->chunkTableSize;
            }
            Chunk* chunk = level->chunkTable + index;
            if (chunk->loaded &&
                chunk->coord.x == x &&
                chunk->coord.y == y &&
                chunk->coord.z == z)
            {
                result = chunk;
                break;
            }
        }
        return result;
    }

    inline Chunk*
    GetChunk(Level* level, v3i coord)
    {
        Chunk* result = GetChunk(level, coord.x, coord.y, coord.z);
        return result;
    }

    inline Chunk*
    InitChunk(Level* level, i32 x, i32 y, i32 z)
    {
        Chunk* result = 0;

        SOKO_ASSERT(x >= LEVEL_MIN_DIM_CHUNKS && x <= LEVEL_MAX_DIM_CHUNKS);
        SOKO_ASSERT(y >= LEVEL_MIN_DIM_CHUNKS && y <= LEVEL_MAX_DIM_CHUNKS);
        SOKO_ASSERT(z >= LEVEL_MIN_DIM_CHUNKS && z <= LEVEL_MAX_DIM_CHUNKS);

        // TODO: Better hash
        u32 hash = (Abs(x) * 7 + Abs(y) * Abs(13) + Abs(z) * 23) % level->chunkTableSize;

        for (u32 offset = 0; offset < level->chunkTableSize; offset++)
        {
            u32 index = hash + offset;
            if (index > (level->chunkTableSize - 1))
            {
                index -= level->chunkTableSize;
            }
            Chunk* chunk = level->chunkTable + index;
            if (!chunk->loaded)
            {
                chunk->loaded = true;
                chunk->coord = V3I(x, y, z);
                result = chunk;
                break;
            }
        }

        level->loadedChunksCount++;
        if (result)
        {
            for (u32 tileZ = 0; tileZ < CHUNK_DIM; tileZ++)
            {
                for (u32 tileY = 0; tileY < CHUNK_DIM; tileY++)
                {
                    for (u32 tileX = 0; tileX < CHUNK_DIM; tileX++)
                    {
                        u32 tileIndex = tileZ * CHUNK_DIM * CHUNK_DIM + tileY * CHUNK_DIM + tileX;
                        Tile* tile = result->tiles + tileIndex;
                        tile->coord = V3I(x * CHUNK_DIM + tileX, y * CHUNK_DIM + tileY, z * CHUNK_DIM + tileZ);
                    }
                }
            }
        }
        return result;
    }

    inline Chunk*
    InitChunk(Level* level, v3i coord)
    {
        Chunk* result = InitChunk(level, coord.x, coord.y, coord.z);
        return result;
    }

    inline Tile*
    GetTileInChunkInternal(Chunk* chunk, u32 x, u32 y, u32 z)
    {
        // TODO: Actial check?
        SOKO_ASSERT(x < CHUNK_DIM);
        SOKO_ASSERT(y < CHUNK_DIM);
        SOKO_ASSERT(z < CHUNK_DIM);

        Tile* tile = 0;
        tile = chunk->tiles + (z * CHUNK_DIM * CHUNK_DIM + y * CHUNK_DIM + x);

        return tile;
    }

    inline Tile*
    GetTileInChunk(Chunk* chunk, u32 x, u32 y, u32 z)
    {
        Tile* result = 0;
        if (x < CHUNK_DIM &&
            y < CHUNK_DIM &&
            z < CHUNK_DIM)
        {
            result = GetTileInChunkInternal(chunk, x, y, z);
        }
        return result;
    }

    internal Tile*
    GetTile(Level* level, i32 x, i32 y, i32 z)
    {
        Tile* result = 0;
        // TODO: Real check instead of asserts
        SOKO_ASSERT(x >= LEVEL_MIN_DIM && x <= LEVEL_MAX_DIM);
        SOKO_ASSERT(y >= LEVEL_MIN_DIM && y <= LEVEL_MAX_DIM);
        SOKO_ASSERT(z >= LEVEL_MIN_DIM && z <= LEVEL_MAX_DIM);

        u32 chunkX = x >> CHUNK_BIT_SHIFT;
        u32 chunkY = y >> CHUNK_BIT_SHIFT;
        u32 chunkZ = z >> CHUNK_BIT_SHIFT;

        u32 tileX = x & CHUNK_BIT_MASK;
        u32 tileY = y & CHUNK_BIT_MASK;
        u32 tileZ = z & CHUNK_BIT_MASK;

        Chunk* chunk = GetChunk(level, chunkX, chunkY, chunkZ);

        if (chunk)
        {
            result = GetTileInChunkInternal(chunk, tileX, tileY, tileZ);
        }
        return result;
    }

    inline Tile*
    GetTile(Level* level, v3i coord)
    {
        Tile* result = GetTile(level, coord.x, coord.y, coord.z);
        return result;
    }

    // NOTE: Checks if it is allowed to place entity on tile
    // NOT if tile is EMPTY!!!!
    inline bool
    TileIsFree(const Tile* tile)
    {
        bool result = true;
        result = tile->value == TILE_VALUE_EMPTY;
        if (result)
        {
            for (const Entity& entity : tile->entityList)
            {
                if (IsSet(entity, ENTITY_FLAG_COLLIDES))
                {
                    result = false;
                    break;
                }
            }
        }
        return result;
    }


    internal void
    DrawLevel(Level* level, GameState* gameState)
    {
        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH, 0);
        // TODO: Sparseness
        for (u32 chunkIndex = 0; chunkIndex < level->chunkTableSize; chunkIndex++)
        {
            Chunk* chunk = level->chunkTable + chunkIndex;
            if (chunk)// && chunk->coord.x == 0 && chunk->coord.y == 0)
            {
                f32 chunkSize = LEVEL_TILE_SIZE * CHUNK_DIM;
                v3 chunkCoord = V3((f32)chunk->coord.x, (f32)chunk->coord.z, (f32)chunk->coord.y);
                v3 offset = Hadamard(chunkCoord, V3(chunkSize));
                RenderCommandPushChunkMesh c = {};
                c.offset = offset;
                c.meshIndex = chunk->loadedMesh.gpuHandle;
                c.quadCount = chunk->loadedMesh.quadCount;
                RenderGroupPushCommand(gameState->renderGroup,
                                       RENDER_COMMAND_PUSH_CHUNK_MESH, (void*)&c);
            }
        }
        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_END_CHUNK_MESH_BATCH, 0);
    }

#pragma pack(push, 1)

    struct SerializedTile
    {
        TileValue value;
    };

    struct SerializedChunk
    {
        v3i coord;
        SerializedTile tiles[CHUNK_DIM * CHUNK_DIM * CHUNK_DIM];
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
            header->version = AB::AAB_FILE_VERSION;
            // TODO: Is that should be size of data placed after header?
            header->assetSize = bufferSize - sizeof(AB::AABLevelHeader);
            header->assetType = AB::AAB_FILE_TYPE_LEVEL;
            header->chunkCount = level->loadedChunksCount;
            header->chunkMeshBlockCount = level->globalChunkMeshBlockCount;
            header->firstChunkOffset = sizeof(AB::AABLevelHeader);
            header->entityCount = level->entityCount - 1; // Null entity is not considered
            header->firstEntityOffset = headerSize + chunksSize;

            auto chunks = (SerializedChunk*)(buffer + header->firstChunkOffset);

            u32 chunksWritten = 0;
            for (u32 i = 0; i < level->chunkTableSize; i++)
            {
                Chunk* chunk = level->chunkTable + i;
                if (chunk->loaded)
                {
                    chunksWritten++;
                    SOKO_ASSERT(chunksWritten <= header->chunkCount);
                    chunks[chunksWritten - 1].coord = chunk->coord;
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
            SerializeEntititiesToBuffer(level, entities, entitiesSize);

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
                header.version == AAB_FILE_VERSION &&
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

    inline bool
    LoadChunks(AB::MemoryArena* levelArena, Level* loadedLevel,
               SerializedChunk* chunks, u32 chunkCount)
    {
        bool result = false;
        for (u32 chunkIdx = 0; chunkIdx < chunkCount; chunkIdx++)
        {
            SerializedChunk* sChunk = chunks + chunkIdx;
            Chunk* newChunk = InitChunk(loadedLevel, sChunk->coord);
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

                            newChunk->tiles[tileIdx].coord = V3I(x, y, z);
                            newChunk->tiles[tileIdx].value = sChunk->tiles[tileIdx].value;
                        }
                    }
                }
                // TODO: Store meshes on the cpu and load on the gpu only when necessary
                ChunkMesh mesh;
                if (GenChunkMesh(newChunk, &mesh, levelArena))
                {
                    // TODO: Check if loading failed
                    LoadedChunkMesh loadedMesh = RendererLoadChunkMesh(&mesh);
                    newChunk->loadedMesh = loadedMesh;
                    newChunk->mesh = mesh;
                    loadedLevel->globalChunkMeshBlockCount += mesh.blockCount;
                    result = true;
                }
                else
                {
                    // TODO: Decide what to do when chunk mesh loading failed
                    result = false;
                    break;
                }
            }
        }
        return result;
    }

    inline bool
    LoadEntities(Level* loadedLevel, SerializedEntity* entities, u32 entityCount)
    {
        bool result = true;
        for (u32 idx = 0; idx < entityCount; idx++)
        {
            SerializedEntity* sEntity = entities + idx;
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
                header.version == AB::AAB_FILE_VERSION &&
                header.assetType == AB::AAB_FILE_TYPE_LEVEL)
            {
                void* fileBuffer = PUSH_SIZE(tempArena, fileSize);
                if (fileBuffer)
                {
                    u32 bytesRead = DebugReadFile(fileBuffer, fileSize, filename);
                    if (bytesRead == fileSize)
                    {
                        Level* loadedLevel = CreateLevel(levelArena, header.chunkCount);
                        if (loadedLevel)
                        {
                            auto chunks = (SerializedChunk*)((byte*)fileBuffer + header.firstChunkOffset);
                            if (LoadChunks(levelArena, loadedLevel, chunks, header.chunkCount))
                            {
                                auto entities = (SerializedEntity*)((byte*)fileBuffer + header.firstEntityOffset);
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
        Level* level = CreateLevel(tempArena, 8 * 8);
        for (i32 chunkX = LEVEL_MIN_DIM_CHUNKS; chunkX < LEVEL_MAX_DIM_CHUNKS; chunkX++)
        {
            for (i32 chunkY = LEVEL_MIN_DIM_CHUNKS; chunkY < LEVEL_MAX_DIM_CHUNKS; chunkY++)
            {
                Chunk* chunk = InitChunk(level, chunkX, chunkY, 0);
                SOKO_ASSERT(chunk);

                for (u32 x = 0; x < CHUNK_DIM; x++)
                {
                    for (u32 y = 0; y < CHUNK_DIM; y++)
                    {
                        Tile* tile = GetTileInChunk(chunk, x, y, 0);
                        SOKO_ASSERT(tile);
                        tile->value = TILE_VALUE_WALL;

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
                            Tile* tile1 = GetTileInChunk(chunk, x, y, z);
                            SOKO_ASSERT(tile1);
                            tile1->value = TILE_VALUE_WALL;
                        }
                    }
                }
                ChunkMesh mesh;
                bool meshResult = GenChunkMesh(chunk, &mesh, tempArena);
                SOKO_ASSERT(meshResult);
                chunk->mesh = mesh;
                level->globalChunkMeshBlockCount += mesh.blockCount;
            }
        }

        Entity entity1 = {};
        entity1.type = ENTITY_TYPE_BLOCK;
        entity1.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity1.coord = V3I(5, 7, 1);
        entity1.mesh = EntityMesh_Cube;
        entity1.material = EntityMaterial_Block;

        AddEntity(level, entity1);
        //AddEntity(playerLevel)

        Entity entity2 = {};
        entity2.type = ENTITY_TYPE_BLOCK;
        entity2.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity2.coord = V3I(5, 8, 1);
        entity2.mesh = EntityMesh_Cube;
        entity2.material = EntityMaterial_Block;

        AddEntity(level, entity2);

        Entity entity3 = {};
        entity3.type = ENTITY_TYPE_BLOCK;
        entity3.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity3.coord = V3I(5, 9, 1);
        entity3.mesh = EntityMesh_Cube;
        entity3.material = EntityMaterial_Block;

        AddEntity(level, entity3);

        Entity plate = {};
        plate.type = ENTITY_TYPE_PLATE;
        plate.flags = 0;
        plate.coord = V3I(10, 9, 1);
        plate.mesh = EntityMesh_Plate;
        plate.material = EntityMaterial_RedPlate;

        AddEntity(level, plate);

        Entity portal1 = {};
        portal1.type = ENTITY_TYPE_PORTAL;
        portal1.flags = 0;
        portal1.coord = V3I(12, 12, 1);
        portal1.mesh = EntityMesh_Portal;
        portal1.material = EntityMaterial_Portal;
        portal1.portalDirection = DIRECTION_NORTH;

        Entity* portal1Entity = GetEntity(level, AddEntity(level, portal1));

        Entity portal2 = {};
        portal2.type = ENTITY_TYPE_PORTAL;
        portal2.flags = 0;
        portal2.coord = V3I(17, 17, 1);
        portal2.mesh = EntityMesh_Portal;
        portal2.material = EntityMaterial_Portal;
        portal2.portalDirection = DIRECTION_WEST;

        Entity* portal2Entity = GetEntity(level, AddEntity(level, portal2));

        portal1Entity->bindedPortalID = portal2Entity->id;
        portal2Entity->bindedPortalID = portal1Entity->id;

        AddEntity(level, ENTITY_TYPE_SPIKES, V3I(15, 15, 1),
                  EntityMesh_Spikes, EntityMaterial_Spikes);
        Entity* button = GetEntity(level, AddEntity(level, ENTITY_TYPE_BUTTON, V3I(4, 4, 1),
                                                    EntityMesh_Button, EntityMaterial_Button));
#if 0
        // TODO: Entity custom behavior
        button->updateProc = [](Level* level, Entity* entity, void* data) {
            GameState* gameState = (GameState*)data;
            AddEntity(level, ENTITY_TYPE_BLOCK, V3I(4, 5, 1),
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
