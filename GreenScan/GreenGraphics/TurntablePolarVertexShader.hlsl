#include "Header.hlsli"
Texture2D<float> DepthTexture : register(t0);

VertexPolar main(VertexPositionTextureIn vi)
{
	int3 id = int3((int)(vi.Texture.x * DepthResolution.x), (int)(vi.Texture.y * DepthResolution.y), 0);
	float depth = DepthTexture.Load(id);

	float4 posDepth = float4(
		DepthResolution.x * vi.Texture.x * depth, 
		DepthResolution.y * vi.Texture.y * depth, 
		depth, 
		1);

	float4 posWorld = mul(posDepth, DepthToTurntableTransform);
	
	int side = sign(posWorld.x);

	float4 posCore = float4(posWorld.x - CorePosition.x * side, posWorld.y, posWorld.z - CorePosition.y, 1.f);
	
	float4 posOutput;
	posOutput.x = atan2(posCore.z, posCore.x) / Pi;
	posOutput.y = posCore.y / ClipLimit.y;
	posOutput.z = sqrt(posCore.x * posCore.x + posCore.z * posCore.z);
	posOutput.w = 1.f;

	VertexPolar vo;
	vo.Position = posOutput;
	vo.World = posWorld;
	return vo;
}