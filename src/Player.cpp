#include "Player.h"
#include "SimRegion.h"

namespace soko
{
    internal void
    UpdatePlayer(Level* level, Entity* e)
    {
        SOKO_ASSERT(e->type == EntityType_Player);
        auto* data = &e->behavior.data.player;
        if (data->slot == PlayerSlot_First)
        {
            u32 steps = 1;
            u32 pushDepth = data->reversed ? 1 : 1;

            if (JustPressed(AB::KEY_SPACE))
            {
                data->reversed = !data->reversed;
            }

            if (JustPressed(AB::KEY_SPACE))
            {
                BeginEntityTransition(level, e, Direction_Up, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_SHIFT))
            {
                BeginEntityTransition(level, e, Direction_Down, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_UP))
            {
                BeginEntityTransition(level, e, Direction_North, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_DOWN))
            {
                BeginEntityTransition(level, e, Direction_South, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_RIGHT))
            {
                BeginEntityTransition(level, e, Direction_East, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_LEFT))
            {
                BeginEntityTransition(level, e, Direction_West, steps, e->movementSpeed, pushDepth);
            }
        }
        else if (data->slot == PlayerSlot_Second)
        {
            u32 steps = 1;
            u32 pushDepth = data->reversed ? 1 : 1;

            if (JustPressed(AB::KEY_SHIFT))
            {
                data->reversed = !data->reversed;
            }

            if (JustPressed(AB::KEY_W))
            {
                BeginEntityTransition(level, e, Direction_North, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_S))
            {
                BeginEntityTransition(level, e, Direction_South, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_D))
            {
                BeginEntityTransition(level, e, Direction_East, steps, e->movementSpeed, pushDepth);
            }

            if (JustPressed(AB::KEY_A))
            {
                BeginEntityTransition(level, e, Direction_West, steps, e->movementSpeed, pushDepth);
            }
        }
        else
        {
            INVALID_CODE_PATH;
        }
    }

    inline bool
    AddPlayer(GameSession* session, iv3 coord, PlayerSlot slot)
    {
        bool result = false;

        Level* level = session->level;

        bool slotIsFree = false;

        if (slot == PlayerSlot_First)
        {
            slotIsFree = (session->firstPlayer == 0);
        }
        else if (slot == PlayerSlot_Second)
        {
            slotIsFree = (session->secondPlayer == 0);
        }
        else
        {
            INVALID_CODE_PATH;
        }

        u32 playerId = AddEntity(level, EntityType_Player, coord, 8.0f,
                                 EntityMesh_Cat, EntityMaterial_Cat);
        if (playerId)
        {
            Entity* p = GetEntity(level, playerId);
            SOKO_ASSERT(p);
            if (slot == PlayerSlot_First)
            {
                session->firstPlayer = p;
            }
            else
            {
                session->secondPlayer = p;
            }
            p->behavior.data.player.reversed = false;
            p->behavior.data.player.slot = slot;
            result = true;
        }
        return result;
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
