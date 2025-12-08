vec2 normalToOct(vec3 normal)
{
    vec3 normalized = normal / (abs(normal.x) + abs(normal.y) + abs(normal.z));
    return normal.y >= 0.0 ? normalized.xz : vec2(
        sign(normalized.x) * (1.0 - abs(normalized.x)),
        sign(normalized.z) * (1.0 - abs(normalized.z))
    )
}

vec3 octToNormal(vec2 oct)
{
    vec3 normal = vec3(oct.x, oct.y, 1.0 - abs(oct.x) - abs(oct.y));
    if (normal.z < 0.0)
    {
        normal.x = (1.0 - abs(normal.y)) * sign(normal.x);
        normal.y = (1.0 - abs(normal.x)) * sign(normal.y);
    }
    return normalize(normal);
}
