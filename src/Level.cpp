#include "Level.h"
#include "Memory.h"

namespace soko
{
    inline v3i DirToUnitOffset(Direction dir)
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

    inline Chunk*
    GetChunk(Level* level, i32 x, i32 y, i32 z, AB::MemoryArena* arena = 0)
    {
        Chunk* result = 0;

        SOKO_ASSERT(x >= Level::MIN_DIM_CHUNKS && x <= Level::MAX_DIM_CHUNKS);
        SOKO_ASSERT(y >= Level::MIN_DIM_CHUNKS && y <= Level::MAX_DIM_CHUNKS);
        SOKO_ASSERT(z >= Level::MIN_DIM_CHUNKS && z <= Level::MAX_DIM_CHUNKS);

        u32 xIndex = (u32)(x + Level::MAX_DIM_CHUNKS - 1);
        u32 yIndex = (u32)(y + Level::MAX_DIM_CHUNKS - 1);
        u32 zIndex = (u32)(z + Level::MAX_DIM_CHUNKS - 1);

        u32 chunkTableIndex =
            zIndex * Level::FULL_DIM_CHUNKS * Level::FULL_DIM_CHUNKS +
            yIndex * Level::FULL_DIM_CHUNKS +
            xIndex;

        if (chunkTableIndex <= (Level::MAX_DIM * 2) * (Level::MAX_DIM * 2) * (Level::MAX_DIM * 2))
        {
            result = level->chunkTable[chunkTableIndex];
            if (!result && arena)
            {
                result = PUSH_STRUCT(arena, Chunk);
                if (result)
                {
                    // TODO: Do I need to store this?
                    result->coord = V3I(xIndex, yIndex, zIndex);
                    level->chunkTable[chunkTableIndex] = result;
                    level->chunkCount++;

                    for (u32 tileZ = 0; tileZ < Chunk::DIM; tileZ++)
                    {
                        for (u32 tileY = 0; tileY < Chunk::DIM; tileY++)
                        {
                            for (u32 tileX = 0; tileX < Chunk::DIM; tileX++)
                            {
                                u32 tileIndex = tileZ * Chunk::DIM * Chunk::DIM + tileY * Chunk::DIM + tileX;
                                Tile* tile = result->tiles + tileIndex;
                                tile->coord = V3I(x * Chunk::DIM + tileX, y * Chunk::DIM + tileY, z * Chunk::DIM + tileZ);
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

    inline static Tile*
    GetTileInChunkInternal(Chunk* chunk, u32 x, u32 y, u32 z)
    {
        // TODO: Actial check?
        SOKO_ASSERT(x < Chunk::DIM);
        SOKO_ASSERT(y < Chunk::DIM);
        SOKO_ASSERT(z < Chunk::DIM);

        Tile* tile = 0;
        tile = chunk->tiles + (z * Chunk::DIM * Chunk::DIM + y * Chunk::DIM + x);

        return tile;
    }

    inline Tile*
    GetTileInChunk(Chunk* chunk, u32 x, u32 y, u32 z)
    {
        Tile* result = 0;
        if (x < Chunk::DIM &&
            y < Chunk::DIM &&
            z < Chunk::DIM)
        {
            result = GetTileInChunkInternal(chunk, x, y, z);
        }
        return result;
    }

    Tile*
    GetTile(Level* level, i32 x, i32 y, i32 z, AB::MemoryArena* arena = 0)
    {
        Tile* result = 0;
        // TODO: Real check instead of asserts
        SOKO_ASSERT(x >= Level::MIN_DIM && x <= Level::MAX_DIM);
        SOKO_ASSERT(y >= Level::MIN_DIM && y <= Level::MAX_DIM);
        SOKO_ASSERT(z >= Level::MIN_DIM && z <= Level::MAX_DIM);

        u32 chunkX = x >> Chunk::BIT_SHIFT;
        u32 chunkY = y >> Chunk::BIT_SHIFT;
        u32 chunkZ = z >> Chunk::BIT_SHIFT;

        u32 tileX = x & Chunk::BIT_MASK;
        u32 tileY = y & Chunk::BIT_MASK;
        u32 tileZ = z & Chunk::BIT_MASK;

        Chunk* chunk = GetChunk(level, chunkX, chunkY, chunkZ, arena);

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
    inline bool TileIsFree(const Tile* tile)
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

    #if 0
    inline TileQuery
    GetTile(Level* level, i32 x, i32 y, i32 z, AB::MemoryArena* arena = nullptr)
    {
        TileQuery result = {};
        if (x > Level::MAX_DIM || x < Level::MIN_DIM ||
            y > Level::MAX_DIM || y < Level::MIN_DIM ||
            z > Level::MAX_DIM || z < Level::MIN_DIM)
        {
            result.result = TileQuery::OutOfBounds;
        }
        else
        {
            // TODO: Better hash
            u32 tileHash = (Abs(x) * 22 + Abs(y) * 12 + Abs(z) * 7) % Level::TILE_TABLE_SIZE;
            Tile* tile = level->tiles[tileHash];
            if (tile)
            {
                // TODO: Collapse to do-while
                if (tile->coord.x == x &&
                    tile->coord.y == y &&
                    tile->coord.z == z)
                {
                    result.result = TileQuery::Found;
                    result.tile = tile;
                }
                else
                {
                    while (tile->nextTile)
                    {
                        Tile* nextTile = tile->nextTile;
                        if (nextTile->coord.x == x &&
                            nextTile->coord.y == y &&
                            nextTile->coord.z == z)
                        {
                            result.result = TileQuery::Found;
                            result.tile = nextTile;
                            break;
                        }
                        else
                        {
                            tile = tile->nextTile;
                        }
                    }
                }
            }
            // NOTE: Allocation call always should be called with arena
            // Even if it takes space from freelist
            if (!result.tile && arena)
            {
                Tile* newTile = nullptr;
                if (level->tileFreeList)
                {
                    newTile = level->tileFreeList;
                    level->tileFreeList = newTile->nextTile;
                    level->freeTileCount--;
                    ZERO_STRUCT(Tile, newTile);
                }
                else
                {
                    newTile = PUSH_STRUCT(arena, Tile);
                }

                if (!newTile)
                {
                    result.result = TileQuery::AllocationError;
                }
                else
                {
                    if (!tile)
                    {
                        level->tiles[tileHash] = newTile;
                    }
                    else
                    {
                        tile->nextTile = newTile;
                    }

                    newTile->coord.x = x;
                    newTile->coord.y = y;
                    newTile->coord.z = z;
                    level->tileCount++;

                    result.result = TileQuery::Found;
                    result.tile = newTile;

                }
            }
        }
        return result;
    }

    inline TileQuery
    GetTile(Level* level, v3i coord, AB::MemoryArena* arena = NULL)
    {
        TileQuery result = GetTile(level, coord.x, coord.y, coord.z, arena);
        return result;
    }

    void
    FreeTileIfEmpty(Level* level, Tile* tile)
    {
        if (tile->value == TILE_VALUE_EMPTY && !tile->entityList.first)
        {
            i32 x = tile->coord.x;
            i32 y = tile->coord.y;
            i32 z = tile->coord.z;
            // TODO: This is the same code as in GetTile
            // TODO: Better hash
            u32 tileHash = (Abs(x) * 22 + Abs(y) * 12 + Abs(z) * 7) % Level::TILE_TABLE_SIZE;
            Tile* currentTile = level->tiles[tileHash];
            bool found = false;
            if (currentTile)
            {
                // TODO: Collapse to do-while
                if (currentTile->coord.x == x &&
                    currentTile->coord.y == y &&
                    currentTile->coord.z == z)
                {
                    level->tiles[tileHash] = currentTile->nextTile;
                    found = true;
                }
                else
                {
                    while (currentTile->nextTile)
                    {
                        Tile* nextTile = currentTile->nextTile;
                        if (nextTile->coord.x == x &&
                            nextTile->coord.y == y &&
                            nextTile->coord.z == z)
                        {
                            currentTile->nextTile = tile->nextTile;
                            found = true;
                            break;
                        }
                        else
                        {
                            currentTile = currentTile->nextTile;
                        }
                    }
                }

            }
            SOKO_ASSERT(found);
            if (found)
            {
                tile->nextTile = level->tileFreeList;
                level->tileFreeList = tile;
                level->freeTileCount++;
                level->tileCount--;
            }
        }
    }
#endif

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

    void
    DeleteEntity(Level* level, Entity* entity)
    {
        u32 entityHash = entity->id % Level::ENTITY_TABLE_SIZE;
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

    inline u32
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
                u32 entityHash = entityId % Level::ENTITY_TABLE_SIZE;
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
              Mesh* mesh, Material* material, AB::MemoryArena* arena)
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
        u32 entityHash = id % Level::ENTITY_TABLE_SIZE;
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

    bool
    ChangeEntityLocation(Level* level, Entity* entity, v3i desiredCoord, AB::MemoryArena* arena);

    void UpdateEntitiesInTile(Level* level, Tile* tile, AB::MemoryArena* arena)
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
                for (Entity& e : tile->entityList)
                {
                    if (e.type == ENTITY_TYPE_BLOCK || e.type == ENTITY_TYPE_PLAYER)
                    {
                        entity.updateProc(level, &entity, entity.updateProcData);
                        break;
                    }
                }
            }
            default: {} break;
            }
        }
    }

    bool
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

    bool
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

    void
    DrawLevel(Level* level, GameState* gameState)
    {
        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH, 0);
        // TODO: Sparseness
        for (u32 chunkIndex = 0; chunkIndex < Level::CHUNK_TABLE_SIZE; chunkIndex++)
        {
            Chunk* chunk = level->chunkTable[chunkIndex];
            if (chunk)
            {
                f32 chunkSize = Level::TILE_SIZE * Chunk::DIM;
                // TODO: FIX That subtraction
                v3 chunkCoord = V3((f32)chunk->coord.x, (f32)chunk->coord.z, (f32)chunk->coord.y) - V3(Level::MAX_DIM_CHUNKS) + 1;
                //chunkCoord.z *= -1.0f;
                v3 offset = Hadamard(chunkCoord, V3(chunkSize));
                RenderCommandPushChunkMesh c = {};
                c.offset = offset;
                c.meshIndex = chunk->mesh.gpuHandle;
                c.quadCount = chunk->mesh.quadCount;;
                RenderGroupPushCommand(gameState->renderGroup,
                                       RENDER_COMMAND_PUSH_CHUNK_MESH, (void*)&c);
            }
        }
        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_END_CHUNK_MESH_BATCH, 0);
    }

    void
    DrawEntities(Level* level, GameState* gameState)
    {
        // TODO: Entity data oriented storage
        for (u32 i = 0; i < Level::ENTITY_TABLE_SIZE; i++)
        {
            Entity* entity = level->entities[i];
            while (entity)
            {
                f32 xCoord = entity->coord.x * Level::TILE_SIZE;
                f32 yCoord = entity->coord.z * Level::TILE_SIZE;
                f32 zCoord = entity->coord.y * Level::TILE_SIZE;
                v3 pos = V3(xCoord, yCoord, -zCoord);
                RenderCommandDrawMesh command = {};
                command.transform = Translation(pos);
                SOKO_ASSERT(entity->mesh);
                SOKO_ASSERT(entity->material);
                command.mesh = entity->mesh;
                command.material = entity->material;
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
        SerializedTile tiles[Chunk::DIM * Chunk::DIM * Chunk::DIM];
    };
#pragma pack(pop)

    bool
    SaveLevel(const Level* level, const wchar_t* filename, AB::MemoryArena* arena)
    {
        bool result = false;
        uptr bufferSize = sizeof(AB::AABLevelHeader) + sizeof(SerializedChunk) * level->chunkCount;
        byte* buffer = (byte*)PUSH_SIZE(arena, bufferSize);
        if (buffer)
        {
            auto header = (AB::AABLevelHeader*)buffer;
            header->magicValue = AB::AAB_FILE_MAGIC_VALUE;
            header->version = AB::AAB_FILE_VERSION;
            // TODO: Is that should be size of data placed after header?
            header->assetSize = bufferSize - sizeof(AB::AABLevelHeader);
            header->assetType = AB::AAB_FILE_TYPE_LEVEL;
            header->chunkCount = level->chunkCount;
            header->firstChunkOffset = sizeof(AB::AABLevelHeader);

            auto chunks = (SerializedChunk*)(buffer + header->firstChunkOffset);

            u32 chunksWritten = 0;
            for (u32 i = 0; i < Level::CHUNK_TABLE_SIZE; i++)
            {
                Chunk* chunk = level->chunkTable[i];
                if (chunk)
                {
                    chunksWritten++;
                    SOKO_ASSERT(chunksWritten <= header->chunkCount);
                    chunks[chunksWritten - 1].coord = chunk->coord;
                    for (u32 tileInd = 0;
                         tileInd < (Chunk::DIM * Chunk::DIM * Chunk::DIM);
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

    Level*
    LoadLevel(const wchar_t* filename, AB::MemoryArena* arenaForLevel, AB::MemoryArena* tempArena)
    {
        Level* result = 0;
        u32 fileSize = DebugGetFileSize(filename);
        if (fileSize)
        {
            byte* buffer = (byte*)PUSH_SIZE(tempArena, fileSize);
            u32 bytesRead = DebugReadFile(buffer, fileSize, filename);
            if (bytesRead == fileSize)
            {
                auto header = (AB::AABLevelHeader*)buffer;
                if (header->magicValue == AB::AAB_FILE_MAGIC_VALUE &&
                    header->version == AB::AAB_FILE_VERSION &&
                    header->assetType == AB::AAB_FILE_TYPE_LEVEL)
                {
                    Level* loadedLevel = PUSH_STRUCT(arenaForLevel, Level);
                    if (loadedLevel)
                    {
                        //loadedLevel->chunkCount = header->chunkCount;
                        auto chunks = (SerializedChunk*)(buffer + header->firstChunkOffset);
                        for (u32 chunkIdx = 0; chunkIdx < header->chunkCount; chunkIdx++)
                        {
                            SerializedChunk* sChunk = chunks + chunkIdx;
                            Chunk* newChunk = PUSH_STRUCT(arenaForLevel, Chunk);
                            if (newChunk)
                            {
                                newChunk->coord = sChunk->coord;
                                u32 chunkIdx =
                                    sChunk->coord.z * Level::FULL_DIM_CHUNKS * Level::FULL_DIM_CHUNKS +
                                    sChunk->coord.y * Level::FULL_DIM_CHUNKS +
                                    sChunk->coord.x;

                                loadedLevel->chunkTable[chunkIdx] = newChunk;
                                loadedLevel->chunkCount++;
                                for (u32 z = 0; z < Chunk::DIM; z++)
                                {
                                    for (u32 y = 0; y < Chunk::DIM; y++)
                                    {
                                        for (u32 x = 0; x < Chunk::DIM; x++)
                                        {
                                            u32 tileIdx =
                                                z * Chunk::DIM * Chunk::DIM +
                                                y * Chunk::DIM +
                                                x;

                                            newChunk->tiles[tileIdx].coord = V3I(x, y, z);
                                            newChunk->tiles[tileIdx].value = sChunk->tiles[tileIdx].value;
                                        }
                                    }
                                }
                                ChunkMesh mesh = GenChunkMesh(newChunk, tempArena);
                                LoadedChunkMesh loadedMesh = RendererLoadChunkMesh(&mesh);
                                newChunk->mesh = loadedMesh;
                            }
                            else
                            {
                                char buff[512];
                                auto strConvRes = wcstombs(buff, filename, 512);
                                if (strConvRes != (std::size_t)(-1))
                                {
                                    SOKO_WARN("Failed to allocate chunk (x: %i32, y: %i32, z: %i32) while loading level from file: %s",
                                              sChunk->coord.x, sChunk->coord.y, sChunk->coord.z, buff);
                                }
                                else
                                {
                                    SOKO_WARN("Failed to allocate chunk (x: %i32, y: %i32, z: %i32) while loading level from file: <filename print error>",
                                              sChunk->coord.x, sChunk->coord.y, sChunk->coord.z);
                                }

                            }
                        }
                        SOKO_ASSERT(loadedLevel->chunkCount == header->chunkCount);
                        result = loadedLevel;
                    }
                }

            }
        }
        return result;
    }
}
