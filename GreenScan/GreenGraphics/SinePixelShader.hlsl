#include "Header.hlsli"
float4 main(VertexPositionWorldNormalDepthTexture v) : SV_TARGET
{
	float i = sin((v.Depth / ShadingPeriode + ShadingPhase) * Pi * 2.f) / 2.f + 0.5f;
	return float4(i, i, i, 1.f);
}