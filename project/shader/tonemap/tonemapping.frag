// Tonemapping Fragment Shader

#version 460

#extension GL_GOOGLE_include_directive : enable
#include "agx.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 ndc;

layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler2D light_buffer_tex;
layout(set = 2, binding = 1) uniform sampler2D bloom_tex;

layout(std430, set = 2, binding = 2) readonly buffer Auto_exposure
{
    float avg_luminance;
    float exposure_mult;
};

layout(std140, set = 3, binding = 0) uniform Parameter
{
    float exposure;
    float bloom_strength;
};

const uint dither_pattern[4] = uint[4](0, 2, 3, 1);

void main()
{
    float exposure_mult_adjusted = exposure_mult * exposure;

    vec3 hdr_color =
        (
            textureLod(light_buffer_tex, uv, 0).rgb * exposure_mult_adjusted 
            + textureLod(bloom_tex, uv, 0).rgb * bloom_strength
            );
    vec3 srgb_color = pow(agx(hdr_color), vec3(2.2));

    vec3 srgb_color_8bit = srgb_color * 254.9;

    uvec3 sub_quant = uvec3(floor(fract(srgb_color_8bit) * 4));
    uint dither_index = (uint(gl_FragCoord.x) % 2) + (uint(gl_FragCoord.y) % 2) * 2;
    bvec3 dither = lessThan(uvec3(dither_pattern[dither_index]), sub_quant);
    vec3 dithered_color = mix(floor(srgb_color_8bit), ceil(srgb_color_8bit), dither);

    out_color = vec4(dithered_color / 255.0, 1.0);
}
