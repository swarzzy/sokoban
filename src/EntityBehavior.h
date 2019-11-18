#pragma once
namespace soko
{
    enum [reflect] EntityBehaviorType
    {
        EntityBehavior_None,
        EntityBehavior_Spawner,
        EntityBehavior_Button,
        EntityBehavior_Portal
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
    };

    struct EntityBehavior
    {
        EntityBehaviorType type;
        EntityBehaviorData data;
    };

    struct SimEntity;
    struct SimRegion;
    internal bool ProcessEntityTileOverlap(SimRegion* region, iv3 tile, SimEntity* overlappingEntity);

#pragma pack(push, 1)
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
