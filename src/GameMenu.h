#pragma once

namespace soko
{
    enum MainMenuState
    {
        MainMenu_ModeSelection,
        MainMenu_SingleSelectLevel,
        MainMenu_SingleLoadLevel,
        MainMenu_EnterLevel,
        MainMenu_ConfigureServer,
        MainMenu_CreateServer,
        MainMenu_ConfigureClient,
        MainMenu_TryConnectToServer,
        MainMenu_ClientLoadLevel,
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
    };

    static constexpr u32 LEVEL_PATH_BUFFER_SIZE = 256;

    struct GameMenu
    {
        u32 gameMode; // GameMode
        MainMenuState state;
        char levelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        wchar_t wLevelPathBuffer[LEVEL_PATH_BUFFER_SIZE];
        ServerConfig serverConf;
        ClientConfig clientConf;
    };
}
