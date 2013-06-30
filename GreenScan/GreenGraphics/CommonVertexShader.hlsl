#include "Header.hlsli"
VertexPositionTextureOut main( VertexPositionTextureIn vi )
{
	float4 pos = float4(vi.Position, 1.f);
	pos.xy = pos.xy * AspectScale.xy;
	pos = mul(pos, SceneRotation);
	pos.xy = (pos.xy + Move) * Scale;
	
	VertexPositionTextureOut vo;
	vo.Position = pos;
	vo.Texture = vi.Texture;
	return vo;
}