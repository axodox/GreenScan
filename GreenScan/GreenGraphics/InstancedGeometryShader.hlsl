#include "Header.hlsli"
[maxvertexcount(3)]
void main(
	triangle VertexPositionTextureInstanceOut input[3], 
	inout TriangleStream< VertexPositionTextureTarget > output
)
{
	for (uint i = 0; i < 3; i++)
	{
		VertexPositionTextureTarget element;
		element.Position = input[i].Position;
		element.Texture = input[i].Texture;
		element.Target = input[i].Instance;
		output.Append(element);
	}
}