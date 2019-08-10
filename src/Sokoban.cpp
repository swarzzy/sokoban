#define HYPERMATH_IMPL
#include "hypermath.h"
#undef HYPERMATH_IMPL


#include "Sokoban.h"
#include "Memory.h"

#include "FileFormats.h"

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
#define LogAssert SOKO_PLATFORM_FUNCTION(LogAssert)
#define SetInputMode SOKO_PLATFORM_FUNCTION(SetInputMode)

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
		gameState->renderer = AllocAndInitRenderer(arena);
		gameState->renderGroup = AllocateRenderGroup(arena, KILOBYTES(4), 512);

		CameraConfig camera = {};
		camera.position = V3(0.0f);
		camera.front = V3(0.0f, 0.0f, -1.0f);
		camera.fovDeg = 45.0f;
		camera.aspectRatio = 16.0f / 9.0f;
		camera.nearPlane = 0.1f;
		camera.farPlane = 10.0f;

		gameState->camera.conf = camera;
		gameState->camera.moveSpeed = 3.0f;
		gameState->camera.rotateSpeed = 60.0f;
		gameState->camera.moveSmooth = 0.8f;
		gameState->camera.rotateSmooth = 0.45f;

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
		stbi_set_flip_vertically_on_load(1);

		i32 width;
		i32 height;
		i32 bpp;
		BeginTemporaryMemory(gameState->tempArena);
		unsigned char* diffBitmap = stbi_load("../res/ABB_Manipulator_Diffuse.png", &width, &height, &bpp, 4);

		Material material = {};
		material.diffMap.format = GL_RGBA8;
		material.diffMap.width = width;
		material.diffMap.height = height;
		material.diffMap.data = diffBitmap;
		RendererLoadTexture(&material.diffMap);
	   	SOKO_ASSERT(material.diffMap.gpuHandle, "");

		EndTemporaryMemory(gameState->tempArena);
		BeginTemporaryMemory(gameState->tempArena);

		unsigned char* specBitmap = stbi_load("../res/ABB_Manipulator_Specular.png", &width, &height, &bpp, 4);
		material.specMap.format = GL_RGBA8;
		material.specMap.width = width;
		material.specMap.height = height;
		material.specMap.data = specBitmap;

		RendererLoadTexture(&material.specMap);
	   	SOKO_ASSERT(material.specMap.gpuHandle, "");
		EndTemporaryMemory(gameState->tempArena);

		gameState->material = material;

	}
	
	void
	GameReload(AB::MemoryArena* arena, AB::PlatformState* platform)
	{
		_GlobalPlatform = platform;
		_GlobalStaticStorage = (StaticStorage*)arena->begin;
	}

	void
	GameUpdate(AB::MemoryArena* arena, AB::PlatformState* platform)
	{
		SOKO_INFO("Sokoban!");
	}

	void
	UpdateCamera(Camera* camera)
	{
		v3 pos = camera->targetPosition;
		v3 front = camera->targetFront;
		
		if (GlobalInput.keys[AB::KEY_W].pressedNow)
		{
			pos += front * GlobalAbsDeltaTime * camera->moveSpeed;
		}
		
		if (GlobalInput.keys[AB::KEY_S].pressedNow)
		{
			pos -= front * GlobalAbsDeltaTime * camera->moveSpeed;
		}
		
		if (GlobalInput.keys[AB::KEY_A].pressedNow)
		{
			v3 right = Normalize(Cross(front, V3(0.0f, 1.0f, 0.0f)));
			pos -= right * GlobalAbsDeltaTime * camera->moveSpeed;
		}
		
		if (GlobalInput.keys[AB::KEY_D].pressedNow)
		{
			v3 right = Normalize(Cross(front, V3( 0, 1, 0 )));
			pos += right * GlobalAbsDeltaTime * camera->moveSpeed;
		}

		if (GlobalInput.keys[AB::KEY_TAB].pressedNow &&
			!GlobalInput.keys[AB::KEY_TAB].wasPressed)
		{
			camera->cursorCaptured = !camera->cursorCaptured;
			if (camera->cursorCaptured)
			{
				SetInputMode(AB::INPUT_MODE_CAPTURE_CURSOR);
			}
			else
			{
				SetInputMode(AB::INPUT_MODE_FREE_CURSOR);				
			}
		}

		if (camera->cursorCaptured)
		{
			camera->pitch += GlobalInput.mouseFrameOffsetY * camera->rotateSpeed;
			camera->yaw += GlobalInput.mouseFrameOffsetX * camera->rotateSpeed;	
		}

			
		if (camera->pitch > 89.0f)
		{
			camera->pitch = 89.0f;			
		}
		if (camera->pitch < -89.0f)
		{
			camera->pitch = -89.0f;			
		}

		front.x = Cos(ToRadians(camera->pitch)) * Cos(ToRadians(camera->yaw));
		front.y = Sin(ToRadians(camera->pitch));
		front.z = Cos(ToRadians(camera->pitch))	* Sin(ToRadians(camera->yaw));
		front = Normalize(front);

		camera->targetPosition = pos;
		camera->targetFront = front;
		
		camera->conf.position = Lerp(camera->conf.position, pos, camera->moveSmooth);
		camera->conf.front = Lerp(camera->conf.front, front, camera->rotateSmooth);
	}

	void
	GameRender(AB::MemoryArena* arena, AB::PlatformState* platform)
	{
		auto* gameState = _GlobalStaticStorage->gameState;

		UpdateCamera(&gameState->camera);
		RenderGroupSetCamera(gameState->renderGroup, &gameState->camera.conf);

		RendererBeginFrame(gameState->renderer, V2(1280.0f, 800.0f));
		DirectionalLight light = {};
		light.dir = Normalize(V3(0.2f, -0.9f, -0.3f));
		light.ambient = V3(0.3f);
		light.diffuse = V3(0.8f);
		light.specular = V3(1.0f);
		RenderCommandSetDirLight lightCommand = {};
		lightCommand.light = light;
		RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_SET_DIR_LIGHT,
							   (void*)&lightCommand);
		DrawStraightLine(gameState->renderGroup, V3(0.0f, 0.0f, -1.0f), V3(16.0f, 9.0f, -1.0f), V3(1.0f, 0.0f, 0.0f), 4.0f);
		RenderCommandDrawMesh command = {};
		command.transform = Scaling(V3(0.8f));
		command.mesh = &gameState->mesh;
		command.material = &gameState->material;
		RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
							   (void*)&command);
		FlushRenderGroup(gameState->renderer, gameState->renderGroup);
	}
}

#include "RenderGroup.cpp"
#include "Renderer.cpp"

