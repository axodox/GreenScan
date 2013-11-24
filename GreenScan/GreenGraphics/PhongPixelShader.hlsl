#include "Header.hlsli"
#include "Phong.hlsli"

float4 main(VertexPositionWorldNormalDepthTexture surface) : SV_TARGET
{
	float intensity = GetPhongIntensity(surface.World, surface.Normal, 0.f);
	return float4(intensity, intensity, intensity, 1.0f);
}