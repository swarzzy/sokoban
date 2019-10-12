namespace soko
{
    struct SimEntity
    {
        u32 id;
        v3 pos;
        Entity* stored;
#if defined (SOKO_DEBUG)
        v3i mappedTile;
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
        SOKO_ASSERT(!(e->sim && !e->doesMovement), "Multiple entity entries allowed only for transitioning entities!");
        SimEntity* result = 0;
        SimEntity* entry = GetRegionEntityHashMapEntry(region, e->id);
        if (entry)
        {
            if (!(e->sim && e->doesMovement))
            {
                SOKO_ASSERT(entry->id == 0, "Cloned entities are not allowed!");
                entry->id = e->id;
                entry->stored = e;
                entry->pos = GetRelPos(region->origin, e->coord);
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
            v3i minBound = GetChunkCoord(origin.tile) - actualRadius;
            v3i maxBound = GetChunkCoord(origin.tile) + actualRadius;

            region->origin = origin;
            region->radius = radius;

            for (i32 z = minBound.z; z <= maxBound.z; z++)
            {
                for (i32 y = minBound.y; y <= maxBound.y; y++)
                {
                    for (i32 x = minBound.x; x <= maxBound.x; x++)
                    {
                        Chunk* chunk = GetChunk(level, x, y, z);
                        if (chunk)
                        {
                            for (i32 index = 0; index < CHUNK_MAX_ENTITIES; index++)
                            {
                                Entity* entityInTile = (chunk->entityMap + index)->ptr;
                                if (entityInTile)
                                {
                                    SimEntity* e = AddEntityToRegion(region, entityInTile);
                                    SOKO_ASSERT(e);
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
        // TODO: Calculate tile coord from sim coord
        auto tile = GetTile(region->level, entity->stored->beginTile);
        UnregisterEntityInTile(region->level, entity->stored->beginTile, entity->id);
    }


    // TODO: More data oriented architecture
    internal void
    UpdateEntity(SimRegion* region, SimEntity* e)
    {
        // TODO: We resetting entity offset now.
        // So if it has some value which was set before, it will be erased.
        // TODO: Claim both tiles at transition startup:
        // in which the entity is placed currently
        // and the tile in which entity will be moved. Free
        // old tile at the end of transition.
        // Introduce consistent notion of transition

        Entity* stored = e->stored;
        if (stored->doesMovement)
        {
            v3 delta = Normalize(stored->fullPath) * stored->currentSpeed * GlobalGameDeltaTime * LEVEL_TILE_SIZE;
            e->stored->pathTraveled += delta;
            e->pos += delta;

            if (LengthSq(e->stored->pathTraveled) > LengthSq(stored->fullPath))
            {
                v3 pathRemainder = stored->fullPath - e->stored->pathTraveled;
                e->pos += pathRemainder;
                stored->doesMovement = false;
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
                UpdateEntity(region, e);
                iv3 oldTile = e->stored->coord.tile;
                WorldPos newPos = GetWorldPos(region->origin, e->pos);
                if ((oldTile != newPos.tile) && !e->stored->doesMovement)
                {
                    if (!ChangeEntityLocation(level, e->stored, &newPos))
                    {
                        e->stored->doesMovement = false;
                        e->pos -= e->stored->pathTraveled;
                        e->stored->coord = GetWorldPos(region->origin, e->pos);
                    }
                }
                else
                {
                    e->stored->coord = newPos;
                }
                e->stored->sim = 0;
            }
        }
    }

    internal void
    MoveEntity(Level* level, SimRegion* region, SimEntity* entity, Direction dir, f32 speed, bool reverse = false, u32 depth = 2)
    {
        Entity* stored = entity->stored;
        if (!stored->doesMovement)
        {
            v3i desiredPos = stored->coord.tile;
            desiredPos += DirToUnitOffset(dir);
            v3i revDesiredPos = stored->coord.tile;
            revDesiredPos -= DirToUnitOffset(dir);

            Tile* desiredTile = GetTile(level, desiredPos);
            if (desiredTile && TileIsFree(level, desiredPos, TileOccupancy_Terrain))
            {
                // TODO: IMPORTANT: Stop using stored tile position here
                // and calculate it from sim position
                stored->doesMovement = true;
                stored->currentSpeed = speed;
                stored->movementDirection = dir;
                stored->fullPath = (v3)DirToUnitOffset(dir) * LEVEL_TILE_SIZE;
                stored->pathTraveled = {};
                stored->beginTile = stored->coord.tile;

                Tile* oldTile = GetTile(level, stored->coord.tile);
                SOKO_ASSERT(oldTile);

                RegisterEntityInTile(level, entity->stored, desiredPos);

                auto pushTilePos = reverse ? revDesiredPos : desiredPos;
                Tile* pushTile = GetTile(level, pushTilePos);

                if (pushTile && pushTile->value != TileValue_Wall)
                {
                    bool recursive = (bool)depth;
                    if (recursive)
                    {
                        ChunkEntityMapEntry* at = 0;
                        while (true)
                        {
                            Entity* e = YieldEntityIdFromTile(level, pushTilePos, &at);
                            if (!e) break;

                            if (e != stored &&
                                IsSet(*e, EntityFlag_Movable) &&
                                IsSet(*e, EntityFlag_Collides) &&
                                !IsSet(*e, EntityFlag_Player))
                            {
                                // NOTE: Resolve entity collision
                                SimEntity* sim = e->sim;
                                if (!e->sim)
                                {
                                    AddEntityToRegion(region, e);
                                }
                                MoveEntity(level, region, sim, dir, speed, reverse, depth - 1);
                            }
                        }
                    }
                }
            }
        }
    }

}
