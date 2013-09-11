#include "Header.hlsli"

VertexPositionWorld main(float3 Position : POSITION0)
{
	float4 posTemp;
	Position.y -= CubeSize.x / 2.f;
	posTemp = mul(float4(Position, 1.f), ModelToScreenTransform);
	posTemp.xy = (posTemp.xy / (posTemp.z * DepthResolution) * 2.f - 1.f) * AspectScale * float2(1.f, -1.f);
	posTemp = mul(posTemp, SceneRotation);
	posTemp.xy = (posTemp.xy + Move) * Scale;
	posTemp.z /= MaxDepth;

	VertexPositionWorld output;
	output.Position = posTemp;
	output.World = Position;
	return output;
}