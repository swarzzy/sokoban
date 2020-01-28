#define HYPERMATH_IMPL
#include "hypermath.h"
#undef HYPERMATH_IMPL
#include "Constants.h"
#include "Memory.h"
#include "Platform.h"

using namespace AB;

namespace soko
{
    namespace meta
    {
        struct MetaInfo;
    }

    struct GameState;

    struct StaticStorage
    {
        GameState* gameState;
        meta::MetaInfo* metaInfo;
    };

    static AB::PlatformState* _GlobalPlatform;
    static StaticStorage* _GlobalStaticStorage;

#define GlobalMetaInfo (*(const meta::MetaInfo*)(_GlobalStaticStorage->metaInfo))

    void GameInit(AB::MemoryArena* arena,  AB::PlatformState* platform);
    void GameReload(AB::MemoryArena* arena,  AB::PlatformState* platform);
    void GameUpdate(AB::MemoryArena* arena,  AB::PlatformState* platform);
    void GameRender(AB::MemoryArena* arena,  AB::PlatformState* platform);

#if defined(AB_COMPILER_MSVC)
#define SOKO_PLATFORM_FUNCTION(func) soko::_GlobalPlatform->functions.##func
#else
#define SOKO_PLATFORM_FUNCTION(func) soko::_GlobalPlatform->functions. func
#endif

#define DebugGetFileSize SOKO_PLATFORM_FUNCTION(DebugGetFileSize)
#define DebugReadFile SOKO_PLATFORM_FUNCTION(DebugReadFile)
#define DebugReadTextFile SOKO_PLATFORM_FUNCTION(DebugReadTextFile)
#define DebugWriteFile SOKO_PLATFORM_FUNCTION(DebugWriteFile)
#define DebugOpenFile SOKO_PLATFORM_FUNCTION(DebugOpenFile)
#define DebugCloseFile SOKO_PLATFORM_FUNCTION(DebugCloseFile)
#define DebugWriteToOpenedFile  SOKO_PLATFORM_FUNCTION(DebugWriteToOpenedFile)
#define FormatString SOKO_PLATFORM_FUNCTION(FormatString)
#define PrintString SOKO_PLATFORM_FUNCTION(PrintString)
#define Log SOKO_PLATFORM_FUNCTION(Log)
#define LogAssertV SOKO_PLATFORM_FUNCTION(LogAssertV)
#define SetInputMode SOKO_PLATFORM_FUNCTION(SetInputMode)
#define QueryNewArena SOKO_PLATFORM_FUNCTION(QueryNewArena)
#define FreeArena SOKO_PLATFORM_FUNCTION(FreeArena)
#define GetTimeStamp SOKO_PLATFORM_FUNCTION(GetTimeStamp)
#define EnumerateFilesInDirectory SOKO_PLATFORM_FUNCTION(EnumerateFilesInDirectory)

#define PLATFORM_QUERY_NEW_ARENA QueryNewArena
#define PLATFORM_FREE_ARENA FreeArena

        inline void
        LogAssert(AB::LogLevel level, const char* file, const char* func, u32 line,
                  const char* assertStr, const char* fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            LogAssertV(level, file, func, line, assertStr, fmt, &args);
            va_end(args);
        }

    inline void
    LogAssert(AB::LogLevel level, const char* file, const char* func, u32 line,
              const char* assertStr)
    {
        LogAssertV(level, file, func, line, assertStr, nullptr, nullptr);
    }
}

// NOTE: Panic macro should not be stripped in release build
#if defined(AB_COMPILER_CLANG)
#define SOKO_INFO(format, ...) Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define SOKO_WARN(format, ...) Log(AB::LOG_WARN, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define SOKO_ASSERT(expr, ...) do { if (!(expr)) {soko::LogAssert(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, ##__VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#define SOKO_PANIC(format, ...) do{ Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__); abort();} while(false)
#else
#define SOKO_INFO(format, ...) Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, __VA_ARGS__)
#define SOKO_WARN(format, ...) Log(AB::LOG_WARN, __FILE__, __func__, __LINE__, format, __VA_ARGS__)
#define SOKO_ASSERT(expr, ...) do { if (!(expr)) {soko::LogAssert(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, __VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#define SOKO_PANIC(format, ...) do{ Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, __VA_ARGS__); abort();} while(false)
#endif
#define INVALID_DEFAULT_CASE default:{ SOKO_ASSERT(false, "Invalid default case."); }break
#define INVALID_CODE_PATH SOKO_ASSERT(false, "Invalid code path.")
#if defined(UNITY_BUILD)
#include "MetaInfo.cpp"
#endif
#include "MetaInfo_Generated.h"

#include "RenderGroup.h"
#include "Camera.h"
#include "GameSession.h"
#include "Sokoban.h"

#include "FileFormats.h"
#include "DebugOverlay.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_HDR
#define STBI_MALLOC(sz) PUSH_SIZE(soko::_GlobalStaticStorage->gameState->tempArena, sz)
#define STBI_FREE(sz) do{}while(false)
inline void* ReallocForSTBI(void* p, uptr oldSize, uptr newSize)
{
    void* newMem = PUSH_SIZE(soko::_GlobalStaticStorage->gameState->tempArena, newSize);
    COPY_BYTES(oldSize, newMem, p);
    return newMem;
}
// NOTE: Realloc not used in PNG loading code
#define STBI_REALLOC(p, newsz)
#define STBI_REALLOC_SIZED(p, oldsz, newsz) ReallocForSTBI(p, oldsz, newsz)
#include "stb/stb_image.h"

#undef STB_IMAGE_IMPLEMENTATION

// NOTE: Actual frame time
#define GlobalAbsDeltaTime soko::_GlobalPlatform->absDeltaTime
// NOTE: Frame time corrected by game speed
#define GlobalGameDeltaTime soko::_GlobalPlatform->gameDeltaTime
// NOTE: Why clang inserts the dot by itself
#define GlobalInput soko::_GlobalPlatform->input
#define PlatformGlobals (*soko::_GlobalPlatform)

#define GL_FUNCTION(func) soko::_GlobalPlatform->gl->_##func

#define glGenTextures GL_FUNCTION(glGenTextures)
#define glBindTexture GL_FUNCTION(glBindTexture)
#define glTexParameteri GL_FUNCTION(glTexParameteri)
#define glTexImage2D GL_FUNCTION(glTexImage2D)
#define glDeleteTextures GL_FUNCTION(glDeleteTextures)
#define glPolygonMode GL_FUNCTION(glPolygonMode)
#define glDisable GL_FUNCTION(glDisable)
#define glClearColor GL_FUNCTION(glClearColor)
#define glEnable GL_FUNCTION(glEnable)
#define glBindBuffer GL_FUNCTION(glBindBuffer)
#define glBufferData GL_FUNCTION(glBufferData)
#define glEnableVertexAttribArray GL_FUNCTION(glEnableVertexAttribArray)
#define glVertexAttribPointer GL_FUNCTION(glVertexAttribPointer)
#define glUseProgram GL_FUNCTION(glUseProgram)
#define glActiveTexture GL_FUNCTION(glActiveTexture)
#define glUniform1i GL_FUNCTION(glUniform1i)
#define glUniformSubroutinesuiv GL_FUNCTION(glUniformSubroutinesuiv)
#define glDrawElements GL_FUNCTION(glDrawElements)
#define glGenBuffers GL_FUNCTION(glGenBuffers)
#define glCreateShader GL_FUNCTION(glCreateShader)
#define glShaderSource GL_FUNCTION(glShaderSource)
#define glCompileShader GL_FUNCTION(glCompileShader)
#define glGetShaderiv GL_FUNCTION(glGetShaderiv)
#define glGetShaderInfoLog GL_FUNCTION(glGetShaderInfoLog)
#define glCreateProgram GL_FUNCTION(glCreateProgram)
#define glAttachShader GL_FUNCTION(glAttachShader)
#define glLinkProgram GL_FUNCTION(glLinkProgram)
#define glGetProgramiv GL_FUNCTION(glGetProgramiv)
#define glGetProgramInfoLog GL_FUNCTION(glGetProgramInfoLog)
#define glViewport GL_FUNCTION(glViewport)
#define glDeleteShader GL_FUNCTION(glDeleteShader)
#define glGetSubroutineIndex GL_FUNCTION(glGetSubroutineIndex)
#define glGetUniformLocation GL_FUNCTION(glGetUniformLocation)
#define glTexImage2DMultisample GL_FUNCTION(glTexImage2DMultisample)
#define glGenFramebuffers GL_FUNCTION(glGenFramebuffers)
#define glBindFramebuffer GL_FUNCTION(glBindFramebuffer)
#define glCheckFramebufferStatus GL_FUNCTION(glCheckFramebufferStatus)
#define glFramebufferTexture2D GL_FUNCTION(glFramebufferTexture2D)
#define glClear GL_FUNCTION(glClear)
#define glMapBuffer GL_FUNCTION(glMapBuffer)
#define glUnmapBuffer GL_FUNCTION(glUnmapBuffer)
#define glDepthMask GL_FUNCTION(glDepthMask)
#define glDepthFunc GL_FUNCTION(glDepthFunc)
#define glBlendEquation GL_FUNCTION(glBlendEquation)
#define glBlendFunc GL_FUNCTION(glBlendFunc)
#define glCullFace GL_FUNCTION(glCullFace)
#define glFrontface GL_FUNCTION(glFrontface)
#define glGenVertexArrays GL_FUNCTION(glGenVertexArrays)
#define glBindVertexArray GL_FUNCTION(glBindVertexArray)
#define glFrontFace GL_FUNCTION(glFrontFace)
#define glGetUniformBlockIndex GL_FUNCTION(glGetUniformBlockIndex)
#define glUniformBlockBinding GL_FUNCTION(glUniformBlockBinding)
#define glBindBufferRange GL_FUNCTION(glBindBufferRange)
#define glBindBufferBase GL_FUNCTION(glBindBufferBase)
#define glDrawArrays GL_FUNCTION(glDrawArrays)
#define glUniform1f GL_FUNCTION(glUniform1f)
#define glBufferSubData GL_FUNCTION(glBufferSubData)
#define glUniform3fv GL_FUNCTION(glUniform3fv)
#define glLineWidth GL_FUNCTION(glLineWidth)
#define glUniformMatrix4fv GL_FUNCTION(glUniformMatrix4fv)
#define glBlitFramebuffer GL_FUNCTION(glBlitFramebuffer)
#define glTexParameterfv GL_FUNCTION(glTexParameterfv)
#define glVertexAttribDivisor GL_FUNCTION(glVertexAttribDivisor)
#define glDrawElementsInstanced GL_FUNCTION(glDrawElementsInstanced)
#define glDrawArraysInstanced GL_FUNCTION(glDrawArraysInstanced)
#define glClearDepth GL_FUNCTION(glClearDepth)
#define glTexImage3D GL_FUNCTION(glTexImage3D)
#define glTexSubImage3D GL_FUNCTION(glTexSubImage3D)
#define glTexStorage3D GL_FUNCTION(glTexStorage3D)
#define glGenerateMipmap GL_FUNCTION(glGenerateMipmap)
#define glTexParameterf GL_FUNCTION(glTexParameterf)
#define glCreateBuffers GL_FUNCTION(glCreateBuffers)
#define glNamedBufferData GL_FUNCTION(glNamedBufferData)
#define glMapNamedBuffer GL_FUNCTION(glMapNamedBuffer)
#define glUnmapNamedBuffer GL_FUNCTION(glUnmapNamedBuffer)
#define glUniformMatrix3fv GL_FUNCTION(glUniformMatrix3fv)
#define glUniform4fv GL_FUNCTION(glUniform4fv)
#define glVertexAttribIPointer GL_FUNCTION(glVertexAttribIPointer)
#define glGetFloatv GL_FUNCTION(glGetFloatv)
#define glUniform2fv GL_FUNCTION(glUniform2fv)
#define glFinish GL_FUNCTION(glFinish)
#define glGetTexImage GL_FUNCTION(glGetTexImage)
#define glFlush GL_FUNCTION(glFlush)
#define glDrawBuffer GL_FUNCTION(glDrawBuffer)
#define glReadBuffer GL_FUNCTION(glReadBuffer)
#define glPolygonOffset GL_FUNCTION(glPolygonOffset)
#define glTexImage1D GL_FUNCTION(glTexImage1D)

// NOTE: Functions used by ImGUI
#define glGetIntegerv GL_FUNCTION(glGetIntegerv)
#define glBindSampler GL_FUNCTION(glBindSampler)
#define glIsEnabled GL_FUNCTION(glIsEnabled)
#define glScissor GL_FUNCTION(glScissor)
#define glDrawElementsBaseVertex GL_FUNCTION(glDrawElementsBaseVertex)
#define glDeleteVertexArrays GL_FUNCTION(glDeleteVertexArrays)
#define glBindSampler GL_FUNCTION(glBindSampler)
#define glBlendEquationSeparate GL_FUNCTION(glBlendEquationSeparate)
#define glBlendFuncSeparate GL_FUNCTION(glBlendFuncSeparate)
#define glPixelStorei GL_FUNCTION(glPixelStorei)
#define glGetAttribLocation GL_FUNCTION(glGetAttribLocation)
#define glDeleteBuffers GL_FUNCTION(glDeleteBuffers)
#define glDetachShader GL_FUNCTION(glDetachShader)
#define glDeleteProgram GL_FUNCTION(glDeleteProgram)

                namespace soko
                {

// TODO: Bounds checking of using enum classes
                    inline bool
                    JustPressed(AB::KeyCode code)
                    {
                        bool result = GlobalInput.keys[(u32)code].pressedNow && !GlobalInput.keys[(u32)code].wasPressed;
                        return result;
                    }

                    inline bool
                    JustReleased(AB::KeyCode code)
                    {
                        bool result = !GlobalInput.keys[(u32)code].pressedNow && GlobalInput.keys[(u32)code].wasPressed;
                        return result;
                    }

                    inline bool
                    IsDown(AB::KeyCode code)
                    {
                        bool result = GlobalInput.keys[(u32)code].pressedNow;
                        return result;
                    }

                    inline bool
                    JustPressed(AB::MouseButton button)
                    {
                        bool result = GlobalInput.mouseButtons[(u32)button].pressedNow && !GlobalInput.mouseButtons[(u32)button].wasPressed;
                        return result;
                    }

                    inline bool
                    JustReleased(AB::MouseButton button)
                    {
                        bool result = !GlobalInput.mouseButtons[(u32)button].pressedNow && GlobalInput.mouseButtons[(u32)button].wasPressed;
                        return result;
                    }

                    inline bool
                    IsDown(AB::MouseButton button)
                    {
                        bool result = GlobalInput.mouseButtons[(u32)button].pressedNow;
                        return result;
                    }

                }

#include "imgui/imgui.h"
//#include "imgui/imgui_internal.h"
#if defined(UNITY_BUILD)
#include "Renderer.cpp"
#include "Level.cpp"
#include "Chunk.cpp"
#include "Entity.cpp"
#include "DebugOverlay.cpp"
#include "Camera.cpp"
#include "Player.cpp"
#include "MeshGen.cpp"
#include "SimRegion.cpp"
#include "EntityBehavior.cpp"

#include "Editor.cpp"

#include "GameMenu.cpp"
#include "GameSession.cpp"

#include "MetaInfo_Generated.cpp"
#include "RenderGroup.cpp"
#endif

extern "C" GAME_CODE_ENTRY void
GameUpdateAndRender(AB::MemoryArena* arena,
                    AB::PlatformState* platform,
                    AB::GameUpdateAndRenderReason reason)
{
    using namespace AB;
    switch (reason)
    {
    case GUR_REASON_INIT:
    {
        soko::GameInit(arena, platform);

    } break;
    case GUR_REASON_RELOAD:
    {
        soko::GameReload(arena, platform);
    } break;
    case GUR_REASON_UPDATE:
    {
        soko::GameUpdate(arena, platform);
    } break;
    case GUR_REASON_RENDER:
    {
        soko::GameRender(arena, platform);
    } break;
    INVALID_DEFAULT_CASE;
    }
}
namespace soko
{
    internal void
    GenMeshTangents(Mesh* mesh, AB::MemoryArena* arena)
    {
        u32 triCount = mesh->indexCount / 3;
        mesh->tangents = PUSH_ARRAY(arena, v3, mesh->vertexCount);
        SOKO_ASSERT(mesh->tangents);

        for (u32 index = 0; index < mesh->indexCount; index += 3)
        {
            SOKO_ASSERT((index + 2) < mesh->indexCount);
            v3 vt0 = mesh->vertices[mesh->indices[index]];
            v3 vt1 = mesh->vertices[mesh->indices[index + 1]];
            v3 vt2 = mesh->vertices[mesh->indices[index + 2]];

            v2 uv0 = mesh->uvs[mesh->indices[index]];
            v2 uv1 = mesh->uvs[mesh->indices[index + 1]];
            v2 uv2 = mesh->uvs[mesh->indices[index + 2]];

            v3 e0 = vt1 - vt0;
            v3 e1 = vt2 - vt0;

            v2 dUV0 = uv1 - uv0;
            v2 dUV1 = uv2 - uv0;

            f32 f = 1.0f / (dUV0.u * dUV1.v - dUV1.u * dUV0.v);

            v3 tangent;
            tangent.x = f * (dUV1.v * e0.x - dUV0.v * e1.x);
            tangent.y = f * (dUV1.v * e0.y - dUV0.v * e1.y);
            tangent.z = f * (dUV1.v * e0.z - dUV0.v * e1.z);

            tangent = Normalize(tangent);

            mesh->tangents[index] = tangent;
            mesh->tangents[index + 1] = tangent;
            mesh->tangents[index + 2] = tangent;
        }
    }


    internal void
    GameInit(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
        _GlobalStaticStorage = (StaticStorage*)PUSH_STRUCT(arena, StaticStorage);
        SOKO_ASSERT(_GlobalStaticStorage == arena->begin);
        _GlobalPlatform = platform;

        _GlobalPlatform->gameSpeed = 1.0f;

        auto* tempArena = AllocateSubArena(arena, arena->size / 2, true);
        SOKO_ASSERT(tempArena, "Failed to allocate tempArena.");
        _GlobalStaticStorage->gameState = PUSH_STRUCT(arena, GameState);
        SOKO_ASSERT(_GlobalStaticStorage->gameState, "");

        _GlobalStaticStorage->gameState->tempArena = tempArena;
        _GlobalStaticStorage->gameState->memoryArena = arena;

        GameState* gameState = _GlobalStaticStorage->gameState;

        _GlobalStaticStorage->metaInfo = meta::InitMetaInfo(gameState->memoryArena);

        // NOTE: ImGui
        IMGUI_CHECKVERSION();
        ImGui::SetAllocatorFunctions(_GlobalPlatform->functions.AllocForImGui,
                                     _GlobalPlatform->functions.FreeForImGui,
                                     _GlobalPlatform->imGuiAllocatorData);
        ImGui::SetCurrentContext(_GlobalPlatform->imGuiContext);

        ImGuiIO* io = &ImGui::GetIO();
        //gameState->notoMonoFont10px = io->Fonts->AddFontFromFileTTF("../res/NotoMono-Regular.ttf", 10.0f);
        //gameState->notoMonoFont14px = io->Fonts->AddFontFromFileTTF("../res/NotoMono-Regular.ttf", 14.0f);

        //ImGui::PushFont(0);

        gameState->overlayCorner = 1;

        gameState->renderer = AllocAndInitRenderer(arena, tempArena, {PlatformGlobals.windowWidth, PlatformGlobals.windowHeight});
        gameState->renderGroup = AllocateRenderGroup(arena, KILOBYTES(1024), 16384);

        gameState->renderer->clearColor = V4(0.8f, 0.8f, 0.8f, 1.0f);

        auto tempMem = BeginTemporaryMemory(gameState->tempArena);
        gameState->meshes[EntityMesh_Sphere] = LoadMesh(gameState->tempArena, L"../res/mesh/sphere.aab");
//        gameState->meshes[EntityMesh_Gun] = LoadMesh(gameState->tempArena, L"../res/mesh/gun.aab");
        gameState->meshes[EntityMesh_Cube] = LoadMesh(gameState->tempArena, L"../res/mesh/cube.aab");
        gameState->meshes[EntityMesh_Plate] = LoadMesh(gameState->tempArena, L"../res/mesh/plate.aab");
        gameState->meshes[EntityMesh_Portal] = LoadMesh(gameState->tempArena, L"../res/mesh/portal.aab");
        gameState->meshes[EntityMesh_Spikes] = LoadMesh(gameState->tempArena, L"../res/mesh/spikes.aab");
        gameState->meshes[EntityMesh_Button] = LoadMesh(gameState->tempArena, L"../res/mesh/button.aab");
        gameState->meshes[EntityMesh_Box] = LoadMesh(gameState->tempArena, L"../res/mesh/box.aab");
        gameState->meshes[EntityMesh_Altar] = LoadMesh(gameState->tempArena, L"../res/mesh/altar.aab");
        gameState->meshes[EntityMesh_Cat] = LoadMesh(gameState->tempArena, L"../res/mesh/cat.aab");

        EndTemporaryMemory(&tempMem);

        stbi_set_flip_vertically_on_load(1);

//        gameState->materials[EntityMaterial_PbrMetal] = LoadMaterialPBR(gameState->tempArena, "../res/rust_metal/grimy-metal-albedo.png", "../res/rust_metal/grimy-metal-roughness.png", "../res/rust_metal/grimy-metal-metalness.png", "../res/rust_metal/grimy-metal-normal-dx.png");
//        gameState->materials[EntityMaterial_Rock] = LoadMaterialPBR(gameState->tempArena, "../res/rock/layered-rock1-albedo.png", "../res/rock/layered-rock1-rough.png", "../res/rock/layered-rock1-Metalness.png", "../res/rock/layered-rock1-normal-dx.png");
        gameState->materials[EntityMaterial_OldMetal] = LoadMaterialPBR(gameState->tempArena, "../res/oldmetal/greasy-metal-pan1-albedo.png", "../res/oldmetal/greasy-metal-pan1-roughness.png", "../res/oldmetal/greasy-metal-pan1-metal.png", "../res/oldmetal/greasy-metal-pan1-normal.png");
//        gameState->materials[EntityMaterial_Gun] = LoadMaterialPBR(gameState->tempArena, "../res/gun/Cerberus_A.png", "../res/gun/Cerberus_R.png", "../res/gun/Cerberus_M.png", "../res/gun/Cerberus_N.png");
        gameState->materials[EntityMaterial_Box] = LoadMaterialPBR(gameState->tempArena, "../res/material/box/Box_albedo.png", "../res/material/box/Box_roughness.png", "../res/material/box/Box_metallic.png", "../res/material/box/Box_normal.png");
        gameState->materials[EntityMaterial_Altar] = LoadMaterialPBR(gameState->tempArena, "../res/material/altar/DefaultMaterial_Base_Color.png", "../res/material/altar/DefaultMaterial_Roughness.png", "../res/material/altar/DefaultMaterial_Metallic.png", "../res/material/altar/DefaultMaterial_Normal_DirectX.png");
        gameState->materials[EntityMaterial_Cat] = LoadMaterialPBR(gameState->tempArena, "../res/material/cat/DefaultMaterial_albedo.png", "../res/material/cat/DefaultMaterial_roughness.png", "../res/material/cat/DefaultMaterial_metallic.png", "../res/material/cat/DefaultMaterial_normal.png");

        gameState->materials[EntityMaterial_BRDFCustom] = {};
        gameState->materials[EntityMaterial_BRDFCustom].type = Material::PBR;
        gameState->materials[EntityMaterial_BRDFCustom].pbr.isCustom = 1;

        gameState->materials[EntityMaterial_Tile] = LoadMaterialLegacy(gameState->tempArena, "../res/tile.png");
        gameState->materials[EntityMaterial_Player] = LoadMaterialLegacy(gameState->tempArena, "../res/tile_player.png");
        gameState->materials[EntityMaterial_Block] = LoadMaterialLegacy(gameState->tempArena, "../res/tile_block.png");
        gameState->materials[EntityMaterial_RedPlate] = LoadMaterialLegacy(gameState->tempArena, "../res/plate_palette.png");
        gameState->materials[EntityMaterial_Portal] = LoadMaterialLegacy(gameState->tempArena, "../res/portal_diff.png", "../res/portal_spec.png");
        gameState->materials[EntityMaterial_Button] = LoadMaterialLegacy(gameState->tempArena, "../res/button.png");


        byte bitmap[3];
        bitmap[0] = 128;
        bitmap[1] = 128;
        bitmap[2] = 128;
        gameState->materials[EntityMaterial_Spikes] = LoadMaterialLegacy(1, 1, bitmap);

        gameState->skybox = LoadCubemap(gameState->tempArena,
                                        "../res/skybox/sky_back.png",
                                        "../res/skybox/sky_down.png",
                                        "../res/skybox/sky_front.png",
                                        "../res/skybox/sky_left.png",
                                        "../res/skybox/sky_right.png",
                                        "../res/skybox/sky_up.png");

        stbi_set_flip_vertically_on_load(0);

        gameState->hdrMap = LoadCubemapHDR(gameState->tempArena,
                                           "../res/desert_sky/nz.hdr",
                                           "../res/desert_sky/ny.hdr",
                                           "../res/desert_sky/pz.hdr",
                                           "../res/desert_sky/nx.hdr",
                                           "../res/desert_sky/px.hdr",
                                           "../res/desert_sky/py.hdr");


        gameState->irradanceMap = MakeEmptyCubemap(64, 64, GL_RGB16F);
        gameState->enviromentMap = MakeEmptyCubemap(256, 256, GL_RGB16F, TextureFilter_Trilinear, true);
        //gameState->BRDFLut = LoadTexture(512, 512, 0, GL_RG16F, GL_CLAMP_TO_EDGE, TextureFilter_Bilinear);

        gameState->renderGroup->drawSkybox = true;
        gameState->renderGroup->skyboxHandle = gameState->enviromentMap.gpuHandle;
        gameState->renderGroup->irradanceMapHandle = gameState->irradanceMap.gpuHandle;
        gameState->renderGroup->envMapHandle = gameState->enviromentMap.gpuHandle;

        GenIrradanceMap(gameState->renderer, &gameState->irradanceMap, gameState->hdrMap.gpuHandle);
        GenEnvPrefiliteredMap(gameState->renderer, &gameState->enviromentMap, gameState->hdrMap.gpuHandle, 6);
        //GenBRDFLut(gameState->renderer, &gameState->BRDFLut);

        //gameState->renderer->BRDFLutHandle = gameState->BRDFLut.gpuHandle;

        //auto* level = gameState->level;
        //gameState->port = 9999;
        //gameState->ipOctets[0] = 127;
        //gameState->ipOctets[3] = 1;

        // TODO: Loading player name from config file
        strcpy(gameState->playerName, "Unnamed player");
    }

    void
    GameReload(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
        _GlobalPlatform = platform;
        _GlobalStaticStorage = (StaticStorage*)arena->begin;

        IMGUI_CHECKVERSION();
        ImGui::SetAllocatorFunctions(_GlobalPlatform->functions.AllocForImGui,
                                     _GlobalPlatform->functions.FreeForImGui,
                                     _GlobalPlatform->imGuiAllocatorData);
        ImGui::SetCurrentContext(_GlobalPlatform->imGuiContext);
        UnloadShaders(&_GlobalStaticStorage->gameState->renderer->shaders);
        _GlobalStaticStorage->gameState->renderer->shaders = LoadShaders();
        SOKO_INFO("Game code was reloaded");
    }

    void
    GameUpdate(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
        auto* gameState = _GlobalStaticStorage->gameState;
        Level* level = gameState->session.level;

        if (gameState->globalGameMode != GAME_MODE_MENU)
        {
        }
    }

    void
    SessionUpdateAndRender(GameState* gameState)
    {
        DrawOverlay(gameState);
        Entity* player = gameState->session.player;
        Level* level = gameState->session.level;
        DEBUG_OVERLAY_TRACE(level->globalChunkMeshBlockCount);
        DEBUG_OVERLAY_TRACE(level->mesherFreeList.count);

        auto tempMemory = BeginTemporaryMemory(gameState->tempArena);
        SimRegion _simRegion = BeginSim(gameState->tempArena,
                                        level,
                                        MakeWorldPos(player->pos),
                                        2);
        auto simRegion = &_simRegion;

        UpdateRegion(simRegion);

        if (JustPressed(AB::KEY_F1))
        {
            gameState->session.useDebugCamera = !gameState->session.useDebugCamera;
        }

        CameraConfig* camConf = 0;
        GameCamera* camera = &gameState->session.camera;
        if (gameState->session.useDebugCamera)
        {
            UpdateCamera(&gameState->session.debugCamera);
            camConf = &gameState->session.debugCamera.conf;
        }
        else
        {
            UpdateCamera(&gameState->session.camera, &MakeWorldPos(player->pos));
            camConf = &gameState->session.camera.conf;
        }

        // TODO: This crashes the game
#if 0
        if (JustPressed(MBUTTON_LEFT))
        {
            v3 from = RHToWorld(camera->conf.position);
            v3 ray = RHToWorld(camera->mouseRayRH);
            auto raycast = Raycast(simRegion, from, ray, Raycast_Tilemap);
            if (raycast.hit == RaycastResult::Tile)
            {
                iv3 tile = raycast.tile.coord + DirToUnitOffset(raycast.tile.normalDir);
                PrintString("Entities in tile: (%i32, %i32, %i32)\n", tile.x, tile.y, tile.z);
                EntityMapIterator it = {};
                while (true)
                {
                    Entity* pe = YieldEntityFromTile(level, tile, &it);
                    if (!pe) break;
                    PrintString("Entity: id = %u32, type = %s, pos = (%i32, %i32, %i32)\n", pe->id, meta::GetEnumName(pe->type), pe->pos.x, pe->pos.y, pe->pos.z);
                }
            }
        }
#endif
        RenderGroupSetCamera(gameState->renderGroup, camConf);

        v3 beg = gameState->session.camera.conf.position;

        DirectionalLight light = {};
        light.dir = Normalize(V3(1.0f, -5.0f, -2.5f));
        light.from = V3(0.0f, 1.5f, 10.0f) + WorldToRH(GetRelPos(gameState->session.camera.worldPos, WorldPos{}));
        light.ambient = V3(0.3f);
        light.diffuse = V3(0.8f);
        light.specular = V3(1.0f);
        RenderCommandSetDirLight lightCommand = {};
        lightCommand.light = light;
        RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_SET_DIR_LIGHT,
                               (void*)&lightCommand);

        DrawRegion(simRegion, gameState, gameState->session.camera.worldPos);

        ShadowPass(gameState->renderer, gameState->renderGroup);
        RendererBeginFrame(gameState->renderer, V2(PlatformGlobals.windowWidth, PlatformGlobals.windowHeight));
        FlushRenderGroup(gameState->renderer, gameState->renderGroup);
        RendererEndFrame(gameState->renderer);

        EndTemporaryMemory(&tempMemory);

        SOKO_ASSERT(level->completePlatformCount <= level->platformCount);

        DEBUG_OVERLAY_TRACE(level->completePlatformCount);
        DEBUG_OVERLAY_TRACE(level->platformCount);

        if (level->completePlatformCount == level->platformCount)
        {
            DestroyGameSession(&gameState->session);
            gameState->globalGameMode = GAME_MODE_MENU;
            gameState->mainMenu.state = MainMenu_LevelCompleted;
        }

        if (DebugOverlayBeginCustom())
        {
            if (ImGui::Button("Exit to main menu", {150.0f, 20.0f}))
            {
                gameState->globalGameMode = GAME_MODE_MENU;
            }
        }
        DebugOverlayEndCustom();
    }

    internal void
    GameRender(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
        auto* gameState = _GlobalStaticStorage->gameState;
        BeginDebugOverlay();
        switch (gameState->globalGameMode)
        {
        case GAME_MODE_MENU: { MenuUpdateAndRender(&gameState->mainMenu, gameState); } break;
        case GAME_MODE_EDITOR: { EditorUpdateAndRender(gameState); } break;
        default:
        {
            SessionUpdateAndRender(gameState);
            CheckTempArena(gameState->tempArena);
            if (gameState->globalGameMode == GAME_MODE_MENU)
            {
                DestroyGameSession(&gameState->session);
            }
        } break;
        }
    }
}

// NOTE: IMGUI
#include "imgui/imconfig.h"
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"
