namespace soko
{
    constant u32 SIM_REGION_MAX_ENTITIES = 256;

    struct SimRegion
    {
        // TODO: Store a little array of pointers to chunk in sim region
        // And traverse chunks from that array, not by using loops and
        // GetChunk()

        Level* level;
        WorldPos origin;
        u32 radius;
        // TODO: Pick size based on entitiesInChunk counter
        u32 entityCount;
        Entity* entities[SIM_REGION_MAX_ENTITIES];
    };

    enum RaycastFlags : u32
    {
        Raycast_Tilemap = (1 << 0),
        Raycast_Entities = (1 << 1),
    };

    struct RaycastResult
    {
        enum { None = 0, Entity, Tile, Chunk } hit;
        f32 tMin;
        union
        {
            struct
            {
                iv3 coord;
                v3 normal;
                Direction normalDir;
            } tile;
            struct
            {
                i32 id;
                v3 normal;
                iv3 tile;
            } entity;
        };
    };

    inline Entity**
    GetRegionEntityHashMapEntry(SimRegion* region, u32 id)
    {
        SOKO_STATIC_ASSERT(IsPowerOfTwo(SIM_REGION_MAX_ENTITIES));
        Entity** result = 0;
        // TODO: Better hash
        u32 hashMask = SIM_REGION_MAX_ENTITIES - 1;
        u32 hash = id & hashMask;

        for (u32 offset = 0; offset < SIM_REGION_MAX_ENTITIES; offset++)
        {
            u32 index = (hash + offset) & hashMask;
            Entity* entry = region->entities[index];
            if ((entry && entry->id == id) || !entry)
            {
                result = region->entities + index;
                break;
            }
        }
        return result;
    }

    inline Entity*
    AddEntityToRegion(SimRegion* region, Entity* e)
    {
        // NOTE: Since entity itself is a bucket of tile table
        // Double mapped entitiles also double map all entities
        SOKO_ASSERT(!(e->region && !e->inTransition), "Multiple entity entries allowed only for transitioning entities!");
        Entity* result = 0;
        Entity** entry = GetRegionEntityHashMapEntry(region, e->id);
        if (entry)
        {
            if (!(e->region && e->inTransition))
            {
                //SOKO_ASSERT((*entry)->id == 0, "Cloned entities are not allowed!");
                *entry = e;
                e->region = region;
                region->entityCount++;
            }
            result = e;
        }
        return result;
    }

    inline Entity*
    GetEntity(SimRegion* region, u32 id)
    {
        Entity* result = 0;
        Entity** entry = GetRegionEntityHashMapEntry(region, id);
        if (entry && *entry && (*entry)->id)
        {
            result = *entry;
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
                                    bool added = AddEntityToRegion(region, entity);
                                    SOKO_ASSERT(added);
                                }

                                auto block = head->next;
                                while (block)
                                {
                                    for (u32 entityIndex = 0;
                                         entityIndex < block->at;
                                         entityIndex++)
                                    {
                                        Entity* entity = block->entities[entityIndex];
                                        bool added = AddEntityToRegion(region, entity);
                                        SOKO_ASSERT(added);
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

    inline bool
    ChangeEntityLocation(SimRegion* region, Entity* entity, iv3 desiredCoord)
    {
        bool result = false;
        iv3 oldCoord = entity->coord.tile;

        Tile desiredTile = GetTile(region->level, desiredCoord);
        if (IsTileFree(region->level, desiredCoord))
        {
#if 0
            bool allowedToLeave = ProcessEntityTileOverlap(region, oldCoord, 0);
            //SOKO_ASSERT(allowedToLeave);
            bool allowedToPlace = ProcessEntityTileOverlap(region, desiredCoord, _entity);
            if (allowedToPlace)
                // {
#endif
                UnregisterEntityInTile(region->level, entity);
            entity->coord = MakeWorldPos(desiredCoord);
            bool registered = RegisterEntityInTile(region->level, entity);
            SOKO_ASSERT(registered);
            result = true;
        }
        return result;
    }

    inline bool
    BeginEntityTransition(SimRegion* region, Entity* e, Direction dir, u32 length, f32 speed, i32 push)
    {
        bool result = false;
        if (!e->inTransition)
        {
            iv3 beginP = e->coord.tile;
            iv3 targetP = e->coord.tile + DirToUnitOffset(dir);

            if (push > 0)
            {
                EntityMapIterator it = {};
                while (true)
                {
                    Entity* pe = YieldEntityFromTile(region->level, targetP, &it);
                    if (!pe) break;
                    SOKO_ASSERT(pe != e);
                    if (IsSet(pe, EntityFlag_Collides) && IsSet(pe, EntityFlag_Movable))
                    {
                        BeginEntityTransition(region, pe, dir, length, speed, push - 1);
                    }
                }
            }

            if (CanMove(region->level, targetP))
            {
                if (ChangeEntityLocation(region, e, targetP))
                {
                    e->inTransition = true;
                    e->transitionCount = length;
                    e->transitionPushCount = push;
                    e->transitionDir = dir;
                    e->transitionSpeed = speed;
                    e->transitionFullPath = (v3)DirToUnitOffset(dir) * LEVEL_TILE_SIZE;
                    e->transitionTraveledPath = {};
                    e->transitionOrigin = beginP;
                    e->transitionDest = targetP;
                    //e->transitionOffset = {};
                    e->transitionSpeed = speed;
                    result = true;
                }
            }

            if (push < 0)
            {
                iv3 grabP = beginP - DirToUnitOffset(dir);
                EntityMapIterator it = {};
                while (true)
                {
                    Entity* pe = YieldEntityFromTile(region->level, grabP, &it);
                    if (!pe) break;
                    SOKO_ASSERT(pe != e);
                    if (IsSet(pe, EntityFlag_Collides) && IsSet(pe, EntityFlag_Movable))
                    {
                        BeginEntityTransition(region, pe, dir, length, speed, push + 1);
                    }
                }
            }

        }
        return result;
    }

    inline void
    UpdateEntityTransition(SimRegion* region, Entity* e)
    {
        if (e->inTransition)
        {
            v3 delta = Normalize(e->transitionFullPath) * e->transitionSpeed * GlobalGameDeltaTime * LEVEL_TILE_SIZE;
            e->transitionTraveledPath += delta;
            e->transitionOffset += delta;

            if (LengthSq(e->transitionTraveledPath) > LengthSq(e->transitionFullPath))
            {
                v3 pathRemainder = e->transitionFullPath - e->transitionTraveledPath;
                e->transitionOffset += pathRemainder;
                e->inTransition = false;

                if (e->transitionCount) e->transitionCount--;
            }

            if (!e->inTransition)
            {
                SOKO_ASSERT(e->transitionDest == e->coord.tile);
                if (e->transitionCount)
                {
                    BeginEntityTransition(region, e, e->transitionDir, e->transitionCount, e->transitionSpeed, e->transitionPushCount);
                }
            }
        }
    }

# if 0
    inline void
    EndTileTransition(SimRegion* region, Entity* entity)
    {
        auto tile = GetTile(region->level, entity->stored->transitionOrigin);
        UnregisterEntityInTile(region->level, entity->stored->transitionOrigin, entity->stored);
        ProcessEntityTileOverlap(region, entity->stored->coord.tile, entity);
    }

    // TODO: More data oriented architecture
    internal void
    UpdateTileTransition(SimRegion* region, Entity* e)
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
    UpdateSim(SimRegion* region)
    {
        for (u32 index = 0; index < SIM_REGION_MAX_ENTITIES; index++)
        {
            Entity* e = region->entities + index;
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
                        if (!ChangeEntityLocation(region, e, &newPos))
                        {
                            // TODO: clear offset
                        }
                    }
                    else
                    {
                        e->stored->coord = newPos;
                    }
                }
            }
        }
    }
#endif
    internal void
    EndSim(Level* level, SimRegion* region)
    {
        for (u32 index = 0; index < SIM_REGION_MAX_ENTITIES; index++)
        {
            Entity* e = region->entities[index];
            if (e)
            {
                if (e->inTransition)
                {
                    UpdateEntityTransition(region, e);
                }
                e->region = 0;
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
#if 0
    internal bool
    MoveEntity(Level* level, SimRegion* region, Entity* entity, Direction dir, f32 speed, bool reverse, u32 depth);

    internal bool
    PushTileNonReverse(SimRegion* region, Entity* stored, iv3 pushTilePos, Direction dir, f32 speed, u32 depth)
    {
        bool result = false;

        Level* level = region->level;
        Tile pushTile = GetTile(level, pushTilePos);
        // TODO: Consistent way of checking "is tile empty"
        if (pushTile.value == TileValue_Empty)
        {
            result = true;
            EntityMapIterator it = {};
            while (true)
            {
                Entity* e = YieldEntityFromTile(level, pushTilePos, &it);
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
                        Entity* e = YieldEntityFromTile(level, pushTilePos, &it);
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
        if (TileIsTerrain(level, pushTilePos))
        {
            result = true;
        }
        else
        {
            Tile pushTile = GetTile(level, pushTilePos);
            result = true;

            if (result)
            {
                bool recursive = (bool)depth;
                if (recursive)
                {
                    EntityMapIterator it = {};
                    while (true)
                    {
                        Entity* e = YieldEntityFromTile(level, pushTilePos, &it);
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
    MoveEntity(Level* level, SimRegion* region, Entity* entity, Direction dir, f32 speed, bool reverse = false, u32 depth = 3)
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

            Tile desiredTile = GetTile(level, desiredPos);
            Tile desiredGroundTile = GetTile(level, desiredPos - IV3(0, 0, 1));

            u32 flags = TileOccupancy_Terrain;
            if (reverse && entity->stored->type == EntityType_Player)
            {
                flags |= TileOccupancy_Entities;
            }
            if (IsTileFree(level, desiredPos, flags) && TileIsTerrain(desiredGroundTile))
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

                    Tile oldTile = GetTile(level, stored->coord.tile);
                    SOKO_ASSERT(oldTile.value);

                    pushTileFree = pushTileFree && ChangeEntityLocation(region, entity, &MakeWorldPos(desiredPos), true);
                }
            }
        }
        return pushTileFree;
    }
#endif
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
            Entity* e = region->entities[index];
            if (e)
            {
                v3 wp;
                if (e->inTransition)
                {
                    wp = GetRelPos(camOrigin, e->transitionOrigin) + e->transitionOffset;
                }
                else
                {
                    wp = GetRelPos(camOrigin, MakeWorldPos(e->coord.tile));
                }
                v3 pos = WorldToRH(wp);

                RenderCommandDrawMesh command = {};
                command.transform = Translation(pos);
                //SOKO_ASSERT(entity->mesh);
                //SOKO_ASSERT(entity->material);
                command.mesh = gameState->meshes + e->mesh;
                auto material = gameState->materials + e->material;
                if (material->type == Material::PBR && material->pbr.isCustom)
                {
                    Material m = {};
                    m.type = Material::PBR;
                    m.pbr.isCustom = true;
                    m.pbr.custom.albedo = e->materialAlbedo;
                    m.pbr.custom.roughness = e->materialRoughness;
                    m.pbr.custom.metalness = e->materialMetallic;

                    command.material = m;
                }
                else
                {
                    command.material = *material;
                }

                RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
                                       (void*)&command);
            }
        }

    }

    struct IntersectionResult
    {
        b32 intersects;
        f32 tMin;
        v3 normal;
        Direction normalDir;
    };

    // TODO: Move this to hypermath.h
    IntersectionResult
    RayAABBIntersectionSlow(v3 from, v3 ray, const BBoxAligned* aabb)
    {
        IntersectionResult result = {};
        //result.intersects = false;

        f32 tMin = 0.0f;
        bool hit = false;
        v3 normal = V3(0.0f);
        Direction normalDir = Direction_Invalid;

        if (AbsF32(ray.x) > FLOAT_EPS)
        {
            f32 t = (aabb->min.x - from.x) / ray.x;
            if (t >= 0.0f)
            {
                f32 yMin = from.y + ray.y * t;
                f32 zMin = from.z + ray.z * t;
                if (yMin >= aabb->min.y && yMin <= aabb->max.y &&
                    zMin >= aabb->min.z && zMin <= aabb->max.z)
                {
                    if (!hit || t < tMin)
                    {
                        tMin = t;
                        normal = V3(-1.0f, 0.0f, 0.0f);
                        normalDir = Direction_West;
                    }
                    hit = true;
                }
            }
        }

        if (AbsF32(ray.x) > FLOAT_EPS)
        {
            f32 t = (aabb->max.x - from.x) / ray.x;
            if (t >= 0.0f)
            {
                f32 yMax = from.y + ray.y * t;
                f32 zMax = from.z + ray.z * t;
                if (yMax >= aabb->min.y && yMax <= aabb->max.y &&
                    zMax >= aabb->min.z && zMax <= aabb->max.z)
                {
                    if (!hit || t < tMin)
                    {
                        tMin = t;
                        normal = V3(1.0f, 0.0f, 0.0f);
                        normalDir = Direction_East;
                    }

                    hit = true;
                }
            }
        }

        if (AbsF32(ray.y) > FLOAT_EPS)
        {
            f32 t = (aabb->min.y - from.y) / ray.y;
            if (t >= 0.0f)
            {
                f32 xMin = from.x + ray.x * t;
                f32 zMin = from.z + ray.z * t;
                if (xMin >= aabb->min.x && xMin <= aabb->max.x &&
                    zMin >= aabb->min.z && zMin <= aabb->max.z)
                {
                    if (!hit || t < tMin)
                    {
                        tMin = t;
                        normal = V3(0.0f, -1.0f, 0.0f);
                        normalDir = Direction_South;
                    }

                    hit = true;
                }
            }
        }

        if (AbsF32(ray.y) > FLOAT_EPS)
        {
            f32 t = (aabb->max.y - from.y) / ray.y;
            if (t >= 0.0f)
            {
                f32 xMax = from.x + ray.x * t;
                f32 zMax = from.z + ray.z * t;
                if (xMax >= aabb->min.x && xMax <= aabb->max.x &&
                    zMax >= aabb->min.z && zMax <= aabb->max.z)
                {
                    if (!hit || t < tMin)
                    {
                        tMin = t;
                        normal = V3(0.0f, 1.0f, 0.0f);
                        normalDir = Direction_North;
                    }

                    hit = true;
                }
            }
        }

        if (AbsF32(ray.z) > FLOAT_EPS)
        {
            f32 t = (aabb->min.z - from.z) / ray.z;
            if (t >= 0.0f)
            {
                f32 xMin = from.x + ray.x * t;
                f32 yMin = from.y + ray.y * t;
                if (xMin >= aabb->min.x && xMin <= aabb->max.x &&
                    yMin >= aabb->min.y && yMin <= aabb->max.y)
                {
                    if (!hit || t < tMin)
                    {
                        tMin = t;
                        normal = V3(0.0f, 0.0f, -1.0f);
                        normalDir = Direction_Down;
                    }

                    hit = true;
                }
            }
        }

        if (AbsF32(ray.z) > FLOAT_EPS)
        {
            f32 t = (aabb->max.z - from.z) / ray.z;
            if (t >= 0.0f)
            {
                f32 xMax = from.x + ray.x * t;
                f32 yMax = from.y + ray.y * t;
                if (xMax >= aabb->min.x && xMax <= aabb->max.x &&
                    yMax >= aabb->min.y && yMax <= aabb->max.y)
                {
                    if (!hit || t < tMin)
                    {
                        tMin = t;
                        normal = V3(0.0f, 0.0f, 1.0f);
                        normalDir = Direction_Up;
                    }

                    hit = true;
                }
            }
        }

        result.intersects = hit;
        result.tMin = tMin;
        result.normal = normal;
        result.normalDir = normalDir;

        return result;
    }

    inline BBoxAligned
    GetChunkAABB(v3 offset)
    {
        BBoxAligned result;
        v3 dim = V3(CHUNK_DIM * LEVEL_TILE_SIZE);
        result.min = offset;
        result.max = offset + dim;
        return result;
    }

    internal RaycastResult
    RaycastTilemap(SimRegion* region, v3 from, v3 ray)
    {
        RaycastResult result = {};

        i32 actualRadius = region->radius - 1;
        iv3 minBound = GetChunkCoord(region->origin.tile) - actualRadius;
        iv3 maxBound = GetChunkCoord(region->origin.tile) + actualRadius;

        IntersectionResult tileResult = {};
        iv3 tileResultWorldPos = {};

        tileResult.tMin = F32_MAX;

        // TODO: Store a little array of pointers to chunk in sim region
        // And traverse chunks from that array, not by using loops and
        // GetChunk()
        for (i32 chunkZ = minBound.z; chunkZ <= maxBound.z; chunkZ++)
        {
            for (i32 chunkY = minBound.y; chunkY <= maxBound.y; chunkY++)
            {
                for (i32 chunkX = minBound.x; chunkX <= maxBound.x; chunkX++)
                {
                    v3 chunkSimPos = GetRelPos(region->origin, IV3(chunkX, chunkY, chunkZ) * CHUNK_DIM);
                    BBoxAligned chunkBBox = GetChunkAABB(chunkSimPos);
                    v3 tileOff = V3(LEVEL_TILE_RADIUS, LEVEL_TILE_RADIUS, LEVEL_TILE_RADIUS);
                    // NOTE: Subtract tile radius because tile (0, 0, 0) coord is aligned to tile center in camera space
                    chunkBBox.min -= tileOff;
                    chunkBBox.max -= tileOff;
                    auto chunkIntersection = RayAABBIntersectionSlow(from, ray, &chunkBBox);

                    if (chunkIntersection.intersects)
                    {
                        Chunk* chunk = GetChunk(region->level, chunkX, chunkY, chunkZ);

                        // TODO: Chunk query may be faster than ray-aabb intersection
                        // So maybe query chunk before intersection test
                        // and then if chunk exists do the test
                        if (chunk)
                        {
                            for (u32 z = 0; z < CHUNK_DIM; z++)
                            {
                                for (u32 y = 0; y < CHUNK_DIM; y++)
                                {
                                    for (u32 x = 0; x < CHUNK_DIM; x++)
                                    {
                                        // TODO: Grids
                                        // NOTE: Internal is faster
                                        Tile* tile = GetTilePointerInChunkInternal(chunk, x, y, z);
                                        SOKO_ASSERT(tile);
                                        // TODO: TileIsFree?
                                        if (TileIsTerrain(*tile))
                                        {
                                            iv3 worldPos = WorldTileFromChunkTile({chunkX, chunkY, chunkZ}, {x, y, z});
                                            v3 simPos = GetRelPos(region->origin, worldPos);
                                            BBoxAligned aabb;
                                            aabb.min = simPos - LEVEL_TILE_RADIUS;
                                            aabb.max = simPos + LEVEL_TILE_RADIUS;
                                            auto intersection = RayAABBIntersectionSlow(from, ray, &aabb);
                                            if (intersection.intersects &&
                                                intersection.tMin < tileResult.tMin)
                                            {
                                                tileResult = intersection;
                                                tileResultWorldPos = worldPos;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (tileResult.intersects)
        {
            result.hit = RaycastResult::Tile;
            result.tMin = tileResult.tMin;
            result.tile.coord = tileResultWorldPos;
            result.tile.normal = tileResult.normal;
            result.tile.normalDir = tileResult.normalDir;
        }

        return result;
    }

    internal RaycastResult
    RaycastEntities(SimRegion* region, v3 from, v3 ray)
    {
        RaycastResult result = {};

        result.tMin = F32_MAX;

        for (u32 index = 0; index < SIM_REGION_MAX_ENTITIES; index++)
        {
            Entity* entity = region->entities[index];
            if (entity)
            {
                v3 simPos = GetRelPos(region->origin, entity->coord);
                BBoxAligned aabb;
                aabb.min = simPos - LEVEL_TILE_RADIUS;
                aabb.max = simPos + LEVEL_TILE_RADIUS;
                auto intersection = RayAABBIntersectionSlow(from, ray, &aabb);
                if (intersection.intersects &&
                    intersection.tMin < result.tMin)
                {
                    result.hit = RaycastResult::Entity;
                    result.tMin = intersection.tMin;
                    result.entity.normal = intersection.normal;
                    result.entity.id = entity->id;
                    // TODO: Is using stored position during the simulation allowed?
                    result.entity.tile = entity->coord.tile;
                }
            }
        }
        return result;
    }

    // NOTE: from is the origin of ray's basis
    // So if camera ray passed, then it should be
    // region relative camera position
    inline RaycastResult
    Raycast(SimRegion* region, v3 from, v3 ray, u32 flags)
    {
        RaycastResult result = {};
        result.tMin = F32_MAX;
        if (flags & Raycast_Tilemap)
        {
            result = RaycastTilemap(region, from, ray);
        }
        if (flags & Raycast_Entities)
        {
            auto raycast = RaycastEntities(region, from, ray);
            if (raycast.tMin < result.tMin)
            {
                result = raycast;
            }
        }
        return result;
    }

}
