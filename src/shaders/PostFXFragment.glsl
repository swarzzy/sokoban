#version 330 core
in vec2 v_UV;
out vec4 fragColorResult;

uniform float u_Gamma = 2.4f;
// TODO: Tonemapping
uniform float u_Exposure = 1.0f;

uniform sampler2D u_ColorSourceLinear;

float D3DX_FLOAT_to_SRGB(float val)
{
    if (val < 0.0031308f)
    {
        val *= 12.92f;
    }
    else
    {
        val = 1.055f * pow(val, 1.0f / u_Gamma) - 0.055f;
    }
    return val;
}

vec3 D3DX_RGB_to_SRGB(vec3 rgb)
{
    rgb.r = D3DX_FLOAT_to_SRGB(rgb.r);
    rgb.g = D3DX_FLOAT_to_SRGB(rgb.g);
    rgb.b = D3DX_FLOAT_to_SRGB(rgb.b);
    return rgb;
}

void main()
{
    vec3 hdrSample = texture(u_ColorSourceLinear, v_UV).xyz;
    //vec3 ldrSample = hdrSample / (hdrSample + vec3(1.0f))
    vec3 ldrSample = vec3(1.0f) - exp(-hdrSample * u_Exposure);
    vec3 resultSample = D3DX_RGB_to_SRGB(ldrSample);
    fragColorResult = vec4(resultSample, 1.0f);
}