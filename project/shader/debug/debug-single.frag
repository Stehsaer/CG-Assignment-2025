#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 ndc;

layout(location = 0) out vec4 out_color;

layout(set = 2, binding = 0) uniform sampler2D input_texture;

layout(set = 3, binding = 0) uniform Params
{
    uint channel_count;
};

void main()
{
    switch(channel_count)
    {
        case 1:
            out_color = vec4(textureLod(input_texture, uv, 0).rrr, 1.0);
            return;
        case 2:
            out_color = vec4(textureLod(input_texture, uv, 0).rg, 0.0, 1.0);
            return;
        case 3:
            out_color = vec4(textureLod(input_texture, uv, 0).rgb, 1.0);
            return;
        case 4:
            out_color = textureLod(input_texture, uv, 0);
            return;
    }
}
