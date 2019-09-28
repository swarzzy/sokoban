#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "MeshGen.h"

namespace soko
{
    union v3u
    {
        struct
        {
            u32 x, y, z;
        };
    };

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
    };

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

    inline v3i operator-(v3i v)
    {
        v3i result = V3I(-v.x, -v.y, -v.z);
        return result;
    }

    // NOTE: Should fit in one byte
    enum Direction : u32
    {
        // NOTE: Movement action values should be same
        // as in PlayerAction enum
        DIRECTION_INVALID = 0,
        DIRECTION_NORTH,
        DIRECTION_SOUTH,
        DIRECTION_WEST,
        DIRECTION_EAST,
        DIRECTION_UP,
        DIRECTION_DOWN
    };

    enum TileValue
    {
        TILE_VALUE_EMPTY = 0,
        TILE_VALUE_WALL,
    };

    enum EntityType
    {
        ENTITY_TYPE_BLOCK,
        ENTITY_TYPE_PLAYER,
        ENTITY_TYPE_PLATE,
        ENTITY_TYPE_PORTAL,
        ENTITY_TYPE_SPIKES,
        ENTITY_TYPE_BUTTON
    };

    enum EntityFlags : u32
    {
        ENTITY_FLAG_COLLIDES = (1 << 1),
        ENTITY_FLAG_MOVABLE = (1 << 2),
        ENTITY_FLAG_JUST_TELEPORTED = (1 << 3),
        ENTITY_FLAG_PLAYER = (1 << 4)
    };

    struct Level;
    struct Entity;
    typedef void(UpdateProcFn)(Level* level, Entity* entity, void* data);

    struct Entity
    {
        u32 id;
        EntityType type;
        u32 flags;
        v3i coord;
        Mesh* mesh;
        Material* material;
        u32 bindedPortalID;
        Direction portalDirection;
        void* updateProcData;
        UpdateProcFn* updateProc;

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
        v3i coord;
        TileValue value;
        TileEntityList entityList;
    };

    struct Chunk
    {
        bool loaded;
        v3i coord;
        Tile tiles[CHUNK_TILE_COUNT];
        LoadedChunkMesh loadedMesh;
        ChunkMesh mesh;
    };

    struct Level
    {
        AB::MemoryArena* levelArena;
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
