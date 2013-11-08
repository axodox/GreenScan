#include "Header.hlsli"

VertexPositionWorldNormal main(VertexPositionNormal vi)
{
	float4 posTemp;
	vi.Position.y = -vi.Position.y + CubeSize.x / 2.f;
	posTemp = mul(float4(vi.Position, 1.f), ModelToScreenTransform);
	posTemp.xy = (posTemp.xy / (posTemp.z * DepthResolution) * 2.f - 1.f) * AspectScale * float2(1.f, -1.f);
	posTemp = mul(posTemp, SceneRotation);
	posTemp.xy = (posTemp.xy + Move) * Scale;
	posTemp.z /= MaxDepth;

	VertexPositionWorldNormal output;
	output.Position = posTemp;
	output.World = vi.Position;
	output.Normal = vi.Normal * float3(1.f, -1.f, 1.f);
	return output;
}