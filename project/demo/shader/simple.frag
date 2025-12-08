#version 460

#extension GL_GOOGLE_include_directive : enable
#include "include/tonemap/agx.glsl"

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec3 in_normal;

layout (location = 0) out vec4 out_color;

layout (set = 2, binding = 0) uniform sampler2D tex;

void main()
{
	float brightness = max(dot(in_normal, vec3(-1.0, 1.0, -1.0)), 0.0);
	out_color = vec4(
		pow(
			tonemap(texture(tex, in_uv).rgb * vec3(0.05 + brightness)), 
			vec3(1/2.2)
		)
	, 1.0);
}