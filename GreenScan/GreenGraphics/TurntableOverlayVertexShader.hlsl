#include "Header.hlsli"

VertexPositionColorOut main(VertexPositionColorIn vi)
{
	float4 posTemp;
	posTemp = mul(float4(vi.Position, 1), TurntableToScreenTransform);
	posTemp.xy = (posTemp.xy / (posTemp.z * DepthResolution) * 2.f - 1.f) * AspectScale * float2(1.f, -1.f);
	posTemp = mul(posTemp, SceneRotation);
	posTemp.xy = (posTemp.xy + Move) * Scale;
	posTemp.z /= MaxDepth;	
	float4 posScreen = posTemp;

	VertexPositionColorOut vo;
	vo.Position = posScreen;
	vo.Color = vi.Color;
	return vo;
}