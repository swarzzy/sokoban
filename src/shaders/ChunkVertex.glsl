#version 330 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in int a_TileId;

out vec4 v_Position;
out vec3 v_ViewPosition;
out vec3 v_MeshSpacePos;
out vec4 v_LightSpacePos[3];
flat out int v_TileId;
out vec3 v_Normal;
out vec2 v_UV;

#define NUM_SHADOW_CASCADES 3

uniform mat4 u_ModelMatrix;
uniform mat3 u_NormalMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform mat4 u_LightSpaceMatrix[3];

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
    v_Position = (u_ModelMatrix * vec4(a_Position, 1.0f));
    v_ViewPosition = (u_ViewMatrix * u_ModelMatrix * vec4(a_Position, 1.0f)).xyz;
    v_Normal = u_NormalMatrix * a_Normal;
    v_LightSpacePos[0] = u_LightSpaceMatrix[0] * u_ModelMatrix * vec4(a_Position, 1.0f);
    v_LightSpacePos[1] = u_LightSpaceMatrix[1] * u_ModelMatrix * vec4(a_Position, 1.0f);
    v_LightSpacePos[2] = u_LightSpaceMatrix[2] * u_ModelMatrix * vec4(a_Position, 1.0f);
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * u_ModelMatrix * vec4(a_Position, 1.0f);
}