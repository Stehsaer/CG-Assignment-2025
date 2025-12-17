#ifndef _OCT_GLSL_
#define _OCT_GLSL_

precision highp float;

vec2 normalToOct(vec3 n)
{
    // project the normal onto the octahedron
    n = n / (abs(n.x) + abs(n.y) + abs(n.z));

    vec2 enc = n.xy; // store x and y components

    // reflect the folds of the lower hemisphere onto the upper one
    if (n.z < 0.0)
    {
        enc = (1.0 - abs(enc.yx)) * sign(enc);
    }

    return enc;
}

vec3 octToNormal(vec2 e)
{
    // reconstruct z and then unfold if it was folded
    vec3 n = vec3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
    if (n.z < 0.0)
    {
        n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
    }
    return normalize(n);
}

#endif