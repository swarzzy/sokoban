#pragma once
#include "Level.h"
#include "Player.h"

namespace soko
{
    struct Level;
    struct Entity;

    enum GameMode
    {
        GAME_MODE_MENU = 0,
        GAME_MODE_SINGLE,
        GAME_MODE_EDITOR
    };

    struct EditorCamera;
    struct Editor;
    struct GameSession
    {
        GameMode gameMode;
        AB::MemoryArena* sessionArena;
        Level* level;
        Editor* editor;

        Entity* player;

        // TODO: Store pointers
        FPCamera debugCamera;
        GameCamera camera;
        EditorCamera* editorCamera;
        b32 useDebugCamera;
    };

    internal void DestroyGameSession(GameSession* session);
}
