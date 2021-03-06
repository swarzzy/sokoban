#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "MeshGen.h"
#include "Chunk.h"

#include "FreeList.h"

namespace soko
{

    // TODO: Temporary stuff while we haven't an asset system

    enum [reflect seq_enum] EntityMesh
    {
        EntityMesh_Cube = 0,
        EntityMesh_Plate,
        EntityMesh_Portal,
        EntityMesh_Spikes,
        EntityMesh_Button,
        EntityMesh_Sphere,
//        EntityMesh_Gun,
        EntityMesh_Box,
        EntityMesh_Altar,
        EntityMesh_Cat,
            EntityMesh_Plane,
            EntityMesh_PrevSphere
    };

    enum [reflect seq_enum] EntityMaterial
    {
        EntityMaterial_Tile = 0,
        EntityMaterial_Player,
        EntityMaterial_Block,
        EntityMaterial_RedPlate,
        EntityMaterial_Portal,
        EntityMaterial_Spikes,
        EntityMaterial_Button,
        EntityMaterial_BRDFCustom,
//        EntityMaterial_PbrMetal,
//        EntityMaterial_Rock,
        EntityMaterial_OldMetal,
//        EntityMaterial_Gun,
        EntityMaterial_Box,
        EntityMaterial_Altar,
        EntityMaterial_Cat
    };

    // NOTE: Should fit in one byte
    enum [reflect seq_enum] Direction : u32
    {
        // NOTE: Movement action values should be same
        // as in PlayerAction enum
        Direction_Invalid = 0,
        Direction_North,
        Direction_South,
        Direction_West,
        Direction_East,
        Direction_Up,
        Direction_Down
    };

    struct Level;
    struct Entity;
    typedef void(UpdateProcFn)(Level* level, Entity* entity, void* data);

    struct GameSession;
    struct Level
    {
        u64 guid;
        GameSession* session;
        AB::MemoryArena* sessionArena;
        u32 globalChunkMeshBlockCount;

        b32 initialized;

        u32 loadedChunksCount;
        // TODO: @Robustness Store an array of loaded chunks
        // for fast traversing
        // TODO: @Speed store key data inside table buckets
        Chunk* chunkTable[LEVEL_CHUNK_TABLE_SIZE];

        u32 entitySerialNumber;
        u32 entityCount;
        FreeList<Entity> entityFreeList;
        // NOTE: This is chained hash table
        Entity* entities[LEVEL_ENTITY_TABLE_SIZE];
        b32 platePressed;

        iv3 playerSpawnPos;

        u32 platformCount;
        u32 completePlatformCount;

        FreeList<ChunkMeshVertexBlock> mesherFreeList;
        FreeList<ChunkEntityBlock> freeChunkEntityBlocks;
        FreeList<EntityArrayBlock> chunkEntityArrayBlockFreeList;
    };

    struct LevelMetaInfo
    {
        u64 guid;
        u32 chunkCount;
        u32 chunkMeshBlockCount;
    };
}
