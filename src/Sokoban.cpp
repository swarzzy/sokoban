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
#define LogAssertV SOKO_PLATFORM_FUNCTION(LogAssertV)
#define SetInputMode SOKO_PLATFORM_FUNCTION(SetInputMode)

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
	void LogAssert(AB::LogLevel level, const char* file, const char* func, u32 line,
				   const char* assertStr, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		LogAssertV(level, file, func, line, assertStr, fmt, &args);
		va_end(args);
	}

	void LogAssert(AB::LogLevel level, const char* file, const char* func, u32 line,
				   const char* assertStr)
	{
		LogAssertV(level, file, func, line, assertStr, nullptr, nullptr);
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

#include "Level.cpp"

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
		
		gameState->level.xDim = 64;
		gameState->level.yDim = 64;
		gameState->level.zDim = 3;
		gameState->level.entityCount = 1;

		u32 tileArraySize = gameState->level.xDim * gameState->level.yDim * gameState->level.zDim;

		auto* level = &gameState->level;
		level->tiles = PUSH_ARRAY(gameState->memoryArena, Tile, tileArraySize);
		SOKO_ASSERT(level->tiles);

		for (u32 x = 0; x < level->xDim; x++)
		{
			for (u32 y = 0; y < level->yDim; y++)
			{
				Tile* tile = GetTile(level, x, y, 0);
				tile->value = TILE_VALUE_WALL;

				if ((x == 0) || (x == level->xDim - 1) ||
					(y == 0) || (y == level->yDim - 1))
				{
					Tile* tile1 = GetTile(level, x, y, 1);
					tile1->value = TILE_VALUE_WALL;
				}
			}
		}

		Entity playerEntity = {};
		playerEntity.type = ENTITY_TYPE_PLAYER;
		playerEntity.coord = V3U(10, 10, 1);
		u32 playerEntityId = AddEntity(&gameState->level, playerEntity);

		auto* player = &gameState->player;
		player->level = &gameState->level;
		player->e = player->level->entities + playerEntityId;
		Tile* playerTile = GetTile(player->level, player->e->coord);
		playerTile->value = TILE_VALUE_ENTITY;
		
		Entity entity1 = {};
		entity1.type = ENTITY_TYPE_BLOCK;
		entity1.coord = V3U(5, 7, 1);
		AddEntity(player->level, entity1);

		Entity entity2 = {};
		entity2.type = ENTITY_TYPE_BLOCK;
		entity2.coord = V3U(5, 8, 1);
		AddEntity(player->level, entity2);

		Entity entity3 = {};
		entity3.type = ENTITY_TYPE_BLOCK;
		entity3.coord = V3U(5, 9, 1);
		AddEntity(player->level, entity3);


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

	void
	UpdateCamera(FPCamera* camera)
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

	void UpdateCamera(GameCamera* camera)
	{
		v3 camFrameOffset = {};
		
		v3 _frontDir = V3(camera->conf.front.x, 0.0f, camera->conf.front.z);
		v3 _rightDir = Cross(V3(0.0f, 1.0f, 0.0f), _frontDir);
		// TODO: Is that normalization is neccessary?
		_rightDir = Normalize(_rightDir);

		v2 frontDir = V2(_frontDir.x, _frontDir.z);
		v2 rightDir = V2(_rightDir.x, _rightDir.z);

		v2 acceleration = {};
		if (GlobalInput.keys[AB::KEY_W].pressedNow)
		{
			acceleration += frontDir;
		}
		if (GlobalInput.keys[AB::KEY_S].pressedNow)
		{
			acceleration -= frontDir;
		}
		if (GlobalInput.keys[AB::KEY_A].pressedNow)
		{
			acceleration += rightDir;
		}
		if (GlobalInput.keys[AB::KEY_D].pressedNow)
		{
			acceleration -= rightDir;
		}

		acceleration = Normalize(acceleration);
		acceleration *= camera->moveSpeed; // 500.0f

		f32 friction = camera->moveFriction; // 8.0f
		acceleration = acceleration - camera->velocity * friction;

		v2 movementDelta;
		movementDelta = 0.5f * acceleration *
			Square(GlobalAbsDeltaTime) + 
			camera->velocity *
			GlobalAbsDeltaTime;

		camera->targetPos += V3(movementDelta.x, 0.0f, movementDelta.y);
		camera->velocity += acceleration * GlobalAbsDeltaTime;

		if (GlobalInput.mouseButtons[AB::MBUTTON_RIGHT].pressedNow)
		{
			v2 mousePos;
			f32 speed = camera->rotSpeed; // 1000.0f
			mousePos.x = GlobalInput.mouseFrameOffsetX * speed;
			mousePos.y = GlobalInput.mouseFrameOffsetY * speed;
			camera->targetOrbit.x -= mousePos.x;
			camera->targetOrbit.y -= mousePos.y;
		}

		if (camera->targetOrbit.y < 95.0f)
		{
			camera->targetOrbit.y = 95.0f;
		}
		else if (camera->targetOrbit.y > 170.0f)
		{
			camera->targetOrbit.y = 170.0f;
		}
		
		i32 frameScrollOffset = GlobalInput.scrollFrameOffset;
		camera->targetDistance -= frameScrollOffset * camera->zoomSpeed; // 5.0f

		if (camera->targetDistance < 5.0f)
		{
			camera->targetDistance = 5.0f;
		}
		else if (camera->targetDistance > 50.0f)
		{
			camera->targetDistance = 50.0f;
		}
		
		camera->latitude = Lerp(camera->latitude, camera->targetOrbit.y,
								camera->latSmooth);

		camera->longitude = Lerp(camera->longitude, camera->targetOrbit.x,
								 camera->longSmooth);

		camera->distance = Lerp(camera->distance, camera->targetDistance,
								camera->distSmooth);

		f32 latitude = ToRadians(camera->latitude);
		f32 longitude = ToRadians(camera->longitude);
		f32 polarAngle = PI_32 - latitude;
			
		f32 x = camera->distance * Sin(polarAngle) * Sin(longitude);
		f32 z = camera->distance * Sin(polarAngle) * Cos(longitude);
		f32 y = camera->distance * Cos(polarAngle);

		camera->conf.position = camera->targetPos + V3(x, y, z);
		camera->conf.front = -Normalize(V3(x, y, z));

#if 0
		v2 normMousePos;
		normMousePos.x = 2.0f *	GlobalInput.mouseX - 1.0f;
		normMousePos.y = 2.0f *	GlobalInput.mouseY - 1.0f;
		v4 mouseClip = V4(normMousePos, 1.0f, 0.0f);
		v4 mouseView = MulM4V4(camera->invProjection, mouseClip);
		mouseView = V4(mouseView.xy, 1.0f, 0.0f);
		v3 mouseWorld = MulM4V4(camera->invLookAt, mouseView).xyz;
		mouseWorld = Normalize(mouseWorld);
		camera->mouseRay = mouseWorld;
		camera->mouseRayWorld = FlipYZ(mouseWorld);
#endif
	}

	void DrawOverlay(GameState* gameState)
	{
		const float DISTANCE = 10.0f;
		int corner = gameState->overlayCorner;
		ImGuiIO& io = ImGui::GetIO();
		if (corner != -1)
		{
			ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
			ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		}
		ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
		bool open = true;
		if (ImGui::Begin("Overlay", &open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
		{
			char fpsBuffer[128];
			FormatString(fpsBuffer, 128, "FPS: %11i64\nUPS: % 11i64\ndT(abs):  %.4f32\ndT(game): %.4f32",
						 PlatformGlobals.fps, PlatformGlobals.ups,
						 GlobalAbsDeltaTime, GlobalGameDeltaTime);
			ImGui::Text(fpsBuffer);
			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
				if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
				if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
				if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
				if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
				ImGui::EndPopup();
			}
		}
		ImGui::End();
		gameState->overlayCorner = corner;
	}

	void BeginDebugOverlay()
	{
		const float xPos = 10.0f;
		const float yPos = 10.0f;
		
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 window_pos = ImVec2(xPos, yPos);
		ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background
		if (ImGui::Begin("Debug overlay", NULL,
						 //ImGuiWindowFlags_NoMove |
						 //ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_NoFocusOnAppearing |
						 ImGuiWindowFlags_NoNav))
		{
			//ImGui::Text("Debug overlay");
		}
		ImGui::End();
	}

	void DebugOverlayPushStr(const char* string)
	{
		if (ImGui::Begin("Debug overlay", NULL,
						 ImGuiWindowFlags_NoMove |
						 //ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_NoFocusOnAppearing |
						 ImGuiWindowFlags_NoNav))
		{
			ImGui::Separator();
			ImGui::Text(string);
		}
		ImGui::End();		
	}

	void DebugOverlayPushVar(const char* title, v3u var)
	{
		if (ImGui::Begin("Debug overlay", NULL,
						 ImGuiWindowFlags_NoMove |
						 //ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_NoFocusOnAppearing |
						 ImGuiWindowFlags_NoNav))
		{
			ImGui::Separator();
			char buffer[128];
			FormatString(buffer, 128, "%s: x: %u32; y: %u32; z: %u32", title, var.x, var.y, var.z);
			ImGui::Text(buffer);
		}
		ImGui::End();		
	}

	void DebugOverlayPushVar(const char* title, v3 var)
	{
		if (ImGui::Begin("Debug overlay", NULL,
						 ImGuiWindowFlags_NoMove |
						 //ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_NoFocusOnAppearing |
						 ImGuiWindowFlags_NoNav))
		{
			ImGui::Separator();
			char buffer[128];
			FormatString(buffer, 128, "%s: x: %.3f32; y: %.3f32; z: %.3f32", title, var.x, var.y, var.z);
			ImGui::Text(buffer);
		}
		ImGui::End();		
	}

	void DebugOverlayPushSlider(const char* title, v3* var, f32 min, f32 max)
	{
		if (ImGui::Begin("Debug overlay", NULL,
						 ImGuiWindowFlags_NoMove |
						 //ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_NoFocusOnAppearing |
						 ImGuiWindowFlags_NoNav))
		{
			ImGui::Separator();
			ImGui::SliderFloat3(title, var->data, min, max);
		}
		ImGui::End();		

	}


#define DEBUG_OVERLAY_TRACE(var) DebugOverlayPushVar(#var, var)
#define DEBUG_OVERLAY_SLIDER(var, min, max) DebugOverlayPushSlider(#var, &var, min, max)


	void
	GameRender(AB::MemoryArena* arena, AB::PlatformState* platform)
	{
		auto* gameState = _GlobalStaticStorage->gameState;
		//bool show = true;
		//ImGui::ShowDemoWindow(&show);

		DrawOverlay(gameState);
		BeginDebugOverlay();
		DebugOverlayPushStr("Hello!");
		DEBUG_OVERLAY_TRACE(gameState->player.e->coord);
		DEBUG_OVERLAY_TRACE(gameState->camera.conf.position);
		DEBUG_OVERLAY_SLIDER(gameState->camera.conf.position, -60.0f, 60.0f);

		if (JustPressed(GlobalInput.keys[AB::KEY_F1]))
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

		Player* player = &gameState->player;

		if (JustPressed(GlobalInput.keys[AB::KEY_UP]) &&
			(player->inputFlags & PENDING_MOVEMENT_BIT_FORWARD) == 0)
		{
			player->inputFlags |= PENDING_MOVEMENT_BIT_FORWARD;
			MoveEntity(&gameState->level, player->e, MOVE_DIR_FORWARD);
		}

		if (JustPressed(GlobalInput.keys[AB::KEY_DOWN]) &&
			(player->inputFlags & PENDING_MOVEMENT_BIT_BACKWARD) == 0)
		{
			player->inputFlags |= PENDING_MOVEMENT_BIT_BACKWARD;
			MoveEntity(&gameState->level, player->e, MOVE_DIR_BACK);
		}

		if (JustPressed(GlobalInput.keys[AB::KEY_RIGHT]) &&
			(player->inputFlags & PENDING_MOVEMENT_BIT_RIGHT) == 0)
		{
			player->inputFlags |= PENDING_MOVEMENT_BIT_RIGHT;
			MoveEntity(&gameState->level, player->e, MOVE_DIR_RIGHT);
		}

		if (JustPressed(GlobalInput.keys[AB::KEY_LEFT]) &&
			(player->inputFlags & PENDING_MOVEMENT_BIT_LEFT) == 0)
		{
			player->inputFlags |= PENDING_MOVEMENT_BIT_LEFT;
			MoveEntity(&gameState->level, player->e, MOVE_DIR_LEFT);
		}

		if (JustReleased(GlobalInput.keys[AB::KEY_UP]))
		{
			player->inputFlags &= ~PENDING_MOVEMENT_BIT_FORWARD;
		}
		if (JustReleased(GlobalInput.keys[AB::KEY_DOWN]))
		{
			player->inputFlags &= ~PENDING_MOVEMENT_BIT_BACKWARD;
		}
		if (JustReleased(GlobalInput.keys[AB::KEY_RIGHT]))
		{
			player->inputFlags &= ~PENDING_MOVEMENT_BIT_RIGHT;
		}
		if (JustReleased(GlobalInput.keys[AB::KEY_LEFT]))
		{
			player->inputFlags &= ~PENDING_MOVEMENT_BIT_LEFT;
		}

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

