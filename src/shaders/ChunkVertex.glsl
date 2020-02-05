#version 450
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in int a_TileId;

layout (location = 0) out vec3 v_ViewPosition;
layout (location = 1) out vec4 v_LightSpacePos[3];
layout (location = 2) flat out int v_TileId;
layout (location = 3) out vec3 v_Normal;
layout (location = 4) out vec2 v_UV;
layout (location = 5) out vec3 v_Position;

layout (std140, binding = 0) uniform CameraData
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 lightSpaceMatrix[3];
} camera;

layout (std140, binding = 1) uniform MeshData
{
    mat4 modelMatrix;
    mat3 normalMatrix;
} mesh;

#define NUM_SHADOW_CASCADES 3
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
    v_Position = (mesh.modelMatrix * vec4(a_Position, 1.0f));
    v_ViewPosition = (camera.viewMatrix * mesh.modelMatrix * vec4(a_Position, 1.0f)).xyz;
    v_Normal = mesh.normalMatrix * a_Normal;
    v_LightSpacePos[0] = camera.lightSpaceMatrix[0] * mesh.modelMatrix * vec4(a_Position, 1.0f);
    v_LightSpacePos[1] = camera.lightSpaceMatrix[1] * mesh.modelMatrix * vec4(a_Position, 1.0f);
    v_LightSpacePos[2] = camera.lightSpaceMatrix[2] * mesh.modelMatrix * vec4(a_Position, 1.0f);
    gl_Position = camera.projectionMatrix * camera.viewMatrix * mesh.modelMatrix * vec4(a_Position, 1.0f);
}