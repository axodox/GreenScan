#include "Header.hlsli"
static const float2 LightProperty = float2(1.f, 0.5f);
static const float Ambient = 0.1f, SpecularPower = 40.f;
static const float3 DiffuseColor = 0.5f, SpecularColor = 1.f;
static const float3 CameraPos = 0.f,  LightPos = 0.f;
float4 main(VertexPositionWorldNormalDepthTexture input) : SV_TARGET
{
	float4 color = 1.f;
	float3 specular = 0.f, diffuse = 0.f, normal = input.Normal/sign(input.Normal.z);
	float3 V = normalize(input.WorldPosition - CameraPos);
	float l, nl, d;
	float3 L = normalize(input.WorldPosition - LightPos);
	l = L.x * L.x + L.y * L.y + L.z * L.z;
	nl = dot(normal, L); 
	d = max(nl * LightProperty.x, 0.f) / l;
	diffuse += d;
	if(d > 0.f)
	{
		specular += max(0.f, pow(dot(normalize(V + L), normal), SpecularPower) * LightProperty.y / l);
	}
	return float4(color.rgb * (Ambient + diffuse * DiffuseColor + specular * SpecularColor), 1.f);
}