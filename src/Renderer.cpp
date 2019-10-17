#include "Renderer.h"

#include "Shaders.cpp"

namespace soko
{
    struct SkyboxProgram
    {
        GLint handle;
        GLint viewMatrixLoc;
        GLint projMatrixLoc;
        GLint cubeTextureLoc;
        u32 cubeTextureSampler;
        GLenum cubeTextureSlot;
    };

    struct LineProgram
    {
        GLuint handle;
        GLint viewProjLocation;
        GLint colorLocation;
    };

    struct MeshProgram
    {
        GLint handle;
        u32 diffMapSampler;
        u32 specMapSampler;
        GLenum diffMapSlot;
        GLenum specMapSlot;
        GLint viewPosLocation;
        GLint viewProjLocation;
        GLint modelMtxLocation;
        GLint normalMtxLocation;
        GLint dirLightDirLoc;
        GLint dirLightAmbLoc;
        GLint dirLightDiffLoc;
        GLint dirLightSpecLoc;
        GLint diffMapLocation;
        GLint specMapLocation;
    };

    struct ChunkProgram
    {
        GLint handle;
        u32 atlasSampler;
        GLenum atlasSlot;
        GLint viewPosLocation;
        GLint viewProjLocation;
        GLint modelMtxLocation;
        GLint normalMtxLocation;
        GLint dirLightDirLoc;
        GLint dirLightAmbLoc;
        GLint dirLightDiffLoc;
        GLint dirLightSpecLoc;
        GLint terrainAtlasLoc;
        GLint aoDistribLoc;
    };

    enum TerrainTexture
    {
        TerrainTexture_Null = 0,
        TerrainTexture_Block,
    };

    constant u32 RENDERER_TILE_TEX_DIM = 256;
    constant u32 RENDERER_TERRAIN_TEX_ARRAY_SIZE = 32;
    constant u32 RENDERER_MAX_CHUNK_QUADS = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM * 4 / 2;
    constant u32 RENDERER_INDICES_PER_CHUNK_QUAD = 6;

    struct Renderer
    {
        LineProgram lineProgram;
        MeshProgram meshProgram;
        ChunkProgram chunkProgram;
        SkyboxProgram skyboxProgram;

        GLuint tileTexArrayHandle;

        GLuint lineBufferHandle;
        GLuint chunkIndexBuffer;
        v4 clearColor;

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

    internal LineProgram
    CreateLineProgram()
    {
        LineProgram result = {};
        auto handle = CreateProgram(LINE_VERTEX_SOURCE, LINE_FRAG_SOURCE);
        if (handle)
        {
            result.handle = handle;
            result.viewProjLocation = glGetUniformLocation(handle, "u_ViewProjMatrix");
            result.colorLocation = glGetUniformLocation(handle, "u_Color");
        }
        return result;
    }

    internal MeshProgram
    CreateMeshProgram()
    {
        MeshProgram result = {};
        auto handle = CreateProgram(MESH_VERTEX_SOURCE, MESH_FRAG_SOURCE);
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

    internal ChunkProgram
    CreateChunkProgram()
    {
        ChunkProgram result = {};
        auto handle = CreateProgram(CHUNK_VERTEX_SOURCE, CHUNK_FRAG_SOURCE);
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
            result.viewPosLocation = glGetUniformLocation(handle, "u_ViewPos");
            result.terrainAtlasLoc = glGetUniformLocation(handle, "u_TerrainAtlas");
            result.aoDistribLoc = glGetUniformLocation(handle, "u_AODistrib");

            result.atlasSampler = 0;
            result.atlasSlot = GL_TEXTURE0;

            glUseProgram(handle);

            glUniform1i(result.terrainAtlasLoc, result.atlasSampler);

            glUseProgram(0);
        }
        return result;
    }

    internal SkyboxProgram
    CreateSkyboxProgram()
    {
        SkyboxProgram result = {};
        auto handle = CreateProgram(SKYBOX_VERTEX_SOURCE, SKYBOX_FRAG_SOURCE);
        if (handle)
        {
            result.handle = handle;
            result.viewMatrixLoc = glGetUniformLocation(handle, "u_ViewMatrix");
            result.projMatrixLoc = glGetUniformLocation(handle, "u_ProjMatrix");
            result.cubeTextureLoc = glGetUniformLocation(handle, "u_CubeTexture");

            result.cubeTextureSampler = 0;
            result.cubeTextureSlot = GL_TEXTURE0;

            glUseProgram(handle);

            glUniform1i(result.cubeTextureLoc, result.cubeTextureSampler);

            glUseProgram(0);
        }
        return result;
    }

    internal Renderer*
    AllocAndInitRenderer(AB::MemoryArena* arena, AB::MemoryArena* tempArena)
    {
        Renderer* renderer = nullptr;
        renderer = PUSH_STRUCT(arena, Renderer);
        SOKO_ASSERT(renderer);

        renderer->lineProgram = CreateLineProgram();
        SOKO_ASSERT(renderer->lineProgram.handle);

        renderer->meshProgram = CreateMeshProgram();
        SOKO_ASSERT(renderer->meshProgram.handle);

        renderer->chunkProgram = CreateChunkProgram();
        SOKO_ASSERT(renderer->chunkProgram.handle);

        renderer->skyboxProgram = CreateSkyboxProgram();
        SOKO_ASSERT(renderer->skyboxProgram.handle);

        GLuint lineBufferHandle;
        glGenBuffers(1, &lineBufferHandle);
        SOKO_ASSERT(lineBufferHandle);
        renderer->lineBufferHandle = lineBufferHandle;

        GLuint chunkIndexBuffer;
        u32 indexCount = RENDERER_MAX_CHUNK_QUADS * RENDERER_INDICES_PER_CHUNK_QUAD;
        SOKO_ASSERT(indexCount % RENDERER_INDICES_PER_CHUNK_QUAD == 0);
        uptr size = indexCount * sizeof(u32);

        glGenBuffers(1, &chunkIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunkIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);
        u32* buffer = (u32*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE);

        u32 k = 0;
        for (u32 i = 0; i < indexCount; i += RENDERER_INDICES_PER_CHUNK_QUAD)
        {
            SOKO_ASSERT(i < indexCount);
            buffer[i] = k;
            buffer[i + 1] = k + 1;
            buffer[i + 2] = k + 2;
            buffer[i + 3] = k + 2;
            buffer[i + 4] = k + 3;
            buffer[i + 5] = k + 0;
            k += 4;
        }

        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        renderer->chunkIndexBuffer = chunkIndexBuffer;

        i32 width;
        i32 height;
        i32 bpp;
        BeginTemporaryMemory(tempArena);
        unsigned char* bitmap = stbi_load("../res/tile.png", &width, &height, &bpp, 3);

        GLuint terrainTexArray;
        glGenTextures(1, &terrainTexArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, terrainTexArray);
        // TODO : STUDY: glTexStorage and glTexSubImage
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8,
                     RENDERER_TILE_TEX_DIM, RENDERER_TILE_TEX_DIM,
                     RENDERER_TERRAIN_TEX_ARRAY_SIZE,
                     0, GL_RGB, GL_UNSIGNED_BYTE, 0);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, TerrainTexture_Block,
                        RENDERER_TILE_TEX_DIM, RENDERER_TILE_TEX_DIM, 1,
                        GL_RGB, GL_UNSIGNED_BYTE, bitmap);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        EndTemporaryMemory(tempArena);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        renderer->tileTexArrayHandle = terrainTexArray;

        return renderer;
    }

    internal void
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

    internal LoadedChunkMesh
    RendererLoadChunkMesh(ChunkMesh* mesh)
    {
        LoadedChunkMesh result = {};
        GLuint handle;
        static_assert(sizeof(u32) == sizeof(GLuint));

        glGenBuffers(1, &handle);
        if (handle)
        {
            glBindBuffer(GL_ARRAY_BUFFER, handle);
            uptr bufferSize = mesh->quadCount * 4 * sizeof(ChunkMeshVertex);
            glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_STATIC_DRAW);

            // TODO: Use glBufferSubData
            ChunkMeshVertex* buffer;
            buffer = (ChunkMeshVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
            SOKO_ASSERT(buffer);
            u32 bufferCount = 0;
            u32 blockCount = 0;
            ChunkMeshVertexBlock* block = mesh->tail;
            if (block)
            {
                do
                {
                    blockCount++;
                    for (u32 i = 0; i < block->at; i++)
                    {
                        buffer[bufferCount].pos = block->positions[i];
                        buffer[bufferCount].normal = block->normals[i];
                        buffer[bufferCount].tileId = block->tileIds[i];
                        buffer[bufferCount].AO = block->AO[i];
                        bufferCount++;
                    }
                    block = block->prevBlock;
                }
                while(block);
            }

            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            result.gpuHandle = handle;
            result.quadCount = mesh->quadCount;
        }
        return result;
    }

    internal void
    RendererLoadTexture(Texture* texture)
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
                case GL_RGB8: { format = GL_RGB; } break;
                    INVALID_DEFAULT_CASE;
                }

                glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->width,
                             texture->height, 0, format, GL_UNSIGNED_BYTE, texture->data);

                glBindTexture(GL_TEXTURE_2D, 0);

                texture->gpuHandle = handle;
            }
        }
    }

    internal void
    RendererLoadCubeTexture(CubeTexture* texture)
    {
        if (!texture->gpuHandle)
        {
            GLuint handle;
            glGenTextures(1, &handle);
            if (handle)
            {
                glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

                for (u32 i = 0; i < 6; i++)
                {
                    GLenum internalFormat = texture->images[i].format;
                    GLenum format;

                    u32 width = texture->images[i].width;
                    u32 height = texture->images[i].height;
                    void* data = texture->images[i].data;

                    switch (internalFormat)
                    {
                    case GL_RGBA8: { format = GL_RGBA; } break;
                    case GL_RGB8: { format = GL_RGB; } break;
                        INVALID_DEFAULT_CASE;
                    }

                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                                 internalFormat, width, height, 0,
                                 format, GL_UNSIGNED_BYTE, data);
                }

                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

                texture->gpuHandle = handle;
            }
        }
    }

    internal void
    RendererBeginFrame(Renderer* renderer, v2 viewportDim)
    {
        glViewport(0, 0, (GLsizei)viewportDim.x, (GLsizei)viewportDim.y);
        glClearColor(renderer->clearColor.r,
                     renderer->clearColor.g,
                     renderer->clearColor.b,
                     renderer->clearColor.a);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline void
    DrawSkybox(Renderer* renderer, RenderGroup* group,
               const m4x4* view, const m4x4* proj)
    {
        //glDisable(GL_DEPTH_TEST);
        //glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        auto prog = &renderer->skyboxProgram;
        glUseProgram(prog->handle);

        glUniformMatrix4fv(prog->viewMatrixLoc,
                           1, GL_FALSE, view->data);
        glUniformMatrix4fv(prog->projMatrixLoc,
                           1, GL_FALSE, proj->data);

        glActiveTexture(prog->cubeTextureSlot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, group->skyboxHandle);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDepthMask(GL_TRUE);

    }

    internal void
    FlushRenderGroup(Renderer* renderer, RenderGroup* group)
    {
        CameraConfig* camera = &group->cameraConfig;

        m4x4 lookAt = LookAtDirRH(camera->position, camera->front, V3(0.0f, 1.0f, 0.0f));
        m4x4 projection = PerspectiveOpenGLRH(camera->fovDeg, camera->aspectRatio,
                                              camera->nearPlane, camera->farPlane);
        m4x4 viewProj = MulM4M4(&projection, &lookAt);

        if (group->commandQueueAt)
        {
            bool firstLineShaderInvocation = true;
            bool firstMeshShaderInvocation = true;
            bool firstChunkMeshShaderInvocation = true;

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
                    m4x4 transModel = Transpose(&invModel);
                    m3x3 normalMatrix = M3x3(&transModel);

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

                case RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH:
                {
                    //glFrontFace(GL_CCW);
                    auto* chunkProg = &renderer->chunkProgram;
                    glUseProgram(chunkProg->handle);
                    glActiveTexture(chunkProg->atlasSlot);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->tileTexArrayHandle);

                    if (firstChunkMeshShaderInvocation)
                    {
                        local_persist v4 aoDistrib = {0.75f, 0.825f, 0.9f, 1.0f};
                        DEBUG_OVERLAY_SLIDER(aoDistrib, 0.0f, 1.0f);

                        glUniform4fv(chunkProg->aoDistribLoc, 1, aoDistrib.data);

                        firstChunkMeshShaderInvocation = false;
                        glUniformMatrix4fv(chunkProg->viewProjLocation,
                                           1, GL_FALSE, viewProj.data);
                        glUniform3fv(chunkProg->dirLightDirLoc, 1, group->dirLight.dir.data);
                        glUniform3fv(chunkProg->dirLightAmbLoc, 1, group->dirLight.ambient.data);
                        glUniform3fv(chunkProg->dirLightDiffLoc, 1, group->dirLight.diffuse.data);
                        glUniform3fv(chunkProg->dirLightSpecLoc, 1, group->dirLight.specular.data);
                        glUniform3fv(chunkProg->viewPosLocation, 1, group->cameraConfig.position.data);
                    }

                    for (u32 i = 0; i < command->instanceCount; i++)
                    {
                        auto* data = ((RenderCommandPushChunkMesh*)(group->renderBuffer + command->rbOffset)) + i;

                        m4x4 world = Translation(data->offset);
                        glUniformMatrix4fv(chunkProg->modelMtxLocation,
                                           1, GL_FALSE, world.data);

                        m4x4 invModel = world;
                        bool inverted = Inverse(&invModel);
                        SOKO_ASSERT(inverted);
                        m4x4 transModel = Transpose(&invModel);
                        m3x3 normalMatrix = M3x3(&transModel);

                        glUniformMatrix3fv(chunkProg->normalMtxLocation,
                                           1, GL_FALSE, normalMatrix.data);

                        glBindBuffer(GL_ARRAY_BUFFER, data->meshIndex);

                        GLsizei stride = sizeof(ChunkMeshVertex);
                        glEnableVertexAttribArray(0);
                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                        glEnableVertexAttribArray(1);
                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(v3)));
                        glEnableVertexAttribArray(2);
                        glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, stride, (void*)(sizeof(v3) * 2));
                        glEnableVertexAttribArray(3);
                        glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, stride, (void*)(sizeof(v3) * 2 + 1));

                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->chunkIndexBuffer);

                        GLsizei numIndices = (GLsizei)(data->quadCount * RENDERER_INDICES_PER_CHUNK_QUAD);
                        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
                    }
                } break;

                }
            }
            RenderGroupResetQueue(group);

        }

        if (group->drawSkybox)
        {
            glDepthFunc(GL_EQUAL);
            DrawSkybox(renderer, group, &lookAt, &projection);
            glDepthFunc(GL_LESS);
        }

    }

}
