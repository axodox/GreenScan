#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct PSOutput
{
	float4 Model : SV_TARGET0;
	float4 Tex : SV_TARGET1;
};

PSOutput main(VertexPositionWorldTexture v)
{
	PSOutput output;
	float4 color = Texture.Sample(Sampler, (v.Texture + TextureMove) * TextureScale).bgra;
	color.a = 1.f;
	float4 depth = float4(v.Position.z, 1.f, 0.f, 0.f);
	output.Model = depth;
	output.Tex = color;	
	return output;
}