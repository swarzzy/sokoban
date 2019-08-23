#pragma once
#include "Platform.h"

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

	const f32 LEVEL_TILE_SIZE = 1.0f;

	enum TileValue
	{
		TILE_VALUE_NULL = 0,
		TILE_VALUE_WALL,
		TILE_VALUE_ENTITY,
	};

	enum EntityType
	{
		ENTITY_TYPE_BLOCK,
		ENTITY_TYPE_PLAYER,
		ENTITY_TYPE_PLATE
	};

	struct Entity
	{
		EntityType type;
		v3i coord;
		Mesh* mesh;
		Material* material;
		Entity* nextEntityInTile;
		Entity* prevEntityInTile;
	};
	
	struct Tile
	{
		// TODO: think about using i16 for less memory footprint
		v3i coord;
		TileValue value;
		Entity* firstEntity;
		// TODO: use 4bit offsets from allocator base for less footprint
		Tile* nextTile;
	};

	enum MovementDir
	{
		MOVE_DIR_FORWARD,
		MOVE_DIR_BACK,
		MOVE_DIR_RIGHT,
		MOVE_DIR_LEFT
	};

	enum class TileQueryResult
	{
		Empty = 0, OutOfBounds, Found, AllocationError
	};
	
	struct TileQuery
	{
		TileQueryResult result;
		Tile* tile;
	};


	struct Level
	{
		static const u32 MAX_ENTITIES = 128;
		static const u32 TILE_TABLE_SIZE = 8192;

		// NOTE: Maximum size of the level is 1024-tile-side cube
		// so count of tiles in cube is less than 2^32
		static const i32 MAX_DIM = 512;
		static const i32 MIN_DIM = -511;
		
		u32 xDim;
		u32 yDim;
		u32 zDim;
		// TODO: This fixed for now. Pick size based on level fill percentage?
		Tile* tiles[TILE_TABLE_SIZE];
		u32 freeTileCount;
		Tile* tileFreeList;
		u32 entityCount;
		Entity entities[MAX_ENTITIES];
		u32 tileCount;
	};
}
