#version 330 core
in vec3 v_UV;

out vec4 f_Color;
uniform float uLod = 1.0f;

uniform samplerCube u_CubeTexture;
void main()
{
    f_Color = textureLod(u_CubeTexture, v_UV, uLod);
}