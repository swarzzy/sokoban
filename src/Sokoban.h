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

	struct GameState
	{
		Renderer* renderer;
		RenderGroup* renderGroup;
		Mesh mesh;
	};
}
