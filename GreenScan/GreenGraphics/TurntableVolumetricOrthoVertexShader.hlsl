#include "Header.hlsli"
Texture2D<float> DepthTexture : register(t0);

float3 PosOf(float x, float y)
{
	int3 id = DepthCoords(float2(x, y));
	float depth = DepthTexture.Load(id);
	return mul(float4(DepthSize.x * x * depth, DepthSize.y * y * depth, depth, 1), DepthInvIntrinsics).xyz;
}

bool CheckTriangle(float2 uv)
{
	float3 posx = PosOf(uv.x, uv.y);
	float3 posu = PosOf(uv.x, uv.y + DepthStep.y);
	float3 posd = PosOf(uv.x, uv.y - DepthStep.y);
	float3 posl = PosOf(uv.x - DepthStep.x, uv.y);
	float3 posr = PosOf(uv.x + DepthStep.x, uv.y);
	return pow(length(posu - posd) / posx.z, 2) + pow(length(posr - posl) / posx.z, 2) > TriangleLimit;
}

VertexPositionOut main(VertexPositionTextureIn vi)
{
	int3 id = int3((int)(vi.Texture.x * DepthResolution.x), (int)(vi.Texture.y * DepthResolution.y), 0);
	float depth = DepthTexture.Load(id);

	float4 posDepth = float4(
		DepthResolution.x * vi.Texture.x * depth,
		DepthResolution.y * vi.Texture.y * depth,
		depth,
		1);

	float4 posWorld = mul(posDepth, DepthToWorldTransform);

	float4 posOutput;
	posOutput.x = posWorld.x / CubeSize.y * 2.f;
	posOutput.y = posWorld.y / CubeSize.x * 2.f - 1.f;
	posOutput.z = posWorld.z / CubeSize.y + 0.5f;
	posOutput.w = 1.f;

	if (depth == 0.f || CheckTriangle(vi.Texture)) posOutput.z = -1.f;

	VertexPositionOut output;
	output.Position = posOutput;
	return output;
}