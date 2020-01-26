#version 330 core
in vec4 v_Position;
in vec3 v_MeshSpacePos;
in vec4 v_LightSpacePos;
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
// TODO: Use shadow sampler and sampling with comparsion
uniform sampler2D u_ShadowMap;

float Shadow()
{
    vec3 coord = v_LightSpacePos.xyz / v_LightSpacePos.w;
    coord = coord * 0.5f + 0.5f;
    float currentDepth = coord.z;
    float bias = 0.001f;
    float shadowDepth = texture(u_ShadowMap, coord.xy).r + bias;
    return (currentDepth < shadowDepth ? 1.0f : 0.0f);
}

vec3 CalcDirectionalLight(DirLight light, vec3 normal,
                          vec3 viewDir,
                          vec3 diffSample)
{
    vec3 lightDir = normalize(-light.dir);
    vec3 lightDirReflected = reflect(-lightDir, normal);

    float Kd = max(dot(normal, lightDir), 0.0);
    float Kshadow = Shadow();
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