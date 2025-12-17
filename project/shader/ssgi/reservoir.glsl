#ifndef _RESERVOIR_GLSL_
#define _RESERVOIR_GLSL_

#extension GL_GOOGLE_include_directive : enable
#include "../common/oct.glsl"
#include "../common/pbr.glsl"

precision highp float;

struct Sample
{
    vec3 hit_position;
    vec3 hit_normal;
    vec3 hit_luminance;
    vec3 start_position;
    vec3 start_normal;
};

struct Reservoir
{
    /* Hit information */

    Sample z;

    /* Reservoir data */

    float w;
    float M;
    float W;
};

Reservoir empty_reservoir()
{
    return Reservoir(
        Sample(
            vec3(0.0), vec3(0.0), vec3(0.0),
            vec3(0.0), vec3(0.0)
        ),
        0.0, 0.0, 0.0
    );
}

Reservoir decode_reservoir(vec4 tex1, uvec4 tex2, uvec4 tex3, vec4 tex4)
{
    Reservoir reservoir;

    reservoir.w = clamp(tex1.x, 0.0, 1048576.0);
    reservoir.M = clamp(tex1.y, 0.0, 1048576.0);
    reservoir.W = clamp(tex1.z, 0.0, 1048576.0);

    reservoir.z.hit_normal = octToNormal(unpackSnorm2x16(tex2.w));
    reservoir.z.hit_position = vec3(
            uintBitsToFloat(tex2.x),
            uintBitsToFloat(tex2.y),
            uintBitsToFloat(tex2.z)
        );

    reservoir.z.start_normal = octToNormal(unpackSnorm2x16(tex3.w));
    reservoir.z.start_position = vec3(
            uintBitsToFloat(tex3.x),
            uintBitsToFloat(tex3.y),
            uintBitsToFloat(tex3.z)
        );

    reservoir.z.hit_luminance = tex4.rgb;

    return reservoir;
}

void encode_reservoir(Reservoir reservoir, out vec4 tex1, out uvec4 tex2, out uvec4 tex3, out vec4 tex4)
{
    tex1 = vec4(reservoir.w, reservoir.M, reservoir.W, 0.0);

    tex2 = uvec4(
            floatBitsToUint(reservoir.z.hit_position.x),
            floatBitsToUint(reservoir.z.hit_position.y),
            floatBitsToUint(reservoir.z.hit_position.z),
            packSnorm2x16(normalToOct(reservoir.z.hit_normal))
        );

    tex3 = uvec4(
            floatBitsToUint(reservoir.z.start_position.x),
            floatBitsToUint(reservoir.z.start_position.y),
            floatBitsToUint(reservoir.z.start_position.z),
            packSnorm2x16(normalToOct(reservoir.z.start_normal))
        );

    tex4 = vec4(reservoir.z.hit_luminance, 0.0);
}

float p_hat_at(Sample z, vec3 W_pos, vec3 W_normal, vec3 W_view_dir, float roughness)
{
    return dot(z.hit_luminance.rgb, vec3(0.2126, 0.7152, 0.0722));
}

bool update_reservoir(
    inout Reservoir reservoir,
    Sample hit_sample,
    float w_new,
    float noise
)
{
    reservoir.M += 1.0;
    reservoir.w += w_new;

    if (reservoir.w == 0 || noise < w_new / reservoir.w)
    {
        reservoir.z = hit_sample;
        return true;
    }

    return false;
}

void clamp_reservoir(inout Reservoir reservoir, float M_max)
{
    if (reservoir.M > M_max)
    {
        float scale = M_max / reservoir.M;
        reservoir.w = clamp(reservoir.w * scale, 0.0, 1048576.0);
        reservoir.M = M_max;
    }
}

float jacobian_determinant(
    vec3 W_hit_normal,
    vec3 W_hit_pos,
    vec3 W_prev_start_pos,
    vec3 W_start_pos
)
{
    const vec3 prev_dir = W_prev_start_pos - W_hit_pos;
    const vec3 curr_dir = W_start_pos - W_hit_pos;

    W_hit_normal = normalize(W_hit_normal);

    const float prev_cos_phi = abs(dot(normalize(prev_dir), W_hit_normal));
    const float curr_cos_phi = abs(dot(normalize(curr_dir), W_hit_normal));

    return (curr_cos_phi / prev_cos_phi) * (dot(prev_dir, prev_dir) / dot(curr_dir, curr_dir));
}

void merge_reservoir(inout Reservoir target, Reservoir source, float p_hat, float noise, float roughness, vec3 W_curr_pos, vec3 W_curr_normal, vec3 W_view_dir)
{
    // Aggregate source contribution as if source.M independent samples
    float w_new = p_hat * source.W * source.M; // aggregated importance from source

    float w_before = target.w;
    float w_after = w_before + w_new;

    // Selection probability for replacing the target sample with the source's sample
    float selection_prob = (w_after > 0.0) ? (w_new / w_after) : 0.0;
    bool updated = (w_after == 0.0) || (noise < selection_prob);

    if (updated)
    {
        target.z = source.z;
    }

    target.w = w_after;
    target.M = target.M + source.M;

    if (updated)
    {
        float denom = target.M * p_hat_at(target.z, W_curr_pos, W_curr_normal, W_view_dir, roughness);
        if (denom > 0.0)
            target.W = target.w / denom;
        else
            target.W = 0.0;
    }
}

#endif
