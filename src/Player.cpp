#include "Player.h"

namespace soko
{
    internal Player*
    AddPlayer(GameSession* session, iv3 coord)
    {
        Level* level = session->level;
        Player* p = nullptr;

        i32 freePlayerIndex = -1;
        for (i32 i = 0; i < SESSION_MAX_PLAYERS; i++)
        {
            if (!session->playersOccupancy[i])
            {
                freePlayerIndex = i;
                break;
            }
        }

        if (freePlayerIndex != -1)
        {

            u32 playerId = AddEntity(level, EntityType_Player, coord, 8.0f,
                                     EntityMesh_Cube, EntityMaterial_Player);
            if (playerId)
            {
                p = session->players + freePlayerIndex;
                p->level = level;
                p->e = GetEntity(p->level, playerId);
                SOKO_ASSERT(p->e);
                session->playersOccupancy[freePlayerIndex] = true;
            }

        }
        return p;
    }

    internal void
    DeletePlayer(GameSession* session, Player* player)
    {
        DeleteEntity(player->level, player->e);
        for (i32 i = 0; i < SESSION_MAX_PLAYERS; i++)
        {
            if ((session->players + i) == player)
            {
                session->playersOccupancy[i] = false;
                session->players[i] = {};
                break;
            }
        }
    }
}
