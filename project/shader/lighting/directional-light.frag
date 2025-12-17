// Calculates direcitonal lighting

#version 460

#extension GL_GOOGLE_include_directive : enable

#include "../common/pbr.glsl"
#include "../common/gbuffer-storage.glsl"
#include "../common/ndc-uv-conv.glsl"

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 ndc;

layout(set = 2, binding = 0) uniform sampler2D albedo_tex;
layout(set = 2, binding = 1) uniform usampler2D light_info_tex;
layout(set = 2, binding = 2) uniform sampler2D depth_tex;
layout(set = 2, binding = 3) uniform sampler2DShadow shadow_tex_level0;
layout(set = 2, binding = 4) uniform sampler2DShadow shadow_tex_level1;
layout(set = 2, binding = 5) uniform sampler2DShadow shadow_tex_level2;

layout(location = 0) out vec4 out_light_buffer;

layout(std140, set = 3, binding = 0) uniform Param
{
    mat4 VP_inv;
    mat4 shadow_VP_levels[3];
    vec3 eye_position;
    vec3 direction;
    vec3 intensity;
};

bool try_csm_level(vec3 world_pos, const uint level, out float shadow_mult)
{
    vec4 shadow_clip_space_pos = shadow_VP_levels[level] * vec4(world_pos, 1.0);
    shadow_clip_space_pos /= shadow_clip_space_pos.w;
    vec2 shadow_uv = ndc_to_uv(shadow_clip_space_pos.xy);

    if (all(lessThanEqual(abs(shadow_clip_space_pos.xyz), vec3(1.0))))
    {
        if (level == 0)
        {
            shadow_mult = texture(shadow_tex_level0, vec3(shadow_uv, shadow_clip_space_pos.z + 0.001)).r;
            return true;
        }
        else if (level == 1)
        {
            shadow_mult = texture(shadow_tex_level1, vec3(shadow_uv, shadow_clip_space_pos.z + 0.001)).r;
            return true;
        }
        else if (level == 2)
        {
            shadow_mult = texture(shadow_tex_level2, vec3(shadow_uv, shadow_clip_space_pos.z + 0.001)).r;
            return true;
        }
        else
            return false;
    }

    return false;
}

void main()
{
    /* Fetch Texture */

    uvec2 light_info_tex_sample = textureLod(light_info_tex, uv, 0).rg;
    float depth = textureLod(depth_tex, uv, 0).r;

    /* Get Albedo */

    vec3 albedo = textureLod(albedo_tex, uv, 0).rgb;

    /* Unpack */

    GBufferLighting lighting = unpack_gbuffer_lighting(light_info_tex_sample);

    lighting.roughness = mix(0.04, 1.0, lighting.roughness);

    /* Get View Direction */

    vec4 clip_space_pos = vec4(ndc, depth, 1.0);
    vec4 world_pos = VP_inv * clip_space_pos;
    world_pos /= world_pos.w;

    vec3 target_to_eye = normalize(eye_position - world_pos.xyz);

    float shadow_mult = 1.0;

    if (!try_csm_level(world_pos.xyz, 0, shadow_mult))
        if (!try_csm_level(world_pos.xyz, 1, shadow_mult))
            try_csm_level(world_pos.xyz, 2, shadow_mult);

    /* Color Calculation */

    vec3 pbr_color = gltf_calculate_pbr(
            direction,
            intensity,
            target_to_eye,
            lighting.normal,
            albedo,
            lighting.roughness,
            lighting.metalness
        );

    /* Output */
    out_light_buffer = vec4(pbr_color * shadow_mult, shadow_mult);
}
