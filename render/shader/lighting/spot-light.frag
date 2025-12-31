#version 460

#extension GL_GOOGLE_include_directive : enable

#include "../common/pbr.glsl"
#include "../common/gbuffer-storage.glsl"
#include "../common/ndc-uv-conv.glsl"

layout(set = 2, binding = 0) uniform sampler2D albedo_tex;
layout(set = 2, binding = 1) uniform usampler2D light_info_tex;
layout(set = 2, binding = 2) uniform sampler2D depth_tex;

layout(location = 0) out vec4 out_light_buffer;

layout(std140, set = 3, binding = 0) uniform Param
{
    mat4 VP_inv;
    mat4 M_inv; // inverse model matrix
    vec2 screen_size;
    vec3 eye_position;
    vec3 light_position;
    vec3 rel_intensity;
    float range;
    float inner_cone_cos;
    float outer_cone_cos;
};

void main()
{
    const ivec2 puv = ivec2(gl_FragCoord.xy);
    const vec2 ndc = uv_to_ndc((vec2(puv) + 0.5) / screen_size);

    const float depth = texelFetch(depth_tex, puv, 0).r;
    if (depth == 0.0)
    {
        out_light_buffer = vec4(0.0);
        return;
    }

    const vec3 albedo = texelFetch(albedo_tex, puv, 0).rgb;
    const uvec2 light_info = texelFetch(light_info_tex, puv, 0).rg;

    const vec4 W_pos_h = VP_inv * vec4(ndc, depth, 1.0);
    const vec3 W_pos = W_pos_h.xyz / W_pos_h.w;

    const vec3 W_light_vec = light_position - W_pos;
    const vec3 W_light_dir = normalize(W_light_vec);
    const vec3 W_view_dir = normalize(eye_position - W_pos);

    const vec3 light_dir_local = normalize((M_inv * vec4(W_light_dir, 0.0)).xyz);
    const float spot_cos = light_dir_local.z;
    const float angle_falloff = smoothstep(outer_cone_cos, inner_cone_cos, spot_cos);

    const float range_sqr = range * range;
    const float distance_sqr = dot(W_light_vec, W_light_vec);
    const float range_falloff = clamp(1.0 - distance_sqr * distance_sqr / (range_sqr * range_sqr), 0.0, 1.0);
    const vec3 radiance = rel_intensity * range_falloff * angle_falloff / distance_sqr;

    const GBufferLighting gbuffer = unpack_gbuffer_lighting(light_info);
    const vec3 brightness = gltf_calculate_pbr(
            W_light_dir, radiance,
            W_view_dir,
            gbuffer.normal,
            albedo, gbuffer.roughness, gbuffer.metalness
        );

    out_light_buffer = vec4(clamp(brightness, 0.0, 65000.0), 1.0);
}
