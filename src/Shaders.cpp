namespace soko
{
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
    vec3 sample = D3DX_RGB_to_SRGB(texture(u_ColorSourceLinear, v_UV).xyz);
    vec3 hdrSample = sample;
    vec3 mappedSample = vec3(1.0f) - exp(-hdrSample * u_Exposure);
    fragColorResult = vec4(mappedSample, 1.0f);
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
