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
    GetEnumName(MetaInfo* info, ...)
    {
        return UNKNOWN_ENUM_VALUE;
    }

    struct MetaInfo;

    internal MetaInfo* InitMetaInfo(AB::MemoryArena* arena);

#include "MetaInfo_Generated.cpp"
}
