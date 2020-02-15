#version 450
#include Common.glh

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 UV;

void main()
{
    gl_Position = FrameData.viewProjMatrix * MeshData.modelMatrix * vec4(Position, 1.0f);
}