#pragma once
namespace soko
{
    enum [reflect] EntityBehaviorType
    {
        EntityBehavior_None,
        EntityBehavior_Spawner,
        EntityBehavior_Button,
        EntityBehavior_Portal,
        EntityBehavior_Spikes,
        EntityBehavior_Enenmy,
        EntityBahavior_Lift
    };

    struct LiftBehaviorData
    {
        u32 height;
    };

    struct EnemyBehaviorData
    {
        Direction dir;
        u32 count;
        b32 atDest;
    };

    struct PortalBehaviorData
    {
        u32 destPortalID;
        iv3 teleportP;
    };

    struct ButtonBehaviorData
    {
        u32 boundEntityID;
    };

    struct SpawnerBehaviorData
    {
        iv3 spawnP;
        EntityType entityType;
    };

    union EntityBehaviorData
    {
        SpawnerBehaviorData spawner;
        ButtonBehaviorData button;
        PortalBehaviorData portal;
        EnemyBehaviorData enemy;
    };

    struct EntityBehavior
    {
        EntityBehaviorType type;
        EntityBehaviorData data;
    };

    internal bool ProcessEntityTileOverlap(Level* level, iv3 tile, Entity* overlappingEntity);
    internal bool EntityCollides(Level* level, Entity* e);

#pragma pack(push, 1)

    struct SerializedLiftBehaviorData
    {
        u32 height;
    };

    struct SerializedEnemyBehaviorData
    {
        u32 dir;
        u32 count;
        b32 atDest;
    };

    struct SerializedPortalBehaviorData
    {
        u32 destPortalID;
        iv3 teleportP;
    };

    struct SerializedButtonBehaviorData
    {
        u32 boundEntityID;
    };

    struct SerializedSpawnerBehaviorData
    {
        iv3 spawnP;
        u32 entityType;
    };

    union SerializedEntityBehaviorData
    {
        SerializedSpawnerBehaviorData spawner;
        SerializedButtonBehaviorData button;
        SerializedPortalBehaviorData portal;
    };
#pragma pack(pop)

}
