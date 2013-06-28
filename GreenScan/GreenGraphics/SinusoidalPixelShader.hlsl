#include "Header.hlsli"
float4 main(VertexPositionTextureDepth v) : SV_TARGET
{
	return float4(v.Depth, v.Depth, v.Depth, 1.f);
}