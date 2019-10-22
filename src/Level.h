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
        _EntityMesh_Count,
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
        _EntityMaterial_Count
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
        Direction_Down,
        _Direction_Count
    };

    enum [reflect seq_enum] TileValue : u8
    {
        TileValue_TileNotExist = 0,
        TileValue_Empty,
        TileValue_Wall,
        TileValue_Stone,
        TileValue_Grass,
        _TileValue_Count
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
        // TODO: think about using i16 for less memory footprint
        // TODO: Store array of tile coords for debugging
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
        bool loaded;
        iv3 coord;
        Tile tiles[CHUNK_TILE_COUNT];
        LoadedChunkMesh loadedMesh;
        ChunkMesh mesh;
        ChunkEntityMapResidentBlock entityMap[CHUNK_ENTITY_MAP_SIZE];
        // TODO: Acelleration structure for traversing entities
        // in chunk sequentially
    };

    struct Level
    {
        AB::MemoryArena* sessionArena;
        u32 globalChunkMeshBlockCount;

        // NOTE: Maximum size of the level is 1024-tile-side cube
        // so count of tiles in cube is less than 2^32

        u32 loadedChunksCount;
        // TODO: @Robustness Store linked list of loaded chunks
        // for fast traversing
        u32 chunkTableSize;
        Chunk* chunkTable;

        // TODO: Use 64bit IDs for entities
        u32 entitySerialNumber;
        u32 entityCount;
        u32 deletedEntityCount;
        Entity* entityFreeList;
        Entity* entities[LEVEL_ENTITY_TABLE_SIZE];
        b32 platePressed;

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
