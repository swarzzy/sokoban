#version 450
#include Common.glh
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;

layout (location = 4) out vec3 vFragPos;
layout (location = 5) out vec3 vNormal;
layout (location = 6) out vec2 vUV;
layout (location = 7) out mat3 vTBN;

void main()
{
    vec3 n = normalize(MeshData.normalMatrix * aNormal);
    vec3 t = normalize(MeshData.normalMatrix * aTangent);
    t = normalize(t - dot(t, n) * n);
    vec3 b = normalize(cross(n, t));
    mat3 tbn = mat3(t, b, n);

    gl_Position = FrameData.viewProjMatrix * MeshData.modelMatrix * vec4(aPos, 1.0f);
    vFragPos = (MeshData.modelMatrix * vec4(aPos, 1.0f)).xyz;
    vUV = aUV;
    vNormal = n;
    vTBN = tbn;
}