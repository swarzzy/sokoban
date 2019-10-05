#include "GameSession.h"
namespace soko
{
    // TODO: Use this in menu cleanup

    internal void
    DestroyGameSession(GameSession* session)
    {
        if (session->gameMode = GAME_MODE_SERVER)
        {
            if (session->server)
            {
                bool result = NetCloseSocket(session->server->socket);
                SOKO_ASSERT(result);
            }
        }
        else if (session->gameMode = GAME_MODE_CLIENT)
        {
            if (session->client)
            {
                bool result = NetCloseSocket(session->client->socket);
                SOKO_ASSERT(result);
            }
        }
        else
        {
            INVALID_CODE_PATH;
        }


        if (session->sessionArena)
        {
            PLATFORM_FREE_ARENA(session->sessionArena);
        }
        ZERO_STRUCT(GameSession, session);
    }
}
