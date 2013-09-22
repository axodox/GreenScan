#include "Structs.hlsli"
#define MaxDepth 10.f

cbuffer Constants : register(b0)
{
	float4x4 DepthIntrinsics;
	float4 Scale;
	float2 DepthCoeffs;
	int2 DepthSize;
};

int3 DepthCoords(float2 uv)
{
	return int3((int)(uv.x * (DepthSize.x - 1)), (int)(uv.y * (DepthSize.y - 1)), 0);
}