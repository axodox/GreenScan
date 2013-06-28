#include "Header.hlsli"
float4 main(VertexPositionTextureDepth v) : SV_TARGET
{
	float i = sin(v.Depth);
	return float4(i, i, i, 1.f);
}