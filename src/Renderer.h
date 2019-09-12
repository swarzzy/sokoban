#pragma once
#include "Platform.h"
#include "RenderGroup.h"
#include "OpenGL.h"
#include "Memory.h"
#include "MeshGen.h"
#include "Level.h"

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

    struct ChunkProgram
    {
        GLint handle;
        u32 atlasSampler;
        GLenum atlasSlot;
        GLint viewPosLocation;
        GLint viewProjLocation;
        GLint modelMtxLocation;
        GLint normalMtxLocation;
        GLint dirLightDirLoc;
        GLint dirLightAmbLoc;
        GLint dirLightDiffLoc;
        GLint dirLightSpecLoc;
        GLint terrainAtlasLoc;
    };


    struct Renderer
    {
        const_val u32 MAX_CHUNK_QUADS = Chunk::DIM * Chunk::DIM * Chunk::DIM * 4 / 2;
        const_val u32 INDICES_PER_CHUNK_QUAD = 6;
        LineProgram lineProgram;
        MeshProgram meshProgram;
        ChunkProgram chunkProgram;

        GLuint lineBufferHandle;
        GLuint chunkIndexBuffer;
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
    u32 RendererLoadChunkMesh(ChunkMesh* mesh);
    void RendererLoadTexture(Texture* texture);
    void RendererBeginFrame(Renderer* renderer, v2 viewportDim);
    void FlushRenderGroup(Renderer* renderer, RenderGroup* group);
}
