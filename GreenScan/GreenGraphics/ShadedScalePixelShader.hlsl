#include "Header.hlsli"
Texture1D ScaleTexture : register(t0);
SamplerState ScaleSampler : register(s0);

float4 main(VertexPositionTextureDepth v) : SV_TARGET
{
	float shade = abs(v.Normal.z);
	if(shade < 0.f || shade > 1.f)
		return 0.f;
	else
	{
		float4 color = ScaleTexture.Sample(ScaleSampler, v.Depth / DepthLimit) * (0.5f + shade / 4.f);
		color.a = 1.f;
		return color;
	}
}