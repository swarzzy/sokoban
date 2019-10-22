// This file was generated by a meta preprocessor
// Generation time (UTC): 22.10.2019 17:10:48

constant u32 ENTITY_META_TABLE_SIZE_EntityType = 8;

struct MetaTable_EntityType
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_EntityType];
    const char* names[ENTITY_META_TABLE_SIZE_EntityType];
};

constant u32 ENTITY_META_TABLE_SIZE_TestEnum = 4;

struct MetaTable_TestEnum
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_TestEnum];
    const char* names[ENTITY_META_TABLE_SIZE_TestEnum];
};

constant u32 ENTITY_META_TABLE_SIZE_EntityFlags = 4;

struct MetaTable_EntityFlags
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_EntityFlags];
    const char* names[ENTITY_META_TABLE_SIZE_EntityFlags];
};

constant u32 ENTITY_META_TABLE_SIZE_EntityMesh = 8;

struct MetaTable_EntityMesh
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_EntityMesh];
    const char* names[ENTITY_META_TABLE_SIZE_EntityMesh];
};

constant u32 ENTITY_META_TABLE_SIZE_EntityMaterial = 8;

struct MetaTable_EntityMaterial
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_EntityMaterial];
    const char* names[ENTITY_META_TABLE_SIZE_EntityMaterial];
};

constant u32 ENTITY_META_TABLE_SIZE_Direction = 8;

struct MetaTable_Direction
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_Direction];
    const char* names[ENTITY_META_TABLE_SIZE_Direction];
};

constant u32 ENTITY_META_TABLE_SIZE_TileValue = 8;

struct MetaTable_TileValue
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_TileValue];
    const char* names[ENTITY_META_TABLE_SIZE_TileValue];
};

struct MetaInfo
{
    MetaTable_EntityType EntityType;
    MetaTable_TestEnum TestEnum;
    MetaTable_EntityFlags EntityFlags;
    MetaTable_EntityMesh EntityMesh;
    MetaTable_EntityMaterial EntityMaterial;
    MetaTable_Direction Direction;
    MetaTable_TileValue TileValue;
};

inline void
AddEntry_EntityType(MetaTable_EntityType* table, EntityType at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_EntityType));
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityType - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityType; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->names[index]))
        {
            table->values[index] = (i32)at;
            table->names[index] = string;
            break;
        }
    }
 }

inline const char*
GetEnumName_EntityType(MetaTable_EntityType* table, EntityType e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityType - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityType; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->values[index] == (i32)e)
        {
            result = table->names[index];
            break;
        }
    }
    return result;
}

inline void
AddEntry_TestEnum(MetaTable_TestEnum* table, TestEnum at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_TestEnum));
    u32 hashMask = ENTITY_META_TABLE_SIZE_TestEnum - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_TestEnum; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->names[index]))
        {
            table->values[index] = (i32)at;
            table->names[index] = string;
            break;
        }
    }
 }

inline const char*
GetEnumName_TestEnum(MetaTable_TestEnum* table, TestEnum e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_TestEnum - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_TestEnum; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->values[index] == (i32)e)
        {
            result = table->names[index];
            break;
        }
    }
    return result;
}

inline void
AddEntry_EntityFlags(MetaTable_EntityFlags* table, EntityFlags at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_EntityFlags));
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityFlags - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityFlags; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->names[index]))
        {
            table->values[index] = (i32)at;
            table->names[index] = string;
            break;
        }
    }
 }

inline const char*
GetEnumName_EntityFlags(MetaTable_EntityFlags* table, EntityFlags e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityFlags - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityFlags; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->values[index] == (i32)e)
        {
            result = table->names[index];
            break;
        }
    }
    return result;
}

inline void
AddEntry_EntityMesh(MetaTable_EntityMesh* table, EntityMesh at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_EntityMesh));
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityMesh - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityMesh; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->names[index]))
        {
            table->values[index] = (i32)at;
            table->names[index] = string;
            break;
        }
    }
 }

inline const char*
GetEnumName_EntityMesh(MetaTable_EntityMesh* table, EntityMesh e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityMesh - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityMesh; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->values[index] == (i32)e)
        {
            result = table->names[index];
            break;
        }
    }
    return result;
}

inline void
AddEntry_EntityMaterial(MetaTable_EntityMaterial* table, EntityMaterial at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_EntityMaterial));
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityMaterial - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityMaterial; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->names[index]))
        {
            table->values[index] = (i32)at;
            table->names[index] = string;
            break;
        }
    }
 }

inline const char*
GetEnumName_EntityMaterial(MetaTable_EntityMaterial* table, EntityMaterial e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_EntityMaterial - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_EntityMaterial; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->values[index] == (i32)e)
        {
            result = table->names[index];
            break;
        }
    }
    return result;
}

inline void
AddEntry_Direction(MetaTable_Direction* table, Direction at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_Direction));
    u32 hashMask = ENTITY_META_TABLE_SIZE_Direction - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_Direction; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->names[index]))
        {
            table->values[index] = (i32)at;
            table->names[index] = string;
            break;
        }
    }
 }

inline const char*
GetEnumName_Direction(MetaTable_Direction* table, Direction e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_Direction - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_Direction; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->values[index] == (i32)e)
        {
            result = table->names[index];
            break;
        }
    }
    return result;
}

inline void
AddEntry_TileValue(MetaTable_TileValue* table, TileValue at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_TileValue));
    u32 hashMask = ENTITY_META_TABLE_SIZE_TileValue - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_TileValue; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->names[index]))
        {
            table->values[index] = (i32)at;
            table->names[index] = string;
            break;
        }
    }
 }

inline const char*
GetEnumName_TileValue(MetaTable_TileValue* table, TileValue e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_TileValue - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_TileValue; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->values[index] == (i32)e)
        {
            result = table->names[index];
            break;
        }
    }
    return result;
}

inline const char*
GetEnumName(MetaInfo* info, EntityType e)
{
    return GetEnumName_EntityType(&info->EntityType, e);
}

inline const char*
GetEnumName(MetaInfo* info, TestEnum e)
{
    return GetEnumName_TestEnum(&info->TestEnum, e);
}

inline const char*
GetEnumName(MetaInfo* info, EntityFlags e)
{
    return GetEnumName_EntityFlags(&info->EntityFlags, e);
}

inline const char*
GetEnumName(MetaInfo* info, EntityMesh e)
{
    return GetEnumName_EntityMesh(&info->EntityMesh, e);
}

inline const char*
GetEnumName(MetaInfo* info, EntityMaterial e)
{
    return GetEnumName_EntityMaterial(&info->EntityMaterial, e);
}

inline const char*
GetEnumName(MetaInfo* info, Direction e)
{
    return GetEnumName_Direction(&info->Direction, e);
}

inline const char*
GetEnumName(MetaInfo* info, TileValue e)
{
    return GetEnumName_TileValue(&info->TileValue, e);
}

internal MetaInfo*
InitMetaInfo(AB::MemoryArena* arena)
{
    MetaInfo* info = PUSH_STRUCT(arena, MetaInfo);
    SOKO_ASSERT(info);

    info->EntityType.isEnumClass = 0;
    info->EntityType.hint = MetaEnumHint_Sequential;
    AddEntry_EntityType(&info->EntityType, EntityType_Block, "EntityType_Block");
    AddEntry_EntityType(&info->EntityType, EntityType_Player, "EntityType_Player");
    AddEntry_EntityType(&info->EntityType, EntityType_Plate, "EntityType_Plate");
    AddEntry_EntityType(&info->EntityType, EntityType_Portal, "EntityType_Portal");
    AddEntry_EntityType(&info->EntityType, EntityType_Spikes, "EntityType_Spikes");
    AddEntry_EntityType(&info->EntityType, EntityType_Button, "EntityType_Button");
    AddEntry_EntityType(&info->EntityType, _EntityType_Count, "_EntityType_Count");

    info->TestEnum.isEnumClass = 1;
    info->TestEnum.hint = MetaEnumHint_NoHint;
    AddEntry_TestEnum(&info->TestEnum, TestEnum::Member1, "TestEnum::Member1");
    AddEntry_TestEnum(&info->TestEnum, TestEnum::Member2, "TestEnum::Member2");
    AddEntry_TestEnum(&info->TestEnum, TestEnum::Member3, "TestEnum::Member3");
    AddEntry_TestEnum(&info->TestEnum, TestEnum::_Count, "TestEnum::_Count");

    info->EntityFlags.isEnumClass = 0;
    info->EntityFlags.hint = MetaEnumHint_Flags;
    AddEntry_EntityFlags(&info->EntityFlags, EntityFlag_Collides, "EntityFlag_Collides");
    AddEntry_EntityFlags(&info->EntityFlags, EntityFlag_Movable, "EntityFlag_Movable");
    AddEntry_EntityFlags(&info->EntityFlags, EntityFlag_JustTeleported, "EntityFlag_JustTeleported");
    AddEntry_EntityFlags(&info->EntityFlags, EntityFlag_Player, "EntityFlag_Player");

    info->EntityMesh.isEnumClass = 0;
    info->EntityMesh.hint = MetaEnumHint_Sequential;
    AddEntry_EntityMesh(&info->EntityMesh, EntityMesh_Cube, "EntityMesh_Cube");
    AddEntry_EntityMesh(&info->EntityMesh, EntityMesh_Plate, "EntityMesh_Plate");
    AddEntry_EntityMesh(&info->EntityMesh, EntityMesh_Portal, "EntityMesh_Portal");
    AddEntry_EntityMesh(&info->EntityMesh, EntityMesh_Spikes, "EntityMesh_Spikes");
    AddEntry_EntityMesh(&info->EntityMesh, EntityMesh_Button, "EntityMesh_Button");
    AddEntry_EntityMesh(&info->EntityMesh, _EntityMesh_Count, "_EntityMesh_Count");

    info->EntityMaterial.isEnumClass = 0;
    info->EntityMaterial.hint = MetaEnumHint_Sequential;
    AddEntry_EntityMaterial(&info->EntityMaterial, EntityMaterial_Tile, "EntityMaterial_Tile");
    AddEntry_EntityMaterial(&info->EntityMaterial, EntityMaterial_Player, "EntityMaterial_Player");
    AddEntry_EntityMaterial(&info->EntityMaterial, EntityMaterial_Block, "EntityMaterial_Block");
    AddEntry_EntityMaterial(&info->EntityMaterial, EntityMaterial_RedPlate, "EntityMaterial_RedPlate");
    AddEntry_EntityMaterial(&info->EntityMaterial, EntityMaterial_Portal, "EntityMaterial_Portal");
    AddEntry_EntityMaterial(&info->EntityMaterial, EntityMaterial_Spikes, "EntityMaterial_Spikes");
    AddEntry_EntityMaterial(&info->EntityMaterial, EntityMaterial_Button, "EntityMaterial_Button");
    AddEntry_EntityMaterial(&info->EntityMaterial, _EntityMaterial_Count, "_EntityMaterial_Count");

    info->Direction.isEnumClass = 0;
    info->Direction.hint = MetaEnumHint_Sequential;
    AddEntry_Direction(&info->Direction, Direction_Invalid, "Direction_Invalid");
    AddEntry_Direction(&info->Direction, Direction_North, "Direction_North");
    AddEntry_Direction(&info->Direction, Direction_South, "Direction_South");
    AddEntry_Direction(&info->Direction, Direction_West, "Direction_West");
    AddEntry_Direction(&info->Direction, Direction_East, "Direction_East");
    AddEntry_Direction(&info->Direction, Direction_Up, "Direction_Up");
    AddEntry_Direction(&info->Direction, Direction_Down, "Direction_Down");
    AddEntry_Direction(&info->Direction, _Direction_Count, "_Direction_Count");

    info->TileValue.isEnumClass = 0;
    info->TileValue.hint = MetaEnumHint_Sequential;
    AddEntry_TileValue(&info->TileValue, TileValue_TileNotExist, "TileValue_TileNotExist");
    AddEntry_TileValue(&info->TileValue, TileValue_Empty, "TileValue_Empty");
    AddEntry_TileValue(&info->TileValue, TileValue_Wall, "TileValue_Wall");
    AddEntry_TileValue(&info->TileValue, TileValue_Stone, "TileValue_Stone");
    AddEntry_TileValue(&info->TileValue, TileValue_Grass, "TileValue_Grass");
    AddEntry_TileValue(&info->TileValue, _TileValue_Count, "_TileValue_Count");

    return info;
}