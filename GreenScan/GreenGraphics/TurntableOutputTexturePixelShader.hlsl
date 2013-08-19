#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float4 color = Texture.Sample(Sampler, v.Texture);
	if(color.a > 0.f)
		return color / color.a;
	else
		return 0.f;
}