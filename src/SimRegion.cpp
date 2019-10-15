namespace soko
{
    struct SimEntity
    {
        u32 id;
        // NOTE: Changing this coord not affects on entity position
        // when entity is in a transition
        v3 pos;
        v3 _transitionPos;
        Entity* stored;
#if defined (SOKO_DEBUG)
        iv3 mappedTile;
#endif
    };

    constant u32 SIM_REGION_MAX_ENTITIES = 256;

    struct SimRegion
    {
        Level* level;
        WorldPos origin;
        u32 radius;
        // TODO: Pick size based on entitiesInChunk counter
        u32 entityCount;
        SimEntity entities[SIM_REGION_MAX_ENTITIES];
    };

    inline SimEntity*
    GetRegionEntityHashMapEntry(SimRegion* region, u32 id)
    {
        SOKO_STATIC_ASSERT(IsPowerOfTwo(SIM_REGION_MAX_ENTITIES));
        SimEntity* result = null;
        // TODO: Better hash
        u32 hashMask = SIM_REGION_MAX_ENTITIES - 1;
        u32 hash = id & hashMask;

        for (u32 offset = 0; offset < SIM_REGION_MAX_ENTITIES; offset++)
        {
            u32 index = (hash + offset) & hashMask;
            SimEntity* entry = region->entities + index;
            if (entry->id == id || entry->id == 0)
            {
                result = entry;
                break;
            }
        }
        return result;
    }

    inline SimEntity*
    AddEntityToRegion(SimRegion* region, Entity* e)
    {
        // NOTE: Since entity itself is a bucket of tile table
        // Double mapped entitiles also double map all entities
        SOKO_ASSERT(!(e->sim && !e->inTransition), "Multiple entity entries allowed only for transitioning entities!");
        SimEntity* result = 0;
        SimEntity* entry = GetRegionEntityHashMapEntry(region, e->id);
        if (entry)
        {
            if (!(e->sim && e->inTransition))
            {
                SOKO_ASSERT(entry->id == 0, "Cloned entities are not allowed!");
                entry->id = e->id;
                entry->stored = e;
                entry->pos = GetRelPos(region->origin, e->coord);
                if (!e->inTransition)
                {
                    WorldPos alignedPos = e->coord;
                    alignedPos.offset = {};
                    entry->_transitionPos = GetRelPos(region->origin, alignedPos);
                }
                else
                {
                    entry->_transitionPos = entry->pos;
                }

#if defined(SOKO_DEBUG)
                entry->mappedTile = e->coord.tile;
#endif
                region->entityCount++;

                e->sim = entry;
            }
            result = entry;
        }
        return result;
    }

    inline SimEntity*
    GetEntity(SimRegion* region, u32 id)
    {
        SimEntity* result = 0;
        SimEntity* entry = GetRegionEntityHashMapEntry(region, id);
        if (entry && entry->id)
        {
            result = entry;
        }
        return result;
    }

    internal SimRegion*
    BeginSim(AB::MemoryArena* frameArena, Level* level, WorldPos origin, u32 radius)
    {
        SimRegion* region = PUSH_STRUCT(frameArena, SimRegion);
        if (region)
        {
            region->level = level;
            i32 actualRadius = radius - 1;
            SOKO_ASSERT(actualRadius >= 0);
            iv3 minBound = GetChunkCoord(origin.tile) - actualRadius;
            iv3 maxBound = GetChunkCoord(origin.tile) + actualRadius;

            region->origin = origin;
            region->radius = radius;

            //
            // TODO: IMPORTANT: Optimize this!!!
            //

            for (i32 z = minBound.z; z <= maxBound.z; z++)
            {
                for (i32 y = minBound.y; y <= maxBound.y; y++)
                {
                    for (i32 x = minBound.x; x <= maxBound.x; x++)
                    {
                        Chunk* chunk = GetChunk(level, x, y, z);
                        if (chunk)
                        {
                            // TODO: @Speed: Optimize entity gathering from
                            // Maybe store entities in some additional
                            // cache-friendly structure
                            // _or_
                            // Change chunk entity map so it will be
                            // more cache-friendly to traverse

                            for (u32 headBlockindex = 0;
                                 headBlockindex < CHUNK_ENTITY_MAP_SIZE;
                                 headBlockindex++)
                            {
                                auto head = chunk->entityMap + headBlockindex;
                                for (u32 headEntityIndex = 0;
                                     headEntityIndex < head->at;
                                     headEntityIndex++)
                                {
                                    Entity* entity = head->entities[headEntityIndex];
                                    SimEntity* e = AddEntityToRegion(region, entity);
                                    SOKO_ASSERT(e);
                                }

                                auto block = head->next;
                                while (block)
                                {
                                    for (u32 entityIndex = 0;
                                         entityIndex < block->at;
                                         entityIndex++)
                                    {
                                        Entity* entity = block->entities[entityIndex];
                                        SimEntity* e = AddEntityToRegion(region, entity);
                                        SOKO_ASSERT(e);
                                    }
                                    block = block->next;
                                }
                            }
                        }
                    }
                }
            }
        }
        return region;
    }

    inline void
    EndTileTransition(SimRegion* region, SimEntity* entity)
    {
        auto tile = GetTile(region->level, entity->stored->transitionOrigin);
        UnregisterEntityInTile(region->level, entity->stored->transitionOrigin, entity->stored);
        UpdateEntitiesInTile(region->level, entity->stored->coord.tile);
    }

    // TODO: More data oriented architecture
    internal void
    UpdateTileTransition(SimRegion* region, SimEntity* e)
    {
        Entity* stored = e->stored;
        if (stored->inTransition)
        {
            v3 delta = Normalize(stored->transitionFullPath) * stored->transitionSpeed * GlobalGameDeltaTime * LEVEL_TILE_SIZE;
            e->stored->transitionTraveledPath += delta;
            e->_transitionPos += delta;

            if (LengthSq(e->stored->transitionTraveledPath) > LengthSq(stored->transitionFullPath))
            {
                v3 pathRemainder = stored->transitionFullPath - e->stored->transitionTraveledPath;
                e->_transitionPos += pathRemainder;
                stored->inTransition = false;
            }

            WorldPos newPos = GetWorldPos(region->origin, e->_transitionPos);
            e->stored->coord = newPos;

            if (!stored->inTransition)
            {
                EndTileTransition(region, e);
            }
        }
    }

    internal void
    EndSim(Level* level, SimRegion* region)
    {
        for (u32 index = 0; index < SIM_REGION_MAX_ENTITIES; index++)
        {
            SimEntity* e = region->entities + index;
            if (e->id)
            {
                Entity* stored = e->stored;
                if (stored->inTransition)
                {
                    UpdateTileTransition(region, e);
                }
                else
                {
                    iv3 oldTile = e->stored->coord.tile;
                    WorldPos newPos = GetWorldPos(region->origin, e->pos);
                    if ((oldTile != newPos.tile))
                    {
                        if (!ChangeEntityLocation(level, e->stored, &newPos))
                        {
                            // TODO: clear offset
                        }
                    }
                    else
                    {
                        e->stored->coord = newPos;
                    }
                }
                e->stored->sim = 0;
            }
        }
    }

    //
    // TODO: Clean this stuff up in the future
    //
    // NOTE: Entity transitions on region edges does not work correctly for now
    // It might be not necessary to fix that since we always do regions with
    // safe margin so player cannot go out of region
    //

    internal bool
    MoveEntity(Level* level, SimRegion* region, SimEntity* entity, Direction dir, f32 speed, bool reverse, u32 depth);

    internal bool
    PushTileNonReverse(SimRegion* region, Entity* stored, iv3 pushTilePos, Direction dir, f32 speed, u32 depth)
    {
        bool result = false;

        Level* level = region->level;
        Tile* pushTile = GetTile(level, pushTilePos);
        if (pushTile && pushTile->value != TileValue_Wall)
        {
            result = true;
            EntityMapIterator it = {};
            while (true)
            {
                Entity* e = YieldEntityIdFromTile(level, pushTilePos, &it);
                if (!e) break;

                if (!IsSet(e, EntityFlag_Movable) &&
                    IsSet(e, EntityFlag_Collides))
                {
                    result = false;
                }
            }
            if (result)
            {
                bool recursive = (bool)depth;
                if (recursive)
                {
                    EntityMapIterator it = {};
                    while (true)
                    {
                        Entity* e = YieldEntityIdFromTile(level, pushTilePos, &it);
                        if (!e) break;

                        if (e != stored &&
                            IsSet(e, EntityFlag_Movable) &&
                            IsSet(e, EntityFlag_Collides) &&
                            !IsSet(e, EntityFlag_Player))
                        {
                            // NOTE: Resolve entity collision
                            if (!e->sim)
                            {
                                AddEntityToRegion(region, e);
                            }
                            result = MoveEntity(level, region, e->sim, dir, speed, false, depth - 1);
                        }
                    }
                }
            }
        }
        return result;
    }

    internal bool
    PushTileReverse(SimRegion* region, Entity* stored, iv3 pushTilePos, Direction dir, f32 speed, u32 depth)
    {
        bool result = false;

        Level* level = region->level;
        if (!TileIsFree(level, pushTilePos, TileOccupancy_Terrain))
        {
            result = true;
        }
        else
        {
            Tile* pushTile = GetTile(level, pushTilePos);
            result = true;

            if (result)
            {
                bool recursive = (bool)depth;
                if (recursive)
                {
                    EntityMapIterator it = {};
                    while (true)
                    {
                        Entity* e = YieldEntityIdFromTile(level, pushTilePos, &it);
                        if (!e) break;

                        if (e != stored &&
                            IsSet(e, EntityFlag_Movable) &&
                            IsSet(e, EntityFlag_Collides) &&
                            !IsSet(e, EntityFlag_Player))
                        {
                            // NOTE: Resolve entity collision
                            if (!e->sim)
                            {
                                AddEntityToRegion(region, e);
                            }
                            result = MoveEntity(level, region, e->sim, dir, speed, true, depth - 1);
                        }
                    }
                }
            }
        }
        return result;
    }


    internal bool
    MoveEntity(Level* level, SimRegion* region, SimEntity* entity, Direction dir, f32 speed, bool reverse = false, u32 depth = 3)
    {
        bool pushTileFree = false;

        Entity* stored = entity->stored;
        if (stored->inTransition)
        {
            pushTileFree = true;
        }
        else
        {
            iv3 desiredPos = stored->coord.tile;
            desiredPos += DirToUnitOffset(dir);
            iv3 revDesiredPos = stored->coord.tile;
            revDesiredPos -= DirToUnitOffset(dir);

            Tile* desiredTile = GetTile(level, desiredPos);
            u32 flags = TileOccupancy_Terrain;
            if (reverse && entity->stored->type == EntityType_Player)
            {
                flags |= TileOccupancy_Entities;
            }
            if (desiredTile && TileIsFree(level, desiredPos, flags))
            {
                auto pushTilePos = reverse ? revDesiredPos : desiredPos;

                if (!reverse)
                {
                    pushTileFree = PushTileNonReverse(region, entity->stored, pushTilePos, dir, speed, depth);
                }
                else
                {
                    pushTileFree = PushTileReverse(region, entity->stored, pushTilePos, dir, speed, depth);
                }

                if (pushTileFree)
                {
                    stored->inTransition = true;
                    stored->transitionSpeed = speed;
                    stored->transitionFullPath = (v3)DirToUnitOffset(dir) * LEVEL_TILE_SIZE;
                    stored->transitionTraveledPath = {};
                    stored->transitionOrigin = stored->coord.tile;
                    stored->transitionDest = desiredPos;

                    Tile* oldTile = GetTile(level, stored->coord.tile);
                    SOKO_ASSERT(oldTile);

                    RegisterEntityInTile(level, entity->stored, desiredPos);
                }
            }
        }
        return pushTileFree;
    }

    internal void
    DrawRegion(const SimRegion* region, GameState* gameState, WorldPos camOrigin)
    {
        i32 actualRadius = region->radius - 1;
        iv3 minBound = GetChunkCoord(region->origin.tile) - actualRadius;
        iv3 maxBound = GetChunkCoord(region->origin.tile) + actualRadius;

        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH, 0);

        for (i32 z = minBound.z; z <= maxBound.z; z++)
        {
            for (i32 y = minBound.y; y <= maxBound.y; y++)
            {
                for (i32 x = minBound.x; x <= maxBound.x; x++)
                {
                    Chunk* chunk = GetChunk(region->level, x, y, z);
                    if (chunk)
                    {
                        WorldPos chunkPos = MakeWorldPos(chunk->coord * CHUNK_DIM);
                        v3 camOffset = WorldToRH(GetRelPos(camOrigin, chunkPos));
                        v3 offset = camOffset;
                        RenderCommandPushChunkMesh c = {};
                        c.offset = offset;
                        c.meshIndex = chunk->loadedMesh.gpuHandle;
                        c.quadCount = chunk->loadedMesh.quadCount;
                        RenderGroupPushCommand(gameState->renderGroup,
                                               RENDER_COMMAND_PUSH_CHUNK_MESH, (void*)&c);
                    }

                }
            }
        }

        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_END_CHUNK_MESH_BATCH, 0);

        for (u32 index = 0; index < SIM_REGION_MAX_ENTITIES; index++)
        {
            const SimEntity* e = region->entities + index;
            if (e->id)
            {
                WorldPos wp = e->stored->coord;
                if (!e->stored->inTransition)
                {
                    wp.offset = {};
                }

                v3 pos = WorldToRH(GetRelPos(camOrigin, wp));

                RenderCommandDrawMesh command = {};
                command.transform = Translation(pos);
                //SOKO_ASSERT(entity->mesh);
                //SOKO_ASSERT(entity->material);
                command.mesh = gameState->meshes + e->stored->mesh;
                command.material = gameState->materials + e->stored->material;
                RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
                                       (void*)&command);
            }
        }

    }
}
