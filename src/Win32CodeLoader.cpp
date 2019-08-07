#include "Win32CodeLoader.h"
#include <Windows.h>

namespace AB
{
	static void GameUpdateAndRenderDummy(MemoryArena*, PlatformState*,
										 GameUpdateAndRenderReason)
	{
		
	}
		
	void UnloadGameCode(LibraryData* lib)
	{
		TCHAR tmpLibPath[MAX_GAME_LIB_PATH];
		const TCHAR* name = TEMP_GAME_CODE_DLL_NAME;
		_stprintf_s(tmpLibPath, MAX_GAME_LIB_PATH, TEXT("%s%s"),
				   lib->dir, name);
		
		FreeLibrary(lib->handle);

		lib->GameUpdateAndRender = GameUpdateAndRenderDummy;

		DeleteFile(tmpLibPath);
	}
	
	b32 UpdateGameCode(LibraryData* lib)
	{
		b32 updated = false;
		WIN32_FIND_DATA findData;
		HANDLE findHandle = FindFirstFile(lib->fullPath, &findData);
		if (findHandle != INVALID_HANDLE_VALUE)
		{
			FindClose(findHandle);
			FILETIME fileTime = findData.ftLastWriteTime;
			u64 writeTime = ((u64)0 | fileTime.dwLowDateTime) | ((u64)0 | fileTime.dwHighDateTime) << 32;
			if (writeTime != lib->lastChangeTime)
			{
				UnloadGameCode(lib);

				TCHAR buff[MAX_GAME_LIB_PATH];
				_stprintf_s(buff, MAX_GAME_LIB_PATH, TEXT("%s%s"),
							lib->dir, TEMP_GAME_CODE_DLL_NAME);
				
				auto result = CopyFile(lib->fullPath, buff, FALSE);
				if (result)
				{
					lib->handle = LoadLibrary(buff);
					if (lib->handle)
					{
						GameUpdateAndRenderFn* gameUpdateAndRender =
							(GameUpdateAndRenderFn*)GetProcAddress(lib->handle, "GameUpdateAndRender");
						if (gameUpdateAndRender)
						{
							lib->GameUpdateAndRender = gameUpdateAndRender;
							updated = true;
							lib->lastChangeTime = writeTime;
						}
						else
						{
							AB_CORE_ERROR("Failed to get GameUpdateAndRender() address.");
						}
					}
					else
					{
						AB_CORE_ERROR("Failed to load library.");
					}
				}
				else
				{
					//AB_CORE_INFO("Waiting for game code loading.");
				}
			}
		}
		else
		{
			//AB_CORE_ERROR("Game code not found");
		}
		return updated;
	}

	void SetupDirs(LibraryData* lib)
	{
		u32 executablePathStrSize = 0;
		// TODO: Error checking!
		// This function may fail!!
		TCHAR execPath[MAX_GAME_LIB_PATH];
		AB::GetExecutablePath(execPath, MAX_GAME_LIB_PATH, &executablePathStrSize);

		TCHAR* executableDirPtr = execPath;
		for (TCHAR* ch = execPath; *ch; ch++)
		{
			if (*ch == TEXT('\\'))
			{
				executableDirPtr = ch + 1;
			}
		}
		memcpy(lib->dir, execPath, (u64)(executableDirPtr - execPath) * sizeof(TCHAR));
		lib->dir[(u64)(executableDirPtr - execPath)] = TEXT('\0');

		const TCHAR* name = GAME_CODE_DLL_NAME;
		_stprintf_s(lib->fullPath, MAX_GAME_LIB_PATH, TEXT("%s%s"), lib->dir, name);
	}
}
