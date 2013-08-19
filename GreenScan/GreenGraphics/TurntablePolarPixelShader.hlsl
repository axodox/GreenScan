#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct PSOutput
{
	float4 ModelL : SV_TARGET0;
	float4 ModelR : SV_TARGET1;
	float4 TexL : SV_TARGET2;
	float4 TexR : SV_TARGET3;
};

PSOutput main(VertexPolar v)
{
	PSOutput output;
	float4 color = Texture.Sample(Sampler, (v.Texture + TextureMove) * TextureScale).bgra;
	color.a = 1.f;
	float4 depth = float4(v.Position.z, 1.f, 0.f, 0.f);
	if(sign(v.World.x) == 1)
	{
		output.ModelL = 0.f;
		output.ModelR = depth;
		output.TexL = 0.f;
		output.TexR = color;
	}
	else
	{
		output.ModelL = depth;
		output.ModelR = 0.f;
		output.TexL = color;	
		output.TexR = 0.f;
	}
	return output;
}