#include "Header.hlsli"
Texture2D<int> Texture  : register(t0);

int main(VertexPositionTextureOut v) : SV_TARGET
{
	int3 pos, center = DepthCoords(v.Texture.yx);
	int depth = Texture.Load(center);
	if(depth > 0) return 0;
	float sum = 0;
	float gain = 0;

	for(int i = 0; i < GaussCoeffCount; i++)
	{
		pos = center + int3(GaussDispos[i], 0, 0);
		depth = Texture.Load(pos);
		if(depth > 0)
		{
			sum += depth * GaussCoeffs[i];
			gain += GaussCoeffs[i];
		}
	}

	return (int)(sum / gain);
}