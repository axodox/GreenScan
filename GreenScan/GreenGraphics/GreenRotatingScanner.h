#pragma once
#pragma unmanaged
#include "GreenGraphicsModules.h"

namespace Green
{
	namespace Graphics
	{
		class RotatingScannerModule : public GraphicsModule
		{
		public:
			enum class Views {
				Overlay,
				Polar
			};
		private:
			GraphicsDevice* Device;
			RenderTarget *ModelTarget, *TextureTarget;
			int NextModelWidth, NextModelHeight, NextTextureWidth, NextTextureHeight;
			bool Processing;
			struct RenderingParameters
			{
				XMFLOAT4X4 DepthIntrinsics, World, TurntableTransform;
				float Rotation;
				Views View;
			} Params;

			struct TurntableConstants
			{
				XMFLOAT4X4 TurntableToScreenTransform;
				XMFLOAT4X4 DepthToTurntableTransform;
				XMFLOAT2 CorePosition;
				XMFLOAT2 ClipLimit;
				XMINT2 DepthResolution;
			} TurntableOptions;

			ConstantBuffer<TurntableConstants>* Constants;
			VertexPositionColor* CrossVertices;
			VertexBuffer<VertexPositionColor>* Cross;
			VertexShader *VSOverlay, *VSPolar, *VSCommon;
			GeometryShader *GSPolar;
			PixelShader *PSOverlay, *PSPolar, *PSPolarDepth;
			Blend *BOpaque, *BAdditive;
			ID3D11DeviceContext *Context;
			Plane *PDepth;
			Quad *QMain;
			SamplerState *SLinearClamp;

			bool CrossChanged;
			static const int CrossVertexCount = 2 * 18 + 10;
			static const int CilinderSides = 18;
			void SetCross()
			{
				CrossVertices[6] = VertexPositionColor(XMFLOAT3(TurntableOptions.CorePosition.x, 0.f, TurntableOptions.CorePosition.y), XMFLOAT4(1.f, 0.f, 1.f, 1.f));
				CrossVertices[7] = VertexPositionColor(XMFLOAT3(TurntableOptions.CorePosition.x, TurntableOptions.ClipLimit.y, TurntableOptions.CorePosition.y), XMFLOAT4(1.f, 0.f, 1.f, 1.f));
				CrossVertices[8] = VertexPositionColor(XMFLOAT3(-TurntableOptions.CorePosition.x, 0.f, TurntableOptions.CorePosition.y), XMFLOAT4(1.f, 0.f, 1.f, 1.f));
				CrossVertices[9] = VertexPositionColor(XMFLOAT3(-TurntableOptions.CorePosition.x, TurntableOptions.ClipLimit.y, TurntableOptions.CorePosition.y), XMFLOAT4(1.f, 0.f, 1.f, 1.f));
				VertexPositionColor* vertex = &CrossVertices[10];
				float x, y, angleStep = XM_2PI / CilinderSides;
				for(int i = 0; i < CilinderSides; i++)
				{
					x = sin(i * angleStep) * TurntableOptions.ClipLimit.x;
					y = cos(i * angleStep) * TurntableOptions.ClipLimit.x;
					*vertex++ = VertexPositionColor(XMFLOAT3(x, 0.f, y), XMFLOAT4(0.f, 0.f, 0.f, 1.f));
					*vertex++ = VertexPositionColor(XMFLOAT3(x, TurntableOptions.ClipLimit.y, y), XMFLOAT4(0.f, 0.f, 0.f, 1.f));
				}
				CrossChanged = true;
			}
		public:
			RotatingScannerModule()
			{
				Processing = false;
				DrawsOverlay = true;
			}

			virtual void CreateResources(GraphicsDevice* device) override
			{
				Device = device;
				VSOverlay = new VertexShader(Device, L"TurntableOverlayVertexShader.cso");
				VSOverlay->SetInputLayout(VertexDefinition::VertexPositionColor);
				VSPolar = new VertexShader(Device, L"TurntablePolarVertexShader.cso");
				VSPolar->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSCommon = new VertexShader(Device, L"CommonVertexShader.cso");
				VSCommon->SetInputLayout(VertexDefinition::VertexPositionTexture);

				GSPolar = new GeometryShader(Device, L"TurntablePolarGeometryShader.cso");
				
				PSOverlay = new PixelShader(Device, L"TurntableOverlayPixelShader.cso");
				PSPolar = new PixelShader(Device, L"TurntablePolarPixelShader.cso");
				PSPolarDepth = new PixelShader(Device, L"TurntablePolarDepthPixelShader.cso");

				Context = Device->GetImmediateContext();
				
			}

			virtual void DestroyResources() override
			{
				delete VSOverlay;
				delete VSPolar;
				delete VSCommon;
				delete GSPolar;
				delete PSOverlay;
				delete PSPolar;
				delete PSPolarDepth;
				Context->Release();
			}

			virtual void StartProcessing() override
			{
				Processing = true;
				ModelTarget = new RenderTarget(Device, NextModelWidth, NextModelHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
				TextureTarget = new RenderTarget(Device, NextTextureWidth, NextTextureHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

				CrossVertices = new VertexPositionColor[CrossVertexCount];
				CrossVertices[0] = VertexPositionColor(XMFLOAT3(-0.1f, 0.f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f));
				CrossVertices[1] = VertexPositionColor(XMFLOAT3(0.1f, 0.f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f));
				CrossVertices[2] = VertexPositionColor(XMFLOAT3(0.f, -0.1f, 0.f), XMFLOAT4(0.f, 1.f, 0.f, 1.f));
				CrossVertices[3] = VertexPositionColor(XMFLOAT3(0.f, 0.1f, 0.f), XMFLOAT4(0.f, 1.f, 0.f, 1.f));
				CrossVertices[4] = VertexPositionColor(XMFLOAT3(0.f, 0.f, -0.1f), XMFLOAT4(0.f, 0.f, 1.f, 1.f));
				CrossVertices[5] = VertexPositionColor(XMFLOAT3(0.f, 0.f, 0.1f), XMFLOAT4(0.f, 0.f, 1.f, 1.f));
				
				Cross = new VertexBuffer<VertexPositionColor>(Device, CrossVertexCount, VertexDefinition::VertexPositionColor);
				SetCross();

				Constants = new ConstantBuffer<TurntableConstants>(Device);
				TurntableOptions.DepthResolution = XMINT2(KinectDevice::DepthWidth, KinectDevice::DepthHeight);

				BOpaque = new Blend(Device, Blend::Opaque);
				BAdditive = new Blend(Device, Blend::Additive);
				PDepth = new Plane(Device, KinectDevice::DepthWidth, KinectDevice::DepthHeight);

				QMain = new Quad(Device);
				SLinearClamp = new SamplerState(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
			}

			virtual void EndProcessing() override
			{
				if (!Processing) return;
				Processing = false;
				delete ModelTarget;
				delete TextureTarget;
				delete Cross;
				delete Constants;				
				delete BOpaque;
				delete BAdditive;
				delete PDepth;
				delete QMain;
				delete SLinearClamp;
			}

			virtual void ProcessFrame(RenderTargetPair* depth, Texture2D* color) override
			{
				if (!Processing) return;
				Constants->Update(&TurntableOptions);
				ModelTarget->SetAsRenderTarget();
				Device->SetShaders(VSPolar, PSPolar, GSPolar);
				depth->SetForVS();
				PDepth->Draw();
			}

			virtual void Draw() override
			{
				if (!Processing) return;
				Device->SetAsRenderTarget();
				switch (Params.View)
				{
				case Views::Polar:
					Device->SetShaders(VSCommon, PSPolarDepth);
					ModelTarget->SetForPS();
					QMain->Draw();
					break;
				default:
					if (CrossChanged)
					{
						Cross->Load(CrossVertices, CrossVertexCount);
						CrossChanged = false;
					}					
					Constants->SetForVS(1);
					Cross->Set();
					Device->SetShaders(VSOverlay, PSOverlay);
					Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
					Context->Draw(CrossVertexCount, 0);
					break;
				}
				
			}

			void SetPerformance(int modelWidth, int modelHeight, int textureWidth, int textureHeight)
			{
				NextModelWidth = modelWidth;
				NextModelHeight = modelHeight;
				NextTextureWidth = textureWidth;
				NextTextureHeight = textureHeight;
			}

			void SetCalibration(
				float* turntableTransform,
				float height, float radius, 
				float coreX, float coreY)
			{
				Params.TurntableTransform = XMFLOAT4X4(turntableTransform);
				TurntableOptions.CorePosition = XMFLOAT2(coreX, coreY);
				TurntableOptions.ClipLimit = XMFLOAT2(radius, height);
				SetTurntableOptions();
				if(Processing) SetCross();
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

			void SetShading(Views view)
			{
				Params.View = view;
				DrawsOverlay = view == Views::Overlay;
			}

			void SetTurntablePosition(float angle)
			{
				Params.Rotation = angle;
				SetTurntableOptions();
			}

			void SetTurntableOptions()
			{
				XMMATRIX DepthIntrinsics = XMLoadFloat4x4(&Params.DepthIntrinsics);
				XMMATRIX World = XMLoadFloat4x4(&Params.World);
				XMMATRIX TurntableTransform = XMLoadFloat4x4(&Params.TurntableTransform) * XMMatrixRotationY(Params.Rotation);

				XMMATRIX TurntableToScreenTransform = DepthIntrinsics * World * TurntableTransform;
				XMStoreFloat4x4(&TurntableOptions.TurntableToScreenTransform, TurntableToScreenTransform);
			}
		};
	}
}