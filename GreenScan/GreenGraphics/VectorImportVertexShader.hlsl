#include "ImportHeader.hlsli"
Texture2D VectorTexture : register(t0);

VertexPositionDepth main(VertexPositionTextureIn vi)
{
	float4 v = VectorTexture.Load(DepthCoords(vi.Texture)) * Scale;

	float4 r = mul(v, DepthIntrinsics);
	r.xy = (r.xy / (r.z * DepthSize) * 2.f - 1.f);
	r.z /= MaxDepth;
	r.w = 1.f;

	VertexPositionDepth vo;
	vo.Position = r;
	vo.Depth = (v.z - DepthCoeffs.y) / DepthCoeffs.x;
	return vo;
}