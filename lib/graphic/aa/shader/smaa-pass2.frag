#version 460

layout(set = 2, binding = 0) uniform sampler2D edge_tex;
layout(set = 2, binding = 1) uniform sampler2D area_lut;

layout(location = 0) out vec4 out_color;

/*==========*/

#define MAX_SEARCH_STEPS 8

vec2 texture_size = vec2(textureSize(edge_tex, 0));

/*==========*/

float luma(in vec3 color)
{
    return dot(color, vec3(0.21, 0.72, 0.07));
}

vec2 get_edge(vec2 offset)
{
    return textureLod(edge_tex, (gl_FragCoord.xy + offset) / texture_size, 0.0).rg;
}

float find_horizontal(vec2 offset, const vec2 increment)
{
    float e;
    int i;

    for (i = 0; i < MAX_SEARCH_STEPS; i++)
    {
        e = get_edge(offset).g;
        if (e < 0.9) break;
        offset += increment;
    }

    return 2.0 * min(i + e, MAX_SEARCH_STEPS);
}

float find_vertical(vec2 offset, const vec2 increment)
{
    float e;
    int i;

    for (i = 0; i < MAX_SEARCH_STEPS; i++)
    {
        e = get_edge(offset).r;
        if (e < 0.9) break;
        offset += increment;
    }

    return 2.0 * min(i + e, MAX_SEARCH_STEPS);
}

vec2 get_precomputed_area(float neg, float pos, float neg_edge, float pos_edge)
{
    vec2 texcoord = fma(
        round(4.0 * vec2(neg_edge, pos_edge)), 
        vec2(MAX_SEARCH_STEPS * 2 + 1), 
        vec2(neg, pos)
    );
    return texelFetch(area_lut, ivec2(texcoord), 0).rg;
}

void main()
{
    vec2 current_edge = texelFetch(edge_tex, ivec2(floor(gl_FragCoord.xy)), 0).rg;

    vec4 blend_factor = vec4(0.0);

    // Left edge found, search vertically
    if (current_edge.r > 0.5)
    {
        float top = find_vertical(vec2(0.0, -1.5), vec2(0.0, -2.0));
        float bottom = find_vertical(vec2(0.0, 1.5), vec2(0.0, 2.0));

        float top_edge = get_edge(vec2(-0.25, -top)).g;
        float bottom_edge = get_edge(vec2(-0.25, bottom + 1.0)).g;

        blend_factor.ba = get_precomputed_area(top, bottom, top_edge, bottom_edge);
    }

    // Top edge found, search horizontally
    if (current_edge.g > 0.5)
    {
        float left = find_horizontal(vec2(-1.5, 0.0), vec2(-2.0, 0.0));
        float right = find_horizontal(vec2(1.5, 0.0), vec2(2.0, 0.0));

        float left_edge = get_edge(vec2(-left, -0.25)).r;
        float right_edge = get_edge(vec2(right + 1.0, -0.25)).r;

        blend_factor.rg = get_precomputed_area(left, right, left_edge, right_edge);
    }

    out_color = blend_factor;
}
