#include "Header.hlsli"

float4 main(VertexPolar v) : SV_TARGET
{
	if(sign(v.World.x) == 1)
		return float4(v.Position.z, 1.f, 0.f, 0.f);
	else
		return float4(0.f, 0.f, v.Position.z, 1.f);
}