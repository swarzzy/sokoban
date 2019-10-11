namespace soko
{
    struct SimEntity
    {
        u32 id;
        v3 pos;
        Entity* stored;
    };

    constant u32 SIM_REGION_MAX_ENTITIES = 256;

    struct SimRegion
    {
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
        SOKO_ASSERT(!e->sim, "Multiple sim regions for entities not allowed for now!");
        SimEntity* result = 0;
        SimEntity* entry = GetRegionEntityHashMapEntry(region, e->id);
        if (entry)
        {
            SOKO_ASSERT(entry->id == 0, "Cloned entities are not allowed!");
            entry->id = e->id;
            entry->stored = e;
            entry->pos = GetRelPos(region->origin, e->coord);
            region->entityCount++;

            e->sim = entry;

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
                            for (u32 tileIndex = 0; tileIndex < CHUNK_TILE_COUNT; tileIndex++)
                            {
                                Tile* tile = chunk->tiles + tileIndex;
                                for (Entity& entityInTile : tile->entityList)
                                {
                                    SimEntity* e = AddEntityToRegion(region, &entityInTile);
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
                if (oldTile != newPos.tile)
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
            stored->doesMovement = true;
            stored->currentSpeed = speed;
            stored->movementDirection = dir;
            stored->fullPath = (v3)DirToUnitOffset(dir) * LEVEL_TILE_SIZE;
            stored->pathTraveled = {};

            v3i desiredPos = stored->coord.tile;
            desiredPos += DirToUnitOffset(dir);
            v3i revDesiredPos = stored->coord.tile;
            revDesiredPos -= DirToUnitOffset(dir);

            Tile* desiredTile = GetTile(level, desiredPos);
            Tile* oldTile = GetTile(level, stored->coord.tile);
            Tile* pushTile = GetTile(level, reverse ? revDesiredPos : desiredPos);

            SOKO_ASSERT(oldTile);
            if (desiredTile)
            {
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
                                SimEntity* sim = e.sim;
                                if (!e.sim)
                                {
                                    AddEntityToRegion(region, &e);
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
