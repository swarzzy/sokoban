#pragma once
#include "Platform.h"
#include "RenderGroup.h"
#include "OpenGL.h"
#include "Memory.h"

namespace soko
{
	struct LineProgram
	{
		GLint handle;
		GLint viewProjLocation;
		GLint colorLocation;
	};
	
	struct Renderer
	{
		LineProgram lineProgram;
		GLuint lineBufferHandle;
		v4 clearColor;
	};

	Renderer* AllocAndInitRenderer(AB::MemoryArena* arena);
	void RendererBeginFrame(Renderer* renderer, v2 viewportDim);
	void FlushRenderGroup(Renderer* renderer, RenderGroup* group);
}
