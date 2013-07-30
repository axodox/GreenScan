#include "Header.hlsli"
Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

struct PSOutput
{
	float4 Model : SV_TARGET0;
	float4 TexL : SV_TARGET1;
	float4 TexR : SV_TARGET2;
};

PSOutput main(VertexPolar v)
{
	PSOutput output;
	float4 color = Texture.Sample(Sampler, (v.Texture + TextureMove) * TextureScale).bgra;
	color.a = 1.f;
	if(sign(v.World.x) == 1)
	{
		output.Model = float4(v.Position.z, 1.f, 0.f, 0.f);
		output.TexL = 0.f;
		output.TexR = color;
	}
	else
	{
		output.Model = float4(0.f, 0.f, v.Position.z, 1.f);
		output.TexL = color;	
		output.TexR = 0.f;
	}
	return output;
}