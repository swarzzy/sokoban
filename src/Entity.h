#pragma once
namespace soko
{
#pragma pack(push, 1)
    struct SerializedEntity
    {
        u32 id;
        u32 type;
        u32 flags;
        v3i coord;
        u32 boundPortalID;
        u32 portalDirection;
        u32 mesh;
        u32 material;
    };
#pragma pack(pop)

    internal void SerializeEntititiesToBuffer(const Level* level, void* buffer, uptr bufferSize);
    inline uptr CalcSerializedEntitiesSize(const Level* level);
    inline Entity* GetEntity(Level* level, u32 id);
    internal u32 AddEntity(Level* level, Entity entity, AB::MemoryArena* arena);
    inline u32 AddEntity(Level* level, EntityType type, v3i coord, EntityMesh mesh, EntityMaterial material, AB::MemoryArena* arena);
    internal u32 AddSerializedEntity(Level* level, AB::MemoryArena* arena, const SerializedEntity* sEntity);
}
