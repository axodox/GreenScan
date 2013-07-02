#include "Header.hlsli"
Texture2D<int> Texture  : register(t0);

float2 main(VertexPositionTextureOut v) : SV_TARGET
{
	int3 id = int3((int)(v.Texture.x * DepthSize.x), (int)(v.Texture.y * DepthSize.y), 0);
	int depth = Texture.Load(id);
	if(depth > 0)
		return float2(depth, 1);
	else
		return 0;
}