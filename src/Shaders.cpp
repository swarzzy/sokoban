namespace soko
{
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

uniform samplerCube u_CubeTexture;
void main()
{
    f_Color = texture(u_CubeTexture, v_UV);
}
)";

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
layout (location = 3) in int a_AO;

out vec3 v_Position;
out vec3 v_MeshSpacePos;
//out vec4 v_LightSpacePosition;
flat out int v_TileId;
out vec3 v_Normal;
out vec4 v_AO;
out vec2 v_UV;

uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;
uniform mat4 u_ViewProjMatrix;
//uniform mat4 u_LightSpaceMatrix;
uniform vec4 u_AODistrib;

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

    v_AO.x = u_AODistrib[(a_AO & 0x03)];
    v_AO.y = u_AODistrib[(a_AO & 0x0c) >> 2];
    v_AO.z = u_AODistrib[(a_AO & 0x30) >> 4];
    v_AO.w = u_AODistrib[(a_AO & 0xc0) >> 6];

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
in vec4 v_AO;
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
    diffSample = texture(u_TerrainAtlas, vec3(v_UV.x, v_UV.y, v_TileId)).rgb;
    alpha = 1.0f;

    vec3 directional = CalcDirectionalLight(u_DirLight, normal, viewDir, diffSample);
    //directional = diffSample;

    float ao0 = mix(v_AO.x, v_AO.y, fract(v_UV.x));
    float ao1 = mix(v_AO.w, v_AO.z, fract(v_UV.x));
    float ao = mix(ao0, ao1, fract(v_UV.y));

    color = vec4(directional * ao, alpha);
})";
}
