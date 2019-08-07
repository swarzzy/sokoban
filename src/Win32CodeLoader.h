#pragma once
#include "Platform.h"
#include <windows.h>

namespace AB
{
	const TCHAR* GAME_CODE_DLL_NAME = TEXT("SokoGame.dll");
	const TCHAR* TEMP_GAME_CODE_DLL_NAME = TEXT("TEMP_SokoGame.dll");
	const TCHAR* RENDERER_DLL_NAME = TEXT("SokoRenderer.dll");
	const TCHAR* TEMP_RENDERER_DLL_NAME = TEXT("TEMP_SokoRenderer.dll");
		
	// TODO: @Important: Get rid of max path constant
	const u32 MAX_GAME_LIB_PATH = 256;

	struct MemoryArena;
	struct PlatformState;
	struct Application;

	typedef void(GameUpdateAndRenderFn)(MemoryArena*, PlatformState*, GameUpdateAndRenderReason);

	struct LibraryData
	{
		GameUpdateAndRenderFn* GameUpdateAndRender;
		u64 lastChangeTime;
		TCHAR fullPath[MAX_GAME_LIB_PATH];
		TCHAR dir[MAX_GAME_LIB_PATH];
		HMODULE handle;
	};
	
	b32 UpdateGameCode(LibraryData* lib);
	void UnloadGameCode(LibraryData* lib);
	void SetupDirs(LibraryData* lib);
}
