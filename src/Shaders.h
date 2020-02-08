#pragma once
#include "Platform.h"
#include "Std140.h"
namespace soko
{
    struct ShaderProgramSource
    {
        const char* vert;
        const char* frag;
    };
}
#include "Shaders_Generated.h"

namespace soko
{
    constexpr u32 ShaderCount = sizeof(Shaders) / sizeof(GLuint);
    static_assert(ShaderCount == ArrayCount(ShaderSources));

    GLuint CompileGLSL(const char* name, const char* vert, const char* frag);
    void RecompileShaders(Renderer* renderer);
    inline void DeleteProgram(GLuint handle) { glDeleteProgram(handle); }

    template <typename T, u32 Binding>
    struct UniformBuffer
    {
        GLuint handle;
    };

    template<typename T, u32 Binding>
    void ReallocUniformBuffer(UniformBuffer<T, Binding>* buffer);

    template<typename T, u32 Binding>
    T* Map(UniformBuffer<T, Binding> buffer);

    template<typename T, u32 Binding>
    void Unmap(UniformBuffer<T, Binding> buffer);

    struct layout_std140 ShaderFrameData
    {
        struct layout_std140 DirLight
        {
            std140_vec3 dir;
            std140_vec3 ambient;
            std140_vec3 diffuse;
            std140_vec3 specular;
        };

        std140_mat4 viewProjMatrix;
        std140_mat4 viewMatrix;
        std140_mat4 projectionMatrix;
        std140_mat4 lightSpaceMatrices[3];
        DirLight dirLight;
        std140_vec3 viewPos;
        std140_vec3 shadowCascadeSplits;
        std140_int showShadowCascadeBoundaries;
        std140_float shadowFilterSampleScale;
        std140_int debugF;
        std140_int debugG;
        std140_int debugD;
        std140_int debugNormals;
    };

    struct layout_std140 ShaderMeshData
    {
        std140_mat4 modelMatrix;
        std140_mat3 normalMatrix;
        std140_vec3 lineColor;
        std140_int customMaterial;
        std140_vec3 customAlbedo;
        std140_float customRoughness;
        std140_float customMetalness;
    };

    struct layout_std140 ChunkFragUniformBuffer
    {
        struct layout_std140 DirLight
        {
            std140_vec3 dir;
            std140_vec3 ambient;
            std140_vec3 diffuse;
            std140_vec3 specular;
        };

        DirLight u_DirLight;
        std140_vec3 u_ViewPos;
        std140_vec3 u_ShadowCascadeSplits;
        std140_int u_ShowShadowCascadesBoundaries;
        std140_float shadowFilterSampleScale;
    };

    struct layout_std140 ChunkCameraBuffer
    {
        std140_mat4 viewMatrix;
        std140_mat4 projectionMatrix;
        std140_mat4_array lightSpaceMatrix[3];
    };

    struct layout_std140 ChunkMeshBuffer
    {
        std140_mat4 modelMatrix;
        std140_mat3 normalMatrix;
    };
}
