#include "Header.hlsli"
[maxvertexcount(3)]
void main(
	triangle VertexPositionTextureDepth input[3], 
	inout TriangleStream<VertexPositionTextureDepth> output
)
{
	for (uint i = 0; i < 3; i++)
	{
		if(input[i].Depth < MinDepth || input[i].Depth > DepthMaximum || input[i].Depth < DepthMinimum) return;
	}

	for (uint i = 0; i < 3; i++)
	{
		output.Append(input[i]);
	}
	output.RestartStrip();
}