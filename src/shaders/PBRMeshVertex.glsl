#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;

uniform mat4 uViewProjMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vUV;
out mat3 vTBN;

void main()
{
    vec3 n = normalize(uNormalMatrix * aNormal);
    vec3 t = normalize(uNormalMatrix * aTangent);
    t = normalize(t - dot(t, n) * n);
    vec3 b = normalize(cross(n, t));
    mat3 tbn = mat3(t, b, n);

    gl_Position = uViewProjMatrix * uModelMatrix * vec4(aPos, 1.0f);
    vFragPos = (uModelMatrix * vec4(aPos, 1.0f)).xyz;
    vUV = aUV;
    vNormal = n;
    vTBN = tbn;
}