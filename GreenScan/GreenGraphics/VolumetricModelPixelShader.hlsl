#include "Header.hlsli"
static const float2 LightProperty = float2(1.f, 0.5f);
static const float Ambient = 0.1f, SpecularPower = 40.f;
static const float3 DiffuseColor = 0.6f, SpecularColor = 1.f;
float4 main(VertexPositionWorldNormal input) : SV_TARGET
{
	float diff = max(dot(-CameraPosition, input.Normal), 0.f);
	return float4(Ambient + diff * DiffuseColor, 1.f);
}