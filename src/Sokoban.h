#pragma once
#include "Platform.h"
#include "Renderer.h"
#include "Level.h"
#include "Camera.h"
#include "GameMenu.h"

// NOTE: For now assume that all msvc target devices are little-endian
#if defined (AB_COMPILER_MSVC)
#define SOKO_BYTE_ORDER AB_LITTLE_ENDIAN
#endif

namespace soko::net
{
    struct Server;
    struct Client;
}

namespace soko::meta
{
    struct MetaInfo;
}

struct ImFont;

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
        enum { Legacy = 0, PBR } type;
        union
        {
            struct
            {
                Texture diffMap;
                Texture specMap;
            } legacy;
            struct
            {
                Texture albedoMap;
            } pbr;
        };
    };

    struct Renderer;
    struct GameState
    {
        GameMode globalGameMode;
        GameMenu mainMenu;
        GameSession session;


        AB::MemoryArena* memoryArena;
        AB::MemoryArena* tempArena;
        Renderer* renderer;
        RenderGroup* renderGroup;
        Material materials[TypeTraits(EntityMaterial)::MemberCount];
        Mesh meshes[TypeTraits(EntityMesh)::MemberCount];
        CubeTexture skybox;
        CubeTexture irradanceMap;
        CubeTexture enviromentMap;
        CubeTexture hdrMap;
        u32 overlayCorner;
        ImFont* notoMonoFont10px;
        ImFont* notoMonoFont14px;
    };
}
