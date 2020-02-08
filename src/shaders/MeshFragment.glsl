#version 450
#include Common.glh

out vec4 out_Color;

layout (location = 3) in vec3 vout_FragPos;
layout (location = 4) in vec3 vout_Normal;
layout (location = 5) in vec2 vout_UV;

layout (binding = 0) uniform sampler2D u_DiffMap;
layout (binding = 1) uniform sampler2D u_SpecMap;

void main()
{
    vec3 normal = normalize(vout_Normal);
    vec4 diffSamle = texture(u_DiffMap, vout_UV);
    vec4 specSample = texture(u_SpecMap, vout_UV);
    specSample.a = 1.0f;
    vec3 lightDir = normalize(-FrameData.dirLight.dir);
    float kDiff = max(dot(normal, lightDir), 0.0f);
    vec3 viewDir = normalize(FrameData.viewPos - vout_FragPos);
    vec3 rFromLight = reflect(-lightDir, normal);
    float kSpec = pow(max(dot(viewDir, rFromLight), 0.0f), 32.0f);
    vec4 ambient = diffSamle * vec4(FrameData.dirLight.ambient, 1.0f);
    vec4 diffuse = diffSamle * kDiff * vec4(FrameData.dirLight.diffuse, 1.0f);
    vec4 specular = specSample * kSpec * vec4(FrameData.dirLight.specular, 1.0f);
    out_Color = ambient + diffuse + specular;
}