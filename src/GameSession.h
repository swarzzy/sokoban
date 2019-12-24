#pragma once
#include "Level.h"
#include "Network.h"
#include "Player.h"

namespace soko
{
    struct Level;
    struct Entity;

    enum GameMode
    {
        GAME_MODE_MENU = 0,
        GAME_MODE_SINGLE,
        GAME_MODE_CLIENT,
        GAME_MODE_SERVER,
        GAME_MODE_EDITOR
    };

    struct EditorCamera;
    struct Editor;
    struct GameSession
    {
        GameMode gameMode;
        Client* client;
        Server* server;
        SocketBuffer* socketBuffer;
        AB::MemoryArena* sessionArena;
        Level* level;
        Editor* editor;

        Entity* firstPlayer;
        Entity* secondPlayer;

        // NOTE: Just always store them here for now
        PlayerActionBuffer<256> playerActionBuffer;

        // TODO: Store pointers
        FPCamera debugCamera;
        GameCamera camera;
        EditorCamera* editorCamera;
        b32 useDebugCamera;
    };

    internal void DestroyGameSession(GameSession* session);
}
