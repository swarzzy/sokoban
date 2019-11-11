#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "MeshGen.h"

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
        EntityMesh_PreviewSphere,
        EntityMesh_Gun
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
        EntityMaterial_PbrMetal,
        EntityMaterial_Rock,
        EntityMaterial_Metal,
        EntityMaterial_OldMetal,
        EntityMaterial_Burlap,
        EntityMaterial_Gold,
        EntityMaterial_Gun
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

    enum [reflect seq_enum] TileValue : u8
    {
        TileValue_TileNotExist = 0,
        TileValue_Empty,
        TileValue_Wall,
        TileValue_Stone,
        TileValue_Grass
    };

    struct Level;
    struct Entity;
    typedef void(UpdateProcFn)(Level* level, Entity* entity, void* data);

    struct WorldPos
    {
        iv3 tile;
        v3 offset;
    };

    inline WorldPos MakeWorldPos(iv3 tile) { return {tile, {}}; }
    inline WorldPos MakeWorldPos(i32 x, i32 y, i32 z) { return {{x, y, z}, {}}; }

    struct Tile
    {
        TileValue value;
    };

    constant u32 CHUNK_ENTITY_MAP_SIZE = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

    // NOTE: 14 perfetly fits in cache line (128 bit)
    constant u32 CHUNK_ENTITY_MAP_BLOCK_SIZE = 14;
    struct ChunkEntityMapBlock
    {
        ChunkEntityMapBlock* next;
        u32 at;
        Entity* entities[CHUNK_ENTITY_MAP_BLOCK_SIZE];
    };

    constant u32 CHUNK_ENTITY_MAP_RESIDENT_BLOCK_SIZE = 2;
    struct ChunkEntityMapResidentBlock
    {
        ChunkEntityMapBlock* next;
        u32 at;
        Entity* entities[CHUNK_ENTITY_MAP_RESIDENT_BLOCK_SIZE];
    };

    struct Chunk
    {
        Level* level;
        bool dirty;
        iv3 coord;
        Tile tiles[CHUNK_TILE_COUNT];
        LoadedChunkMesh loadedMesh;
        ChunkMesh mesh;
        ChunkEntityMapResidentBlock entityMap[CHUNK_ENTITY_MAP_SIZE];

        Chunk* nextInHash;
        // TODO: Acelleration structure for traversing entities
        // in chunk sequentially
    };


    struct Level
    {
        AB::MemoryArena* sessionArena;
        u32 globalChunkMeshBlockCount;

        u32 loadedChunksCount;
        // TODO: @Robustness Store linked list of loaded chunks
        // for fast traversing
        // TODO: @Speed store key data inside table buckets
        Chunk* chunkTable[LEVEL_CHUNK_TABLE_SIZE];

        u32 entitySerialNumber;
        u32 entityCount;
        u32 deletedEntityCount;
        Entity* entityFreeList;
        Entity* entities[LEVEL_ENTITY_TABLE_SIZE];
        b32 platePressed;

        u32 spawnerID;

        u32 chunkEntityMapBlockCount;
        u32 freeChunkEntityMapBlockCount;
        ChunkEntityMapBlock* freeChunkEntityMapBlocks;
    };

    struct LevelMetaInfo
    {
        u32 chunkCount;
        u32 chunkMeshBlockCount;
    };
}
