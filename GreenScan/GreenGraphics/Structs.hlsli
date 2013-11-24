struct VertexPositionWorld
{
	float4 Position : SV_POSITION;
	float3 World : TEXCOORD0;
};

struct VertexPositionNormal
{
	float3 Position : POSITION0;
	float3 Normal : NORMAL0;
};

struct VertexPositionWorldNormal
{
	float4 Position : SV_POSITION;
	float3 World : TEXCOORD0;
	float3 Normal : NORMAL0;
};

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

struct VertexPositionTextureInstanceIn
{
	float3 Position : POSITION0;
	float2 Texture : TEXCOORD0;
	uint Instance : SV_InstanceID;
};

struct VertexPositionTextureInstanceOut
{
	float4 Position : SV_POSITION;
	float2 Texture : TEXCOORD0;
	uint Instance : POSITION0;
};

struct VertexPositionTextureTarget
{
	float4 Position : SV_POSITION;
	float2 Texture : TEXCOORD0;
	uint Target: SV_RenderTargetArrayIndex;
};

struct VertexPositionColorIn
{
	float3 Position : POSITION0;
	float4 Color : COLOR0;
};

struct VertexPositionColorOut
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR0;
};

struct VertexPositionDepth
{
    float4 Position : SV_POSITION;
	float Depth : POSITION1;
};

struct VertexPositionWorldNormalDepthTexture
{
    float4 Position : SV_POSITION;
	float3 World : POSITION1;
	float3 Normal: NORMAL0;
	float Depth : NORMAL1;
	float2 Texture : TEXCOORD0;
};

struct VertexPositionWorldTexture
{
    float4 Position : SV_POSITION;
	float3 World : POSITION1;
	float2 Texture : TEXCOORD0;
};

struct VertexPositionWorldDepthTexture
{
    float4 Position : SV_POSITION;
	float3 World : POSITION1;
	float Depth : POSITION2;
	float2 Texture : TEXCOORD0;
};

struct VertexPositionOut
{
    float4 Position : SV_POSITION;
};

struct VertexPositionTextureInstance
{
	float3 Position : POSITION0;
	float2 Texture : TEXCOORD0;
	uint Instance : SV_InstanceID;
};

struct VertexPositionInstance
{
	float4 Position : SV_POSITION;
	uint Instance : POSITION0;
};

struct VertexPositionTarget
{
	float4 Position : SV_POSITION;
	uint Target: SV_RenderTargetArrayIndex;
};