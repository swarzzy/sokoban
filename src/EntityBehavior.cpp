#include "EntityBehavior.h"
namespace soko
{
    // TODO: Put this to GameState?
    typedef bool(EntityOverlapBehaviorFn)(SimRegion* region, Entity* e, Entity* caller);
    // NOTE: Returns true if caller entity was moved
    internal bool EmptyOverlapBehavior(SimRegion* region, Entity* e, Entity* caller) { return false; }
    internal bool ButtonOverlapBehavior(SimRegion* region, Entity* e, Entity* caller);
    internal bool SpawnerOverlapBehavior(SimRegion* region, Entity* e, Entity* caller);
    internal bool PortalOverlapBehavior(SimRegion* region, Entity* e, Entity* caller);
    internal bool SpikesOverlapBehavior(SimRegion* region, Entity* e, Entity* caller);

    constant EntityOverlapBehaviorFn* EntityOverlapActions[TypeTraits(EntityBehaviorType)::MemberCount] =
    {
        EmptyOverlapBehavior,
        SpawnerOverlapBehavior,
        ButtonOverlapBehavior,
        PortalOverlapBehavior,
        SpikesOverlapBehavior,
        SpikesOverlapBehavior
    };

    typedef void(EntityBehaviorUpdateFn)(SimRegion* region, Entity* e);

    internal void DefaultBehaviorUpdate(SimRegion* region, Entity* e) {};
//    internal void EnemyUpdate(SimRegion* region, Entity* e);

    constant EntityBehaviorUpdateFn* EntityUpdateActions[TypeTraits(EntityBehaviorType)::MemberCount] =
    {
        DefaultBehaviorUpdate,
        DefaultBehaviorUpdate,
        DefaultBehaviorUpdate,
        DefaultBehaviorUpdate,
        DefaultBehaviorUpdate,
        //EnemyUpdate,
    };
#if 0
    internal void
    EnemyUpdate(SimRegion* region, Entity* e)
    {
        SOKO_ASSERT(e->behavior.type == EntityBehavior_Enemy);
        if (!e->inTransition)
        {
            if (e->behavior.enemy.atDest)
            {
                BeginEntityTransition(region, e, e->behavior.enemy.dir)
            }
        }
    }
#endif

    // NOTE: Returns true if COLLIDES
    internal bool
    DefaultEntityCollisionCheck(Level* level, Entity* e)
    {
        return IsSet(e, EntityFlag_Collides);
    }

    internal bool
    PortalCollisionCheck(Level* level, Entity* e)
    {
        bool result = true;
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
        return result;
    }

    internal bool
    SpikesOverlapBehavior(SimRegion* region, Entity* e, Entity* caller)
    {
        bool deleted = false;
        SOKO_ASSERT(e->behavior.type == EntityBehavior_Spikes);
        if (caller && caller->type != EntityType_Player)
        {
            // TODO: Delete from sim region
            DeleteEntity(region->level, caller);
            deleted = true;
        }
        return deleted;
    }

    internal bool
    PortalOverlapBehavior(SimRegion* region, Entity* e, Entity* caller)
    {
        bool result = false;
        auto data = &e->behavior.data.portal;
        if (caller && IsSet(caller, EntityFlag_Movable))
        {
            if (data->destPortalID)
            {
                Entity* destPortal = GetEntity(region, data->destPortalID);
                if (destPortal && destPortal->behavior.type == EntityBehavior_Portal)
                {
                    iv3 destPos = destPortal->behavior.data.portal.teleportP;
                    if (CheckTile(region->level, destPos))
                    {
                        if (ChangeEntityLocation(region, caller, destPos))
                        {
                            result = true;
                        }
                    }
                }
            }
        }
        return result;
    }

    internal bool
    ButtonOverlapBehavior(SimRegion* region, Entity* e, Entity* caller)
    {
        if (caller && IsSet(caller, EntityFlag_Collides))
        {
            auto data = &e->behavior.data.button;
            if (data->boundEntityID)
            {
                Entity* boundE = GetEntity(region, data->boundEntityID);
                if (boundE)
                {
                    EntityOverlapActions[boundE->behavior.type](region, boundE, e);
                }
            }
        }
        return false;
    }

    internal bool
    SpawnerOverlapBehavior(SimRegion* region, Entity* e, Entity* caller)
    {
        // TODO: Check is p valid?
        auto data = &e->behavior.data.spawner;
        if (CheckTile(region->level, data->spawnP))
        {
            u32 id = AddEntity(region->level, data->entityType, data->spawnP, 2.0f, EntityMesh_Box, EntityMaterial_Box);
            if (id)
            {
                // TODO: Manage entity memory in regions
                AddEntityToRegion(region, GetEntity(region->level, id));
            }
        }
        return false;
    }

    // TODO: Clean the sim and stored entity concept up
    // TODO: Update ticks
    internal bool
    ProcessEntityTileOverlap(SimRegion* region, iv3 tile, Entity* overlappingEntity)
    {
        bool result = false;
        EntityMapIterator it = {};
        while (true)
        {
            Entity* entity = YieldEntityFromTile(region->level, tile, &it);
            if (!entity) break;
            if (!overlappingEntity || entity->id != overlappingEntity->id)
            {
                result = result || EntityOverlapActions[entity->behavior.type](region, entity, overlappingEntity);
            }
        }
        return result;
    }
}
