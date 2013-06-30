#include "Header.hlsli"
Texture2D InfraredTexture  : register(t0);
Texture1D ScaleTexture : register(t1);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float i = InfraredTexture.Sample(Sampler, v.Texture).r;
	if (i == 0.f) 
		return 1.f;
	else 
		return ScaleTexture.Sample(Sampler, i / ShadingPeriode + ShadingPhase);
}