#include "Header.hlsli"
Texture2D<float2> Texture  : register(t0);

float main(VertexPositionTextureOut v) : SV_TARGET
{
	int3 id = DepthCoords(v.Texture);
	float2 sum = Texture.Load(id);
	if (sum.y > 0)
	{
		return ToDepth(sum.x / sum.y);
	}
	return 0.f;
}