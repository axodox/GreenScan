struct VertexPositionTextureIn
{
	float3 Position : POSITION0;
	float2 Texture : TEXCOORD0;
};

struct VertexPositionTextureOut
{
	float4 Position : SV_POSITION;
	float2 Texture : TEXCOORD0;
};

struct VertexPositionTextureDepth
{
    float4 Position : SV_POSITION;
	float3 WorldPosition : POSITION1;
	float3 Normal: NORMAL0;
	float Depth : NORMAL1;
	float2 Texture : TEXCOORD0;
};

cbuffer DepthAndColorConstants : register(b0)
{
	float4x4 ReprojectionTransform;
	float4x4 ModelTransform;
	float4x4 WorldTransform;
	float4x4 NormalTransform;
	float2 ModelScale;
	int2 DepthSize;
	float DepthLimit;
};