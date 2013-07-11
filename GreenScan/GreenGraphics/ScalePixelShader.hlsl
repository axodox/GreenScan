#include "Header.hlsli"
Texture1D ScaleTexture : register(t0);
SamplerState ScaleSampler : register(s0);

float4 main(VertexPositionTextureDepth v) : SV_TARGET
{
	return ScaleTexture.Sample(ScaleSampler, (v.Depth - DepthMinimum) / (DepthMaximum - DepthMinimum));
}