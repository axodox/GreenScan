#include "Header.hlsli"

[maxvertexcount(6)]
void main(
	triangle VertexPolar input[3], 
	inout TriangleStream<VertexPolar> output
)
{
	int side[3];
	bool ab, bc, ca, ok = true;
	float shift;
	for (uint i = 0; i < 3; i++)
	{
		if(input[i].Position.z > ClipLimit.x) ok = false;
		side[i] = sign(input[i].World.x);
	}
	
	if(side[0] != side[1] || side[1] != side[2] || !ok) return;
	
	ab = abs(input[1].World.x - input[0].World.x) < 1.f;
	bc = abs(input[1].World.x - input[2].World.x) < 1.f;
	ca = abs(input[2].World.x - input[0].World.x) < 1.f;
	
	if(ab && bc && ca)
	{
		for (uint i = 0; i < 3; i++)
		{
			output.Append(input[i]);
		}
	}
	else
	{
		if(ab) input[2].Position.x -= shift = 2.f * sign(input[2].Position.x);
		if(bc) input[0].Position.x -= shift = 2.f * sign(input[0].Position.x);
		if(ca) input[1].Position.x -= shift = 2.f * sign(input[1].Position.x);

		for (uint i = 0; i < 3; i++)
		{
			output.Append(input[i]);
		}
		
		for (uint i = 0; i < 3; i++)
		{
			input[i].Position.x -= shift;
			output.Append(input[i]);
		}
	}

}