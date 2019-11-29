#include "EntityBehavior.h"
#include "Player.h"
namespace soko
{
    internal void
    UpdateEntity(Level* level, Entity* e)
    {
        switch (e->type)
        {
        case EntityType_Player: { UpdatePlayer(level, e); }
        default: {} break;
        }
    }

    internal bool
    EntityCollides(const Level* level, const Entity* e)
    {
        bool result = true;
        switch (e->type)
        {
        case EntityType_Portal:
        {
            SOKO_ASSERT(e->type == EntityType_Portal);
            auto data = &e->behavior.data.portal;
            if (data->destPortalID)
            {
                const Entity* destPortal = GetEntity(level, data->destPortalID);
                if (destPortal)
                {
                    SOKO_ASSERT(destPortal->type == EntityType_Portal);
                    if (CheckTile(level, destPortal->behavior.data.portal.teleportP))
                    {
                        result = false;
                    }
                }
            }
        } break;
        default: { result = IsSet(e, EntityFlag_Collides); } break;
        }
        return result;
    }

    // NOTE: Returns true if caller entity was moved
    internal bool
    EntityHandleOverlap(Level* level, Entity* e, Entity* caller)
    {
        bool result = false;
        switch(e->type)
        {
        case EntityType_Spikes:
        {
            SOKO_ASSERT(e->type == EntityType_Spikes);
            if (caller && caller->type != EntityType_Player)
            {
                // TODO: Delete from sim region
                DeleteEntity(level, caller);
                result = true;
            }
        } break;
        case EntityType_Portal:
        {
            auto data = &e->behavior.data.portal;
            if (caller && IsSet(caller, EntityFlag_Movable))
            {
                if (data->destPortalID)
                {
                    Entity* destPortal = GetEntity(level, data->destPortalID);
                    if (destPortal && destPortal->type == EntityType_Portal)
                    {
                        iv3 destPos = destPortal->behavior.data.portal.teleportP;
                        if (CheckTile(level, destPos))
                        {
                            if (ChangeEntityLocation(level, caller, destPos))
                            {
                                result = true;
                            }
                        }
                    }
                }
            }
        }
        case EntityType_Button:
        {
            if (caller && IsSet(caller, EntityFlag_Collides))
            {
                auto data = &e->behavior.data.button;
                if (data->boundEntityID)
                {
                    Entity* boundE = GetEntity(level, data->boundEntityID);
                    if (boundE)
                    {
                        // TODO: Call actual behavior here! Not overlap behavior
                        EntityHandleOverlap(level, boundE, e);
                    }
                }
            }
        } break;
        case EntityType_Spawner:
        {
            // TODO: Check is p valid?
            auto data = &e->behavior.data.spawner;
            if (CheckTile(level, data->spawnP))
            {
                u32 id = AddEntity(level, data->entityType, data->spawnP, 2.0f, EntityMesh_Box, EntityMaterial_Box);
            }
        }
        default: {} break;
        }
        return result;
    }

    // TODO: Clean the sim and stored entity concept up
    // TODO: Update ticks
    internal bool
    ProcessEntityTileOverlap(Level* level, iv3 tile, Entity* overlappingEntity)
    {
        bool result = false;
        EntityMapIterator it = {};
        while (true)
        {
            Entity* entity = YieldEntityFromTile(level, tile, &it);
            if (!entity) break;
            if (!overlappingEntity || entity->id != overlappingEntity->id)
            {
                result = result || EntityHandleOverlap(level, entity, overlappingEntity);
            }
        }
        return result;
    }
}
