#define HYPERMATH_IMPL
#include "hypermath.h"

#include "Sokoban.h"
#include "Memory.h"

#include "FileFormats.h"

namespace soko
{
	struct GameState;
	
	struct StaticStorage
	{
		AB::MemoryArena* tempArena;
		GameState* gameState;
	};

	static AB::PlatformState* _GlobalPlatform;
	static StaticStorage* _GlobalStaticStorage;

	void GameInit(AB::MemoryArena* arena,  AB::PlatformState* platform);
	void GameReload(AB::MemoryArena* arena,  AB::PlatformState* platform);
	void GameUpdate(AB::MemoryArena* arena,  AB::PlatformState* platform);
	void GameRender(AB::MemoryArena* arena,  AB::PlatformState* platform);

}
	
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
#define LogAssert SOKO_PLATFORM_FUNCTION(LogAssert)

#define GL_FUNCTION(func) _GlobalPlatform->gl->_##func

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

// TODO:: Asserts without message
// NOTE: Panic macro should not be stripped in release build
#if defined(AB_COMPILER_CLANG)
#define SOKO_INFO(format, ...) Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define SOKO_ASSERT(expr, fmt, ...) do { if (!(expr)) {Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, fmt, ##__VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#define SOKO_PANIC(format, ...) do{ Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__); abort();} while(false)
#else
#define SOKO_INFO(format, ...) Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, __VA_ARGS__)
#define SOKO_ASSERT(expr, fmt, ...) do { if (!(expr)) {LogAssert(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, fmt, __VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#define SOKO_PANIC(format, ...) do{ Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, __VA_ARGS__); abort();} while(false)
#endif
#define SOKO_INVALID_DEFAULT_CASE default:{ SOKO_ASSERT(false, "Invalid default case."); }break
#define SOKO_INVALID_CODE_PATH SOKO_ASSERT(false, "Invalid code path.")

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
	SOKO_INVALID_DEFAULT_CASE;
	}
}
namespace soko
{
		
	void GameInit(AB::MemoryArena* arena,
				  AB::PlatformState* platform)
	{
		_GlobalStaticStorage = (StaticStorage*)PUSH_SIZE(arena, KILOBYTES(1));
		SOKO_ASSERT(_GlobalStaticStorage == arena->begin, 0);
		_GlobalPlatform = platform;

		_GlobalPlatform->gameSpeed = 1.0f;

		_GlobalStaticStorage->tempArena = AllocateSubArena(arena, arena->size / 2);
		SOKO_ASSERT(_GlobalStaticStorage->tempArena, "Failed to allocate tempArena.");
		_GlobalStaticStorage->gameState = PUSH_STRUCT(arena, GameState);
		SOKO_ASSERT(_GlobalStaticStorage->gameState, "");

		GameState* gameState = _GlobalStaticStorage->gameState;
		gameState->renderer = AllocAndInitRenderer(arena);
		gameState->renderGroup = AllocateRenderGroup(arena, KILOBYTES(4), 512);

		CameraConfig camera = {};
		camera.position = V3(0.0f);
		camera.front = V3(0.0f, 0.0f, -1.0f);
		camera.fovDeg = 45.0f;
		camera.aspectRatio = 16.0f / 9.0f;
		camera.nearPlane = 0.1f;
		camera.farPlane = 10.0f;

		RenderGroupSetCamera(gameState->renderGroup, &camera);

		gameState->renderer->clearColor = V4(0.8f, 0.8f, 0.8f, 1.0f);

		u32 fileSize = DebugGetFileSize(L"../res/manipulator.aab");
		void* fileData = PUSH_SIZE(arena, fileSize);
		u32 result = DebugReadFile(fileData, fileSize, L"../res/manipulator.aab");
		// NOTE: Strict aliasing
		auto header = (AABMeshHeader*)fileData;

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

		SOKO_ASSERT(header->magicValue == AAB_FILE_MAGIC_VALUE, "");
		PrintString("File size : %u32\n", fileSize);

		gameState->mesh = mesh;
	}
	
	void GameReload(AB::MemoryArena* arena, AB::PlatformState* platform)
	{
		_GlobalPlatform = platform;
		_GlobalStaticStorage = (StaticStorage*)arena->begin;
	}

	void GameUpdate(AB::MemoryArena* arena, AB::PlatformState* platform)
	{
		SOKO_INFO("Sokoban!");
	}

	void GameRender(AB::MemoryArena* arena, AB::PlatformState* platform)
	{
		auto* gameState = _GlobalStaticStorage->gameState;

		RendererBeginFrame(gameState->renderer, V2(1280.0f, 800.0f));
		DrawStraightLine(gameState->renderGroup, V3(0.0f, 0.0f, -1.0f), V3(16.0f, 9.0f, -1.0f), V3(1.0f, 0.0f, 0.0f), 4.0f);
		RenderCommandDrawMesh command = {};
		command.transform = Scaling(V3(0.8f));
		command.mesh = &gameState->mesh;
		RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
							   (void*)&command);
		FlushRenderGroup(gameState->renderer, gameState->renderGroup);
	}
}

#include "RenderGroup.cpp"
#include "Renderer.cpp"
