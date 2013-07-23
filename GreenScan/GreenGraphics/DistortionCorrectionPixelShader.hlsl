#include "Header.hlsli"
Texture2D Texture : register(t0);
Texture2D CorrectionTexture : register(t1);
SamplerState Sampler : register(s0);

float main(VertexPositionTextureOut v) : SV_TARGET
{
	float2 tex = v.Texture + CorrectionTexture.Sample(Sampler, v.Texture) * 32.f / DepthSize;
	if(tex.x < 0 || tex.x > 1 || tex.y < 0 || tex.y > 1)
		return 0.f;
	else
		return Texture.Sample(Sampler, tex).x;
}