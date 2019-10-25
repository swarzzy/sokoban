// NOTE: Exceptions in std::vector are complaining
// Do not ever listen to exceptions and always kick it out from your code
#pragma warning(disable : 4530)

//
// TODO: Actually use hash hints!!
//

#define AB_PLATFORM_WINDOWS
#include "Platform.h"
using namespace AB;
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <assert.h>
#include <time.h>

constant const char* DEFAULT_ENUM_TYPE = "int";

struct EnumMember
{
    u32 index;
    char* name;
};

enum MetaEnumHint
{
    MetaEnumHint_NoHint = 0,
    MetaEnumHint_Sequential,
    MetaEnumHint_Flags
};

struct EnumMetaInfo
{
    char* name;
    char* type;
    bool isEnumClass;
    MetaEnumHint hint;
    std::vector<EnumMember> members;
};

internal char*
ReadEntireFileAsText(const char* filename, u32* bytesRead)
{
    char* result = 0;
    *bytesRead = 0;
    LARGE_INTEGER fileSize = {};
    HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            if (fileSize.QuadPart < 0xffffffff)
            {
                void* data = malloc(fileSize.QuadPart + 1);
                if (data)
                {
                    DWORD read;
                    if (ReadFile(fileHandle, data, (DWORD)fileSize.QuadPart, &read, 0) && (read == (DWORD)fileSize.QuadPart))
                    {
                        result = (char*)data;
                        ((byte*)data)[fileSize.QuadPart] = 0;
                        *bytesRead = (u32)fileSize.QuadPart + 1;
                    }
                    else
                    {
                        free(data);
                    }
                }
            }
        }
        CloseHandle(fileHandle);
    }
    return  result;
}

struct Tokenizer
{
    char* text;
    char* at;
    b32 eof;
};

inline bool
MatchStrings(const char* str1, const char* str2)
{
    bool result = true;
    while (*str2)
    {
        if (*str1 == 0)
        {
            result = false;
            break;
        }
        if (*str1 != *str2)
        {
            result = false;
            break;
        }
        str1++;
        str2++;
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

inline char* EatLine(char* at);
inline char* EatMultiLineComment(char* at);

inline char*
EatSpace(char* at)
{
    char* result = 0;
    while (*at)
    {
        if (MatchStrings(at, "//"))
        {
            at = EatLine(at);
        }
        else if (MatchStrings(at, "/*"))
        {
            at = EatMultiLineComment(at);
        }
        else if (IsSpace(*at))
        {
            at++;
        }
        else
        {
            result = at;
            return result;
        }
    }
    result = at;
    return result;
}

inline char*
EatSpaceBackwards(char* at)
{
    char* result = 0;
    while (*at)
    {
        if (MatchStrings(at, "//"))
        {
            at = EatLine(at);
        }
        else if (MatchStrings(at, "/*"))
        {
            at = EatMultiLineComment(at);
        }
        else if (IsSpace(*at))
        {
            at--;
        }
        else
        {
            result = at;
            return result;
        }
    }
    result = at;
    return result;
}

inline char*
EatLine(char* at)
{
    char* result = 0;
    while (*at)
    {
        if (*at == '\n' ||
            *at == '\r')
        {
            result = EatSpace(at);
            return result;
        }
        at++;
    }
    result = at;
    return result;
}

inline char*
EatMultiLineComment(char* at)
{
    char* result = 0;
    u32 opened = 0;
    u32 closed = 0;

    while (*at)
    {
        if (*at == '/' && *(at + 1) == '*')
        {
            opened++;
        }
        else if (*at == '*' && *(at + 1) == '/')
        {
            closed++;
        }
        if (opened == closed)
        {
            result = at + 2;
            return result;
        }
        at++;
    }

    result = at;
    printf("Error: Unbalanced multiline comment found\n");

    return result;
}

inline char*
EatStringLiteral(char* at)
{
    char* result = 0;

    at++;

    while (*at)
    {
        if (*at == '\\' && *(at + 1)  == '\"')
        {
            at += 2;
            continue;
        }
        else if (*at == '\"')
        {
            result = at + 1;
            return result;
        }
        at++;
    }

    result = at;
    printf("Error: Unbalanced string literal found\n");
    return result;
}

inline char*
EatWord(char* at)
{
    char* result = 0;
    while (*at)
    {
        if (((!IsSpace(*at)) && (*at != '=') && (*at != ',')))
        {
            at++;
        }
        else
        {
            result = at;
            return result;
        }
    }

    result = at;
    return result;
}

inline char*
EatUntilChar(char* at, char c)
{
    char* result = 0;
    while (*at)
    {
        if (*at != c)
        {
            at++;
        }
        else
        {
            result = at;
            return result;
        }
    }

    result = at;
    return result;
}

inline char
EatUntilChar2(char** _at, char c1, char c2)
{
    char result = 0;
    char* at = *_at;
    while (*at)
    {
        if (*at == c1)
        {
            result = c1;
            *_at = at;
            return result;
        }
        if (*at == c2)
        {
            result = c2;
            *_at = at;
            return result;
        }
        at++;
    }
    *_at = at;
    return result;
}

internal void
EatUntilEnum(Tokenizer* tokenizer)
{
    while (*tokenizer->at)
    {
        if (*tokenizer->at == '/' && *(tokenizer->at + 1) == '/') // Comment
        {
            tokenizer->at = EatLine(tokenizer->at);
        }
        // TODO: \ symbol whith prevents new line in macros is not supported for now
        else if (*tokenizer->at == '#')
        {
            tokenizer->at = EatLine(tokenizer->at);
        }
        else if (*tokenizer->at == '/' && *(tokenizer->at + 1) == '*')
        {
            tokenizer->at = EatMultiLineComment(tokenizer->at);
        }
        else if (*tokenizer->at == '\\' && *(tokenizer->at + 1) == '\\' && *(tokenizer->at + 2) == '\"')
        {
            tokenizer->at += 3;
        }
        else if (*tokenizer->at == '\"')
        {
            tokenizer->at = EatStringLiteral(tokenizer->at);
        }
        else if (MatchStrings(tokenizer->at, "enum"))
        {
            return;
        }
        else
        {
            tokenizer->at++;
        }
    }
    tokenizer->eof = true;
}

internal EnumMetaInfo
TokenizeEnum(Tokenizer* tokenizer)
{
    EnumMetaInfo info = {};
    char* at = tokenizer->at;

    if (MatchStrings(at, "enum"))
    {
        at += 4;
        at = EatSpace(at);

        if (MatchStrings(at, "class"))
        {
            info.isEnumClass = true;
            at += sizeof("class") - 1;
        }
        else if (MatchStrings(at, "struct"))
        {
            info.isEnumClass = true;
            at += sizeof("struct") - 1;
        }

        at = EatSpace(at);

        if (*at == '[')
        {
            at++;
            at = EatSpace(at);
            if (MatchStrings(at, "reflect"))
            {
                at += sizeof("reflect") - 1;
                at = EatSpace(at);
                if (MatchStrings(at, "seq_enum"))
                {
                    at += sizeof("seq_enum") - 1;
                    info.hint = MetaEnumHint_Sequential;
                    at = EatSpace(at);
                }
                else if (MatchStrings(at, "flag_enum"))
                {
                    at += sizeof("flag_enum") - 1;
                    info.hint = MetaEnumHint_Flags;
                    at = EatSpace(at);
                }

                if (*at == ']')
                {
                    at++;

                    at = EatSpace(at);
                    info.name = at;
                    at = EatWord(at);
                    if (*at == '{')
                    {
                        *at = 0;
                        at++;
                    }
                    else
                    {
                        if (*at)
                        {
                            *at = 0;
                            at++;
                        }

                        at = EatSpace(at);
                        if (*at == ':')
                        {
                            at++;
                            at = EatSpace(at);
                            info.type = at;
                            at = EatUntilChar(at, '{');
                            char* term = EatSpaceBackwards(at - 1);
                            term++;
                            *term = 0;
                            at++;
                        }
                        else
                        {
                            at = EatUntilChar(at, '{');
                            at++;
                        }
                    }

                    at = EatSpace(at);

                    u32 memberIndex = 0;

                    while (*at)
                    {
                        if (*at == '}')
                        {
                            break;
                        }

                        EnumMember member;
                        member.index = memberIndex;
                        memberIndex++;
                        member.name = at;
                        at = EatWord(at);
                        info.members.push_back(member);
                        if (*at == ',')
                        {
                            *at = 0;
                            at = EatSpace(at + 1);
                        }
                        else if (*at == '}')
                        {
                            *at = 0;
                            at++;
                            break;
                        }
                        else
                        {
                            if (*at)
                            {
                                *at = 0;
                                at++;
                            }
                            char meet = EatUntilChar2(&at, '}', ',');
                            if (meet == '}')
                            {
                                break;
                            }
                            at++;
                        }

                        at = EatSpace(at);
                    }

                }
                else
                {
                    printf("Error: Invalid reflection attribute\n");
                }
            }
        }
        else
        {
            at = EatUntilChar(at, '}');
        }
    }

    tokenizer->at = at;
    return info;
}

constant const char* headerComment =
R"(// This file was generated by a meta preprocessor
// Generation time (UTC): %02d.%02d.%d %02d:%02d:%02d

)";

constant const char* enumForwardDeclNoType =
R"(enum %s %s;
)";

constant const char* enumForwardDeclTyped =
R"(enum %s %s : %s;
)";

constant const char* metaTable =
R"(struct MetaTable_%s
{
    constant u32 MemberCount = %d;
    constant b32 IsEnumClass = %d;
    constant char* Name = "%s";
    constant char* UnderlyingTypeName = "%s";
    constant MetaEnumHint MetaHint = %s;
    %s values[MemberCount];
    const char* names[MemberCount];

    constant u32 _HashMapSize = %d;
    %s _hashMapValues[_HashMapSize];
    u32 _hashMapIndices[_HashMapSize];
};

)";

constant const char* metaInfoSourceBegin =
R"(struct MetaInfo
{
)";

constant const char* metaInfoMember =
R"(    MetaTable_%s %s;
)";

constant const char* metaInfoEnd =
R"(};

)";

constant const char* initMetaInfoFnBegin =
R"(internal MetaInfo*
InitMetaInfo(AB::MemoryArena* arena)
{
    MetaInfo* info = PUSH_STRUCT(arena, MetaInfo);
    SOKO_ASSERT(info);
)";

constant const char* initAddEntry =
R"(    AddEntry_%s(&info->%s, %lu, %s, "%s");
)";

constant const char* initAddEntryClass =
R"(    AddEntry_%s(&info->%s, %lu, %s::%s, "%s::%s");
)";

constant const char* initMetaInfoFnEnd =
R"(
    return info;
})";

constant const char* addEntryFuncSeq =
R"(inline void
AddEntry_%s(MetaTable_%s* table, u32 enumeratorIndex, %s value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_%s::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_%s::_HashMapSize - 1;
    u32 hash = (u32)value & hashMask;

    for (u32 offset = 0; offset < MetaTable_%s::_HashMapSize; offset++)
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

)";

constant const char* getNameFuncSeq =
R"(inline const char*
GetEnumName(%s e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->%s;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_%s::_HashMapSize - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < MetaTable_%s::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_%s::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

)";

constant const char* addEntryFuncFlags =
R"(inline void
AddEntry_%s(MetaTable_%s* table, u32 enumeratorIndex, %s value, const char* name)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(MetaTable_%s::_HashMapSize));
    table->values[enumeratorIndex] = value;
    table->names[enumeratorIndex] = name;

    u32 hashMask = MetaTable_%s::_HashMapSize - 1;
    // TODO: Check this guy for performance
    auto bitScanResult = FindLeastSignificantBitSet((u32)value);
    u32 hash = bitScanResult.found ? (bitScanResult.index &  hashMask) : 0;

    for (u32 offset = 0; offset < MetaTable_%s::_HashMapSize; offset++)
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

)";

constant const char* getNameFuncFlags =
R"(inline const char*
GetEnumName(%s e)
{
    auto* table = &_GlobalStaticStorage->metaInfo->%s;
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = MetaTable_%s::_HashMapSize - 1;
    // TODO: Check this guy for performance
    auto bitScanResult = FindLeastSignificantBitSet((u32)e);
    u32 hash = bitScanResult.found ? (bitScanResult.index &  hashMask) : 0;

    for (u32 offset = 0; offset < MetaTable_%s::_HashMapSize; offset++)
    {
        u32 index = (hash + offset) & hashMask;
        if (table->_hashMapValues[index] == e)
        {
            u32 enumeratorIndex = table->_hashMapIndices[index];
            SOKO_ASSERT(enumeratorIndex < MetaTable_%s::MemberCount);
            result = table->names[enumeratorIndex];
            break;
        }
    }
    return result;
}

)";


// NOTE: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline int
NextPowerOfTwo(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

internal void
WriteMetaInfo(FILE* header, FILE* source, const char* headerName, const std::vector<EnumMetaInfo>& info)
{
    auto currentTime = time(0);
    auto timeinfo = gmtime(&currentTime);
    fprintf(header, headerComment, timeinfo->tm_mday,timeinfo->tm_mon + 1, 1900 + timeinfo->tm_year,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    fprintf(header, "#pragma once\n\n");

    fprintf(header, "namespace soko \n{\n");

    for (const EnumMetaInfo& entry : info)
    {
        if (entry.type)
        {
            fprintf(header, enumForwardDeclTyped, entry.isEnumClass ? "class" : "", entry.name, entry.type);
        }
        else
        {
            fprintf(header, enumForwardDeclNoType, entry.isEnumClass ? "class" : "", entry.name);
        }
    }

    fprintf(header, "}\n\nnamespace soko { namespace meta \n{\n\n");

    for (const EnumMetaInfo& entry : info)
    {
        const char* hint;
        switch (entry.hint)
        {
        case MetaEnumHint_NoHint: { hint = "MetaEnumHint_NoHint"; } break;
        case MetaEnumHint_Sequential: { hint = "MetaEnumHint_Sequential"; } break;
        case MetaEnumHint_Flags: { hint = "MetaEnumHint_Flags"; } break;
        default: { assert(false); }
        }

        fprintf(header, metaTable,
                entry.name,
                (int)entry.members.size(),
                (int)entry.isEnumClass,
                entry.name,
                entry.type ? entry.type : DEFAULT_ENUM_TYPE,
                hint,
                entry.name,
                NextPowerOfTwo((int)entry.members.size()),
                entry.name);
    }

    fprintf(header, metaInfoSourceBegin);

    for (const EnumMetaInfo& entry : info)
    {
        fprintf(header, metaInfoMember, entry.name, entry.name);
    }

    fprintf(header, metaInfoEnd);

    for (const EnumMetaInfo& entry : info)
    {
        if (entry.hint == MetaEnumHint_Flags)
        {
            fprintf(header, addEntryFuncFlags, entry.name, entry.name, entry.name,
                entry.name, entry.name, entry.name);
            fprintf(header, getNameFuncFlags, entry.name, entry.name, entry.name,
                entry.name, entry.name);

        }
        else
        {
            fprintf(header, addEntryFuncSeq, entry.name, entry.name, entry.name,
                entry.name, entry.name, entry.name);
            fprintf(header, getNameFuncSeq, entry.name, entry.name, entry.name,
                entry.name, entry.name);
        }
    }

    fprintf(header, "}}");

    fprintf(source, "#include \"%s\"\n\n", headerName);

    fprintf(source, "namespace soko { namespace meta {\n\n");

    fprintf(source, initMetaInfoFnBegin);

    for (const EnumMetaInfo& entry : info)
    {
        for (const EnumMember& member : entry.members)
        {
            if (entry.isEnumClass)
            {
                fprintf(source, initAddEntryClass, entry.name, entry.name, (unsigned long)member.index,
                        entry.name, member.name, entry.name, member.name);
            }
            else
            {
                fprintf(source, initAddEntry, entry.name, entry.name,
                        (unsigned long)member.index, member.name, member.name);
            }
        }
    }

    fprintf(source, initMetaInfoFnEnd);
    fprintf(source, "\n}}");
}

int main(int ac, char** av)
{
    int result = -1;
    if (ac > 3)
    {
        const char* headerInclude = av[1];
        if (headerInclude)
        {
            const char* outFilename = av[2];
            if (outFilename)
            {

                std::vector<EnumMetaInfo> info;

                for (int i = 3; i < ac; i++)
                {
                    const char* filename = av[i];
                    u32 size;
                    char* file = ReadEntireFileAsText(filename, &size);
                    if (file)
                    {
                        Tokenizer tokenizer = {};
                        tokenizer.text = file;
                        tokenizer.at = file;

                        while(true)
                        {
                            EatUntilEnum(&tokenizer);
                            if (tokenizer.eof)
                            {
                                break;
                            }

                            auto inf = TokenizeEnum(&tokenizer);
                            if (inf.name)
                            {
                                info.push_back(inf);
                            }
                            else
                            {
                                //printf("Error: Failed to parse enum declaration\n");
                            }
                        }
                    }
                    else
                    {
                        printf("Error: Failed to open file: %s\n", filename);
                        break;
                    }
                }
                char headerName[128];
                char sourceName[128];
                strcpy(headerName, outFilename);
                strcpy(sourceName, outFilename);
                strcat(headerName, ".h");
                strcat(sourceName, ".cpp");

                FILE* header = fopen(headerName, "w");
                FILE* source = fopen(sourceName, "w");

                if (header && source)
                {
                    WriteMetaInfo(header, source, headerInclude, info);
                    fclose(header);
                    fclose(source);
                }
                else
                {
                    printf("Error: Failed to open output file\n");
                }
            }
        }
    }
    else
    {
        printf("Error: Incorrect input\n");
    }
    return result;
}
