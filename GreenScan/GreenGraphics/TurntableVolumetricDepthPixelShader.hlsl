#include "Header.hlsli"
Texture2D<float> Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float depth = Texture.Sample(Sampler, v.Texture);
	if (depth != 0)
		return float4(depth, 0.f, 1.f - depth, 1.f);
	else
		return 1.f;
}