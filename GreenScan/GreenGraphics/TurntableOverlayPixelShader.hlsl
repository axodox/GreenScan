#include "Header.hlsli"

float4 main(VertexPositionColorOut v) : SV_TARGET
{
	return v.Color;
}