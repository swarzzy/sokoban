#include "Level.h"
#include "Memory.h"

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
    GetTile(Level* level, i32 x, i32 y, i32 z, AB::MemoryArena* arena = 0)
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
    GetTile(Level* level, v3i coord, AB::MemoryArena* arena = 0)
    {
        Tile* result = GetTile(level, coord.x, coord.y, coord.z, arena);
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

    inline Entity*
    GetEntityMemory(Level* level, AB::MemoryArena* arena)
    {
        Entity* result = nullptr;
        if (level->entityFreeList)
        {
            result = level->entityFreeList;
            level->entityFreeList = level->entityFreeList->nextEntity;
            level->deletedEntityCount--;
            ZERO_STRUCT(Entity, result);
        }
        else if (arena)
        {
            result = PUSH_STRUCT(arena, Entity);
        }
        return result;
    }

    internal void
    DeleteEntity(Level* level, Entity* entity)
    {
        u32 entityHash = entity->id % LEVEL_ENTITY_TABLE_SIZE;
        Entity* prevEntity = nullptr;
        Entity* bucketEntity = level->entities[entityHash];
        while (bucketEntity)
        {
            if (bucketEntity->id == entity->id)
            {
                if (prevEntity)
                {
                    prevEntity->nextEntity = entity->nextEntity;
                }
                else
                {
                    level->entities[entityHash] = entity->nextEntity;
                }
                if (entity->prevEntityInTile)
                {
                    entity->prevEntityInTile->nextEntityInTile = entity->nextEntityInTile;
                }
                else
                {
                    Tile* tile = GetTile(level, entity->coord);
                    SOKO_ASSERT(tile);
                    SOKO_ASSERT(tile->entityList.first->id == entity->id);
                    tile->entityList.first = entity->nextEntityInTile;
                }
                if (entity->nextEntityInTile)
                {
                    entity->nextEntityInTile->prevEntityInTile = entity->prevEntityInTile;
                }

                level->entityCount--;
                entity->nextEntity = level->entityFreeList;
                level->entityFreeList = entity;
                level->deletedEntityCount++;
                break;
            }
            prevEntity = bucketEntity;
            bucketEntity = bucketEntity->nextEntity;
        }
    }

    internal u32
    AddEntity(Level* level, Entity entity, AB::MemoryArena* arena)
    {
        u32 result = 0;
        Tile* tile = GetTile(level, entity.coord, arena);
        if (tile)
        {
            if (tile->value != TILE_VALUE_WALL && TileIsFree(tile))
            {
                // TODO: Better hash
                u32 entityId = level->entitySerialNumber + 1;
                u32 entityHash = entityId % LEVEL_ENTITY_TABLE_SIZE;
                Entity* bucketEntity = level->entities[entityHash];
                Entity* newEntity = GetEntityMemory(level, arena);
                if (newEntity)
                {
                    newEntity->nextEntity = bucketEntity;
                    level->entities[entityHash] = newEntity;
                    level->entitySerialNumber++;
                    level->entityCount++;

                    *newEntity = entity;
                    newEntity->id = entityId;
                    newEntity->nextEntityInTile = tile->entityList.first;
                    newEntity->prevEntityInTile = NULL;
                    if (tile->entityList.first)
                    {
                        tile->entityList.first->prevEntityInTile = newEntity;
                    }
                    tile->entityList.first = newEntity;

                    result = entityId;
                }
            }
        }
        return result;
    }

    inline u32
    AddEntity(Level* level, EntityType type, v3i coord,
              EntityMesh mesh, EntityMaterial material, AB::MemoryArena* arena)
    {
        u32 result = 0;
        Entity entity = {};
        entity.type = type;
        entity.coord = coord;
        entity.mesh = mesh;
        entity.material = material;
        switch (type)
        {
        case ENTITY_TYPE_BLOCK:  { entity.flags = ENTITY_FLAG_MOVABLE | ENTITY_FLAG_COLLIDES; } break;
        case ENTITY_TYPE_PLAYER: { entity.flags = ENTITY_FLAG_MOVABLE | ENTITY_FLAG_COLLIDES | ENTITY_FLAG_PLAYER; } break;
        case ENTITY_TYPE_PLATE:  { entity.flags = 0; } break;
        case ENTITY_TYPE_PORTAL: { entity.flags = 0; } break;
        case ENTITY_TYPE_SPIKES: { entity.flags = 0; } break;
        case ENTITY_TYPE_BUTTON: { entity.flags = 0; } break;
            INVALID_DEFAULT_CASE;
        }
        result = AddEntity(level, entity, arena);
        return result;
    }

    inline Entity*
    GetEntity(Level* level, u32 id)
    {
        Entity* result = nullptr;
        u32 entityHash = id % LEVEL_ENTITY_TABLE_SIZE;
        Entity* entity = level->entities[entityHash];
        while (entity)
        {
            if (entity->id == id)
            {
                result = entity;
                break;
            }
            entity = entity->nextEntity;
        }
        return result;
    }

    internal bool
    ChangeEntityLocation(Level* level, Entity* entity, v3i desiredCoord, AB::MemoryArena* arena);

    internal void
    UpdateEntitiesInTile(Level* level, Tile* tile, AB::MemoryArena* arena)
    {
        for (Entity& entity : tile->entityList)
        {
            switch (entity.type)
            {
            case ENTITY_TYPE_PLATE:
            {
                level->platePressed = false;
                for (Entity& e : tile->entityList)
                {
                    if (&e != &entity)
                    {
                        if (e.type == ENTITY_TYPE_BLOCK)
                        {
                            level->platePressed = true;
                        }
                    }
                }
            } break;
            case ENTITY_TYPE_PORTAL:
            {
                for (Entity& e : tile->entityList)
                {
                    if (&e != &entity)
                    {
                        if (IsSet(e, ENTITY_FLAG_MOVABLE))
                        {
                            // NOTE: No need in flag if teleporting
                            // not no the same tile where portal located
#if 0
                            if (IsSet(e, ENTITY_FLAG_JUST_TELEPORTED))
                            {
                                UnsetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);
                            }
                            else
                            {
                                SetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);
                                v3i newCoord = GetEntity(level, entity.bindedPortalID)->coord + DirToUnitOffset(entity.portalDirection);
                                bool teleported = ChangeEntityLocation(level, &e, newCoord, arena);
                                if (!teleported)
                                {
                                    UnsetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);
                                }
                            }
#endif
                            SetFlag(e, ENTITY_FLAG_JUST_TELEPORTED);
                            v3i newCoord = GetEntity(level, entity.bindedPortalID)->coord + DirToUnitOffset(entity.portalDirection);
                            bool teleported = ChangeEntityLocation(level, &e, newCoord, arena);

                        }
                    }
                }
            } break;
            case ENTITY_TYPE_SPIKES: {
                for (Entity& e : tile->entityList)
                {
                    if (&e != &entity)
                    {
                        if (e.type != ENTITY_TYPE_PLAYER)
                        {
                            DeleteEntity(level, &e);
                        }
                    }
                }
            } break;
            case ENTITY_TYPE_BUTTON:
            {
                // TODO: Pass info about entity that causes an update
                // instead of travercing all entities all the time
                // TODO: Entity custom behavoir
#if 0
                for (Entity& e : tile->entityList)
                {
                    if (e.type == ENTITY_TYPE_BLOCK || e.type == ENTITY_TYPE_PLAYER)
                    {
                        entity.updateProc(level, &entity, entity.updateProcData);
                        break;
                    }
                }
#endif
            }
            default: {} break;
            }
        }
    }

    internal bool
    ChangeEntityLocation(Level* level, Entity* entity, v3i desiredCoord, AB::MemoryArena* arena)
    {
        bool result = false;
        Tile* oldTile = GetTile(level, entity->coord);
        SOKO_ASSERT(oldTile);

        Tile* desiredTile = GetTile(level, desiredCoord, arena);
        if (desiredTile)
        {
            bool tileIsFree = desiredTile->value != TILE_VALUE_WALL;
            // TODO: For all entities in tile (iterator)
            if (tileIsFree)
            {
                for (Entity& entityInTile : desiredTile->entityList)
                {
                    if (IsSet(entityInTile, ENTITY_FLAG_COLLIDES))
                    {
                        tileIsFree = false;
                        break;
                    }
                }
            }
            if (tileIsFree)
            {
                if (entity->prevEntityInTile)
                {
                    entity->prevEntityInTile->nextEntityInTile = entity->nextEntityInTile;
                }
                else
                {
                    oldTile->entityList.first = entity->nextEntityInTile;
                    if (oldTile->entityList.first)
                    {
                        oldTile->entityList.first->prevEntityInTile = NULL;
                    }
                }

                entity->nextEntityInTile = desiredTile->entityList.first;
                entity->prevEntityInTile = NULL;
                if (desiredTile->entityList.first)
                {
                    desiredTile->entityList.first->prevEntityInTile = entity;
                }
                desiredTile->entityList.first = entity;
                entity->coord = desiredCoord;

                UpdateEntitiesInTile(level, oldTile, arena);
                UpdateEntitiesInTile(level, desiredTile, arena);

                result = true;
            }
        }
        return result;
    }

    internal bool
    MoveEntity(Level* level, Entity* entity, Direction dir, AB::MemoryArena* arena, bool reverse = false, u32 depth = 2)
    {
        bool result = false;
        v3i desiredPos = entity->coord;
        desiredPos += DirToUnitOffset(dir);
        v3i revDesiredPos = entity->coord;
        revDesiredPos -= DirToUnitOffset(dir);

        Tile* desiredTile = GetTile(level, desiredPos, arena);
        Tile* oldTile = GetTile(level, entity->coord);
        Tile* pushTile = GetTile(level, reverse ? revDesiredPos : desiredPos);

        SOKO_ASSERT(oldTile);
        if (desiredTile)
        {
            if (reverse)
            {
                result = ChangeEntityLocation(level, entity, desiredPos, arena);
            }

            if (pushTile && pushTile->value != TILE_VALUE_WALL)
            {
                Entity* entityInTile = pushTile->entityList.first;
                bool recursive = (bool)depth;
                if (recursive)
                {
                    for (Entity& e : pushTile->entityList)
                    {
                        if (IsSet(e, ENTITY_FLAG_MOVABLE) &&
                            IsSet(e, ENTITY_FLAG_COLLIDES) &&
                            !IsSet(e, ENTITY_FLAG_PLAYER))
                        {
                            // NOTE: Resolve entity collision
                            MoveEntity(level, &e, dir, arena, reverse, depth - 1);
                        }
                    }
                }
            }

            if (!reverse)
            {
                result = ChangeEntityLocation(level, entity, desiredPos, arena);
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

    internal void
    DrawEntities(Level* level, GameState* gameState)
    {
        // TODO: Entity data oriented storage
        for (u32 i = 0; i < LEVEL_ENTITY_TABLE_SIZE; i++)
        {
            Entity* entity = level->entities[i];
            while (entity)
            {
                f32 xCoord = entity->coord.x * LEVEL_TILE_SIZE;
                f32 yCoord = entity->coord.z * LEVEL_TILE_SIZE;
                f32 zCoord = entity->coord.y * LEVEL_TILE_SIZE;
                v3 pos = V3(xCoord, yCoord, -zCoord);
                RenderCommandDrawMesh command = {};
                command.transform = Translation(pos);
                //SOKO_ASSERT(entity->mesh);
                //SOKO_ASSERT(entity->material);
                command.mesh = gameState->meshes + entity->mesh;
                command.material = gameState->materials + entity->material;
                RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
                                       (void*)&command);

                entity = entity->nextEntity;
            }
        }

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
        uptr bufferSize = sizeof(AB::AABLevelHeader) + sizeof(SerializedChunk) * level->loadedChunksCount;
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
    CalcLevelArenaSize(const LevelMetaInfo* info)
    {
        uptr result = 0;
        u64 levelSize = sizeof(Level) + sizeof(Chunk) * info->chunkCount;
        uptr chunkMeshDataSize = sizeof(ChunkMeshVertexBlock) * info->chunkMeshBlockCount;
        SOKO_ASSERT(levelSize <= AB::UPTR_MAX);
        // NOTE: Adding some random value just in case
        uptr safetyPad = 128 + alignof(ChunkMeshVertexBlock) * info->chunkMeshBlockCount;
        result = (uptr)levelSize + chunkMeshDataSize + safetyPad;
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
                            loadedLevel->levelArena = levelArena;
                            auto chunks = (SerializedChunk*)((byte*)fileBuffer + header.firstChunkOffset);
                            for (u32 chunkIdx = 0; chunkIdx < header.chunkCount; chunkIdx++)
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
                                    }
                                    else
                                    {
                                        goto end;
                                    }
                                }
                                else
                                {
                                    goto end;
                                }
                            }
                            SOKO_ASSERT(loadedLevel->loadedChunksCount == header.chunkCount);
                            result = loadedLevel;
                        }
                    }
                }
            }
        }
    end:
        return result;
    }

    internal bool
    GenTestLevel(AB::MemoryArena* tempArena)
    {
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
                            (y == 0) || (y == CHUNK_DIM - 1))
                        {
                            Tile* tile1 = GetTileInChunk(chunk, x, y, 1);
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
        if(SaveLevel(level, L"testLevel.aab", tempArena))
        {
            result = 1;
        }
        return result;
    }
}
