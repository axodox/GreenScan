#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float4 data = Texture.Sample(Sampler, v.Texture);
	return float4(data.x / data.y, data.z / data.z, 0.f, 1.f);
}