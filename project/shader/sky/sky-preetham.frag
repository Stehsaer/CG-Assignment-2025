// Algorithm adapted from: https://github.com/diharaw/SkyModels

#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 ndc;

layout(location = 0) out vec4 out_color;

#extension GL_GOOGLE_include_directive : enable
#include "../common/ndc-uv-conv.glsl"

layout(std140, set = 3, binding = 0) uniform Params
{
    mat4 camera_mat_inv;
    vec2 screen_size;
    vec3 eye_position;
    vec3 sun_dir;
    vec3 sun_intensity;
    vec3 p_A, p_B, p_C, p_D, p_E, p_Z;
};

vec3 perez(float cos_theta, float gamma, float cos_gamma, vec3 A, vec3 B, vec3 C, vec3 D, vec3 E)
{
    return (1 + A * exp(B / (cos_theta + 0.01))) * (1 + C * exp(D * gamma) + E * cos_gamma * cos_gamma);
}

vec3 preetham_sky_rgb(vec3 v, vec3 sun_dir)
{
    float cos_theta = clamp(v.y, 0, 1);
    float cos_gamma = dot(v, sun_dir);
    float gamma = acos(cos_gamma);

    vec3 R_xyY = p_Z * perez(cos_theta, gamma, cos_gamma, p_A, p_B, p_C, p_D, p_E);

    vec3 R_XYZ = vec3(R_xyY.x, R_xyY.y, 1 - R_xyY.x - R_xyY.y) * R_xyY.z / R_xyY.y;

    // Radiance
    float r = dot(vec3(3.240479, -1.537150, -0.498535), R_XYZ);
    float g = dot(vec3(-0.969256, 1.875992, 0.041556), R_XYZ);
    float b = dot(vec3(0.055648, -0.204043, 1.057311), R_XYZ);

    return vec3(r, g, b);
}

void main()
{
    vec4 view_dir_h = camera_mat_inv * vec4(ndc.xy, 0.5, 1.0);
    vec3 view_dir = normalize(view_dir_h.xyz / view_dir_h.w - eye_position);

    out_color = vec4(preetham_sky_rgb(view_dir, sun_dir) * sun_intensity, 1.0);
}
