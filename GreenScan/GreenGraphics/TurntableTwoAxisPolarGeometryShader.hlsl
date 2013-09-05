#include "Header.hlsli"
#define maxL 0.05f
#define overL 1.95f

[maxvertexcount(6)]
void main(
	triangle VertexPositionWorldTexture input[3], 
	inout TriangleStream<VertexPositionWorldTexture> output
)
{
	int side[3];
	bool ok = true;	
	VertexPositionWorldTexture v;
	for (uint i = 0; i < 3; i++)
	{
		v = input[i];
		if(v.Position.z > ClipLimit.x || v.Position.z == 0.f) ok = false;
		side[i] = sign(v.World.x);
	}
	
	if(side[0] != side[1] || side[1] != side[2] || !ok) return;
	
	float lab, lbc, lca;
	lab = abs(input[1].Position.x - input[0].Position.x);
	lbc = abs(input[2].Position.x - input[1].Position.x);
	lca = abs(input[0].Position.x - input[2].Position.x);

	bool _ab, _bc, _ca;
	_ab = lab < maxL;
	_bc = lbc < maxL;
	_ca = lca < maxL;	
	
	if(_ab && _bc && _ca)
	{
		for (uint i = 0; i < 3; i++)
		{
			output.Append(input[i]);
		}
		output.RestartStrip();
		return;
	}

	bool ab, bc, ca;
	ab = lab > overL;
	bc = lbc > overL;
	ca = lca > overL;

	if((_ab && bc && ca) || (ab && _bc && ca) || (ab && bc && _ca))
	{
		float shift;
		if(_ab) input[2].Position.x -= (shift = 2.f * sign(input[2].Position.x));
		if(_bc) input[0].Position.x -= (shift = 2.f * sign(input[0].Position.x));
		if(_ca) input[1].Position.x -= (shift = 2.f * sign(input[1].Position.x));

		for (uint i = 0; i < 3; i++)
		{
			output.Append(input[i]);
		}
		output.RestartStrip();
		
		for (uint i = 0; i < 3; i++)
		{
			v = input[i];
			v.Position.x += shift;
			output.Append(v);
		}
		output.RestartStrip();
	}
}