#include "Win32Platform.h"

#include "Memory.h"
#include "OpenGL.h"

#include <tchar.h>

#include <timeapi.h>

#include <cstdlib>

#include "imgui/imgui_impl_soko_win32.h"
#include "imgui/imgui_impl_opengl3.h"

#if defined (AB_DISCRETE_GRAPHICS_DEFAULT)
extern "C" { __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }
extern "C" { __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x01; }
#endif

// NOTE: For now assume that all windows devices are little-endian
#define AB_BYTE_ORDER AB_LITTLE_ENDIAN

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_ACCELERATION_ARB              0x2003
#define WGL_FULL_ACCELERATION_ARB         0x2027
#define WGL_NO_ACCELERATION_ARB           0x2025

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB       0x2093
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x0001

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

// FORWARD DECLARATIONS

// Macros from windowsx.h
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define AB_KEY_REPEAT_COUNT_FROM_LPARAM(lParam) ((u16)(lParam & 0xffff >> 2))

namespace AB
{

    void OpenglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam)
    {
        const char* sourceStr;
        const char* typeStr;
        const char* severityStr;

        switch (source)
        {
        case GL_DEBUG_SOURCE_API: { sourceStr = "API"; } break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: { sourceStr = "window system"; } break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: { sourceStr = "shader compiler"; } break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: { sourceStr = "third party"; } break;
        case GL_DEBUG_SOURCE_APPLICATION: { sourceStr = "application"; } break;
        case GL_DEBUG_SOURCE_OTHER: { sourceStr = "other"; } break;
            INVALID_DEFAULT();
        }

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR: { typeStr = "error"; } break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: { typeStr = "deprecated behavior"; } break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: { typeStr = "undefined behavior"; } break;
        case GL_DEBUG_TYPE_PORTABILITY: { typeStr = "portability problem"; } break;
        case GL_DEBUG_TYPE_PERFORMANCE: { typeStr = "performance problem"; } break;
        case GL_DEBUG_TYPE_OTHER: { typeStr = "other"; } break;
            INVALID_DEFAULT();
        }

        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH: { severityStr = "high"; } break;
        case GL_DEBUG_SEVERITY_MEDIUM: { severityStr = "medium"; } break;
        case GL_DEBUG_SEVERITY_LOW: { severityStr = "low"; } break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: { severityStr = "notification"; } break;
        default: { severityStr = "unknown"; } break;
        }
        PrintString("OpenGL debug message (source: %s, type: %s, severity: %s): %s\n", sourceStr, typeStr, severityStr, message);
        //AB_DEBUG_BREAK();
    }
    using soko::OpenGL;
    struct OpenGLLoadResult
    {
        OpenGL* context;
        b32 success;
    };

    void* OpenGLGetProcAddress(const char* name)
    {
        auto result = wglGetProcAddress(name);
        if (result == 0 ||
            result == (void*)0x1 ||
            result == (void*)0x2 ||
            result == (void*)0x3 ||
            result == (void*)-1)
        {
            // NOTE: Failed
            result = 0;
        }
        return (void*)result;
    }

    OpenGLLoadResult LoadOpenGL(MemoryArena* memoryArena)
    {
        OpenGL* context = (OpenGL*)PushSize(memoryArena,sizeof(OpenGL), alignof(OpenGL));
        AB_CORE_ASSERT(context);

        PrintString("[Info] Loading OpenGL functions...\n");
        PrintString("[Info] Functions defined: %i32\n", OpenGL::FunctionCount);
        PrintString("[Info] Function names described: %i32\n", ArrayCount(OpenGL::FunctionNames));

        b32 success = true;
        HMODULE glLibHandle = {};
        for (u32 i = 0; i < OpenGL::FunctionCount; i++)
        {
            PrintString("[Info] Loading %s...\n", OpenGL::FunctionNames[i]);
            context->functions.raw[i] = OpenGLGetProcAddress(OpenGL::FunctionNames[i]);
            if (!context->functions.raw[i])
            {
                if (!glLibHandle)
                {
                    glLibHandle = LoadLibrary(TEXT("opengl32.dll"));
                }
                if (glLibHandle)
                {
                    context->functions.raw[i] = (void*)GetProcAddress(glLibHandle, OpenGL::FunctionNames[i]);
                }
                else
                {
                    context->functions.raw[i] = 0;
                    AB_CORE_ERROR("[Error]: Failed to load OpenGL procedure: %s\n", OpenGL::FunctionNames[i]);
                    success = false;
                }
            }
        }

        // NOTE: Querying extensions
        PrintString("\n[Info] Loading OpenGL extensions...\n");
        GLint numExtensions;
        context->functions.fn.glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (i32x i = 0; i < numExtensions; i++)
        {
            const GLubyte* extensionString;
            extensionString = context->functions.fn.glGetStringi(GL_EXTENSIONS, i);
            if (strcmp((const char*)extensionString, "GL_EXT_texture_filter_anisotropic") == 0)
            {
                context->extensions.EXT_texture_filter_anisotropic = true;
            }
            if (strcmp((const char*)extensionString, "GL_ARB_texture_filter_anisotropic") == 0)
            {
                context->extensions.ARB_texture_filter_anisotropic = true;
            }
            if (strcmp((const char*)extensionString, "GL_ARB_gl_spirv") == 0)
            {
                context->extensions.ARB_gl_spirv.glSpecializeShaderARB = (PFNGLSPECIALIZESHADERARBPROC)OpenGLGetProcAddress("glSpecializeShaderARB");
                if (context->extensions.ARB_gl_spirv.glSpecializeShaderARB)
                {
                    context->extensions.ARB_gl_spirv.supported = true;
                }
            }
            if (strcmp((const char*)extensionString, "GL_ARB_spirv_extensions") == 0)
            {
                context->extensions.ARB_spirv_extensions = true;
            }
        }

        if (!context->extensions.ARB_texture_filter_anisotropic && !context->extensions.EXT_texture_filter_anisotropic)
        {
            PrintString("[Info] GL_texture_filter_anisotropic is not supported\n");
        }
        if (!context->extensions.ARB_gl_spirv.supported)
        {
            PrintString("[Info] ARB_gl_spirv is not supported\n");
        }
        if (!context->extensions.ARB_spirv_extensions)
        {
            PrintString("[Info] ARB_spirv_extensions is not supported\n");
        }

        if (success)
        {
            // TODO: Do this in renderer
            u32 globalVAO;
            context->functions.fn.glGenVertexArrays(1, &globalVAO);
            context->functions.fn.glBindVertexArray(globalVAO);
            context->functions.fn.glEnable(GL_DEPTH_TEST);
            context->functions.fn.glDepthFunc(GL_LESS);
            context->functions.fn.glEnable(GL_CULL_FACE);
            context->functions.fn.glCullFace(GL_BACK);
            context->functions.fn.glFrontFace(GL_CCW);
            context->functions.fn.glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

            //context->functions._glEnable(GL_BLEND);
            //context->functions._glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            //context->functions._glBlendEquation(GL_FUNC_ADD);

#if defined(AB_DEBUG_OPENGL)
            // TODO: Set debug callback from game
            context->functions.fn.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            context->functions.fn.glDebugMessageCallback(OpenglDebugCallback, 0);
            context->functions.fn.glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
            context->functions.fn.glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
            context->functions.fn.glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_LOW, 0, 0, GL_FALSE);
#endif
        }

        return {context, success};
    }

    // NOTE: Global variables
    static LARGE_INTEGER GlobalPerformanceFrequency = {};

    inline static void
    ProcessMButtonEvent(InputState* input, MouseButton button, b32 state)
    {
        input->mouseButtons[button].wasPressed =
            input->mouseButtons[button].pressedNow;
        input->mouseButtons[button].pressedNow = state;
    }

    inline static void
    ProcessMButtonDownAndDblclkEventImGui(Application* application, ImGuiIO* io, UINT msg, WPARAM wParam)
    {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
        if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(application->win32WindowHandle);
        io->MouseDown[button] = true;
    }

    inline static void
    ProcessMButtonUpEventImGui(Application* application, ImGuiIO* io, UINT msg, WPARAM wParam)
    {
        int button = 0;
        if (msg == WM_LBUTTONUP) { button = 0; }
        if (msg == WM_RBUTTONUP) { button = 1; }
        if (msg == WM_MBUTTONUP) { button = 2; }
        if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        io->MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == application->win32WindowHandle)
            ::ReleaseCapture();
    }


    // NOTE: Based on Raymond Chen example
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    void WindowToggleFullscreen(Application* app, bool enable)
    {
        DWORD style = GetWindowLong(app->win32WindowHandle, GWL_STYLE);
        if ((style & WS_OVERLAPPEDWINDOW) && enable)
        {
            app->fullscreen = true;
            MONITORINFO mInfo = { sizeof(MONITORINFO) };
            if (GetWindowPlacement(app->win32WindowHandle, &app->wpPrev) &&
                GetMonitorInfo(MonitorFromWindow(app->win32WindowHandle,
                                                 MONITOR_DEFAULTTOPRIMARY), &mInfo))
            {
                SetWindowLong(app->win32WindowHandle, GWL_STYLE,
                              style & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(app->win32WindowHandle, HWND_TOP,
                             mInfo.rcMonitor.left, mInfo.rcMonitor.top,
                             mInfo.rcMonitor.right - mInfo.rcMonitor.left,
                             mInfo.rcMonitor.bottom - mInfo.rcMonitor.top,
                             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            }
        }
        else if (!enable)
        {
            app->fullscreen = false;
            SetWindowLong(app->win32WindowHandle, GWL_STYLE,
                          style | WS_OVERLAPPEDWINDOW);
            SetWindowPlacement(app->win32WindowHandle, &app->wpPrev);
            SetWindowPos(app->win32WindowHandle, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }

    void WindowPollEvents(Application* app)
    {
        MSG message;
        BOOL result;
        while (app->running &&
               (result = PeekMessage(&message, 0, 0, 0, PM_REMOVE)) != 0)
        {
            if (result == -1)
            {
                AB_CORE_FATAL("Window recieve error message.");
            }
            else
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
    }

    void WindowSetMousePosition(Application* app, u32 x, u32 y)
    {
        u32 yFlipped = 0;
        if (y < app->state.windowHeight)
        {
            yFlipped = app->state.windowHeight - y;
            POINT pt = { (LONG)x, (LONG)yFlipped };
            if (ClientToScreen(app->win32WindowHandle, &pt))
            {
                SetCursorPos(pt.x, pt.y);
            }
        }
    }

    void WindowShowCursor(Application* app, b32 show)
    {
        // TODO: Make this work (SetCursor())
        //::ShowCursor(show ? TRUE : FALSE);
    }

    void Win32Initialize(Application* app)
    {
        app->wpPrev = {sizeof(WINDOWPLACEMENT)};
        auto instance = GetModuleHandle(0);

        WNDCLASS windowClass = {};
        windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        windowClass.lpfnWndProc = Win32WindowCallback;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = WINDOW_CLASS_NAME;
        windowClass.hCursor = LoadCursor(0 ,IDC_ARROW);

        auto RCresult = RegisterClass(&windowClass);
        AB_CORE_ASSERT(RCresult, "Failed to create window.");

        HWND fakeWindow = CreateWindowEx(NULL, windowClass.lpszClassName,
                                         TEXT("AB Dummy window"), WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT,  CW_USEDEFAULT, 1, 1,
                                         NULL, NULL, instance, NULL);

        HDC fakeWindowDC = GetDC(fakeWindow);

        PIXELFORMATDESCRIPTOR fakeDesiredPixelFormat = {};
        fakeDesiredPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        fakeDesiredPixelFormat.nVersion = 1;
        fakeDesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        fakeDesiredPixelFormat.cColorBits = 32;
        fakeDesiredPixelFormat.cAlphaBits = 8;
        fakeDesiredPixelFormat.cDepthBits = 24;
        fakeDesiredPixelFormat.cStencilBits = 8;

        auto fakeActualPFIndex = ChoosePixelFormat(fakeWindowDC, &fakeDesiredPixelFormat);

        PIXELFORMATDESCRIPTOR fakeActualPixelFormat = {};
        DescribePixelFormat(fakeWindowDC, fakeActualPFIndex,
                            sizeof(PIXELFORMATDESCRIPTOR), &fakeActualPixelFormat);
        SetPixelFormat(fakeWindowDC, fakeActualPFIndex, &fakeActualPixelFormat);

        HGLRC fakeGLRC = wglCreateContext(fakeWindowDC);
        auto resultMC = wglMakeCurrent(fakeWindowDC, fakeGLRC);
        AB_CORE_ASSERT(resultMC, "Failed to create OpenGL context.");
        // TODO: Should it release dc?
        //ReleaseDC(windowHandle, windowDC);

        auto wglLoadProcsResult = WGLLoadFunctions(app, fakeWindowDC);
        AB_CORE_ASSERT(wglLoadProcsResult, "Failed to load WGL extensions");

        // ACTUAL WINDOW

        RECT actualSize = {};
        actualSize.top = 0;
        actualSize.left = 0;
        actualSize.right = app->state.windowWidth;
        actualSize.bottom = app->state.windowHeight;

        AdjustWindowRectEx(&actualSize, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           NULL, NULL);
#define AB_ABS_INT(n) ((n) < 0 ? -(n) : n)

        i32 width = AB_ABS_INT(actualSize.left) + AB_ABS_INT(actualSize.right);
        i32 height = AB_ABS_INT(actualSize.top) + AB_ABS_INT(actualSize.bottom);

        auto styleFlags = WS_THICKFRAME | WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        HWND actualWindowHandle = CreateWindowEx(NULL, windowClass.lpszClassName,
                                                 app->windowTitle,
                                                 styleFlags,
                                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                                 app->state.windowWidth,
                                                 app->state.windowHeight,
                                                 NULL, NULL, instance, app);
#undef AB_ABS_INT

        AB_CORE_ASSERT(actualWindowHandle, "Failed to create window.");

        HDC actualWindowDC = GetDC(actualWindowHandle);

        // ^^^^ ACTUAL WINDOW

        int attribList[] =
            {
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                //WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
                WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_COLOR_BITS_ARB, 32,
                WGL_DEPTH_BITS_ARB, 24,
                //WGL_STENCIL_BITS_ARB, 8,
                //SafeCastI32Int(multisampling), 1,
                //WGL_SAMPLES_ARB, SafeCastI32Int(window->samples),
                0
            };

        int actualPixelFormatID = 0;
        UINT numFormats = 0;
        // Here was fake DC
        auto resultCPF = app->wglChoosePixelFormatARB(actualWindowDC,
                                                      attribList, nullptr,
                                                      1, &actualPixelFormatID,
                                                      &numFormats);
        AB_CORE_ASSERT(resultCPF, "Failed to initialize OpenGL extended context.");

        PIXELFORMATDESCRIPTOR actualPixelFormat = {};
        auto resultDPF = DescribePixelFormat(actualWindowDC,
                                             actualPixelFormatID,
                                             sizeof(PIXELFORMATDESCRIPTOR),
                                             &actualPixelFormat);
        AB_CORE_ASSERT(resultDPF, "Failed to initialize OpenGL extended context.");
        SetPixelFormat(actualWindowDC, actualPixelFormatID, &actualPixelFormat);

        int contextAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, OPENGL_MAJOR_VERSION,
            WGL_CONTEXT_MINOR_VERSION_ARB, OPENGL_MINOR_VERSION,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if defined(AB_DEBUG_OPENGL)
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0
        };

        HGLRC actualGLRC = app->wglCreateContextAttribsARB(actualWindowDC,
                                                           0, contextAttribs);
        AB_CORE_ASSERT(actualGLRC, "Failed to initialize OpenGL extended context");

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(fakeGLRC);
        ReleaseDC(fakeWindow, fakeWindowDC);
        DestroyWindow(fakeWindow);

        resultMC = wglMakeCurrent(actualWindowDC, actualGLRC);
        AB_CORE_ASSERT(resultMC, "Failed to initialize OpenGL extended context");

        app->win32WindowHandle = actualWindowHandle;
        app->win32WindowDC = actualWindowDC;
        app->OpenGLRC = actualGLRC;

        app->Win32MouseTrackEvent.cbSize = sizeof(TRACKMOUSEEVENT);
        app->Win32MouseTrackEvent.dwFlags = TME_LEAVE;
        app->Win32MouseTrackEvent.dwHoverTime = HOVER_DEFAULT;
        app->Win32MouseTrackEvent.hwndTrack = app->win32WindowHandle;
        TrackMouseEvent(&app->Win32MouseTrackEvent);

        Win32InitKeyTable(app->keyTable);

        SetFocus(app->win32WindowHandle);

    }

    // NOTE: Stealing this from imgui win32 implementation!
    // Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

    static LRESULT CALLBACK
    Win32WindowCallback(HWND windowHandle, UINT message,
                        WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;

        if (message == WM_CREATE)
        {
            CREATESTRUCT* data = (CREATESTRUCT*)lParam;
            Application* app = (Application*)data->lpCreateParams;
            if (app)
            {
                SetWindowLongPtr(windowHandle, GWLP_USERDATA, (LONG_PTR)app);
                app->running = true;
            }
            return result;
        }

        auto ptr = GetWindowLongPtr(windowHandle, GWLP_USERDATA);

        if (ptr)
        {
            Application* app = (Application*)ptr;

            // NOTE: IMPORTANT: Always checking for nullptr
            ImGuiIO* io = nullptr;
            if (ImGui::GetCurrentContext())
            {
                ImGuiIO& _io = ImGui::GetIO();
                io = &_io;
            }

            switch (message)
            {
            case WM_SIZE:
            {
                app->state.windowWidth = LOWORD(lParam);
                app->state.windowHeight = HIWORD(lParam);
            } break;

            case WM_DESTROY:
            {
                PostQuitMessage(0);
            } break;

            case WM_CLOSE:
            {
                app->running = false;
                ShowWindow(app->win32WindowHandle, SW_HIDE);
            } break;

            // NOTE: MOUSE INPUT

            case WM_MOUSEMOVE:
            {
                if (!app->state.input.mouseInWindow)
                {
                    app->state.input.mouseInWindow = true;
                    TrackMouseEvent(&app->Win32MouseTrackEvent);
                }
                i32 mousePositionX = GET_X_LPARAM(lParam);
                i32 mousePositionY = GET_Y_LPARAM(lParam);

                mousePositionY = app->state.windowHeight - mousePositionY;

                f32 normalizedMouseX = (f32)mousePositionX /
                    (f32)app->state.windowWidth;
                f32 normalizedMouseY = (f32)mousePositionY /
                    (f32)app->state.windowHeight;

                app->state.input.mouseFrameOffsetX =
                    normalizedMouseX - app->state.input.mouseX;

                app->state.input.mouseFrameOffsetY =
                    normalizedMouseY - app->state.input.mouseY;

                switch (app->inputMode)
                {
                case INPUT_MODE_FREE_CURSOR:
                {
                    app->state.input.mouseX = normalizedMouseX;
                    app->state.input.mouseY = normalizedMouseY;
                } break;
                case INPUT_MODE_CAPTURE_CURSOR:
                {
                    u32 x = app->state.windowWidth / 2;
                    u32 y = app->state.windowHeight / 2;
                    app->state.input.mouseX = x / (f32)app->state.windowWidth;
                    app->state.input.mouseY = y / (f32)app->state.windowHeight;
                    WindowSetMousePosition(app, x, y);
                } break;
                default: { AB_CORE_ASSERT(false); } break;
                }
            } break;

            case WM_LBUTTONDOWN:
            {
                ProcessMButtonEvent(&app->state.input, MBUTTON_LEFT, true);
                ProcessMButtonDownAndDblclkEventImGui(app, io, message, wParam);
            } break;

            case WM_LBUTTONUP:
            {
                ProcessMButtonEvent(&app->state.input, MBUTTON_LEFT, false);
                ProcessMButtonUpEventImGui(app, io, message, wParam);
            } break;

            case WM_RBUTTONDOWN:
            {
                ProcessMButtonEvent(&app->state.input, MBUTTON_RIGHT, true);
                ProcessMButtonDownAndDblclkEventImGui(app, io, message, wParam);
            } break;

            case WM_RBUTTONUP:
            {
                ProcessMButtonEvent(&app->state.input, MBUTTON_RIGHT, false);
                ProcessMButtonUpEventImGui(app, io, message, wParam);
            } break;

            case WM_MBUTTONDOWN:
            {
                ProcessMButtonEvent(&app->state.input, MBUTTON_MIDDLE, true);
                ProcessMButtonDownAndDblclkEventImGui(app, io, message, wParam);
            } break;

            case WM_MBUTTONUP:
            {
                ProcessMButtonEvent(&app->state.input, MBUTTON_MIDDLE, false);
                ProcessMButtonUpEventImGui(app, io, message, wParam);
            } break;

            case WM_XBUTTONDOWN:
            {
                auto state = HIWORD(wParam);
                if (state & XBUTTON1)
                {
                    ProcessMButtonEvent(&app->state.input, MBUTTON_XBUTTON1, true);
                }
                else
                {
                    ProcessMButtonEvent(&app->state.input, MBUTTON_XBUTTON2, true);
                }
                ProcessMButtonDownAndDblclkEventImGui(app, io, message, wParam);
            } break;

            case WM_XBUTTONUP:
            {
                auto state = HIWORD(wParam);
                if (state & XBUTTON1)
                {
                    ProcessMButtonEvent(&app->state.input,MBUTTON_XBUTTON1, false);
                }
                else
                {
                    ProcessMButtonEvent(&app->state.input,MBUTTON_XBUTTON2, false);
                }
                ProcessMButtonUpEventImGui(app, io, message, wParam);
            } break;

            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDBLCLK:
            {
                ProcessMButtonDownAndDblclkEventImGui(app, io, message, wParam);
            } break;

            case WM_MOUSELEAVE:
            {
                app->state.input.mouseInWindow = false;
            } break;

            case WM_MOUSEWHEEL:
            {
                i32 delta = GET_WHEEL_DELTA_WPARAM(wParam);
                i32 numSteps = delta / WHEEL_DELTA;
                app->state.input.scrollOffset = numSteps;
                app->state.input.scrollFrameOffset = numSteps;

                if (io)
                {
                    io->MouseWheel += (float)numSteps;
                }
            } break;

            case WM_SETCURSOR:
            {
                if (LOWORD(lParam) == HTCLIENT && soko::ImGui::ImplSokoWin32_UpdateMouseCursor())
                {
                    result = 1;
                }
            } break;

            // ^^^^ MOUSE INPUT
            // KEYBOARD INPUT

            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                // TODO: Repeat counts for now doesnt working on windows
                // TODO: Why they are not working and why is this TODO here?
                u32 key = Win32KeyConvertToABKeycode(app, wParam);
                b32 state = true;
                u16 sys_repeat_count =  AB_KEY_REPEAT_COUNT_FROM_LPARAM(lParam);
                app->state.input.keys[key].wasPressed =
                    app->state.input.keys[key].pressedNow;
                app->state.input.keys[key].pressedNow = state;
                // TODO: Temorary
                if (app->state.input.keys[KEY_L].pressedNow &&
                    !app->state.input.keys[KEY_L].wasPressed &&
                    app->state.input.keys[KEY_CTRL].pressedNow)
                {
                    WindowToggleFullscreen(app, !app->fullscreen);
                }

                if (io && wParam < 256)
                {
                    io->KeysDown[wParam] = 1;
                }
            } break;

            case WM_SYSKEYUP:
            case WM_KEYUP:
            {
                u32 key = Win32KeyConvertToABKeycode(app, wParam);
                b32 state = false;
                u16 sys_repeat_count = 0;
                app->state.input.keys[key].wasPressed =
                    app->state.input.keys[key].pressedNow;
                app->state.input.keys[key].pressedNow = state;

                if (io && wParam < 256)
                {
                    io->KeysDown[wParam] = 0;
                }
            } break;

            case WM_CHAR:
            {
                u32 textBufferCount = app->state.input.textBufferCount;
                char* textBuffer =
                    app->state.input.textBuffer + textBufferCount;
                // NOTE: Reserve last character because wcstombs
                // null terminates strings
                u32 textBufferFree = PLATFORM_TEXT_INPUT_BUFFER_SIZE - textBufferCount - 1;
                // TODO: wcstombs implementation
                if (textBufferFree)
                {
                    size_t ret = wcstombs(textBuffer, (wchar_t*)(&wParam),
                                          textBufferFree);
                    if (ret != (size_t)(-1))
                    {
                        app->state.input.textBufferCount += (u32)ret;
                    }
                }

                if (io)
                {
                    DWORD wChar = (DWORD)wParam;
                    if (wChar <= 127)
                    {
                        io->AddInputCharacter((unsigned int)wParam);
                    }
                    else
                    {
                        // TODO: utf8 input support
                        // NOTE: swap lower and upper part.
                        BYTE low = (BYTE)(wChar & 0x00FF);
                        BYTE high = (BYTE)((wChar & 0xFF00) >> 8);
                        wChar = MAKEWORD(high, low);
                        wchar_t ch[6];
                        MultiByteToWideChar(CP_OEMCP, 0, (LPCSTR)&wChar, 4, ch, 3);
                        io->AddInputCharacter(ch[0]);
                    }
                }
            } break;
            // ^^^^ KEYBOARD INPUT

            case WM_ACTIVATEAPP:
            {
                if (wParam == TRUE)
                {
                    app->state.input.activeApp = true;
                }
                else
                {
                    app->state.input.activeApp = false;
                }
            } break;
            default:
            {
                result = DefWindowProc(windowHandle, message, wParam, lParam);
            } break;
            }
        }
        else
        {
            result = DefWindowProc(windowHandle, message, wParam, lParam);
        }
        return result;
    }

    static u8
    Win32KeyConvertToABKeycode(Application* app, u64 Win32Key)
    {
        if (Win32Key < KEYBOARD_KEYS_COUNT)
            return app->keyTable[Win32Key];
        return KEY_INVALIDKEY;
    }

    static void
    Win32InitKeyTable(u8* keytable)
    {
        memset(keytable, KEY_INVALIDKEY, KEYBOARD_KEYS_COUNT);

        keytable[0x08] = KEY_BACKSPACE;
        keytable[0x09] = KEY_TAB;
        keytable[0x0c] = KEY_CLEAR;
        keytable[0x0d] = KEY_ENTER;
        keytable[0x10] = KEY_SHIFT;
        keytable[0x11] = KEY_CTRL;
        keytable[0x12] = KEY_ALT;
        keytable[0x13] = KEY_PAUSE;
        keytable[0x14] = KEY_CAPS_LOCK;
        keytable[0x1b] = KEY_ESCAPE;
        keytable[0x20] = KEY_SPACE;
        keytable[0x21] = KEY_PAGE_UP;
        keytable[0x22] = KEY_PAGE_DOWN;
        keytable[0x23] = KEY_END;
        keytable[0x24] = KEY_HOME;
        keytable[0x25] = KEY_LEFT;
        keytable[0x26] = KEY_UP;
        keytable[0x27] = KEY_RIGHT;
        keytable[0x28] = KEY_DOWN;
        keytable[0x2c] = KEY_PRINT_SCREEN;
        keytable[0x2d] = KEY_INSERT;
        keytable[0x2e] = KEY_DELETE;
        keytable[0x30] = KEY_0;
        keytable[0x31] = KEY_1;
        keytable[0x32] = KEY_2;
        keytable[0x33] = KEY_3;
        keytable[0x34] = KEY_4;
        keytable[0x35] = KEY_5;
        keytable[0x36] = KEY_6;
        keytable[0x37] = KEY_7;
        keytable[0x38] = KEY_8;
        keytable[0x39] = KEY_9;
        keytable[0x41] = KEY_A;
        keytable[0x42] = KEY_B;
        keytable[0x43] = KEY_C;
        keytable[0x44] = KEY_D;
        keytable[0x45] = KEY_E;
        keytable[0x46] = KEY_F;
        keytable[0x47] = KEY_G;
        keytable[0x48] = KEY_H;
        keytable[0x49] = KEY_I;
        keytable[0x4a] = KEY_J;
        keytable[0x4b] = KEY_K;
        keytable[0x4c] = KEY_L;
        keytable[0x4d] = KEY_M;
        keytable[0x4e] = KEY_N;
        keytable[0x4f] = KEY_O;
        keytable[0x50] = KEY_P;
        keytable[0x51] = KEY_Q;
        keytable[0x52] = KEY_R;
        keytable[0x53] = KEY_S;
        keytable[0x54] = KEY_T;
        keytable[0x55] = KEY_U;
        keytable[0x56] = KEY_V;
        keytable[0x57] = KEY_W;
        keytable[0x58] = KEY_X;
        keytable[0x59] = KEY_Y;
        keytable[0x5a] = KEY_Z;
        keytable[0x5b] = KEY_LEFT_SUPER;
        keytable[0x5c] = KEY_RIGHT_SUPER;
        keytable[0x60] = KEY_NUM0;
        keytable[0x61] = KEY_NUM1;
        keytable[0x62] = KEY_NUM2;
        keytable[0x63] = KEY_NUM3;
        keytable[0x64] = KEY_NUM4;
        keytable[0x65] = KEY_NUM5;
        keytable[0x66] = KEY_NUM6;
        keytable[0x67] = KEY_NUM7;
        keytable[0x68] = KEY_NUM8;
        keytable[0x69] = KEY_NUM9;
        keytable[0x6a] = KEY_NUM_MULTIPLY;
        keytable[0x6b] = KEY_NUM_ADD;
        keytable[0x6d] = KEY_NUM_SUBTRACT;
        keytable[0x6e] = KEY_NUM_DECIMAL;
        keytable[0x6f] = KEY_NUM_DIVIDE;
        keytable[0x70] = KEY_F1;
        keytable[0x71] = KEY_F2;
        keytable[0x72] = KEY_F3;
        keytable[0x73] = KEY_F4;
        keytable[0x74] = KEY_F5;
        keytable[0x75] = KEY_F6;
        keytable[0x76] = KEY_F7;
        keytable[0x77] = KEY_F8;
        keytable[0x78] = KEY_F9;
        keytable[0x79] = KEY_F10;
        keytable[0x7a] = KEY_F11;
        keytable[0x7b] = KEY_F12;
        keytable[0x7c] = KEY_F13;
        keytable[0x7d] = KEY_F14;
        keytable[0x7e] = KEY_F15;
        keytable[0x7f] = KEY_F16;
        keytable[0x80] = KEY_F17;
        keytable[0x81] = KEY_F18;
        keytable[0x82] = KEY_F19;
        keytable[0x83] = KEY_F20;
        keytable[0x84] = KEY_F21;
        keytable[0x85] = KEY_F22;
        keytable[0x86] = KEY_F23;
        keytable[0x87] = KEY_F24;
        keytable[0x90] = KEY_NUM_LOCK;
        keytable[0x91] = KEY_SCROLL_LOCK;
        keytable[0xa0] = KEY_LEFT_SHIFT;
        keytable[0xa1] = KEY_RIGHT_SHIFT;
        // Only Ctrl now works and processed by SYSKEY events
        //keytable[0xa2] = KeyboardKey::LeftCtrl;
        //keytable[0xa3] = KeyboardKey::RightCtrl;
        //keytable[0xa4] = KeyboardKey::LeftAlt; 0x11
        keytable[0xa5] = KEY_MENU;
        keytable[0xba] = KEY_SEMICOLON;
        keytable[0xbb] = KEY_EQUAL;
        keytable[0xbc] = KEY_COMMA;
        keytable[0xbd] = KEY_MINUS;
        keytable[0xbe] = KEY_PERIOD;
        keytable[0xbf] = KEY_SLASH;
        keytable[0xc0] = KEY_TILDE;
        keytable[0xdb] = KEY_LEFT_BRACKET;
        keytable[0xdc] = KEY_BACK_SLASH;
        keytable[0xdd] = KEY_RIGHT_BRACKET;
        keytable[0xde] = KEY_APOSTROPHE;
    }

    unsigned int
    WGLLoadFunctions(Application* app, HDC windowDC)
    {
        app->wglGetExtensionsStringARB = (wglGetExtensionsStringARBFn*)wglGetProcAddress("wglGetExtensionsStringARB");
        if (!app->wglGetExtensionsStringARB)
            return 0;

        const char* extensions = app->wglGetExtensionsStringARB(windowDC);
        if (!strstr(extensions, "WGL_ARB_pixel_format"))
            return 0;
        if (!strstr(extensions, "WGL_ARB_create_context_profile"))
            return 0;
        if (!strstr(extensions, "EXT_swap_control"))
            return 0;

        app->wglChoosePixelFormatARB = (wglChoosePixelFormatARBFn*)wglGetProcAddress("wglChoosePixelFormatARB");
        if (!app->wglChoosePixelFormatARB)
            return 0;

        app->wglCreateContextAttribsARB = (wglCreateContextAttribsARBFn*)wglGetProcAddress("wglCreateContextAttribsARB");
        if (!app->wglCreateContextAttribsARB)
            return 0;

        app->wglSwapIntervalEXT = (wglSwapIntervalEXTFn*)wglGetProcAddress("wglSwapIntervalEXT");
        if (!app->wglCreateContextAttribsARB)
            return 0;

        app->wglGetSwapIntervalEXT = (wglGetSwapIntervalEXTFn*)wglGetProcAddress("wglGetSwapIntervalEXT");
        if (!app->wglCreateContextAttribsARB)
            return 0;

        return 1;
    }

    static HANDLE globalConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    b32 ConsolePrint(const char* string)
    {
        DWORD written;
        BOOL retVal = WriteConsoleA(globalConsoleHandle, string,
                                    (DWORD)strlen(string), &written, NULL);
        b32 result = (b32)retVal;
        return result;
    }

    b32 ConsolePrint(const void* data, u32 count)
    {
        DWORD written;
        BOOL result = WriteConsoleA(globalConsoleHandle, data, count, &written, NULL);
        if (written != count || result == 0)
        {
            return 0;

        }
        return 1;
    }

    b32 ConsoleSetColor(ConsoleColor textColor, ConsoleColor backColor)
    {
        if (textColor == CONSOLE_COLOR_DEFAULT)
            textColor = CONSOLE_DEFAULT_TEXT_COLOR;
        if (backColor == CONSOLE_COLOR_DEFAULT)
            backColor = CONSOLE_DEFAULT_BACK_COLOR;

        WORD consoleColor = static_cast<WORD>(textColor) |
            (static_cast<WORD>(backColor) << 4);
        BOOL result = SetConsoleTextAttribute(globalConsoleHandle, consoleColor);

        return (b32)result;
    }

    void GetExecutablePath(TCHAR* buffer, u32 bufferSizeBytes, u32* bytesWritten)
    {
        GetModuleFileName(NULL, buffer, bufferSizeBytes / sizeof(TCHAR));
    }

    f64 GetTimeStamp()
    {
        f64 time = 0.0;
        LARGE_INTEGER currentTime = {};
        if (QueryPerformanceCounter(&currentTime))
        {
            // NOTE: Check that performance was set
            AB_CORE_ASSERT(GlobalPerformanceFrequency.QuadPart);

            time = (f64)(currentTime.QuadPart) / (f64)GlobalPerformanceFrequency.QuadPart;
        }
        return time;
    }

    DateTime GetLocalTime()
    {
        DateTime datetime = {};
        SYSTEMTIME time;
        ::GetLocalTime(&time);

        datetime.year = time.wYear;
        datetime.month = time.wMonth;
        datetime.dayOfWeek = time.wDayOfWeek;
        datetime.day = time.wDay;
        datetime.hour = time.wHour;
        datetime.minute = time.wMinute;
        datetime.seconds = time.wSecond;
        datetime.milliseconds = time.wMilliseconds;

        return datetime;
    }

    inline b32
    TcharToWchar(wchar_t* dest, const TCHAR* src, u32 destSize)
    {
        b32 result = false;
#if defined(UNICODE) || defined(_UNICODE)
        auto cpyResult = wcscpy_s(dest, destSize, src);
        if (cpyResult == 0) result = true;
#else
        auto mbsResult = mbstowcs(dest, src, destSize);
        if (mbsResult) result = true;
#endif
        return result;
    }

    internal i32
    CountFilesInDirectory(const wchar_t* dirName)
    {
        i32 count = 0;
        WIN32_FIND_DATA fd;

        HANDLE handle = FindFirstFileW(dirName, &fd);

        if (handle != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    count++;
                }
            }
            while (FindNextFile(handle, &fd) != 0);

            auto error = GetLastError();
            if (error != ERROR_NO_MORE_FILES)
            {
                AB_CORE_ERROR("An error occured while enumerating files in directory (%u32).", (u32)error);
                count = -1;
            }
            FindClose(handle);
        }
        else
        {
            count = -1;
        }
        return count;
    }

    internal DirectoryContents
    EnumerateFilesInDirectory(const wchar_t* dirName, MemoryArena* tempArena)
    {
        // TODO: Actually support names that are larger than MAX_PATH
        // TODO: This is not well tested. Test with unicode paths, lengthy paths etc...
        DirectoryContents result = {};
        result.scannedSuccesfully = true;

        WIN32_FIND_DATA fd;
        u32 dirNameLen = (u32)wcslen(dirName);
        wchar_t* dirBuf = (wchar_t*)PUSH_SIZE(tempArena, sizeof(wchar_t) * (dirNameLen + 4)); // Extra byte just in case
        if (dirBuf)
        {
            wcscpy(dirBuf, dirName);
            wcscat(dirBuf, L"\\*");

            i32 count = CountFilesInDirectory(dirBuf);
            if (count > 0)
            {
                HANDLE handle = FindFirstFileW(dirBuf, &fd);

                if (handle != INVALID_HANDLE_VALUE)
                {
                    result.filenames = PUSH_ARRAY(tempArena, wchar_t*, count);
                    if (result.filenames)
                    {
                        do
                        {
                            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                            {
                                u32 filenameLen = (u32)_tcslen(fd.cFileName) + 1;
                                wchar_t* filename = (wchar_t*)PUSH_SIZE(tempArena, sizeof(wchar_t) * filenameLen);
                                if (filename)
                                {
                                    bool cvtResult = TcharToWchar(filename, fd.cFileName, filenameLen);
                                    if (cvtResult)
                                    {
                                        result.filenames[result.count] = filename;
                                        result.count++;
                                        AB_CORE_ASSERT(result.count <= (u32)count);
                                    }
                                    else
                                    {
                                        result.scannedSuccesfully = false;
                                        break;
                                    }
                                }
                                else
                                {
                                    result.scannedSuccesfully = false;
                                    break;
                                }
                            }
                        }
                        while (FindNextFile(handle, &fd) != 0);
                    }
                    else
                    {
                        result.scannedSuccesfully = false;
                    }

                    auto error = GetLastError();
                    if (error != ERROR_NO_MORE_FILES)
                    {
                        result.scannedSuccesfully = false;
                    }
                    FindClose(handle);
                }
            }
        }
        return result;
    }

    u32 DebugReadFileToBuffer(void* buffer, u32 bufferSize, const wchar_t* filename)
    {
        u32 written = 0;
        LARGE_INTEGER fileSize = {0};
        HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
                                       NULL, OPEN_EXISTING, 0, 0);
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            if (GetFileSizeEx(fileHandle, &fileSize))
            {
                DWORD readSize = (DWORD)bufferSize;
                if (fileSize.QuadPart < bufferSize)
                {
                    readSize = (DWORD)fileSize.QuadPart;
                }
                if (buffer)
                {
                    DWORD read;
                    BOOL result = ReadFile(fileHandle, buffer, readSize, &read, 0);
                    if (!result && !(read == readSize))
                    {
                        AB_CORE_WARN("Failed to read file.");
                    }
                    else
                    {
                        written = read;
                    }
                }
            }
            CloseHandle(fileHandle);
        }
        return written;
    }

    u32 DebugReadTextFileToBuffer(void* buffer, u32 bufferSize,
                                  const wchar_t* filename)
    {
        u32 bytesRead = 0;
        char* string = nullptr;
        LARGE_INTEGER fileSize = { 0 };
        HANDLE fileHandle = CreateFile(filename, GENERIC_READ,
                                       FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            if (GetFileSizeEx(fileHandle, &fileSize))
            {
                if (fileSize.QuadPart + 1 > bufferSize)
                {
                    AB_CORE_ERROR("Can`t read >4GB file.");
                    CloseHandle(fileHandle);
                    return 0;
                }
                if (buffer)
                {
                    DWORD read;
                    BOOL result = ReadFile(fileHandle, buffer,
                                           (DWORD)fileSize.QuadPart, &read, 0);
                    if (!result && !(read == (DWORD)fileSize.QuadPart))
                    {
                        AB_CORE_ERROR("Failed to read file.");
                        return 0;
                    }
                    else
                    {
                        ((char*)buffer)[fileSize.QuadPart] = '\0';
                        bytesRead = (u32)fileSize.QuadPart + 1;
                    }
                }
            }
            CloseHandle(fileHandle);
        }
        return bytesRead;
    }

    u32 DebugGetFileSize(const wchar_t* filename)
    {
        u32 fileSize = 0;
        HANDLE handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle != INVALID_HANDLE_VALUE)
        {
            DWORD sz = (u32)GetFileSize(handle, 0);
            if (sz != INVALID_FILE_SIZE)
            {
                fileSize = (u32)sz;
            }
            else
            {
                AB_CORE_ERROR("Failed to get file size.");
            }
            CloseHandle(handle);
        }
        else
        {
            AB_CORE_ERROR("Failed to open file.");
        }
        return fileSize;
    }

    internal bool
    DebugWriteFile(const wchar_t* filename, void* data, u32 dataSize)
    {
        HANDLE fileHandle = CreateFile(filename, GENERIC_WRITE, 0, 0,
                                       CREATE_ALWAYS, 0, 0);
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD bytesWritten;
            BOOL writeResult = WriteFile(fileHandle, data,
                                         dataSize, &bytesWritten, 0);
            if (writeResult && (dataSize == bytesWritten))
            {
                CloseHandle(fileHandle);
                return true;
            }
        }
        CloseHandle(fileHandle);
        return false;
    }

    internal FileHandle
    DebugOpenFile(const wchar_t* filename)
    {
        FileHandle result = INVALID_FILE_HANDLE;
        HANDLE w32Handle = CreateFile(filename,
                                      GENERIC_WRITE | GENERIC_READ,
                                      FILE_SHARE_READ, 0,
                                      CREATE_NEW, 0, 0);
        if (w32Handle != INVALID_HANDLE_VALUE)
        {
            result = (FileHandle)w32Handle;
        }
        return result;
    }

    internal bool
    DebugCloseFile(FileHandle handle)
    {
        bool result = false;
        if (CloseHandle((HANDLE)handle))
        {
            result = true;
        }
        return result;
    }

    internal u32
    DebugWriteToOpenedFile(FileHandle handle, void* data, u32 size)
    {
        u32 result = 0;
        DWORD bytesWritten;
        BOOL writeResult = WriteFile((HANDLE)handle, data, size, &bytesWritten, 0);
        if (writeResult && (size == bytesWritten))
        {
            result = size;
        }
        return result;
    }

    MemoryArena* AllocateArena(uptr size, bool isTemp)
    {
        uptr headerSize = sizeof(MemoryArena);
        void* mem = VirtualAlloc(0, size + headerSize,
                                 MEM_RESERVE | MEM_COMMIT,
                                 PAGE_READWRITE);
        AB_CORE_ASSERT(mem, "Allocation failed");
        AB_CORE_ASSERT((uptr)mem % 128 == 0, "Memory aligment violation");
        MemoryArena header = {};
        header.free = size;
        header.size = size;
        header.isTemporary = isTemp;
        header.begin = (void*)((byte*)mem + headerSize);
        COPY_STRUCT(MemoryArena, mem, &header);
        return (MemoryArena*)mem;
    }

    void FreeArena(MemoryArena* arena)
    {
        void* base = (void*)((byte*)arena->begin - sizeof(MemoryArena));
        auto result = VirtualFree(base, 0, MEM_RELEASE);
        AB_CORE_ASSERT(result);
    }

    static Application* GlobalApplication = nullptr;

    void SetInputMode(InputMode mode)
    {
        GlobalApplication->inputMode = mode;
    }

    void* AllocForImGui(size_t sz, void* userData)
    {
        // TODO: Temporary using windows api for general pupose allocations
        // for ImGui
        void* mem = HeapAlloc(*((HANDLE*)userData), IMGUI_HEAP_FLAGS, sz);
        return mem;
    }

    void FreeForImGui(void* ptr, void* userData)
    {
        if (ptr)
        {
            auto result = HeapFree(*((HANDLE*)userData), 0, ptr);
            AB_CORE_ASSERT(result);
        }
    }

    void AppRun(Application* app)
    {
        AB_CORE_INFO("Aberration engine");

        UINT sleepGranularityMs = 1;
        auto granularityWasSet = (timeBeginPeriod(sleepGranularityMs) == TIMERR_NOERROR);

        QueryPerformanceFrequency(&GlobalPerformanceFrequency);

        _tcscpy(app->windowTitle, TEXT("Sokoban"));
        app->state.windowWidth = 1280;
        app->state.windowHeight = 720;

        Win32Initialize(app);

        app->wglSwapIntervalEXT(1);

        OpenGLLoadResult glResult = LoadOpenGL(app->mainArena);
        AB_CORE_ASSERT(glResult.success, "Failed to load OpenGL functions");
        app->state.gl = glResult.context;

        app->state.functions.DebugGetFileSize = DebugGetFileSize;
        app->state.functions.DebugReadFile = DebugReadFileToBuffer;
        app->state.functions.DebugReadTextFile = DebugReadTextFileToBuffer;
        app->state.functions.DebugWriteFile = DebugWriteFile;
        app->state.functions.DebugOpenFile = DebugOpenFile;
        app->state.functions.DebugCloseFile = DebugCloseFile;
        app->state.functions.DebugWriteToOpenedFile = DebugWriteToOpenedFile;
        app->state.functions.FormatString = FormatString;
        app->state.functions.PrintString = PrintString;
        app->state.functions.Log = Log;
        app->state.functions.LogAssertV = LogAssertV;
        app->state.functions.SetInputMode = SetInputMode;

        app->state.functions.QueryNewArena = AllocateArena;
        app->state.functions.FreeArena = FreeArena;

        app->state.functions.GetTimeStamp = GetTimeStamp;

        app->state.functions.EnumerateFilesInDirectory = EnumerateFilesInDirectory;

        app->state.functions.AllocForImGui = AllocForImGui;
        app->state.functions.FreeForImGui = FreeForImGui;

        app->imGuiHeap = HeapCreate(IMGUI_HEAP_FLAGS, 0, 0);
        AB_CORE_ASSERT(app->imGuiHeap);

        app->state.imGuiAllocatorData = (void*)&app->imGuiHeap;

        SetupDirs(&app->gameLib);

        b32 codeLoaded = UpdateGameCode(&app->gameLib);
        AB_CORE_ASSERT(codeLoaded, "Failed to load game lib");

        app->gameArena = AllocateSubArena(app->mainArena, GAME_ARENA_SIZE, false);

        IMGUI_CHECKVERSION();
        ImGui::SetAllocatorFunctions(AllocForImGui, FreeForImGui, (void*)&app->imGuiHeap);
        app->state.imGuiContext = ImGui::CreateContext();
        ImGuiIO& _io = ImGui::GetIO();
        ImGuiIO* io = &_io;
        ImGui::StyleColorsDark();

        io->IniFilename = NULL;

        soko::ImGui::ImplSokoWin32_Init(app);
        auto imResult = ImGui_ImplOpenGL3_Init("#version 330 core");
        AB_CORE_ASSERT(imResult);

        f64 tickTimer = 1.0f;
        u32 updatesSinceLastTick = 0;

        app->state.localTime = AB::GetLocalTime();

        app->gameLib.GameUpdateAndRender(app->gameArena, &app->state,
                                         GUR_REASON_INIT);

        while (app->running)
        {
            auto tickStartTime = GetTimeStamp();

            WindowPollEvents(app);

            ImGui_ImplOpenGL3_NewFrame();
            soko::ImGui::ImplSokoWin32_NewFrame(app);
            ImGui::NewFrame();

            if (tickTimer <= 0)
            {
                tickTimer = 1.0f;
                app->state.ups = updatesSinceLastTick;
                updatesSinceLastTick= 0;
            }

            app->state.localTime = AB::GetLocalTime();

            bool codeReloaded = UpdateGameCode(&app->gameLib);
            if (codeReloaded)
            {
                app->gameLib.GameUpdateAndRender(app->gameArena,
                                                 &app->state,
                                                 GUR_REASON_RELOAD);
            }

            updatesSinceLastTick++;
            app->gameLib.GameUpdateAndRender(app->gameArena, &app->state,
                                             GUR_REASON_UPDATE);

            app->gameLib.GameUpdateAndRender(app->gameArena, &app->state,
                                             GUR_REASON_RENDER);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SwapBuffers(app->win32WindowDC);

            // NOTE: Cache hell!!!
            for (u32 keyIndex = 0; keyIndex < KEYBOARD_KEYS_COUNT; keyIndex ++)
            {
                app->state.input.keys[keyIndex].wasPressed =
                    app->state.input.keys[keyIndex].pressedNow;
            }

            for (u32 mbIndex = 0; mbIndex < MOUSE_BUTTONS_COUNT; mbIndex++)
            {
                app->state.input.mouseButtons[mbIndex].wasPressed =
                    app->state.input.mouseButtons[mbIndex].pressedNow;
            }

            app->state.input.scrollFrameOffset = 0;
            app->state.input.mouseFrameOffsetX = 0;
            app->state.input.mouseFrameOffsetY = 0;

            app->state.input.textBufferCount = 0;

            auto tickEndTime = GetTimeStamp();
            auto timeElapsed = tickEndTime - tickStartTime;
            while (timeElapsed < SECONDS_PER_TICK)
            {
                if (granularityWasSet)
                {
                    auto waitTime = (DWORD)(SECONDS_PER_TICK - timeElapsed) * 1000;
                    if (waitTime)
                    {
                        Sleep(waitTime);
                    }
                }
                auto nowTime = GetTimeStamp();
                timeElapsed = nowTime - tickStartTime;
            }

            tickTimer -= timeElapsed;
            app->state.absDeltaTime = (f32)timeElapsed;
            app->state.fps = (i32)(1.0f / app->state.absDeltaTime);
            app->state.gameDeltaTime = app->state.absDeltaTime * app->state.gameSpeed;
        }
    }

}
#if !defined(AB_ENABLE_CONSOLE_WINDOW)
int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nShowCmd)
#else
    int main()
#endif
{
    AB::MemoryArena* arena = AB::AllocateArena(AB::MAIN_ARENA_SIZE, false);
    AB::Application* app = nullptr;
    app = (AB::Application*)AB::PushSize(arena, sizeof(AB::Application), 0);
    AB::GlobalApplication = app;
    AB_CORE_ASSERT(app, "Failed to allocate Application.");
    app->mainArena = arena;
    AB::AppRun(app);
    return 0;
}

#include "Win32CodeLoader.cpp"
#include "PlatformLog.cpp"

#define AB_GL_FUNCTION(func) AB::GlobalApplication->state.gl->functions.fn.##func

#define glGetIntegerv AB_GL_FUNCTION(glGetIntegerv)
#define glBindSampler AB_GL_FUNCTION(glBindSampler)
#define glIsEnabled AB_GL_FUNCTION(glIsEnabled)
#define glScissor AB_GL_FUNCTION(glScissor)
#define glDrawElementsBaseVertex AB_GL_FUNCTION(glDrawElementsBaseVertex)
#define glDeleteVertexArrays AB_GL_FUNCTION(glDeleteVertexArrays)
#define glBindSampler AB_GL_FUNCTION(glBindSampler)
#define glBlendEquationSeparate AB_GL_FUNCTION(glBlendEquationSeparate)
#define glBlendFuncSeparate AB_GL_FUNCTION(glBlendFuncSeparate)
#define glPixelStorei AB_GL_FUNCTION(glPixelStorei)
#define glGetAttribLocation AB_GL_FUNCTION(glGetAttribLocation)
#define glDeleteBuffers AB_GL_FUNCTION(glDeleteBuffers)
#define glDetachShader AB_GL_FUNCTION(glDetachShader)
#define glDeleteProgram AB_GL_FUNCTION(glDeleteProgram)
#define glEnable AB_GL_FUNCTION(glEnable)
#define glBlendEquation AB_GL_FUNCTION(glBlendEquation)
#define glBlendFunc AB_GL_FUNCTION(glBlendFunc)
#define glDisable AB_GL_FUNCTION(glDisable)
#define glPolygonMode AB_GL_FUNCTION(glPolygonMode)
#define glViewport AB_GL_FUNCTION(glViewport)
#define glUseProgram AB_GL_FUNCTION(glUseProgram)
#define glUniform1i AB_GL_FUNCTION(glUniform1i)
#define glUniformMatrix4fv AB_GL_FUNCTION(glUniformMatrix4fv)
#define glBindVertexArray AB_GL_FUNCTION(glBindVertexArray)
#define glBindBuffer AB_GL_FUNCTION(glBindBuffer)
#define glEnableVertexAttribArray AB_GL_FUNCTION(glEnableVertexAttribArray)
#define glVertexAttribPointer AB_GL_FUNCTION(glVertexAttribPointer)
#define glActiveTexture AB_GL_FUNCTION(glActiveTexture)
#define glGenVertexArrays AB_GL_FUNCTION(glGenVertexArrays)
#define glBufferData AB_GL_FUNCTION(glBufferData)
#define glBindTexture AB_GL_FUNCTION(glBindTexture)
#define glTexParameteri AB_GL_FUNCTION(glTexParameteri)
#define glTexImage2D AB_GL_FUNCTION(glTexImage2D)
#define glGenTextures AB_GL_FUNCTION(glGenTextures)
#define glDeleteTextures AB_GL_FUNCTION(glDeleteTextures)
#define glGetShaderiv AB_GL_FUNCTION(glGetShaderiv)
#define glGetShaderInfoLog AB_GL_FUNCTION(glGetShaderInfoLog)
#define glGetProgramiv AB_GL_FUNCTION(glGetProgramiv)
#define glCreateShader AB_GL_FUNCTION(glCreateShader)
#define glShaderSource AB_GL_FUNCTION(glShaderSource)
#define glCompileShader AB_GL_FUNCTION(glCompileShader)
#define glCreateProgram AB_GL_FUNCTION(glCreateProgram)
#define glAttachShader AB_GL_FUNCTION(glAttachShader)
#define glLinkProgram AB_GL_FUNCTION(glLinkProgram)
#define glGetUniformLocation AB_GL_FUNCTION(glGetUniformLocation)
#define glGetProgramInfoLog AB_GL_FUNCTION(glGetProgramInfoLog)
#define glGenBuffers AB_GL_FUNCTION(glGenBuffers)
#define glDeleteShader AB_GL_FUNCTION(glDeleteShader)

// NOTE: IMGUI
#include "imgui/imconfig.h"
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"
#include "imgui/imgui_impl_opengl3.cpp"
#include "imgui/imgui_impl_soko_win32.cpp"
