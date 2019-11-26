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
        EntityBehavior_Player
    };

    struct PlayerBehaviorData
    {
        bool reversed;
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
        PlayerBehaviorData player;
    };

    struct EntityBehavior
    {
        EntityBehaviorType type;
        EntityBehaviorData data;
    };
    internal void UpdateEntity(Level* level, Entity* e);
    internal bool ProcessEntityTileOverlap(Level* level, iv3 tile, Entity* overlappingEntity);
    internal bool EntityCollides(Level* level, Entity* e);

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
