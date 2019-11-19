#include "EntityBehavior.h"
namespace soko
{
    typedef bool(EntityBehaviorFn)(SimRegion* region, Entity* e, Entity* caller);

    // NOTE: Return value tells is entity allowed to move on entity's tile
    internal bool EmptyBehavior(SimRegion* region, Entity* e, Entity* caller) { return true; }
    internal bool ButtonBehavior(SimRegion* region, Entity* e, Entity* caller);
    internal bool SpawnerBehavior(SimRegion* region, Entity* e, Entity* caller);
    internal bool PortalBehavior(SimRegion* region, Entity* e, Entity* caller);


    constant EntityBehaviorFn* EntityActions[TypeTraits(EntityBehaviorType)::MemberCount] =
    {
        EmptyBehavior,
        SpawnerBehavior,
        ButtonBehavior,
        PortalBehavior
    };

    internal bool
    PortalBehavior(SimRegion* region, Entity* e, Entity* caller)
    {
        bool result = false;
# if 0
        auto data = &e->stored->behavior.data.portal;
        if (caller && IsSet(caller->stored, EntityFlag_Movable))
        {
            if (data->destPortalID)
            {
                Entity* destPortal = GetEntity(region, data->destPortalID);
                if (destPortal && destPortal->stored->behavior.type == EntityBehavior_Portal)
                {
                    iv3 destPos = destPortal->stored->behavior.data.portal.teleportP;
                    if (IsTileFree(region->level, destPos))
                    {
                        if (ChangeEntityLocation(region, caller, &MakeWorldPos(destPos)))
                        {
                            result = true;
                        }
                    }
                }
            }
        }
#endif
        return result;
    }

    internal bool
    ButtonBehavior(SimRegion* region, Entity* e, Entity* caller)
    {
#if 0
        bool result = true;
        if (caller && IsSet(caller->stored, EntityFlag_Collides))
        {
            auto data = &e->stored->behavior.data.button;
            if (data->boundEntityID)
            {
                Entity* boundE = GetEntity(region, data->boundEntityID);
                if (boundE)
                {
                    EntityActions[boundE->stored->behavior.type](region, boundE, e);
                }
            }
        }
#endif
        return true;
    }

    internal bool
    SpawnerBehavior(SimRegion* region, Entity* e, Entity* caller)
    {
        bool result = true;
#if 0
        // TODO: Check is p valid?
        auto data = &e->stored->behavior.data.spawner;
        if (IsTileFree(region->level, data->spawnP))
        {
            u32 id = AddEntity(region->level, data->entityType, data->spawnP, 2.0f, EntityMesh_Box, EntityMaterial_Box);
            if (id)
            {
                // TODO: Manage entity memory in regions
                AddEntityToRegion(region, GetEntity(region->level, id));
            }
        }
#endif
        return result;
    }

    // TODO: Clean the sim and stored entity concept up
    // TODO: Update ticks
    internal bool
    ProcessEntityTileOverlap(SimRegion* region, iv3 tile, Entity* overlappingEntity)
    {
        bool result = true;
        EntityMapIterator it = {};
        while (true)
        {
            Entity* entity = YieldEntityFromTile(region->level, tile, &it);
            if (!entity) break;
            if (!overlappingEntity || entity->id != overlappingEntity->id)
            {
                result = result && EntityActions[entity->behavior.type](region, entity, overlappingEntity);
            }
        }
        return result;
    }
}
