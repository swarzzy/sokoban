#version 330 core
layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

uniform mat4 ModelMatrix;
uniform mat4 ViewProjMatrix;
uniform mat3 NormalMatrix;
uniform vec3 LightPos;

uniform float ShadowNormalBiasScale = 0.0f;

void main()
{
    vec3 normal = normalize(NormalMatrix * normalize(Normal));
    float NdotL = dot(normal, LightPos);
    vec3 p = (ModelMatrix * vec4(Position, 1.0f)).xyz;
    //p += normal * ShadowNormalBiasScale;
    gl_Position = ViewProjMatrix * vec4(p, 1.0f);
}