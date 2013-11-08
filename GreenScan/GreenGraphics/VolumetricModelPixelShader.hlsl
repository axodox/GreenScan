#include "Header.hlsli"
static const float3 Intensity = float3(0.1f, 0.8f, 0.4f);
static const float4 Reflectivity = float4(1.f, 1.f, 1.f, 20.f);

float4 main(VertexPositionWorldNormal surface) : SV_TARGET
{
	float3 dLight = normalize(CameraPosition - surface.World);
	float3 dCamera = normalize(CameraPosition - surface.World);
	float3 dReflected = reflect(-dLight, surface.Normal);
	float ambient = Reflectivity.x * Intensity.x;
	float diffuse = Reflectivity.y * max(dot(dLight, surface.Normal), 0.f) * Intensity.y;
	float specular = (diffuse > 0.f ? Reflectivity.z * pow(max(dot(dReflected, dCamera), 0.f), Reflectivity.w) * Intensity.z : 0.f);

	float intensity = ambient + diffuse + specular;
	return float4(intensity, intensity, intensity, 1.0f);
}