#include "Header.hlsli"
Texture2D<float2> Texture  : register(t0);

int main(VertexPositionTextureOut v) : SV_TARGET
{
	int3 id = int3((int)(v.Texture.x * DepthSize.x), (int)(v.Texture.y * DepthSize.y), 0);
	float2 sum = Texture.Load(id);
	if(sum.y > 0)
		return sum.x / sum.y;
	else
		return 0;
}