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
			virtual void CreateResources(ID3D11Device* device/*, D3D11_VIEWPORT* viewport*/) = 0;
			virtual void DestroyResources() = 0;
			virtual void StartProcessing() = 0;
			virtual void EndProcessing() = 0;
			virtual void Process(/*RenderTargetPair* depth, Texture2D* color*/) = 0;
			//virtual void Draw(DepthBuffer* depthBuffer) = 0;
		};
	}
}