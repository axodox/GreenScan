#include "Header.hlsli"

[maxvertexcount(3)]
void main(
	triangle VertexPositionWorld input[3], 
	inout TriangleStream<VertexPositionWorldNormal> output
)
{
	float3 normal = normalize(cross(input[0].World.xyz - input[1].World.xyz, input[0].World.xyz - input[2].World.xyz));

	for (uint i = 0; i < 3; i++)
	{
		VertexPositionWorldNormal element;
		element.Position = input[i].Position;
		element.World = input[i].World;
		element.Normal = normal;		
		output.Append(element);
	}
}