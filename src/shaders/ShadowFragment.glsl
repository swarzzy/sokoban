#version 330 core
out vec4 color;

uniform float ConstantShadowBias = 0.0f;

void main()
{
    gl_FragDepth = gl_FragCoord.z + ConstantShadowBias;
    color = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0f);
}
