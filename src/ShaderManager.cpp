namespace soko
{
    struct Sampler
    {
        u32 sampler;
        GLuint slot;
    };

    internal GLuint
    CreateProgram(const char* vertexSource, const char* fragmentSource)
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
                                SOKO_WARN("Failed to compile shader!\n%s", message);
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
                        SOKO_WARN("Failed to compile shader!\n%s", message);
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
                SOKO_WARN("Failed to compile shader!\n%s", message);
            }
        }
        else
        {
            SOKO_ASSERT(false, "Falled to create vertex shader");
        }

        return resultHandle;
    }
}
#include "Shaders_Generated.cpp"
