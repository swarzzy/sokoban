#include "GameSession.h"
namespace soko
{
    // TODO: Use this in menu cleanup

    internal void
    DestroyGameSession(GameSession* session)
    {
        if (session->sessionArena)
        {
            PLATFORM_FREE_ARENA(session->sessionArena);
        }
        ZERO_STRUCT(GameSession, session);
    }
}
