struct HSV
{
	float h, s, v;

	float3 as_rgb()
	{
		const float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
		float3 p = abs(fract(float3(h, h, h) + K.xyz) * 6.0 - K.www);
		return v * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), s);
	}
}

struct VertexInput
{
	float2 position;
}

struct VertexOutput
{
	float4 position : SV_Position;
	float height;
}

struct FragmentInput
{
	float height;
}

struct FragmentOutput
{
	float4 color;
}

struct Transform
{
	float4x4 view_projection;
	float4x4 control_point;
}

float4 precompute_bezier_coeff(float t)
{
	float u = 1.0 - t;
	return float4(u * u * u, 3.0 * u * u * t, 3.0 * u * t * t, t * t * t);
}

layout(set = 1, binding = 0) ConstantBuffer<Transform, Std140DataLayout> transform;

[shader("vertex")]
VertexOutput vs_main(VertexInput input)
{
	VertexOutput output;

	const float2 uv = input.position * 0.5 + 0.5;

	const float4 bezier_x = precompute_bezier_coeff(uv.x);
	const float4 bezier_y = precompute_bezier_coeff(uv.y);

	const float a0 = dot(bezier_x, transform.control_point[0]);
	const float a1 = dot(bezier_x, transform.control_point[1]);
	const float a2 = dot(bezier_x, transform.control_point[2]);
	const float a3 = dot(bezier_x, transform.control_point[3]);
	const float height = dot(bezier_y, float4(a0, a1, a2, a3));

	output.position =
		mul(transform.view_projection, float4(input.position.x, height, input.position.y, 1.0));
	output.height = height;

	return output;
}

[shader("fragment")]
FragmentOutput fs_main(FragmentInput input)
{
	const float hue =
		lerp(0, 2.0 / 3.0, (input.height - (-1.0)) / (1.0 - (-1.0)));  // Map height from [-1, 1] to [0, 2/3]
	return FragmentOutput(float4(HSV(hue, 1.0, 1.0).as_rgb(), 1.0));
}
