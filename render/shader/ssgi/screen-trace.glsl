#ifndef _SCREEN_TRACE_GLSL_
#define _SCREEN_TRACE_GLSL_

precision highp float;

#extension GL_GOOGLE_include_directive : enable
#include "../common/ndc-uv-conv.glsl"
#include "../common/constant.glsl"

const int MAX_HIZ_LEVEL = 8;

struct Hit_result
{
    bool hit;
    int iter;
    vec2 uv;
    vec3 V_hit_pos;
};

Hit_result no_hit(int iter)
{
    return Hit_result(false, iter, vec2(0.0), vec3(0.0));
}

Hit_result make_hit(int iter, vec2 uv, vec3 V_hit_pos)
{
    return Hit_result(true, iter, uv, V_hit_pos);
}

// Calculate the intersection point of ray1 (origin at (0,0,0)) and ray2 (origin at ray2_origin)
vec3 ray_intersection(vec3 ray1_dir, vec3 ray2_origin, vec3 ray2_dir)
{
    const float a = dot(ray1_dir, ray1_dir);
    const float b = dot(ray1_dir, ray2_dir);
    const float c = dot(ray2_dir, ray2_dir);
    const float d = dot(ray1_dir, ray2_origin);
    const float e = dot(ray2_dir, ray2_origin);

    const float t1 = (-c * d + b * e) / (b * b - a * c);
    return ray1_dir * t1;
}

// Normalize a PUV space direction to a unit step in PUV space
vec2 dda_step_direction(vec2 dir)
{
    const vec2 abs_dir = abs(dir.xy);
    return abs_dir.x > abs_dir.y
    ? vec2(sign(dir.x), dir.y / abs_dir.x) : vec2(dir.x / abs_dir.y, sign(dir.y));
}

// Calculate PUV space march direction from V space march direction
vec2 uv_space_march_dir(vec3 V_march_dir, vec3 V_start, mat4 proj_mat, ivec2 resolution)
{
    vec4 clip0 = proj_mat * vec4(V_start, 1.0);
    clip0 /= clip0.w;

    const float base_step = 0.02;
    const float min_step = 0.1;
    float step_len = max(length(V_start) * base_step, min_step);
    vec3 stepped = V_start + V_march_dir * step_len;

    vec4 clip1 = proj_mat * vec4(stepped, 1.0);
    clip1 /= clip1.w;

    vec2 NDC_delta = clip1.xy - clip0.xy;

    vec2 PUV_delta = NDC_delta * vec2(0.5, -0.5) * vec2(resolution);
    return PUV_delta;
}

// Precondition: ray must intersect the rectangle, and d components are non-zero (dda step ensures this)
// rect = vec4(x_min, x_max, y_min, y_max)
vec2 ray_rect_intersect(vec2 P, vec2 d, vec4 rect)
{
    const vec4 ts = (rect - P.xxyy) / d.xxyy;

    vec2 t_enter = min(ts.xz, ts.yw);
    vec2 t_exit = max(ts.xz, ts.yw);

    float t_enter_d = max(t_enter.x, t_enter.y);
    float t_exit_d = min(t_exit.x, t_exit.y);

    return vec2(t_enter_d, t_exit_d);
}

vec4 get_hiz_pixelcoord_boundary(int hiz_level, ivec2 hiz_resolution, ivec2 hiz_coord)
{
    int hiz_size = 1 << hiz_level;
    bvec2 coord_at_edge = equal(hiz_coord + 1, hiz_resolution);
    vec2 size = mix(vec2(hiz_size), vec2(hiz_size << 1), vec2(coord_at_edge));
    return vec4(
        vec2(hiz_coord * hiz_size),
        vec2(hiz_coord * hiz_size) + size
    ).xzyw;
}

Hit_result raytrace(
    vec3 V_start,
    vec3 V_march_dir,
    ivec2 PUV_coord,
    ivec2 resolution_ivec,
    mat4 proj_mat,
    vec4 inv_proj_mat_col3,
    vec4 inv_proj_mat_col4,
    vec2 near_plane_span,
    float near_plane,
    sampler2D depth_tex,
    int max_iter
)
{
    const vec2 PUV_delta = uv_space_march_dir(V_march_dir, V_start, proj_mat, resolution_ivec);
    const vec2 PUV_unit_step = dda_step_direction(PUV_delta);

    vec2 PUV_traverse = vec2(PUV_coord) + 0.5;
    const vec2 PUV_initial = PUV_traverse;
    PUV_traverse += PUV_unit_step;

    int hiz_level = 0;

    const vec2 resolution_vec = vec2(resolution_ivec);
    const vec2 resolution_vec_inv = 1.0 / resolution_vec;

    float V_traverse_enter_depth;

    {
        const int hiz_level_size = 1 << hiz_level;
        const float curr_step = hiz_level_size;
        const ivec2 hiz_level_res = resolution_ivec >> hiz_level;

        /* Fetch Hi-Z Depth */

        const ivec2 UV_hiz_texel_coord = ivec2(floor(PUV_traverse)) >> hiz_level;
        const float P_hiz_depth = texelFetch(depth_tex, UV_hiz_texel_coord, hiz_level).r;

        /* Compute Intersection with Hi-Z Pixel Boundary */

        const vec4 hiz_boundary = get_hiz_pixelcoord_boundary(hiz_level, hiz_level_res, UV_hiz_texel_coord);
        const vec2 t_bounds = ray_rect_intersect(PUV_initial, PUV_unit_step, hiz_boundary);

        const float t_enter = t_bounds.x;

        const vec2 PUV_traverse_enter = PUV_unit_step * t_enter + PUV_initial;
        const vec2 UV_traverse_enter = PUV_traverse_enter * resolution_vec_inv;
        const vec3 V_traverse_enter_nearplane = vec3(uv_to_ndc(UV_traverse_enter) * near_plane_span, -near_plane);
        const vec3 V_traverse_enter = ray_intersection(V_traverse_enter_nearplane, V_start, V_march_dir);
        V_traverse_enter_depth = -V_traverse_enter.z;
    }

    for (int iter = 0; iter < max_iter; iter++)
    {
        /* Calculate Hi-Z Level Data */

        const int hiz_level_size = 1 << hiz_level;
        const float curr_step = hiz_level_size;
        const ivec2 hiz_level_res = resolution_ivec >> hiz_level;

        /* Fetch Hi-Z Depth */

        const ivec2 UV_hiz_texel_coord = ivec2(floor(PUV_traverse)) >> hiz_level;
        const float P_hiz_depth = texelFetch(depth_tex, UV_hiz_texel_coord, hiz_level).r;

        /* Compute Intersection with Hi-Z Pixel Boundary */

        const vec4 hiz_boundary = get_hiz_pixelcoord_boundary(hiz_level, hiz_level_res, UV_hiz_texel_coord);
        const vec2 t_bounds = ray_rect_intersect(PUV_initial, PUV_unit_step, hiz_boundary);

        const float t_enter = t_bounds.x;
        const float t_exit = t_bounds.y;

        const vec2 PUV_traverse_exit = PUV_unit_step * t_exit + PUV_initial;
        const vec2 UV_traverse_exit = PUV_traverse_exit * resolution_vec_inv;
        const vec3 V_traverse_exit_nearplane = vec3(uv_to_ndc(UV_traverse_exit) * near_plane_span, -near_plane);
        const vec3 V_traverse_exit = ray_intersection(V_traverse_exit_nearplane, V_start, V_march_dir);
        const float V_traverse_exit_depth = -V_traverse_exit.z;

        const vec4 V_hiz_depth = inv_proj_mat_col3 * P_hiz_depth + inv_proj_mat_col4;

        const vec2 UV_traverse = PUV_traverse * resolution_vec_inv;

        /* Precompute conditions */

        const int next_level = min(hiz_level + 1, MAX_HIZ_LEVEL);
        const int prev_level = max(hiz_level - 2, 0);
        const vec2 PUV_next_traverse = fma(t_bounds.yy + 0.05, PUV_unit_step, PUV_initial);

        const bool next_pixcoord_out_of_bounds = any(greaterThanEqual(PUV_next_traverse, resolution_vec))
                || any(lessThan(PUV_next_traverse, vec2(0.0)));

        const float V_hiz_depth_z = -V_hiz_depth.z / V_hiz_depth.w;

        const bool hiz_empty = P_hiz_depth == 0.0;
        const bool t_outofbounds = any(lessThan(t_bounds, vec2(0.0)));
        const bool hit_detected = V_hiz_depth_z <= max(V_traverse_exit_depth, V_traverse_enter_depth);

        // Empty space, increase HIZ level
        if (hiz_empty)
        {
            if (next_pixcoord_out_of_bounds)
                return no_hit(iter);

            hiz_level = next_level;
            PUV_traverse = PUV_next_traverse;
            V_traverse_enter_depth = V_traverse_exit_depth;

            continue;
        }

        if (t_outofbounds)
        {
            if (hiz_level == 0) return no_hit(iter);
            hiz_level = prev_level;
            continue;
        }

        if (hit_detected)
        {
            if (hiz_level == 0)
            {
                if (abs(V_traverse_enter_depth - V_hiz_depth_z) / V_traverse_enter_depth < 0.01)
                    return make_hit(iter, UV_traverse, V_traverse_exit);
                else
                    return no_hit(iter);
            }

            hiz_level = prev_level;
            continue;
        }

        if (next_pixcoord_out_of_bounds) return no_hit(iter);
        
        PUV_traverse = PUV_next_traverse;
        hiz_level = next_level;
        V_traverse_enter_depth = V_traverse_exit_depth;
    }

    return no_hit(max_iter);
}

// Coarse screen-space visibility test between V_start and V_target (both in view space).
// Samples a small number of points along the segment and compares scene depth to detect occlusion.
// Returns true if the segment is largely free (no closer geometry detected), false if occluded.
bool coarse_screen_trace_visibility(
    vec3 V_start,
    vec3 V_target,
    ivec2 PUV_coord,
    ivec2 resolution_ivec,
    mat4 proj_mat,
    vec4 inv_proj_mat_col3,
    vec4 inv_proj_mat_col4,
    vec2 near_plane_span,
    float near_plane,
    sampler2D depth_tex
)
{
    Hit_result result = raytrace(
        V_start,
        normalize(V_target - V_start),
        PUV_coord,
        resolution_ivec,
        proj_mat,
        inv_proj_mat_col3,
        inv_proj_mat_col4,
        near_plane_span,
        near_plane,
        depth_tex,
        24
    );

    if (result.iter >= 24)
        return true;
        
    if (!result.hit)
        return false;

    const float total_distance = length(V_target - V_start);
    const float error_margin = max(0.03, total_distance * 0.1);

    if (length(result.V_hit_pos - V_target) < error_margin)
    {
        return true;
    }

    return false;
}

#endif
