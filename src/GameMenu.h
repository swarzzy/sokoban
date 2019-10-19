#pragma once
#include "NetMessages.h"

namespace soko
{
    constant u32 ENTITY_MEMORY_SIZE_FOR_LEVEL = MEGABYTES(4);

    enum MainMenuState
    {
        MainMenu_ModeSelection,
        MainMenu_SingleSelectLevel,
        MainMenu_SingleLoadLevel,
        MainMenu_EnterLevel,
        MainMenu_ConfigureServer,
        MainMenu_CreateServer,
        MainMenu_ConfigureClient,
        MainMenu_ClientWaitForServerState,
        MainMenu_ClientLoadLevel,
        MainMenu_ClientConnectToServer,
        MainMenu_GenTestLevel,
        MainMenu_EditorConf,
        MainMenu_EditorLoadLevel,
        MainMenu_EditorCreateLevel,
        MainMenu_EnterEditor,
        MainMenu_Error
    };

    struct ServerConfig
    {
        i16 port;
    };

    struct ClientConfig
    {
        AB::NetAddress serverAddress;
        int ipOctets[4];
        int inputPort;
        AB::Socket socket;
    };

    static constexpr u32 LEVEL_PATH_BUFFER_SIZE = SERVER_MAX_LEVEL_NAME_LEN;

    namespace net { struct Client; }

    struct GameMenu
    {
        MainMenuState state;
        // NOTE: Will be zeroed from this member
        char levelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        wchar_t wLevelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        LevelMetaInfo levelMetaInfo;
        ServerConfig serverConf;
        ClientConfig clientConf;
        GameSession session;
    };
}
