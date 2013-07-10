#include "Header.hlsli"
Texture2D<int> Texture  : register(t0);
Texture1D ScaleTexture : register(t1);
SamplerState Sampler : register(s0);

float4 main(VertexPositionTextureOut v) : SV_TARGET
{
	int3 id = DepthCoords(v.Texture);
	int depth = Texture.Load(id);
	if (depth == 0) 
		return 0.f;
	else 
		return ScaleTexture.Sample(Sampler, ToDepth(depth) / ShadingPeriode + ShadingPhase);
}