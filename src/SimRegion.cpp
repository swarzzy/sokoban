#include "SimRegion.h"
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
        Entity* result = 0;
        if (!e->region)
        {
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
        }
        else
        {
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
        //PrintString("Begin entity gathering...\n");
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
                            for (u32 index = 0;
                                 index < ArrayCount(chunk->entityTable);
                                 index++)
                            {
                                auto block = chunk->entityTable[index];
                                while (block)
                                {
                                    for (u32 entityIndex = 0; entityIndex < block->at; entityIndex++)
                                    {
                                        Entity* entity = block->entities[entityIndex];
                                        bool added = AddEntityToRegion(region, entity);
                                        //PrintString("Entity with id: %u32 and type: %s gathered at tile with index %u32!\n", entity->id, meta::GetEnumName(entity->type), index);
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
        //PrintString("End entity gathering...\n");
        return region;
    }

    inline void
    ChangeEntityLocation(Level* level, Entity* entity, iv3 destP)
    {
        bool result = false;
        iv3 oldP = entity->pos;

        if (CheckTile(level, destP, TileCheck_Terrain | TileCheck_Entities, entity))
        {
            // TODO: Decide how to handle multi-tile entity overlaps
            UnregisterEntityInTile(level, entity);
            bool movedAtLeaving = ProcessEntityTileOverlap(level, oldP, entity, EntityOverlapType_Leaving);
            SOKO_ASSERT(!movedAtLeaving);
            entity->pos = destP;
            RegisterEntityInTile(level, entity);
            bool alreadyMoved = ProcessEntityTileOverlap(level, destP, entity, EntityOverlapType_Entering);
        }
    }

    inline bool
    BeginEntityTransition(Level* level, Entity* e, Direction dir, u32 length, f32 speed, i32 push)
    {
        bool result = false;
        if (!e->inTransition)
        {
            iv3 beginP = e->pos;
            iv3 targetP = e->pos + DirToUnitOffset(dir);

            if (push > 0)
            {
                for (u32 z = 0; z < e->footprintDim.z; z++)
                {
                    for (u32 y = 0; y < e->footprintDim.y; y++)
                    {
                        for (u32 x = 0; x < e->footprintDim.x; x++)
                        {
                            EntityMapIterator it = {};
                            while (true)
                            {
                                Entity* pe = YieldEntityFromTile(level, targetP + IV3(x, y, z), &it);
                                if (!pe) break;
                                if (pe != e)
                                {
                                    if (IsSet(pe, EntityFlag_Collides) && IsSet(pe, EntityFlag_Movable))
                                    {
                                        if (BeginEntityTransition(level, pe, dir, length, speed, push - 1))
                                        {
                                            it = {};
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (CanMove(level, targetP, e))
            {
                ChangeEntityLocation(level, e, targetP);
                e->inTransition = true;
                e->transitionCount = length;
                e->transitionPushCount = push;
                e->transitionDir = dir;
                e->transitionSpeed = speed;
                e->transitionFullPath = (v3)DirToUnitOffset(dir) * LEVEL_TILE_SIZE;
                e->transitionTraveledPath = {};
                e->transitionOrigin = beginP;
                e->transitionDest = targetP;
                e->transitionOffset = {};
                e->transitionSpeed = speed;
                result = true;
            }

            if (push < 0)
            {
                iv3 grabP = beginP - DirToUnitOffset(dir);
                for (u32 z = 0; z < e->footprintDim.z; z++)
                {
                    for (u32 y = 0; y < e->footprintDim.y; y++)
                    {
                        for (u32 x = 0; x < e->footprintDim.x; x++)
                        {

                            EntityMapIterator it = {};
                            while (true)
                            {
                                Entity* pe = YieldEntityFromTile(level, grabP + IV3(x, y, z), &it);
                                if (!pe) break;
                                if (pe != e)
                                {
                                    if (IsSet(pe, EntityFlag_Collides) && IsSet(pe, EntityFlag_Movable))
                                    {
                                        if (BeginEntityTransition(level, pe, dir, length, speed, push + 1))
                                        {
                                            it = {};
                                        }
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
                SOKO_ASSERT(e->transitionDest == e->pos);
                if (e->transitionCount)
                {
                    BeginEntityTransition(region->level, e, e->transitionDir, e->transitionCount, e->transitionSpeed, e->transitionPushCount);
                }
            }
        }
    }

    internal void
    UpdateRegion(SimRegion* region)
    {
        for (u32 index = 0; index < SIM_REGION_MAX_ENTITIES; index++)
        {
            Entity* e = region->entities[index];
            if (e)
            {
                UpdateEntity(region->level, e);
            }
        }
    }

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
    // NOTE: Entity transitions on region edges does not work correctly for now
    // It might be not necessary to fix that since we always do regions with
    // safe margin so player cannot go out of region
    //
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
                    wp = GetRelPos(camOrigin, MakeWorldPos(e->pos));
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
                                        const Tile* tile = GetTilePointerInChunkInternal(chunk, x, y, z);
                                        SOKO_ASSERT(tile);
                                        // TODO: TileIsFree?
                                        if (TileIsTerrain(tile))
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
                v3 simPos = GetRelPos(region->origin, entity->pos) + entity->offset;
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
                    result.entity.tile = entity->pos;
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
