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

	static const char* MESH_VERTEX_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 attr_Pos;
layout (location = 1) in vec3 attr_Normal;
layout (location = 2) in vec2 attr_UV;

uniform mat4 u_ViewProjMatrix;
uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;

out vec3 vout_Normal;
out vec2 vout_UV;

void main()
{
    gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(attr_Pos, 1.0f);
	vout_UV = attr_UV;
	vout_Normal = u_NormalMatrix * attr_Normal;
})";

	static const char* MESH_FRAG_SOURCE = R"(
#version 330 core
out vec4 out_Color;

in vec3 vout_FragPos;
in vec3 vout_Normal;
in vec2 vout_UV;

struct DirLight
{
	vec3 dir;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform DirLight u_DirLight;

uniform vec3 u_ViewPos;

uniform sampler2D u_DiffMap;
uniform sampler2D u_SpecMap;

void main()
{
	vec3 normal = normalize(vout_Normal);
	vec4 diffSamle = texture(u_DiffMap, vout_UV);
	vec4 specSample = texture(u_SpecMap, vout_UV);
	specSample.a = 1.0f;
	float kDiff = max(dot(normal, -u_DirLight.dir), 0.0f);
	vec3 viewDir = normalize(u_ViewPos - vout_FragPos);
	vec3 rFromLight = reflect(u_DirLight.dir, normal);
	float kSpec = pow(max(dot(viewDir, rFromLight), 0.0f), 32.0f);
	vec4 ambient = diffSamle * vec4(u_DirLight.ambient, 1.0f);
	vec4 diffuse = diffSamle * kDiff * vec4(u_DirLight.diffuse, 1.0f);
	vec4 specular = specSample * kSpec * vec4(u_DirLight.specular, 1.0f);
	out_Color = ambient + diffuse + specular;
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

	static MeshProgram
	_CreateMeshProgram()
	{
		MeshProgram result = {};
		auto handle = _CreateProgram(MESH_VERTEX_SOURCE, MESH_FRAG_SOURCE);
		if (handle)
		{
			result.handle = handle;
			result.viewProjLocation = glGetUniformLocation(handle, "u_ViewProjMatrix");
			result.modelMtxLocation = glGetUniformLocation(handle, "u_ModelMatrix");
			result.normalMtxLocation = glGetUniformLocation(handle, "u_NormalMatrix");
			result.dirLightDirLoc = glGetUniformLocation(handle, "u_DirLight.dir");
			result.dirLightAmbLoc = glGetUniformLocation(handle, "u_DirLight.ambient");
			result.dirLightDiffLoc = glGetUniformLocation(handle, "u_DirLight.diffuse");
			result.dirLightSpecLoc = glGetUniformLocation(handle, "u_DirLight.specular");
			result.diffMapLocation = glGetUniformLocation(handle, "u_DiffMap");
			result.viewPosLocation = glGetUniformLocation(handle, "u_ViewPos");
			result.specMapLocation = glGetUniformLocation(handle, "u_SpecMap");

			result.diffMapSampler = 0;
			result.specMapSampler = 1;
			result.diffMapSlot = GL_TEXTURE0;
			result.specMapSlot = GL_TEXTURE1;

			glUseProgram(handle);

			glUniform1i(result.diffMapLocation, result.diffMapSampler);
			glUniform1i(result.specMapLocation, result.specMapSampler);

			glUseProgram(0);
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

		renderer->meshProgram = _CreateMeshProgram();
		SOKO_ASSERT(renderer->meshProgram.handle, "");

		GLuint lineBufferHandle;
		glGenBuffers(1, &lineBufferHandle);
		SOKO_ASSERT(lineBufferHandle, "");
		renderer->lineBufferHandle = lineBufferHandle;

		return renderer;
	}

void
RendererLoadMesh(Mesh* mesh)
{
	if (!mesh->gpuVertexBufferHandle && !mesh->gpuIndexBufferHandle)
	{
		GLuint vboHandle;
		GLuint iboHandle;
		glGenBuffers(1, &vboHandle);
		glGenBuffers(1, &iboHandle);
		if (vboHandle && iboHandle)
		{
// NOTE: Using SOA layout of buffer
			u64 verticesSize = mesh->vertexCount * sizeof(v3);
			u64 normalsSize= mesh->normalCount * sizeof(v3);
			u64 uvsSize = mesh->uvCount * sizeof(v2);
			u64 indexBufferSize = mesh->indexCount * sizeof(u32);
			u64 vertexBufferSize = verticesSize + normalsSize + uvsSize;

			glBindBuffer(GL_ARRAY_BUFFER, vboHandle);

			glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, 0, GL_STATIC_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, verticesSize, (void*)mesh->vertices);
			glBufferSubData(GL_ARRAY_BUFFER, verticesSize, normalsSize, (void*)mesh->normals);
			glBufferSubData(GL_ARRAY_BUFFER, verticesSize + normalsSize, uvsSize, (void*)mesh->uvs);
			
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboHandle);
			
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, (void*)mesh->indices, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			
			mesh->gpuVertexBufferHandle = vboHandle;
			mesh->gpuIndexBufferHandle = iboHandle;
		}
	}
}

void RendererLoadTexture(Texture* texture)
{
	if (!texture->gpuHandle)
	{
		GLuint handle;
		glGenTextures(1, &handle);
		if (handle)
		{
			glBindTexture(GL_TEXTURE_2D, handle);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			GLenum format;

			switch (texture->format)
			{
			case GL_RGBA8: { format = GL_RGBA; } break;
			SOKO_INVALID_DEFAULT_CASE;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->width,
						 texture->height, 0, format, GL_UNSIGNED_BYTE, texture->data);

			glBindTexture(GL_TEXTURE_2D, 0);

			texture->gpuHandle = handle;
		}
	}
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
			bool firstMeshShaderInvocation = true;

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
				case RENDER_COMMAND_DRAW_MESH:
				{
					auto* data = (RenderCommandDrawMesh*)(group->renderBuffer + command->rbOffset);

					auto* meshProg = &renderer->meshProgram;
					glUseProgram(meshProg->handle);

					if (firstMeshShaderInvocation)
					{
						firstMeshShaderInvocation = false;
						glUniformMatrix4fv(meshProg->viewProjLocation,
										   1, GL_FALSE, viewProj.data);
						glUniform3fv(meshProg->dirLightDirLoc, 1, group->dirLight.dir.data);
						glUniform3fv(meshProg->dirLightAmbLoc, 1, group->dirLight.ambient.data);
						glUniform3fv(meshProg->dirLightDiffLoc, 1, group->dirLight.diffuse.data);
						glUniform3fv(meshProg->dirLightSpecLoc, 1, group->dirLight.specular.data);
						glUniform3fv(meshProg->viewPosLocation, 1, group->cameraConfig.position.data);
					}

					glUniformMatrix4fv(meshProg->modelMtxLocation,
									   1, GL_FALSE, data->transform.data);
				
					m4x4 invModel = data->transform;
					bool inverted = Inverse(&invModel);
					SOKO_ASSERT(inverted, "");
					m3x3 normalMatrix = M3x3(&Transpose(&invModel));

					glUniformMatrix3fv(meshProg->normalMtxLocation,
									   1, GL_FALSE, normalMatrix.data);

					auto* mesh = data->mesh;

					glActiveTexture(meshProg->diffMapSlot);
					glBindTexture(GL_TEXTURE_2D, data->material->diffMap.gpuHandle);

					glActiveTexture(meshProg->specMapSlot);
					glBindTexture(GL_TEXTURE_2D, data->material->specMap.gpuHandle);


					glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);
				
					glEnableVertexAttribArray(0);
					glEnableVertexAttribArray(1);
					glEnableVertexAttribArray(2);

					u64 normalsOffset = mesh->vertexCount * sizeof(v3);
					u64 uvsOffset = normalsOffset + mesh->normalCount * sizeof(v3);
				
					glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
					glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)normalsOffset);
					glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)uvsOffset);

					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpuIndexBufferHandle);

					glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
				} break;
				}
			}
			RenderGroupResetQueue(group);
			
		}
	}
	
}


