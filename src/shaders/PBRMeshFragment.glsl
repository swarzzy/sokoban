#version 330 core
out vec4 resultColor;

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;
in mat3 vTBN;

struct DirLight
{
    vec3 dir;
    vec3 color;
};

uniform DirLight uDirLight;
uniform vec3 uViewPos;

uniform samplerCube uIrradanceMap;
uniform samplerCube uEnviromentMap;
uniform sampler2D uBRDFLut;

uniform sampler2D uAlbedoMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uMetalnessMap;
uniform sampler2D uNormalMap;
//uniform sampler2D uAOMap;

uniform int uCustomMaterial;
uniform vec3 uCustomAlbedo;
uniform float uCustomRoughness, uCustomMetalness;
//uniform float uCustomMetalness;

uniform int uDebugF;
uniform int uDebugG;
uniform int uDebugD;
uniform int uDebugNormals;

uniform float uAO = 1.0f;

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
    vec3 envIrradance = textureLod(uEnviromentMap, R, a * MAX_REFLECTION_LOD).rgb;
    vec3 Fenv = FresnelSchlickRoughness(NdotV, F0, a);
    vec2 envBRDF = texture(uBRDFLut, vec2(NdotV, a)).rg;
    vec3 envSpecular = envIrradance * (Fenv * envBRDF.r + envBRDF.g);

    // NOTE: Diffuse irradance
    vec3 kS = FresnelSchlick(NdotV, F0);
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0f - metallic;
    vec3 diffIrradance = texture(uIrradanceMap, N).rgb;
    vec3 diffuse = diffIrradance * albedo;

    vec3 irradance = (kD * diffuse + envSpecular) * uAO;
    return irradance;
}

void main()
{
    vec3 N;
    vec3 albedo;
    float roughness;
    float metalness;

    if (uCustomMaterial == 1)
    {
        N = normalize(vNormal);
        albedo = uCustomAlbedo;
        roughness = uCustomRoughness;
        metalness = uCustomMetalness;
    }
    else
    {
         N = normalize(texture(uNormalMap, vUV).xyz * 2.0f - 1.0f);
         // NOTE: Flipping y because engine uses LH normal maps (UE4) but OpenGL does it's job in RH space
         N.y = -N.y;
         N = normalize(vTBN * N);
         albedo = texture(uAlbedoMap, vUV).xyz;
         roughness = texture(uRoughnessMap, vUV).r;
         metalness = texture(uMetalnessMap, vUV).r;
    }

    vec3 V = normalize(uViewPos - vFragPos);
    vec3 L0 = vec3(0.0f);

    vec3 L = normalize(-uDirLight.dir);
    vec3 H = normalize(V + L);

    // TODO: Adding this to avoid artifacts on edges
    // Why this value so big?
    float NdotV = max(dot(N, V), 0.0f);// + 0.000001f; // NOTE: Adding this value (trick from epic games shaders) reduces artifacts on the edges in Intel gpu's but completely brokes everything on nvidia
    float NdotL = max(dot(N, L), 0.0f);

    // NOTE: Attenuation should be here
    vec3 radiance = uDirLight.color;

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

    resultColor = vec4((envIrradance + L0), 1.0f);

    if (uDebugF == 1) resultColor = vec4(F,  1.0f);
    else if (uDebugG == 1) resultColor = vec4(G, G, G, 1.0f);
    else if (uDebugD == 1) resultColor = vec4(D, D, D, 1.0f);
    else if (uDebugNormals == 1) resultColor = vec4(N, 1.0f);
}