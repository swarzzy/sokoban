#pragma once
#include "Level.h"

namespace soko
{
    namespace net {struct Client; struct Server;}
    struct Level;
    struct Entity;

    constant u32 SESSION_MAX_PLAYERS = 4;

    constant u32 PLAYER_INPUT_BUFFER_SIZE = 512;

    struct ClientInput
    {
        u32 bufferAt;
        byte buffer[PLAYER_INPUT_BUFFER_SIZE];
    };

    struct Player
    {
        Entity* e;
        Level* level;
        u32 inputFlags;
        bool reversed;
    };

    struct PlayerSlot
    {
        b32 used;
        Player* player;
        ClientInput input;
    };

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

    static_assert((byte)PlayerAction_MoveNorth == (byte)DIRECTION_NORTH);
    static_assert((byte)PlayerAction_MoveSouth == (byte)DIRECTION_SOUTH);
    static_assert((byte)PlayerAction_MoveWest == (byte)DIRECTION_WEST);
    static_assert((byte)PlayerAction_MoveEast == (byte)DIRECTION_EAST);
    static_assert((byte)PlayerAction_MoveUp == (byte)DIRECTION_UP);
    static_assert((byte)PlayerAction_MoveDown == (byte)DIRECTION_DOWN);


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
        GAME_MODE_SERVER
    };

    struct GameSession
    {
        GameMode gameMode;
        union
        {
            net::Client* client;
            net::Server* server;
        };
        AB::MemoryArena* sessionArena;
        Level* level;

        Player* controlledPlayer;
        bool playersOccupancy[SESSION_MAX_PLAYERS];
        Player players[SESSION_MAX_PLAYERS];
    };
}
