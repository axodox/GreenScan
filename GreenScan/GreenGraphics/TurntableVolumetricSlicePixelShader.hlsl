#include "Header.hlsli"
Texture3D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float c = Texture.Sample(Sampler, float3(v.Texture, Slice)).x;
	if (c < Threshold)
		return 1.f;
	else
		return float4(c, 1.f - c, 0.f, 0.f);
}