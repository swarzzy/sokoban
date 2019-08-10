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

	struct GameState
	{
		AB::MemoryArena* memoryArena;
		AB::MemoryArena* tempArena;
		Renderer* renderer;
		RenderGroup* renderGroup;
		Camera camera;
		Mesh mesh;
		Material material;
	};
}
