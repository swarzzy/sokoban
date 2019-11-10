#pragma once
#include "Renderer.h"
#include "Platform.h"
#include "hypermath.h"

namespace AB
{
    struct MemoryArena;
}

namespace soko
{
    struct DirectionalLight
    {
        v3 from;
        v3 dir;
        v3 ambient;
        v3 diffuse;
        v3 specular;
    };

    enum RenderCommandType : byte
    {
        RENDER_COMMAND_DRAW_MESH = 1,
        RENDER_COMMAND_SET_DIR_LIGHT,
        RENDER_COMMAND_DRAW_LINE_BEGIN,
        RENDER_COMMAND_PUSH_LINE_VERTEX,
        RENDER_COMMAND_DRAW_LINE_END,
        RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH,
        RENDER_COMMAND_PUSH_CHUNK_MESH,
        RENDER_COMMAND_END_CHUNK_MESH_BATCH,
    };

    enum DrawMeshFlags : u32
    {
        DRAW_MESH_FLAG_HIGHLIGHT,
        DRAW_MESH_FLAG_WIREFRAME
    };

    struct Mesh;
    struct Material;
    struct Texture;

    struct RenderCommandDrawMesh
    {
        m4x4 transform;
        Mesh* mesh;
        Material material;
        u32 flags;
    };

    struct RenderCommandSetDirLight
    {
        DirectionalLight light;
    };

    enum RenderLineType
    {
        RENDER_LINE_TYPE_SEGMENTS,
        RENDER_LINE_TYPE_STRIP
    };

    struct RenderCommandDrawLineBegin
    {
        RenderLineType type;
        v3 color;
        f32 width;
    };

    struct RenderCommandPushLineVertex
    {
        v3 vertex;
    };

    struct RenderCommandPushChunkMesh
    {
        v3 offset;
        u32 meshIndex;
        u64 quadCount;
    };

    struct CommandQueueEntry
    {
        RenderCommandType type;
        u32 rbOffset;
        u32 instanceCount;
    };

    struct CameraConfig
    {
        v3 position;
        v3 front;
        f32 fovDeg;
        f32 aspectRatio;
        f32 nearPlane;
        f32 farPlane;
    };

    struct RenderGroup
    {
        CameraConfig cameraConfig;

        b32 dirLightEnabled;
        DirectionalLight dirLight;

        byte* renderBuffer;
        byte* renderBufferAt;
        u32 renderBufferSize;
        u32 renderBufferFree;

        CommandQueueEntry* pendingLineBatchCommandHeader;

        CommandQueueEntry* pendingChunkMeshBatchHeader;

        CommandQueueEntry* commandQueue;
        u32 commandQueueCapacity;
        u32 commandQueueAt;

        b32 drawSkybox;
        u32 skyboxHandle;

        u32 irradanceMapHandle;
        u32 envMapHandle;
    };

    RenderGroup* AllocateRenderGroup(AB::MemoryArena* mem,
                                     u32 rbSize,
                                     u32 queueCapacity);

    void RenderGroupPushCommand(RenderGroup* group,
                                RenderCommandType type,
                                void* command);

    void RenderGroupResetQueue(RenderGroup* group);

    void RenderGroupSetCamera(RenderGroup* group, const CameraConfig* config);

    internal void DrawAlignedBoxOutline(RenderGroup* renderGroup, v3 min, v3 max, v3 color, f32 lineWidth);

    void DrawStraightLine(RenderGroup* renderGroup, v3 begin, v3 end, v3 color, f32 lineWidth);
}
