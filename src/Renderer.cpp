#include "Renderer.h"

#include "ShaderManager.cpp"

#include "Frustum.cpp"

namespace soko
{
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
        ShaderInfo shaders;

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

        constant u32 RandomValuesTextureSize = 1024;
        constant u32 NumShadowCascades = 3;
        GLuint shadowMapFramebuffers[NumShadowCascades];
        GLuint shadowMapDepthTarget;
        GLuint shadowMapDebugColorTarget;
        u32 shadowMapRes = 1024;
        GLuint randomValuesTexture;
        b32 stableShadows = true;
        b32 showShadowCascadesBoundaries;

        // TODO: Camera should cares about that
        m4x4 shadowCascadeProjections[NumShadowCascades];

        f32 shadowConstantBias;
        f32 shadowSlopeBiasScale;
        f32 shadowNormalBiasScale;

        GLuint srgbBufferHandle;
        GLuint srgbColorTarget;

        GLfloat maxAnisotropy;

        GLuint captureFramebuffer;

        GLuint BRDFLutHandle;

        b32 debugF;
        b32 debugD;
        b32 debugG;
        b32 debugNormals;
    };
#if 0
    GLuint _CreateTexture2D(GLenum target, GLenum wrapS, GLenum wrapT, GLenum minFilter, GLenum magFilter)
    {
        GLuint handle = 0;
        glGenTextures(1, &handle);
        if (handle)
        {
            glBindTexture(target, handle);
            defer { glBindTexture(target, 0); };

            glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapS);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapT);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
        }
        return handle;
    }

    GLuint CreateTexture2D(GLenum wrapS, GLenum wrapT, GLenum minFilter, GLenum magFilter)
    {
        return _CreateTexture2D(GL_TEXTURE_2D, wrapS, wrapT, minFilter, magFilter);
    }

    GLuint CreateTexture2DArray(GLenum wrapS, GLenum wrapT, GLenum minFilter, GLenum magFilter)
    {
        return _CreateTexture2D(GL_TEXTURE_2D_ARRAY, wrapS, wrapT, minFilter, magFilter);
    }
#endif
    GLenum GetGLTextureFormatFromInternalFormat(GLenum internalFormat)
    {
        GLenum result = 0;
        switch (internalFormat)
        {
        case GL_SRGB8_ALPHA8: { result = GL_RGBA; } break;
        case GL_SRGB8: { result = GL_RGB; } break;
        case GL_RGB8: { result = GL_RGB; } break;
        case GL_RGB16F: { result = GL_RGB; } break;
        case GL_RG16F: { result = GL_RG; } break;
        case GL_R8: { result = GL_RED; } break;
        case GL_DEPTH_COMPONENT32F: { result = GL_DEPTH_COMPONENT; } break;
        case GL_DEPTH_COMPONENT32: { result = GL_DEPTH_COMPONENT; } break;
        case GL_DEPTH_COMPONENT24: { result = GL_DEPTH_COMPONENT; } break;
        case GL_DEPTH_COMPONENT16: { result = GL_DEPTH_COMPONENT; } break;
            INVALID_DEFAULT();
        }
        return result;
    }

    GLenum GetGLTextureTypeForInternalFormat(GLenum internalFormat)
    {
        GLenum type = 0;
        switch (internalFormat)
        {
        case GL_SRGB8_ALPHA8: { type = GL_UNSIGNED_BYTE; } break;
        case GL_SRGB8: { type = GL_UNSIGNED_BYTE; } break;
        case GL_RGB8: { type = GL_UNSIGNED_BYTE; } break;
        case GL_RGB16F: { type = GL_FLOAT; } break;
        case GL_RG16F: { type = GL_FLOAT; } break;
        case GL_R8: { type = GL_UNSIGNED_BYTE; } break;
        case GL_DEPTH_COMPONENT32F: { type = GL_FLOAT; } break;
        case GL_DEPTH_COMPONENT32: { type = GL_FLOAT; } break;
        case GL_DEPTH_COMPONENT24: { type = GL_FLOAT; } break;
        case GL_DEPTH_COMPONENT16: { type = GL_FLOAT; } break;
            INVALID_DEFAULT();
        }
        return type;
    }

    void AllocTexture2D(GLuint handle, u32x width, u32x height, u32x mipLevel, GLenum format, void* data = 0, GLenum customType = 0)
    {
        GLenum secondaryFormat = GetGLTextureFormatFromInternalFormat(format);
        GLenum type;
        if (customType)
        {
            type = customType;
        }
        else
        {
            type = GetGLTextureTypeForInternalFormat(format);
        }

        glBindTexture(GL_TEXTURE_2D, handle);
        defer { glBindTexture(GL_TEXTURE_2D, 0); };

        glTexImage2D(GL_TEXTURE_2D, mipLevel, format, width, height, 0, secondaryFormat, type, data);
    }

    // TODO: Refactor these
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
                GLenum minFilter = GL_NEAREST;
                GLenum magFilter = GL_NEAREST;
                bool anisotropic = false;
                GLenum wrapMode = texture->wrapMode;

                switch (texture->format)
                {
                case GL_SRGB8_ALPHA8: { format = GL_RGBA; type = GL_UNSIGNED_BYTE; } break;
                case GL_SRGB8: { format = GL_RGB; type = GL_UNSIGNED_BYTE; } break;
                case GL_RGB8: { format = GL_RGB; type = GL_UNSIGNED_BYTE; } break;
                case GL_RGB16F: { format = GL_RGB; type = GL_FLOAT; } break;
                case GL_RG16F: { format = GL_RG, type = GL_FLOAT; } break;
                case GL_R8: {format = GL_RED, type = GL_UNSIGNED_BYTE; } break;
                    INVALID_DEFAULT_CASE;
                }

                switch (texture->filter)
                {
                case TextureFilter_Bilinear: { minFilter = GL_LINEAR; magFilter = GL_LINEAR; } break;
                case TextureFilter_Trilinear: { minFilter = GL_LINEAR_MIPMAP_LINEAR; magFilter = GL_LINEAR; } break;
                case TextureFilter_Anisotropic: {minFilter = GL_LINEAR_MIPMAP_LINEAR; magFilter = GL_LINEAR; anisotropic = true; } break;
                    INVALID_DEFAULT_CASE;
                }

                glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->width,
                             texture->height, 0, format, type, texture->data);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
                if (anisotropic)
                {
                    // TODO: Anisotropy value
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
                }

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
                // TODO: this is redundant. Use only vertexCount
                uptr normalsSize= mesh->normalCount * sizeof(v3);
                uptr uvsSize = mesh->uvCount * sizeof(v2);
                uptr tangentsSize = mesh->vertexCount * sizeof(v3);
                uptr indexBufferSize = mesh->indexCount * sizeof(u32);
                uptr vertexBufferSize = verticesSize + normalsSize + uvsSize + tangentsSize;

                glBindBuffer(GL_ARRAY_BUFFER, vboHandle);

                glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, 0, GL_STATIC_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, verticesSize, (void*)mesh->vertices);
                glBufferSubData(GL_ARRAY_BUFFER, verticesSize, normalsSize, (void*)mesh->normals);
                glBufferSubData(GL_ARRAY_BUFFER, verticesSize + normalsSize, uvsSize, (void*)mesh->uvs);
                glBufferSubData(GL_ARRAY_BUFFER, verticesSize + normalsSize + uvsSize, tangentsSize, (void*)mesh->tangents);

                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboHandle);

                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, (void*)mesh->indices, GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                mesh->gpuVertexBufferHandle = vboHandle;
                mesh->gpuIndexBufferHandle = iboHandle;
            }
        }
    }

    internal CubeTexture
    LoadCubemap(AB::MemoryArena* tempArena,
                const char* back, const char* down, const char* front,
                const char* left, const char* right, const char* up)
    {
        CubeTexture texture = {};

        auto tempMem = BeginTemporaryMemory(tempArena);

        i32 backWidth, backHeight, backBpp;
        unsigned char* backData = stbi_load(back, &backWidth, &backHeight, &backBpp, 3);

        i32 downWidth, downHeight, downBpp;
        unsigned char* downData = stbi_load(down, &downWidth, &downHeight, &downBpp, 3);

        i32 frontWidth, frontHeight, frontBpp;
        unsigned char* frontData = stbi_load(front, &frontWidth, &frontHeight, &frontBpp, 3);

        i32 leftWidth, leftHeight, leftBpp;
        unsigned char* leftData = stbi_load(left, &leftWidth, &leftHeight, &leftBpp, 3);

        i32 rightWidth, rightHeight, rightBpp;
        unsigned char* rightData = stbi_load(right, &rightWidth, &rightHeight, &rightBpp, 3);

        i32 upWidth, upHeight, upBpp;
        unsigned char* upData = stbi_load(up, &upWidth, &upHeight, &upBpp, 3);

        texture.back.format = GL_SRGB8;
        texture.back.width = backWidth;
        texture.back.height = backHeight;
        texture.back.data = backData;

        texture.down.format = GL_SRGB8;
        texture.down.width = downWidth;
        texture.down.height = downHeight;
        texture.down.data = downData;

        texture.front.format = GL_SRGB8;
        texture.front.width = frontWidth;
        texture.front.height = frontHeight;
        texture.front.data = frontData;

        texture.left.format = GL_SRGB8;
        texture.left.width = leftWidth;
        texture.left.height = leftHeight;
        texture.left.data = leftData;

        texture.right.format = GL_SRGB8;
        texture.right.width = rightWidth;
        texture.right.height = rightHeight;
        texture.right.data = rightData;

        texture.up.format = GL_SRGB8;
        texture.up.width = upWidth;
        texture.up.height = upHeight;
        texture.up.data = upData;

        RendererLoadCubeTexture(&texture);
        SOKO_ASSERT(texture.gpuHandle);

        EndTemporaryMemory(&tempMem);

        return texture;
    }

    internal CubeTexture
    MakeEmptyCubemap(int w, int h, GLenum format, TextureFilter filter = TextureFilter_Bilinear, bool useMips = false)
    {
        CubeTexture texture = {};
        texture.useMips = useMips;
        texture.filter = filter;
        for (u32 i = 0; i < 6; i++)
        {
            texture.images[i].format = format;
            texture.images[i].width = w;
            texture.images[i].height = h;
            texture.images[i].data = 0;
        }
        RendererLoadCubeTexture(&texture);
        SOKO_ASSERT(texture.gpuHandle);
        return texture;
    }

    internal CubeTexture
    LoadCubemapHDR(AB::MemoryArena* tempArena,
                   const char* back, const char* down, const char* front,
                   const char* left, const char* right, const char* up)
    {
        CubeTexture texture = {};

        auto tempMem = BeginTemporaryMemory(tempArena);

        i32 backWidth, backHeight, backBpp;
        f32* backData = stbi_loadf(back, &backWidth, &backHeight, &backBpp, 3);

        i32 downWidth, downHeight, downBpp;
        f32* downData = stbi_loadf(down, &downWidth, &downHeight, &downBpp, 3);

        i32 frontWidth, frontHeight, frontBpp;
        f32* frontData = stbi_loadf(front, &frontWidth, &frontHeight, &frontBpp, 3);

        i32 leftWidth, leftHeight, leftBpp;
        f32* leftData = stbi_loadf(left, &leftWidth, &leftHeight, &leftBpp, 3);

        i32 rightWidth, rightHeight, rightBpp;
        f32* rightData = stbi_loadf(right, &rightWidth, &rightHeight, &rightBpp, 3);

        i32 upWidth, upHeight, upBpp;
        f32* upData = stbi_loadf(up, &upWidth, &upHeight, &upBpp, 3);

        texture.useMips = true;
        texture.filter = TextureFilter_Trilinear;

        texture.back.format = GL_RGB16F;
        texture.back.width = backWidth;
        texture.back.height = backHeight;
        texture.back.data = backData;

        texture.down.format = GL_RGB16F;
        texture.down.width = downWidth;
        texture.down.height = downHeight;
        texture.down.data = downData;

        texture.front.format = GL_RGB16F;
        texture.front.width = frontWidth;
        texture.front.height = frontHeight;
        texture.front.data = frontData;

        texture.left.format = GL_RGB16F;
        texture.left.width = leftWidth;
        texture.left.height = leftHeight;
        texture.left.data = leftData;

        texture.right.format = GL_RGB16F;
        texture.right.width = rightWidth;
        texture.right.height = rightHeight;
        texture.right.data = rightData;

        texture.up.format = GL_RGB16F;
        texture.up.width = upWidth;
        texture.up.height = upHeight;
        texture.up.data = upData;

        RendererLoadCubeTexture(&texture);
        SOKO_ASSERT(texture.gpuHandle);

        EndTemporaryMemory(&tempMem);

        return texture;
    }

    internal Texture
    LoadTexture(AB::MemoryArena* tempArena, const char* filename,
                GLenum format = 0,
                GLenum wrapMode = GL_REPEAT,
                TextureFilter filter = TextureFilter_Bilinear)
    {
        Texture t = {};
        auto tempMem = BeginTemporaryMemory(tempArena);

        i32 width;
        i32 height;
        i32 bpp;
        i32 desiredBpp = 0;

        if (format)
        {
            switch (format)
            {
            case GL_SRGB8_ALPHA8:
            case GL_RGBA: { desiredBpp = 4; } break;
            case GL_SRGB8:
            case GL_RGB8:
            case GL_RGB16F: { desiredBpp = 3; } break;
            case GL_RG16F: { desiredBpp = 2; } break;
            case GL_R8: { desiredBpp = 1; } break;
            }
        }

        unsigned char* diffBitmap = stbi_load(filename, &width, &height, &bpp, desiredBpp);

        if (!format)
        {
            switch (bpp)
            {
            case 1: { format = GL_R8; } break;
            case 2: { format = GL_RG8; } break; // TODO: Implement in renderer
            case 3: { format = GL_RGB8; } break;
            case 4: { format = GL_SRGB8_ALPHA8; } break;
                INVALID_DEFAULT_CASE;
            }
        }

        t.format = format;
        t.width = width;
        t.height = height;
        t.wrapMode = wrapMode;
        t.filter = filter;

        t.data = diffBitmap;
        RendererLoadTexture(&t);
        SOKO_ASSERT(t.gpuHandle);

        EndTemporaryMemory(&tempMem);

        return t;
    }

    internal Material
    LoadMaterialLegacy(AB::MemoryArena* tempArena,
                       const char* diffusePath,
                       const char* specularPath = 0)
    {
        Texture diffMap = LoadTexture(tempArena, diffusePath, GL_SRGB8, GL_REPEAT, TextureFilter_Anisotropic);
        Texture specMap = {};
        if (specularPath)
        {
            Texture specMap = LoadTexture(tempArena, specularPath, GL_SRGB8, GL_REPEAT, TextureFilter_Anisotropic);
        }

        Material material = {};
        material.type = Material::Legacy;
        material.legacy.diffMap = diffMap;
        material.legacy.specMap = specMap;

        return material;
    }

    internal Material
    LoadMaterialPBR(AB::MemoryArena* tempArena,
                    const char* albedoPath,
                    const char* roughnessPath,
                    const char* metalnessPath,
                    const char* normalsPath)
    {
        Texture albedo = LoadTexture(tempArena, albedoPath, GL_SRGB8, GL_REPEAT, TextureFilter_Anisotropic);
        Texture roughness = LoadTexture(tempArena, roughnessPath, 0, GL_REPEAT, TextureFilter_Anisotropic);
        Texture metalness = LoadTexture(tempArena, metalnessPath, 0, GL_REPEAT, TextureFilter_Anisotropic);
        Texture normals = LoadTexture(tempArena, normalsPath, GL_RGB8, GL_REPEAT, TextureFilter_Anisotropic);


        Material material = {};
        material.type = Material::PBR;
        material.pbr.isCustom = false;
        material.pbr.map.albedo = albedo;
        material.pbr.map.roughness = roughness;
        material.pbr.map.metalness = metalness;
        material.pbr.map.normals = normals;

        return material;
    }

    internal Texture
    LoadTexture(i32 width, i32 height, void* data = 0,
                GLenum format = GL_SRGB8,
                GLenum wrapMode = GL_REPEAT,
                TextureFilter filter = TextureFilter_Bilinear)
    {
        Texture t = {};

        t.format = format;
        t.width = width;
        t.height = height;
        t.filter = filter;
        t.wrapMode = wrapMode;
        t.data = data;

        RendererLoadTexture(&t);
        SOKO_ASSERT(t.gpuHandle);

        return t;
    }

        internal Mesh
    LoadMesh(AB::MemoryArena* tempArena, const wchar_t* filepath)
    {
        Mesh mesh = {};
        u32 fileSize = DebugGetFileSize(filepath);
        if (fileSize)
        {
            void* fileData = PUSH_SIZE(tempArena, fileSize);
            u32 result = DebugReadFile(fileData, fileSize, filepath);
            if (result)
            {
                // NOTE: Strict aliasing
                auto header = (AB::AABMeshHeaderV2*)fileData;
                SOKO_ASSERT(header->magicValue == AB::AAB_FILE_MAGIC_VALUE);

                mesh.vertexCount = header->vertexCount;
                mesh.normalCount = header->vertexCount;
                mesh.uvCount = header->vertexCount;
                mesh.indexCount = header->indexCount;

                mesh.vertices = (v3*)((byte*)fileData + header->vertexOffset);
                mesh.normals = (v3*)((byte*)fileData + header->normalsOffset);
                mesh.uvs = (v2*)((byte*)fileData + header->uvOffset);
                mesh.indices = (u32*)((byte*)fileData + header->indicesOffset);
                mesh.tangents = (v3*)((byte*)fileData + header->tangentsOffset);

                RendererLoadMesh(&mesh);
                SOKO_ASSERT(mesh.gpuVertexBufferHandle);
                SOKO_ASSERT(mesh.gpuIndexBufferHandle);
            }
        }

        return mesh;
    }

    // NOTE: Diffise only
    internal Material
    LoadMaterialLegacy(i32 width, i32 height, void* bitmap)
    {
        Texture diffMap = LoadTexture(width, height, bitmap, GL_SRGB8, GL_REPEAT, TextureFilter_Anisotropic);

        Material material = {};
        material.type = Material::Legacy;
        material.legacy.diffMap = diffMap;

        return material;
    }

    internal void
    GenBRDFLut(const Renderer* renderer, Texture* t)
    {
        SOKO_ASSERT(t->gpuHandle);
        SOKO_ASSERT(t->wrapMode == GL_CLAMP_TO_EDGE);
        SOKO_ASSERT(t->filter == TextureFilter_Bilinear);
        SOKO_ASSERT(t->format = GL_RGB16F);

        auto prog = &renderer->shaders.BRDFIntegrator;
        glUseProgram(prog->handle);

        glViewport(0, 0, t->width, t->height);

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, t->gpuHandle, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glFlush();
    }

    internal Renderer*
    AllocAndInitRenderer(AB::MemoryArena* arena, AB::MemoryArena* tempArena, uv2 renderRes)
    {
        Renderer* renderer = nullptr;
        renderer = PUSH_STRUCT(arena, Renderer);
        SOKO_ASSERT(renderer);
        *renderer = {};

        renderer->shaders = LoadShaders();

        GLfloat maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        renderer->maxAnisotropy = maxAnisotropy;

        renderer->gamma = 2.4f;
        renderer->exposure = 1.0f;
        renderer->renderRes = renderRes;

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
        auto tempMem = BeginTemporaryMemory(tempArena);
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

        EndTemporaryMemory(&tempMem);
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

        // NOTE: Init shadow framebuffer
        { // Initializing depth targets
            glGenTextures(1, &renderer->shadowMapDepthTarget);
            glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->shadowMapDepthTarget);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32, renderer->shadowMapRes, renderer->shadowMapRes, Renderer::NumShadowCascades, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        }
        { // Initializing debug color targets
            glGenTextures(1, &renderer->shadowMapDebugColorTarget);
            glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->shadowMapDebugColorTarget);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, renderer->shadowMapRes, renderer->shadowMapRes, Renderer::NumShadowCascades, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        glGenFramebuffers(3, renderer->shadowMapFramebuffers);
        // TODO: Checkin!
        SOKO_ASSERT(renderer->shadowMapFramebuffers[0]);

        for (u32x i = 0; i < Renderer::NumShadowCascades; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, renderer->shadowMapFramebuffers[i]);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, renderer->shadowMapDepthTarget, 0, i);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderer->shadowMapDebugColorTarget, 0, i);
            //glDrawBuffer(GL_NONE);
            //glReadBuffer(GL_NONE);
            SOKO_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glGenTextures(1, &renderer->randomValuesTexture);
        SOKO_ASSERT(renderer->randomValuesTexture);
        {
            glBindTexture(GL_TEXTURE_1D, renderer->randomValuesTexture);
            defer { glBindTexture(GL_TEXTURE_1D, 0); };

            auto tempMem = BeginTemporaryMemory(tempArena);
            defer { EndTemporaryMemory(&tempMem); };

            u8* randomTextureBuffer = (u8*)PUSH_SIZE(tempArena, sizeof(u8) * Renderer::RandomValuesTextureSize);
            RandomSeries series = {};
            for (u32x i = 0; i < Renderer::RandomValuesTextureSize; i++)
            {
                randomTextureBuffer[i] = (u8)(RandomUnilateral(&series) * 255.0f);
            }
            glTexImage1D(GL_TEXTURE_1D, 0, GL_R8, Renderer::RandomValuesTextureSize, 0, GL_RED, GL_UNSIGNED_BYTE, randomTextureBuffer);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }


        u32 brdfLUTSize = DebugGetFileSize(L"brdf_lut.aab");
        if (brdfLUTSize)
        {
            SOKO_ASSERT(brdfLUTSize == sizeof(f32) * 2 * 512 * 512);
            auto tempMem = BeginTemporaryMemory(tempArena);
            void* brdfBitmap = PUSH_SIZE(tempArena, brdfLUTSize);
            auto loadedSize = DebugReadFile(brdfBitmap, brdfLUTSize, L"brdf_lut.aab");
            EndTemporaryMemory(&tempMem);
            if (loadedSize == brdfLUTSize)
            {
                Texture t = LoadTexture(512, 512, brdfBitmap, GL_RG16F, GL_CLAMP_TO_EDGE, TextureFilter_Bilinear);
                if (t.gpuHandle)
                {
                    renderer->BRDFLutHandle = t.gpuHandle;
                }
            }
        }
        if (!renderer->BRDFLutHandle)
        {
            Texture t = LoadTexture(512, 512, 0, GL_RG16F, GL_CLAMP_TO_EDGE, TextureFilter_Bilinear);
            SOKO_ASSERT(t.gpuHandle);
            GenBRDFLut(renderer, &t);
            renderer->BRDFLutHandle = t.gpuHandle;
            auto tempMem = BeginTemporaryMemory(tempArena);
            void* bitmap = PUSH_SIZE(tempArena, sizeof(f32) * 2 * 512 * 512);
            glBindTexture(GL_TEXTURE_2D, renderer->BRDFLutHandle);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, bitmap);
            DebugWriteFile(L"brdf_lut.aab", bitmap, sizeof(f32) * 2 * 512 * 512);
            EndTemporaryMemory(&tempMem);
        }

        return renderer;
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
    RendererBeginFrame(Renderer* renderer, v2 viewportDim)
    {
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
        auto prog = &renderer->shaders.PostFX;
        glUseProgram(prog->handle);
        glUniform1f(prog->fragment.uniforms.u_Gamma, renderer->gamma);
        glUniform1f(prog->fragment.uniforms.u_Exposure, renderer->exposure);

        glActiveTexture(prog->fragment.samplers.u_ColorSourceLinear.slot);
        glBindTexture(GL_TEXTURE_2D, renderer->offscreenColorTarget);
        //glBindTexture(GL_TEXTURE_2D, renderer->shadowMapDepthTarget);

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

            auto fxaaProg = &renderer->shaders.FXAA;
            glUseProgram(fxaaProg->handle);
            v2 invScreenSize = V2(1.0f / renderer->renderRes.x, 1.0f / renderer->renderRes.y);
            glUniform2fv(fxaaProg->fragment.uniforms.u_InvScreenSize, 1, invScreenSize.data);

            glActiveTexture(fxaaProg->fragment.samplers.u_ColorSourcePerceptual.slot);
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

        local_persist i32 showShadowMap = false;
        DEBUG_OVERLAY_SLIDER(showShadowMap, 0, 1);

        if (showShadowMap)
        {
            local_persist i32 shadowCascadeLevel = 0;
            DEBUG_OVERLAY_SLIDER(shadowCascadeLevel, 0, Renderer::NumShadowCascades - 1);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->shadowMapFramebuffers[shadowCascadeLevel]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

            glBlitFramebuffer(0, 0, renderer->shadowMapRes, renderer->shadowMapRes,
                              0, 0, 512, 512, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
    }

    internal void
    GenIrradanceMap(const Renderer* renderer, CubeTexture* t, GLuint sourceHandle)
    {
        SOKO_ASSERT(t->gpuHandle);
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

        auto prog = &renderer->shaders.IrradanceConvolver;
        glUseProgram(prog->handle);
        glUniformMatrix4fv(prog->vertex.uniforms.u_ProjMatrix, 1, GL_FALSE, capProj.data);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

        glActiveTexture(prog->fragment.samplers.uSourceCubemap.slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, sourceHandle);

        for (u32 i = 0; i < 6; i++)
        {
            glViewport(0, 0, t->images[i].width, t->images[i].height);
            glUniformMatrix4fv(prog->vertex.uniforms.u_ViewMatrix, 1, GL_FALSE, capViews[i].data);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, t->gpuHandle, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glFlush();
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

        auto prog = &renderer->shaders.EnvMapPrefilter;
        glUseProgram(prog->handle);
        glUniformMatrix4fv(prog->vertex.uniforms.u_ProjMatrix, 1, GL_FALSE, capProj.data);
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->captureFramebuffer);

        SOKO_ASSERT(t->images[0].width == t->images[0].height);
        glUniform1i(prog->fragment.uniforms.uResolution, t->images[0].width);

        glActiveTexture(prog->fragment.samplers.uSourceCubemap.slot);
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
            glUniform1f(prog->fragment.uniforms.uRoughness, roughness);

            for (u32 i = 0; i < 6; i++)
            {
                glUniformMatrix4fv(prog->vertex.uniforms.u_ViewMatrix, 1, GL_FALSE, capViews[i].data);
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, t->gpuHandle, mipLevel);
                glClear(GL_COLOR_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

        }
        glFlush();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    internal void
    DrawSkybox(Renderer* renderer, RenderGroup* group,
               const m4x4* view, const m4x4* proj)
    {
        //glDisable(GL_DEPTH_TEST);
        //glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        auto prog = &renderer->shaders.Skybox;
        glUseProgram(prog->handle);

        glUniformMatrix4fv(prog->vertex.uniforms.u_ViewMatrix, 1, GL_FALSE, view->data);
        glUniformMatrix4fv(prog->vertex.uniforms.u_ProjMatrix, 1, GL_FALSE, proj->data);
        local_persist f32 lod = 1.0f;
        DEBUG_OVERLAY_SLIDER(lod, 0.0f, 5.0f);

        glUniform1f(prog->fragment.uniforms.uLod, lod);
        glActiveTexture(prog->fragment.samplers.u_CubeTexture.slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, group->skyboxHandle);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDepthMask(GL_TRUE);
    }

    m3x3 MakeNormalMatrix(m4x4 model)
    {
        bool inverted = Inverse(&model);
        SOKO_ASSERT(inverted);
        model = Transpose(&model);
        m3x3 normal = M3x3(&model);
        return normal;
    }

    // TODO: @Speed Make shure this function inlined
    m4x4 CalcShadowProjection(CameraConfig camera, m4x4 lightLookAt, m4x4 cameraLookAtInv, u32 shadowMapRes, bool stable)
    {
        // NOTE Shadow projection bounds
        v3 min;
        v3 max;

        if (stable)
        {
            v4 bSphereP;
            f32 bSphereR;

            // NOTE: Computing frustum bounding sphere
            // Reference: https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
            f32 ar = 1.0f / camera.aspectRatio;
            f32 k = Sqrt(1.0f + ar * ar) * Tan(ToRadians(camera.fovDeg) * 0.5f);
            f32 kSq = k * k;
            f32 f = camera.farPlane;
            f32 n = camera.nearPlane;
            if (kSq >= ((f - n) / (f + n)))
            {
                bSphereP = V4(0.0f, 0.0f, -f, 1.0f);
                bSphereR = f * k;
            }
            else
            {
                bSphereP = V4(0.0f, 0.0f, -0.5f * (f + n) * (1.0f + kSq), 1.0f);
                bSphereR = 0.5f * Sqrt((f - n) * (f - n) + 2.0f * (f * f + n * n) * kSq + (f + n) * (f + n) * kSq * kSq);
            }

            // From camera space to world space
            bSphereP = cameraLookAtInv * bSphereP;
            // From world space to light space
            bSphereP = lightLookAt * bSphereP;

            // Constructing AABB in light space
            v3 xAxis = V3(1.0f, 0.0f, 0.0f);
            v3 yAxis = V3(0.0f, 1.0f, 0.0f);
            v3 zAxis = V3(0.0f, 0.0f, 1.0f);
            min = bSphereP.xyz - (xAxis * bSphereR + yAxis * bSphereR + zAxis * bSphereR);
            max = bSphereP.xyz + (xAxis * bSphereR + yAxis * bSphereR + zAxis * bSphereR);

            // We get min and max Z values in light space, so Z is negative forward therefore if sphere
            // is in front of the light source then we have negative values.
            // Clamp max Z to 0 because if it's positive, then we are behind the light source.
            if (max.z > 0.0f) max.z = 0.0f;
            // Inverting Z sign because we will use it for building projection matrix.
            // So it describe distance to far plane.
            min.z = -min.z;
            max.z = -max.z;
            // Swapping values so we can use min as near plane distance and max for far plane
            auto tmp = min.z;
            min.z = max.z;
            max.z = tmp;

            // TODO: On a particular camera angle there are still
            // shimmering on shadows
            auto bboxSideSize = Abs(min.x) + Abs(max.x);
            auto pixelSize = bboxSideSize / shadowMapRes;

            min.x = Round(min.x / pixelSize) * pixelSize;
            min.y = Round(min.y / pixelSize) * pixelSize;
            min.z = Round(min.z / pixelSize) * pixelSize;

            max.x = Round(max.x / pixelSize) * pixelSize;
            max.y = Round(max.y / pixelSize) * pixelSize;
            max.z = Round(max.z / pixelSize) * pixelSize;
        }
        else
        {
            Basis cameraBasis;
            cameraBasis.zAxis = Normalize(camera.front);
            cameraBasis.xAxis = Normalize(Cross(V3(0.0f, 1.0f, 0.0), cameraBasis.zAxis));
            cameraBasis.yAxis = Cross(cameraBasis.zAxis, cameraBasis.xAxis);
            cameraBasis.p = camera.position;

            auto camFrustumCorners = GetFrustumCorners(cameraBasis, camera.fovDeg, camera.aspectRatio, camera.nearPlane, camera.farPlane);
            for (u32x i = 0; i < ArrayCount(camFrustumCorners.corners); i++)
            {
                camFrustumCorners.corners[i] = (lightLookAt * V4(camFrustumCorners.corners[i], 1.0f)).xyz;
            }

            min = V3(F32_MAX);
            max = V3(-F32_MAX);

            for (u32x i = 0; i < ArrayCount(camFrustumCorners.corners); i++)
            {
                auto corner = camFrustumCorners.corners[i];
                if (corner.x < min.x) min.x = corner.x;
                if (corner.x > max.x) max.x = corner.x;
                if (corner.y < min.y) min.y = corner.y;
                if (corner.y > max.y) max.y = corner.y;
                if (corner.z < min.z) min.z = corner.z;
                if (corner.z > max.z) max.z = corner.z;
            }

            v3 minViewSpace = min;
            v3 maxViewSpace = max;

            // NOTE: Inverting Z because right-handed Z is negative-forward
            // but ortho ptojections gets constructed from Z positive-far
            auto tmp = minViewSpace.z;
            minViewSpace.z = -maxViewSpace.z;
            maxViewSpace.z = -tmp;

            min = minViewSpace;
            max = maxViewSpace;
        }

        m4x4 projection = OrthogonalOpenGLRH(min.x, max.x, min.y, max.y, min.z, max.z);
        return projection;
    }

    void RenderShadowMap(Renderer* renderer, RenderGroup* group)
    {
        if (group->commandQueueAt)
        {
            auto shader = &renderer->shaders.Shadow;
            for (u32 i = 0; i < group->commandQueueAt; i++)
            {
                CommandQueueEntry* command = group->commandQueue + i;

                switch (command->type)
                {
                case RENDER_COMMAND_DRAW_LINE_BEGIN:
                {
                } break;
                case RENDER_COMMAND_DRAW_MESH:
                {
                    auto* data = (RenderCommandDrawMesh*)(group->renderBuffer + command->rbOffset);

                    glUniformMatrix4fv(shader->vertex.uniforms.ModelMatrix, 1, GL_FALSE, data->transform.data);

                    auto normalMatrix = MakeNormalMatrix(data->transform);
                    glUniformMatrix3fv(shader->vertex.uniforms.NormalMatrix, 1, GL_FALSE, normalMatrix.data);

                    auto* mesh = data->mesh;

                    glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);

                    auto posAttrLoc = shader->vertex.vertexAttribs.Position;
                    glEnableVertexAttribArray(posAttrLoc);
                    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpuIndexBufferHandle);
                    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
                } break;

                case RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH:
                {
                    for (u32 i = 0; i < command->instanceCount; i++)
                    {
                        auto* data = ((RenderCommandPushChunkMesh*)(group->renderBuffer + command->rbOffset)) + i;

                        m4x4 world = Translation(data->offset);
                        glUniformMatrix4fv(shader->vertex.uniforms.ModelMatrix, 1, GL_FALSE, world.data);

                        auto normalMatrix = MakeNormalMatrix(world);
                        glUniformMatrix3fv(shader->vertex.uniforms.NormalMatrix, 1, GL_FALSE, normalMatrix.data);

                        glBindBuffer(GL_ARRAY_BUFFER, data->meshIndex);

                        GLsizei stride = sizeof(ChunkMeshVertex);
                        auto posAttrLoc = shader->vertex.vertexAttribs.Position;
                        auto normalAttrLoc = shader->vertex.vertexAttribs.Normal;
                        glEnableVertexAttribArray(posAttrLoc);
                        glEnableVertexAttribArray(normalAttrLoc);
                        glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                        glVertexAttribPointer(normalAttrLoc, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(v3)));

                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->chunkIndexBuffer);

                        GLsizei numIndices = (GLsizei)(data->quadCount * RENDERER_INDICES_PER_CHUNK_QUAD);
                        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
                    }
                } break;
                }
            }
        }
    }

    void ShadowPass(Renderer* renderer, RenderGroup* group)
    {
        local_persist f32 slopeBiasScale = 0.0f;
        local_persist f32 constBias = 0.008f;
        local_persist f32 normalBiasScale = 0.0f;

        DEBUG_OVERLAY_SLIDER(slopeBiasScale, 0.0f, 2.5f);
        DEBUG_OVERLAY_SLIDER(constBias, 0.0f, 0.5f);
        DEBUG_OVERLAY_SLIDER(normalBiasScale, 0.0f, 2.0f);

        renderer->shadowConstantBias = constBias;
        renderer->shadowSlopeBiasScale = slopeBiasScale;
        renderer->shadowNormalBiasScale = normalBiasScale;

        i32 EnableStableShadows = renderer->stableShadows;
        DEBUG_OVERLAY_SLIDER(EnableStableShadows, 0, 1);
        renderer->stableShadows = EnableStableShadows;

        auto light = &group->dirLight;
        auto camera = &group->cameraConfig;

        glEnable(GL_POLYGON_OFFSET_FILL);
        defer { glDisable(GL_POLYGON_OFFSET_FILL); };
        glPolygonOffset(renderer->shadowSlopeBiasScale, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, (GLsizei)renderer->shadowMapRes, (GLsizei)renderer->shadowMapRes);

        auto shader = &renderer->shaders.Shadow;
        glUseProgram(shader->handle);
        glUniform1f(shader->vertex.uniforms.ShadowNormalBiasScale, renderer->shadowNormalBiasScale);
        glUniform1f(shader->fragment.uniforms.ConstantShadowBias, renderer->shadowConstantBias);
        glUniform3fv(shader->vertex.uniforms.LightPos, 1, light->from.data);

        // TODO: Fix the mess with lookAt matrices4
        m4x4 lightLookAt = LookAtDirRH(light->from, light->dir, V3(0.0f, 1.0f, 0.0f));
        m4x4 cameraLookAt = LookAtDirRH(camera->position, camera->front, V3(0.0f, 1.0f, 0.0f));
        m4x4 cameraLookAtInv = InverseAndUnwrap(cameraLookAt);

        f32 frustrumDepth = camera->farPlane - camera->nearPlane;
        f32 cascadeDepth = frustrumDepth / Renderer::NumShadowCascades;
        for (u32x cascadeIndex = 0; cascadeIndex < Renderer::NumShadowCascades; cascadeIndex++)
        {
            f32 cascadeNear = cascadeDepth * cascadeIndex;
            f32 cascadeFar = cascadeDepth * (cascadeIndex + 1);
            CameraConfig cascadeCamera = *camera;
            cascadeCamera.nearPlane = cascadeNear;
            cascadeCamera.farPlane = cascadeFar;
            renderer->shadowCascadeProjections[cascadeIndex] = CalcShadowProjection(cascadeCamera, lightLookAt, cameraLookAtInv, renderer->shadowMapRes, renderer->stableShadows);
            auto viewProj = MulM4M4(&renderer->shadowCascadeProjections[cascadeIndex], &lightLookAt);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->shadowMapFramebuffers[cascadeIndex]);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glUniformMatrix4fv(shader->vertex.uniforms.ViewProjMatrix, 1, GL_FALSE, viewProj.data);

            RenderShadowMap(renderer, group);
        }
    }
#if 0
    void ShadowPass(Renderer* renderer, RenderGroup* group)
    {
        if (group->commandQueueAt)
        {
            local_persist f32 slopeBiasScale = 0.0f;
            local_persist f32 constBias = 0.001f;
            local_persist f32 normalBiasScale = 0.0f;

            DEBUG_OVERLAY_SLIDER(slopeBiasScale, 0.0f, 2.5f);
            DEBUG_OVERLAY_SLIDER(constBias, 0.0f, 0.5f);
            DEBUG_OVERLAY_SLIDER(normalBiasScale, 0.0f, 2.0f);

            renderer->shadowConstantBias = constBias;
            renderer->shadowSlopeBiasScale = slopeBiasScale;
            renderer->shadowNormalBiasScale = normalBiasScale;

            auto light = &group->dirLight;
            auto camera = &group->cameraConfig;

            // TODO: Fix the mess with lookAt matrices4
            m4x4 lightLookAt = LookAtDirRH(light->from, light->dir, V3(0.0f, 1.0f, 0.0f));
            m4x4 cameraLookAt = LookAtDirRH(camera->position, camera->front, V3(0.0f, 1.0f, 0.0f));
            m4x4 cameraLookAtInv = InverseAndUnwrap(cameraLookAt);

            // NOTE Shadow projection bounds
            v3 min;
            v3 max;

            {
                i32 EnableStableShadows = renderer->stableShadows;
                DEBUG_OVERLAY_SLIDER(EnableStableShadows, 0, 1);
                renderer->stableShadows = EnableStableShadows;
            }

            if (renderer->stableShadows)
            {
                v4 bSphereP;
                f32 bSphereR;

                // NOTE: Computing frustum bounding sphere
                // Reference: https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
                f32 ar = 1.0f / camera->aspectRatio;
                f32 k = Sqrt(1.0f + ar * ar) * Tan(ToRadians(camera->fovDeg) * 0.5f);
                f32 kSq = k * k;
                f32 f = camera->farPlane;
                f32 n = camera->nearPlane;
                if (kSq >= ((f - n) / (f + n)))
                {
                    bSphereP = V4(0.0f, 0.0f, -f, 1.0f);
                    bSphereR = f * k;
                }
                else
                {
                    bSphereP = V4(0.0f, 0.0f, -0.5f * (f + n) * (1.0f + kSq), 1.0f);
                    bSphereR = 0.5f * Sqrt((f - n) * (f - n) + 2.0f * (f * f + n * n) * kSq + (f + n) * (f + n) * kSq * kSq);
                }

                // From camera space to world space
                bSphereP = cameraLookAtInv * bSphereP;
                // From world space to light space
                bSphereP = lightLookAt * bSphereP;

                // Constructing AABB in light space
                v3 xAxis = V3(1.0f, 0.0f, 0.0f);
                v3 yAxis = V3(0.0f, 1.0f, 0.0f);
                v3 zAxis = V3(0.0f, 0.0f, 1.0f);
                min = bSphereP.xyz - (xAxis * bSphereR + yAxis * bSphereR + zAxis * bSphereR);
                max = bSphereP.xyz + (xAxis * bSphereR + yAxis * bSphereR + zAxis * bSphereR);

                min.z = 5.0f;
                max.z = 50.0f;

                // TODO: On a particular camera cngle there are still
                // shimmering on shadows
                auto bboxSideSize = Abs(min.x) + Abs(max.x);
                auto pixelSize = bboxSideSize / renderer->shadowMapRes;

                min.x = Round(min.x / pixelSize) * pixelSize;
                min.y = Round(min.y / pixelSize) * pixelSize;
                min.z = Round(min.z / pixelSize) * pixelSize;

                max.x = Round(max.x / pixelSize) * pixelSize;
                max.y = Round(max.y / pixelSize) * pixelSize;
                max.z = Round(max.z / pixelSize) * pixelSize;
            }
            else
            {
                Basis cameraBasis;
                cameraBasis.zAxis = Normalize(camera->front);
                cameraBasis.xAxis = Normalize(Cross(V3(0.0f, 1.0f, 0.0), cameraBasis.zAxis));
                cameraBasis.yAxis = Cross(cameraBasis.zAxis, cameraBasis.xAxis);
                cameraBasis.p = camera->position;

                auto camFrustumCorners = GetFrustumCorners(cameraBasis, camera->fovDeg, camera->aspectRatio, camera->nearPlane, camera->farPlane);
                for (u32x i = 0; i < ArrayCount(camFrustumCorners.corners); i++)
                {
                    camFrustumCorners.corners[i] = (lightLookAt * V4(camFrustumCorners.corners[i], 1.0f)).xyz;
                }

                min = V3(F32_MAX);
                max = V3(-F32_MAX);

                for (u32x i = 0; i < ArrayCount(camFrustumCorners.corners); i++)
                {
                    auto corner = camFrustumCorners.corners[i];
                    if (corner.x < min.x) min.x = corner.x;
                    if (corner.x > max.x) max.x = corner.x;
                    if (corner.y < min.y) min.y = corner.y;
                    if (corner.y > max.y) max.y = corner.y;
                    if (corner.z < min.z) min.z = corner.z;
                    if (corner.z > max.z) max.z = corner.z;
                }

                v3 minViewSpace = min;
                v3 maxViewSpace = max;

                // NOTE: Inverting Z because right-handed Z is negative-forward
                // but ortho ptojections gets constructed from Z positive-far
                auto tmp = minViewSpace.z;
                minViewSpace.z = -maxViewSpace.z;
                maxViewSpace.z = -tmp;

                min = minViewSpace;
                max = maxViewSpace;
            }

            renderer->shadowMapBounds.min = min;
            renderer->shadowMapBounds.max = max;

            m4x4 projection = OrthogonalOpenGLRH(min.x, max.x, min.y, max.y, min.z, max.z);
            m4x4 viewProj = MulM4M4(&projection, &lightLookAt);

            glEnable(GL_POLYGON_OFFSET_FILL);
            defer { glDisable(GL_POLYGON_OFFSET_FILL); };
            glPolygonOffset(renderer->shadowSlopeBiasScale, 0.0f);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->shadowMapFramebuffers[0]);
            defer { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); };
            glViewport(0, 0, (GLsizei)renderer->shadowMapRes, (GLsizei)renderer->shadowMapRes);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            auto shader = &renderer->shaders.Shadow;
            glUseProgram(shader->handle);
            defer { glUseProgram(0); };
            glUniformMatrix4fv(shader->vertex.uniforms.ViewProjMatrix, 1, GL_FALSE, viewProj.data);
            glUniform1f(shader->vertex.uniforms.ShadowNormalBiasScale, renderer->shadowNormalBiasScale);
            glUniform1f(shader->fragment.uniforms.ConstantShadowBias, renderer->shadowConstantBias);
            glUniform3fv(shader->vertex.uniforms.LightPos, 1, light->from.data);

            for (u32 i = 0; i < group->commandQueueAt; i++)
            {
                CommandQueueEntry* command = group->commandQueue + i;

                switch (command->type)
                {
                case RENDER_COMMAND_DRAW_LINE_BEGIN:
                {
                } break;
                case RENDER_COMMAND_DRAW_MESH:
                {
                    auto* data = (RenderCommandDrawMesh*)(group->renderBuffer + command->rbOffset);

                    glUniformMatrix4fv(shader->vertex.uniforms.ModelMatrix, 1, GL_FALSE, data->transform.data);

                    auto normalMatrix = MakeNormalMatrix(data->transform);
                    glUniformMatrix3fv(shader->vertex.uniforms.NormalMatrix, 1, GL_FALSE, normalMatrix.data);

                    auto* mesh = data->mesh;

                    glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);

                    auto posAttrLoc = shader->vertex.vertexAttribs.Position;
                    glEnableVertexAttribArray(posAttrLoc);
                    glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gpuIndexBufferHandle);
                    glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
                } break;

                case RENDER_COMMAND_BEGIN_CHUNK_MESH_BATCH:
                {
                    for (u32 i = 0; i < command->instanceCount; i++)
                    {
                        auto* data = ((RenderCommandPushChunkMesh*)(group->renderBuffer + command->rbOffset)) + i;

                        m4x4 world = Translation(data->offset);
                        glUniformMatrix4fv(shader->vertex.uniforms.ModelMatrix, 1, GL_FALSE, world.data);

                        auto normalMatrix = MakeNormalMatrix(world);
                        glUniformMatrix3fv(shader->vertex.uniforms.NormalMatrix, 1, GL_FALSE, normalMatrix.data);

                        glBindBuffer(GL_ARRAY_BUFFER, data->meshIndex);

                        GLsizei stride = sizeof(ChunkMeshVertex);
                        auto posAttrLoc = shader->vertex.vertexAttribs.Position;
                        auto normalAttrLoc = shader->vertex.vertexAttribs.Normal;
                        glEnableVertexAttribArray(posAttrLoc);
                        glEnableVertexAttribArray(normalAttrLoc);
                        glVertexAttribPointer(posAttrLoc, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                        glVertexAttribPointer(normalAttrLoc, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(v3)));

                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->chunkIndexBuffer);

                        GLsizei numIndices = (GLsizei)(data->quadCount * RENDERER_INDICES_PER_CHUNK_QUAD);
                        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
                    }
                } break;
                }
            }
        }
    }
#endif
    internal void
    FlushRenderGroup(Renderer* renderer, RenderGroup* group)
    {
        CameraConfig* camera = &group->cameraConfig;

        bool showShadowCascadesBoundaries = renderer->showShadowCascadesBoundaries;
        DEBUG_OVERLAY_TOGGLE(showShadowCascadesBoundaries);
        renderer->showShadowCascadesBoundaries = showShadowCascadesBoundaries;

        m4x4 lookAt = LookAtDirRH(camera->position, camera->front, V3(0.0f, 1.0f, 0.0f));
        m4x4 projection = PerspectiveOpenGLRH(camera->fovDeg, camera->aspectRatio,
                                              camera->nearPlane, camera->farPlane);
        m4x4 viewProj = MulM4M4(&projection, &lookAt);

        if (group->commandQueueAt)
        {
            auto light = &group->dirLight;
            m4x4 lightLookAt = LookAtRH(light->from, light->from + light->dir, V3(0.0f, 1.0f, 0.0f));
            m4x4 lightViewProj0 = MulM4M4(renderer->shadowCascadeProjections, &lightLookAt);
            m4x4 lightViewProj1 = MulM4M4(renderer->shadowCascadeProjections + 1, &lightLookAt);
            m4x4 lightViewProj2 = MulM4M4(renderer->shadowCascadeProjections + 2, &lightLookAt);


            local_persist f32 shadowFilterScale = 1.0f;
            DEBUG_OVERLAY_SLIDER(shadowFilterScale, 0.1f, 5.0f);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, renderer->offscreenBufferHandle);

            bool firstLineShaderInvocation = true;
            bool firstMeshShaderInvocation = true;
            bool firstChunkMeshShaderInvocation = true;

            glUseProgram(renderer->shaders.PbrMesh.handle);
            glUniform1i(renderer->shaders.PbrMesh.fragment.uniforms.uDebugF, (i32)renderer->debugF);
            glUniform1i(renderer->shaders.PbrMesh.fragment.uniforms.uDebugG, (i32)renderer->debugG);
            glUniform1i(renderer->shaders.PbrMesh.fragment.uniforms.uDebugD, (i32)renderer->debugD);
            glUniform1i(renderer->shaders.PbrMesh.fragment.uniforms.uDebugNormals, (i32)renderer->debugNormals);

            for (u32 i = 0; i < group->commandQueueAt; i++)
            {
                CommandQueueEntry* command = group->commandQueue + i;

                switch (command->type)
                {
                case RENDER_COMMAND_DRAW_LINE_BEGIN:
                {
                    auto* data = (RenderCommandDrawLineBegin*)(group->renderBuffer + command->rbOffset);

                    glUseProgram(renderer->shaders.Line.handle);
                    // TODO: Store info about command queue contents and set uniforms at the beginning
                    if (firstLineShaderInvocation)
                    {
                        firstLineShaderInvocation = false;
                        glUniformMatrix4fv(renderer->shaders.Line.vertex.uniforms.u_ViewProjMatrix, 1, GL_FALSE, viewProj.data);
                    }

                    glUniform3fv(renderer->shaders.Line.vertex.uniforms.u_Color, 1, data->color.data);

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
                    if (data->material.type == Material::Legacy)
                    {
                        auto* meshProg = &renderer->shaders.Mesh;
                        glUseProgram(meshProg->handle);

                        if (firstMeshShaderInvocation)
                        {
                            firstMeshShaderInvocation = false;
                            glUniformMatrix4fv(meshProg->vertex.uniforms.u_ViewProjMatrix, 1, GL_FALSE, viewProj.data);
                            glUniform3fv(meshProg->fragment.uniforms.u_DirLight.dir, 1, group->dirLight.dir.data);
                            glUniform3fv(meshProg->fragment.uniforms.u_DirLight.ambient, 1, group->dirLight.ambient.data);
                            glUniform3fv(meshProg->fragment.uniforms.u_DirLight.diffuse, 1, group->dirLight.diffuse.data);
                            glUniform3fv(meshProg->fragment.uniforms.u_DirLight.specular, 1, group->dirLight.specular.data);
                            glUniform3fv(meshProg->fragment.uniforms.u_ViewPos, 1, group->cameraConfig.position.data);
                        }

                        glUniformMatrix4fv(meshProg->vertex.uniforms.u_ModelMatrix, 1, GL_FALSE, data->transform.data);

                        m3x3 normalMatrix = MakeNormalMatrix(data->transform);

                        glUniformMatrix3fv(meshProg->vertex.uniforms.u_NormalMatrix, 1, GL_FALSE, normalMatrix.data);

                        auto* mesh = data->mesh;

                        glActiveTexture(meshProg->fragment.samplers.u_DiffMap.slot);
                        glBindTexture(GL_TEXTURE_2D, data->material.legacy.diffMap.gpuHandle);

                        glActiveTexture(meshProg->fragment.samplers.u_SpecMap.slot);
                        glBindTexture(GL_TEXTURE_2D, data->material.legacy.specMap.gpuHandle);


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
                    else if (data->material.type == Material::PBR)
                    {
                        SOKO_ASSERT(group->irradanceMapHandle);
                        auto* meshProg = &renderer->shaders.PbrMesh;
                        glUseProgram(meshProg->handle);

                        //  if (firstMeshShaderInvocation)
                        {
                            // firstMeshShaderInvocation = false;
                            glUniformMatrix4fv(meshProg->vertex.uniforms.uViewProjMatrix, 1, GL_FALSE, viewProj.data);
                            glUniform3fv(meshProg->fragment.uniforms.uDirLight.dir, 1, group->dirLight.dir.data);
                            glUniform3fv(meshProg->fragment.uniforms.uDirLight.color, 1, group->dirLight.diffuse.data);
                            glUniform3fv(meshProg->fragment.uniforms.uViewPos, 1, group->cameraConfig.position.data);
                        }

                        //glUniform1f(meshProg->aoLoc, data->material.pbr.ao);

                        glUniformMatrix4fv(meshProg->vertex.uniforms.uModelMatrix, 1, GL_FALSE, data->transform.data);

                        m4x4 invModel = data->transform;
                        bool inverted = Inverse(&invModel);
                        SOKO_ASSERT(inverted, "");
                        m4x4 transModel = Transpose(&invModel);
                        m3x3 normalMatrix = M3x3(&transModel);

                        glUniformMatrix3fv(meshProg->vertex.uniforms.uNormalMatrix, 1, GL_FALSE, normalMatrix.data);

                        auto* mesh = data->mesh;

                        auto* m = &data->material;

                        if (!m->pbr.isCustom)
                        {
                            glUniform1i(meshProg->fragment.uniforms.uCustomMaterial, 0);

                            glActiveTexture(meshProg->fragment.samplers.uAlbedoMap.slot);
                            glBindTexture(GL_TEXTURE_2D, m->pbr.map.albedo.gpuHandle);

                            glActiveTexture(meshProg->fragment.samplers.uRoughnessMap.slot);
                            glBindTexture(GL_TEXTURE_2D, m->pbr.map.roughness.gpuHandle);

                            glActiveTexture(meshProg->fragment.samplers.uMetalnessMap.slot);
                            glBindTexture(GL_TEXTURE_2D, m->pbr.map.metalness.gpuHandle);

                            glActiveTexture(meshProg->fragment.samplers.uNormalMap.slot);
                            glBindTexture(GL_TEXTURE_2D, m->pbr.map.normals.gpuHandle);
                        }
                        else
                        {
                            glUniform1i(meshProg->fragment.uniforms.uCustomMaterial, 1);
                            glUniform3fv(meshProg->fragment.uniforms.uCustomAlbedo, 1, m->pbr.custom.albedo.data);
                            glUniform1f(meshProg->fragment.uniforms.uCustomRoughness, m->pbr.custom.roughness);
                            glUniform1f(meshProg->fragment.uniforms.uCustomMetalness, m->pbr.custom.metalness);
                        }

                        glActiveTexture(meshProg->fragment.samplers.uIrradanceMap.slot);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, group->irradanceMapHandle);

                        glActiveTexture(meshProg->fragment.samplers.uEnviromentMap.slot);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, group->envMapHandle);

                        glActiveTexture(meshProg->fragment.samplers.uBRDFLut.slot);
                        glBindTexture(GL_TEXTURE_2D, renderer->BRDFLutHandle);

                        glBindBuffer(GL_ARRAY_BUFFER, mesh->gpuVertexBufferHandle);

                        glEnableVertexAttribArray(0);
                        glEnableVertexAttribArray(1);
                        glEnableVertexAttribArray(2);
                        glEnableVertexAttribArray(3);

                        u64 normalsOffset = mesh->vertexCount * sizeof(v3);
                        u64 uvsOffset = normalsOffset + mesh->normalCount * sizeof(v3);
                        u64 tangentsOffset = uvsOffset + mesh->uvCount * sizeof(v2);

                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)normalsOffset);
                        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)uvsOffset);
                        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)tangentsOffset);

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
                    auto* chunkProg = &renderer->shaders.Chunk;
                    glUseProgram(chunkProg->handle);
                    glActiveTexture(chunkProg->fragment.samplers.u_TerrainAtlas.slot);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->tileTexArrayHandle);

                    glActiveTexture(chunkProg->fragment.samplers.u_ShadowMap.slot);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->shadowMapDepthTarget);

                    glActiveTexture(chunkProg->fragment.samplers.randomTexture.slot);
                    glBindTexture(GL_TEXTURE_1D, renderer->randomValuesTexture);

                    if (firstChunkMeshShaderInvocation)
                    {
                        firstChunkMeshShaderInvocation = false;
                        glUniformMatrix4fv(chunkProg->vertex.uniforms.u_ViewMatrix, 1, GL_FALSE, lookAt.data);
                        glUniformMatrix4fv(chunkProg->vertex.uniforms.u_ProjectionMatrix, 1, GL_FALSE, projection.data);
                        glUniformMatrix4fv(chunkProg->vertex.uniforms.u_LightSpaceMatrix[0], 1, GL_FALSE, lightViewProj0.data);
                        glUniformMatrix4fv(chunkProg->vertex.uniforms.u_LightSpaceMatrix[1], 1, GL_FALSE, lightViewProj1.data);
                        glUniformMatrix4fv(chunkProg->vertex.uniforms.u_LightSpaceMatrix[2], 1, GL_FALSE, lightViewProj2.data);
                        glUniform3fv(chunkProg->fragment.uniforms.u_DirLight.dir, 1, group->dirLight.dir.data);
                        glUniform3fv(chunkProg->fragment.uniforms.u_DirLight.ambient, 1, group->dirLight.ambient.data);
                        glUniform3fv(chunkProg->fragment.uniforms.u_DirLight.diffuse, 1, group->dirLight.diffuse.data);
                        glUniform3fv(chunkProg->fragment.uniforms.u_DirLight.specular, 1, group->dirLight.specular.data);
                        glUniform3fv(chunkProg->fragment.uniforms.u_ViewPos, 1, group->cameraConfig.position.data);
                        glUniform1f(chunkProg->fragment.uniforms.shadowFilterSampleScale, shadowFilterScale);
                        f32 cascadeDepth = (camera->farPlane - camera->nearPlane) / 3.0f;
                        v3 splits = V3(cascadeDepth, cascadeDepth * 2.0f, cascadeDepth * 3.0f);
                        glUniform3fv(chunkProg->fragment.uniforms.u_ShadowCascadeSplits, 1, splits.data);
                        glUniform1i(chunkProg->fragment.uniforms.u_ShowShadowCascadesBoundaries, renderer->showShadowCascadesBoundaries);
                    }

                    for (u32 i = 0; i < command->instanceCount; i++)
                    {
                        auto* data = ((RenderCommandPushChunkMesh*)(group->renderBuffer + command->rbOffset)) + i;

                        m4x4 world = Translation(data->offset);
                        glUniformMatrix4fv(chunkProg->vertex.uniforms.u_ModelMatrix, 1, GL_FALSE, world.data);

                        m4x4 invModel = world;
                        bool inverted = Inverse(&invModel);
                        SOKO_ASSERT(inverted);
                        m4x4 transModel = Transpose(&invModel);
                        m3x3 normalMatrix = M3x3(&transModel);

                        glUniformMatrix3fv(chunkProg->vertex.uniforms.u_NormalMatrix, 1, GL_FALSE, normalMatrix.data);

                        glBindBuffer(GL_ARRAY_BUFFER, data->meshIndex);

                        GLsizei stride = sizeof(ChunkMeshVertex);
                        glEnableVertexAttribArray(0);
                        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                        glEnableVertexAttribArray(1);
                        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(v3)));
                        glEnableVertexAttribArray(2);
                        glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, stride, (void*)(sizeof(v3) * 2));

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
