#version 450
#include Common.glh
layout (location = 0) in vec3 attr_Pos;
layout (location = 1) in vec3 attr_Normal;
layout (location = 2) in vec2 attr_UV;

layout (location = 3) out vec3 vout_FragPos;
layout (location = 4) out vec3 vout_Normal;
layout (location = 5) out vec2 vout_UV;

void main()
{
    gl_Position = FrameData.projectionMatrix * FrameData.viewMatrix * MeshData.modelMatrix * vec4(attr_Pos, 1.0f);
    vout_FragPos = (MeshData.modelMatrix * vec4(attr_Pos, 1.0f)).xyz;
    vout_UV = attr_UV;
    vout_Normal = MeshData.normalMatrix * attr_Normal;
}