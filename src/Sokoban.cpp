#define HYPERMATH_IMPL
#include "hypermath.h"
#undef HYPERMATH_IMPL
#include "Constants.h"
#include "Memory.h"
#include "Platform.h"

#include "RenderGroup.h"
#include "Camera.h"
#include "GameSession.h"
#include "Sokoban.h"

#include "FileFormats.h"
#include "DebugOverlay.h"

using namespace AB;

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
#define DebugWriteFile SOKO_PLATFORM_FUNCTION(DebugWriteFile)
#define DebugOpenFile SOKO_PLATFORM_FUNCTION(DebugOpenFile)
#define DebugCloseFile SOKO_PLATFORM_FUNCTION(DebugCloseFile)
#define DebugWriteToOpenedFile  SOKO_PLATFORM_FUNCTION(DebugWriteToOpenedFile)
#define FormatString SOKO_PLATFORM_FUNCTION(FormatString)
#define PrintString SOKO_PLATFORM_FUNCTION(PrintString)
#define Log SOKO_PLATFORM_FUNCTION(Log)
#define LogAssertV SOKO_PLATFORM_FUNCTION(LogAssertV)
#define SetInputMode SOKO_PLATFORM_FUNCTION(SetInputMode)
#define NetCreateSocket SOKO_PLATFORM_FUNCTION(NetCreateSocket)
#define NetCloseSocket SOKO_PLATFORM_FUNCTION(NetCloseSocket)
#define NetBindSocket SOKO_PLATFORM_FUNCTION(NetBindSocket)
#define NetSend SOKO_PLATFORM_FUNCTION(NetSend)
#define NetRecieve SOKO_PLATFORM_FUNCTION(NetRecieve)
#define QueryNewArena SOKO_PLATFORM_FUNCTION(QueryNewArena)
#define FreeArena SOKO_PLATFORM_FUNCTION(FreeArena)

#define PLATFORM_QUERY_NEW_ARENA(size) QueryNewArena(size)
#define PLATFORM_FREE_ARENA(arena) FreeArena(arena)

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

#include "imgui/imgui.h"
//#include "imgui/imgui_internal.h"

#include "Renderer.cpp"
#include "Level.cpp"
#include "Entity.cpp"
#include "DebugOverlay.cpp"
#include "Camera.cpp"
#include "Player.cpp"
#include "Network.cpp"
#include "MeshGen.cpp"
#include "GameMenu.cpp"
#include "GameSession.cpp"
#include "SimRegion.cpp"


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

        gameState->renderer = AllocAndInitRenderer(arena, tempArena);
        gameState->renderGroup = AllocateRenderGroup(arena, KILOBYTES(1024), 16384);

        gameState->renderer->clearColor = V4(0.8f, 0.8f, 0.8f, 1.0f);
        {
            u32 fileSize = DebugGetFileSize(L"../res/cube.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/cube.aab");
            // NOTE: Strict aliasing
            auto header = (AB::AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AB::AAB_FILE_MAGIC_VALUE, "");

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

            gameState->meshes[EntityMesh_Cube] = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/plate.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/plate.aab");
            // NOTE: Strict aliasing
            auto header = (AB::AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AB::AAB_FILE_MAGIC_VALUE, "");

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

            gameState->meshes[EntityMesh_Plate] = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/portal.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/portal.aab");
            // NOTE: Strict aliasing
            auto header = (AB::AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AB::AAB_FILE_MAGIC_VALUE, "");

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

            gameState->meshes[EntityMesh_Portal] = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/spikes.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/spikes.aab");
            // NOTE: Strict aliasing
            auto header = (AB::AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AB::AAB_FILE_MAGIC_VALUE, "");

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

            gameState->meshes[EntityMesh_Spikes] = mesh;
        }
        {
            u32 fileSize = DebugGetFileSize(L"../res/button.aab");
            void* fileData = PUSH_SIZE(arena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, L"../res/button.aab");
            // NOTE: Strict aliasing
            auto header = (AB::AABMeshHeader*)fileData;
            SOKO_ASSERT(header->magicValue == AB::AAB_FILE_MAGIC_VALUE, "");

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

            gameState->meshes[EntityMesh_Button] = mesh;
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
            gameState->materials[EntityMaterial_Tile] = material;
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
            gameState->materials[EntityMaterial_Player] = material;
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
            gameState->materials[EntityMaterial_Block] = material;
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
            gameState->materials[EntityMaterial_RedPlate] = material;
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
            gameState->materials[EntityMaterial_Portal] = material;
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

            gameState->materials[EntityMaterial_Spikes] = material;
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
            gameState->materials[EntityMaterial_Button] = material;
        }

        {
            BeginTemporaryMemory(gameState->tempArena);

            i32 backWidth, backHeight, backBpp;
            unsigned char* backData = stbi_load("../res/skybox/sky_back.png", &backWidth, &backHeight, &backBpp, 3);

            i32 downWidth, downHeight, downBpp;
            unsigned char* downData = stbi_load("../res/skybox/sky_down.png", &downWidth, &downHeight, &downBpp, 3);

            i32 frontWidth, frontHeight, frontBpp;
            unsigned char* frontData = stbi_load("../res/skybox/sky_front.png", &frontWidth, &frontHeight, &frontBpp, 3);

            i32 leftWidth, leftHeight, leftBpp;
            unsigned char* leftData = stbi_load("../res/skybox/sky_left.png", &leftWidth, &leftHeight, &leftBpp, 3);

            i32 rightWidth, rightHeight, rightBpp;
            unsigned char* rightData = stbi_load("../res/skybox/sky_right.png", &rightWidth, &rightHeight, &rightBpp, 3);

            i32 upWidth, upHeight, upBpp;
            unsigned char* upData = stbi_load("../res/skybox/sky_up.png", &upWidth, &upHeight, &upBpp, 3);

            CubeTexture texture = {};
            texture.back.format = GL_RGB8;
            texture.back.width = backWidth;
            texture.back.height = backHeight;
            texture.back.data = backData;

            texture.down.format = GL_RGB8;
            texture.down.width = downWidth;
            texture.down.height = downHeight;
            texture.down.data = downData;

            texture.front.format = GL_RGB8;
            texture.front.width = frontWidth;
            texture.front.height = frontHeight;
            texture.front.data = frontData;

            texture.left.format = GL_RGB8;
            texture.left.width = leftWidth;
            texture.left.height = leftHeight;
            texture.left.data = leftData;

            texture.right.format = GL_RGB8;
            texture.right.width = rightWidth;
            texture.right.height = rightHeight;
            texture.right.data = rightData;

            texture.up.format = GL_RGB8;
            texture.up.width = upWidth;
            texture.up.height = upHeight;
            texture.up.data = upData;

            RendererLoadCubeTexture(&texture);
            SOKO_ASSERT(texture.gpuHandle);

            EndTemporaryMemory(gameState->tempArena);
            gameState->skybox = texture;
        }

        gameState->renderGroup->drawSkybox = true;
        gameState->renderGroup->skyboxHandle = gameState->skybox.gpuHandle;

        //auto* level = gameState->level;
        //gameState->port = 9999;
        //gameState->ipOctets[0] = 127;
        //gameState->ipOctets[3] = 1;
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
        auto* gameState = _GlobalStaticStorage->gameState;
        Level* level = gameState->session.level;

        if (gameState->globalGameMode != GAME_MODE_MENU)
        {
        }
    }


    void
    DoOtherStuff(GameState* gameState)
    {
        AB::MemoryArena* arena = gameState->memoryArena;
        bool show = true;

        DrawOverlay(gameState);
        //ImGui::ShowDemoWindow(&show);
        DEBUG_OVERLAY_TRACE(gameState->session.debugCamera.conf.position);
        DEBUG_OVERLAY_TRACE(gameState->session.debugCamera.conf.front);

        //DEBUG_OVERLAY_TRACE(gameState->camera.conf.position);
        //DEBUG_OVERLAY_TRACE(gameState->level->platePressed);
        //DEBUG_OVERLAY_TRACE(gameState->level->entityCount);
        //DEBUG_OVERLAY_TRACE(gameState->level->deletedEntityCount);

        if (gameState->globalGameMode == GAME_MODE_SERVER)
        {
            net::Server* server = gameState->session.server;
            net::ServerPollInputMessages(gameState, server);

            // TODO: Move input buffers to player
            SOKO_ASSERT(!server->slots[net::SERVER_LOCAL_PLAYER_SLOT].inputBuffer.at);
            CollectPlayerInput(&server->slots[net::SERVER_LOCAL_PLAYER_SLOT].inputBuffer);

            net::ServerSendOutputMessages(gameState, gameState->session.level, server);
        }
        else if (gameState->globalGameMode == GAME_MODE_CLIENT)
        {
            net::Client* client = gameState->session.client;
            byte* netBuffer = client->socketBuffer;
            u32 netBufferAt = 0;

            {
#if 0
                // TODO: @ShouldDisconnect
                if (gameState->shouldDisconnect)
                {
                    auto header = (ClientMsgHeader*)netBuffer;
                    header->type = ClientMsg_Leave;
                    header->slot = client->playerSlot;
                    auto[status, size] = NetSend(client->socket, client->serverAddr, netBuffer, sizeof(ClientMsgHeader));
                    SOKO_ASSERT(status);
                }
                else
#endif
                {

                    // TODO: Handle connection failing
                    SOKO_ASSERT(client->playerSlot);

                    SOKO_ASSERT(!client->slots[client->playerSlot].inputBuffer.at);
                    auto* playerInput = &client->slots[client->playerSlot].inputBuffer;
                    net::CollectPlayerInput(playerInput);

                    auto header = (ClientMsgHeader*)netBuffer;
                    header->type = ClientMsg_PlayerAction;
                    header->slot = client->playerSlot;

                    u32 netBufferAt = sizeof(ClientMsgHeader);
                    // TODO: Check for buffer overflow
                    COPY_BYTES(playerInput->at,
                               netBuffer + netBufferAt, playerInput->base);
                    netBufferAt += playerInput->at;
                    auto[status, size] = NetSend(client->socket, client->serverAddr, netBuffer, netBufferAt);
                    SOKO_ASSERT(status);
                    // TODO: Use some temporary buffer instead of player input buffer
                    playerInput->at = 0;

                    while (true)
                    {
                        auto[rcStatus, rcSize, rcFrom] = NetRecieve(client->socket, netBuffer,
                                                                    net::SERVER_SOCKET_BUFFER_SIZE);
                        if (rcStatus == AB::NetRecieveResult::Success && rcSize) // TODO: empty packets
                        {
                            auto header = (ServerMsgHeader*)netBuffer;
                            switch (header->type)
                            {
                            case ServerMsg_DeletePlayer:
                            {
                                auto msg = (ServerDeletePlayerMsg*)(netBuffer + sizeof(ServerMsgHeader));
                                if (msg->slot >= 0 && msg->slot < net::SERVER_SLOTS_NUM)
                                {
                                    bool occupied = client->slotsOccupancy[msg->slot];
                                    SOKO_ASSERT(occupied);
                                    client->slotsOccupancy[msg->slot] = false;
                                    net::ClientSlot* s = client->slots + msg->slot;
                                    DeletePlayer(&gameState->session, s->player);
                                }
                            } break;
                            case ServerMsg_AddPlayer:
                            {
                                auto msg = (ServerAddPlayerMsg*)(netBuffer + sizeof(ServerMsgHeader));
                                SOKO_ASSERT(!client->slotsOccupancy[msg->newPlayer.slot]);
                                client->slotsOccupancy[msg->newPlayer.slot] = 1;
                                auto* s = client->slots + msg->newPlayer.slot;
                                iv3 coord = IV3(msg->newPlayer.x, msg->newPlayer.y, msg->newPlayer.z);
                                Player* player = AddPlayer(&gameState->session, coord);
                                SOKO_ASSERT(player);
                                s->player = player;
                            } break;
                            case ServerMsg_PlayerAction:
                            {
                                auto msg = (ServerPlayerActionMsg*)(netBuffer + sizeof(ServerMsgHeader));

                                net::ClientSlot* s = client->slots + msg->slot;
                                // TODO: Validate _of_do_something_ with controlled
                                // player input
                                u32 offset = sizeof(ServerMsgHeader) + sizeof(ServerPlayerActionMsg);
                                COPY_BYTES(rcSize - offset, s->inputBuffer.base, netBuffer + offset);
                                s->inputBuffer.at += rcSize - offset;
                            }
                            break;
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
                    for (u32 i = 0; i < net::SERVER_SLOTS_NUM; i++)
                    {
                        net::ClientSlot* slot = client->slots + i;
                        bool slotOccupied = client->slotsOccupancy[i];
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
# if 0 // TODO: Use sim regions
                                    MoveEntity(gameState->session.level,
                                               slot->player->e,
                                               (Direction)action,
                                               slot->player->reversed);
#endif
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
        }

        else if (gameState->globalGameMode == GAME_MODE_SINGLE)
        {
            BeginTemporaryMemory(gameState->tempArena, true);
            SimRegion* simRegion = BeginSim(gameState->tempArena,
                                            gameState->session.level,
                                            gameState->session.camera.worldPos,
                                            1);
            Player* player = gameState->session.controlledPlayer;
            //player->e->sim->pos += V3(GlobalInput.mouseFrameOffsetX, GlobalInput.mouseFrameOffsetY, 0.0f) * 7.0f;
            if (JustPressed(AB::KEY_SPACE))
            {
                player->reversed = !player->reversed;
            }

            if (JustPressed(AB::KEY_UP))
            {
                MoveEntity(gameState->session.level, simRegion, player->e->sim, Direction_North, player->e->movementSpeed, player->reversed);
            }

            if (JustPressed(AB::KEY_DOWN))
            {
                MoveEntity(gameState->session.level, simRegion, player->e->sim, Direction_South, player->e->movementSpeed, player->reversed);
            }

            if (JustPressed(AB::KEY_RIGHT))
            {
                MoveEntity(gameState->session.level, simRegion, player->e->sim, Direction_East, player->e->movementSpeed, player->reversed);
            }

            if (JustPressed(AB::KEY_LEFT))
            {
                MoveEntity(gameState->session.level, simRegion, player->e->sim, Direction_West, player->e->movementSpeed, player->reversed);
            }
# if 0
            for (u32 i = 0; i < LEVEL_ENTITY_TABLE_SIZE; i++)
            {
                Entity* e = gameState->session.level->entities[i];
                if (e)
                {
                    while (e)
                    {
                        UpdateEntity(gameState->session.level, e);
                        e = e->nextEntity;
                    }
                }
            }
#endif
            EndSim(gameState->session.level, simRegion);
            EndTemporaryMemory(gameState->tempArena);

        }
        else
        {
            INVALID_CODE_PATH;
        }

        if (JustPressed(AB::KEY_F1))
        {
            gameState->session.useDebugCamera = !gameState->session.useDebugCamera;
        }

        CameraConfig* camConf = 0;
        if (gameState->session.useDebugCamera)
        {
            UpdateCamera(&gameState->session.debugCamera);
            camConf = &gameState->session.debugCamera.conf;
        }
        else
        {
            UpdateCamera(&gameState->session.camera);
            camConf = &gameState->session.camera.conf;
        }

        RenderGroupSetCamera(gameState->renderGroup, camConf);

        RendererBeginFrame(gameState->renderer, V2(PlatformGlobals.windowWidth, PlatformGlobals.windowHeight));
        DirectionalLight light = {};
        light.dir = Normalize(V3(-0.3f, -1.0f, -1.0f));
        light.ambient = V3(0.3f);
        light.diffuse = V3(0.8f);
        light.specular = V3(1.0f);
        RenderCommandSetDirLight lightCommand = {};
        lightCommand.light = light;
        RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_SET_DIR_LIGHT,
                               (void*)&lightCommand);
#if 0
        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH, 0);
        RenderCommandPushChunkMesh c = {};
        c.offset = V3(0.0f);
        c.meshIndex = gameState->testChunkMesh;
        c.quadCount = gameState->testMeshQuadCount;
        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_PUSH_CHUNK_MESH, (void*)&c);
        RenderGroupPushCommand(gameState->renderGroup,
                               RENDER_COMMAND_END_CHUNK_MESH_BATCH, 0);
#endif

        DrawLevel(gameState->session.level, gameState);
        DrawEntities(gameState->session.level, gameState);
        FlushRenderGroup(gameState->renderer, gameState->renderGroup);
    }

    void
    GameRender(AB::MemoryArena* arena, AB::PlatformState* platform)
    {
        auto* gameState = _GlobalStaticStorage->gameState;
        BeginDebugOverlay();
        switch (gameState->globalGameMode)
        {
        case GAME_MODE_MENU: { MenuUpdateAndRender(&gameState->mainMenu, gameState); } break;
        default:
        {
            DoOtherStuff(gameState);
            if (DebugOverlayBeginCustom())
            {
                if (ImGui::Button("Exit to main menu", {150.0f, 20.0f}))
                {
                    DestroyGameSession(&gameState->session);
                    gameState->globalGameMode = GAME_MODE_MENU;

                }
                DebugOverlayEndCustom();
            }
        } break;
        }
    }
}

#include "RenderGroup.cpp"
//#include "Renderer.cpp"

// NOTE: IMGUI
#include "imgui/imconfig.h"
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"
