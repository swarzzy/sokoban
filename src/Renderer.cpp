#include "Renderer.h"

#include "Shaders.cpp"

namespace soko
{
    struct EnvMapPrefilterProgram
    {
        GLint handle;
        GLint projLoc;
        GLint viewLoc;
        GLint roughnessLoc;
        GLint sourceCubemapLoc;
        u32 sourceCubemapSampler;
        GLenum sourceCubemapSlot;
    };

    struct IrradanceConvolutionProgram
    {
        GLint handle;
        GLint projLoc;
        GLint viewLoc;
        GLint sourceCubemapLoc;
        u32 sourceCubemapSampler;
        GLenum sourceCubemapSlot;
    };

    struct PBRMeshProgram
    {
        GLint handle;

        u32 albedoSampler;
        GLenum albedoSlot;
        GLint albedoLoc;

        u32 irradanceMapSampler;
        GLenum irradanceMapSlot;
        GLint irradanceMapLoc;

        GLint viewPosLoc;
        GLint viewProjLoc;
        GLint modelMtxLoc;
        GLint normalMtxLoc;
        GLint dirLightDirLoc;
        GLint dirLightColorLoc;
        GLint metallicLoc;
        GLint roughnessLoc;
        GLint aoLoc;
    };

    struct FxaaProgram
    {
        GLint handle;

        //GLint iterationsLoc;
        //GLint edgeMinLoc;
        //GLint edgeMaxLoc;
        //GLint subpixelQualityLoc;

        GLint invScreenSizeLoc;
        GLint colorSourceLoc;
        u32 colorSourceSampler;
        GLenum colorSourceSlot;
   };

    struct PostfxProgram
    {
        GLint handle;
        GLint gammaLoc;
        GLint expLoc;
        GLint colorSourceLoc;
        u32 colorSourceSampler;
        GLenum colorSourceSlot;
    };

    struct SkyboxProgram
    {
        b32 usesEqurectUV;
        GLint handle;
        GLint viewMatrixLoc;
        GLint projMatrixLoc;
        GLint cubeTextureLoc;
        GLint lodLoc;
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

    // TODO: Cleanup tile values and terrain textures
    enum TerrainTexture
    {
        TerrainTexture_Null = 0,
        TerrainTexture_Block = TileValue_Wall,
        TerrainTexture_Stone = TileValue_Stone,
        TerrainTexture_Grass = TileValue_Grass
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
        PostfxProgram postfxProgram;
        FxaaProgram fxaaProgram;
        PBRMeshProgram pbrMeshProgram;
        IrradanceConvolutionProgram irradanceConvProgram;
        EnvMapPrefilterProgram envMapPrefilterProgram;

        GLuint tileTexArrayHandle;

        GLuint lineBufferHandle;
        GLuint chunkIndexBuffer;
        v4 clearColor;

        uv2 renderRes;
        f32 gamma;
        f32 exposure;

        GLuint offscreenBufferHandle;
        GLuint offscreenColorTarget;
        GLuint offscreenDepthTarget;

        GLuint srgbBufferHandle;
        GLuint srgbColorTarget;

        GLfloat maxAnisotropy;

        GLuint captureFramebuffer;
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

    internal EnvMapPrefilterProgram
    CreateEnvMapPrefilterProgram()
    {
        EnvMapPrefilterProgram result = {};
        auto handle = CreateProgram(SKYBOX_VERTEX_SOURCE, ENV_MAP_PREFILTER_FRAG_SOURCE);
        if (handle)
        {
            result.handle = handle;
            result.viewLoc = glGetUniformLocation(handle, "u_ViewMatrix");
            result.projLoc = glGetUniformLocation(handle, "u_ProjMatrix");
            result.sourceCubemapLoc = glGetUniformLocation(handle, "uSourceCubemap");
            result.roughnessLoc = glGetUniformLocation(handle, "uRoughness");

            result.sourceCubemapSampler = 0;
            result.sourceCubemapSlot = GL_TEXTURE0;

            glUseProgram(handle);

            glUniform1i(result.sourceCubemapLoc, result.sourceCubemapSampler);

            glUseProgram(0);
        }
        return result;
    }

    internal IrradanceConvolutionProgram
    CreateIrradanceConvolutionProgram()
    {
        IrradanceConvolutionProgram result = {};
        auto handle = CreateProgram(SKYBOX_VERTEX_SOURCE, IRRADANCE_CONVOLUTION_FRAG_SOURCE);
        if (handle)
        {
            result.handle = handle;
            result.viewLoc = glGetUniformLocation(handle, "u_ViewMatrix");
            result.projLoc = glGetUniformLocation(handle, "u_ProjMatrix");
            result.sourceCubemapLoc = glGetUniformLocation(handle, "uSourceCubemap");

            result.sourceCubemapSampler = 0;
            result.sourceCubemapSlot = GL_TEXTURE0;

            glUseProgram(handle);

            glUniform1i(result.sourceCubemapLoc, result.sourceCubemapSampler);

            glUseProgram(0);
        }
        return result;
    }


    internal PBRMeshProgram
    CreatePBRMeshProgram()
    {
        PBRMeshProgram result = {};
        auto handle = CreateProgram(PBR_MESH_VERTEX_SOURCE, PBR_MESH_FRAG_SOURCE);
        if (handle)
        {
            result.handle = handle;
            result.viewPosLoc = glGetUniformLocation(handle, "uViewPos");
            result.viewProjLoc = glGetUniformLocation(handle, "uViewProjMatrix");
            result.modelMtxLoc = glGetUniformLocation(handle, "uModelMatrix");
            result.normalMtxLoc = glGetUniformLocation(handle, "uNormalMatrix");
            result.dirLightDirLoc = glGetUniformLocation(handle, "uDirLight.dir");
            result.dirLightColorLoc = glGetUniformLocation(handle, "uDirLight.color");
            result.albedoLoc = glGetUniformLocation(handle, "uAlbedoMap");
            result.metallicLoc = glGetUniformLocation(handle, "uMetallic");
            result.roughnessLoc = glGetUniformLocation(handle, "uRoughness");
            result.aoLoc = glGetUniformLocation(handle, "uAO");
            result.irradanceMapLoc = glGetUniformLocation(handle, "uIrradanceMap");


            result.albedoSampler = 0;
            result.albedoSlot = GL_TEXTURE0;
            result.irradanceMapSampler = 1;
            result.irradanceMapSlot = GL_TEXTURE1;


            glUseProgram(handle);

            glUniform1i(result.albedoLoc, result.albedoSampler);
            glUniform1i(result.irradanceMapLoc, result.irradanceMapSampler);

            glUseProgram(0);
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
    CreateSkyboxProgram(bool useEquirectUV = false)
    {
        SkyboxProgram result = {};
        GLuint handle;
        if (useEquirectUV)
        {
            handle = CreateProgram(SKYBOX_EQUIRECT_VERTEX_SOURCE, SKYBOX_EQUIRECT_FRAG_SOURCE);
        }
        else
        {
            handle = CreateProgram(SKYBOX_VERTEX_SOURCE, SKYBOX_FRAG_SOURCE);
        }
        if (handle)
        {
            result.handle = handle;
            result.viewMatrixLoc = glGetUniformLocation(handle, "u_ViewMatrix");
            result.projMatrixLoc = glGetUniformLocation(handle, "u_ProjMatrix");
            result.cubeTextureLoc = glGetUniformLocation(handle, "u_CubeTexture");
            result.lodLoc = glGetUniformLocation(handle, "uLod");

            result.cubeTextureSampler = 0;
            result.cubeTextureSlot = GL_TEXTURE0;

            glUseProgram(handle);

            glUniform1i(result.cubeTextureLoc, result.cubeTextureSampler);

            glUseProgram(0);
        }
        return result;
    }

    internal PostfxProgram
    CreatePostfxProgram()
    {
        PostfxProgram result = {};
        auto handle = CreateProgram(SS_VERTEX_SOURCE, POSTFX_FRAG_SOURCE);
        if (handle)
        {
            result.handle = handle;
            result.gammaLoc = glGetUniformLocation(handle, "u_Gamma");
            result.expLoc = glGetUniformLocation(handle, "u_Exposure");
            result.colorSourceLoc = glGetUniformLocation(handle, "u_ColorSourceLinear");

            result.colorSourceSampler = 0;
            result.colorSourceSlot = GL_TEXTURE0;

            glUseProgram(handle);

            glUniform1i(result.colorSourceLoc, result.colorSourceSampler);

            glUseProgram(0);
        }
        return result;
    }

    internal FxaaProgram
    CreateFxaaProgram()
    {
        FxaaProgram result = {};
        auto handle = CreateProgram(SS_VERTEX_SOURCE, FXAA_FRAG_SOURCE);
        if (handle)
        {
            result.handle = handle;
            result.invScreenSizeLoc = glGetUniformLocation(handle, "u_InvScreenSize");

            //result.iterationsLoc = glGetUniformLocation(handle, "ITERATIONS");
            //result.edgeMinLoc = glGetUniformLocation(handle, "EDGE_MIN_THRESHOLD");
            //result.edgeMaxLoc = glGetUniformLocation(handle, "EDGE_MAX_THRESHOLD");
            //result.subpixelQualityLoc = glGetUniformLocation(handle, "SUBPIXEL_QUALITY");

            result.colorSourceLoc = glGetUniformLocation(handle, "u_ColorSourcePerceptual");

            result.colorSourceSampler = 0;
            result.colorSourceSlot = GL_TEXTURE0;

            glUseProgram(handle);

            glUniform1i(result.colorSourceLoc, result.colorSourceSampler);

            glUseProgram(0);
        }
        return result;
    }

    internal Renderer*
    AllocAndInitRenderer(AB::MemoryArena* arena, AB::MemoryArena* tempArena, uv2 renderRes)
    {
        Renderer* renderer = nullptr;
        renderer = PUSH_STRUCT(arena, Renderer);
        SOKO_ASSERT(renderer);

        GLfloat maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        renderer->maxAnisotropy = maxAnisotropy;

        renderer->gamma = 2.4f;
        renderer->exposure = 1.0f;
        renderer->renderRes = renderRes;

        renderer->lineProgram = CreateLineProgram();
        SOKO_ASSERT(renderer->lineProgram.handle);

        renderer->meshProgram = CreateMeshProgram();
        SOKO_ASSERT(renderer->meshProgram.handle);

        renderer->chunkProgram = CreateChunkProgram();
        SOKO_ASSERT(renderer->chunkProgram.handle);

        renderer->skyboxProgram = CreateSkyboxProgram();
        SOKO_ASSERT(renderer->skyboxProgram.handle);

        renderer->postfxProgram = CreatePostfxProgram();
        SOKO_ASSERT(renderer->postfxProgram.handle);

        renderer->fxaaProgram = CreateFxaaProgram();
        SOKO_ASSERT(renderer->fxaaProgram.handle);

        renderer->pbrMeshProgram = CreatePBRMeshProgram();
        SOKO_ASSERT(renderer->pbrMeshProgram.handle);

        renderer->irradanceConvProgram = CreateIrradanceConvolutionProgram();
        SOKO_ASSERT(renderer->irradanceConvProgram.handle);

        renderer->envMapPrefilterProgram = CreateEnvMapPrefilterProgram();
        SOKO_ASSERT(renderer->envMapPrefilterProgram.handle);

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

        i32 stoneW;
        i32 stoneH;
        i32 stoneBpp;
        unsigned char* stoneBitmap = stbi_load("../res/tile_stone.png", &stoneW, &stoneH, &stoneBpp, 3);

        i32 grassW;
        i32 grassH;
        i32 grassBpp;
        unsigned char* grassBitmap = stbi_load("../res/tile_grass.png", &grassW, &grassH, &grassBpp, 3);


        GLuint terrainTexArray;
        glGenTextures(1, &terrainTexArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, terrainTexArray);

        // NOTE: https://www.khronos.org/opengl/wiki/Common_Mistakes#Automatic_mipmap_generation
        glEnable(GL_TEXTURE_2D_ARRAY);

        // TODO : STUDY: glTexStorage and glTexSubImage
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8,
                     RENDERER_TILE_TEX_DIM, RENDERER_TILE_TEX_DIM,
                     RENDERER_TERRAIN_TEX_ARRAY_SIZE,
                     0, GL_RGB, GL_UNSIGNED_BYTE, 0);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, TerrainTexture_Block,
                        RENDERER_TILE_TEX_DIM, RENDERER_TILE_TEX_DIM, 1,
                        GL_RGB, GL_UNSIGNED_BYTE, bitmap);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, TerrainTexture_Stone,
                        RENDERER_TILE_TEX_DIM, RENDERER_TILE_TEX_DIM, 1,
                        GL_RGB, GL_UNSIGNED_BYTE, stoneBitmap);

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, TerrainTexture_Grass,
                        RENDERER_TILE_TEX_DIM, RENDERER_TILE_TEX_DIM, 1,
                        GL_RGB, GL_UNSIGNED_BYTE, grassBitmap);

        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);


        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_LOD_BIAS, -0.1f); // -0.86 for trilinear
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, renderer->maxAnisotropy);

        EndTemporaryMemory(tempArena);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        renderer->tileTexArrayHandle = terrainTexArray;

        glGenFramebuffers(1, &renderer->offscreenBufferHandle);
        SOKO_ASSERT(renderer->offscreenBufferHandle);
        glGenTextures(1, &renderer->offscreenColorTarget);
        SOKO_ASSERT(renderer->offscreenColorTarget);
        glGenTextures(1, &renderer->offscreenDepthTarget);
        SOKO_ASSERT(renderer->offscreenDepthTarget);

        glBindTexture(GL_TEXTURE_2D, renderer->offscreenColorTarget);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, renderRes.x, renderRes.y, 0, GL_RGBA, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, renderer->offscreenDepthTarget);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, renderRes.x, renderRes.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, renderer->offscreenBufferHandle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->offscreenColorTarget, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, renderer->offscreenDepthTarget, 0);
        SOKO_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        glGenFramebuffers(1, &renderer->srgbBufferHandle);
        SOKO_ASSERT(renderer->srgbBufferHandle);
        glGenTextures(1, &renderer->srgbColorTarget);
        SOKO_ASSERT(renderer->srgbColorTarget);

        glBindTexture(GL_TEXTURE_2D, renderer->srgbColorTarget);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, renderRes.x, renderRes.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, renderer->srgbBufferHandle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->srgbColorTarget, 0);
        SOKO_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glGenFramebuffers(1, &renderer->captureFramebuffer);
        SOKO_ASSERT(renderer->captureFramebuffer);

#if 0
        // NOTE: Creating normalized cube vertex buffer

        const f32 vertices[] =
            {
                // back face
                -1.0f, -1.0f, -1.0f,
                1.0f,  1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                // front face
                -1.0f, -1.0f,  1.0f,
                1.0f, -1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,
                // left face
                -1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                // right face
                1.0f,  1.0f,  1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f,  1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f, -1.0f,  1.0f,
                // bottom face
                -1.0f, -1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f, -1.0f,  1.0f,
                1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f, -1.0f,
                // top face
                -1.0f,  1.0f, -1.0f,
                1.0f,  1.0f , 1.0f,
                1.0f,  1.0f, -1.0f,
                1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f,  1.0f,
            };

        glGenBuffers(1, &renderer->normalizedCubeVertexBufferHandle);
        SOKO_ASSERT(renderer->normalizedCubeVertexBufferHandle);

        glBindBuffer(GL_ARRAY_BUFFER, renderer->normalizedCubeVertexBufferHandle);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
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
                uptr verticesSize = mesh->vertexCount * sizeof(v3);
                uptr normalsSize= mesh->normalCount * sizeof(v3);
                uptr uvsSize = mesh->uvCount * sizeof(v2);
                uptr indexBufferSize = mesh->indexCount * sizeof(u32);
                uptr vertexBufferSize = verticesSize + normalsSize + uvsSize;

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

    internal u32 // NOTE: Quad count
    RendererReloadChunkMesh(const ChunkMesh* mesh, u32 handle)
    {
        u32 result = 0;
        glBindBuffer(GL_ARRAY_BUFFER, handle);
        uptr bufferSize = mesh->quadCount * 4 * sizeof(ChunkMeshVertex);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_STATIC_DRAW);

        if (bufferSize)
        {
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
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        result = mesh->quadCount;
        return result;
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
            result.quadCount = RendererReloadChunkMesh(mesh, handle);
            result.gpuHandle = handle;
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
                //glEnable(GL_TEXTURE_2D);
                GLenum format;
                GLenum type;

                switch (texture->format)
                {
                case GL_SRGB8_ALPHA8: { format = GL_RGBA; type = GL_UNSIGNED_BYTE; } break;
                case GL_SRGB8: { format = GL_RGB; type = GL_UNSIGNED_BYTE; } break;
                case GL_RGB16F: { format = GL_RGB; type = GL_FLOAT; } break;
                    INVALID_DEFAULT_CASE;
                }

                glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->width,
                             texture->height, 0, format, type, texture->data);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                // TODO: Anisotropy value
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);

                glGenerateMipmap(GL_TEXTURE_2D);


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

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

                auto minFilter = GL_NEAREST;
                auto magFilter = GL_NEAREST;

                switch (texture->filter)
                {
                case TextureFilter_None: {} break;
                case TextureFilter_Bilinear: { minFilter = GL_LINEAR; magFilter = GL_LINEAR; } break;
                case TextureFilter_Trilinear: { if (texture->useMips) minFilter = GL_LINEAR_MIPMAP_LINEAR; else minFilter = GL_LINEAR; magFilter = GL_LINEAR; } break;
                INVALID_DEFAULT_CASE;
                }

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);

                for (u32 i = 0; i < 6; i++)
                {
                    GLenum internalFormat = texture->images[i].format;
                    GLenum format;
                    GLenum type;

                    u32 width = texture->images[i].width;
                    u32 height = texture->images[i].height;
                    void* data = texture->images[i].data;

                    switch (internalFormat)
                    {
                    case GL_SRGB8_ALPHA8: { format = GL_RGBA; type = GL_UNSIGNED_BYTE; } break;
                    case GL_SRGB8: { format = GL_RGB; type = GL_UNSIGNED_BYTE; } break;
                    case GL_RGB16F: {format = GL_RGB; type = GL_FLOAT; } break;
                        INVALID_DEFAULT_CASE;
                    }

                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                                 internalFormat, width, height, 0,
                                 format, type, data);
                }

                if (texture->useMips)
                {
                    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
                }

                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

                texture->gpuHandle = handle;
            }
        }
    }

    internal void
    RendererBeginFrame(Renderer* renderer, v2 viewportDim)
    {
        DEBUG_OVERLAY_TRACE(renderer->maxAnisotropy);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->offscreenBufferHandle);
        glViewport(0, 0, (GLsizei)viewportDim.x, (GLsizei)viewportDim.y);
        glClearColor(renderer->clearColor.r,
                     renderer->clearColor.g,
                     renderer->clearColor.b,
                     renderer->clearColor.a);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
    }

    internal void
    RendererEndFrame(Renderer* renderer)
    {
        // NOTE: Gamma-correction pass
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->srgbBufferHandle);
        glClearColor(renderer->clearColor.r,
                     renderer->clearColor.g,
                     renderer->clearColor.b,
                     renderer->clearColor.a);

        glClear(GL_COLOR_BUFFER_BIT);
        auto prog = &renderer->postfxProgram;
        glUseProgram(prog->handle);
        glUniform1f(prog->gammaLoc, renderer->gamma);
        glUniform1f(prog->expLoc, renderer->exposure);

        glActiveTexture(prog->colorSourceSlot);
        glBindTexture(GL_TEXTURE_2D, renderer->offscreenColorTarget);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // NOTE: FXAA pass
        local_persist bool enableFXAA = true;
        DEBUG_OVERLAY_TOGGLE(enableFXAA);
        if (enableFXAA)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->srgbBufferHandle);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glClearColor(renderer->clearColor.r,
                         renderer->clearColor.g,
                         renderer->clearColor.b,
                         renderer->clearColor.a);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto fxaaProg = &renderer->fxaaProgram;
            glUseProgram(fxaaProg->handle);
            v2 invScreenSize = V2(1.0f / renderer->renderRes.x, 1.0f / renderer->renderRes.y);
            glUniform2fv(fxaaProg->invScreenSizeLoc, 1, invScreenSize.data);

            glActiveTexture(fxaaProg->colorSourceSlot);
            glBindTexture(GL_TEXTURE_2D, renderer->srgbColorTarget);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->srgbBufferHandle);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

            glBlitFramebuffer(0, 0, renderer->renderRes.x, renderer->renderRes.y,
                              0, 0, renderer->renderRes.x, renderer->renderRes.y,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);

        }
    }

    internal void
    GenIrradanceMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle)
    {
        SOKO_ASSERT(t->gpuHandle);
#define PROFILE_IRRADANCE_GEN
#if defined(PROFILE_IRRADANCE_GEN)
        SOKO_INFO("Generating irradance map...");
        glFinish();
        i64 beginTime = GetTimeStamp();
#endif

        const m4x4 capProj = PerspectiveOpenGLRH(90.0f, 1.0f, 0.1, 10.0f);
        const m4x4 capViews[] =
        {
            LookAtRH(V3(0.0f), V3(1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f)),
            LookAtRH(V3(0.0f), V3(-1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f)),
            LookAtRH(V3(0.0f), V3(0.0f, 1.0f, 0.0f), V3(0.0f, 0.0f, 1.0f)),
            LookAtRH(V3(0.0f), V3(0.0f, -1.0f, 0.0f), V3(0.0f, 0.0f, -1.0f)),
            LookAtRH(V3(0.0f), V3(0.0f, 0.0f, 1.0f), V3(0.0f, -1.0f, 0.0f)),
            LookAtRH(V3(0.0f), V3(0.0f, 0.0f, -1.0f), V3(0.0f, -1.0f, 0.0f)),
        };

        auto prog = &renderer->irradanceConvProgram;
        glUseProgram(prog->handle);
        glUniformMatrix4fv(prog->projLoc, 1, GL_FALSE, capProj.data);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

        glActiveTexture(prog->sourceCubemapSlot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sourceHandle);

        for (u32 i = 0; i < 6; i++)
        {
            glViewport(0, 0, t->images[i].width, t->images[i].height);
            glUniformMatrix4fv(prog->viewLoc, 1, GL_FALSE, capViews[i].data);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, t->gpuHandle, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

#if defined (PROFILE_IRRADANCE_GEN)
        glFinish();
        i64 timeElapsed = GetTimeStamp() - beginTime;
        SOKO_INFO("Time: %i64 us", timeElapsed);
#endif

    }

    internal void
    GenEnvPrefiliteredMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle, u32 mipLevels)
    {
        SOKO_ASSERT(t->gpuHandle);
        SOKO_ASSERT(t->useMips);
        SOKO_ASSERT(t->filter == TextureFilter_Trilinear);

        const m4x4 capProj = PerspectiveOpenGLRH(90.0f, 1.0f, 0.1, 10.0f);
        const m4x4 capViews[] =
            {
                LookAtRH(V3(0.0f), V3(1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f)),
                LookAtRH(V3(0.0f), V3(-1.0f, 0.0f, 0.0f), V3(0.0f, -1.0f, 0.0f)),
                LookAtRH(V3(0.0f), V3(0.0f, 1.0f, 0.0f), V3(0.0f, 0.0f, 1.0f)),
                LookAtRH(V3(0.0f), V3(0.0f, -1.0f, 0.0f), V3(0.0f, 0.0f, -1.0f)),
                LookAtRH(V3(0.0f), V3(0.0f, 0.0f, 1.0f), V3(0.0f, -1.0f, 0.0f)),
                LookAtRH(V3(0.0f), V3(0.0f, 0.0f, -1.0f), V3(0.0f, -1.0f, 0.0f)),
            };

        auto prog = &renderer->envMapPrefilterProgram;
        glUseProgram(prog->handle);
        glUniformMatrix4fv(prog->projLoc, 1, GL_FALSE, capProj.data);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

        glActiveTexture(prog->sourceCubemapSlot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sourceHandle);

        // TODO: There are still visible seams on low mip levels

        for (u32 mipLevel = 0; mipLevel < mipLevels; mipLevel++)
        {
            // TODO: Pull texture size out of imges and put to a cubemap itself
            // all sides should havethe same size
            u32 w = (u32)(t->images[0].width * Pow(0.5f, (f32)mipLevel));
            u32 h = (u32)(t->images[0].height * Pow(0.5f, (f32)mipLevel));

            glViewport(0, 0, w, h);
            f32 roughness = (f32)mipLevel / (f32)(mipLevels - 1);
            glUniform1f(prog->roughnessLoc, roughness);

            for (u32 i = 0; i < 6; i++)
            {
                glUniformMatrix4fv(prog->viewLoc, 1, GL_FALSE, capViews[i].data);
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, t->gpuHandle, mipLevel);
                glClear(GL_COLOR_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    internal void
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
        local_persist f32 lod = 1.0f;
        DEBUG_OVERLAY_SLIDER(lod, 0.0f, 5.0f);

        glUniform1f(prog->lodLoc, lod);
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

                    uptr bufferSize = command->instanceCount * sizeof(RenderCommandPushLineVertex);
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
                    if (data->material.type == MeshMaterial::Legacy)
                    {
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
                        glBindTexture(GL_TEXTURE_2D, data->material.legacy.diffMap->gpuHandle);

                        glActiveTexture(meshProg->specMapSlot);
                        glBindTexture(GL_TEXTURE_2D, data->material.legacy.specMap->gpuHandle);


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
                    }
                    else if (data->material.type == MeshMaterial::PBR)
                    {
                        SOKO_ASSERT(group->irradanceMapHandle);
                        auto* meshProg = &renderer->pbrMeshProgram;
                        glUseProgram(meshProg->handle);

                        //  if (firstMeshShaderInvocation)
                        {
                            // firstMeshShaderInvocation = false;
                            glUniformMatrix4fv(meshProg->viewProjLoc,
                                               1, GL_FALSE, viewProj.data);
                            glUniform3fv(meshProg->dirLightDirLoc, 1, group->dirLight.dir.data);
                            glUniform3fv(meshProg->dirLightColorLoc, 1, group->dirLight.diffuse.data);
                            glUniform3fv(meshProg->viewPosLoc, 1, group->cameraConfig.position.data);
                        }

                        glUniform1f(meshProg->metallicLoc, data->material.pbr.metallic);
                        glUniform1f(meshProg->roughnessLoc, data->material.pbr.roughness);
                        glUniform1f(meshProg->aoLoc, data->material.pbr.ao);

                        glUniformMatrix4fv(meshProg->modelMtxLoc,
                                           1, GL_FALSE, data->transform.data);

                        m4x4 invModel = data->transform;
                        bool inverted = Inverse(&invModel);
                        SOKO_ASSERT(inverted, "");
                        m4x4 transModel = Transpose(&invModel);
                        m3x3 normalMatrix = M3x3(&transModel);

                        glUniformMatrix3fv(meshProg->normalMtxLoc,
                                           1, GL_FALSE, normalMatrix.data);

                        auto* mesh = data->mesh;

                        glActiveTexture(meshProg->albedoSlot);
                        glBindTexture(GL_TEXTURE_2D, data->material.pbr.albedo->gpuHandle);

                        glActiveTexture(meshProg->irradanceMapSlot);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, group->irradanceMapHandle);

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
                    }
                    else
                    {
                        INVALID_CODE_PATH;
                    }
                } break;

                case RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH:
                {
                    //glFrontFace(GL_CCW);
                    auto* chunkProg = &renderer->chunkProgram;
                    glUseProgram(chunkProg->handle);
                    glActiveTexture(chunkProg->atlasSlot);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->tileTexArrayHandle);
                    local_persist f32 bias = -0.1f;
                    DEBUG_OVERLAY_SLIDER(bias, -1.0f, 1.0f);
                    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_LOD_BIAS, bias);


                    if (firstChunkMeshShaderInvocation)
                    {
                        local_persist v4 aoDistrib = {0.75f, 0.825f, 0.9f, 1.0f};
                        //DEBUG_OVERLAY_SLIDER(aoDistrib, 0.0f, 1.0f);

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
