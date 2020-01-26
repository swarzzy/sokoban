#include "OpenGLLoader.h"

namespace AB
{
    static const char* procNames[AB_OPENGL_FUNCTIONS_COUNT] =
    {
        // 1.0
        "glCullFace",
        "glFrontFace",
        "glHint",
        "glLineWidth",
        "glPointSize",
        "glPolygonMode",
        "glScissor",
        "glTexParameterf",
        "glTexParameterfv",
        "glTexParameteri",
        "glTexParameteriv",
        "glTexImage1D",
        "glTexImage2D",
        "glDrawBuffer",
        "glClear",
        "glClearColor",
        "glClearStencil",
        "glClearDepth",
        "glStencilMask",
        "glColorMask",
        "glDepthMask",
        "glDisable",
        "glEnable",
        "glFinish",
        "glFlush",
        "glBlendFunc",
        "glLogicOp",
        "glStencilFunc",
        "glStencilOp",
        "glDepthFunc",
        "glPixelStoref",
        "glPixelStorei",
        "glReadBuffer",
        "glReadPixels",
        "glGetBooleanv",
        "glGetDoublev",
        "glGetError",
        "glGetFloatv",
        "glGetIntegerv",
        "glGetString",
        "glGetTexImage",
        "glGetTexParameterfv",
        "glGetTexParameteriv",
        "glGetTexLevelParameterfv",
        "glGetTexLevelParameteriv",
        "glIsEnabled",
        "glDepthRange",
        "glViewport",
        // 1.1
        "glDrawArrays",
        "glDrawElements",
        "glGetPointerv",
        "glPolygonOffset",
        "glCopyTexImage1D",
        "glCopyTexImage2D",
        "glCopyTexSubImage1D",
        "glCopyTexSubImage2D",
        "glTexSubImage1D",
        "glTexSubImage2D",
        "glBindTexture",
        "glDeleteTextures",
        "glGenTextures",
        "glIsTexture",
        // 1.2
        "glDrawRangeElements",
        "glTexImage3D",
        "glTexSubImage3D",
        "glCopyTexSubImage3D",
        //1.3
        "glActiveTexture",
        "glSampleCoverage",
        "glCompressedTexImage3D",
        "glCompressedTexImage2D",
        "glCompressedTexImage1D",
        "glCompressedTexSubImage3D",
        "glCompressedTexSubImage2D",
        "glCompressedTexSubImage1D",
        "glGetCompressedTexImage",
        // 1.4
        "glBlendFuncSeparate",
        "glMultiDrawArrays",
        "glMultiDrawElements",
        "glPointParameterf",
        "glPointParameterfv",
        "glPointParameteri",
        "glPointParameteriv",
        "glBlendColor",
        "glBlendEquation",
        // 1.5
        "glGenQueries",
        "glDeleteQueries",
        "glIsQuery",
        "glBeginQuery",
        "glEndQuery",
        "glGetQueryiv",
        "glGetQueryObjectiv",
        "glGetQueryObjectuiv",
        "glBindBuffer",
        "glDeleteBuffers",
        "glGenBuffers",
        "glIsBuffer",
        "glBufferData",
        "glBufferSubData",
        "glGetBufferSubData",
        "glMapBuffer",
        "glUnmapBuffer",
        "glGetBufferParameteriv",
        "glGetBufferPointerv",
        // 2.0
        "glBlendEquationSeparate",
        "glDrawBuffers",
        "glStencilOpSeparate",
        "glStencilFuncSeparate",
        "glStencilMaskSeparate",
        "glAttachShader",
        "glBindAttribLocation",
        "glCompileShader",
        "glCreateProgram",
        "glCreateShader",
        "glDeleteProgram",
        "glDeleteShader",
        "glDetachShader",
        "glDisableVertexAttribArray",
        "glEnableVertexAttribArray",
        "glGetActiveAttrib",
        "glGetActiveUniform",
        "glGetAttachedShaders",
        "glGetAttribLocation",
        "glGetProgramiv",
        "glGetProgramInfoLog",
        "glGetShaderiv",
        "glGetShaderInfoLog",
        "glGetShaderSource",
        "glGetUniformLocation",
        "glGetUniformfv",
        "glGetUniformiv",
        "glGetVertexAttribdv",
        "glGetVertexAttribfv",
        "glGetVertexAttribiv",
        "glGetVertexAttribPointerv",
        "glIsProgram",
        "glIsShader",
        "glLinkProgram",
        "glShaderSource",
        "glUseProgram",
        "glUniform1f",
        "glUniform2f",
        "glUniform3f",
        "glUniform4f",
        "glUniform1i",
        "glUniform2i",
        "glUniform3i",
        "glUniform4i",
        "glUniform1fv",
        "glUniform2fv",
        "glUniform3fv",
        "glUniform4fv",
        "glUniform1iv",
        "glUniform2iv",
        "glUniform3iv",
        "glUniform4iv",
        "glUniformMatrix2fv",
        "glUniformMatrix3fv",
        "glUniformMatrix4fv",
        "glValidateProgram",
        "glVertexAttrib1d",
        "glVertexAttrib1dv",
        "glVertexAttrib1f",
        "glVertexAttrib1fv",
        "glVertexAttrib1s",
        "glVertexAttrib1sv",
        "glVertexAttrib2d",
        "glVertexAttrib2dv",
        "glVertexAttrib2f",
        "glVertexAttrib2fv",
        "glVertexAttrib2s",
        "glVertexAttrib2sv",
        "glVertexAttrib3d",
        "glVertexAttrib3dv",
        "glVertexAttrib3f",
        "glVertexAttrib3fv",
        "glVertexAttrib3s",
        "glVertexAttrib3sv",
        "glVertexAttrib4Nbv",
        "glVertexAttrib4Niv",
        "glVertexAttrib4Nsv",
        "glVertexAttrib4Nub",
        "glVertexAttrib4Nubv",
        "glVertexAttrib4Nuiv",
        "glVertexAttrib4Nusv",
        "glVertexAttrib4bv",
        "glVertexAttrib4d",
        "glVertexAttrib4dv",
        "glVertexAttrib4f",
        "glVertexAttrib4fv",
        "glVertexAttrib4iv",
        "glVertexAttrib4s",
        "glVertexAttrib4sv",
        "glVertexAttrib4ubv",
        "glVertexAttrib4uiv",
        "glVertexAttrib4usv",
        "glVertexAttribPointer",
        // 2.1
        "glUniformMatrix2x3fv",
        "glUniformMatrix3x2fv",
        "glUniformMatrix2x4fv",
        "glUniformMatrix4x2fv",
        "glUniformMatrix3x4fv",
        "glUniformMatrix4x3fv",
        // 3.0
        "glColorMaski",
        "glGetBooleani_v",
        "glGetIntegeri_v",
        "glEnablei",
        "glDisablei",
        "glIsEnabledi",
        "glBeginTransformFeedback",
        "glEndTransformFeedback",
        "glBindBufferRange",
        "glBindBufferBase",
        "glTransformFeedbackVaryings",
        "glGetTransformFeedbackVarying",
        "glClampColor",
        "glBeginConditionalRender",
        "glEndConditionalRender",
        "glVertexAttribIPointer",
        "glGetVertexAttribIiv",
        "glGetVertexAttribIuiv",
        "glVertexAttribI1i",
        "glVertexAttribI2i",
        "glVertexAttribI3i",
        "glVertexAttribI4i",
        "glVertexAttribI1ui",
        "glVertexAttribI2ui",
        "glVertexAttribI3ui",
        "glVertexAttribI4ui",
        "glVertexAttribI1iv",
        "glVertexAttribI2iv",
        "glVertexAttribI3iv",
        "glVertexAttribI4iv",
        "glVertexAttribI1uiv",
        "glVertexAttribI2uiv",
        "glVertexAttribI3uiv",
        "glVertexAttribI4uiv",
        "glVertexAttribI4bv",
        "glVertexAttribI4sv",
        "glVertexAttribI4ubv",
        "glVertexAttribI4usv",
        "glGetUniformuiv",
        "glBindFragDataLocation",
        "glGetFragDataLocation",
        "glUniform1ui",
        "glUniform2ui",
        "glUniform3ui",
        "glUniform4ui",
        "glUniform1uiv",
        "glUniform2uiv",
        "glUniform3uiv",
        "glUniform4uiv",
        "glTexParameterIiv",
        "glTexParameterIuiv",
        "glGetTexParameterIiv",
        "glGetTexParameterIuiv",
        "glClearBufferiv",
        "glClearBufferuiv",
        "glClearBufferfv",
        "glClearBufferfi",
        "glGetStringi",
        "glIsRenderbuffer",
        "glBindRenderbuffer",
        "glDeleteRenderbuffers",
        "glGenRenderbuffers",
        "glRenderbufferStorage",
        "glGetRenderbufferParameteriv",
        "glIsFramebuffer",
        "glBindFramebuffer",
        "glDeleteFramebuffers",
        "glGenFramebuffers",
        "glCheckFramebufferStatus",
        "glFramebufferTexture1D",
        "glFramebufferTexture2D",
        "glFramebufferTexture3D",
        "glFramebufferRenderbuffer",
        "glGetFramebufferAttachmentParameteriv",
        "glGenerateMipmap",
        "glBlitFramebuffer",
        "glRenderbufferStorageMultisample",
        "glFramebufferTextureLayer",
        "glMapBufferRange",
        "glFlushMappedBufferRange",
        "glBindVertexArray",
        "glDeleteVertexArrays",
        "glGenVertexArrays",
        "glIsVertexArray",
        // 3.1
        "glDrawArraysInstanced",
        "glDrawElementsInstanced",
        "glTexBuffer",
        "glPrimitiveRestartIndex",
        "glCopyBufferSubData",
        "glGetUniformIndices",
        "glGetActiveUniformsiv",
        "glGetActiveUniformName",
        "glGetUniformBlockIndex",
        "glGetActiveUniformBlockiv",
        "glGetActiveUniformBlockName",
        "glUniformBlockBinding",
        // 3.2
        "glDrawElementsBaseVertex",
        "glDrawRangeElementsBaseVertex",
        "glDrawElementsInstancedBaseVertex",
        "glMultiDrawElementsBaseVertex",
        "glProvokingVertex",
        "glFenceSync",
        "glIsSync",
        "glDeleteSync",
        "glClientWaitSync",
        "glWaitSync",
        "glGetInteger64v",
        "glGetSynciv",
        "glGetInteger64i_v",
        "glGetBufferParameteri64v",
        "glFramebufferTexture",
        "glTexImage2DMultisample",
        "glTexImage3DMultisample",
        "glGetMultisamplefv",
        "glSampleMaski",
        // 3.3
        "glBindFragDataLocationIndexed",
        "glGetFragDataIndex",
        "glGenSamplers",
        "glDeleteSamplers",
        "glIsSampler",
        "glBindSampler",
        "glSamplerParameteri",
        "glSamplerParameteriv",
        "glSamplerParameterf",
        "glSamplerParameterfv",
        "glSamplerParameterIiv",
        "glSamplerParameterIuiv",
        "glGetSamplerParameteriv",
        "glGetSamplerParameterIiv",
        "glGetSamplerParameterfv",
        "glGetSamplerParameterIuiv",
        "glQueryCounter",
        "glGetQueryObjecti64v",
        "glGetQueryObjectui64v",
        "glVertexAttribDivisor",
        "glVertexAttribP1ui",
        "glVertexAttribP1uiv",
        "glVertexAttribP2ui",
        "glVertexAttribP2uiv",
        "glVertexAttribP3ui",
        "glVertexAttribP3uiv",
        "glVertexAttribP4ui",
        "glVertexAttribP4uiv"
#if 0
        "glGetSubroutineUniformLocation",
        "glGetSubroutineIndex",
        "glGetActiveSubroutineUniformiv",
        "glGetActiveSubroutineUniformName",
        "glGetActiveSubroutineName",
        "glUniformSubroutinesuiv",
        "glGetUniformSubroutineuiv",
        "glGetProgramStageiv",

        "glTexStorage3D",
        "glCreateBuffers",
        "glNamedBufferData",
        "glMapNamedBuffer",
        "glUnmapNamedBuffer"
#endif
    };

#if defined(AB_PLATFORM_WINDOWS)
#include <Windows.h>

    void* LoadOpenGLExtFunction(const char* name)
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

    LoadFunctionsResult OpenGLLoadFunctions(MemoryArena* memoryArena)
    {
        GLFuncTable* funcTable = (GLFuncTable*)PushSize(memoryArena,
                                                    sizeof(GLFuncTable),
                                                    alignof(GLFuncTable));
        AB_CORE_ASSERT(funcTable, "Allocation failed.");

        b32 success = 1;
        HMODULE glLibHandle = {};
        for (u32 i = 0; i < AB_OPENGL_FUNCTIONS_COUNT; i++)
        {
            funcTable->procs[i] = wglGetProcAddress(procNames[i]);
            if (funcTable->procs[i] == 0 ||
                funcTable->procs[i] == (void*)0x1 ||
                funcTable->procs[i] == (void*)0x2 ||
                funcTable->procs[i] == (void*)0x3 ||
                funcTable->procs[i] == (void*)-1)
            {
                if (!glLibHandle)
                {
                    glLibHandle = LoadLibrary(TEXT("opengl32.dll"));
                }
                if (glLibHandle)
                {
                    funcTable->procs[i] = (AB_GLFUNCPTR)GetProcAddress(glLibHandle, procNames[i]);
                }
                else
                {
                    funcTable->procs[i] = NULL;
                    AB_CORE_ERROR("ERROR: Failed to load OpenGL procedure: %s\n", procNames[i]);
                    success = 0;
                }
            }
        }

        if (success)
        {
            GLint numExtensions;
            funcTable->_glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
            for (i32 i = 0; i < numExtensions; i++)
            {
                const GLubyte* extensionString;
                extensionString = funcTable->_glGetStringi(GL_EXTENSIONS, i);
                if (strcmp((const char*)extensionString, "GL_EXT_texture_filter_anisotropic") == 0)
                {
                    funcTable->EXT_texture_filter_anisotropic = true;
                }
                if (strcmp((const char*)extensionString, "GL_ARB_debug_output") == 0)
                {
                    funcTable->arbDebugOutput._glDebugMessageCallbackARB = (glDebugMessageCallbackARBFn*)LoadOpenGLExtFunction("glDebugMessageCallbackARB");
                    funcTable->arbDebugOutput._glDebugMessageControlARB = (glDebugMessageControlARBFn*)LoadOpenGLExtFunction("glDebugMessageControlARB");
                    if (funcTable->arbDebugOutput._glDebugMessageCallbackARB && funcTable->arbDebugOutput._glDebugMessageControlARB)
                    {
                        funcTable->ARB_debug_output = true;
                    }
                }
                if (funcTable->EXT_texture_filter_anisotropic &&
                    funcTable->ARB_debug_output)
                {
                    break;
                }
            }
            AB_CORE_ASSERT(funcTable->EXT_texture_filter_anisotropic);
        }

        return {funcTable, success};
    }

#elif defined(AB_PLATFORM_LINUX)
#include <dlfcn.h>

    typedef void* glXGetProcAddressFn(const GLubyte *procname);

    LoadFunctionsResult OpenGLLoadFunctions(MemoryArena* memoryArena)
    {
        glXGetProcAddressFn* glXGetProcAddress = nullptr;
        GLFuncTable* funcTable = nullptr;
        b32 success = false;

        void* libgl = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
        if (libgl)
        {
            glXGetProcAddress = (glXGetProcAddressFn*)dlsym(libgl,
                                                            "glXGetProcAddress");
            if (glXGetProcAddress)
            {
                funcTable = (GLFuncTable*)PushSize(memoryArena,
                                                 sizeof(GLFuncTable),
                                                 alignof(GLFuncTable));
                if (funcTable)
                {
                    for (u32 i = 0; i < AB_OPENGL_FUNCTIONS_COUNT; i++)
                    {
                        funcTable->procs[i] =
                            (AB_GLFUNCPTR)glXGetProcAddress((const uchar*)procNames[i]);
                        if (funcTable->procs[i])
                        {
                            success = true;
                        }
                        else
                        {
                            funcTable->procs[i] = nullptr;
                            AB_CORE_ERROR("ERROR: Failed to load OpenGL procedure: %s\n", procNames[i]);
                        }
                    }

                }
                else
                {
                    AB_CORE_ERROR("Failed to allocate OpenGL function table.");
                }

            }
            else
            {
                AB_CORE_ERROR("Failed to get glXGetProcAddress pointer.");
            }
        }
        else
        {
            AB_CORE_ERROR("Failed to load libGL.so.1 library.");
        }
        return {funcTable, success};
    }
#endif

    void OpenglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam)
    {
        const char* sourceStr;
        const char* typeStr;
        const char* severityStr;

        switch (source)
        {
        case GL_DEBUG_SOURCE_API_ARB: { sourceStr = "API"; } break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: { sourceStr = "window system"; } break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: { sourceStr = "shader compiler"; } break;
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: { sourceStr = "third party"; } break;
        case GL_DEBUG_SOURCE_APPLICATION_ARB: { sourceStr = "application"; } break;
        case GL_DEBUG_SOURCE_OTHER_ARB: { sourceStr = "other"; } break;
            INVALID_DEFAULT();
        }

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR_ARB: { typeStr = "error"; } break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: { typeStr = "deprecated behavior"; } break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: { typeStr = "undefined behavior"; } break;
        case GL_DEBUG_TYPE_PORTABILITY_ARB: { typeStr = "portability problem"; } break;
        case GL_DEBUG_TYPE_PERFORMANCE_ARB: { typeStr = "performance problem"; } break;
        case GL_DEBUG_TYPE_OTHER_ARB: { typeStr = "other"; } break;
            INVALID_DEFAULT();
        }

        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH_ARB: { severityStr = "high"; } break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB: { severityStr = "medium"; } break;
        case GL_DEBUG_SEVERITY_LOW_ARB: { severityStr = "low"; } break;
        case GL_DEBUG_SEVERITY_NOTIFICATION_ARB: { severityStr = "notification"; } break;
        default: { severityStr = "unknown"; } break;
        }
        PrintString("OpenGL debug message (source: %s, type: %s, severity: %s): %s\n", sourceStr, typeStr, severityStr, message);
        AB_DEBUG_BREAK();
    }


    void InitOpenGL(GLFuncTable* funcTable)
    {
        u32 globalVAO;
        funcTable->_glGenVertexArrays(1, &globalVAO);
        funcTable->_glBindVertexArray(globalVAO);
        //funcTable->_glEnable(GL_BLEND);
        //funcTable->_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //funcTable->_glBlendEquation(GL_FUNC_ADD);
        funcTable->_glEnable(GL_DEPTH_TEST);
        funcTable->_glDepthFunc(GL_LESS);
        funcTable->_glEnable(GL_CULL_FACE);
        funcTable->_glCullFace(GL_BACK);
        funcTable->_glFrontFace(GL_CCW);
        funcTable->_glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

#if defined(AB_DEBUG_OPENGL)
        funcTable->_glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        funcTable->arbDebugOutput._glDebugMessageCallbackARB(OpenglDebugCallback, 0);
        funcTable->arbDebugOutput._glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
        funcTable->arbDebugOutput._glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION_ARB, 0, 0, GL_FALSE);
        funcTable->arbDebugOutput._glDebugMessageControlARB(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER_ARB, GL_DEBUG_SEVERITY_LOW_ARB, 0, 0, GL_FALSE);
#endif
    }
}
