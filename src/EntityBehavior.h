#pragma once
namespace soko
{
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
        EntityBehaviorData data;
    };
    internal void UpdateEntity(Level* level, Entity* e);
    internal bool ProcessEntityTileOverlap(Level* level, iv3 tile, Entity* overlappingEntity);
    internal bool EntityCollides(const Level* level, const Entity* e);

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
