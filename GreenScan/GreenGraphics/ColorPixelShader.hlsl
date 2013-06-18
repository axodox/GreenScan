#include "Header.hlsli"
Texture2D ColorTexture  : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	return ColorTexture.Sample(Sampler, v.Texture).bgra;
}