#include "Shaders.h"
namespace soko
{
    GLuint CompileGLSL(const char* name, const char* vertexSource, const char* fragmentSource)
    {
        GLuint resultHandle = 0;

        GLuint vertexHandle = glCreateShader(GL_VERTEX_SHADER);
        if (vertexHandle)
        {
            glShaderSource(vertexHandle, 1, &vertexSource, nullptr);
            glCompileShader(vertexHandle);

            GLint vertexResult = 0;
            glGetShaderiv(vertexHandle, GL_COMPILE_STATUS, &vertexResult);
            if (vertexResult)
            {
                GLuint fragmentHandle;
                fragmentHandle = glCreateShader(GL_FRAGMENT_SHADER);
                if (fragmentHandle)
                {
                    glShaderSource(fragmentHandle, 1, &fragmentSource, nullptr);
                    glCompileShader(fragmentHandle);

                    GLint fragmentResult = 0;
                    glGetShaderiv(fragmentHandle, GL_COMPILE_STATUS, &fragmentResult);
                    if (fragmentResult)
                    {
                        GLint programHandle;
                        programHandle = glCreateProgram();
                        if (programHandle)
                        {
                            glAttachShader(programHandle, vertexHandle);
                            glAttachShader(programHandle, fragmentHandle);
                            glLinkProgram(programHandle);

                            GLint linkResult = 0;
                            glGetProgramiv(programHandle, GL_LINK_STATUS, &linkResult);
                            if (linkResult)
                            {
                                glDeleteShader(vertexHandle);
                                glDeleteShader(fragmentHandle);
                                resultHandle = programHandle;
                            }
                            else
                            {
                                i32 logLength;
                                glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH, &logLength);
                                // TODO: Stop using alloca
                                char* message = (char*)alloca(logLength);
                                SOKO_ASSERT(message, "");
                                glGetProgramInfoLog(programHandle, logLength, 0, message);
                                PrintString("[Error]: Failed to link shader program (%s) \n%s\n", name, message);
                            }
                        }
                        else
                        {
                            SOKO_ASSERT(false, "Failed to create shader program");
                        }
                    }
                    else
                    {
                        GLint logLength;
                        glGetShaderiv(fragmentHandle, GL_INFO_LOG_LENGTH, &logLength);
                        GLchar* message = (GLchar*)alloca(logLength);
                        SOKO_ASSERT(message, "");
                        glGetShaderInfoLog(fragmentHandle, logLength, nullptr, message);
                        PrintString("[Error]: Failed to compile frag shader (%s)\n%s\n", name, message);
                    }
                }
                else
                {
                    SOKO_ASSERT(false, "Failed to create fragment shader");
                }
            }
            else
            {
                GLint logLength;
                glGetShaderiv(vertexHandle, GL_INFO_LOG_LENGTH, &logLength);
                GLchar* message = (GLchar*)alloca(logLength);
                SOKO_ASSERT(message, "");
                glGetShaderInfoLog(vertexHandle, logLength, nullptr, message);
                PrintString("[Error]: Failed to compile vertex shader (%s)\n%s", name, message);
            }
        }
        else
        {
            SOKO_ASSERT(false, "Falled to create vertex shader");
        }
        return resultHandle;
    }

    void RecompileShaders(Renderer* renderer)
    {
        for (u32x i = 0; i < ArrayCount(renderer->shaderHandles); i++)
        {
            auto handle = renderer->shaderHandles[i];
            if (handle)
            {
                DeleteProgram(handle);
            }
            renderer->shaderHandles[i] = CompileGLSL(ShaderNames[i], ShaderSources[i].vert, ShaderSources[i].frag);
        }
    }

    template<typename T, u32 Binding>
    inline void ReallocUniformBuffer(UniformBuffer<T, Binding>* buffer)
    {
        if (buffer->handle)
        {
            glDeleteBuffers(1, &buffer->handle);
            buffer->handle = 0;
        }
        glCreateBuffers(1, &buffer->handle);
        SOKO_ASSERT(buffer->handle);
        glNamedBufferStorage(buffer->handle, sizeof(T), 0, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
    }

    template<typename T, u32 Binding>
    T* Map(UniformBuffer<T, Binding> buffer)
    {
        auto mem = (T*)glMapNamedBuffer(buffer.handle, GL_WRITE_ONLY);
        SOKO_ASSERT(mem);
        return mem;
    }

    template<typename T, u32 Binding>
    void Unmap(UniformBuffer<T, Binding> buffer)
    {
        glUnmapNamedBuffer(buffer.handle);

        glBindBuffer(GL_UNIFORM_BUFFER, buffer.handle);
        glBindBufferRange(GL_UNIFORM_BUFFER, Binding, buffer.handle, 0, sizeof(T));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

}
