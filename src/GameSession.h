#pragma once
#include "Level.h"
#include "Network.h"

namespace soko
{
    struct Level;
    struct Entity;


    constant u32 PLAYER_INPUT_BUFFER_SIZE = 512;

    enum PlayerAction : byte
    {
        // NOTE: Movement action values should be same
        // as in Direction enum
        PlayerAction_MoveNorth = 1,
        PlayerAction_MoveSouth,
        PlayerAction_MoveWest,
        PlayerAction_MoveEast,
        PlayerAction_MoveUp,
        PlayerAction_MoveDown,
        PlayerAction_ToggleInteractionMode,
    };

    static_assert((byte)PlayerAction_MoveNorth == (byte)Direction_North);
    static_assert((byte)PlayerAction_MoveSouth == (byte)Direction_South);
    static_assert((byte)PlayerAction_MoveWest == (byte)Direction_West);
    static_assert((byte)PlayerAction_MoveEast == (byte)Direction_East);
    static_assert((byte)PlayerAction_MoveUp == (byte)Direction_Up);
    static_assert((byte)PlayerAction_MoveDown == (byte)Direction_Down);


    inline bool ActionIsMovement(PlayerAction action)
    {
        bool result = (byte)action >= PlayerAction_MoveNorth && (byte)action <= PlayerAction_MoveDown;
        return result;
    }

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
        AB::MemoryArena* sessionArena;
        Level* level;
        Editor* editor;

        Entity* firstPlayer;
        Entity* secondPlayer;

        // TODO: Store pointers
        FPCamera debugCamera;
        GameCamera camera;
        EditorCamera* editorCamera;
        b32 useDebugCamera;
    };

    internal void DestroyGameSession(GameSession* session);
}
