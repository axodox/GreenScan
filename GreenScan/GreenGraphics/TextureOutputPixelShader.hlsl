#include "Header.hlsli"
Texture2D<float4> VertexTexture : register(t0);
Texture2D ColorTexture : register(t1);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	int3 id = SaveCoords(v.Texture);
	float4 vertex = VertexTexture.Load(id);

	if(vertex.w == 0.f) return 0.f;
	vertex.y = -vertex.y;

	float4 temp;
	temp = mul(vertex, WorldToColorTransform);
	temp.xyz /= temp.z;
	temp.xy /= ColorSize;
	float2 uv = temp.xy;

	if(uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f) return 0.f;
	
	float4 color = ColorTexture.Sample(Sampler, (uv + ColorMove) * ColorScale).bgra;
	color.a = 1.f;
	return color;
}