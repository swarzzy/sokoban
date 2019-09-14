#pragma once
#include "Platform.h"
#include "RenderGroup.h"
#include "OpenGL.h"
#include "Memory.h"
#include "MeshGen.h"
//#include "Level.h"

namespace soko
{
    struct LoadedChunkMesh
    {
        u32 gpuHandle;
        u64 quadCount;
    };

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

    enum TerrainTexture
    {
        TerrainTexture_Null = 0,
        TerrainTexture_Block,
    };

    // TODO: IMPORTANT: move this to other file
    static constexpr u32 CHUNK_DIM = 32;

    struct Renderer
    {
        const_val u32 TILE_TEX_DIM = 256;
        const_val u32 TERRAIN_TEX_ARRAY_SIZE = 32;
        const_val u32 MAX_CHUNK_QUADS = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM * 4 / 2;
        const_val u32 INDICES_PER_CHUNK_QUAD = 6;
        LineProgram lineProgram;
        MeshProgram meshProgram;
        ChunkProgram chunkProgram;

        GLuint tileTexArrayHandle;

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

    Renderer* AllocAndInitRenderer(AB::MemoryArena* arena, AB::MemoryArena* tempArena);
    void RendererLoadMesh(Mesh* mesh);
    LoadedChunkMesh RendererLoadChunkMesh(ChunkMesh* mesh);
    void RendererLoadTexture(Texture* texture);
    void RendererBeginFrame(Renderer* renderer, v2 viewportDim);
    void FlushRenderGroup(Renderer* renderer, RenderGroup* group);
}
