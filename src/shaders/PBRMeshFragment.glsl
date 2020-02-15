#version 450

#include Common.glh
#include Pbr.glh
#include ShadowsCommon.glh

out vec4 resultColor;

layout (location = 4) in VertOut
{
    vec3 fragPos;
    vec3 normal;
    vec2 uv;
    mat3 tbn;
    vec3 viewPosition;
    vec4 lightSpacePos[3];
} fragIn;

layout (binding = 0) uniform samplerCube IrradanceMap;
layout (binding = 1) uniform samplerCube EnviromentMap;
layout (binding = 2) uniform sampler2D BRDFLut;

layout (binding = 3) uniform sampler2D AlbedoMap;
layout (binding = 4) uniform sampler2D RoughnessMap;
layout (binding = 5) uniform sampler2D MetalnessMap;
layout (binding = 6) uniform sampler2D NormalMap;

layout (binding = 7) uniform sampler2DArrayShadow ShadowMap;
//uniform sampler2D uAOMap;

const float MAX_REFLECTION_LOD = 5.0f;

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 IBLIrradance(vec3 Vo, vec3 N, float NdotV, float a, vec3 F0, float metallic, vec3 albedo)
{
    // NOTE: Specular irradance
    vec3 R = reflect(-Vo, N);
    vec3 envIrradance = textureLod(EnviromentMap, R, a * MAX_REFLECTION_LOD).rgb;
    vec3 Fenv = FresnelSchlickRoughness(NdotV, F0, a);
    vec2 envBRDF = texture(BRDFLut, vec2(NdotV, a)).rg;
    vec3 envSpecular = envIrradance * (Fenv * envBRDF.r + envBRDF.g);

    // NOTE: Diffuse irradance
    vec3 kS = FresnelSchlick(NdotV, F0);
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0f - metallic;
    vec3 diffIrradance = texture(IrradanceMap, N).rgb;
    vec3 diffuse = diffIrradance * albedo;

    vec3 irradance = (kD * diffuse + envSpecular);// * uAO;
    return irradance;
}

void main()
{
    vec3 N;
    vec3 albedo;
    float roughness;
    float metalness;

    if (MeshData.customMaterial == 1)
    {
        N = normalize(fragIn.normal);
        albedo = MeshData.customAlbedo;
        roughness = MeshData.customRoughness;
        metalness = MeshData.customMetalness;
    }
    else
    {
         N = normalize(texture(NormalMap, fragIn.uv).xyz * 2.0f - 1.0f);
         // NOTE: Flipping y because engine uses LH normal maps (UE4) but OpenGL does it's job in RH space
         N.y = -N.y;
         N = normalize(fragIn.tbn * N);
         albedo = texture(AlbedoMap, fragIn.uv).xyz;
         roughness = texture(RoughnessMap, fragIn.uv).r;
         metalness = texture(MetalnessMap, fragIn.uv).r;
    }

    vec3 V = normalize(FrameData.viewPos - fragIn.fragPos);
    vec3 L0 = vec3(0.0f);

    vec3 L = normalize(-FrameData.dirLight.dir);
    vec3 H = normalize(V + L);

    vec3 irradance = FrameData.dirLight.diffuse;
    L0 += Unreal4BRDF(L, V, N, albedo, roughness, metalness) * irradance;

    float NdotV = saturate(dot(N, V)); // + 0.000001f; // NOTE: Adding this value (trick from epic games shaders) reduces artifacts on the edges in Intel gpu's but completely brokes everything on nvidia
    // TODO: Specify F0 for dielectrics
    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metalness);

    vec3 envIrradance = IBLIrradance(V, N, NdotV, roughness, F0, metalness, albedo);

    vec3 kShadow = CalcShadow(fragIn.viewPosition, FrameData.shadowCascadeSplits, fragIn.lightSpacePos, ShadowMap, FrameData.shadowFilterSampleScale, FrameData.showShadowCascadeBoundaries);

    resultColor = vec4((envIrradance + L0 * kShadow), 1.0f);
#if 0
    if (FrameData.debugF == 1) resultColor = vec4(F,  1.0f);
    else if (FrameData.debugG == 1) resultColor = vec4(G, G, G, 1.0f);
    else if (FrameData.debugD == 1) resultColor = vec4(D, D, D, 1.0f);
    else if (FrameData.debugNormals == 1) resultColor = vec4(N, 1.0f);
#endif
}