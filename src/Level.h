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
//        EntityMesh_Gun,
        EntityMesh_Box,
        EntityMesh_Altar,
        EntityMesh_Cat
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

    enum [reflect seq_enum] TileValue : u8
    {
        TileValue_Empty = 0,
        TileValue_Wall,
        TileValue_Stone,
        TileValue_Grass
    };

    struct Level;
    struct Entity;
    typedef void(UpdateProcFn)(Level* level, Entity* entity, void* data);

    struct ChunkPos
    {
        iv3 chunk;
        uv3 tile;
    };

    struct Tile
    {
        TileValue value;
    };

    struct ChunkEntityBlock
    {
        u32 at;
        Entity* entities[8];
        ChunkEntityBlock* next;
    };

    struct Chunk
    {
        Level* level;
        b32 dirty;
        iv3 coord;
        u32 filledTileCount;
        Tile tiles[CHUNK_TILE_COUNT];
        LoadedChunkMesh loadedMesh;
        ChunkMesh mesh;
        ChunkEntityBlock* entityTable[CHUNK_TILE_COUNT];

        Chunk* nextInHash;
        // TODO: Acelleration structure for traversing entities
        // in chunk sequentially
    };

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
        u32 deletedEntityCount;
        Entity* entityFreeList;
        Entity* entities[LEVEL_ENTITY_TABLE_SIZE];
        b32 platePressed;

        iv3 firstPlayerSpawnPos;
        b32 hasSecondPlayer;
        iv3 secondPlayerSpawnPos;


        u32 platformCount;
        u32 completePlatformCount;

        u32 chunkEntityBlockCount;
        ChunkEntityBlock* freeChunkEntityBlocks;
    };

    struct LevelMetaInfo
    {
        u64 guid;
        b32 supportsMultiplayer;
        u32 chunkCount;
        u32 chunkMeshBlockCount;
    };
}
