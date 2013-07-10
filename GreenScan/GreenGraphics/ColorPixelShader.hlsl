#include "Header.hlsli"
Texture2D ColorTexture  : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float4 color = ColorTexture.Sample(Sampler, v.Texture).bgra;
	color.a = 1.f;
	return color;
}