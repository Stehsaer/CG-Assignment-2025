#ifndef _NDC_UV_CONV_GLSL_
#define _NDC_UV_CONV_GLSL_

precision highp float;

vec2 ndc_to_uv(vec2 ndc)
{
    return fma(ndc, vec2(0.5, -0.5), vec2(0.5));
}

vec2 uv_to_ndc(vec2 uv)
{
    return fma(uv, vec2(2.0, -2.0), vec2(-1.0, 1.0));
}

#endif