#include "Header.hlsli"
Texture2D<float2> Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float2 data = Texture.Sample(Sampler, v.Texture);
	if(data.y > 0.f)
	{
		float depth = (data.y > 0.f ? data.x / data.y : 0.f);
		float angle = (v.Texture.x * 2.f - 1.f) * -Pi;
		float4 posModel = float4(
			depth * cos(angle) + CorePosition.x * Side,
			(v.Texture.y - 0.5f) * ClipLimit.y,
			depth * sin(angle) + CorePosition.y,
			1.f);
		return posModel;
	}
	else
		return 0.f;
}