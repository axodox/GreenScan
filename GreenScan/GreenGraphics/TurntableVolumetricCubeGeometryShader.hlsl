#include "Header.hlsli"

[maxvertexcount(2)]
void main(
	line VertexPositionInstance input[2],
	inout LineStream<VertexPositionTarget> output
	)
{
	for (uint i = 0; i < 2; i++)
	{
		if (input[i].Position.z == 0.f) return;
	}

	for (uint i = 0; i < 2; i++)
	{
		VertexPositionTarget element;
		element.Position = input[i].Position;
		element.Target = input[i].Instance;
		output.Append(element);
	}
}