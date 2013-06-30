#include "Header.hlsli"
static const float2 LightProperty = float2(1.f, 0.5f);
static const float Ambient = 0.1f, SpecularPower = 40.f;
static const float3 DiffuseColor = 0.5f, SpecularColor = 1.f;
static const float3 CameraPosition = 0.f,  LightPosition = 0.f;
float4 main(VertexPositionTextureDepth input) : SV_TARGET
{
	float4 color = 1;
	float3 specular = 0, diffuse = 0, normal = input.Normal/sign(input.Normal.z);
	float3 V = normalize(input.WorldPosition - CameraPosition);
	float l, nl, d;
	float3 L = normalize(input.WorldPosition - LightPosition);
	l = L.x * L.x + L.y * L.y + L.z * L.z;
	nl = dot(normal, L); 
	d = max(nl * LightProperty.x, 0) / l;
	diffuse += d;
	if(d > 0)
	{
		specular += max(0, pow(dot(normalize(V + L), normal), SpecularPower) * LightProperty.y / l);
	}
	return float4(color.rgb * (Ambient + diffuse * DiffuseColor + specular * SpecularColor), 1);
}