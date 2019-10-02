#include "Player.h"

namespace soko
{
    Player*
    AddPlayer(GameState* gameState, Level* level, v3i coord, AB::MemoryArena* arena)
    {
        Player* p = nullptr;

        i32 freePlayerIndex = -1;
        for (i32 i = 0; i < GameState::MAX_PLAYERS; i++)
        {
            if (!gameState->playersOccupancy[i])
            {
                freePlayerIndex = i;
                break;
            }
        }

        if (freePlayerIndex != -1)
        {

            u32 playerId =  AddEntity(level, ENTITY_TYPE_PLAYER, coord,
                                      EntityMesh_Cube, EntityMaterial_Player, arena);

            if (playerId)
            {
                p = gameState->players + freePlayerIndex;
                p->level = level;
                p->e = GetEntity(p->level, playerId);
                SOKO_ASSERT(p->e);
                gameState->playersOccupancy[freePlayerIndex] = true;
            }

        }
        return p;
    }

    void DeletePlayer(GameState* gameState, Player* player)
    {
        DeleteEntity(player->level, player->e);
        for (i32 i = 0; i < GameState::MAX_PLAYERS; i++)
        {
            if ((gameState->players + i) == player)
            {
                gameState->playersOccupancy[i] = false;
                gameState->players[i] = {};
                break;
            }
        }
    }
}
