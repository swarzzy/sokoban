#pragma once
#include "Platform.h"
#include "Renderer.h"

namespace soko
{
	struct Mesh
	{
		u32 vertexCount;
		u32 normalCount;
		u32 uvCount;
		u32 indexCount;
		v3* vertices;
		v3* normals;
		v2* uvs;
		u32* indices;
		u32 gpuVertexBufferHandle;
		u32 gpuIndexBufferHandle;
	};

	struct Material
	{
		Texture diffMap;
		Texture specMap;
	};

	struct Camera
	{
		CameraConfig conf;
		v3 targetPosition;
		v3 targetFront;
		b32 cursorCaptured;
		f32 moveSpeed;
		f32 moveSmooth;
		f32 rotateSpeed;
		f32 rotateSmooth;
		f32 pitch;
		f32 yaw;
	};

	const f32 LEVEL_TILE_SIZE = 1.0f;

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


	struct Tile
	{
		u32 value;
	};

	struct Level
	{
		u32 xDim;
		u32 yDim;
		u32 zDim;
		Tile* tiles;
	};

	enum PlayerPendingMovementBit : u32
	{
		PENDING_MOVEMENT_BIT_FORWARD = 0x1,
		PENDING_MOVEMENT_BIT_BACKWARD = 0x2,
		PENDING_MOVEMENT_BIT_LEFT = 0x4,
		PENDING_MOVEMENT_BIT_RIGHT = 0x8
	};

	struct Player
	{
		Level* level;
		v3u lvlCoord;
		u32 inputFlags;
	};

	struct GameState
	{
		AB::MemoryArena* memoryArena;
		AB::MemoryArena* tempArena;
		Renderer* renderer;
		RenderGroup* renderGroup;
		Camera camera;
		Mesh cubeMesh;
		Material tileMaterial;
		u32 overlayCorner;
		Level level;
		Player player;
	};
}
