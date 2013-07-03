#include "Header.hlsli"
Texture2D<int> Texture  : register(t0);

float2 main(VertexPositionTextureOut v) : SV_TARGET
{
	int3 id = DepthCoords(v.Texture);
	int depth = Texture.Load(id);
	if(depth > 0)
		return float2(depth, 1.f);
	else
		return 0.f;
}