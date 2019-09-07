#define HYPERMATH_IMPL
#include "hypermath.h"
#undef HYPERMATH_IMPL


#include "Sokoban.h"
#include "Memory.h"

#include "FileFormats.h"
#include "DebugOverlay.h"

namespace soko
{
    struct GameState;

    struct StaticStorage
    {
        GameState* gameState;
    };

    static AB::PlatformState* _GlobalPlatform;
    static StaticStorage* _GlobalStaticStorage;

    void GameInit(AB::MemoryArena* arena,  AB::PlatformState* platform);
    void GameReload(AB::MemoryArena* arena,  AB::PlatformState* platform);
    void GameUpdate(AB::MemoryArena* arena,  AB::PlatformState* platform);
    void GameRender(AB::MemoryArena* arena,  AB::PlatformState* platform);

}

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_MALLOC(sz) PUSH_SIZE(soko::_GlobalStaticStorage->gameState->tempArena, sz)
#define STBI_FREE(sz)
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
#if defined(AB_COMPILER_MSVC)
#define SOKO_PLATFORM_FUNCTION(func) soko::_GlobalPlatform->functions.##func
#else
#define SOKO_PLATFORM_FUNCTION(func) soko::_GlobalPlatform->functions. func
#endif

#define DebugGetFileSize SOKO_PLATFORM_FUNCTION(DebugGetFileSize)
#define DebugReadFile SOKO_PLATFORM_FUNCTION(DebugReadFile)
#define DebugReadTextFile SOKO_PLATFORM_FUNCTION(DebugReadTextFile)
#define FormatString SOKO_PLATFORM_FUNCTION(FormatString)
#define PrintString SOKO_PLATFORM_FUNCTION(PrintString)
#define Log SOKO_PLATFORM_FUNCTION(Log)
#define LogAssertV SOKO_PLATFORM_FUNCTION(LogAssertV)
#define SetInputMode SOKO_PLATFORM_FUNCTION(SetInputMode)
#define NetCreateSocket SOKO_PLATFORM_FUNCTION(NetCreateSocket)
#define NetBindSocket SOKO_PLATFORM_FUNCTION(NetBindSocket)
#define NetSend SOKO_PLATFORM_FUNCTION(NetSend)
#define NetRecieve SOKO_PLATFORM_FUNCTION(NetRecieve)

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
}

// TODO:: Asserts without message
// NOTE: Panic macro should not be stripped in release build
#if defined(AB_COMPILER_CLANG)
#define SOKO_INFO(format, ...) Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define SOKO_ASSERT(expr, ...) do { if (!(expr)) {soko::LogAssert(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, ##__VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#define SOKO_PANIC(format, ...) do{ Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__); abort();} while(false)
#else
#define SOKO_INFO(format, ...) Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, __VA_ARGS__)
#define SOKO_ASSERT(expr, ...) do { if (!(expr)) {soko::LogAssert(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, __VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#define SOKO_PANIC(format, ...) do{ Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, __VA_ARGS__); abort();} while(false)
#endif
#define INVALID_DEFAULT_CASE default:{ SOKO_ASSERT(false, "Invalid default case."); }break
#define INVALID_CODE_PATH SOKO_ASSERT(false, "Invalid code path.")

#include "imgui/imgui.h"
//#include "imgui/imgui_internal.h"

#include "Level.cpp"
#include "DebugOverlay.cpp"
#include "Camera.cpp"
#include "Network.cpp"

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

    Player*
    AddPlayer(GameState* gameState, v3i coord, AB::MemoryArena* arena)
    {
        Player* p = nullptr;
        if (gameState->playerCount < GameState::MAX_PLAYERS)
        {
            p = gameState->players + gameState->playerCount;
            gameState->playerCount++;


            u32 playerId =  AddEntity(&gameState->level, ENTITY_TYPE_PLAYER, coord,
                                      &gameState->cubeMesh, &gameState->tilePlayerMaterial, arena);

            p->level = &gameState->level;
            p->e = GetEntity(p->level, playerId);
            SOKO_ASSERT(p->e);
        }
        return p;
    }

    void
    GameInit(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
        _GlobalStaticStorage = (StaticStorage*)PUSH_SIZE(arena, KILOBYTES(1));
        SOKO_ASSERT(_GlobalStaticStorage == arena->begin, 0);
        _GlobalPlatform = platform;

        _GlobalPlatform->gameSpeed = 1.0f;

        auto* tempArena = AllocateSubArena(arena, arena->size / 2);
        SOKO_ASSERT(tempArena, "Failed to allocate tempArena.");
        _GlobalStaticStorage->gameState = PUSH_STRUCT(arena, GameState);
        SOKO_ASSERT(_GlobalStaticStorage->gameState, "");

        _GlobalStaticStorage->gameState->tempArena = tempArena;
        _GlobalStaticStorage->gameState->memoryArena = arena;

        GameState* gameState = _GlobalStaticStorage->gameState;

        // NOTE: ImGui
        IMGUI_CHECKVERSION();
        ImGui::SetAllocatorFunctions(_GlobalPlatform->functions.AllocForImGui,
                                     _GlobalPlatform->functions.FreeForImGui,
                                     _GlobalPlatform->imGuiAllocatorData);
        ImGui::SetCurrentContext(_GlobalPlatform->imGuiContext);

        gameState->overlayCorner = 1;

        gameState->renderer = AllocAndInitRenderer(arena);
        gameState->renderGroup = AllocateRenderGroup(arena, KILOBYTES(1024), 16384);

        CameraConfig camera = {};
        camera.position = V3(0.0f);
        camera.front = V3(0.0f, 0.0f, -1.0f);
        camera.fovDeg = 45.0f;
        camera.aspectRatio = 16.0f / 9.0f;
        camera.nearPlane = 0.1f;
        camera.farPlane = 100.0f;

        gameState->debugCamera.conf = camera;
        gameState->debugCamera.moveSpeed = 3.0f;
        gameState->debugCamera.rotateSpeed = 60.0f;
        gameState->debugCamera.moveSmooth = 0.8f;
        gameState->debugCamera.rotateSmooth = 0.45f;

        gameState->camera.conf = camera;
        gameState->camera.longSmooth = 0.3f;
        gameState->camera.latSmooth = 0.3f;
        gameState->camera.distSmooth = 0.3f;
        gameState->camera.rotSpeed = 1000.0f;
        gameState->camera.zoomSpeed = 5.0f;
        gameState->camera.moveSpeed = 500.0f;
        gameState->camera.moveFriction = 8.0f;

        RenderGroupSetCamera(gameState->renderGroup, &camera);

        gameState->renderer->clearColor = V4(0.8f, 0.8f, 0.8f, 1.0f);
        {
            u32 fileSize = DebugGetFileSize(L"../res/cube.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/cube.aab");
            // NOTE: Strict aliasing
            auto header = (AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AAB_FILE_MAGIC_VALUE, "");

            Mesh mesh = {};
            mesh.vertexCount = header->verticesCount;
            mesh.normalCount = header->normalsCount;
            mesh.uvCount = header->uvsCount;
            mesh.indexCount = header->indicesCount;

            mesh.vertices = (v3*)((byte*)fileData + header->verticesOffset);
            mesh.normals = (v3*)((byte*)fileData + header->normalsOffset);
            mesh.uvs = (v2*)((byte*)fileData + header->uvsOffset);
            mesh.indices = (u32*)((byte*)fileData + header->indicesOffset);

            RendererLoadMesh(&mesh);
            SOKO_ASSERT(mesh.gpuVertexBufferHandle, "");
            SOKO_ASSERT(mesh.gpuIndexBufferHandle, "");

            gameState->cubeMesh = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/plate.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/plate.aab");
            // NOTE: Strict aliasing
            auto header = (AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AAB_FILE_MAGIC_VALUE, "");

            Mesh mesh = {};
            mesh.vertexCount = header->verticesCount;
            mesh.normalCount = header->normalsCount;
            mesh.uvCount = header->uvsCount;
            mesh.indexCount = header->indicesCount;

            mesh.vertices = (v3*)((byte*)fileData + header->verticesOffset);
            mesh.normals = (v3*)((byte*)fileData + header->normalsOffset);
            mesh.uvs = (v2*)((byte*)fileData + header->uvsOffset);
            mesh.indices = (u32*)((byte*)fileData + header->indicesOffset);

            RendererLoadMesh(&mesh);
            SOKO_ASSERT(mesh.gpuVertexBufferHandle, "");
            SOKO_ASSERT(mesh.gpuIndexBufferHandle, "");

            gameState->plateMesh = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/portal.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/portal.aab");
            // NOTE: Strict aliasing
            auto header = (AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AAB_FILE_MAGIC_VALUE, "");

            Mesh mesh = {};
            mesh.vertexCount = header->verticesCount;
            mesh.normalCount = header->normalsCount;
            mesh.uvCount = header->uvsCount;
            mesh.indexCount = header->indicesCount;

            mesh.vertices = (v3*)((byte*)fileData + header->verticesOffset);
            mesh.normals = (v3*)((byte*)fileData + header->normalsOffset);
            mesh.uvs = (v2*)((byte*)fileData + header->uvsOffset);
            mesh.indices = (u32*)((byte*)fileData + header->indicesOffset);

            RendererLoadMesh(&mesh);
            SOKO_ASSERT(mesh.gpuVertexBufferHandle, "");
            SOKO_ASSERT(mesh.gpuIndexBufferHandle, "");

            gameState->portalMesh = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/spikes.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/spikes.aab");
            // NOTE: Strict aliasing
            auto header = (AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AAB_FILE_MAGIC_VALUE, "");

            Mesh mesh = {};
            mesh.vertexCount = header->verticesCount;
            mesh.normalCount = header->normalsCount;
            mesh.uvCount = header->uvsCount;
            mesh.indexCount = header->indicesCount;

            mesh.vertices = (v3*)((byte*)fileData + header->verticesOffset);
            mesh.normals = (v3*)((byte*)fileData + header->normalsOffset);
            mesh.uvs = (v2*)((byte*)fileData + header->uvsOffset);
            mesh.indices = (u32*)((byte*)fileData + header->indicesOffset);

            RendererLoadMesh(&mesh);
            SOKO_ASSERT(mesh.gpuVertexBufferHandle, "");
            SOKO_ASSERT(mesh.gpuIndexBufferHandle, "");

            gameState->spikesMesh = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/button.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/button.aab");
            // NOTE: Strict aliasing
            auto header = (AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AAB_FILE_MAGIC_VALUE, "");

            Mesh mesh = {};
            mesh.vertexCount = header->verticesCount;
            mesh.normalCount = header->normalsCount;
            mesh.uvCount = header->uvsCount;
            mesh.indexCount = header->indicesCount;

            mesh.vertices = (v3*)((byte*)fileData + header->verticesOffset);
            mesh.normals = (v3*)((byte*)fileData + header->normalsOffset);
            mesh.uvs = (v2*)((byte*)fileData + header->uvsOffset);
            mesh.indices = (u32*)((byte*)fileData + header->indicesOffset);

            RendererLoadMesh(&mesh);
            SOKO_ASSERT(mesh.gpuVertexBufferHandle, "");
            SOKO_ASSERT(mesh.gpuIndexBufferHandle, "");

            gameState->buttonMesh = mesh;
        }

        stbi_set_flip_vertically_on_load(1);

        {
            i32 width;
            i32 height;
            i32 bpp;
            BeginTemporaryMemory(gameState->tempArena);
            unsigned char* diffBitmap = stbi_load("../res/tile.png", &width, &height, &bpp, 3);

            Material material = {};
            material.diffMap.format = GL_RGB8;
            material.diffMap.width = width;
            material.diffMap.height = height;
            material.diffMap.data = diffBitmap;
            RendererLoadTexture(&material.diffMap);
            SOKO_ASSERT(material.diffMap.gpuHandle, "");

            EndTemporaryMemory(gameState->tempArena);
            gameState->tileMaterial = material;
        }
        {
            i32 width;
            i32 height;
            i32 bpp;
            BeginTemporaryMemory(gameState->tempArena);
            unsigned char* diffBitmap = stbi_load("../res/tile_player.png", &width, &height, &bpp, 3);

            Material material = {};
            material.diffMap.format = GL_RGB8;
            material.diffMap.width = width;
            material.diffMap.height = height;
            material.diffMap.data = diffBitmap;
            RendererLoadTexture(&material.diffMap);
            SOKO_ASSERT(material.diffMap.gpuHandle, "");

            EndTemporaryMemory(gameState->tempArena);
            gameState->tilePlayerMaterial = material;
        }
        {
            i32 width;
            i32 height;
            i32 bpp;
            BeginTemporaryMemory(gameState->tempArena);
            unsigned char* diffBitmap = stbi_load("../res/tile_block.png", &width, &height, &bpp, 3);

            Material material = {};
            material.diffMap.format = GL_RGB8;
            material.diffMap.width = width;
            material.diffMap.height = height;
            material.diffMap.data = diffBitmap;
            RendererLoadTexture(&material.diffMap);
            SOKO_ASSERT(material.diffMap.gpuHandle, "");

            EndTemporaryMemory(gameState->tempArena);
            gameState->tileBlockMaterial = material;
        }
        {
            i32 width;
            i32 height;
            i32 bpp;
            BeginTemporaryMemory(gameState->tempArena);
            unsigned char* diffBitmap = stbi_load("../res/plate_palette.png", &width, &height, &bpp, 3);

            Material material = {};
            material.diffMap.format = GL_RGB8;
            material.diffMap.width = width;
            material.diffMap.height = height;
            material.diffMap.data = diffBitmap;
            RendererLoadTexture(&material.diffMap);
            SOKO_ASSERT(material.diffMap.gpuHandle, "");

            EndTemporaryMemory(gameState->tempArena);
            gameState->redPlateMaterial = material;
        }
        {
            i32 widthDiff;
            i32 heightDiff;
            i32 bppDiff;
            BeginTemporaryMemory(gameState->tempArena);
            unsigned char* diffBitmap = stbi_load("../res/portal_diff.png", &widthDiff, &heightDiff, &bppDiff, 3);
            i32 widthSpec;
            i32 heightSpec;
            i32 bppSpec;
            unsigned char* specBitmap = stbi_load("../res/portal_spec.png", &widthSpec, &heightSpec, &bppSpec, 3);


            Material material = {};
            material.diffMap.format = GL_RGB8;
            material.diffMap.width = widthDiff;
            material.diffMap.height = heightDiff;
            material.diffMap.data = diffBitmap;
            material.specMap.format = GL_RGB8;
            material.specMap.width = widthSpec;
            material.specMap.height = heightSpec;
            material.specMap.data = specBitmap;

            RendererLoadTexture(&material.diffMap);
            RendererLoadTexture(&material.specMap);
            SOKO_ASSERT(material.diffMap.gpuHandle, "");
            SOKO_ASSERT(material.specMap.gpuHandle, "");

            EndTemporaryMemory(gameState->tempArena);
            gameState->portalMaterial = material;
        }

        {
            byte bitmap[3];
            bitmap[0] = 128;
            bitmap[0] = 128;
            bitmap[0] = 128;

            i32 width = 1;
            i32 height = 1;
            i32 bpp = 3;

            Material material = {};
            material.diffMap.format = GL_RGB8;
            material.diffMap.width = width;
            material.diffMap.height = height;
            material.diffMap.data = bitmap;
            RendererLoadTexture(&material.diffMap);
            SOKO_ASSERT(material.diffMap.gpuHandle, "");

            gameState->spikesMaterial = material;
        }
        {
            i32 width;
            i32 height;
            i32 bpp;
            BeginTemporaryMemory(gameState->tempArena);
            unsigned char* diffBitmap = stbi_load("../res/button.png", &width, &height, &bpp, 3);

            Material material = {};
            material.diffMap.format = GL_RGB8;
            material.diffMap.width = width;
            material.diffMap.height = height;
            material.diffMap.data = diffBitmap;
            RendererLoadTexture(&material.diffMap);
            SOKO_ASSERT(material.diffMap.gpuHandle, "");

            EndTemporaryMemory(gameState->tempArena);
            gameState->buttonMaterial = material;
        }


        gameState->level.xDim = 64;
        gameState->level.yDim = 64;
        gameState->level.zDim = 3;
        gameState->level.entityCount = 1;

        u32 tileArraySize = gameState->level.xDim * gameState->level.yDim * gameState->level.zDim;

        auto* level = &gameState->level;

        for (u32 x = 0; x < level->xDim; x++)
        {
            for (u32 y = 0; y < level->yDim; y++)
            {
                auto[queryResult, tile] = GetTile(level, x, y, 0, gameState->memoryArena);
                SOKO_ASSERT(queryResult == TileQueryResult::Found);
                tile->value = TILE_VALUE_WALL;

                if ((x == 0) || (x == level->xDim - 1) ||
                    (y == 0) || (y == level->yDim - 1))
                {
                    auto[queryResult1, tile1] = GetTile(level, x, y, 1, gameState->memoryArena);
                    SOKO_ASSERT(queryResult1 == TileQueryResult::Found);
                    tile1->value = TILE_VALUE_WALL;
                }
            }
        }
#if 0
        Entity playerEntity = {};
        playerEntity.type = ENTITY_TYPE_PLAYER;
        playerEntity.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        playerEntity.coord = V3I(10, 10, 1);
        playerEntity.mesh = &gameState->cubeMesh;
        playerEntity.material = &gameState->tilePlayerMaterial;
        u32 playerEntityId = AddEntity(&gameState->level, playerEntity, gameState->memoryArena);

        auto* player = &gameState->player;
        player->level = &gameState->level;
        player->e = GetEntity(player->level, playerEntityId);
#endif

        Entity entity1 = {};
        entity1.type = ENTITY_TYPE_BLOCK;
        entity1.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity1.coord = V3I(5, 7, 1);
        entity1.mesh = &gameState->cubeMesh;
        entity1.material = &gameState->tileBlockMaterial;

        AddEntity(level, entity1, gameState->memoryArena);
        //AddEntity(playerLevel)

        Entity entity2 = {};
        entity2.type = ENTITY_TYPE_BLOCK;
        entity2.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity2.coord = V3I(5, 8, 1);
        entity2.mesh = &gameState->cubeMesh;
        entity2.material = &gameState->tileBlockMaterial;

        AddEntity(level, entity2, gameState->memoryArena);

        Entity entity3 = {};
        entity3.type = ENTITY_TYPE_BLOCK;
        entity3.flags = ENTITY_FLAG_COLLIDES | ENTITY_FLAG_MOVABLE;
        entity3.coord = V3I(5, 9, 1);
        entity3.mesh = &gameState->cubeMesh;
        entity3.material = &gameState->tileBlockMaterial;

        AddEntity(level, entity3, gameState->memoryArena);

        Entity plate = {};
        plate.type = ENTITY_TYPE_PLATE;
        plate.flags = 0;
        plate.coord = V3I(10, 9, 1);
        plate.mesh = &gameState->plateMesh;
        plate.material = &gameState->redPlateMaterial;

        AddEntity(level, plate, gameState->memoryArena);

        Entity portal1 = {};
        portal1.type = ENTITY_TYPE_PORTAL;
        portal1.flags = 0;
        portal1.coord = V3I(12, 12, 1);
        portal1.mesh = &gameState->portalMesh;
        portal1.material = &gameState->portalMaterial;
        portal1.portalDirection = DIRECTION_NORTH;

        Entity* portal1Entity = GetEntity(level, AddEntity(level, portal1, gameState->memoryArena));

        Entity portal2 = {};
        portal2.type = ENTITY_TYPE_PORTAL;
        portal2.flags = 0;
        portal2.coord = V3I(17, 17, 1);
        portal2.mesh = &gameState->portalMesh;
        portal2.material = &gameState->portalMaterial;
        portal2.portalDirection = DIRECTION_WEST;

        Entity* portal2Entity = GetEntity(level, AddEntity(level, portal2, gameState->memoryArena));

        portal1Entity->bindedPortalID = portal2Entity->id;
        portal2Entity->bindedPortalID = portal1Entity->id;

        AddEntity(level, ENTITY_TYPE_SPIKES, V3I(15, 15, 1),
                  &gameState->spikesMesh, &gameState->spikesMaterial, gameState->memoryArena);
        Entity* button = GetEntity(level, AddEntity(level, ENTITY_TYPE_BUTTON, V3I(4, 4, 1),
                                                    &gameState->buttonMesh, &gameState->buttonMaterial,
                                                    gameState->memoryArena));
        // TODO(emacs): Lambdas indenting
        button->updateProc = [](Level* level, Entity* entity, void* data)
            {
                GameState* gameState = (GameState*)data;
                AddEntity(level, ENTITY_TYPE_BLOCK, V3I(4, 5, 1),
                          &gameState->cubeMesh, &gameState->tileBlockMaterial,
                          gameState->memoryArena);
            };
        button->updateProcData = (void*)gameState;

        gameState->port = 9999;
        gameState->ipOctets[0] = 127;
        gameState->ipOctets[3] = 1;
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

    }

    void
    GameUpdate(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
    }

    void ShowNetSettings(GameState* gameState, AB::MemoryArena* arena)
    {
        bool open = true;
        bool windowCreated = false;
        ImGui::SetNextWindowSizeConstraints(ImVec2(400, 100), ImVec2(400, 200));
        if (ImGui::Begin("LAN", &open, ImGuiWindowFlags_AlwaysAutoResize))
        {
            windowCreated = true;
            if(ImGui::RadioButton("Client", gameState->gameMode == GAME_MODE_CLIENT)) { gameState->gameMode = GAME_MODE_CLIENT; }
            ImGui::SameLine();
            if(ImGui::RadioButton("Server", gameState->gameMode == GAME_MODE_SERVER)) { gameState->gameMode = GAME_MODE_SERVER; }
            ImGui::SameLine();
            if(ImGui::RadioButton("Single", gameState->gameMode == GAME_MODE_SINGLE)) { gameState->gameMode = GAME_MODE_SINGLE; }

            ImGui::Separator();
            if (gameState->gameMode == GAME_MODE_SERVER)
            {
                // TODO(emacs): Lambda indentiation in emacs
                auto numFilter = [](ImGuiInputTextCallbackData* data) -> int
                    {
                        return !(data->EventChar >= '0' && data->EventChar <='9');
                    };

                char buf[64] = "";
                FormatString(buf, 64, "%i16", gameState->port);
                ImGui::InputText("port", buf, 64, ImGuiInputTextFlags_CallbackCharFilter, numFilter);
                u64 val = strtol(buf, nullptr, 10);
                bool buttonEnabled = false;
                if (val > 1023 && val < 65536)
                {
                    gameState->port = (u16)val;
                }
                if (gameState->port > 1023 && gameState->port < 65536)
                {
                    buttonEnabled = true;
                }

                if(ImGui::Button("Create", ImVec2(50.0f, 20.0f)))
                {
                    if (buttonEnabled)
                    {
                        gameState->gameModeReadyToInit = true;
                    }
                }
            }
            else if (gameState->gameMode == GAME_MODE_CLIENT)
            {
                float width = ImGui::CalcItemWidth();
                ImGui::BeginGroup();
                ImGui::PushID("IP");
                ImGui::TextUnformatted("IP");
                ImGui::SameLine();
                bool inputSucceed = true;
                for (u32 i = 0; i < 4; i++)
                {
                    ImGui::PushItemWidth(width / 4.0f);
                    ImGui::PushID(i);
                    bool invalidOctet = false;
                    if (gameState->ipOctets[i] > 255)
                    {
                        // Make values over 255 red, and when focus is lost reset it to 255.
                        gameState->ipOctets[i] = 255;
                        invalidOctet = true;
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                    if (gameState->ipOctets[i] < 0)
                    {
                        // Make values below 0 yellow, and when focus is lost reset it to 0.
                        gameState->ipOctets[i] = 0;
                        invalidOctet = true;
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                    }
                    ImGui::InputInt("##v", (int*)&gameState->ipOctets[i], 0, 0, ImGuiInputTextFlags_CharsDecimal);
                    if (invalidOctet)
                    {
                        if (inputSucceed)
                        {
                            inputSucceed = false;
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::SameLine();


                    ImGui::PopID();
                    ImGui::PopItemWidth();
                }
                if (inputSucceed)
                {
                    gameState->ipAddress =
                        (gameState->ipOctets[0] << 24) |
                        (gameState->ipOctets[1] << 16) |
                        (gameState->ipOctets[2] << 8) |
                        (gameState->ipOctets[3]);
                }
                // TODO(emacs): Lambda indentiation in emacs
                auto numFilter = [](ImGuiInputTextCallbackData* data) -> int
                    {
                        return !(data->EventChar >= '0' && data->EventChar <='9');
                    };


                ImGui::Text(":");
                ImGui::SameLine();
                ImGui::PushItemWidth(width / 4.0f);
                char buf[64] = "";
                FormatString(buf, 64, "%i16", gameState->port);
                ImGui::InputText("", buf, 64, ImGuiInputTextFlags_CallbackCharFilter, numFilter);
                u64 val = strtol(buf, nullptr, 10);
                bool buttonEnabled = false;
                if (val > 1023 && val < 65536)
                {
                    gameState->port = (u16)val;
                }
                if (gameState->port > 1023 && gameState->port < 65536 && inputSucceed)
                {
                    buttonEnabled = true;
                }

                ImGui::PopID();
                ImGui::EndGroup();


                if(ImGui::Button("Create", ImVec2(50.0f, 20.0f)))
                {
                    if (buttonEnabled)
                    {
                        gameState->gameModeReadyToInit = true;
                    }
                }
            }
            else if (gameState->gameMode == GAME_MODE_SINGLE)
            {
                if(ImGui::Button("Create", ImVec2(50.0f, 20.0f)))
                {
                    gameState->gameModeReadyToInit = true;
                }
            }

            if (gameState->gameModeReadyToInit && !gameState->gameModeInitialized)
            {
                uptr socket = NetCreateSocket();
                SOKO_ASSERT(socket);

                if (gameState->gameMode == GAME_MODE_SERVER)
                {
                    SOKO_ASSERT(!gameState->server);
                    gameState->server = PUSH_STRUCT(arena, net::Server);
                    SOKO_ASSERT(gameState->server);
                    gameState->server->socket = socket;
                    gameState->server->port = gameState->port;
                    bool bindResult = NetBindSocket(gameState->server->socket,
                                                    gameState->server->port);
                    SOKO_ASSERT(bindResult);
                }
                else
                {
                    SOKO_ASSERT(!gameState->client);
                    gameState->client = PUSH_STRUCT(arena, net::Client);
                    SOKO_ASSERT(gameState->client);
                    gameState->client->socket = socket;
                    gameState->client->serverAddr.ip = gameState->ipAddress;
                    gameState->client->serverAddr.port = gameState->port;
                }

                gameState->gameModeReadyToInit = false;
                gameState->gameModeInitialized = true;
            }
        }

        ImGui::End();
    }

    void
    GameRender(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
        auto* gameState = _GlobalStaticStorage->gameState;
        bool show = true;

        DrawOverlay(gameState);
        BeginDebugOverlay();
        //ImGui::ShowDemoWindow(&show);
        DebugOverlayPushStr("Hello!");
        DEBUG_OVERLAY_TRACE(gameState->camera.conf.position);
        DEBUG_OVERLAY_TRACE(gameState->level.tileCount);
        DEBUG_OVERLAY_TRACE(gameState->level.freeTileCount);
        DEBUG_OVERLAY_TRACE(gameState->level.platePressed);
        DEBUG_OVERLAY_TRACE(gameState->level.entityCount);
        DEBUG_OVERLAY_TRACE(gameState->level.deletedEntityCount);
        ShowNetSettings(gameState, arena);

        if (gameState->gameModeInitialized)
        {
            if (gameState->gameMode == GAME_MODE_SERVER)
            {
                if (!gameState->controlledPlayer)
                {
                    gameState->controlledPlayer = AddPlayer(gameState, V3I(10, 10, 1), arena);
                    gameState->server->slotsOccupancy[net::Server::PLAYER_SLOT] = 1;
                    gameState->server->slots[net::Server::PLAYER_SLOT].player = gameState->controlledPlayer;
                }

                byte* netBuffer = gameState->server->socketBuffer;

                while (true)
                {
                    auto[rcStatus, rcSize, rcFrom] =
                        NetRecieve(gameState->server->socket, netBuffer, net::Server::SOCKET_BUFFER_SIZE);
                    if (rcStatus == AB::NetRecieveResult::Success && rcSize) // TODO: empty packets
                    {
                        auto header = (net::ClientMsgHeader*)netBuffer;
                        switch (header->type)
                        {
                        case net::ClientMsg_Join:
                        {
                            i16 clientSlot = -1;
                            u32 packetSize = 0;
                            for (u32 i = 0; i < net::Server::SLOTS_NUM; i++)
                            {
                                if (!gameState->server->slotsOccupancy[i])
                                {
                                    clientSlot = i;
                                }
                            }

                            u32 netBufferAt = 0;
                            netBuffer[netBufferAt] = (byte)net::ServerMsg_JoinResult;
                            netBufferAt += 1;

                            auto msg = (net::ServerJoinResultMsg*)(netBuffer + 1);
                            netBufferAt += sizeof(net::ServerJoinResultMsg);

                            bool slotInitialized = false;
                            if (clientSlot != -1)
                            {
                                i32 playerX = 13;
                                i32 playerY = 13;
                                i32 playerZ = 1;

                                Player* player = AddPlayer(gameState, V3I(playerX, playerY, playerZ), arena);
                                SOKO_ASSERT(player);

                                if (player)
                                {
                                    gameState->server->slotsOccupancy[clientSlot] = 1;
                                    gameState->server->slots[clientSlot].address = rcFrom;
                                    gameState->server->slots[clientSlot].player = player;

                                    msg->succeed = 1;
                                    msg->newPlayer = {clientSlot, playerX, playerY, playerZ};

                                    for (i16 otherSlot = 0;
                                         otherSlot < net::Server::SLOTS_NUM;
                                         otherSlot++)
                                    {
                                        if (otherSlot != clientSlot)
                                        {
                                            if (gameState->server->slotsOccupancy[otherSlot])
                                            {
                                                net::ServerSlot* s = gameState->server->slots + otherSlot;
                                                auto data = (net::NewPlayerData*)(netBuffer + netBufferAt);
                                                netBufferAt += sizeof(net::NewPlayerData);

                                                data->slot = otherSlot;
                                                data->x = s->player->e->coord.x;
                                                data->y = s->player->e->coord.y;
                                                data->z = s->player->e->coord.z;
                                                msg->otherPlayersCount++;
                                            }
                                        }
                                    }
                                    slotInitialized = true;
                                }
                            }
                            if (!slotInitialized)
                            {
                                msg->succeed = 0;
                            }

                            packetSize = netBufferAt;

                            auto[sndStatus, sndSize] = NetSend(gameState->server->socket, rcFrom, netBuffer, packetSize);
                            if (!sndStatus && slotInitialized)
                            {
                                // TODO: Message: failed to connect client
                                gameState->server->slotsOccupancy[clientSlot] = 0;
                            }
                        } break;
                        case net::ClientMsg_Leave:
                        {
                            // TODO: Check for bounds
                            gameState->server->slotsOccupancy[header->slot] = 0;
                        } break;
                        case net::ClientMsg_PlayerAction:
                        {
                            net::ServerSlot* slot = gameState->server->slots + header->slot;
                            u32 recvInputSize = rcSize - sizeof(net::ClientMsgHeader);
                            // TODO: Handle case when recieved input is too big
                            SOKO_ASSERT(recvInputSize <= (net::InputBuffer::SIZE - slot->inputBuffer.at));
                            COPY_BYTES(recvInputSize, slot->inputBuffer.base + slot->inputBuffer.at, netBuffer + sizeof(net::ClientMsgHeader));
                            slot->inputBuffer.at += recvInputSize;
                        } break;
                        INVALID_DEFAULT_CASE;
                        }
                    }
                    else if (rcStatus == AB::NetRecieveResult::Nothing)
                    {
                        break;
                    }
                    else
                    {
                        INVALID_CODE_PATH;
                    }
                }

                SOKO_ASSERT(!gameState->server->slots[net::Server::PLAYER_SLOT].inputBuffer.at);
                net::CollectPlayerInput(&gameState->server->slots[net::Server::PLAYER_SLOT].inputBuffer);

                for (u32 i = 0; i < net::Server::SLOTS_NUM; i++)
                {
                    b32 slotOccupied = gameState->server->slotsOccupancy[i];
                    net::ServerSlot* slot = gameState->server->slots + i;
                    if (slotOccupied && slot->inputBuffer.at)
                    {
                        for (u32 inputIndex = 0;
                             inputIndex < slot->inputBuffer.at;
                             inputIndex++)
                        {
                            PlayerAction action = (PlayerAction)slot->inputBuffer.base[inputIndex];
                            if (ActionIsMovement(action))
                            {
                                MoveEntity(&gameState->level,
                                           slot->player->e,
                                           (Direction)action, arena,
                                           slot->player->reversed);
                            }
                            else
                            {
                                switch (action)
                                {
                                case PlayerAction_ToggleInteractionMode:
                                {
                                    slot->player->reversed = !slot->player->reversed;
                                } break;
                                INVALID_DEFAULT_CASE;
                                }
                            }
                        }
                        auto header = (net::ServerMsgHeader*)netBuffer;
                        header->type = net::ServerMsg_PlayerAction;

                        auto msg = (net::ServerPlayerActionMsg*)(netBuffer + sizeof(net::ServerMsgHeader));

                        msg->slot = (i16)i;

                        u32 inputOffset = sizeof(net::ServerMsgHeader) + sizeof(net::ServerPlayerActionMsg);
                        // TODO: Check for buffer overflow
                        COPY_BYTES(slot->inputBuffer.at, netBuffer + inputOffset, slot->inputBuffer.base);
                        inputOffset += slot->inputBuffer.at;
                        for (u32 sendIndex = 1; sendIndex < net::Server::SLOTS_NUM; sendIndex++)
                        {
                            if (gameState->server->slotsOccupancy[sendIndex])
                            {
                                net::ServerSlot* sendSlot = gameState->server->slots + sendIndex;
                                auto[status, size] = NetSend(gameState->server->socket,
                                                             sendSlot->address,
                                                             netBuffer, inputOffset);
                                SOKO_ASSERT(status);
                            }
                        }
                        slot->inputBuffer.at = 0;
                    }
                }
            }
            else if (gameState->gameMode == GAME_MODE_CLIENT)
            {
                byte* netBuffer = gameState->client->socketBuffer;
                u32 netBufferAt = 0;

                // TODO: This is totally crappy hack
                static f32 timeToWait = 30.0f;

                if (!gameState->controlledPlayer && timeToWait >= 29.99f)
                {
                    netBuffer[netBufferAt] = net::ClientMsg_Join;
                    netBufferAt += 1;

                    auto[sndStatus, sndSize] = NetSend(gameState->client->socket,
                                                       gameState->client->serverAddr,
                                                       netBuffer, netBufferAt);
                    // TODO: Try again!
                    SOKO_ASSERT(sndStatus);
                }

                // TODO: Refactor this!!!
                if (timeToWait > 0.0f && !gameState->controlledPlayer)
                {
                    timeToWait -= GlobalAbsDeltaTime;
                    auto[recvStatus, recvSize, recvFrom] =
                        NetRecieve(gameState->client->socket, netBuffer, net::Server::SOCKET_BUFFER_SIZE);
                    if (recvStatus == AB::NetRecieveResult::Success && recvSize)
                    {
                        if (netBuffer[0] == net::ServerMsg_JoinResult)
                        {
                            auto msg = (net::ServerJoinResultMsg*)(netBuffer + 1);
                            netBufferAt = sizeof(net::ServerJoinResultMsg) + 1;
                            if (msg->succeed)
                            {
                                v3i playerCoord = V3I(msg->newPlayer.x, msg->newPlayer.y, msg->newPlayer.z);
                                Player* player = AddPlayer(gameState, playerCoord, arena);
                                SOKO_ASSERT(player);
                                gameState->controlledPlayer = player;
                                gameState->client->playerSlot = msg->newPlayer.slot;

                                // TODO: Check for overflow
                                gameState->client->slotsOccupancy[gameState->client->playerSlot] = 1;
                                gameState->client->slots[gameState->client->playerSlot].player = player;

                                netBufferAt = sizeof(net::ServerJoinResultMsg) + 1;
                                while (netBufferAt < recvSize)
                                {
                                    auto nextPlayer = (net::NewPlayerData*)(netBuffer + netBufferAt);
                                    netBufferAt += sizeof(net::NewPlayerData);

                                    v3i coord = V3I(nextPlayer->x, nextPlayer->y, nextPlayer->z);
                                    Player* player = AddPlayer(gameState, coord, arena);
                                    SOKO_ASSERT(player);
                                    // TODO: Check for overflow
                                    gameState->client->slotsOccupancy[nextPlayer->slot] = 1;
                                    gameState->client->slots[nextPlayer->slot].player = player;
                                }
                            }
                        }
                    }
                }
                else
                {
                    // TODO: Handle connection failing
                    SOKO_ASSERT(gameState->client->playerSlot);

                    SOKO_ASSERT(!gameState->client->slots[gameState->client->playerSlot].inputBuffer.at);
                    auto* playerInput = &gameState->client->slots[gameState->client->playerSlot].inputBuffer;
                    net::CollectPlayerInput(playerInput);

                    auto header = (net::ClientMsgHeader*)netBuffer;
                    header->type = net::ClientMsg_PlayerAction;
                    header->slot = gameState->client->playerSlot;

                    u32 netBufferAt = sizeof(net::ClientMsgHeader);
                    // TODO: Check for buffer overflow
                    COPY_BYTES(playerInput->at,
                               netBuffer + netBufferAt, playerInput->base);
                    netBufferAt += playerInput->at;
                    auto[status, size] = NetSend(gameState->client->socket, gameState->client->serverAddr, netBuffer, netBufferAt);
                    SOKO_ASSERT(status);
                    // TODO: Use some temporary buffer instead of player input buffer
                    playerInput->at = 0;

                    while (true)
                    {
                        auto[rcStatus, rcSize, rcFrom] = NetRecieve(gameState->client->socket, netBuffer,
                                                                    net::Server::SOCKET_BUFFER_SIZE);
                        if (rcStatus == AB::NetRecieveResult::Success && rcSize) // TODO: empty packets
                        {
                            auto header = (net::ServerMsgHeader*)netBuffer;
                            switch (header->type)
                            {
                            case net::ServerMsg_PlayerAction:
                            {
                                auto msg = (net::ServerPlayerActionMsg*)(netBuffer + sizeof(net::ServerMsgHeader));

                                net::ClientSlot* s = gameState->client->slots + msg->slot;
                                // TODO: Validate _of_do_something_ with controlled
                                // player input
                                u32 offset = sizeof(net::ServerMsgHeader) + sizeof(net::ServerPlayerActionMsg);
                                COPY_BYTES(rcSize - offset, s->inputBuffer.base, netBuffer + offset);
                                s->inputBuffer.at += rcSize - offset;
                            }
                            break;
                            case net::ServerMsg_AddPlayer: {} break;
                                INVALID_DEFAULT_CASE;
                            }
                        }
                        else if (rcStatus == AB::NetRecieveResult::Nothing)
                        {
                            break;
                        }
                        else
                        {
                            INVALID_CODE_PATH;
                        }
                    }

                    // TODO: Counter of connected players
                    for (u32 i = 0; i < net::Server::SLOTS_NUM; i++)
                    {
                        net::ClientSlot* slot = gameState->client->slots + i;
                        bool slotOccupied = gameState->client->slotsOccupancy[i];
                        if (slotOccupied && slot->inputBuffer.at)
                        {
                            for (u32 inputIndex = 0;
                                 inputIndex < slot->inputBuffer.at;
                                 inputIndex++)
                            {
                                // TODO: Stop passing keycodes through connection
                                PlayerAction action = (PlayerAction)slot->inputBuffer.base[inputIndex];
                                if (ActionIsMovement(action))
                                {
                                    MoveEntity(&gameState->level,
                                               slot->player->e,
                                               (Direction)action, arena,
                                               slot->player->reversed);
                                }
                                else
                                {
                                    switch (action)
                                    {
                                    case PlayerAction_ToggleInteractionMode:
                                    {
                                        slot->player->reversed = !slot->player->reversed;
                                    } break;
                                    INVALID_DEFAULT_CASE;
                                    }
                                }
                            }
                            slot->inputBuffer.at = 0;
                        }
                    }
                }
            }
            else if (gameState->gameMode == GAME_MODE_SINGLE)
            {
                if (!gameState->controlledPlayer)
                {
                    gameState->controlledPlayer = AddPlayer(gameState, V3I(10, 10, 1), arena);
                }
                Player* player = gameState->controlledPlayer;
                if (JustPressed(AB::KEY_SPACE))
                {
                    player->reversed = ! player->reversed;
                }

                if (JustPressed(AB::KEY_UP))
                {
                    MoveEntity(&gameState->level, player->e, DIRECTION_NORTH, arena, player->reversed);
                }

                if (JustPressed(AB::KEY_DOWN))
                {
                    MoveEntity(&gameState->level, player->e, DIRECTION_SOUTH, arena, player->reversed);
                }

                if (JustPressed(AB::KEY_RIGHT))
                {
                    MoveEntity(&gameState->level, player->e, DIRECTION_WEST, arena, player->reversed);
                }

                if (JustPressed(AB::KEY_LEFT))
                {
                    MoveEntity(&gameState->level, player->e, DIRECTION_EAST, arena, player->reversed);
                }
            }
            else
            {
                INVALID_CODE_PATH;
            }
        }

        if (JustPressed(AB::KEY_F1))
        {
            gameState->useDebugCamera = !gameState->useDebugCamera;
        }

        CameraConfig* camConf = NULL;
        if (gameState->useDebugCamera)
        {
            UpdateCamera(&gameState->debugCamera);
            camConf = &gameState->debugCamera.conf;
        }
        else
        {
            UpdateCamera(&gameState->camera);
            camConf = &gameState->camera.conf;
        }

        RenderGroupSetCamera(gameState->renderGroup, camConf);

        RendererBeginFrame(gameState->renderer, V2(PlatformGlobals.windowWidth, PlatformGlobals.windowHeight));
        DirectionalLight light = {};
        light.dir = Normalize(V3(0.2f, -0.9f, -0.3f));
        light.ambient = V3(0.3f);
        light.diffuse = V3(0.8f);
        light.specular = V3(1.0f);
        RenderCommandSetDirLight lightCommand = {};
        lightCommand.light = light;
        RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_SET_DIR_LIGHT,
                               (void*)&lightCommand);



#if 0
        if ((IsDown(AB::KEY_ENTER) ||
             IsDown(AB::KEY_NUM8)  ||
             IsDown(AB::KEY_NUM5)  ||
             IsDown(AB::KEY_NUM4)  ||
             IsDown(AB::KEY_NUM6)) &&
            !gameState->player2Active)
        {
            gameState->player2Active = true;
        }
        else
        {
            Player* player = gameState->player2;
            if (JustPressed(AB::KEY_ENTER))
            {
                player->reversed = ! player->reversed;
            }

            if (JustPressed(AB::KEY_NUM8) &&
                (player->inputFlags & PENDING_MOVEMENT_BIT_FORWARD) == 0)
            {
                player->inputFlags |= PENDING_MOVEMENT_BIT_FORWARD;
                MoveEntity(&gameState->level, player->e, DIRECTION_NORTH, arena, player->reversed);
            }

            if (JustPressed(AB::KEY_NUM5) &&
                (player->inputFlags & PENDING_MOVEMENT_BIT_BACKWARD) == 0)
            {
                player->inputFlags |= PENDING_MOVEMENT_BIT_BACKWARD;
                MoveEntity(&gameState->level, player->e, DIRECTION_SOUTH, arena, player->reversed);
            }

            if (JustPressed(AB::KEY_NUM6) &&
                (player->inputFlags & PENDING_MOVEMENT_BIT_RIGHT) == 0)
            {
                player->inputFlags |= PENDING_MOVEMENT_BIT_RIGHT;
                MoveEntity(&gameState->level, player->e, DIRECTION_WEST, arena, player->reversed);
            }

            if (JustPressed(AB::KEY_NUM4) &&
                (player->inputFlags & PENDING_MOVEMENT_BIT_LEFT) == 0)
            {
                player->inputFlags |= PENDING_MOVEMENT_BIT_LEFT;
                MoveEntity(&gameState->level, player->e, DIRECTION_EAST, arena, player->reversed);
            }

            if (JustReleased(AB::KEY_NUM8))
            {
                player->inputFlags &= ~PENDING_MOVEMENT_BIT_FORWARD;
            }
            if (JustReleased(AB::KEY_NUM5))
            {
                player->inputFlags &= ~PENDING_MOVEMENT_BIT_BACKWARD;
            }
            if (JustReleased(AB::KEY_NUM6))
            {
                player->inputFlags &= ~PENDING_MOVEMENT_BIT_RIGHT;
            }
            if (JustReleased(AB::KEY_NUM4))
            {
                player->inputFlags &= ~PENDING_MOVEMENT_BIT_LEFT;
            }
        }
#endif



        DrawLevel(&gameState->level, gameState);
        //DrawPlayer(&gameState->player, gameState);
        DrawEntities(&gameState->level, gameState);
        FlushRenderGroup(gameState->renderer, gameState->renderGroup);
    }
}

#include "RenderGroup.cpp"
#include "Renderer.cpp"

// NOTE: IMGUI
#include "imgui/imconfig.h"
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

