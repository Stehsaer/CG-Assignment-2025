// G-Buffer Fragment Shader, MASKED

#version 460

#extension GL_GOOGLE_include_directive : enable
#include "../common/gbuffer-storage.glsl"

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out uvec2 out_light_info;
layout(location = 2) out vec4 out_light_buffer;

layout(set = 2, binding = 0) uniform sampler2D albedo_tex; // Note: SRGB
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D metalness_roughness_tex;
layout(set = 2, binding = 3) uniform sampler2D occlusion_tex;
layout(set = 2, binding = 4) uniform sampler2D emissive_tex;

layout(std140, set = 3, binding = 0) uniform Material
{
    vec3 base_color_factor;
    vec3 emissive_factor;
    float metallic_factor;
    float roughness_factor;
    float normal_scale;
    float alpha_cutoff;
    float occlusion_strength;
};

void main()
{
    /* Texture Fetch */

    vec4 albedo_tex_sample = texture(albedo_tex, in_uv);
    if (albedo_tex_sample.a < alpha_cutoff) discard;

    vec2 normal_tex_sample = texture(normal_tex, in_uv).xy;
    vec2 metalness_roughness_tex_sample = texture(metalness_roughness_tex, in_uv).bg;
    float occlusion_tex_sample = texture(occlusion_tex, in_uv).r;
    vec3 emissive_tex_sample = texture(emissive_tex, in_uv).rgb;

    /* Albedo */

    vec3 out_color = pow(
            albedo_tex_sample.rgb * base_color_factor, // Get color and apply factor
            vec3(1 / 2.2) // Linear -> SRGB
        );

    out_albedo = vec4(out_color, 1.0);

    /* Normal Mapping */

    vec2 normal_xy = fma(normal_tex_sample, vec2(2.0), vec2(-1.0));
    vec3 normal = vec3(normal_xy, max(0.0, 1.0 - dot(normal_xy, normal_xy)));
    normal.xy *= normal_scale;

    mat3 TBN = mat3(normalize(in_tangent), normalize(in_bitangent), normalize(in_normal));
    normal = normalize(TBN * normal);
    if (!gl_FrontFacing) normal = -normal;

    /* Lighting Info */

    GBufferLighting lighting;
    lighting.normal = normal;
    lighting.metalness = metalness_roughness_tex_sample.x * metallic_factor;
    lighting.roughness = metalness_roughness_tex_sample.y * roughness_factor;
    lighting.occlusion = 1 - (1 - occlusion_tex_sample) * occlusion_strength;

    out_light_info = pack_gbuffer_lighting(lighting);

    /* Emissive */

    vec3 emissive = emissive_tex_sample * emissive_factor;
    out_light_buffer = vec4(emissive, smoothstep(0.0, 1.0, dot(emissive, vec3(0.2, 0.7, 0.1)) * 3));
}
