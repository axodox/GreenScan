#pragma once
#pragma unmanaged
#include "GreenGraphicsModules.h"

namespace Green
{
	namespace Graphics
	{
		class RotatingScannerModule : public GraphicsModule
		{
		private:
			GraphicsDevice* Device;
			RenderTarget *ModelTarget, *TextureTarget;
			int NextModelWidth, NextModelHeight, NextTextureWidth, NextTextureHeight;
			bool Processing;
			struct RenderingParameters
			{
				XMFLOAT4X4 DepthIntrinsics, World, TurntableTransform;
			} Params;

			struct TurntableConstants
			{
				XMFLOAT4X4 TurntableToScreenTransform;
				XMINT2 DepthResolution;
			} TurntableOptions;

			ConstantBuffer<TurntableConstants>* Constants; 
			VertexBuffer<VertexPositionColor>* Cross;
			VertexShader *VSTurntableOverlay;
			PixelShader *PSTurntableOverlay;
			ID3D11DeviceContext *Context;
		public:
			RotatingScannerModule()
			{
				Processing = false;
				DrawsOverlay = true;
			}

			virtual void CreateResources(GraphicsDevice* device) override
			{
				Device = device;
				VSTurntableOverlay = new VertexShader(Device, L"TurntableOverlayVertexShader.cso");
				VSTurntableOverlay->SetInputLayout(VertexDefinition::VertexPositionColor);
				PSTurntableOverlay = new PixelShader(Device, L"TurntableOverlayPixelShader.cso");
				Context = Device->GetImmediateContext();
			}

			virtual void DestroyResources() override
			{
				delete VSTurntableOverlay;
				delete PSTurntableOverlay;
				Context->Release();
			}

			virtual void StartProcessing() override
			{
				Processing = true;
				ModelTarget = new RenderTarget(Device, NextModelWidth, NextModelHeight, DXGI_FORMAT_R32G32_FLOAT);
				TextureTarget = new RenderTarget(Device, NextTextureWidth, NextTextureHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

				VertexPositionColor* crossVertices = new VertexPositionColor[6];
				crossVertices[0] = VertexPositionColor(XMFLOAT3(-0.1, 0, 0), XMFLOAT4(1, 0, 0, 1));
				crossVertices[1] = VertexPositionColor(XMFLOAT3(0.1, 0, 0), XMFLOAT4(1, 0, 0, 1));
				crossVertices[2] = VertexPositionColor(XMFLOAT3(0, -0.1, 0), XMFLOAT4(0, 1, 0, 1));
				crossVertices[3] = VertexPositionColor(XMFLOAT3(0, 0.1, 0), XMFLOAT4(0, 1, 0, 1));
				crossVertices[4] = VertexPositionColor(XMFLOAT3(0, 0, -0.1), XMFLOAT4(0, 0, 1, 1));
				crossVertices[5] = VertexPositionColor(XMFLOAT3(0, 0, 0.1), XMFLOAT4(0, 0, 1, 1));
				Cross = new VertexBuffer<VertexPositionColor>(Device, 6, VertexDefinition::VertexPositionColor, crossVertices);

				Constants = new ConstantBuffer<TurntableConstants>(Device);
				TurntableOptions.DepthResolution = XMINT2(KinectDevice::DepthWidth, KinectDevice::DepthHeight);
			}

			virtual void EndProcessing() override
			{
				if (!Processing) return;
				Processing = false;
				delete ModelTarget;
				delete TextureTarget;
				delete Cross;
				delete Constants;				
			}

			virtual void ProcessFrame(RenderTargetPair* depth, Texture2D* color) override
			{
				if (!Processing) return;
				Constants->Update(&TurntableOptions);
			}

			virtual void Draw() override
			{
				if (!Processing) return;
				Device->SetAsRenderTarget();
				Constants->SetForVS(1);
				Cross->Set();
				Device->SetShaders(VSTurntableOverlay, PSTurntableOverlay);
				Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
				Context->Draw(6, 0);
			}

			void SetPerformance(int modelWidth, int modelHeight, int textureWidth, int textureHeight)
			{
				NextModelWidth = modelWidth;
				NextModelHeight = modelHeight;
				NextTextureWidth = textureWidth;
				NextTextureHeight = textureHeight;
			}

			void SetCalibration(
				float transX, float transY, float transZ,
				float rotX, float rotY, float rotZ,
				float height, float radius, 
				float coreX, float coreY)
			{
				XMMATRIX T = XMMatrixTranspose(XMMatrixTranslation(transX, transY, transZ));
				XMMATRIX R = 
					XMMatrixRotationZ(XMConvertToRadians(rotZ)) *
					XMMatrixRotationX(XMConvertToRadians(rotX)) *
					XMMatrixRotationY(XMConvertToRadians(rotY));
				XMMATRIX world = T * R;
				XMStoreFloat4x4(&Params.TurntableTransform, world);
				SetTurntableOptions();
			}

			virtual void SetCameras(XMFLOAT4X4 depthIntrinsics, XMFLOAT4X4 depthInvIntrinsics, XMFLOAT4X4 worldToColorTransform) override
			{
				Params.DepthIntrinsics = depthIntrinsics;
				SetTurntableOptions();
			}

			virtual void SetView(XMFLOAT4X4 world) override
			{ 
				Params.World = world;
				SetTurntableOptions();
			}

			void SetTurntableOptions()
			{
				XMMATRIX DepthIntrinsics = XMLoadFloat4x4(&Params.DepthIntrinsics);
				XMMATRIX World = XMLoadFloat4x4(&Params.World);
				XMMATRIX TurntableTransform = XMLoadFloat4x4(&Params.TurntableTransform);

				XMMATRIX TurntableToScreenTransform = DepthIntrinsics * World * TurntableTransform;
				XMStoreFloat4x4(&TurntableOptions.TurntableToScreenTransform, TurntableToScreenTransform);
			}
		};
	}
}