#include "Header.hlsli"
Texture3D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float c = Texture.Sample(Sampler, float3(v.Texture, Slice / (CubeRes - 1.f))).x;
	if (c == 0.f)
		return 1.f;
	else
		return float4(c, 1.f - c, 0.f, 0.f);
}