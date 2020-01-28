#include "SimRegion.h"
namespace soko
{
    internal SimRegion
    BeginSim(MemoryArena* frameArena, Level* level, WorldPos origin, u32 radius)
    {
        SimRegion region = {};
        region.level = level;
        i32 actualRadius = radius - 1;
        SOKO_ASSERT(actualRadius >= 0);
        iv3 minBound = GetChunkCoord(origin.tile) - actualRadius;
        iv3 maxBound = GetChunkCoord(origin.tile) + actualRadius;

        i32 regionSpan = 1 + actualRadius * 2;
        u32 fullChunkCount = regionSpan * regionSpan * regionSpan;

        // NOTE: If some chunks in regions are not exist then not all allocated memory will be used
        region.chunks = (Chunk**)PUSH_SIZE(frameArena, sizeof(Chunk**) * fullChunkCount);
        SOKO_ASSERT(region.chunks);

        for (i32 z = minBound.z; z <= maxBound.z; z++)
        {
            for (i32 y = minBound.y; y <= maxBound.y; y++)
            {
                for (i32 x = minBound.x; x <= maxBound.x; x++)
                {
                    Chunk* chunk = GetChunk(level, IV3(x, y, z));
                    if (chunk)
                    {
                        region.chunks[region.chunkCount] = chunk;
                        region.chunkCount++;
                    }
                }
            }
        }

        region.origin = origin;
        region.radius = radius;

        return region;
    }

    internal void
    UpdateRegion(SimRegion* region)
    {
        for (u32 chunkIndex = 0; chunkIndex < region->chunkCount; chunkIndex++)
        {
            auto chunk = region->chunks[chunkIndex];
            foreach (chunk->entityArray)
            {
                UpdateEntity(region->level, &it);
                if (it.inTransition)
                {
                    UpdateEntityTransition(region->level, &it);
                }
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

        for (u32 chunkIndex = 0; chunkIndex < region->chunkCount; chunkIndex++)
        {
            Chunk* chunk = region->chunks[chunkIndex];
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

        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_END_CHUNK_MESH_BATCH, 0);

        for (u32 chunkIndex = 0; chunkIndex < region->chunkCount; chunkIndex++)
        {
            auto chunk = region->chunks[chunkIndex];

            foreach (chunk->entityArray)
            {
                v3 wp;
                if (it.inTransition)
                {
                    wp = GetRelPos(camOrigin, it.transitionOrigin) + it.transitionOffset;
                }
                else
                {
                    wp = GetRelPos(camOrigin, MakeWorldPos(it.pos));
                }
                v3 pos = WorldToRH(wp);

                RenderCommandDrawMesh command = {};
                command.transform = Translation(pos);
                //SOKO_ASSERT(entity->mesh);
                //SOKO_ASSERT(entity->material);
                command.mesh = gameState->meshes + it.mesh;
                auto material = gameState->materials + it.material;
                if (material->type == Material::PBR && material->pbr.isCustom)
                {
                    Material m = {};
                    m.type = Material::PBR;
                    m.pbr.isCustom = true;
                    m.pbr.custom.albedo = it.materialAlbedo;
                    m.pbr.custom.roughness = it.materialRoughness;
                    m.pbr.custom.metalness = it.materialMetallic;

                    command.material = m;
                }
                else
                {
                    command.material = *material;
                }

                RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH, (void*)&command);
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
        for (u32 chunkIndex = 0; chunkIndex < region->chunkCount; chunkIndex++)
        {
            Chunk* chunk = region->chunks[chunkIndex];

            v3 chunkSimPos = GetRelPos(region->origin, chunk->coord * CHUNK_DIM);
            BBoxAligned chunkBBox = GetChunkAABB(chunkSimPos);
            v3 tileOff = V3(LEVEL_TILE_RADIUS, LEVEL_TILE_RADIUS, LEVEL_TILE_RADIUS);
            // NOTE: Subtract tile radius because tile (0, 0, 0) coord is aligned to tile center in camera space
            chunkBBox.min -= tileOff;
            chunkBBox.max -= tileOff;
            auto chunkIntersection = RayAABBIntersectionSlow(from, ray, &chunkBBox);

            if (chunkIntersection.intersects)
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
                                iv3 worldPos = WorldTileFromChunkTile(chunk->coord, {x, y, z});
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

        for (u32 chunkIndex = 0; chunkIndex < region->chunkCount; chunkIndex++)
        {
            auto chunk = region->chunks[chunkIndex];

            v3 chunkSimPos = GetRelPos(region->origin, chunk->coord * CHUNK_DIM);
            BBoxAligned chunkBBox = GetChunkAABB(chunkSimPos);
            v3 tileOff = V3(LEVEL_TILE_RADIUS, LEVEL_TILE_RADIUS, LEVEL_TILE_RADIUS);
            // NOTE: Subtract tile radius because tile (0, 0, 0) coord is aligned to tile center in camera space
            chunkBBox.min -= tileOff;
            chunkBBox.max -= tileOff;
            auto chunkIntersection = RayAABBIntersectionSlow(from, ray, &chunkBBox);

            if (chunkIntersection.intersects)
            {
                foreach (chunk->entityArray)
                {
                    v3 simPos = GetRelPos(region->origin, it.pos) + it.offset;
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
                        result.entity.id = it.id;
                        // TODO: Is using stored position during the simulation allowed?
                        result.entity.tile = it.pos;
                    }
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
