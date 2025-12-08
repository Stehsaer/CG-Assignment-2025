#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;

layout(std140, set = 1, binding = 0) uniform Transform
{
    mat4 matrix;
} transform;

void main()
{
    out_uv = in_uv;
    out_normal = in_normal;

    vec4 pos = transform.matrix * vec4(in_pos, 1.0);

    gl_Position = pos;
}
