//static const float3 Intensity = float3(0.1f, 0.8f, 0.4f);
static const float3 Reflectivity = float3(1.f, 1.f, 1.f);

cbuffer LightingConstants : register(b3)
{
	float3 Intensity;
	float Shininess;
};

float GetPhongIntensity(float3 position, float3 normal, float3 cameraPosition) : SV_TARGET
{
	float3 dLight = normalize(cameraPosition - position);
	float3 dCamera = normalize(cameraPosition - position);
	float3 dReflected = reflect(-dLight, normal);
	float ambient = Reflectivity.x * Intensity.x;
	float diffuse = Reflectivity.y * max(dot(dLight, normal), 0.f) * Intensity.y;
	float specular = (diffuse > 0.f ? Reflectivity.z * pow(max(dot(dReflected, dCamera), 0.f), Shininess) * Intensity.z : 0.f);

	return ambient + diffuse + specular;
}