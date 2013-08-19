#include "Header.hlsli"
Texture2D<float2> ModelTexture : register(t0);

VertexPositionTextureOut main(VertexPositionTextureIn vi)
{
	int3 id = int3((int)(vi.Texture.x * ModelResolution.x), (int)(vi.Texture.y * ModelResolution.y), 0);
	float2 data = ModelTexture.Load(id);
	float depth = (data.y > 0.f ? data.x / data.y : 0.f);

	float angle = (vi.Texture.x * 2.f - 1.f) * -Pi;

	float4 posModel = float4(
		depth * cos(angle) + CorePosition.x * Side,
		(vi.Texture.y - 0.5f) * ClipLimit.y,
		depth * sin(angle) + CorePosition.y,
		1.f);

	float4 posOutput = mul(posModel, ModelToScreenTransform);
	posOutput.xy = (posOutput.xy / (posOutput.z * DepthResolution) * 2.f - 1.f) * AspectScale * float2(1.f, -1.f);
	posOutput = mul(posOutput, SceneRotation);
	posOutput.xy = (posOutput.xy + Move) * Scale;
	posOutput.z /= MaxDepth;
	
	if (depth == 0.f) posOutput.w = 0.f;

	VertexPositionTextureOut vo;
	vo.Position = posOutput;
	vo.Texture = vi.Texture;
	return vo;
}