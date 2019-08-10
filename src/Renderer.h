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
		u32 diffMapSampler;
		u32 specMapSampler;
		GLenum diffMapSlot;
		GLenum specMapSlot;
		GLint viewPosLocation;
		GLint viewProjLocation;
		GLint modelMtxLocation;
		GLint normalMtxLocation;
		GLint dirLightDirLoc;
		GLint dirLightAmbLoc;
		GLint dirLightDiffLoc;
		GLint dirLightSpecLoc;
		GLint diffMapLocation;
		GLint specMapLocation;
	};
	
	struct Renderer
	{
		LineProgram lineProgram;
		MeshProgram meshProgram;
		
		GLuint lineBufferHandle;
		v4 clearColor;
	};

	struct Texture
	{
		GLuint gpuHandle;
		GLenum format;
		u32 width;
		u32 height;
		void* data;
	};

	Renderer* AllocAndInitRenderer(AB::MemoryArena* arena);
	void RendererLoadMesh(Mesh* mesh);
	void RendererLoadTexture(Texture* texture);
	void RendererBeginFrame(Renderer* renderer, v2 viewportDim);
	void FlushRenderGroup(Renderer* renderer, RenderGroup* group);
}
