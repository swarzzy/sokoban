#include "Renderer.h"
#include "hypermath.h"
#include "RenderGroup.h"

#include <malloc.h>

namespace soko
{
	static const char* LINE_VERTEX_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 v_Pos;

out vec3 f_Color;

uniform mat4 u_ViewProjMatrix;
uniform vec3 u_Color;

void main()
{
    gl_Position = u_ViewProjMatrix * vec4(v_Pos, 1.0f);
	f_Color = u_Color;
})";

	static const char* LINE_FRAG_SOURCE = R"(
#version 330 core
out vec4 fragColor;

in vec3 f_Color;

void main()
{
	fragColor = vec4(f_Color, 1.0f);
})";

	static GLuint
	_CreateProgram(const char* vertexSource, const char* fragmentSource) 
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
								char* message = (char*)alloca(logLength);
								SOKO_ASSERT(message, "");
								glGetProgramInfoLog(programHandle, logLength, 0, message);
								SOKO_ASSERT(false, "Shader program linking error:\n%s", message);
							}
						} 
						else 
						{
							SOKO_ASSERT(false, "Falled to create shader program");
						}
					}
					else
					{
						GLint logLength;
						glGetShaderiv(fragmentHandle, GL_INFO_LOG_LENGTH, &logLength);
						GLchar* message = (GLchar*)alloca(logLength);
						SOKO_ASSERT(message, "");
						glGetShaderInfoLog(fragmentHandle, logLength, nullptr, message);
						SOKO_ASSERT(false, "Frgament shader compilation error:\n%s", message);
					}
				}
				else 
				{
					SOKO_ASSERT(false, "Falled to create fragment shader");
				}
			}
			else 
			{
				GLint logLength;
				glGetShaderiv(vertexHandle, GL_INFO_LOG_LENGTH, &logLength);
				GLchar* message = (GLchar*)alloca(logLength);
				SOKO_ASSERT(message, "");
				glGetShaderInfoLog(vertexHandle, logLength, nullptr, message);
				SOKO_ASSERT(false, "Vertex shader compilation error:\n%s", message);
			}
		}
		else 
		{
			SOKO_ASSERT(false, "Falled to create vertex shader");
		}

		return resultHandle;
	}
	
	static LineProgram
	_CreateLineProgram()
	{
		LineProgram result = {};
		auto handle = _CreateProgram(LINE_VERTEX_SOURCE, LINE_FRAG_SOURCE);
		if (handle)
		{
			result.handle = handle;
			result.viewProjLocation = glGetUniformLocation(handle, "u_ViewProjMatrix");
			result.colorLocation = glGetUniformLocation(handle, "u_Color");
		}
		return result;
	}

	Renderer*
	AllocAndInitRenderer(AB::MemoryArena* arena)
	{
		Renderer* renderer = nullptr;
		renderer = PUSH_STRUCT(arena, Renderer);
		SOKO_ASSERT(renderer, "");

		renderer->lineProgram = _CreateLineProgram();
		SOKO_ASSERT(renderer->lineProgram.handle, "");

		GLuint lineBufferHandle;
		glGenBuffers(1, &lineBufferHandle);
		SOKO_ASSERT(lineBufferHandle, "");
		renderer->lineBufferHandle = lineBufferHandle;

		return renderer;
	}

	void
	RendererBeginFrame(Renderer* renderer, v2 viewportDim)
	{
		glViewport(0, 0, (GLsizei)viewportDim.x, (GLsizei)viewportDim.y);
		glClearColor(renderer->clearColor.r,
					 renderer->clearColor.g,
					 renderer->clearColor.b,
					 renderer->clearColor.a);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	
	void
	FlushRenderGroup(Renderer* renderer, RenderGroup* group)
	{
		if (group->commandQueueAt)
		{
			CameraConfig* camera = &group->cameraConfig;
			m4x4 lookAt = LookAtDirRH(camera->position, camera->front, V3(0.0f, 1.0f, 0.0f));
			m4x4 projection = PerspectiveOpenGLRH(camera->fovDeg, camera->aspectRatio,
												  camera->nearPlane, camera->farPlane);
			m4x4 viewProj = MulM4M4(&projection, &lookAt);

			bool firstLineShaderInvocation = true;

			for (u32 i = 0; i < group->commandQueueAt; i++)
			{
				CommandQueueEntry* command = group->commandQueue + i;

				switch (command->type)
				{
				case RENDER_COMMAND_DRAW_LINE_BEGIN:
				{
					auto* data = (RenderCommandDrawLineBegin*)(group->renderBuffer + command->rbOffset);

					glUseProgram(renderer->lineProgram.handle);
					// TODO: Store info about command queue contents and set uniforms at the beginning
					if (firstLineShaderInvocation)
					{
						firstLineShaderInvocation = false;
						glUniformMatrix4fv(renderer->lineProgram.viewProjLocation,
										   1, GL_FALSE, viewProj.data);
					}
					
					glUniform3fv(renderer->lineProgram.colorLocation,
								 1, data->color.data);

					u64 bufferSize = command->instanceCount * sizeof(RenderCommandPushLineVertex);
					void* instanceData = (void*)((byte*)data + sizeof(RenderCommandDrawLineBegin));

					glBindBuffer(GL_ARRAY_BUFFER, renderer->lineBufferHandle);
					glBufferData(GL_ARRAY_BUFFER, bufferSize, instanceData, GL_STATIC_DRAW);

					glEnableVertexAttribArray(0);
					glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(v3), 0);

					glLineWidth(data->width);

					GLuint lineType;
					switch (data->type)
					{
					case RENDER_LINE_TYPE_SEGMENTS: { lineType = GL_LINES; } break;
					case RENDER_LINE_TYPE_STRIP: { lineType = GL_LINE_STRIP; } break;
					default: {lineType = GL_LINES; SOKO_ASSERT(false, ""); } break;
					}

					glDrawArrays(lineType, 0, command->instanceCount);

				} break;
				}
			}
			RenderGroupResetQueue(group);
			
		}
	}
	
}


