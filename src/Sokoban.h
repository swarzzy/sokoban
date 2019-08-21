#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "Level.h"

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

	struct FPCamera
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

	struct GameCamera
	{
		CameraConfig conf;
		f32 longitude;
		f32 latitude;
		f32 distance;
		v2 targetOrbit;
		f32 targetDistance;
		v3 targetPos;
		f32 rotSpeed;
		f32 zoomSpeed;
		f32 moveSpeed;
		f32 moveFriction;
		v2 velocity;
		f32 latSmooth;
		f32 longSmooth;
		f32 distSmooth;
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
		Entity* e;
		Level* level;
		u32 inputFlags;
	};

	struct GameState
	{
		AB::MemoryArena* memoryArena;
		AB::MemoryArena* tempArena;
		Renderer* renderer;
		RenderGroup* renderGroup;
		FPCamera debugCamera;
		GameCamera camera;
		b32 useDebugCamera;
		Mesh cubeMesh;
		Material tileMaterial;
		Material tilePlayerMaterial;
		Material tileBlockMaterial;
		u32 overlayCorner;
		Level level;
		Player player;
	};
}
