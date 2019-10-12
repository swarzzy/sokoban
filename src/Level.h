#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "MeshGen.h"

namespace soko
{

    // TODO: Temporary stuff while we haven't an asset system

    enum EntityMesh
    {
        EntityMesh_Cube = 0,
        EntityMesh_Plate,
        EntityMesh_Portal,
        EntityMesh_Spikes,
        EntityMesh_Button,
        _EntityMesh_Count,
    };

    enum EntityMaterial
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

    union v3u
    {
        struct
        {
            u32 x, y, z;
        };
    };
    typedef v3u uv3;

    inline v3u V3U(u32 x, u32 y, u32 z)
    {
        v3u result;
        result.x = x;
        result.y = y;
        result.z = z;
        return result;
    }

    inline v3u V3U(u32 a)
    {
        v3u result;
        result.x = a;
        result.y = a;
        result.z = a;
        return result;
    }

    inline v3u& operator+=(v3u& l, v3u r)
    {
        l.x += r.x;
        l.y += r.y;
        l.z += r.z;
        return l;
    }

    inline v3u& operator-=(v3u& l, v3u r)
    {
        l.x -= r.x;
        l.y -= r.y;
        l.z -= r.z;
        return l;
    }

    inline v3u operator+(v3u l, v3u r)
    {
        return V3U(l.x + r.x, l.y + r.y, l.z + r.z);
    }

    union v3i
    {
        struct
        {
            i32 x, y, z;
        };
        explicit operator v3() { return V3((f32)x, (f32)y, (f32)z); }
    };
    // TODO: Switch to iv3
    typedef v3i iv3;


    inline bool operator==(const v3i& a, const v3i& b)
    {
        bool result = (a.x == b.x && a.y == b.y && a.z == b.z);
        return result;
    }

    inline bool operator!=(const v3i& a, const v3i& b)
    {
        bool result = !(a == b);
        return result;
    }

    inline v3i V3I(i32 x, i32 y, i32 z)
    {
        v3i result;
        result.x = x;
        result.y = y;
        result.z = z;
        return result;
    }

    inline v3i V3I(i32 a)
    {
        v3i result;
        result.x = a;
        result.y = a;
        result.z = a;
        return result;
    }

    inline v3i& operator+=(v3i& l, v3i r)
    {
        l.x += r.x;
        l.y += r.y;
        l.z += r.z;
        return l;
    }

    inline v3i& operator-=(v3i& l, v3i r)
    {
        l.x -= r.x;
        l.y -= r.y;
        l.z -= r.z;
        return l;
    }

    inline v3i operator+(v3i l, v3i r)
    {
        return V3I(l.x + r.x, l.y + r.y, l.z + r.z);
    }

    inline v3i operator+(v3i l, i32 r)
    {
        return V3I(l.x + r, l.y + r, l.z + r);
    }

    inline v3i operator-(v3i l, v3i r)
    {
        return V3I(l.x - r.x, l.y - r.y, l.z - r.z);
    }

    inline v3i operator-(v3i l, i32 r)
    {
        return V3I(l.x - r, l.y - r, l.z - r);
    }

    inline v3i operator-(v3i v)
    {
        v3i result = V3I(-v.x, -v.y, -v.z);
        return result;
    }

    inline v3i operator*(v3i l, i32 s)
    {
        return V3I(l.x * s, l.y * s, l.z * s);
    }


    // NOTE: Should fit in one byte
    enum Direction : u32
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

    enum TileValue
    {
        TileValue_Empty = 0,
        TileValue_Wall,
    };

    enum EntityType
    {
        EntityType_Block,
        EntityType_Player,
        EntityType_Plate,
        EntityType_Portal,
        EntityType_Spikes,
        EntityType_Button
    };

    enum EntityFlags : u32
    {
        EntityFlag_Collides = (1 << 1),
        EntityFlag_Movable = (1 << 2),
        EntityFlag_JustTeleported = (1 << 3),
        EntityFlag_Player = (1 << 4)
    };

    struct Level;
    struct Entity;
    typedef void(UpdateProcFn)(Level* level, Entity* entity, void* data);

    struct WorldPos
    {
        v3i tile;
        v3 offset;
    };

    inline WorldPos MakeWorldPos(v3i tile) { return {tile, {}}; }
    inline WorldPos MakeWorldPos(i32 x, i32 y, i32 z) { return {{x, y, z}, {}}; }

    struct SimEntity;
    struct Entity
    {
        u32 id;
        SimEntity* sim;
        EntityType type;
        u32 flags;
        WorldPos coord;
        EntityMesh mesh;
        EntityMaterial material;

        u32 bindedPortalID;
        Direction portalDirection;
        // TODO: Entity custom behavior
        //void* updateProcData;
        //UpdateProcFn* updateProc;

        b32 doesMovement;
        Direction movementDirection;
        b32 didTileTransition;
        v3 fullPath;
        v3 pathTraveled;
        f32 currentSpeed;
        iv3 beginTile;

        f32 movementSpeed;

        Entity* nextEntityInTile;
        Entity* prevEntityInTile;
        Entity* nextEntity;
    };

    struct TileEntityList
    {
        Entity* first;
    };

    struct TileEntityListIterator
    {
        Entity* ptr;

        inline TileEntityListIterator& operator++()
        {
            ptr = ptr->nextEntityInTile;
            return *this;
        }
        bool operator!=(TileEntityListIterator const& other) const
        {
            return ptr != other.ptr;
        }
        Entity& operator*()
        {
            return *ptr;
        }
    };

    struct ConstTileEntityListIterator
    {
        const Entity* ptr;

        inline ConstTileEntityListIterator& operator++()
        {
            ptr = ptr->nextEntityInTile;
            return *this;
        }
        bool operator!=(ConstTileEntityListIterator const& other) const
        {
            return ptr != other.ptr;
        }
        const Entity& operator*() const
        {
            return *ptr;
        }
    };


    inline TileEntityListIterator begin(TileEntityList& list)
    {
        TileEntityListIterator iter = {};
        iter.ptr = list.first;
        return iter;
    }

    inline TileEntityListIterator end(TileEntityList& list)
    {
        TileEntityListIterator iter = {};
        return iter;
    }

    inline ConstTileEntityListIterator begin(const TileEntityList& list)
    {
        ConstTileEntityListIterator iter = {};
        iter.ptr = list.first;
        return iter;
    }

    inline ConstTileEntityListIterator end(const TileEntityList& list)
    {
        ConstTileEntityListIterator iter = {};
        return iter;
    }


    struct Tile
    {
        // TODO: think about using i16 for less memory footprint
#if defined(SOKO_DEBUG)
        v3i coord;
#endif
        TileValue value;
        //TileEntityList entityList;
    };


    constant u32 CHUNK_ENTITY_MAP_SIZE = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

    constant u32 CHUNK_ENTITY_MAP_BLOCK_SIZE = 16;
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
        bool loaded;
        v3i coord;
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


    inline bool IsSet(const Entity* entity, u32 flag)
    {
        return entity->flags & flag;
    }

    inline bool IsSet(const Entity& entity, u32 flag)
    {
        return entity.flags & flag;
    }

    inline void SetFlag(Entity* entity, u32 flag)
    {
        entity->flags |= flag;
    }

    inline void SetFlag(Entity& entity, u32 flag)
    {
        entity.flags |= flag;
    }


    inline void UnsetFlag(Entity& entity, u32 flag)
    {
        entity.flags &= (~flag);
    }

}
