namespace soko
{
    inline void
    SerializeBinEntity(const Entity* e, SerializedEntity* se)
    {
        se->id = e->id;
        se->type = e->type;
        se->flags = e->flags;
        se->tile = e->coord.tile;
        se->offset = e->coord.offset;
        se->boundPortalID = e->bindedPortalID;
        se->portalDirection = e->portalDirection;
        se->mesh = e->mesh;
        se->material = e->material;
        se->movementSpeed = e->movementSpeed;
    }

    inline void
    DeserializeBinEntity(const SerializedEntity* se, Entity* e)
    {
        e->id = se->id;
        e->type = (EntityType)se->type;
        e->flags = se->flags;
        e->coord.tile = se->tile;
        e->coord.offset = se->offset;
        e->bindedPortalID = se->boundPortalID;
        e->portalDirection = (Direction)se->portalDirection;
        e->mesh = (EntityMesh)se->mesh;
        e->material = (EntityMaterial)se->material;
        e->movementSpeed = se->movementSpeed;
    }

    inline uptr
    CalcSerializedEntitiesSize(const Level* level)
    {
        u64 result = level->entityCount * sizeof(SerializedEntity);
        return result;
    }

    internal void
    SerializeEntititiesToBuffer(const Level* level, void* buffer, uptr bufferSize)
    {
        uptr bufferCapacity = bufferSize / sizeof(SerializedEntity);
        u32 entitiesWritten = 0;
        auto serializedEntities = (SerializedEntity*)buffer;

        for (u32 i = 0; i < LEVEL_ENTITY_TABLE_SIZE; i++)
        {
            const Entity* e = level->entities[i];
            while (e)
            {
                if (e->type != EntityType_Player)
                {
                    SOKO_ASSERT(bufferCapacity);
                    if (!bufferCapacity) goto end;

                    auto out = serializedEntities + entitiesWritten;

                    SerializeBinEntity(e, out);

                    entitiesWritten++;
                    bufferCapacity--;
                }

                e = e->nextEntity;
            }
        }
    end:
        return;
    }


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
    AddSerializedEntity(Level* level, const SerializedEntity* sEntity)
    {
        u32 result = 0;
        Tile* tile = GetTile(level, sEntity->tile);
        if (tile)
        {
            if (tile->value != TileValue_Wall && TileIsFree(level, sEntity->tile))
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
                    DeserializeBinEntity(sEntity, newEntity);

                    newEntity->nextEntity = bucketEntity;
                    level->entities[entityHash] = newEntity;
                    level->entitySerialNumber++;
                    level->entityCount++;

                    bool registered = RegisterEntityInTile(level, newEntity);
                    SOKO_ASSERT(registered);

                    result = newEntity->id;
                }
            }
            else
            {
                SOKO_WARN("Trying to load entity at occupied tile. Entity id: %u32. Tile coord: {%i32, %i32, %i32 }",
                          sEntity->id, sEntity->tile.x, sEntity->tile.y, sEntity->tile.z);
            }
        }
        return result;
    }

    internal u32
    AddEntity(Level* level, Entity entity)
    {
        u32 result = 0;
        Tile* tile = GetTile(level, entity.coord.tile);
        if (tile)
        {
            if (tile->value != TileValue_Wall && TileIsFree(level, entity.coord.tile))
            {
                // TODO: Better hash
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

                    bool registered = RegisterEntityInTile(level, newEntity);
                    SOKO_ASSERT(registered);

                    result = entityId;
                }
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
        entity.coord.tile = coord;
        entity.mesh = mesh;
        entity.material = material;
        entity.movementSpeed = movementSpeed;
        switch (type)
        {
        case EntityType_Block:  { entity.flags = EntityFlag_Movable | EntityFlag_Collides; } break;
        case EntityType_Player: { entity.flags = EntityFlag_Movable | EntityFlag_Collides | EntityFlag_Player; } break;
        case EntityType_Plate:  { entity.flags = 0; } break;
        case EntityType_Portal: { entity.flags = 0; } break;
        case EntityType_Spikes: { entity.flags = 0; } break;
        case EntityType_Button: { entity.flags = 0; } break;
            INVALID_DEFAULT_CASE;
        }
        result = AddEntity(level, entity);
        return result;
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

    internal bool
    ChangeEntityLocation(Level* level, Entity* entity, const WorldPos* desiredCoord);

    // TODO: Update ticks
    internal void
    UpdateEntitiesInTile(Level* level, iv3 tile)
    {
        EntityMapIterator it = {};
        while (true)
        {
            Entity* entity = YieldEntityIdFromTile(level, tile, &it);
            if (!entity) break;

            switch (entity->type)
            {
            case EntityType_Plate:
            {
                level->platePressed = false;
                EntityMapIterator it = {};
                while (true)
                {
                    Entity* e = YieldEntityIdFromTile(level, tile, &it);
                    if (!e) break;

                    if (e != entity)
                    {
                        if (e->type == EntityType_Block)
                        {
                            level->platePressed = true;
                        }
                    }
                }
            } break;
            case EntityType_Portal:
            {
                EntityMapIterator it = {};
                while (true)
                {
                    Entity* e = YieldEntityIdFromTile(level, tile, &it);
                    if (!e) break;

                    if (e != entity)
                    {
                        if (IsSet(e, EntityFlag_Movable))
                        {
                            // NOTE: No need in flag if teleporting
                            // not no the same tile where portal located
#if 0
                            if (IsSet(*e, EntityFlag_JustTeleported))
                            {
                                UnsetFlag(*e, EntityFlag_JustTeleported);
                            }
                            else
                            {
                                SetFlag(*e, EntityFlag_JustTeleported);
                                iv3 newCoord = GetEntity(level, entity->bindedPortalID)->coord + DirToUnitOffset(entity->portalDirection);
                                bool teleported = ChangeEntityLocation(level, e, newCoord, arena);
                                if (!teleported)
                                {
                                    UnsetFlag(*e, EntityFlag_JustTeleported);
                                }
                            }
#endif
                            SetFlag(e, EntityFlag_JustTeleported);
                            WorldPos newCoord = GetEntity(level, entity->bindedPortalID)->coord;
                            newCoord.tile += DirToUnitOffset(entity->portalDirection);
                            bool teleported = ChangeEntityLocation(level, e, &newCoord);

                        }
                    }
                }
            } break;
            case EntityType_Spikes: {
                EntityMapIterator it = {};
                while (true)
                {
                    Entity* e = YieldEntityIdFromTile(level, tile, &it);
                    if (!e) break;

                    if (e != entity)
                    {
                        if (e->type != EntityType_Player)
                        {
                            DeleteEntity(level, e);
                        }
                    }
                }
            } break;
            case EntityType_Button:
            {
                // TODO: Pass info about entity that causes an update
                // instead of travercing all entities all the time
                // TODO: Entity custom behavoir
#if 0
                for (Entity& e : tile->entityList)
                {
                    if (e.type == ENTITY_TYPE_BLOCK || e.type == ENTITY_TYPE_PLAYER)
                    {
                        entity->updateProc(level, &entity, entity->updateProcData);
                        break;
                    }
                }
#endif
            }
            default: {} break;
            }
        }
    }


    internal bool
    ChangeEntityLocation(Level* level, Entity* entity, const WorldPos* desiredCoord)
    {
        bool result = false;
        iv3 oldCoord = entity->coord.tile;
        Tile* oldTile = GetTile(level, entity->coord.tile);
        SOKO_ASSERT(oldTile);
        //SOKO_ASSERT(oldTile->entityList.first);

        Tile* desiredTile = GetTile(level, desiredCoord->tile);
        if (desiredTile)
        {
            bool tileIsFree = desiredTile->value != TileValue_Wall;
            if (tileIsFree)
            {
                EntityMapIterator it = {};
                while (true)
                {
                    Entity* entityInTile = YieldEntityIdFromTile(level, desiredCoord->tile, &it);
                    if (!entityInTile) break;

                    if (IsSet(entityInTile, EntityFlag_Collides))
                    {
                        tileIsFree = false;
                        break;
                    }
                }
            }
            if (tileIsFree)
            {
                UnregisterEntityInTile(level, entity);
                entity->coord = *desiredCoord;
                bool registered = RegisterEntityInTile(level, entity);
                SOKO_ASSERT(registered);

                UpdateEntitiesInTile(level, oldCoord);
                UpdateEntitiesInTile(level, desiredCoord->tile);

                result = true;
            }
        }
        return result;
    }

    internal void
    DrawEntities(Level* level, GameState* gameState)
    {
        // TODO: Entity data oriented storage
        for (u32 i = 0; i < LEVEL_ENTITY_TABLE_SIZE; i++)
        {
            Entity* entity = level->entities[i];
            while (entity)
            {
                WorldPos drawCoord = entity->coord;
                if (!entity->inTransition)
                {
                    drawCoord.offset = {};
                }
                v3 camOffset = WorldToRH(GetRelPos(gameState->session.camera.worldPos, drawCoord));
                v3 pos = camOffset;
                RenderCommandDrawMesh command = {};
                command.transform = Translation(pos);
                //SOKO_ASSERT(entity->mesh);
                //SOKO_ASSERT(entity->material);
                command.mesh = gameState->meshes + entity->mesh;
                command.material = gameState->materials + entity->material;
                RenderGroupPushCommand(gameState->renderGroup, RENDER_COMMAND_DRAW_MESH,
                                       (void*)&command);

                entity = entity->nextEntity;
            }
        }
    }


#if defined (ENTITY_TEXT_SERIALIZATION)
    // TODO: Templated bucket array?
    // Because it also used for chunk meshing
    // and in other places

    constant u32 ENTITY_STR_BLOCK_CAPACITY = 2;
    struct EntityStrBlock
    {
        EntityStrBlock* next;
        u32 at;
        u32 bytesLeft;
        char data[ENTITY_STR_BLOCK_CAPACITY];
    };

    struct EntityStr
    {
        u32 blockCount;
        EntityStrBlock* head;
        EntityStrBlock* last;
    };

    inline EntityStr
    CreateEntityStr(AB::MemoryArena* arena)
    {
        EntityStr result = {};
        EntityStrBlock* head = PUSH_STRUCT(arena, EntityStrBlock);
        if (head)
        {
            result.head = head;
            result.head->at = 0;
            result.head->bytesLeft = ENTITY_STR_BLOCK_CAPACITY;
            result.last = head;
            result.blockCount++;
        }
        return result;
    }

    // TODO: temp arena clearing policy
    internal bool
    AppendEntityStr(EntityStr* entityStr, const char* str, u32 strSize, AB::MemoryArena* arena)
    {
        bool result = false;
        u32 bytesLeft = strSize;
        u32 strAt = 0;

        while(bytesLeft)
        {
            bool hasFreeBlock = true;
            if (!entityStr->last->bytesLeft)
            {
                auto newBlock = PUSH_STRUCT(arena, EntityStrBlock);
                if (newBlock)
                {
                    newBlock->bytesLeft = ENTITY_STR_BLOCK_CAPACITY;
                    entityStr->last->next = newBlock;
                    entityStr->last = newBlock;
                    entityStr->blockCount++;
                }
                else
                {
                    hasFreeBlock = false;
                }
            }

            if (hasFreeBlock)
            {
                u32 bytesToWrite;
                u32 strAdvance;
                if (bytesLeft > entityStr->last->bytesLeft)
                {
                    bytesToWrite = entityStr->last->bytesLeft;
                    bytesLeft -= entityStr->last->bytesLeft;
                    strAdvance = entityStr->last->bytesLeft;
                }
                else
                {
                    bytesToWrite = bytesLeft;
                    strAdvance = bytesLeft;
                    bytesLeft = 0;
                    result = true;
                }

                char* writePtr = entityStr->last->data + entityStr->last->at;
                COPY_BYTES(bytesToWrite, writePtr, str + strAt);
                strAt += strAdvance;
                entityStr->last->at += bytesToWrite;
                entityStr->last->bytesLeft -= bytesToWrite;
                SOKO_ASSERT(entityStr->last->at + entityStr->last->bytesLeft == ENTITY_STR_BLOCK_CAPACITY);
            }
            else
            {
                result = false;
                break;
            }
        }
        return result;
    }

    inline void
    BeginEntity(EntityStr* str, AB::MemoryArena* arena)
    {
        AppendEntityStr(str, "! Entity\n", sizeof("! Entity\n") - 1, arena);
    }

    inline void
    EndEntity(EntityStr* str, AB::MemoryArena* arena)
    {
        AppendEntityStr(str, "\n", sizeof("\n") - 1, arena);
    }

    enum EntityFieldType
    {
        EntityFieldType_u32,
        EntityFieldType_iv3
    };

    inline void
    WriteField(EntityStr* str, const char* name, u32 nameSize,
               EntityFieldType type,
               const void* val, AB::MemoryArena* arena)
    {
        AppendEntityStr(str, "; ", sizeof("; ") - 1, arena);
        AppendEntityStr(str, name, nameSize, arena);

        const char* typeStr;
        switch (type)
        {
        case EntityFieldType_u32:
        {
            typeStr = " : u32 = ";
        } break;
        case EntityFieldType_iv3:
        {
            typeStr = " : iv3 = ";
        } break;
        INVALID_DEFAULT_CASE;
        }
        AppendEntityStr(str, typeStr, (u32)strlen(typeStr), arena);

        switch (type)
        {
        case EntityFieldType_iv3:
        {
            iv3* value = (iv3*)val;
            AppendEntityStr(str, "{ ", sizeof("{ ") - 1, arena);
            const u32 valueStrSize = 32;
            char valueStr[valueStrSize];
            _ltoa_s(value->x, valueStr, valueStrSize, 10);
            AppendEntityStr(str, valueStr, (u32)strlen(valueStr), arena);
            AppendEntityStr(str, ", ", sizeof(", ") - 1, arena);
            _ltoa_s(value->y, valueStr, valueStrSize, 10);
            AppendEntityStr(str, valueStr, (u32)strlen(valueStr), arena);
            AppendEntityStr(str, ", ", sizeof(", ") - 1, arena);
            _ltoa_s(value->z, valueStr, valueStrSize, 10);
            AppendEntityStr(str, valueStr, (u32)strlen(valueStr), arena);
            AppendEntityStr(str, " }", sizeof(" }") - 1, arena);
        } break;
        case EntityFieldType_u32:
        {
            u32 value = *(u32*)val;
            const u32 valueStrSize = 32;
            char valueStr[valueStrSize];
            _ultoa_s(value, valueStr, valueStrSize, 10);
            AppendEntityStr(str, valueStr, (u32)strlen(valueStr), arena);
        } break;
        INVALID_DEFAULT_CASE;
        }

        AppendEntityStr(str, "\n", sizeof("\n") - 1, arena);
    }

    internal void
    SerializeEntity(EntityStr* string, const Entity* e, AB::MemoryArena* tempArena)
    {
        BeginEntity(string, tempArena);
        WriteField(string, "id", sizeof("id") - 1, EntityFieldType_u32,
                   (void*)&e->id, tempArena);
        WriteField(string, "type", sizeof("type") - 1, EntityFieldType_u32,
                   (void*)&((u32)e->type), tempArena);
        WriteField(string, "flags", sizeof("flags") - 1, EntityFieldType_u32,
                   (void*)&e->flags, tempArena);
        WriteField(string, "coord", sizeof("coord") - 1, EntityFieldType_iv3,
                   (void*)&e->coord, tempArena);
        WriteField(string, "boundPortalID", sizeof("boundPortalID") - 1, EntityFieldType_u32,
                   (void*)&e->bindedPortalID, tempArena);
        WriteField(string, "portalDirection", sizeof("portalDirection") - 1, EntityFieldType_u32,
                   (void*)&e->portalDirection, tempArena);
        WriteField(string, "mesh", sizeof("mesh") - 1, EntityFieldType_u32,
                   (void*)&e->mesh, tempArena);
        WriteField(string, "material", sizeof("material") - 1, EntityFieldType_u32,
                   (void*)&e->material, tempArena);

        EndEntity(string, tempArena);
    }

    internal bool
    WriteEntityStringToFile(const wchar_t* filename, const EntityStr* string)
    {
        bool result = false;
        FileHandle handle = DebugOpenFile(filename);
        if (handle)
        {
            string->last->at--;
            EntityStrBlock* block = string->head;
            while (block)
            {
                if(!DebugWriteToOpenedFile(handle, block->data, block->at))
                {
                    break;
                }
                block = block->next;
            }
            if(DebugCloseFile(handle))
            {
                result = true;
            }
        }
        return result;
    }

    inline bool
    IsSpace(char c)
    {
        bool result;
        result = (c == ' '  ||
                  c == '\n' ||
                  c == '\r' ||
                  c == '\t' ||
                  c == '\v' ||
                  c == '\f');
        return result;
    }

    inline const char*
    EatSpace(const char* at)
    {
        const char* result = 0;
        while (*at)
        {
            if (IsSpace(*at))
            {
                at++;
            }
            else
            {
                result = at;
                break;
            }
        }
        return result;
    }

    struct EntityTokenizer
    {
        const char* string;
        const char* at;
    };

    enum EntityTokenType
    {
        EntityToken_Unkown = 0,
        EntityToken_Comment,
        EntityToken_BeginStruct,
        EntityToken_Field
    };

    struct EntityToken
    {
        EntityTokenType type;
        union
        {
            // TODO: Use offsets onstead of pointers
            struct
            {
                const char* structTypeBeg;
                const char* structTypeEnd;
            } beginStruct;
            struct
            {
                EntityFieldType type;
                const char* nameBeg;
                const char* nameEnd;
                const char* valueBeg;
                const char* valueEnd;
            } field;
        };
    };

    inline const char*
    EatUntillSpace(const char* at)
    {
        const char* result = 0;
        while(*at)
        {
            if (IsSpace(*at))
            {
                result = at;
                break;
            }
            at++;
        }
        return result;
    }

    inline const char*
    EatLine(const char* at)
    {
        const char* result = 0;
        while (*at)
        {
            if (*at == '\n' ||
                *at == '\r')
            {
                result = EatSpace(at);
                break;
            }
            at++;
        }
        return result;
    }

    inline bool
    StringsAreEqual(const char* a, const char* b)
    {
        bool result = true;
        while (*a)
        {
            if (*b != *a)
            {
                result = false;
                break;
            }
            a++;
            b++;
        }
        return result;
    }

    inline const char*
    FindNextOccurence(const char* str, char c)
    {
        const char* result = 0;
        while(*str)
        {
            if (*str == c)
            {
                result = str;
                break;
            }
            str++;
        }
        return result;
    }

    internal bool
    TokenizeFieldValue(EntityTokenizer* tokenizer, EntityToken* token)
    {
        bool result = false;
        tokenizer->at = EatSpace(tokenizer->at);
        if (tokenizer->at && *tokenizer->at == '=')
        {
            tokenizer->at = EatSpace(tokenizer->at + 1);
            if (tokenizer->at)
            {
                switch (token->field.type)
                {
                case EntityFieldType_u32:
                {
                    const char* valBeg = tokenizer->at;
                    tokenizer->at = EatUntillSpace(tokenizer->at);
                    if (tokenizer->at)
                    {
                        const char* valEnd = tokenizer->at;
                        token->field.valueBeg = valBeg;
                        token->field.valueEnd = valEnd;
                        result = true;
                    }
                } break;
                case EntityFieldType_iv3:
                {
                    if (*tokenizer->at == '{')
                    {
                        const char* valBeg = tokenizer->at;
                        tokenizer->at = FindNextOccurence(tokenizer->at, '}');
                        if (tokenizer->at)
                        {
                            tokenizer->at++;
                            const char* valEnd = tokenizer->at;
                            token->field.valueBeg = valBeg;
                            token->field.valueEnd = valEnd;
                            result = true;
                        }
                    }
                } break;
                INVALID_DEFAULT_CASE;
                }
            }
        }
        return result;
    }

    internal bool
    TokenizeField(EntityTokenizer* tokenizer, EntityToken* token)
    {
        bool result = false;
        tokenizer->at = EatSpace(tokenizer->at);
        if (tokenizer->at)
        {
            const char* nameBeg = tokenizer->at;
            tokenizer->at = EatUntillSpace(tokenizer->at);
            const char* nameEnd = tokenizer->at;
            if (nameEnd)
            {
                tokenizer->at = EatSpace(tokenizer->at);
                if (*tokenizer->at == ':')
                {
                    tokenizer->at = EatSpace(tokenizer->at + 1);
                    if (tokenizer->at)
                    {
                        const char* typeBeg = tokenizer->at;
                        const char* typeEnd = EatUntillSpace(tokenizer->at);
                        tokenizer->at = typeEnd;
                        if (typeEnd)
                        {
                            char typeBuffer[32];
                            u32 typeLength = (u32)(typeEnd - typeBeg);
                            SOKO_ASSERT(typeLength < 32);
                            COPY_BYTES(typeLength, typeBuffer, typeBeg);
                            typeBuffer[typeLength] = 0;
                            bool match = false;
                            if (StringsAreEqual(typeBuffer, "u32"))
                            {
                                token->field.type = EntityFieldType_u32;
                                match = true;
                            }
                            else if (StringsAreEqual(typeBuffer, "iv3"))
                            {
                                token->field.type = EntityFieldType_iv3;
                                match = true;
                            }
                            if (match)
                            {
                                token->field.nameBeg = nameBeg;
                                token->field.nameEnd = nameEnd;
                                if (TokenizeFieldValue(tokenizer, token))
                                {
                                    result = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }


    inline bool
    GetNextToken(EntityTokenizer* tokenizer, EntityToken* token)
    {
        bool result = true;
        tokenizer->at = EatSpace(tokenizer->at);
        if (tokenizer->at)
        {
            *token = {};
            if (*tokenizer->at == '/')
            {
                if (*(tokenizer->at + 1) == '/')
                {
                    token->type = EntityToken_Comment;
                    tokenizer->at = EatLine(tokenizer->at);
                }
            }
            else if (*tokenizer->at == '!')
            {
                tokenizer->at = EatSpace(tokenizer->at + 1);
                if (tokenizer->at)
                {
                    const char* end = EatUntillSpace(tokenizer->at);
                    if (end)
                    {
                        token->type = EntityToken_BeginStruct;
                        token->beginStruct.structTypeBeg = tokenizer->at;
                        token->beginStruct.structTypeEnd = end;
                        tokenizer->at = EatSpace(end);
                    }
                }
            }
            else if (*tokenizer->at == ';')
            {
                tokenizer->at++;
                if(TokenizeField(tokenizer, token))
                {
                    token->type = EntityToken_Field;
                }
            }
        }
        else
        {
            result = false;
        }
        return result;
    }

    internal void
    DeserializeEntities(const char* string)
    {
        EntityTokenizer tokenizer = {};
        tokenizer.string = string;
        tokenizer.at = string;

        const char* at = string;
        EntityToken token = {};

        u32 id;
        EntityType type;
        u32 flags;
        iv3 coord;
        u32 boundPortalID;
        u32 portalDirection;
        u32 mesh;
        u32 material;

        while(GetNextToken(&tokenizer, &token))
        {
            switch (token.type)
            {
            case EntityToken_Comment: {} break;
            case EntityToken_BeginStruct: {} break;
            case EntityToken_Field: {}
            default: {} break;
            }

        }
        return;
    }
#endif
}
