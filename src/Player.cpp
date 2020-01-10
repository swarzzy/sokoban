#include "Player.h"
#include "SimRegion.h"

namespace soko
{
    void
    UpdatePlayer(Level* level, Entity* e)
    {
        SOKO_ASSERT(e->type == EntityType_Player);
        auto* data = &e->behavior.data.player;

        // TODO: Formalize this
        u32 steps = 1;
        u32 pushDepth = data->reversed ? 1 : 1;

        if (JustPressed(AB::KEY_SPACE)) data->reversed = !data->reversed;
        if (JustPressed(AB::KEY_UP)) BeginEntityTransition(level, e, Direction_North, steps, e->movementSpeed, pushDepth);
        if (JustPressed(AB::KEY_DOWN)) BeginEntityTransition(level, e, Direction_South, steps, e->movementSpeed, pushDepth);
        if (JustPressed(AB::KEY_RIGHT)) BeginEntityTransition(level, e, Direction_East, steps, e->movementSpeed, pushDepth);
        if (JustPressed(AB::KEY_LEFT)) BeginEntityTransition(level, e, Direction_West, steps, e->movementSpeed, pushDepth);
    }

    bool
    AddPlayer(GameSession* session, iv3 coord)
    {
        bool result = false;

        Level* level = session->level;

        if (!session->player)
        {
            u32 playerId = AddEntity(level, EntityType_Player, coord, 8.0f,
                                     EntityMesh_Cat, EntityMaterial_Cat);
            if (playerId)
            {
                Entity* p = GetEntity(level, playerId);
                SOKO_ASSERT(p);
                p->behavior.data.player.reversed = false;
                session->player = p;
                result = true;
            }
        }
        return result;
    }

    void
    DeletePlayer(GameSession* session, Entity* player)
    {
        DeleteEntity(session->level, player);
    }
}
