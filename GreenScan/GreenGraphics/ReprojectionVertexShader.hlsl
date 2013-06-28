#include "Header.hlsli"
Texture2D<int> DepthTexture : register(t0);

VertexPositionTextureDepth main(VertexPositionTextureIn vi)
{
	int3 id = int3((int)(vi.Texture.x * DepthSize.x), (int)(vi.Texture.y * DepthSize.y), 0);
	float depth = DepthTexture.Load(id) / 8000.f;

	float4 posTemp;

	float4 posDepth = float4(
		DepthSize.x * vi.Texture.x * depth, 
		DepthSize.y * vi.Texture.y * depth, 
		depth, 
		1);

	posTemp = mul(posDepth, ReprojectionTransform);
	posTemp.x = (posTemp.x / posTemp.z / DepthSize.x * 2 - 1) * DepthLimit * ModelScale.x;
	posTemp.y = (-posTemp.y / posTemp.z / DepthSize.y * 2 + 1) * DepthLimit * ModelScale.y;
	posTemp.z /= 10;
	posTemp.w = DepthLimit;
	float4 posScreen = posTemp;

	VertexPositionTextureDepth vo;
	vo.Position = posScreen;
	vo.WorldPosition = float3(1,1,1);
	vo.Normal = float3(1,1,1);
	vo.Depth = depth;
	vo.Texture = vi.Texture;
	return vo;
}