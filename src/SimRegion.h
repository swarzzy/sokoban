#pragma once
namespace soko
{
    struct SimRegion
    {
        Level* level;
        WorldPos origin;
        u32 radius;

        u32 chunkCount;
        Chunk** chunks;
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

    inline bool BeginEntityTransition(Level* level, Entity* e, Direction dir, u32 length, f32 speed, i32 push);
}
