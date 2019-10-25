namespace soko::meta
{
    enum MetaEnumHint
    {
        MetaEnumHint_NoHint = 0,
        MetaEnumHint_Sequential,
        MetaEnumHint_Flags
    };

    constant const char* UNKNOWN_ENUM_VALUE = "<unknown enum value>";

    inline const char*
    GetEnumName(...)
    {
        return UNKNOWN_ENUM_VALUE;
    }

    struct MetaInfo;

    internal MetaInfo* InitMetaInfo(AB::MemoryArena* arena);

// TODO: Functions for this and bounds checking
#define TypeTraits(type) soko::meta::MetaTable_##type
#define TypeInfo(type) GlobalMetaInfo.##type
#define EnumeratorName meta::GetEnumName
}
