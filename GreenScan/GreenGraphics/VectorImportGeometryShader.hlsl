#include "ImportHeader.hlsli"

[maxvertexcount(3)]
void main(
	triangle VertexPositionDepth input[3], 
	inout TriangleStream<VertexPositionDepth> output
)
{
	for (uint i = 0; i < 3; i++)
	{
		if(isnan(input[i].Depth) || input[i].Depth == 0) return;
	}

	for (uint i = 0; i < 3; i++)
	{
		output.Append(input[i]);
	}
}