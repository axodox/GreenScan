#include "Header.hlsli"
Texture2D<int> DepthTexture  : register(t0);

VertexPositionTextureDepth main(VertexPositionTextureIn vi)
{
	int3 id = int3((int)(vi.Texture.x * DepthSize), (int)(vi.Texture.y * 480), 0);
	float depth = DepthTexture.Load(id) / 8000.f;

	VertexPositionTextureDepth vo;
	vo.Position = float4(vi.Position, 1.f);
	vo.WorldPosition = float3(1,1,1);
	vo.Normal = float3(1,1,1);
	vo.Depth = depth;
	vo.Texture = vi.Texture;
	return vo;
}