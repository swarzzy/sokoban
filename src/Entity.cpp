namespace soko
{
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
        EntityFieldType_v3i
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
        case EntityFieldType_v3i:
        {
            typeStr = " : v3i = ";
        } break;
            INVALID_DEFAULT_CASE;
        }
        AppendEntityStr(str, typeStr, (u32)strlen(typeStr), arena);

        switch (type)
        {
        case EntityFieldType_v3i:
        {
            v3i* value = (v3i*)val;
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
        WriteField(string, "coord", sizeof("coord") - 1, EntityFieldType_v3i,
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
}
