struct VertexInput
{
	float2 position;
	float4 color;
}

struct VertexOutput
{
	float4 position : SV_Position;
	float4 color;
}

struct FragmentInput
{
	float4 color;
}

struct FragmentOutput
{
	float4 color;
}

struct Transform
{
	float4x4 view_projection;
}

layout(set = 1, binding = 0) ConstantBuffer<Transform> transform;

[shader("vertex")]
VertexOutput vs_main(VertexInput input)
{
	VertexOutput output;

	output.position = mul(transform.view_projection, float4(input.position, 0.0, 1.0));
	output.color = input.color;

	return output;
}

[shader("fragment")]
FragmentOutput fs_main(FragmentInput input)
{
	return FragmentOutput(input.color);
}
