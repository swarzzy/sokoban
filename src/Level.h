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
		ENTITY_TYPE_PLAYER
	};

	struct Entity
	{
		EntityType type;
		v3i coord;
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

	const u32 MAX_LEVEL_ENTITIES = 128;
	const u32 LEVEL_TILE_TABLE_SIZE = 8192;

	enum MovementDir
	{
		MOVE_DIR_FORWARD,
		MOVE_DIR_BACK,
		MOVE_DIR_RIGHT,
		MOVE_DIR_LEFT
	};

	struct Level
	{
		u32 xDim;
		u32 yDim;
		u32 zDim;
		// TODO: This fixed for now. Pick size based on level fill percentage?
		Tile* tiles[LEVEL_TILE_TABLE_SIZE];
		u32 entityCount;
		Entity entities[MAX_LEVEL_ENTITIES];
		u32 tileCount;
	};
}
