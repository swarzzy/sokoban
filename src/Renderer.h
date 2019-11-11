#pragma once

namespace soko
{
    struct ChunkMesh;
    struct LoadedChunkMesh
    {
        u32 gpuHandle;
        u64 quadCount;
    };

    enum TextureFilter
    {
        TextureFilter_None,
        TextureFilter_Bilinear,
        TextureFilter_Trilinear,
        TextureFilter_Anisotropic
    };

    struct Texture
    {
        GLuint gpuHandle;
        TextureFilter filter;
        GLenum format;
        GLenum wrapMode;
        u32 width;
        u32 height;
        void* data;
    };

    struct Material
    {
        enum { Legacy = 0, PBR } type;
        union
        {
            struct
            {
                Texture diffMap;
                Texture specMap;
            } legacy;
            struct
            {
                b32 isCustom;
                union
                {
                    struct
                    {
                        v3 albedo;
                        f32 roughness;
                        f32 metalness;
                    } custom;
                    struct
                    {
                        Texture albedo;
                        Texture roughness;
                        Texture metalness;
                        Texture normals;
                    } map;
                };
            } pbr;
        };
    };

    struct CubeTexture
    {
        struct Img
        {
            GLenum format;
            u32 width;
            u32 height;
            void* data;
        };

        GLuint gpuHandle;
        b32 useMips;
        TextureFilter filter;

        union
        {
            struct
            {
                Img right;
                Img left;
                Img up;
                Img down;
                Img front;
                Img back;
            };
            Img images[6];
        };
    };
}
