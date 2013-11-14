#include "Header.hlsli"

[maxvertexcount(3)]
void main(
	triangle VertexPositionOut input[3],
	inout TriangleStream<VertexPositionOut> output
	)
{
	for (uint i = 0; i < 3; i++)
	{
		if (input[i].Position.z < 0.f || input[i].Position.z > 1.f) return;
	}

	for (uint i = 0; i < 3; i++)
	{
		output.Append(input[i]);
	}
	output.RestartStrip();
}