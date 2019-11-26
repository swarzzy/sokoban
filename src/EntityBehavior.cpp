#include "EntityBehavior.h"
#include "Player.h"
namespace soko
{
    internal void
    UpdateEntity(Level* level, Entity* e)
    {
        switch (e->behavior.type)
        {
        case EntityBehavior_Player: { UpdatePlayer(level, e); }
        default: {} break;
        }
    }

    internal bool
    EntityCollides(Level* level, Entity* e)
    {
        bool result = true;
        switch (e->behavior.type)
        {
        case EntityBehavior_Portal:
        {
            SOKO_ASSERT(e->behavior.type == EntityBehavior_Portal);
            auto data = &e->behavior.data.portal;
            if (data->destPortalID)
            {
                Entity* destPortal = GetEntity(level, data->destPortalID);
                if (destPortal)
                {
                    SOKO_ASSERT(destPortal->behavior.type == EntityBehavior_Portal);
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
        switch(e->behavior.type)
        {
        case EntityBehavior_Spikes:
        {
            SOKO_ASSERT(e->behavior.type == EntityBehavior_Spikes);
            if (caller && caller->type != EntityType_Player)
            {
                // TODO: Delete from sim region
                DeleteEntity(level, caller);
                result = true;
            }
        } break;
        case EntityBehavior_Portal:
        {
            auto data = &e->behavior.data.portal;
            if (caller && IsSet(caller, EntityFlag_Movable))
            {
                if (data->destPortalID)
                {
                    Entity* destPortal = GetEntity(level, data->destPortalID);
                    if (destPortal && destPortal->behavior.type == EntityBehavior_Portal)
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
        case EntityBehavior_Button:
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
        case EntityBehavior_Spawner:
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
