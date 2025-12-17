#ifndef _PBR_GLSL_
#define _PBR_GLSL_

precision highp float;

#extension GL_GOOGLE_include_directive : enable
#include "constant.glsl"

float gltf_microfacet_distribution(float n_dot_h, float alpha)
{
    float nominator = alpha * alpha;
    float f = n_dot_h * n_dot_h * (alpha * alpha - 1.0) + 1.0;
    float denominator = PI * f * f;

    return nominator / denominator;
}

float gltf_mask_shadowing(float vdot, float alpha)
{
    float nominator = mix(0.0001, 2.0, vdot);
    float alpha2 = alpha * alpha;
    float denominator = vdot + sqrt(alpha2 + (1.0 - alpha2) * vdot * vdot);

    return nominator / denominator;
}

float gltf_geometry_occlusion(float n_dot_l, float n_dot_v, float alpha)
{
    return gltf_mask_shadowing(n_dot_l, alpha) * gltf_mask_shadowing(n_dot_v, alpha);
}

vec3 gltf_calculate_pbr(vec3 light_dir, vec3 light_intensity, vec3 view_dir, vec3 normal_dir, vec3 albedo, float roughness, float metalness)
{
    vec3 halfway_dir = normalize(light_dir + view_dir);

    float
    n_dot_v = max(0.0, abs(dot(view_dir, normal_dir))),
    n_dot_l = max(0.0, dot(normal_dir, light_dir)),
    n_dot_h = max(0.0, dot(normal_dir, halfway_dir)),
    v_dot_h = max(0.0, dot(view_dir, halfway_dir));

    vec3 c_diff = mix(albedo, vec3(0.0), metalness);
    vec3 f0 = mix(vec3(0.04), albedo, metalness);
    float alpha = roughness * roughness;

    vec3 F = f0 + (1.0 - f0) * pow(1.0 - v_dot_h, 5.0);

    vec3 f_diffuse = (1.0 - F) * (1.0 / PI) * c_diff;
    vec3 f_specular = F
            * gltf_microfacet_distribution(n_dot_h, alpha)
            * gltf_geometry_occlusion(n_dot_l, n_dot_v, alpha)
            / (4.0 * n_dot_v * n_dot_l + 0.0001);

    return (f_diffuse + f_specular) * n_dot_l * light_intensity;
}

float gltf_pbr_pdf(vec3 out_ray, vec3 in_ray, float roughness, vec3 normal)
{
    vec3 halfway_dir = normalize(out_ray + in_ray);
    float cos_theta = max(0.0, dot(normal, halfway_dir));
    float pdf = (gltf_microfacet_distribution(cos_theta, roughness * roughness) * cos_theta) / (4.0 * dot(in_ray, halfway_dir) + 0.0001);
    return pdf;
}

#endif