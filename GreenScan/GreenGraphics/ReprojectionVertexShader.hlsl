#include "Header.hlsli"
Texture2D<int> DepthTexture : register(t0);

float3 PosOf(float x, float y)
{
	int3 id = DepthCoords(float2(x, y));
	float depth = ToDepth(DepthTexture.Load(id));
	return mul(float4(DepthSize.x * x * depth, DepthSize.y * y * depth, depth, 1), DepthInvIntrinsics).xyz;
}

bool CalculateWorldNormal(float2 uv, out float3 posx, out float3 normal)
{
	posx = PosOf(uv.x, uv.y);
	float3 posu = PosOf(uv.x, uv.y + DepthStep.y);
	float3 posd = PosOf(uv.x, uv.y - DepthStep.y);
	float3 posl = PosOf(uv.x - DepthStep.x, uv.y);
	float3 posr = PosOf(uv.x + DepthStep.x, uv.y);
	normal = normalize(cross(posu - posd, posr - posl));
	return pow(length(posu - posd) / posx.z, 2) + pow(length(posr - posl) / posx.z, 2) > TriangleLimit;
}

VertexPositionTextureDepth main(VertexPositionTextureIn vi)
{
	int3 id = DepthCoords(vi.Texture);
	float depth = ToDepth(DepthTexture.Load(id));

	float4 posTemp;

	float4 posDepth = float4(
		DepthSize.x * vi.Texture.x * depth, 
		DepthSize.y * vi.Texture.y * depth, 
		depth, 
		1);

	float screenDepth;
	posTemp = mul(posDepth, ReprojectionTransform);
	screenDepth = posTemp.z;
	posTemp.xy = (posTemp.xy / (posTemp.z * DepthSize) * 2.f - 1.f) * AspectScale * float2(1.f, -1.f);
	posTemp = mul(posTemp, SceneRotation);
	posTemp.xy = (posTemp.xy + Move) * Scale;
	posTemp.z /= MaxDepth;
	posTemp *= DepthLimit;	
	float4 posScreen = posTemp;

	posTemp = mul(posDepth, DepthToColorTransform);
	posTemp.xyz /= posTemp.z;
	posTemp.xy /= DepthSize;
	float2 tex = posTemp.xy;
	
	float3 posWorld, normal;
	if(CalculateWorldNormal(vi.Texture, posWorld, normal)) depth = 0.f;

	VertexPositionTextureDepth vo;
	vo.Position = posScreen;
	vo.WorldPosition = posWorld;
	vo.Normal = normalize(mul(normal, NormalTransform));
	vo.Depth = depth;
	vo.Texture = tex/2;
	return vo;
}