#pragma once


namespace soko
{
    namespace net {struct Client; struct Server;}
    struct Level;

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
    };
}
