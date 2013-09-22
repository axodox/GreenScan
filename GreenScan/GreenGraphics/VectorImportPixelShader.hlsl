#include "ImportHeader.hlsli"

int main(VertexPositionDepth v) : SV_TARGET
{
	return v.Depth;
}