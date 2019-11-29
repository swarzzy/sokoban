#include "Player.h"
#include "SimRegion.h"

namespace soko
{
    internal void
    UpdatePlayer(Level* level, Entity* e)
    {
        SOKO_ASSERT(e->type == EntityType_Player);
        auto* data = &e->behavior.data.player;
        u32 steps = 1;

        if (JustPressed(AB::KEY_SPACE))
        {
            data->reversed = !data->reversed;
        }

        if (JustPressed(AB::KEY_SPACE))
        {
            BeginEntityTransition(level, e, Direction_Up, steps, e->movementSpeed, data->reversed ? -2 : 2);
        }

        if (JustPressed(AB::KEY_SHIFT))
        {
            BeginEntityTransition(level, e, Direction_Down, steps, e->movementSpeed, data->reversed ? -2 : 2);
        }

        if (JustPressed(AB::KEY_UP))
        {
            BeginEntityTransition(level, e, Direction_North, steps, e->movementSpeed, data->reversed ? -2 : 2);
        }

        if (JustPressed(AB::KEY_DOWN))
        {
            BeginEntityTransition(level, e, Direction_South, steps, e->movementSpeed, data->reversed ? -2 : 2);
        }

        if (JustPressed(AB::KEY_RIGHT))
        {
            BeginEntityTransition(level, e, Direction_East, steps, e->movementSpeed, data->reversed ? -2 : 2);
        }

        if (JustPressed(AB::KEY_LEFT))
        {
            BeginEntityTransition(level, e, Direction_West, steps, e->movementSpeed, data->reversed ? -2 : 2);
        }
    }

    internal Entity*
    AddPlayer(GameSession* session, iv3 coord)
    {
        Level* level = session->level;
        Entity* p = 0;
#if 0
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
#endif
            u32 playerId = AddEntity(level, EntityType_Player, coord, 8.0f,
                                     EntityMesh_Cat, EntityMaterial_Cat);
            if (playerId)
            {
                //p = session->players + freePlayerIndex;
                //p->level = level;
                p = GetEntity(level, playerId);
                p->behavior.data.player.reversed = false;
                SOKO_ASSERT(p);
                //session->playersOccupancy[freePlayerIndex] = true;
            }

//        }
        return p;
    }

    internal void
    DeletePlayer(GameSession* session, Entity* player)
    {
        DeleteEntity(session->level, player);
#if 0
        for (i32 i = 0; i < SESSION_MAX_PLAYERS; i++)
        {
            if ((session->players + i) == player)
            {
                session->playersOccupancy[i] = false;
                session->players[i] = {};
                break;
            }
        }
#endif
    }
}
