#include "Header.hlsli"

float4 main(VertexPositionOut input) : SV_TARGET
{
	return float4(input.Position.z, 1.0f, 1.0f, 1.0f);
}