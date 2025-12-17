#ifndef _GBUFFER_STORAGE_GLSL_
#define _GBUFFER_STORAGE_GLSL_

precision highp float;

#extension GL_GOOGLE_include_directive : enable
#include "oct.glsl"

struct GBufferLighting
{
    float occlusion;
    float roughness;
    float metalness;
    vec3 normal;
};

uvec2 pack_gbuffer_lighting(GBufferLighting lighting)
{
    vec2 oct_encoded_normal = normalToOct(lighting.normal);
    uint packed_normal = packSnorm2x16(oct_encoded_normal);
    uint packed_material = packUnorm4x8(vec4(lighting.metalness, lighting.roughness, 0.0, lighting.occlusion));
    return uvec2(packed_normal, packed_material);
}

vec3 unpack_normal(uint packed)
{
    vec2 oct_encoded_normal = unpackSnorm2x16(packed);
    return octToNormal(oct_encoded_normal);
}

vec2 unpack_roughness_metalness(uint packed)
{
    vec4 material = unpackUnorm4x8(packed);
    return material.gr;
}

GBufferLighting unpack_gbuffer_lighting(uvec2 packed)
{
    vec3 normal = unpack_normal(packed.x);
    vec4 material = unpackUnorm4x8(packed.y);

    GBufferLighting lighting;

    lighting.normal = normal;
    lighting.metalness = material.r;
    lighting.roughness = material.g;
    lighting.occlusion = material.a;

    return lighting;
}

#endif