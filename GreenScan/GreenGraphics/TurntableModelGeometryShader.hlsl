#include "Header.hlsli"
[maxvertexcount(3)]
void main(
	triangle VertexPositionTextureOut input[3], 
	inout TriangleStream<VertexPositionTextureOut> output
)
{
	for (uint i = 0; i < 3; i++)
	{
		if(input[i].Position.w == 0.f) return;
	}

	for (uint i = 0; i < 3; i++)
	{
		output.Append(input[i]);
	}
	output.RestartStrip();
}