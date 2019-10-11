#pragma once
namespace soko
{

#pragma pack(push, 1)
    struct SerializedEntity
    {
        u32 id;
        u32 type;
        u32 flags;
        v3i tile;
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
    inline u32 AddEntity(Level* level, EntityType type, v3i coord, f32 movementSpeed, EntityMesh mesh, EntityMaterial material);
    internal u32 AddSerializedEntity(Level* level, const SerializedEntity* sEntity);
}
