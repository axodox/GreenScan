#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float4 data = Texture.Sample(Sampler, v.Texture);
	if(data.w > 0.f)
		return data / data.w;
	else
		return 0.f;
}