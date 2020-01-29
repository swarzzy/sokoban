#define AB_PLATFORM_WINDOWS
#include "Platform.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

using namespace AB;

struct Tokenizer
{
    char* text;
    char* at;
    b32 eof;

    Tokenizer Clone()
    {
        Tokenizer result = {};
        result.eof = this->eof;
        auto length = StrLength(this->text) + 1;
        result.text = (char*)malloc(length);
        auto offset = this->text - this->at;
        result.at = result.text + offset;
        memcpy(result.text, this->text, length);
        return result;
    }
};

inline bool
Match(const char* str1, const char* str2)
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
Match(const char* str, char c)
{
    bool result = (*str == c);
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
        if (Match(at, "//"))
        {
            at = EatLine(at);
        }
        else if (Match(at, "/*"))
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
        if (Match(at, "//"))
        {
            at = EatLine(at);
        }
        else if (Match(at, "/*"))
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

inline char* EatLine(char* at)
{
    char* result = 0;
    while (*at)
    {
        // NOTE: Supporting only LF and CRLF
        if (*at == '\n')
        {
            result = EatSpace(at);
            return result;
        }
        at++;
    }
    result = at;
    return result;
}

inline char* EatMultiLineComment(char* at)
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

inline u32
SafeTruncateU64U32(u64 val)
{
    assert(val <= 0xffffffff);
    return (u32)val;
}

std::string GetDirectory(const std::string& path)
{
#if defined(AB_PLATFORM_WINDOWS)
    const char* separators = "\\/";
#elif defined(AB_PLATFORM_LINUX)
    const char* separators = "/";
#endif

    auto index = path.find_last_of(separators);
    if (index == std::string::npos)
    {
        return std::string("");
    }
    else
    {
        return path.substr(0, index + 1);
    }
}

internal bool
GetDirectory(const char* file_path, char* buffer, u32 buffer_size, u32* written)
{
#if defined(AB_PLATFORM_WINDOWS)
    char sep_1 = '\\';
    char sep_2 = '/';
#elif defined(AB_PLATFORM_LINUX)
    char sep_1 = '/';
    char sep_2 = '/';

#else
#error Unsupported OS
#endif
    u32 last_sep_index = 0;
    char used_separator = '/';

    bool succeeded = false;

    u32 i = 0;
    char at = file_path[i];
    while (at != '\0')
    {
        if (at == sep_1 || at == sep_2)
        {
            last_sep_index = i;
            used_separator = at;
        }
        i++;
        at = file_path[i];
    }

    u32 dir_num_chars = last_sep_index + 2;

    if (buffer_size >= dir_num_chars)
    {
        memcpy(buffer, file_path, dir_num_chars - 2);
        buffer[dir_num_chars - 2] = used_separator;
        buffer[dir_num_chars - 1] = '\0';
        succeeded = true;
    }
    else
    {
        succeeded = false;
    }
    if (succeeded)
    {
        *written = dir_num_chars;
    }
    return succeeded;
}

#if defined(AB_PLATFORM_WINDOWS)
#include <windows.h>

void FreeFileMemory(void* memory)
{
    if (memory) {
        free(memory);
    }
}

internal void*
ReadEntireFile(const char* filename, u32* read)
{
    u32 bytesRead = 0;
    void* bitmap = nullptr;
    LARGE_INTEGER fileSize = { 0 };
    HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            if (fileSize.QuadPart > 0xffffffff)
            {
                printf("Can`t read >4GB file.");
                CloseHandle(fileHandle);
                *read = 0;
                return nullptr;
            }
            bitmap = malloc(fileSize.QuadPart);
            if (bitmap)
            {
                DWORD read;
                if (!ReadFile(fileHandle, bitmap, (DWORD)fileSize.QuadPart, &read, 0) && !(read == (DWORD)fileSize.QuadPart))
                {
                    printf("Failed to read file.");
                    FreeFileMemory(bitmap);
                    bitmap = nullptr;
                }
                else
                {
                    bytesRead = (u32)fileSize.QuadPart;
                }
            }
        }
        CloseHandle(fileHandle);
    }
    *read = bytesRead;
    return  bitmap;
}

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

internal char*
ReadEntireFileAsTextCRT(const char* filename, u32* bytesRead)
{
    char* result = 0;
    *bytesRead = 0;
    FILE* file = fopen(filename, "r");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        auto size = ftell(file);

        fseek(file, 0, SEEK_SET);

        char* buffer = (char*)malloc(sizeof(char) * size + 1);
        if (buffer)
        {
            auto read = fread(buffer, sizeof(char), size, file);
            if (read == size)
            {
                // NOTE: Success
                result = buffer;
                buffer[size] = 0;
                *bytesRead = size;
            }
            else
            {
                free(buffer);
            }
        }
        fclose(file);
    }
    return result;
}

internal b32
WriteFile(const char* filename, void* data, u32 dataSize)
{
    HANDLE fileHandle = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, data, dataSize, &bytesWritten, 0) && (dataSize == bytesWritten))
        {
            CloseHandle(fileHandle);
            return true;
        }
    }
    CloseHandle(fileHandle);
    return false;
}
#endif
