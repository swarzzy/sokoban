#pragma once

namespace soko
{
    constant u32 ENTITY_MEMORY_SIZE_FOR_LEVEL = MEGABYTES(4);

    enum MainMenuState
    {
        MainMenu_ModeSelection,
        MainMenu_SingleSelectLevel,
        MainMenu_SingleLoadLevel,
        MainMenu_EnterLevel,
        MainMenu_GenTestLevel,
        MainMenu_EditorConf,
        MainMenu_EditorLoadLevel,
        MainMenu_EditorCreateLevel,
        MainMenu_EnterEditor,
        MainMenu_LevelCompleted,
        MainMenu_Error
    };

    // TODO: Unbounded sizes to pathes
    constant u32 LEVEL_PATH_BUFFER_SIZE = 256;

    struct MenuLevelCache
    {
        bool initialized;
        u32 selectedIndex;
        DirectoryContents dirScanResult;
        // TODO: Switch to array of structs
        b32* isLevel;
        u64* GUIDs;
    };

    struct GameMenu
    {
        MainMenuState state;
        MenuLevelCache levelCache;
        char playerNameCache[PLAYER_NAME_LEN];
        // NOTE: Will be zeroed from this member
        char levelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        wchar_t wLevelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        LevelMetaInfo levelMetaInfo;
        GameSession session;
    };
}
