#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionWorldNormalDepthTexture v) : SV_TARGET
{
	float4 color = Texture.Sample(Sampler, (v.Texture + ColorMove) * ColorScale).bgra;
	color.a = 1.f;
	return color;
}