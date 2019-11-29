namespace soko
{
    inline void
    SerializeBinEntityV1(const Entity* e, SerializedEntityV1* se)
    {
        se->id = e->id;
        se->type = e->type;
        se->flags = e->flags;
        se->tile = e->pos;
        se->offset = e->offset;
        se->boundPortalID = e->bindedPortalID;
        se->portalDirection = e->portalDirection;
        se->mesh = e->mesh;
        se->material = e->material;
        se->movementSpeed = e->movementSpeed;

        se->materialRoughness = e->materialRoughness;
        se->materialMetallic = e->materialMetallic;
        se->materialAlbedo = e->materialAlbedo;
    }

    inline void
    DeserializeBinEntityV1(const SerializedEntityV1* se, Entity* e)
    {
        e->id = se->id;
        e->type = (EntityType)se->type;
        e->flags = se->flags;
        e->pos = se->tile;
        e->offset = se->offset;
        e->bindedPortalID = se->boundPortalID;
        e->portalDirection = (Direction)se->portalDirection;
        e->mesh = (EntityMesh)se->mesh;
        e->material = (EntityMaterial)se->material;
        e->movementSpeed = se->movementSpeed;

        e->materialRoughness = se->materialRoughness;
        e->materialMetallic = se->materialMetallic;
        e->materialAlbedo = se->materialAlbedo;
    }

    inline void
    SerializeBinEntityV2(const Entity* e, SerializedEntityV2* se)
    {
        se->id = e->id;
        se->type = e->type;
        se->flags = e->flags;
        se->tile = e->pos;
        se->offset = e->offset;
        se->footprintDim = e->footprintDim;
        se->boundPortalID = e->bindedPortalID;
        se->portalDirection = e->portalDirection;
        se->mesh = e->mesh;
        se->material = e->material;
        se->movementSpeed = e->movementSpeed;

        switch (e->type)
        {
        case EntityType_Spikes:
        case EntityType_Button: { se->behaviorData.button.boundEntityID = e->behavior.data.button.boundEntityID; } break;
        case EntityType_Spawner: { se->behaviorData.spawner.spawnP = e->behavior.data.spawner.spawnP; se->behaviorData.spawner.entityType = e->behavior.data.spawner.entityType; } break;
        case EntityType_Portal: { se->behaviorData.portal.destPortalID = e->behavior.data.portal.destPortalID; se->behaviorData.portal.teleportP = e->behavior.data.portal.teleportP;} break;
        default: {} break;
        }

        se->materialRoughness = e->materialRoughness;
        se->materialMetallic = e->materialMetallic;
        se->materialAlbedo = e->materialAlbedo;
    }

    inline void
    DeserializeBinEntityV2(const SerializedEntityV2* se, Entity* e)
    {
        e->id = se->id;
        e->type = (EntityType)se->type;
        e->flags = se->flags;
        e->pos = se->tile;
        e->offset = se->offset;
        e->footprintDim = se->footprintDim;
        e->bindedPortalID = se->boundPortalID;
        e->portalDirection = (Direction)se->portalDirection;
        e->mesh = (EntityMesh)se->mesh;
        e->material = (EntityMaterial)se->material;
        e->movementSpeed = se->movementSpeed;

        switch (e->type)
        {
        case EntityType_Button: { e->behavior.data.button.boundEntityID = se->behaviorData.button.boundEntityID; } break;
        case EntityType_Spawner: { e->behavior.data.spawner.spawnP = se->behaviorData.spawner.spawnP; e->behavior.data.spawner.entityType = (EntityType)se->behaviorData.spawner.entityType; } break;
        case EntityType_Portal: { e->behavior.data.portal.destPortalID = se->behaviorData.portal.destPortalID; e->behavior.data.portal.teleportP = se->behaviorData.portal.teleportP;} break;
        default: {} break;
        }

        e->materialRoughness = se->materialRoughness;
        e->materialMetallic = se->materialMetallic;
        e->materialAlbedo = se->materialAlbedo;
    }

    inline uptr
    CalcSerializedEntitiesSize(const Level* level)
    {
        uptr result = level->entityCount * sizeof(SerializedEntityV2);
        return result;
    }

    internal u32
    SerializeEntititiesToBuffer(const Level* level, void* buffer, uptr bufferSize)
    {
        uptr bufferCapacity = bufferSize / sizeof(SerializedEntityV2);
        u32 entitiesWritten = 0;
        auto serializedEntities = (SerializedEntityV2*)buffer;

        for (u32 i = 0; i < LEVEL_ENTITY_TABLE_SIZE; i++)
        {
            const Entity* e = level->entities[i];
            while (e)
            {
                if (e->id != 0 &&
                    e->type != EntityType_Player)
                {
                    SOKO_ASSERT(bufferCapacity);
                    if (!bufferCapacity) goto end;

                    auto out = serializedEntities + entitiesWritten;

                    SerializeBinEntityV2(e, out);

                    entitiesWritten++;
                    bufferCapacity--;
                }

                e = e->nextEntity;
            }
        }
    end:
        return entitiesWritten;
    }

#if 0
    internal void
    ChangeEntityType(Entity* entity, EntityType newType)
    {
        entity->type = newType;
        entity->flags = EntityTypesFlags[newType];
    }
#endif

    inline Entity*
    GetEntityMemory(Level* level)
    {
        Entity* result = nullptr;
        if (level->entityFreeList)
        {
            result = level->entityFreeList;
            level->entityFreeList = level->entityFreeList->nextEntity;
            level->deletedEntityCount--;
            ZERO_STRUCT(Entity, result);
        }
        else
        {
            result = PUSH_STRUCT(level->sessionArena, Entity);
        }
        return result;
    }

    internal void
    DeleteEntity(Level* level, Entity* entity)
    {
        u32 entityHash = entity->id % LEVEL_ENTITY_TABLE_SIZE;
        Entity* prevEntity = nullptr;
        Entity* bucketEntity = level->entities[entityHash];
        while (bucketEntity)
        {
            if (bucketEntity->id == entity->id)
            {
                if (prevEntity)
                {
                    prevEntity->nextEntity = entity->nextEntity;
                }
                else
                {
                    level->entities[entityHash] = entity->nextEntity;
                }

                UnregisterEntityInTile(level, entity);

                level->entityCount--;
                entity->nextEntity = level->entityFreeList;
                level->entityFreeList = entity;
                level->deletedEntityCount++;
                break;
            }
            prevEntity = bucketEntity;
            bucketEntity = bucketEntity->nextEntity;
        }
    }

    // TODO: @Cleanup @Robustness Collapse repeated code in theese funtions
    internal u32
    AddSerializedEntity(Level* level, const SerializedEntityV2* sEntity)
    {
        u32 result = 0;
        if (CheckTile(level, sEntity->tile))
        {
            // TODO: Better hash
            u32 entityHash = sEntity->id % LEVEL_ENTITY_TABLE_SIZE;

#if defined(SOKO_DEBUG)
            if (level->entities[entityHash])
            {
                Entity* e = level->entities[entityHash];
                while (e)
                {
                    SOKO_ASSERT(e->id != sEntity->id);
                }
            }
#endif
            Entity* bucketEntity = level->entities[entityHash];
            Entity* newEntity = GetEntityMemory(level);

            if (newEntity)
            {
                DeserializeBinEntityV2(sEntity, newEntity);

                newEntity->nextEntity = bucketEntity;
                level->entities[entityHash] = newEntity;
                if (newEntity->id > level->entitySerialNumber)
                {
                    level->entitySerialNumber = newEntity->id + 1;
                }
                else
                {
                    level->entitySerialNumber++;
                }
                level->entityCount++;

                RegisterEntityInTile(level, newEntity);

                result = newEntity->id;
            }
        }
        else
        {
            SOKO_WARN("Trying to load entity at occupied tile. Entity id: %u32. Tile coord: {%i32, %i32, %i32 }",
                      sEntity->id, sEntity->tile.x, sEntity->tile.y, sEntity->tile.z);
        }

        return result;
    }

    internal u32
    AddEntity(Level* level, Entity entity)
    {
        u32 result = 0;
        if (CheckTile(level, entity.pos))
        {
            // TODO: Better hash
            entity.footprintDim = UV3(1);
            u32 entityId = level->entitySerialNumber + 1;
            u32 entityHash = entityId % LEVEL_ENTITY_TABLE_SIZE;
            Entity* bucketEntity = level->entities[entityHash];
            Entity* newEntity = GetEntityMemory(level);
            if (newEntity)
            {
                newEntity->nextEntity = bucketEntity;
                level->entities[entityHash] = newEntity;
                level->entitySerialNumber++;
                level->entityCount++;

                *newEntity = entity;
                newEntity->id = entityId;

                RegisterEntityInTile(level, newEntity);

                result = entityId;
            }
        }
        return result;
    }

    inline u32
    AddEntity(Level* level, EntityType type, iv3 coord, f32 movementSpeed,
              EntityMesh mesh, EntityMaterial material)
    {
        u32 result = 0;
        Entity entity = {};
        entity.type = type;
        entity.pos = coord;
        entity.mesh = mesh;
        entity.material = material;
        entity.movementSpeed = movementSpeed;
        entity.flags = EntityTypesFlags[type];
        result = AddEntity(level, entity);
        return result;
    }

    inline void
    ChangeEntityFootprint(Level* level, Entity* e, uv3 newDim)
    {
        UnregisterEntityInTile(level, e);
        e->footprintDim = newDim;
        RegisterEntityInTile(level, e);
    }

    inline Entity*
    GetEntity(Level* level, u32 id)
    {
        Entity* result = nullptr;
        u32 entityHash = id % LEVEL_ENTITY_TABLE_SIZE;
        Entity* entity = level->entities[entityHash];
        while (entity)
        {
            if (entity->id == id)
            {
                result = entity;
                break;
            }
            entity = entity->nextEntity;
        }
        return result;
    }

    inline const Entity*
    GetEntity(const Level* level, u32 id)
    {
        return GetEntity((Level*)level, id);
    }
}
