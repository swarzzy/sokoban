#version 330 core
#include "ShadowsCommon.glsl"

in vec4 v_Position;
in vec3 v_ViewPosition;
in vec4 v_LightSpacePos[3];
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
uniform sampler2DArrayShadow u_ShadowMap;
uniform vec3 u_ShadowCascadeSplits;
uniform int u_ShowShadowCascadesBoundaries = 0;

uniform float shadowFilterSampleScale = 1.0f;
uniform sampler1D randomTexture;

#define PI (3.14159265359)

//#define RANDOM_DISC_PCF 1
#define RANDOMIZE_OFFSETS 1
#define DUMMY_PCF 1


vec3 CalcDirectionalLight(DirLight light, vec3 normal,
                          vec3 viewDir,
                          vec3 diffSample)
{
    vec3 lightDir = normalize(-light.dir);
    vec3 lightDirReflected = reflect(-lightDir, normal);

    float Kd = max(dot(normal, lightDir), 0.0);

    float viewSpaceDepth = -v_ViewPosition.z;
    int cascadeIndex = GetShadowCascadeIndex(-v_ViewPosition.z, u_ShadowCascadeSplits);
    vec3 lightSpaceP = v_LightSpacePos[cascadeIndex].xyz / v_LightSpacePos[cascadeIndex].w;
   lightSpaceP = lightSpaceP * 0.5f + 0.5f;
#if DUMMY_PCF
   vec3 Kshadow = ShadowPCF(u_ShadowMap, cascadeIndex, lightSpaceP, viewSpaceDepth, shadowFilterSampleScale, u_ShowShadowCascadesBoundaries);
#endif
#if RANDOM_DISC_PCF
   vec3 Kshadow = ShadowRandomDisc(u_ShadowMap, randomTexture, cascadeIndex, lightSpaceP, viewSpaceDepth, shadowFilterSampleScale, u_ShowShadowCascadesBoundaries);
#endif

    vec3 ambient = light.ambient * diffSample;
    vec3 diffuse = Kd * light.diffuse * diffSample * Kshadow;
    return ambient + diffuse;
}

#define TERRAIN_TEX_ARRAY_NUM_LAYERS 32

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(u_ViewPos - v_Position.xyz);

    int tileID = clamp(v_TileId, 0, TERRAIN_TEX_ARRAY_NUM_LAYERS);

    vec3 diffSample;
    float alpha;
    diffSample = texture(u_TerrainAtlas, vec3(v_UV.x, v_UV.y, tileID)).rgb;
    alpha = 1.0f;

    vec3 directional = CalcDirectionalLight(u_DirLight, normal, viewDir, diffSample);
    //directional = diffSample;

    color = vec4(directional, alpha);
}
