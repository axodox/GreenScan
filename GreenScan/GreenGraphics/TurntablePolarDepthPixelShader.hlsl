#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float4 data = Texture.Sample(Sampler, v.Texture);
	float i = data.x / data.y;
	if(data.y > 0.f)
		return float4(i, i, i, 1.f);
	else
		return 0.f;
}