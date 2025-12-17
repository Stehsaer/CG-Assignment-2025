#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 ndc;

layout(location = 0) out vec4 out_radiance;

layout(set = 2, binding = 0) uniform sampler2D curr_radiance_tex;

void main()
{
    out_radiance = textureLod(curr_radiance_tex, uv, 0);
}
