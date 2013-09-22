#include "Header.hlsli"
[maxvertexcount(3)]
void main(
	triangle VertexPositionWorldNormalDepthTexture input[3], 
	inout TriangleStream<VertexPositionWorldNormalDepthTexture> output
)
{
	for (uint i = 0; i < 3; i++)
	{
		if(input[i].Depth < MinDepth || input[i].Depth > DepthMaximum || input[i].Depth < DepthMinimum) return;
	}

	for (uint i = 0; i < 3; i++)
	{
		input[i].Depth = input[i].Position.z * MaxDepth;
		output.Append(input[i]);
	}
	output.RestartStrip();
}