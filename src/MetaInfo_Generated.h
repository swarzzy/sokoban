// This file was generated by a meta preprocessor
// Generation time (UTC): 13.12.2019 17:52:27

#pragma once

namespace soko 
{
enum  EntityType;
enum  EntityFlags : u32;
enum  EntityMesh;
enum  EntityMaterial;
enum  Direction : u32;
enum  TileValue : u8;
}

namespace soko { namespace meta 
{

struct MetaTable_EntityType
{
    constant u32 MemberCount = 8;
    constant b32 IsEnumClass = 0;
    constant char* Name = "EntityType";
    constant char* UnderlyingTypeName = "int";
    constant MetaEnumHint MetaHint = MetaEnumHint_NoHint;
    EntityType values[MemberCount];
    const char* names[MemberCount];

    constant u32 _HashMapSize = 8;
    EntityType _hashMapValues[_HashMapSize];
    u32 _hashMapIndices[_HashMapSize];
};

struct MetaTable_EntityFlags
{
    constant u32 MemberCount = 4;
    constant b32 IsEnumClass = 0;
    constant char* Name = "EntityFlags";
    constant char* UnderlyingTypeName = "u32";
    constant MetaEnumHint MetaHint = MetaEnumHint_Flags;
    EntityFlags values[MemberCount];
    const char* names[MemberCount];

    constant u32 _HashMapSize = 4;
    EntityFlags _hashMapValues[_HashMapSize];
    u32 _hashMapIndices[_HashMapSize];
};

struct MetaTable_EntityMesh
{
    constant u32 MemberCount = 10;
    constant b32 IsEnumClass = 0;
    constant char* Name = "EntityMesh";
    constant char* UnderlyingTypeName = "int";
    constant MetaEnumHint MetaHint = MetaEnumHint_Sequential;
    EntityMesh values[MemberCount];
    const char* names[MemberCount];

    constant u32 _HashMapSize = 16;
    EntityMesh _hashMapValues[_HashMapSize];
    u32 _hashMapIndices[_HashMapSize];
};

struct MetaTable_EntityMaterial
{
    constant u32 MemberCount = 15;
    constant b32 IsEnumClass = 0;
    constant char* Name = "EntityMaterial";
    constant char* UnderlyingTypeName = "int";
    constant MetaEnumHint MetaHint = MetaEnumHint_Sequential;
    EntityMaterial values[MemberCount];
    const char* names[MemberCount];

    constant u32 _HashMapSize = 16;
    EntityMaterial _hashMapValues[_HashMapSize];
    u32 _hashMapIndices[_HashMapSize];
};

struct MetaTable_Direction
{
    constant u32 MemberCount = 7;
    constant b32 IsEnumClass = 0;
    constant char* Name = "Direction";
    constant char* UnderlyingTypeName = "u32";
    constant MetaEnumHint MetaHint = MetaEnumHint_Sequential;
    Direction values[MemberCount];
    const char* names[MemberCount];

    constant u32 _HashMapSize = 8;
    Direction _hashMapValues[_HashMapSize];
    u32 _hashMapIndices[_HashMapSize];
};

struct MetaTable_TileValue
{
    constant u32 MemberCount = 4;
    constant b32 IsEnumClass = 0;
    constant char* Name = "TileValue";
    constant char* UnderlyingTypeName = "u8";
    constant MetaEnumHint MetaHint = MetaEnumHint_Sequential;
    TileValue values[MemberCount];
    const char* names[MemberCount];

    constant u32 _HashMapSize = 4;
    TileValue _hashMapValues[_HashMapSize];
    u32 _hashMapIndices[_HashMapSize];
};

struct MetaInfo
{
    MetaTable_EntityType EntityType;
    MetaTable_EntityFlags EntityFlags;
    MetaTable_EntityMesh EntityMesh;
    MetaTable_EntityMaterial EntityMaterial;
    MetaTable_Direction Direction;
    MetaTable_TileValue TileValue;
};

inline void
AddEntry_EntityType(MetaTable_EntityType* table, u32 enumeratorIndex, EntityType value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_EntityType::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_EntityType::_HashMapSize - 1;
    u32 hash = (u32)value & hashMask;

    for (u32 offset = 0; offset < MetaTable_EntityType::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->_hashMapIndices[index]))
        {
            table->_hashMapValues[index] = value;
            table->_hashMapIndices[index] = enumeratorIndex;
            break;
        }
    }
}

inline const char*
GetEnumName(EntityType e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->EntityType;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_EntityType::_HashMapSize - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < MetaTable_EntityType::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_EntityType::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

inline void
AddEntry_EntityFlags(MetaTable_EntityFlags* table, u32 enumeratorIndex, EntityFlags value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_EntityFlags::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_EntityFlags::_HashMapSize - 1;
    // TODO: Check this guy for performance
    auto bitScanResult = FindLeastSignificantBitSet((u32)value);
    u32 hash = bitScanResult.found ? (bitScanResult.index &  hashMask) : 0;

    for (u32 offset = 0; offset < MetaTable_EntityFlags::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->_hashMapIndices[index]))
        {
            table->_hashMapValues[index] = value;
            table->_hashMapIndices[index] = enumeratorIndex;
            break;
        }
    }
}

inline const char*
GetEnumName(EntityFlags e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->EntityFlags;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_EntityFlags::_HashMapSize - 1;
    // TODO: Check this guy for performance
    auto bitScanResult = FindLeastSignificantBitSet((u32)e);
    u32 hash = bitScanResult.found ? (bitScanResult.index &  hashMask) : 0;

    for (u32 offset = 0; offset < MetaTable_EntityFlags::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_EntityFlags::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

inline void
AddEntry_EntityMesh(MetaTable_EntityMesh* table, u32 enumeratorIndex, EntityMesh value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_EntityMesh::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_EntityMesh::_HashMapSize - 1;
    u32 hash = (u32)value & hashMask;

    for (u32 offset = 0; offset < MetaTable_EntityMesh::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->_hashMapIndices[index]))
        {
            table->_hashMapValues[index] = value;
            table->_hashMapIndices[index] = enumeratorIndex;
            break;
        }
    }
}

inline const char*
GetEnumName(EntityMesh e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->EntityMesh;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_EntityMesh::_HashMapSize - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < MetaTable_EntityMesh::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_EntityMesh::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

inline void
AddEntry_EntityMaterial(MetaTable_EntityMaterial* table, u32 enumeratorIndex, EntityMaterial value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_EntityMaterial::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_EntityMaterial::_HashMapSize - 1;
    u32 hash = (u32)value & hashMask;

    for (u32 offset = 0; offset < MetaTable_EntityMaterial::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->_hashMapIndices[index]))
        {
            table->_hashMapValues[index] = value;
            table->_hashMapIndices[index] = enumeratorIndex;
            break;
        }
    }
}

inline const char*
GetEnumName(EntityMaterial e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->EntityMaterial;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_EntityMaterial::_HashMapSize - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < MetaTable_EntityMaterial::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_EntityMaterial::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

inline void
AddEntry_Direction(MetaTable_Direction* table, u32 enumeratorIndex, Direction value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_Direction::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_Direction::_HashMapSize - 1;
    u32 hash = (u32)value & hashMask;

    for (u32 offset = 0; offset < MetaTable_Direction::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->_hashMapIndices[index]))
        {
            table->_hashMapValues[index] = value;
            table->_hashMapIndices[index] = enumeratorIndex;
            break;
        }
    }
}

inline const char*
GetEnumName(Direction e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->Direction;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_Direction::_HashMapSize - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < MetaTable_Direction::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_Direction::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

inline void
AddEntry_TileValue(MetaTable_TileValue* table, u32 enumeratorIndex, TileValue value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_TileValue::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_TileValue::_HashMapSize - 1;
    u32 hash = (u32)value & hashMask;

    for (u32 offset = 0; offset < MetaTable_TileValue::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (!(table->_hashMapIndices[index]))
        {
            table->_hashMapValues[index] = value;
            table->_hashMapIndices[index] = enumeratorIndex;
            break;
        }
    }
}

inline const char*
GetEnumName(TileValue e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->TileValue;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_TileValue::_HashMapSize - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < MetaTable_TileValue::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_TileValue::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

}}