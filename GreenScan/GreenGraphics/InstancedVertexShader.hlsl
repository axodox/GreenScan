#include "Header.hlsli"
VertexPositionTextureInstanceOut main( VertexPositionTextureInstanceIn vi )
{
	VertexPositionTextureInstanceOut vo;
	vo.Position = float4(vi.Position, 1.f);
	vo.Texture = vi.Texture;
	vo.Instance = vi.Instance;
	return vo;
}