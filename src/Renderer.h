#pragma once

namespace soko
{
    struct ChunkMesh;
    struct LoadedChunkMesh
    {
        u32 gpuHandle;
        u64 quadCount;
    };

    struct Texture
    {
        GLuint gpuHandle;
        GLenum format;
        u32 width;
        u32 height;
        void* data;
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
