#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureDepth v) : SV_TARGET
{
	return Texture.Sample(Sampler, (v.Texture + ColorMove) * ColorScale).bgra;
}