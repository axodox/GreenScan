#include "Header.hlsli"
Texture2D InfraredTexture  : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float i = InfraredTexture.Sample(Sampler, v.Texture).r;
	return float4(i, i, i, 1.f);
}