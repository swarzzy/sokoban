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
        //MainMenu_CreateServer,
        MainMenu_ConnectToServer,
        MainMenu_ClientWaitForConnection,
        MainMenu_ClientSessionLobby,
        MainMenu_ConnectionError,
        //MainMenu_ClientWaitForServerState,
        //MainMenu_ClientLoadLevel,
        //MainMenu_ClientConnectToServer,
        MainMenu_GenTestLevel,
        MainMenu_EditorConf,
        MainMenu_EditorLoadLevel,
        MainMenu_EditorCreateLevel,
        MainMenu_EnterEditor,
        MainMenu_LevelCompleted,
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

    struct MenuLevelCache
    {
        bool initialized;
        u32 selectedIndex;
        DirectoryContents dirScanResult;
        // TODO: Switch to array of structs
        b32* isLevel;
        u64* GUIDs;
        b32* availableOnClient;
        b32* supportsMultiplayer;
    };

    enum ServerState
    {
        ServerState_NotInitialized = 0,
        ServerState_Listening,
        ServerState_Connected
    };

    struct GameMenu
    {
        MainMenuState state;
        MenuLevelCache levelCache;
        char playerNameCache[PLAYER_NAME_LEN];
        char secondPlayerName[PLAYER_NAME_LEN];
        ServerState serverState;
        u16 serverPort;
        Socket serverListenSocket;
        Socket serverConnectionSocket;
        // NOTE: Set to true when connect message recieved
        b32 serverPlayerConnected;
        Socket clientSocket;
        f32 clientConnectionTimeout;
        b32 clientConnectionEstablished;
        // NOTE: Will be zeroed from this member
        char levelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        wchar_t wLevelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        LevelMetaInfo levelMetaInfo;
        ServerConfig serverConf;
        ClientConfig clientConf;
        GameSession session;
    };
}
