#define Pi 3.14159f
#define MaxDepth 10.f
#define GaussCoeffCount 9

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

cbuffer CommonConstants : register(b0)
{
	float4x4 SceneRotation;
	float2 AspectScale;
	float2 Move;
	float Scale;
};

cbuffer DepthAndColorConstants : register(b1)
{
	float4x4 DepthInvIntrinsics;
	float4x4 ReprojectionTransform;
	float4x4 ModelTransform;
	float4x4 WorldTransform;
	float4x4 NormalTransform;
	float4x4 DepthToColorTransform;
	float2 DepthStep;
	float2 ColorMove;
	float2 ColorScale;
	int2 DepthSize;
	float DepthLimit;
	float ShadingPeriode;
	float ShadingPhase;
	float TriangleLimit;
	int GaussDispos[GaussCoeffCount];
	float GaussCoeffs[GaussCoeffCount];
};

float ToDepth(int raw)
{
	return raw / 8000.f;
}

int3 DepthCoords(float2 uv)
{
	return int3((int)(uv.x * DepthSize.x), (int)(uv.y * DepthSize.y), 0);
}