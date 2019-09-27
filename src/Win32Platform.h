#pragma once
#include "Platform.h"
#include "Memory.h"
#include <Windows.h>
#include "Win32CodeLoader.h"

// NOTE: For lastMouseCursor in Application
#include "imgui/imgui.h"

extern "C"
{
    typedef const char*(APIENTRY wglGetExtensionsStringARBFn)(HDC);
    typedef BOOL (APIENTRY wglChoosePixelFormatARBFn)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
    typedef HGLRC (APIENTRY wglCreateContextAttribsARBFn)(HDC, HGLRC, const int*);
    typedef BOOL (APIENTRY wglSwapIntervalEXTFn)(int interval);
    typedef int (APIENTRY wglGetSwapIntervalEXTFn)(void);
}

namespace AB
{
    struct GameCode;

//#define AB_ENABLE_CONSOLE_WINDOW

    const WORD WINSOCK_VER = 0x202;

    const TCHAR* WINDOW_CLASS_NAME = TEXT("Aberration Engine Win32");

    const u32 OPENGL_MAJOR_VERSION = 4;
    const u32 OPENGL_MINOR_VERSION = 5;

    const u32 WINDOW_TITLE_SIZE = 32;

    const uptr MAIN_ARENA_SIZE = MEGABYTES(2048);
    const uptr GAME_ARENA_SIZE = MEGABYTES(2000);

    const ConsoleColor CONSOLE_DEFAULT_TEXT_COLOR = CONSOLE_COLOR_DARKWHITE;
    const ConsoleColor CONSOLE_DEFAULT_BACK_COLOR = CONSOLE_COLOR_BLACK;

    const DWORD IMGUI_HEAP_FLAGS = HEAP_GENERATE_EXCEPTIONS;


    struct Application
    {
        MemoryArena* mainArena;
        MemoryArena* gameArena;
        void* gameStaticStorage;
        PlatformState state;

        TCHAR windowTitle[32];
        b32 running;
        b32 fullscreen;
        HWND win32WindowHandle;
        HDC win32WindowDC;
        HGLRC OpenGLRC;
        WINDOWPLACEMENT wpPrev;

        i64 runningTime;

        TRACKMOUSEEVENT Win32MouseTrackEvent;

        wglGetExtensionsStringARBFn* wglGetExtensionsStringARB;
        wglChoosePixelFormatARBFn* wglChoosePixelFormatARB;
        wglCreateContextAttribsARBFn* wglCreateContextAttribsARB;
        wglSwapIntervalEXTFn* wglSwapIntervalEXT;
        wglGetSwapIntervalEXTFn* wglGetSwapIntervalEXT;
        u8 keyTable[KEYBOARD_KEYS_COUNT];

        // TODO: @Important: Get rid of max path constant
        LibraryData gameLib;

        LARGE_INTEGER performanceFrequency;

        InputMode inputMode;

        ImGuiMouseCursor imguiLastMouseCursor;

        HANDLE imGuiHeap;
    };

    static const i64 UPDATE_INTERVAL = 16000;
    static const i64 SECOND_INTERVAL = 1000000;

    MemoryArena* AllocateArena(uptr size);
    Application* AppCreate(MemoryArena* sysMemory);
    void AppRun(Application* app);

    struct MemoryArena;

    // @TODO: @Cleanup
    void WindowPollEvents(Application* app);
    void WindowToggleFullscreen(Application* app, bool enable);
    void WindowShowCursor(Application* app, b32 show);
    void WindowSetMousePosition(Application* app, u32 x, u32 y);

    static LRESULT CALLBACK Win32WindowCallback(HWND windowHandle, UINT message,
                                                WPARAM wParam, LPARAM lParam);
    static void Win32InitKeyTable(u8* keytable);
    static u8 Win32KeyConvertToABKeycode(Application* app, u64 Win32Key);
    static void Win32Initialize(Application* app);
    unsigned int WGLLoadFunctions(Application* app, HDC windowDC);
};
