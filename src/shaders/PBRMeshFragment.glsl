#version 450
#include Common.glh
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

#define PI (3.14159265359)

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (vec3(1.0f) - F0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a4 = a * a * a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotHSq = NdotH * NdotH;

    float num = a4;
    float denom = (NdotHSq * (a4 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / max(denom, 0.001f);
}

float GeometrySchlickGGX(float NdotV, float a)
{
    float k = ((a + 1.0f) * (a + 1.0f)) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

float GeometrySmith(float NdotV, float NdotL, float a)
{
    float ggx1 = GeometrySchlickGGX(NdotV, a);
    float ggx2 = GeometrySchlickGGX(NdotL, a);

    return ggx1 * ggx2;
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

    // TODO: Adding this to avoid artifacts on edges
    // Why this value so big?
    float NdotV = max(dot(N, V), 0.0f);// + 0.000001f; // NOTE: Adding this value (trick from epic games shaders) reduces artifacts on the edges in Intel gpu's but completely brokes everything on nvidia
    float NdotL = max(dot(N, L), 0.0f);

    // NOTE: Attenuation should be here
    vec3 radiance = FrameData.dirLight.diffuse;

    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metalness);

    // NOTE: Seems like it prodices visually incorrect result with H vector
    // and N gives more Fresnel-look-like result
    // but in papers people usually use H
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);

    vec3 num = D * G * F;
    float denom = 4.0f * NdotV * NdotL;;
    vec3 specular = num / max(denom, 0.001f);

    {
        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metalness;
        L0 += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 envIrradance = IBLIrradance(V, N, NdotV, roughness, F0, metalness, albedo);

    vec3 kShadow = CalcShadow(fragIn.viewPosition, FrameData.shadowCascadeSplits, fragIn.lightSpacePos, ShadowMap, FrameData.shadowFilterSampleScale, FrameData.showShadowCascadeBoundaries);

    resultColor = vec4((envIrradance + L0 * kShadow), 1.0f);

    if (FrameData.debugF == 1) resultColor = vec4(F,  1.0f);
    else if (FrameData.debugG == 1) resultColor = vec4(G, G, G, 1.0f);
    else if (FrameData.debugD == 1) resultColor = vec4(D, D, D, 1.0f);
    else if (FrameData.debugNormals == 1) resultColor = vec4(N, 1.0f);
}