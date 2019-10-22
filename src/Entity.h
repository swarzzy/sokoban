#pragma once
namespace soko
{
    enum [reflect seq_enum] EntityType
    {
        EntityType_Block = 0,
        EntityType_Player,
        EntityType_Plate,
        EntityType_Portal,
        EntityType_Spikes,
        EntityType_Button,
        _EntityType_Count
    };

    enum class [reflect] TestEnum
    {
        Member1 = 0, Member2, Member3, _Count
    };

    enum [reflect flag_enum] EntityFlags : u32
    {
        EntityFlag_Collides = (1 << 1),
        EntityFlag_Movable = (1 << 2),
        EntityFlag_JustTeleported = (1 << 3),
        EntityFlag_Player = (1 << 4)
    };

    struct SimEntity;
    struct Entity
    {
        u32 id;
        SimEntity* sim;
        EntityType type;
        u32 flags;
        WorldPos coord;
        EntityMesh mesh;
        EntityMaterial material;

        u32 bindedPortalID;
        Direction portalDirection;
        // TODO: Entity custom behavior
        //void* updateProcData;
        //UpdateProcFn* updateProc;

        // NOTE: Tile transitions variables
        // TODO: Stop storing transition vriables in all entities
        // even if the entity never does transitions
        b32 inTransition;
        v3 transitionFullPath;
        v3 transitionTraveledPath;
        f32 transitionSpeed;
        iv3 transitionOrigin;
        iv3 transitionDest;

        f32 movementSpeed;

        Entity* nextEntity;
    };

#pragma pack(push, 1)
    struct SerializedEntity
    {
        u32 id;
        u32 type;
        u32 flags;
        iv3 tile;
        v3 offset;
        u32 boundPortalID;
        u32 portalDirection;
        u32 mesh;
        u32 material;
        f32 movementSpeed;
    };
#pragma pack(pop)

    internal void SerializeEntititiesToBuffer(const Level* level, void* buffer, uptr bufferSize);
    inline uptr CalcSerializedEntitiesSize(const Level* level);
    inline Entity* GetEntity(Level* level, u32 id);
    internal u32 AddEntity(Level* level, Entity entity);
    inline u32 AddEntity(Level* level, EntityType type, iv3 coord, f32 movementSpeed, EntityMesh mesh, EntityMaterial material);
    internal u32 AddSerializedEntity(Level* level, const SerializedEntity* sEntity);

    inline bool IsSet(const Entity* entity, u32 flag)
    {
        return entity->flags & flag;
    }

    inline void SetFlag(Entity* entity, u32 flag)
    {
        entity->flags |= flag;
    }

    inline void UnsetFlag(Entity* entity, u32 flag)
    {
        entity->flags &= (~flag);
    }
}
