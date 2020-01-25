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
}