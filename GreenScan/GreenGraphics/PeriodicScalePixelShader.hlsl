#include "Header.hlsli"
Texture1D ScaleTexture : register(t0);
SamplerState ScaleSampler : register(s0);

float4 main(VertexPositionWorldNormalDepthTexture v) : SV_TARGET
{
	return ScaleTexture.Sample(ScaleSampler, v.Depth / ShadingPeriode + ShadingPhase);
}