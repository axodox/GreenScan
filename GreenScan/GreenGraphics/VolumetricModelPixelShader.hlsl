#include "Header.hlsli"
#include "Phong.hlsli"

float4 main(VertexPositionWorldNormal surface) : SV_TARGET
{
	float intensity = GetPhongIntensity(surface.World, surface.Normal, CameraPosition);
	return float4(intensity, intensity, intensity, 1.0f);
}