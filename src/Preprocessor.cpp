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

struct EnumMember
{
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
    bool isEnumClass;
    MetaEnumHint hint;
    std::vector<EnumMember> members;
};

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
                        at = EatUntilChar(at, '{');
                        at++;
                    }

                    at = EatSpace(at);

                    while (*at)
                    {
                        if (*at == '}')
                        {
                            break;
                        }

                        EnumMember member;
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

constant const char* metaTable =
R"(constant u32 ENTITY_META_TABLE_SIZE_%s = %d;

struct MetaTable_%s
{
    b32 isEnumClass;
    MetaEnumHint hint;
    i32 values[ENTITY_META_TABLE_SIZE_%s];
    const char* names[ENTITY_META_TABLE_SIZE_%s];
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

constant const char* metaTableInit =
R"(
    info->%s.isEnumClass = %d;
    info->%s.hint = %s;
)";

constant const char* initAddEntry =
R"(    AddEntry_%s(&info->%s, %s, "%s");
)";

constant const char* initAddEntryClass =
R"(    AddEntry_%s(&info->%s, %s::%s, "%s::%s");
)";

constant const char* initMetaInfoFnEnd =
R"(
    return info;
})";

constant const char* overloadedFunc =
R"(inline const char*
GetEnumName(MetaInfo* info, %s e)
{
    return GetEnumName_%s(&info->%s, e);
}

)";

constant const char* addEntryFunc =
R"(inline void
AddEntry_%s(MetaTable_%s* table, %s at, const char* string)
{
    SOKO_STATIC_ASSERT(IsPowerOfTwo(ENTITY_META_TABLE_SIZE_%s));
    u32 hashMask = ENTITY_META_TABLE_SIZE_%s - 1;
    u32 hash = (u32)at & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_%s; offset++)
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

)";

constant const char* getNameFunc =
R"(inline const char*
GetEnumName_%s(MetaTable_%s* table, %s e)
{
    const char* result = UNKNOWN_ENUM_VALUE;
    u32 hashMask = ENTITY_META_TABLE_SIZE_%s - 1;
    u32 hash = (u32)e & hashMask;

    for (u32 offset = 0; offset < ENTITY_META_TABLE_SIZE_%s; offset++)
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
WriteMetaInfo(FILE* file, const std::vector<EnumMetaInfo>& info)
{
    auto currentTime = time(0);
    auto timeinfo = gmtime(&currentTime);
    fprintf(file, headerComment, timeinfo->tm_mday,timeinfo->tm_mon + 1, 1900 + timeinfo->tm_year,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    for (const EnumMetaInfo& entry : info)
    {
        fprintf(file, metaTable, entry.name, NextPowerOfTwo((int)entry.members.size()),
                entry.name, entry.name, entry.name);
    }

    fprintf(file, metaInfoSourceBegin);

    for (const EnumMetaInfo& entry : info)
    {
        fprintf(file, metaInfoMember, entry.name, entry.name);
    }

    fprintf(file, metaInfoEnd);

    for (const EnumMetaInfo& entry : info)
    {
        fprintf(file, addEntryFunc, entry.name, entry.name, entry.name,
                entry.name, entry.name, entry.name);
        fprintf(file, getNameFunc, entry.name, entry.name, entry.name,
                entry.name, entry.name);
    }

    for (const EnumMetaInfo& entry : info)
    {
        fprintf(file, overloadedFunc, entry.name, entry.name, entry.name);
    }

    fprintf(file, initMetaInfoFnBegin);

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

        fprintf(file, metaTableInit, entry.name, (int)entry.isEnumClass,
                entry.name, hint);

        for (const EnumMember& member : entry.members)
        {
            if (entry.isEnumClass)
            {
                fprintf(file, initAddEntryClass, entry.name, entry.name, entry.name,
                        member.name, entry.name, member.name);
            }
            else
            {
                fprintf(file, initAddEntry, entry.name, entry.name, member.name, member.name);
            }
        }
    }

    fprintf(file, initMetaInfoFnEnd);
}

int main(int ac, char** av)
{
    int result = -1;
    if (ac > 2)
    {
        const char* outFilename = av[1];
        if (!outFilename)
        {
            outFilename = "Generated.cpp";
        }

        std::vector<EnumMetaInfo> info;

        for (int i = 2; i < ac; i++)
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
        FILE* out = fopen(outFilename, "w");
        if (out)
        {
            WriteMetaInfo(out, info);
            fclose(out);
        }
        else
        {
            printf("Error: Failed to open output file\n");
        }
    }
    else
    {
        printf("Error: Incorrect input\n");
    }
    return result;
}
