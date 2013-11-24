#include "Header.hlsli"
#include "Phong.hlsli"
Texture1D ScaleTexture : register(t0);
SamplerState ScaleSampler : register(s0);

float4 main(VertexPositionWorldNormalDepthTexture surface) : SV_TARGET
{
	float intensity = GetPhongIntensity(surface.World, surface.Normal, 0.f);
	float3 color = ScaleTexture.Sample(ScaleSampler, surface.Depth / ShadingPeriode + ShadingPhase).xyz;
	return float4(color * intensity, 1.0f);
}