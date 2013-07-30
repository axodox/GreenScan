#pragma once
#pragma unmanaged
#include "GreenGraphicsClasses.h"

namespace Green
{
	namespace Graphics
	{
		class GraphicsModule
		{
		public:
			bool DrawsOverlay;
			virtual void CreateResources(GraphicsDevice* device) = 0;
			virtual void DestroyResources() = 0;
			virtual void StartProcessing() = 0;
			virtual void EndProcessing() = 0;
			virtual void ProcessFrame(RenderTargetPair* depth, Texture2D* color) = 0;
			virtual void Draw() = 0;
			virtual void SetCameras(XMFLOAT4X4 depthIntrinsics, XMFLOAT4X4 depthInvIntrinsics, XMFLOAT4X4 worldToColorTransform, XMFLOAT4X4 depthToTexture, XMFLOAT2 textureMove, XMFLOAT2 textureScale) { }
			virtual void SetView(XMFLOAT4X4 world) { }
		};
	}
}