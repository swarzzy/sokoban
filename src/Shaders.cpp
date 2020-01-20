namespace soko
{
    static const char* BRDF_INTERGATION_SHADER = R"(
#version 330 core

in vec2 v_UV;
out vec4 ResultColor;

const uint SAMPLE_COUNT = 1024u;

const float PI_32 = 3.14159265358979323846f;

// NOTE: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint n)
{
    return vec2(float(i) / float(n), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 p, vec3 N, float a)
{
    // NOTE: Epic Games GGX
    float aSq = a * a;
    float phi = 2.0f * PI_32 * p.x;
    float cosTheta = sqrt((1.0f - p.y) / (1.0f + (aSq * aSq - 1.0f) * p.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // NOTE: To cartesian
    vec3 d;
    d.x = cos(phi) * sinTheta;
    d.y = sin(phi) * sinTheta;
    d.z = cosTheta;

    // NOTE: From tangent space to world
    vec3 up = abs(N.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f); // ?????
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 sampleVec = right * d.x + up * d.y + N * d.z;
    return normalize(sampleVec);
}

float GeometrySchlickGGX(float NdotV, float a)
{
    float k = (a * a) / 1.0f;
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float a)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, a);
    float ggx1 = GeometrySchlickGGX(NdotL, a);
    return ggx1 * ggx2;
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0f - NdotV * NdotV);
    V.y = 0.0f;
    V.z = NdotV;

    float A = 0.0f;
    float B = 0.0f;

    vec3 N = vec3(0.0f, 0.0f, 1.0f);

    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0f);

        if (NdotL > 0.0f)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0f - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    return vec2(A, B);
}

void main()
{
    ResultColor = vec4(IntegrateBRDF(v_UV.x, v_UV.y), 0.0f, 1.0f);
})";

    static const char* ENV_MAP_PREFILTER_FRAG_SOURCE = R"(
#version 330 core

in vec3 v_UV;
out vec4 resultColor;

uniform samplerCube uSourceCubemap;
uniform float uRoughness;
uniform int uResolution;

const float PI_32 = 3.14159265358979323846f;

// NOTE: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint n)
{
    return vec2(float(i) / float(n), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 p, vec3 N, float a)
{
    // NOTE: Epic Games GGX
    float aSq = a * a;
    float phi = 2.0f * PI_32 * p.x;
    float cosTheta = sqrt((1.0f - p.y) / (1.0f + (aSq * aSq - 1.0f) * p.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // NOTE: To cartesian
    vec3 d;
    d.x = cos(phi) * sinTheta;
    d.y = sin(phi) * sinTheta;
    d.z = cosTheta;

    // NOTE: From tangent space to world
    vec3 up = abs(N.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f); // ?????
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 sampleVec = right * d.x + up * d.y + N * d.z;
    return normalize(sampleVec);
}

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a4 = a * a * a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotHSq = NdotH * NdotH;

    float num = a4;
    float denom = (NdotHSq * (a4 - 1.0f) + 1.0f);
    denom = PI_32 * denom * denom;

    return num / max(denom, 0.001f);
}

const uint SAMPLES = 4096u;

void main()
{
    vec3 N = normalize(v_UV);
    vec3 R = N;
    vec3 V = R;

    float totalWeight = 0.0f;
    vec3 prefColor = vec3(0.0f);

    for (uint i = 0u; i < SAMPLES; i++)
    {
        vec2 p = Hammersley(i, SAMPLES);
        vec3 H = ImportanceSampleGGX(p, N, uRoughness);
        vec3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0f);
        if (NdotL > 0.0f)
        {
            float D = DistributionGGX(N, H, uRoughness);
            float NdotH = max(dot(N, H), 0.0f);
            float HdotV = max(dot(H, V), 0.0f);
            float PDF = D * NdotH / (4.0f * HdotV) + 0.0001;
            float saTexel = 4.0f * PI_32 / (6.0f * uResolution * uResolution);
            float saSample = 1.0f / (float(SAMPLES) * PDF + 0.0001f);
            float mipLevel = uRoughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);
            prefColor += textureLod(uSourceCubemap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefColor = prefColor / totalWeight;

    resultColor = vec4(prefColor, 1.0f);
})";

    static const char* IRRADANCE_CONVOLUTION_FRAG_SOURCE = R"(
#version 330 core

in vec3 v_UV;
out vec4 resultColor;

uniform samplerCube uSourceCubemap;

const float PI_32 = 3.14159265358979323846f;

void main()
{
    vec3 normal = normalize(v_UV);
    vec3 irradance = vec3(0.0f);

    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = cross(up, normal);
    up = cross(normal, right);

    float sampleDelta = 0.025f;
    int sampleCount = 0;
    for (float phi = 0.0f; phi < (2.0f * PI_32); phi += sampleDelta)
    {
        for (float theta = 0.0f; theta < (0.5f * PI_32); theta += sampleDelta)
        {
            vec3 tgSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleDir = tgSample.x * right + tgSample.y * up + tgSample.z * normal;
            irradance += texture(uSourceCubemap, sampleDir).xyz * cos(theta) * sin(theta);
            sampleCount++;
        }
    }

    irradance = PI_32 * irradance * (1.0f / float(sampleCount));

    resultColor = vec4(irradance, 1.0f);
})";

    static const char* PBR_MESH_VERTEX_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;

uniform mat4 uViewProjMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vUV;
out mat3 vTBN;

void main()
{
    vec3 n = normalize(uNormalMatrix * aNormal);
    vec3 t = normalize(uNormalMatrix * aTangent);
    t = normalize(t - dot(t, n) * n);
    vec3 b = normalize(cross(n, t));
    mat3 tbn = mat3(t, b, n);

    gl_Position = uViewProjMatrix * uModelMatrix * vec4(aPos, 1.0f);
    vFragPos = (uModelMatrix * vec4(aPos, 1.0f)).xyz;
    vUV = aUV;
    vNormal = n;
    vTBN = tbn;
})";

    static const char* PBR_MESH_FRAG_SOURCE = R"(
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
})";

    static const char* SS_VERTEX_SOURCE = R"(
#version 330 core
vec2 VERTICES[] = vec2[](vec2(-1.0f, -1.0f),
                         vec2(1.0f, -1.0f),
                         vec2(1.0f, 1.0f),
                         vec2(1.0f, 1.0f),
                         vec2(-1.0f, 1.0f),
                         vec2(-1.0f, -1.0f));
out vec2 v_UV;

void main()
{
    vec4 vertexPos = vec4(VERTICES[min(gl_VertexID, 6)], 0.0f, 1.0f);
    gl_Position = vertexPos;
    v_UV = vertexPos.xy / 2.0f + 0.5f;
})";

    static const char* POSTFX_FRAG_SOURCE = R"(
#version 330 core
in vec2 v_UV;
out vec4 fragColorResult;

uniform float u_Gamma = 2.4f;
// TODO: Tonemapping
uniform float u_Exposure = 1.0f;

uniform sampler2D u_ColorSourceLinear;

float D3DX_FLOAT_to_SRGB(float val)
{
    if (val < 0.0031308f)
    {
        val *= 12.92f;
    }
    else
    {
        val = 1.055f * pow(val, 1.0f / u_Gamma) - 0.055f;
    }
    return val;
}

vec3 D3DX_RGB_to_SRGB(vec3 rgb)
{
    rgb.r = D3DX_FLOAT_to_SRGB(rgb.r);
    rgb.g = D3DX_FLOAT_to_SRGB(rgb.g);
    rgb.b = D3DX_FLOAT_to_SRGB(rgb.b);
    return rgb;
}

void main()
{
    vec3 hdrSample = texture(u_ColorSourceLinear, v_UV).xyz;
    //vec3 ldrSample = hdrSample / (hdrSample + vec3(1.0f))
    vec3 ldrSample = vec3(1.0f) - exp(-hdrSample * u_Exposure);
    vec3 resultSample = D3DX_RGB_to_SRGB(ldrSample);
    fragColorResult = vec4(resultSample, 1.0f);
})";

    static const char* FXAA_FRAG_SOURCE = R"(
#version 330 core
in vec2 v_UV;
out vec4 fragColorResult;

uniform sampler2D u_ColorSourcePerceptual;
uniform vec2 u_InvScreenSize;

float Luma(vec3 rgb)
{
    float result = dot(rgb, vec3(0.299f, 0.587f, 0.114f));
    return result;
}

#define EDGE_MIN_THRESHOLD 0.0625f  //0.0312f
#define EDGE_MAX_THRESHOLD 0.0625f  //0.125f
#define ITERATIONS 12
#define SUBPIXEL_QUALITY 0.75f

float STEPS[6] = float[](1.0f, 1.5f, 2.0f, 2.0f, 2.0f, 8.0f);
#define QUALITY(i) (STEPS[min(0, max(5, i))])

void main()
{
    // STUDY: Dependent texture reads
    vec3 sampleCenter = texture(u_ColorSourcePerceptual, v_UV).xyz;

    float lumaCenter = Luma(sampleCenter);
    float lumaDown = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(0, -1)).xyz);
    float lumaUp = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(0, 1)).xyz);
    float lumaLeft = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(-1, 0)).xyz);
    float lumaRight = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(1, 0)).xyz);

    float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
    float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));
    float lumaRange = lumaMax - lumaMin;

    if (lumaRange >= max(EDGE_MIN_THRESHOLD, lumaMax * EDGE_MAX_THRESHOLD))
    {
        float lumaDownLeft = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(-1, -1)).xyz);
        float lumaUpRight = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(1, 1)).xyz);
        float lumaUpLeft = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(-1, 1)).xyz);
        float lumaDownRight = Luma(textureOffset(u_ColorSourcePerceptual, v_UV, ivec2(1, -1)).xyz);

        float lumaDownUp = lumaDown + lumaUp;
        float lumaLeftRight = lumaLeft + lumaRight;
        float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
        float lumaDownCorners = lumaDownLeft + lumaDownRight;
        float lumaRightCorners = lumaDownRight + lumaUpRight;
        float lumaUpCorners = lumaUpRight + lumaUpLeft;

        float gradH = abs(-2.0f * lumaLeft + lumaLeftCorners) + abs(-2.0f * lumaCenter + lumaDownUp) * 2.0f + abs(-2.0 * lumaRight + lumaRightCorners);
        float gradV = abs(-2.0f * lumaUp + lumaUpCorners) + abs(-2.0f * lumaCenter + lumaLeftRight) * 2.0f + abs(-2.0f * lumaDown + lumaDownCorners);
        bool isHorizontal = (gradH >= gradV);

        float luma1 = isHorizontal ? lumaDown : lumaLeft;
        float luma2 = isHorizontal ? lumaUp : lumaRight;
        float grad1 = abs(luma1 - lumaCenter);
        float grad2 = abs(luma2 - lumaCenter);
        bool is1Steepest = grad1 >= grad2;
        float gradScaled = 0.25f * max(grad1, grad2);

        // TODO: dFdx() dFdy() ?
        float stepLength = isHorizontal ? u_InvScreenSize.y : u_InvScreenSize.x;
        float lumaLocalAvg = 0.0f;
        if (is1Steepest)
        {
            stepLength = -stepLength;
            lumaLocalAvg = 0.5f * (luma1 + lumaCenter);
        }
        else
        {
            lumaLocalAvg = 0.5f * (luma2 + lumaCenter);
        }

        vec2 currUV = v_UV;
        isHorizontal ? (currUV.y = currUV.y + stepLength * 0.5f) : (currUV.x = currUV.x + stepLength * 0.5f);

        vec2 offset = isHorizontal ? vec2(u_InvScreenSize.x, 0.0f) : vec2(0.0f, u_InvScreenSize.y);
        vec2 uv1 = currUV - offset;
        vec2 uv2 = currUV + offset;

        float lumaEnd1 = Luma(texture(u_ColorSourcePerceptual, uv1).xyz);
        float lumaEnd2 = Luma(texture(u_ColorSourcePerceptual, uv2).xyz);
        lumaEnd1 -= lumaLocalAvg;
        lumaEnd2 -= lumaLocalAvg;
        bool reached1 = abs(lumaEnd1) >= gradScaled;
        bool reached2 = abs(lumaEnd2) >= gradScaled;
        bool reachedBoth = reached1 && reached2;
        if (!reached1) uv1 -= offset;
        if (!reached2) uv2 += offset;

        if (!reachedBoth)
        {
            for (int i = 1; i < ITERATIONS; i++)
            {
                if (!reached1)
                {
                    lumaEnd1 = Luma(texture(u_ColorSourcePerceptual, uv1).xyz);
                    lumaEnd1 -= lumaLocalAvg;
                }
                if (!reached2)
                {
                    lumaEnd2 = Luma(texture(u_ColorSourcePerceptual, uv2).xyz);
                    lumaEnd2 -= lumaLocalAvg;
                }
                reached1 = abs(lumaEnd1) >= gradScaled;
                reached2 = abs(lumaEnd2) >= gradScaled;
                reachedBoth = reached1 && reached2;
                if (!reached1) uv1 -= offset * QUALITY(i);
                if (!reached2) uv2 += offset * QUALITY(i);
                if (reachedBoth) break;
            }
        }

        float dist1 = isHorizontal ? (v_UV.x - uv1.x) : (v_UV.y - uv1.y);
        float dist2 = isHorizontal ? (uv2.x - v_UV.x) : (uv2.y - v_UV.y);

        bool isDir1 = (dist1 < dist2);
        float minDist = min(dist1, dist2);
        float edgeLen = (dist1 + dist2);

        float pixelOffset = -minDist / edgeLen + 0.5f;

        bool isLumaCenterSmaller = lumaCenter < lumaLocalAvg;
        bool correctVariation = ((isDir1 ? lumaEnd1 : lumaEnd2) < 0.0f) != isLumaCenterSmaller;
        pixelOffset = correctVariation ? pixelOffset : 0.0f;

        vec2 resultUV = v_UV;
        isHorizontal ? (resultUV.y = resultUV.y + pixelOffset * stepLength) : (resultUV.x = resultUV.x + pixelOffset * stepLength);

        float lumaAvg = (1.0f / 12.0f) * (2.0f * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
        float subPixelOffset1 = clamp(abs(lumaAvg - lumaCenter) / lumaRange, 0.0f, 1.0f);
        float subPixelOffset2 = (-2.0f * subPixelOffset1 + 3.0f) + subPixelOffset1 * subPixelOffset1;
        float subPixelOffsetResult = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
        pixelOffset = max(pixelOffset, subPixelOffsetResult);

        fragColorResult = vec4(texture(u_ColorSourcePerceptual, resultUV).xyz, 1.0f);
    }
    else
    {
        fragColorResult = vec4(sampleCenter, 1.0f);
    }
    //fragColorResult = vec4(sampleCenter, 1.0f);
})";

    static const char* SKYBOX_VERTEX_SOURCE = R"(
#version 330 core

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjMatrix;

out vec3 v_UV;

vec2 VERTICES[] = vec2[](vec2(-1.0f, -1.0f),
                         vec2(1.0f, -1.0f),
                         vec2(1.0f, 1.0f),
                         vec2(1.0f, 1.0f),
                         vec2(-1.0f, 1.0f),
                         vec2(-1.0f, -1.0f));

void main()
{
    vec4 vertexPos = vec4(VERTICES[min(gl_VertexID, 6)], 0.0f, 1.0f);
    gl_Position = vertexPos;
    gl_Position = gl_Position.xyww;
    v_UV = mat3(inverse(u_ViewMatrix)) * (inverse(u_ProjMatrix) * gl_Position).xyz;
})";

    static const char* SKYBOX_FRAG_SOURCE = R"(
#version 330 core
in vec3 v_UV;

out vec4 f_Color;
uniform float uLod = 1.0f;

uniform samplerCube u_CubeTexture;
void main()
{
    f_Color = textureLod(u_CubeTexture, v_UV, uLod);
}
)";

    static const char* SKYBOX_EQUIRECT_VERTEX_SOURCE = R"(
#version 330 core

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjMatrix;

out vec3 v_UV;

vec2 VERTICES[] = vec2[](vec2(-1.0f, -1.0f),
                         vec2(1.0f, -1.0f),
                         vec2(1.0f, 1.0f),
                         vec2(1.0f, 1.0f),
                         vec2(-1.0f, 1.0f),
                         vec2(-1.0f, -1.0f));

void main()
{
    vec4 vertexPos = vec4(VERTICES[min(gl_VertexID, 6)], 0.0f, 1.0f);
    gl_Position = vertexPos;
    gl_Position = gl_Position.xyww;
    v_UV = mat3(inverse(u_ViewMatrix)) * (inverse(u_ProjMatrix) * gl_Position).xyz;
})";

    static const char* SKYBOX_EQUIRECT_FRAG_SOURCE = R"(
#version 330 core
in vec3 v_UV;

out vec4 f_Color;

uniform sampler2D u_CubeTexture;

vec2 SphericalToEquirectUV(vec3 uv)
{
    vec2 result = vec2(atan(uv.z, uv.x), asin(uv.y));
    result *= vec2(0.1591f, 0.3183f);
    result += 0.5f;
    return result;
}

void main()
{
    f_Color = vec4(texture(u_CubeTexture, SphericalToEquirectUV(normalize(v_UV))).rgb, 1.0f);
})";

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

out vec3 vout_FragPos;
out vec3 vout_Normal;
out vec2 vout_UV;

void main()
{
    gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(attr_Pos, 1.0f);
    vout_FragPos = (u_ModelMatrix * vec4(attr_Pos, 1.0f)).xyz; //gl_Position.xyz;
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
    vec3 lightDir = normalize(-u_DirLight.dir);
    float kDiff = max(dot(normal, lightDir), 0.0f);
    vec3 viewDir = normalize(u_ViewPos - vout_FragPos);
    vec3 rFromLight = reflect(-lightDir, normal);
    float kSpec = pow(max(dot(viewDir, rFromLight), 0.0f), 32.0f);
    vec4 ambient = diffSamle * vec4(u_DirLight.ambient, 1.0f);
    vec4 diffuse = diffSamle * kDiff * vec4(u_DirLight.diffuse, 1.0f);
    vec4 specular = specSample * kSpec * vec4(u_DirLight.specular, 1.0f);
    out_Color = ambient + diffuse + specular;
})";

    static const char* CHUNK_VERTEX_SOURCE = R"(
#version 330 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in int a_TileId;

out vec3 v_Position;
out vec3 v_MeshSpacePos;
//out vec4 v_LightSpacePosition;
flat out int v_TileId;
out vec3 v_Normal;
out vec2 v_UV;

uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;
uniform mat4 u_ViewProjMatrix;
//uniform mat4 u_LightSpaceMatrix;

#define TERRAIN_TEX_ARRAY_NUM_LAYERS 32
#define INDICES_PER_CHUNK_QUAD 6
#define VERTICES_PER_QUAD 4

vec2 UV[] = vec2[](vec2(0.0f, 0.0f),
                   vec2(1.0f, 0.0f),
                   vec2(1.0f, 1.0f),
                   vec2(0.0f, 1.0f));

void main()
{
    // TODO: Pass ints as vertex attrib
    // This problem will be solved when we switch to using
    // packed vertex attributes

    int vertIndexInQuad = gl_VertexID % 4;

    v_UV = UV[min(vertIndexInQuad, VERTICES_PER_QUAD - 1)];

    v_TileId = a_TileId;
    v_MeshSpacePos = a_Position;
    v_Position = (u_ModelMatrix * vec4(a_Position, 1.0f)).xyz;
    v_Normal = u_NormalMatrix * a_Normal;
    //v_LightSpacePosition = u_LightSpaceMatrix * modelMatrix * vec4(a_Position, 1.0f);
    gl_Position = u_ViewProjMatrix * u_ModelMatrix * vec4(a_Position, 1.0f);
})";

    static const char* CHUNK_FRAG_SOURCE = R"(
#version 330 core
in vec3 v_Position;
in vec3 v_MeshSpacePos;
//in vec4 v_LightSpacePosition;
flat in int v_TileId;
in vec3 v_Normal;
in vec2 v_UV;

out vec4 color;

struct DirLight
{
    vec3 dir;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirLight u_DirLight;
uniform vec3 u_ViewPos;
uniform sampler2DArray u_TerrainAtlas;

vec3 CalcDirectionalLight(DirLight light, vec3 normal,
                          vec3 viewDir,
                          vec3 diffSample)
{
    vec3 lightDir = normalize(-light.dir);
    vec3 lightDirReflected = reflect(-lightDir, normal);

    float Kd = max(dot(normal, lightDir), 0.0);

    vec3 ambient = light.ambient * diffSample;
    vec3 diffuse = Kd * light.diffuse * diffSample;
    return ambient + diffuse;
}

#define TERRAIN_TEX_ARRAY_NUM_LAYERS 32

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(u_ViewPos - v_Position);

    int tileID = clamp(v_TileId, 0, TERRAIN_TEX_ARRAY_NUM_LAYERS);

    vec3 diffSample;
    float alpha;
    diffSample = texture(u_TerrainAtlas, vec3(v_UV.x, v_UV.y, tileID)).rgb;
    alpha = 1.0f;

    vec3 directional = CalcDirectionalLight(u_DirLight, normal, viewDir, diffSample);
    //directional = diffSample;

    color = vec4(directional, alpha);
})";
}
