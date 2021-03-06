#pragma once
#include <stdint.h>
#include <float.h>
// NOTE: offsetof
#include <stddef.h>

#if defined(_MSC_VER)
#define AB_COMPILER_MSVC
#elif defined(__clang__)
#define AB_COMPILER_CLANG
#else
#error Unsupported compiler
#endif

#if defined(AB_PLATFORM_WINDOWS)
#define GAME_CODE_ENTRY __declspec(dllexport)
#elif defined(AB_PLATFORM_LINUX)
#define GAME_CODE_ENTRY
#else
#error Unsupported OS
#endif

#if defined(AB_PLATFORM_WINDOWS)
#define AB_DEBUG_BREAK() __debugbreak()
#elif defined(AB_PLATFORM_LINUX)
#define AB_DEBUG_BREAK() __builtin_debugtrap()
#endif


#define constant static constexpr

#define internal static
#define local_persist static
#define global_variable static

#define ArrayCount(arr) (sizeof(arr) / sizeof(arr[0]))
#define DeclMember(type, member) (((type*)0)->member)
#define typedecl(type, member) DeclMember(type, member)

#define foreach(collection) for(auto& it : collection)
#define CASE(expr, ...) case expr: {__VA_ARGS__;} break
#define INVALID_DEFAULT() default: { AB_DEBUG_BREAK(); } break;

#define SOKO_STATIC_ASSERT static_assert

// NOTE: Defer implementation. Reference: https://pastebin.com/SX3mSC9n
#define CONCAT_INTERNAL(x,y) x##y
#define CONCAT(x,y) CONCAT_INTERNAL(x,y)

template<typename T>
struct ExitScope
{
    T lambda;
    ExitScope(T lambda) : lambda(lambda) {}
    ~ExitScope() { lambda(); }
    ExitScope(const ExitScope&);
  private:
    ExitScope& operator =(const ExitScope&);
};

class ExitScopeHelp
{
  public:
    template<typename T>
    ExitScope<T> operator+(T t) { return t; }
};

#define defer const auto& CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()

#define _padby(count) byte CONCAT(_pad, __LINE__)[count]

// NOTE: [reflect] - no hint (using seq_enum by default)
// NOTE: [reflect seq_enum] - hint for sequential enums (using more suitable hash function)
// NOTE: [reflect flag_enum] - hint for bitfiels flags enums
#define reflect []
#define seq_enum
#define flag_enum

struct ImGuiContext;

namespace AB
{
    typedef uint8_t             byte;
    typedef unsigned char       uchar;

    typedef int8_t              i8;
    typedef int16_t             i16;
    typedef int32_t             i32;
    typedef int64_t             i64;

    typedef uint8_t             u8;
    typedef uint16_t            u16;
    typedef uint32_t            u32;
    typedef uint64_t            u64;

    typedef uintptr_t           uptr;

    typedef u32                 b32;
    typedef byte                b8;

    typedef float               f32;
    typedef double              f64;

    typedef u32 u32x;
    typedef i32 i32x;
/*
    // TODO: Error here
    const u32 UINT32_MAX = 0xffffffff;
    const i32 INT32_MIN = -(i32)(2147483648);
    const i32 INT32_MAX = (i32)(0x7fffffff);
    const f32 FLOAT_MAX = FLT_MAX;
*/
    constant uptr UPTR_MAX = UINTPTR_MAX;
    constant u32 U32_MAX = 0xffffffff;
    constant f32 F32_MAX = FLT_MAX;

    const u32 MOUSE_BUTTONS_COUNT = 5;

    enum MouseButton : u8
    {
        MBUTTON_LEFT = 0, MBUTTON_RIGHT,
        MBUTTON_MIDDLE, MBUTTON_XBUTTON1, MBUTTON_XBUTTON2
    };

    const u32 KEYBOARD_KEYS_COUNT = 256;

    enum [reflect seq_enum] KeyCode : u8
    {
        KEY_INVALIDKEY = 0x00,
        // Currently works only Ctrl for both left and right keys
        // Right Ctrl and Super key doesn`t work on Linux.
        KEY_CTRL, KEY_SPACE, KEY_APOSTROPHE, KEY_COMMA,
        KEY_MINUS, KEY_PERIOD, KEY_SLASH, KEY_0 = 0x30,
        KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8,
        KEY_9, KEY_SEMICOLON, KEY_EQUAL, KEY_A = 0x41,
        KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H,
        KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,
        KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V,
        KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_LEFT_BRACKET,
        KEY_BACK_SLASH, KEY_RIGHT_BRACKET, KEY_TILDE,
        KEY_ESCAPE, KEY_ENTER, KEY_TAB, KEY_BACKSPACE,
        KEY_INSERT, KEY_DELETE, KEY_RIGHT, KEY_LEFT,
        KEY_DOWN, KEY_UP, KEY_PAGE_UP, KEY_PAGE_DOWN,
        KEY_HOME, KEY_END, KEY_CAPS_LOCK, KEY_SCROLL_LOCK,
        KEY_NUM_LOCK, KEY_PRINT_SCREEN, KEY_PAUSE,
        KEY_RETURN = KEY_ENTER, KEY_F1 = 114,
        KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
        KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11,
        KEY_F12, KEY_F13, KEY_F14, KEY_F15, KEY_F16,
        KEY_F17, KEY_F18, KEY_F19, KEY_F20, KEY_F21,
        KEY_F22, KEY_F23, KEY_F24, KEY_NUM0, KEY_NUM1,
        KEY_NUM2, KEY_NUM3, KEY_NUM4, KEY_NUM5, KEY_NUM6,
        KEY_NUM7, KEY_NUM8, KEY_NUM9, KEY_NUM_DECIMAL,
        KEY_NUM_DIVIDE, KEY_NUM_MULTIPLY, KEY_NUM_SUBTRACT,
        KEY_NUM_ADD, KEY_NUM_ENTER = KEY_ENTER,
        KEY_LEFT_SHIFT = 153, KEY_LEFT_CTRL, KEY_ALT,
        KEY_LEFT_SUPER, KEY_MENU, KEY_RIGHT_SHIFT,
        KEY_RIGHT_CTRL, KEY_RIGHT_SUPER, KEY_CLEAR,
        KEY_SHIFT
    };

    enum LogLevel
    {
        LOG_FATAL = 0,
        LOG_ERROR,
        LOG_WARN,
        LOG_INFO,
    };
}

#include "OpenGL.h"

namespace AB
{
    const i32 RAW_TIME_SECOND = 1000000;

    constexpr u32 StrLength(const char* string)
    {
        u32 result = 0;
        while (*string)
        {
            result++;
            string++;
        }
        return result;
    }

    constexpr u32 StrSize(const char* string)
    {
        u32 result = 0;
        while (*string)
        {
            result++;
            string++;
        }
        result++;
        result *= sizeof(char);
        return result;
    }

    constexpr u32 StrSize(const wchar_t* string)
    {
        u32 result = 0;
        while (*string)
        {
            result++;
            string++;
        }
        result++;
        result *= sizeof(wchar_t);
        return result;
    }

    inline u32 SafeCastU64U32(u64 val)
    {
        if (val > 0xffffffff)
        {
            AB_DEBUG_BREAK();
            return 0;
        }
        else
        {
            return (u32)val;
        }
    }

    inline i32 SafeCastIntI32(int val)
    {
        if (sizeof(int) <= sizeof(i32))
        {
            return (i32)val;
        }
        else if (val <= 0xffffffff)
        {
            return (i32)val;
        }
        else
        {
            AB_DEBUG_BREAK();
            return 0;
        }
    }

    inline u32 SafeCastUintU32(unsigned int val)
    {
        if (sizeof(unsigned int) <= sizeof(u32))
        {
            return (u32)val;
        }
        else if (val <= 0xffffffff)
        {
            return (u32)val;
        }
        else
        {
            AB_DEBUG_BREAK();
            return 0;
        }
    }

    inline int SafeCastI32Int(i32 val)
    {
        if (sizeof(int) >= sizeof(u32))
        {
            return (int)val;
        }
        // TODO: Not aborting if int actually can hold the value
        AB_DEBUG_BREAK();
        return 0;
    }

    inline u32 SafeCastUptrU32(uptr uptr)
    {
        if (uptr <= 0xffffffff)
        {
            return (u32)uptr;
        }
        else
        {
            AB_DEBUG_BREAK();
            return 0;
        }
    }

    inline i32 SafeCastU32I32(u32 val)
    {
        if (val <= 0x7fffffff)
        {
            return (i32)val;
        }
        else
        {
            AB_DEBUG_BREAK();
            return 0;
        }
    }

    struct MemoryArena;

    struct DateTime
    {
        u16 year;
        u16 month;
        u16 dayOfWeek;
        u16 day;
        u16 hour;
        u16 minute;
        u16 seconds;
        u16 milliseconds;
    };

    const u16 DATETIME_STRING_SIZE = 9; // hh:mm:ss\0

    struct DirectoryContents
    {
        b32 scannedSuccesfully;
        u32 count;
        wchar_t** filenames;
    };

    // NOTE: On unix API this should be defined as int
    typedef uptr FileHandle;
    constant FileHandle INVALID_FILE_HANDLE = UPTR_MAX;

    // TODO: Pass some notion is unicode paths allowed to game
    typedef u32(DebugGetFileSizeFn)(const wchar_t* filename);
    typedef u32(DebugReadFileFn)(void* buffer, u32 bufferSize, const wchar_t* filename);
    typedef u32(DebugReadTextFileFn)(void* buffer, u32 bufferSize, const wchar_t* filename);
    typedef bool(DebugWriteFileFn)(const wchar_t* filename, void* data, u32 dataSize);

    typedef FileHandle(DebugOpenFileFn)(const wchar_t* filename);
    typedef bool(DebugCloseFileFn)(FileHandle handle);
    typedef u32(DebugWriteToOpenedFileFn)(FileHandle handle, void* data, u32 size);

    typedef i32(FormatStringFn)(char* buffer, u32 bufferSize, const char* fmt, ...);
    typedef void(PrintStringFn)(const char* fmt, ...);
    typedef void(LogFn)(LogLevel level, const char* file, const char* func, u32 line, const char* fmt, ...);
    typedef void(LogAssertVFn)(LogLevel level, const char* file, const char* func, u32 line, const char* assertStr, const char* fmt, va_list* args);

    typedef MemoryArena*(QueryNewArenaFn)(uptr size, bool isTemp);
    typedef void(FreeArenaFn)(MemoryArena* arena);

    typedef f64(GetTimeStampFn)();

    typedef DirectoryContents(EnumerateFilesInDirectoryFn)(const wchar_t* dirName, MemoryArena* tempArena);

    typedef void*(AllocForImGuiFn)(size_t sz, void* data);
    typedef void(FreeForImGuiFn)(void* ptr, void* data);

    enum InputMode
    {
        INPUT_MODE_FREE_CURSOR = 0,
        INPUT_MODE_CAPTURE_CURSOR
    };

    typedef void(SetInputModeFn)(InputMode);

    enum GameUpdateAndRenderReason
    {
        GUR_REASON_INIT, GUR_REASON_RELOAD, GUR_REASON_UPDATE, GUR_REASON_RENDER
    };

    struct PlatformFuncTable
    {
        DebugGetFileSizeFn* DebugGetFileSize;
        DebugReadFileFn* DebugReadFile;
        DebugReadTextFileFn* DebugReadTextFile;
        DebugWriteFileFn* DebugWriteFile;
        DebugOpenFileFn* DebugOpenFile;
        DebugCloseFileFn* DebugCloseFile;
        DebugWriteToOpenedFileFn* DebugWriteToOpenedFile;
        FormatStringFn* FormatString;
        PrintStringFn* PrintString;
        LogFn* Log;
        LogAssertVFn* LogAssertV;
        SetInputModeFn* SetInputMode;

        QueryNewArenaFn* QueryNewArena;
        FreeArenaFn* FreeArena;

        GetTimeStampFn* GetTimeStamp;

        EnumerateFilesInDirectoryFn* EnumerateFilesInDirectory;

        AllocForImGuiFn* AllocForImGui;
        FreeForImGuiFn* FreeForImGui;
    };

    struct KeyState
    {
        // TODO: Shpuld they be b32?
        b32 pressedNow;
        b32 wasPressed;
    };

    struct MButtonState
    {
        b32 pressedNow;
        b32 wasPressed;
    };

    const u32 PLATFORM_TEXT_INPUT_BUFFER_SIZE = 64;

    struct InputState
    {
        u32 textBufferCount;
        char textBuffer[PLATFORM_TEXT_INPUT_BUFFER_SIZE];
        KeyState keys[KEYBOARD_KEYS_COUNT];
        MButtonState mouseButtons[MOUSE_BUTTONS_COUNT];
        b32 mouseInWindow;
        b32 activeApp;
        // NOTE: All mouse position values are normalized
        f32 mouseX;
        f32 mouseY;
        f32 mouseFrameOffsetX;
        f32 mouseFrameOffsetY;
        // NOTE: Not normalized
        i32 scrollOffset;
        //i32 prevFrameScrollOffset;
        i32 scrollFrameOffset;
    };

    struct PlatformState
    {
        PlatformFuncTable functions;
        soko::OpenGL* gl;
        ImGuiContext* imGuiContext;
        void* imGuiAllocatorData;
        InputState input;
        i32 fps;
        i32 ups;
        f32 gameSpeed;
        f32 absDeltaTime;
        f32 gameDeltaTime;
        u32 windowWidth;
        u32 windowHeight;
        DateTime localTime;
    };

    enum ConsoleColor
    {
        CONSOLE_COLOR_BLACK = 0, CONSOLE_COLOR_DARKBLUE, CONSOLE_COLOR_DARKGREEN,
        CONSOLE_COLOR_DARKCYAN, CONSOLE_COLOR_DARKRED, CONSOLE_COLOR_DARKPURPLE,
        CONSOLE_COLOR_DARKYELLOW, CONSOLE_COLOR_DARKWHITE, CONSOLE_COLOR_GRAY,
        CONSOLE_COLOR_BLUE, CONSOLE_COLOR_GREEN, CONSOLE_COLOR_CYAN,
        CONSOLE_COLOR_RED, CONSOLE_COLOR_PURPLE, CONSOLE_COLOR_YELLOW,
        CONSOLE_COLOR_WHITE, CONSOLE_COLOR_DEFAULT
    };

    b32 ConsolePrint(const char* string);
    b32 ConsolePrint(const void* data, u32 count);
    b32 ConsoleSetColor(ConsoleColor textColor, ConsoleColor backColor);

    void GetExecutablePath(char* buffer, u32 bufferSizeBytes, u32* bytesWritten);

    // Returns microseconds
    i64 GetCurrentRawTime();

    DateTime GetLocalTime();

    u32 DebugReadFileToBuffer(void* buffer, u32 bufferSize, const char* filename);
    u32 DebugReadTextFileToBuffer(void* buffer, u32 bufferSize, const char* filename);

    u32 DebugGetFileSize(const char* filename);
    b32 DebugWriteFile(const char* filename,  void* data, u32 dataSize);

}

// NOTE: For platform use only
#if defined(AB_PLATFORM_CODE)
#include "PlatformLog.h"

#define AB_STATIC_ASSERT(expr) static_assert((expr))
#if defined(AB_CONFIG_DISTRIB)

#define AB_CORE_INFO(format, ...)   do{}while(false)
#define AB_CORE_WARN(format, ...)   do{}while(false)
#define AB_CORE_ERROR(format, ...)  do{}while(false)
#define AB_CORE_FATAL(format, ...)  do{}while(false)
#define AB_CORE_ASSERT(format, expr, ...)   do{}while(false)

#else

#if defined (AB_COMPILER_CLANG)
#define AB_CORE_INFO(format, ...) AB::Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define AB_CORE_WARN(format, ...) AB::Log(AB::LOG_WARN, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define AB_CORE_ERROR(format, ...) AB::Log(AB::LOG_ERROR, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define AB_CORE_FATAL(format, ...) do { AB::Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__); AB_DEBUG_BREAK();} while(false)
#define AB_CORE_ASSERT(expr, ...) do { if (!(expr)) {AB::LogAssert(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, ##__VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#else
#define AB_CORE_INFO(format, ...) AB::Log(AB::LOG_INFO, __FILE__, __func__, __LINE__, format, __VA_ARGS__)
#define AB_CORE_WARN(format, ...) AB::Log(AB::LOG_WARN, __FILE__, __func__, __LINE__, format, __VA_ARGS__)
#define AB_CORE_ERROR(format, ...) AB::Log(AB::LOG_ERROR, __FILE__, __func__, __LINE__, format, __VA_ARGS__)
#define AB_CORE_FATAL(format, ...) do { AB::Log(AB::LOG_FATAL, __FILE__, __func__, __LINE__, format, __VA_ARGS__); AB_DEBUG_BREAK();} while(false)
#define AB_CORE_ASSERT(expr, ...) do { if (!(expr)) {AB::LogAssert(AB::LOG_FATAL, __FILE__, __func__, __LINE__, #expr, __VA_ARGS__); AB_DEBUG_BREAK();}} while(false)
#endif
#endif
#define AB_INVALID_DEFAULT_CASE AB_CORE_ASSERT(false, "Invalid default case.")
#endif
