#include "Header.hlsli"
//Texture2D<float> Texture  : register(t0);
//SamplerState Sampler : register(s0);

//float4 main(VertexPositionTextureOut vi) : SV_TARGET
//{
//	float depth = Texture.Sample(Sampler, vi.Texture).x;
//	float4 posDepth = float4(
//		DepthResolution.x * vi.Texture.x * depth, 
//		DepthResolution.y * vi.Texture.y * depth, 
//		depth, 
//		1);
//
//	float4 posWorld = mul(posDepth, DepthToTurntableTransform);
//
//	int side = sign(posWorld.x);
//
//	float4 posCore = float4(posWorld.x - CorePosition.x * side, posWorld.y, posWorld.z - CorePosition.y, 1.f);
//
//	float4 posOutput;
//	posOutput.x = atan2(posCore.z, posCore.x) / Pi;
//	posOutput.y = posCore.y / ClipLimit.y * 2.f - 1.f;
//	posOutput.z = sqrt(posCore.x * posCore.x + posCore.z * posCore.z);
//	posOutput.w = 1.f;
//
//	float i = posOutput.y;
//	if (depth < MinDepth || i > 1 || i < -1)
//		return float4(1, 1, 1, 0.f);
//	if(i>0)
//		return float4(i, 0, 0, 1.f);
//	else
//		return float4(0, 0, -i, 1.f);
//}

//float4 main(VertexPositionWorldTexture v) : SV_TARGET
//{
//	if(sign(v.World.x) == 1)
//		return float4(v.Position.z * 5, 0.f, 0.f, 1.f);
//	else
//		return float4(0.f, 0.f, v.Position.z * 5, 1.f);
//}
Texture2D<float2> ModelTexture : register(t0);

float4 main(VertexPositionTextureOut vi) : SV_TARGET
{
	int3 id = int3((int)(vi.Texture.x * ModelResolution.x), (int)(vi.Texture.y * ModelResolution.y), 0);
	float2 data = ModelTexture.Load(id);

	//if(data.y > 0)
		return float4(ModelResolution.x, vi.Texture.y, 0, 1);
	//else return 0;
}