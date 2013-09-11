#include "Header.hlsli"
Texture2D<float> DepthTexture : register(t0);
SamplerState Sampler : register(s0);

VertexPositionInstance main(VertexPositionTextureInstance vi)
{
	int3 id = int3((int)(vi.Texture.x * CubeRes), vi.Instance, 0);
	float depth = DepthTexture.Load(id);

	float4 worldPos = float4(
		vi.Position.x * CubeSize.y / 2.f,
		vi.Instance / (CubeRes - 1.f) * CubeSize.x,
		depth * CubeSize.y / 2.f,
		1.f);

	float4 turntablePos = mul(worldPos, WorldToTurntableTransform);

	float4 outputPos = float4(
		turntablePos.x / CubeSize.x * 2.f,
		turntablePos.z / CubeSize.x * 2.f,
		1.f,
		1.f);

	if (depth == 0.f) outputPos.z = 0.f;

	VertexPositionInstance vo;
	vo.Position = outputPos;
	vo.Instance = CubeRes - vi.Instance - 1;
	return vo;
}