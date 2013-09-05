#include "Header.hlsli"
Texture2D<float> Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float depth = Texture.Sample(Sampler, v.Texture);
	float i = (depth + 1.f) / 2.f;
	if(depth != 0 && i > 0.f && i < 1.f)
		return float4(i, 0, 0, 1.f);
	else
		return 1.f;
}