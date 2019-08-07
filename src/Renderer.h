#pragma once
#include "Platform.h"
#include "RenderGroup.h"
#include "OpenGL.h"
#include "Memory.h"

namespace soko
{
	struct LineProgram
	{
		GLuint handle;
		GLint viewProjLocation;
		GLint colorLocation;
	};

	struct MeshProgram
	{
		GLint handle;
		GLint viewProjLocation;
		GLint modelMtxLocation;
		GLint normalMtxLocation;
	};
	
	struct Renderer
	{
		LineProgram lineProgram;
		MeshProgram meshProgram;
		
		GLuint lineBufferHandle;
		v4 clearColor;
	};

	Renderer* AllocAndInitRenderer(AB::MemoryArena* arena);
	void RendererLoadMesh(Mesh* mesh);
	void RendererBeginFrame(Renderer* renderer, v2 viewportDim);
	void FlushRenderGroup(Renderer* renderer, RenderGroup* group);
}
