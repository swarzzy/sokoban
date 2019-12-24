#include "Player.h"
#include "SimRegion.h"

namespace soko
{
    inline bool
    PushPlayerAction(PlayerActionBuffer* buffer, PlayerAction action)
    {
        bool result = false;
        if (buffer->at < ArrayCount(buffer->actions))
        {
            buffer->actions[buffer->at] = action;
            buffer->at++;
            result = true;
        }
        return result;
    }

    internal void
    FillPlayerActionBuffer(PlayerActionBuffer* buffer)
    {
        if (buffer->slot == PlayerSlot_First)
        {
            if (JustPressed(AB::KEY_SPACE)) PushPlayerAction(buffer, PlayerAction_ToggleInteractionMode);
            if (JustPressed(AB::KEY_UP)) PushPlayerAction(buffer, PlayerAction_MoveNorth);
            if (JustPressed(AB::KEY_DOWN)) PushPlayerAction(buffer, PlayerAction_MoveSouth);
            if (JustPressed(AB::KEY_RIGHT)) PushPlayerAction(buffer, PlayerAction_MoveEast);
            if (JustPressed(AB::KEY_LEFT)) PushPlayerAction(buffer, PlayerAction_MoveWest);
        }
        else if (buffer->slot == PlayerSlot_Second)
        {
            if (JustPressed(AB::KEY_SHIFT)) PushPlayerAction(buffer, PlayerAction_ToggleInteractionMode);
            if (JustPressed(AB::KEY_W)) PushPlayerAction(buffer, PlayerAction_MoveNorth);
            if (JustPressed(AB::KEY_S)) PushPlayerAction(buffer, PlayerAction_MoveSouth);
            if (JustPressed(AB::KEY_D)) PushPlayerAction(buffer, PlayerAction_MoveEast);
            if (JustPressed(AB::KEY_A)) PushPlayerAction(buffer, PlayerAction_MoveWest);
        }
        else
        {
            INVALID_CODE_PATH;
        }
    }

    internal void
    UpdatePlayer(Level* level, Entity* e)
    {
        SOKO_ASSERT(e->type == EntityType_Player);
        auto* data = &e->behavior.data.player;

        PlayerActionBuffer* actionBuffer;
        if (data->slot == PlayerSlot_First)
        {
            actionBuffer = &level->session->firstPlayerActionBuffer;
        }
        else if (data->slot == PlayerSlot_Second)
        {
            actionBuffer = &level->session->secondPlayerActionBuffer;
        }
        else
        {
            INVALID_CODE_PATH;
        }

        // TODO: Formalize this
        u32 steps = 1;
        u32 pushDepth = data->reversed ? 1 : 1;

        for (u32 i = 0; i < actionBuffer->at; i++)
        {
            PlayerAction action = actionBuffer->actions[i];
            if (ActionIsMovement(action))
            {
                BeginEntityTransition(level, e, (Direction)action, steps, e->movementSpeed, pushDepth);
            }
            else
            {
                switch (action)
                {
                case PlayerAction_ToggleInteractionMode: { data->reversed = !data->reversed; } break;
                default: { SOKO_INFO("Trying to apply invalid player action: %u8", action); } break;
                }
            }
        }

        actionBuffer->at = 0;
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
