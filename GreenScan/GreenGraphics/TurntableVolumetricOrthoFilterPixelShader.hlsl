#include "Header.hlsli"
Texture2D<float> Texture : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	float resStep = 1.f / CubeRes;
	float dx = Texture.Sample(Sampler, float2(v.Texture.x, v.Texture.y));
	float du = Texture.Sample(Sampler, float2(v.Texture.x, v.Texture.y - resStep));
	float dd = Texture.Sample(Sampler, float2(v.Texture.x, v.Texture.y + resStep));
	float dl = Texture.Sample(Sampler, float2(v.Texture.x - resStep, v.Texture.y));
	float dr = Texture.Sample(Sampler, float2(v.Texture.x + resStep, v.Texture.y));

	float grad = pow(du - dd, 2) + pow(dr - dl, 2);

	if (grad < GradientLimit)
		return float4(dx, 1.0f, 1.0f, 1.0f);
	else
		return 0.0f;
}