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

    inline const Chunk*
    GetChunk(const Level* level, iv3 coord)
    {
        // NOTE: Using this hack for avoiding bothering with separate const routine
        Level* unconstLevel = (Level*)level;
        return GetChunk(unconstLevel, coord);
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
        bool spawnTileIsFree = CanMove(level, level->playerSpawnPos, 0);

        if (spawnTileIsFree)
        {

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

                // NOTE: Go completely crazy and just use some random number instead of hash
                // TODO: IMPORTANT: Hash!!!
                header->guid = (u64)DummyRandom();

                header->chunkCount = level->loadedChunksCount;
                header->chunkMeshBlockCount = level->globalChunkMeshBlockCount;
                header->firstChunkOffset = sizeof(AB::AABLevelHeader);
                header->firstEntityOffset = headerSize + chunksSize;
                header->playerSpawnPos = level->playerSpawnPos;

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
        }
        return result;
    }

    internal bool
    GetLevelMetaInfo(const wchar_t* filename, LevelMetaInfo* outInfo)
    {
        bool result = false;
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
                result = true;
                if (outInfo)
                {
                    ZERO_STRUCT(LevelMetaInfo, outInfo);
                    outInfo->guid = header.guid;
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

    internal void
    InitEntities(Level* level)
    {
        Entity* e = 0;
        for (u32 index = 0; index < ArrayCount(level->entities); index++)
        {
            e = level->entities[index];
            if (e)
            {
                do
                {
                    UpdateEntity(level, e);
                    e = e->nextEntity;
                }
                while (e);
            }
        }
        level->initialized = true;
    }

    inline bool
    CheckLoadedLevel(const Level* level, const AB::AABLevelHeader* header)
    {
        bool result = false;
        if (level->loadedChunksCount == header->chunkCount)
        {
            if (level->entityCount == header->entityCount + 1)
            {
                if (CanMove(level, header->playerSpawnPos, 0))
                {
                    result = true;
                }
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
                            loadedLevel->playerSpawnPos = header.playerSpawnPos;
                            loadedLevel->guid = header.guid;

                            auto chunks = (SerializedChunk*)((byte*)fileBuffer + header.firstChunkOffset);
                            if (LoadChunks(levelArena, loadedLevel, chunks, header.chunkCount))
                            {
                                auto entities = (SerializedEntityV2*)((byte*)fileBuffer + header.firstEntityOffset);
                                if (LoadEntities(loadedLevel, entities, header.entityCount))
                                {
                                    bool valid = CheckLoadedLevel(loadedLevel, &header);
                                    if (valid)
                                    {
                                        InitEntities(loadedLevel);
                                        result = loadedLevel;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    // TODO: Fix this
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
                        SetTileInChunk(chunk, x, y, 0, TileValue_Wall);

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
                                    SetTileInChunk(chunk, x, y, z, TileValue_Empty);
                                }
                                else
                                {
                                    SetTileInChunk(chunk, x, y, z, TileValue_Wall);
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

        level->playerSpawnPos = IV3(10, 10, 1);

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
