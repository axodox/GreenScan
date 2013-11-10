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
			enum class Modes
			{
				OneAxis,
				TwoAxis,
				Volumetric
			};

			enum class AxialViews {
				Overlay,
				DepthL,
				DepthR,
				TextureL,
				TextureR,
				Model
			};

			enum class VolumetricViews {
				Overlay,
				Projection,
				Slice,
				Model
			};

			enum class States
			{
				Unknown,
				Stopped,
				Processing
			} State;
		private:		
			GraphicsDeviceWithSwapChain* Device;
			RenderTarget2D **ModelTargets, **TextureTargets, *OrthoTarget, *FilterTarget;
			RenderTarget3D *CubeTarget;
			RenderTarget2DGroup *PolarTargetGroup;
			ReadableRenderTarget2D **SaveModelTargets, **SaveTextureTargets;
			ReadableRenderTarget3D *SaveCubeTarget;
			Mesh<VertexPositionNormal, unsigned>* CubeMesh;
			int NextModelWidth, NextModelHeight, NextTextureWidth, NextTextureHeight, TargetCount, CubeRes, CubeResExt, NextCubeRes;
			int ModelWidth, ModelHeight, TextureWidth, TextureHeight;
			bool SaveTextureReady, Scanning, ClearNext, StaticInputNext, SaveInProgress, Calibrated;
			Modes NextMode, Mode;
			struct RenderingParameters
			{
				XMFLOAT4X4 DepthIntrinsics, DepthInvIntrinsics, World, TurntableTransform;
				float Rotation;
				AxialViews View;
				VolumetricViews VolumetricView;
				float TransX, TransY, TransZ, RotX, RotY, RotZ;
				int SaveWidth, SaveHeight, SaveTextureWidth, SaveTextureHeight;
			} Params;

			struct TurntableConstants
			{
				XMFLOAT4X4 TurntableToScreenTransform;
				XMFLOAT4X4 DepthToTurntableTransform;
				XMFLOAT4X4 DepthToTextureTransform;
				XMFLOAT4X4 ModelToScreenTransform;
				XMFLOAT4X4 DepthToWorldTransform;
				XMFLOAT4X4 WorldToTurntableTransform;
				XMFLOAT4 CameraPosition;
				XMFLOAT2 CorePosition;
				XMFLOAT2 ClipLimit;
				XMFLOAT2 TextureMove;
				XMFLOAT2 TextureScale;
				XMFLOAT2 CubeSize;
				XMINT2 DepthResolution;
				XMINT2 ColorResolution;
				XMINT2 ModelResolution;
				float Side;
				float Threshold;
				float GradientLimit;
				float Slice;
				int CubeRes;				
			} TurntableOptions;

			ConstantBuffer<TurntableConstants>* Constants;
			VertexPositionColor* CrossVertices;
			VertexBuffer<VertexPositionColor>* Cross;
			VertexShader *VSOverlay, *VSTwoAxisPolar, *VSOneAxisPolar, *VSVolumetricOrtho, *VSVolumetricCube, *VSCommon, *VSModel, *VSVolumetricModel, *VSSimple;
			GeometryShader *GSTwoAxisPolar, *GSOneAxisPolar, *GSVolumetricOrtho, *GSVolumetricCube, *GSModel;
			PixelShader *PSOverlay, *PSTwoAxisPolar, *PSOneAxisPolar, *PSVolumetricOrtho, *PSVolumetricOrthoFilter, *PSVolumetricCube, *PSPolarDepth, *PSVolumetricDepth, *PSVolumetricSlice, *PSPolarTexture, *PSTest, *PSModel, *PSVolumetricModel, *PSModelOutput, *PSTextureOutput, *PSSimple;
			BlendState *BOpaque, *BAdditive, *BAlpha;
			RasterizerState *RDefault, *RCullNone, *RWireFrame;
			ID3D11DeviceContext *Context;
			Plane *PMain;
			Line *LMain;
			Quad *QMain;
			SamplerState *SLinearClamp, *SPointClamp;
			HANDLE SaveEvent, SaveCompletedEvent;

			bool CrossChanged, RawSave, RestartOnNextFrame;
			int CrossVertexCount;
			static const int CilinderSides = 18;
			void SetCross()
			{
				if (State != States::Processing) return;
				switch (Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
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
					}
					break;
				case Modes::Volumetric:
					{
						int i = 6;
						XMFLOAT4 color = XMFLOAT4(1.f, 0.f, 1.f, 1.f);
						float a = TurntableOptions.CubeSize.x / 2.f;
						float h = TurntableOptions.CubeSize.x;
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, 0.f, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, 0.f, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, 0.f, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, 0.f, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, 0.f, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, 0.f, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, 0.f, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, 0.f, -a), color);

						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, h, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, h, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, h, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, h, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, h, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, h, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, h, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, h, -a), color);

						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, 0.f, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, h, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, 0.f, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, h, a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, 0.f, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(a, h, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, 0.f, -a), color);
						CrossVertices[i++] = VertexPositionColor(XMFLOAT3(-a, h, -a), color);
					}
					break;
				}
				CrossChanged = true;
			}

			void SetOverlay()
			{
				DrawsOverlay = (Params.View == AxialViews::Overlay && (Mode == Modes::OneAxis || Mode == Modes::TwoAxis)) || (Params.VolumetricView == VolumetricViews::Overlay && Mode == Modes::Volumetric);
			}

			void DrawIfNeeded()
			{
				if(StaticInput && State == States::Processing)
				{
					RequestDraw(Host);
				}
			}
		public:
			RotatingScannerModule()
			{
				State = States::Stopped;
				Scanning = false;
				ClearNext = false;
				DrawsOverlay = true;
				StaticInputNext = false;
				StaticInput = false;
				Device = nullptr;
				RestartOnNextFrame = false;
				NextCubeRes = CubeRes = 0;
				CubeMesh = nullptr;
				Params.Rotation = 0;
				SaveCompletedEvent = CreateEvent(0, 0, 0, 0);
				SaveInProgress = false;
				Calibrated = false;
				ModelWidth = ModelHeight = TextureWidth = TextureHeight = 0;
				NextModelWidth = NextModelHeight = NextTextureWidth = NextTextureHeight = 0;
			}

			virtual void CreateResources(GraphicsDeviceWithSwapChain* device) override
			{
				Device = device;
				VSOverlay = new VertexShader(Device, L"TurntableOverlayVertexShader.cso");
				VSOverlay->SetInputLayout(VertexDefinition::VertexPositionColor);
				VSTwoAxisPolar = new VertexShader(Device, L"TurntableTwoAxisPolarVertexShader.cso");
				VSTwoAxisPolar->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSOneAxisPolar = new VertexShader(Device, L"TurntableOneAxisPolarVertexShader.cso");
				VSOneAxisPolar->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSVolumetricOrtho = new VertexShader(Device, L"TurntableVolumetricOrthoVertexShader.cso");
				VSVolumetricOrtho->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSVolumetricCube = new VertexShader(Device, L"TurntableVolumetricCubeVertexShader.cso");
				VSVolumetricCube->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSCommon = new VertexShader(Device, L"CommonVertexShader.cso");
				VSCommon->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSModel = new VertexShader(Device, L"TurntableModelVertexShader.cso");
				VSModel->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSVolumetricModel = new VertexShader(Device, L"VolumetricModelVertexShader.cso");
				VSVolumetricModel->SetInputLayout(VertexDefinition::VertexPositionNormal);
				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(VertexDefinition::VertexPositionTexture);

				GSTwoAxisPolar = new GeometryShader(Device, L"TurntableTwoAxisPolarGeometryShader.cso");
				GSOneAxisPolar = new GeometryShader(Device, L"TurntableOneAxisPolarGeometryShader.cso");
				GSVolumetricOrtho = new GeometryShader(Device, L"TurntableVolumetricOrthoGeometryShader.cso");
				GSVolumetricCube = new GeometryShader(Device, L"TurntableVolumetricCubeGeometryShader.cso");
				GSModel = new GeometryShader(Device, L"TurntableModelGeometryShader.cso");
				
				PSOverlay = new PixelShader(Device, L"TurntableOverlayPixelShader.cso");
				PSTwoAxisPolar = new PixelShader(Device, L"TurntableTwoAxisPolarPixelShader.cso");
				PSOneAxisPolar = new PixelShader(Device, L"TurntableOneAxisPolarPixelShader.cso");
				PSVolumetricOrtho = new PixelShader(Device, L"TurntableVolumetricOrthoPixelShader.cso");
				PSVolumetricOrthoFilter = new PixelShader(Device, L"TurntableVolumetricOrthoFilterPixelShader.cso");
				PSVolumetricCube = new PixelShader(Device, L"TurntableVolumetricCubePixelShader.cso");
				PSPolarDepth = new PixelShader(Device, L"TurntablePolarDepthPixelShader.cso");
				PSVolumetricDepth = new PixelShader(Device, L"TurntableVolumetricDepthPixelShader.cso");
				PSVolumetricSlice = new PixelShader(Device, L"TurntableVolumetricSlicePixelShader.cso");
				PSPolarTexture = new PixelShader(Device, L"TurntablePolarTexturePixelShader.cso");
				PSModel = new PixelShader(Device, L"TurntableModelPixelShader.cso");
				PSVolumetricModel = new PixelShader(Device, L"VolumetricModelPixelShader.cso");
				PSTest = new PixelShader(Device, L"TurntableTestPixelShader.cso");
				PSSimple = new PixelShader(Device, L"SimplePixelShader.cso");
				PSModelOutput = new PixelShader(Device, L"TurntableOutputModelPixelShader.cso");
				PSTextureOutput = new PixelShader(Device, L"TurntableOutputTexturePixelShader.cso");

				Context = Device->GetImmediateContext();
			}

			virtual void DestroyResources() override
			{
				EndProcessing();
				delete VSOverlay;
				delete VSTwoAxisPolar;
				delete VSOneAxisPolar;
				delete VSVolumetricOrtho;
				delete VSVolumetricCube;
				delete VSCommon;
				delete VSModel;
				delete VSVolumetricModel;
				delete VSSimple;
				delete GSTwoAxisPolar;
				delete GSOneAxisPolar;
				delete GSVolumetricOrtho;
				delete GSVolumetricCube;
				delete GSModel;
				delete PSOverlay;
				delete PSTwoAxisPolar;
				delete PSOneAxisPolar;
				delete PSVolumetricOrtho;
				delete PSVolumetricOrthoFilter;
				delete PSVolumetricCube;
				delete PSPolarDepth;
				delete PSVolumetricDepth;
				delete PSVolumetricSlice;
				delete PSPolarTexture;
				delete PSModel;
				delete PSVolumetricModel;
				delete PSTest;
				delete PSSimple;
				delete PSModelOutput;
				delete PSTextureOutput;
				Context->Release();
			}

			virtual void StartProcessing() override
			{
				if (!Device || State == States::Unknown) return;
				if (State == States::Processing) EndProcessing();
				State = States::Unknown;
				StaticInput = StaticInputNext;
				StaticInputNext = false;
				TurntableOptions.ModelResolution = XMINT2(NextModelWidth, NextModelHeight);	

				Constants = new ConstantBuffer<TurntableConstants>(Device);
				TurntableOptions.DepthResolution = XMINT2(KinectDevice::DepthWidth, KinectDevice::DepthHeight);
				TurntableOptions.ColorResolution = XMINT2(KinectDevice::ColorWidth, KinectDevice::ColorHeight);

				BOpaque = new BlendState(Device, BlendState::Opaque);
				BAdditive = new BlendState(Device, BlendState::Additive);
				BAlpha = new BlendState(Device, BlendState::AlphaBlend);
				RDefault = new RasterizerState(Device, RasterizerState::Default);
				RCullNone = new RasterizerState(Device, RasterizerState::CullNone);
				RWireFrame = new RasterizerState(Device, RasterizerState::Wireframe);
				PMain = new Plane(Device, KinectDevice::DepthWidth, KinectDevice::DepthHeight);

				QMain = new Quad(Device);
				SLinearClamp = new SamplerState(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
				SPointClamp = new SamplerState(Device, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
				SaveTextureReady = true;

				Mode = NextMode;
				SetOverlay();
				switch (Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
					{
						ModelWidth = NextModelWidth;
						ModelHeight = NextModelHeight;
						TextureWidth = NextTextureWidth;
						TextureHeight = NextTextureHeight;
						CrossVertexCount = 2 * CilinderSides + 10;
						TargetCount = (Mode == Modes::OneAxis ? 1 : 2);
						ModelTargets = new RenderTarget2D*[TargetCount];
						TextureTargets = new RenderTarget2D*[TargetCount];
						RenderTarget2D** targets = new RenderTarget2D*[TargetCount * 2];
						for(int i = 0; i < TargetCount; i++)
						{
							ModelTargets[i] = new RenderTarget2D(Device, ModelWidth, ModelHeight, DXGI_FORMAT_R32G32_FLOAT);
							targets[2 * i] = ModelTargets[i];
							TextureTargets[i] = new RenderTarget2D(Device, TextureWidth, TextureHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
							targets[2 * i + 1] = TextureTargets[i];
						}
						PolarTargetGroup = new RenderTarget2DGroup(Device, TargetCount * 2, targets);
						delete [TargetCount * 2] targets;
					}
					break;
				case Modes::Volumetric:
					CrossVertexCount = 6 + 3 * 2 * 4;
					CubeRes = NextCubeRes;
					CubeResExt = CubeRes + 1;
					OrthoTarget = new RenderTarget2D(Device, CubeRes, CubeRes, DXGI_FORMAT_R32_FLOAT);
					FilterTarget = new RenderTarget2D(Device, CubeRes, CubeRes, DXGI_FORMAT_R32_FLOAT);
					CubeTarget = new RenderTarget3D(Device, CubeRes, CubeRes, CubeRes, DXGI_FORMAT_R8_UNORM);
					LMain = new Line(Device, CubeRes);
					TurntableOptions.CubeRes = CubeRes;
					break;
				}

				CrossVertices = new VertexPositionColor[CrossVertexCount];
				CrossVertices[0] = VertexPositionColor(XMFLOAT3(-0.1f, 0.f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f));
				CrossVertices[1] = VertexPositionColor(XMFLOAT3(0.1f, 0.f, 0.f), XMFLOAT4(1.f, 0.f, 0.f, 1.f));
				CrossVertices[2] = VertexPositionColor(XMFLOAT3(0.f, -0.1f, 0.f), XMFLOAT4(0.f, 1.f, 0.f, 1.f));
				CrossVertices[3] = VertexPositionColor(XMFLOAT3(0.f, 0.1f, 0.f), XMFLOAT4(0.f, 1.f, 0.f, 1.f));
				CrossVertices[4] = VertexPositionColor(XMFLOAT3(0.f, 0.f, -0.1f), XMFLOAT4(0.f, 0.f, 1.f, 1.f));
				CrossVertices[5] = VertexPositionColor(XMFLOAT3(0.f, 0.f, 0.1f), XMFLOAT4(0.f, 0.f, 1.f, 1.f));
				
				Cross = new VertexBuffer<VertexPositionColor>(Device, CrossVertexCount);				
				State = States::Processing;
				SetCross();	
			}

			virtual void EndProcessing() override
			{
				if (State != States::Processing) return;
				State = States::Unknown;
				if(SaveInProgress) 
					WaitForSingleObject(SaveCompletedEvent, INFINITE);
				switch (Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
					for(int i = 0; i < TargetCount; i++)
					{
						delete ModelTargets[i];
						delete TextureTargets[i];
					}
					delete [TargetCount] ModelTargets;
					delete [TargetCount] TextureTargets;
					delete PolarTargetGroup;
					break;
				case Modes::Volumetric:
					delete OrthoTarget;
					delete FilterTarget;
					delete CubeTarget;
					delete LMain;
					break;
				}				
				delete Cross;
				delete CrossVertices;
				delete Constants;				
				delete BOpaque;
				delete BAdditive;
				delete BAlpha;
				delete RDefault;
				delete RCullNone;
				delete RWireFrame;
				delete PMain;
				delete QMain;
				delete SLinearClamp;
				delete SPointClamp;
				State = States::Stopped;
			}

			void Scan()
			{
				if(Mode != NextMode) StartProcessing();
				switch (Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
					PolarTargetGroup->Clear();
					break;
				case Modes::Volumetric:
					CubeTarget->Clear();
					break;
				}				
				Scanning = true;
			}

			void Stop()
			{
				Scanning = false;
				if(State == States::Processing && Mode == Modes::Volumetric) CalculateCubeMesh();
			}

			virtual void ProcessFrame(RenderTargetPair* depth, Texture2D* color) override
			{
				if (State != States::Processing || !Scanning) return;
				Constants->Update(&TurntableOptions);
				Constants->SetForVS(2);
				Constants->SetForGS(2);
				Constants->SetForPS(2);

				switch(Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
					if (ClearNext)
					{
						ClearNext = false;
						PolarTargetGroup->Clear();
					}
										
					RCullNone->Set();
					BAdditive->Apply();
					PolarTargetGroup->SetRenderTargets();
					switch (Mode)
					{
					case Modes::OneAxis:
						Device->SetShaders(VSOneAxisPolar, PSOneAxisPolar, GSOneAxisPolar);
						break;
					case Modes::TwoAxis:
						Device->SetShaders(VSTwoAxisPolar, PSTwoAxisPolar, GSTwoAxisPolar);
						break;
					}					
					depth->SetForVS();
					color->SetForPS();
					SLinearClamp->SetForPS();
					PMain->Draw();
					break;
				case Modes::Volumetric:
					if (ClearNext)
					{
						ClearNext = false;
						CubeTarget->Clear();
					}

					OrthoTarget->Clear();
					RCullNone->Set();
					BOpaque->Apply();					
					OrthoTarget->SetAsRenderTarget();
					Device->SetShaders(VSVolumetricOrtho, PSVolumetricOrtho, GSVolumetricOrtho);
					depth->SetForVS();					
					PMain->Draw();

					FilterTarget->Clear();				
					FilterTarget->SetAsRenderTarget();
					Device->SetShaders(VSSimple, PSVolumetricOrthoFilter);
					OrthoTarget->SetForPS();
					SLinearClamp->SetForPS();
					QMain->Draw();

					CubeTarget->SetAsRenderTarget();
					BAdditive->Apply();
					Device->SetShaders(VSVolumetricCube, PSVolumetricCube, GSVolumetricCube);
					FilterTarget->SetForVS();
					LMain->DrawInstanced(CubeRes);
					break;
				}
			}

			virtual void SetSave(int width, int height, int texWidth, int texHeight) override
			{
				Params.SaveWidth = width;
				Params.SaveHeight = height;
				Params.SaveTextureWidth = texWidth;
				Params.SaveTextureHeight = texHeight;
			}

			template <class T> void GetPolarData(ReadableRenderTarget2D* target, T* data)
			{
				int width = target->GetWidth(), polarWidth = width + 1, height = target->GetHeight();
				ID3D11Texture2D* StagingTexture = target->GetStagingTexture();
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Context->Map(StagingTexture, 0, D3D11_MAP_READ, 0, &ms));
				for(int row = 0; row < height; row++)
				{
					memcpy(data + row * polarWidth, (byte*)ms.pData + row * ms.RowPitch, width * sizeof(T));
					memcpy(data + (row + 1) * polarWidth - 1, (byte*)ms.pData + row * ms.RowPitch, sizeof(T));
				}
				Context->Unmap(StagingTexture, 0);
			}

			static const int SaveVersion = 0;
			bool OpenRaw(LPWSTR path, LPWSTR &metadata)
			{
				if(Scanning) return false;
				bool ok = false;
				FILE* file;
				if(_wfopen_s(&file, path, L"rb") == 0)
				{
					int version;
					fread(&version, 4, 1, file);
					Modes mode;
					fread(&mode, 4, 1, file);
					if(version == SaveVersion)
					{
						NextMode = mode;
						StaticInputNext = true;						

						switch(NextMode)
						{
						case Modes::OneAxis:
						case Modes::TwoAxis:
							{
								StartProcessing();
								int modelWidth, modelHeight, texWidth, texHeight;
								fread(&modelWidth, 4, 1, file);
								fread(&modelHeight, 4, 1, file);
								int modelLen = modelWidth * modelHeight;
								XMFLOAT2* modelData = new XMFLOAT2[modelLen];

								for(int i = 0; i < TargetCount; i++)
								{
									fread(modelData, sizeof(XMFLOAT2), modelLen, file);
									Texture2D* model = new Texture2D(Device, modelWidth, modelHeight, DXGI_FORMAT_R32G32_FLOAT, modelData, modelWidth * sizeof(XMFLOAT2));
									ModelTargets[i]->Load(model);
									SafeDelete(model);
								}
								delete [modelLen] modelData;

								fread(&texWidth, 4, 1, file);
								fread(&texHeight, 4, 1, file);
								int texLen = texWidth * texHeight;
								XMFLOAT4* textureData = new XMFLOAT4[texLen];
								for(int i = 0; i < TargetCount; i++)
								{
									fread(textureData, sizeof(XMFLOAT4), texLen, file);
									Texture2D* texture = new Texture2D(Device, texWidth, texHeight, DXGI_FORMAT_R32G32B32A32_FLOAT, textureData, texWidth * sizeof(XMFLOAT4));
									TextureTargets[i]->Load(texture);
									SafeDelete(texture);
								}
								delete [texLen] textureData;
							}
							break;
						case Modes::Volumetric:
							{
								fread(&NextCubeRes, 4, 1, file);
								StartProcessing();
								int modelLen = pow(CubeRes, 3);
								BYTE* modelData = new BYTE[modelLen];
								fread(modelData, 1, modelLen, file);
								Texture3D* texture = new Texture3D(Device, CubeRes, CubeRes, CubeRes, DXGI_FORMAT_R8_UNORM, modelData, CubeRes, CubeRes * CubeRes);
								CubeTarget->Load(texture);
								SafeDelete(texture);
								delete [modelLen] modelData;
								CalculateCubeMesh();
							}
							break;
						}						
						
						unsigned metalen;
						fread(&metalen, 4, 1, file);
						metadata = new WCHAR[metalen];
						fread(metadata, 2, metalen, file);

						ok = true;
						DrawIfNeeded();
					}
					fclose(file);
				}
				return ok;
			}

			bool SaveRaw(LPWSTR path, LPWSTR metadata)
			{
				if (State != States::Processing) return false;
				RawSave = true;
				SaveEvent = CreateEvent(0, 0, 0, 0);

				switch(Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
					SaveModelTargets = new ReadableRenderTarget2D*[TargetCount];
					SaveTextureTargets = new ReadableRenderTarget2D*[TargetCount];
					for(int i = 0; i < TargetCount; i++)
					{
						SaveModelTargets[i] = new ReadableRenderTarget2D(ModelTargets[i]);
						SaveTextureTargets[i] = new ReadableRenderTarget2D(TextureTargets[i]);
					}
					break;
				case Modes::Volumetric:
					SaveCubeTarget = new ReadableRenderTarget3D(CubeTarget);
					break;
				}
				
				SaveTextureReady = false;

				bool ok = false;
				if(StaticInput)
				{
					Draw();
					ok = true;
				}
				else
					ok = WaitForSingleObject(SaveEvent, 1000) == WAIT_OBJECT_0;

				if(ok)
				{
					FILE* file;
					if(_wfopen_s(&file, path, L"wb") == 0)
					{
						fwrite(&SaveVersion, 4, 1, file);
						fwrite(&Mode, 4, 1, file);
						
						switch(Mode)
						{
						case Modes::OneAxis:
						case Modes::TwoAxis:
							{
								fwrite(&ModelWidth, 4, 1, file);
								fwrite(&ModelHeight, 4, 1, file);
								int modelLen = ModelWidth * ModelHeight;
								XMFLOAT2* modelData = new XMFLOAT2[modelLen];

								for(int i = 0; i < TargetCount; i++)
								{
									SaveModelTargets[i]->GetData<XMFLOAT2>(modelData);
									fwrite(modelData, sizeof(XMFLOAT2), modelLen, file);
								}
								delete [modelLen] modelData;

								fwrite(&TextureWidth, 4, 1, file);
								fwrite(&TextureHeight, 4, 1, file);
								int texLen = TextureWidth * TextureHeight;
								XMFLOAT4* textureData = new XMFLOAT4[texLen];

								for(int i = 0; i < TargetCount; i++)
								{
									SaveTextureTargets[i]->GetData<XMFLOAT4>(textureData);
									fwrite(textureData, sizeof(XMFLOAT4), texLen, file);
								}

								delete [texLen] textureData;
							}
							break;
						case Modes::Volumetric:
							{
								fwrite(&CubeRes, 4, 1, file);
								int modelLen = pow(CubeRes, 3);
								BYTE* modelData = new BYTE[modelLen];
								SaveCubeTarget->GetData(modelData);
								fwrite(modelData, 1, modelLen, file);
								delete [modelLen] modelData;
							}
							break;
						}

						unsigned metalen = wcslen(metadata) + 1;
						fwrite(&metalen, 4, 1, file);
						fwrite((char*)metadata, 2, metalen, file);						
						fclose(file);
					}
				}

				switch(Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
					for(int i = 0; i < TargetCount; i++)
					{
						SafeDelete(SaveModelTargets[i]);
						SafeDelete(SaveTextureTargets[i]);
					}
					delete [TargetCount] SaveModelTargets;
					delete [TargetCount] SaveTextureTargets;
					break;
				case Modes::Volumetric:
					SafeDelete(SaveCubeTarget);
					break;
				}

				CloseHandle(SaveEvent);
				return ok;
			}

			bool SaveModel(LPWSTR path, ModelFormats format)
			{
				if(State != States::Processing) return false;
				ResetEvent(SaveCompletedEvent);
				SaveInProgress = true;
				bool ok = false;
				if(Mode == Modes::Volumetric)
				{
					ok = SaveCube(path, format);
				}
				else
				{
					RawSave = false;
					SaveEvent = CreateEvent(0, 0, 0, 0);

					SaveModelTargets = new ReadableRenderTarget2D*[TargetCount];
					SaveTextureTargets = new ReadableRenderTarget2D*[TargetCount];
					for(int i = 0; i < TargetCount; i++)
					{
						SaveModelTargets[i] = new ReadableRenderTarget2D(Device, Params.SaveWidth, Params.SaveHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
						SaveTextureTargets[i] = new ReadableRenderTarget2D(Device, Params.SaveTextureWidth, Params.SaveTextureHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
					}
					SaveTextureReady = false;

					if(StaticInput)
					{
						Draw();
						ok = true;
					}
					else
						ok = WaitForSingleObject(SaveEvent, 1000) == WAIT_OBJECT_0;

					if(ok)
					{
						WCHAR** textureFilenames = new WCHAR*[TargetCount];
						WCHAR** filenames = new WCHAR*[TargetCount];
						for(int i = 0; i < TargetCount; i++)
						{
							textureFilenames[i] = new WCHAR[MAX_PATH];
							filenames[i] = new WCHAR[MAX_PATH];
							wcscpy(textureFilenames[i], path);
							wcscpy(filenames[i], path);
						}

						switch (Mode)
						{
						case Modes::OneAxis:
							wcscat(textureFilenames[0], L".png");
							break;
						case Modes::TwoAxis:
							wcscat(textureFilenames[0], L"_l.png");
							wcscat(textureFilenames[1], L"_r.png");
							wcscat(filenames[0], L"_l");
							wcscat(filenames[1], L"_r");
							break;
						}

						int dataLen = (Params.SaveWidth + 1) * Params.SaveHeight;
						for(int i = 0; i < TargetCount; i++)
						{					
							XMFLOAT4* data = new XMFLOAT4[dataLen];
							GetPolarData(SaveModelTargets[i], data);
							switch (format)
							{
							case ModelFormats::STL:
								ok &= STLSave(filenames[i], data, Params.SaveWidth + 1, Params.SaveHeight);
								break;
							case ModelFormats::FBX:
								ok &= FBXSave(filenames[i], data, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenames[i], L'\\') + 1, L"fbx");
								break;
							case ModelFormats::DXF:
								ok &= FBXSave(filenames[i], data, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenames[i], L'\\') + 1, L"dxf");
								break;
							case ModelFormats::DAE:
								ok &= FBXSave(filenames[i], data, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenames[i], L'\\') + 1, L"dae");
								break;
							case ModelFormats::OBJ:
								ok &= FBXSave(filenames[i], data, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenames[i], L'\\') + 1, L"obj");
								break;
							case ModelFormats::FL4:
								ok &= FL4Save(filenames[i], data, Params.SaveWidth + 1, Params.SaveHeight);
								break;
							default:
								ok = false;
								break;
							}
							delete [dataLen] data;

							ok &= PNGSave(textureFilenames[i], SaveTextureTargets[i]->GetStagingTexture());						
						}					

						for(int i = 0; i < TargetCount; i++)
						{
							delete [MAX_PATH] filenames[i];
							delete [MAX_PATH] textureFilenames[i];
						}
						delete [TargetCount] filenames;
						delete [TargetCount] textureFilenames;
					}

					for(int i = 0; i < TargetCount; i++)
					{
						SafeDelete(SaveModelTargets[i]);
						SafeDelete(SaveTextureTargets[i]);
					}
					delete [TargetCount] SaveModelTargets;
					delete [TargetCount] SaveTextureTargets;
					CloseHandle(SaveEvent);
				}
				SetEvent(SaveCompletedEvent);
				SaveInProgress = false;
				return ok;
			}
		private:
			bool IsVoxelFilled(byte* const data, int x, int y, int z)
			{
				if(x < 0 || y < 0 || z < 0 || x >= CubeRes || y >= CubeRes || z >= CubeRes)
					return false;
				else
					return *(data + x + CubeRes * y + CubeRes * CubeRes * z) == 255;
			}

			void CheckVertex(unsigned* const vertices, int x, int y, int z, unsigned &index)
			{
				if(x < 0 || y < 0 || z < 0 || x >= CubeResExt || y >= CubeResExt || z >= CubeResExt)
					return;
				unsigned* vertex = vertices + x + CubeResExt * y + CubeResExt * CubeResExt * z;
				if(*vertex == 0)
				{
					*vertex = ++index;
				}
			}

			unsigned GetIndex(unsigned* const vertices, int x, int y, int z)
			{
				if(x < 0 || y < 0 || z < 0 || x >= CubeResExt || y >= CubeResExt || z >= CubeResExt)
					return 0u;
				return *(vertices + x + CubeResExt * y + CubeResExt * CubeResExt * z) - 1u;
			}

			bool GetCube(VertexPosition* &vertices, unsigned &vertexCount, unsigned* &indicies, unsigned &indexCount)
			{
				RawSave = false;
				SaveEvent = CreateEvent(0, 0, 0, 0);
								
				SaveCubeTarget = new ReadableRenderTarget3D(CubeTarget);
				SaveTextureReady = false;

				bool ok = false;
				if(StaticInput)
				{
					Draw();
					ok = true;
				}
				else
					ok = WaitForSingleObject(SaveEvent, 1000) == WAIT_OBJECT_0;

				if(ok)
				{
					int modelLen = pow(CubeRes, 3);
					byte* modelData = new byte[modelLen], *pModelData, *eModelData = modelData + modelLen;
					SaveCubeTarget->GetData<byte>(modelData);
					
					//Thresholding
					byte threshold = TurntableOptions.Threshold * 255;
					for(pModelData = modelData; pModelData < eModelData; pModelData++)
						if(*pModelData > threshold) *pModelData = 255;
						else *pModelData = 0;
					
					//Mark outer space with scanline flood-fill
					stack<unsigned> voxelsToFill;
					unsigned levelSize = CubeRes * CubeRes, index;
					byte* pLevel, *pVoxel;
					bool lastPushUp, lastPushDown, scanDirection;
					for(int z = 0; z < CubeRes; z++)
					{
						for(int i = 0; i < CubeRes; i++)
						{
							voxelsToFill.push(i);
							voxelsToFill.push(levelSize - i - 1);
							voxelsToFill.push(CubeRes * i);
							voxelsToFill.push(CubeRes * (i + 1) - 1);
						}
						
						pLevel = modelData + levelSize * z;
						while(!voxelsToFill.empty())
						{
							index = voxelsToFill.top();
							voxelsToFill.pop();
							pVoxel = pLevel + index;
							if(*pVoxel == 255) continue;
							*pVoxel = 1;
							
							lastPushUp = lastPushDown = false;
							unsigned x = index % CubeRes, y = index / CubeRes;
							scanDirection = true;
							int i = x;
							while(true)
							{
								if(scanDirection)
								{
									i--;
									pVoxel--;
									if(i < 0 || *pVoxel == 255)
									{
										i = x;
										pVoxel = pLevel + index;
										scanDirection = false;
										lastPushUp = lastPushDown = false;
										continue;
									}
								}
								else
								{
									i++;
									pVoxel++;
									if(i == CubeRes || *pVoxel == 255)
									{
										break;
									}
								}

								*pVoxel = 1;

								if(y > 0 && *(pVoxel - CubeRes) == 0)
								{
									if(!lastPushUp)
									{
										voxelsToFill.push(i + CubeRes * (y - 1));
										lastPushUp = true;
									}
								}
								else 
									lastPushUp = false;

								if(y < CubeRes - 1 && *(pVoxel + CubeRes) == 0)
								{
									if(!lastPushDown)
									{
										voxelsToFill.push(i + CubeRes * (y + 1));
										lastPushDown = true;
									}
								}
								else
									lastPushDown = false;
							}
						}
					}

					//Fill closed spaces
					for(pModelData = modelData; pModelData < eModelData; pModelData++)
						if(*pModelData == 0) *pModelData = 255;
					
					//Count quads
					int modelLenExt = pow(CubeResExt, 3);
					bool a, b, c, p;
					vertexCount = 0;
					unsigned* indexCube = new unsigned[modelLenExt], *pIndex = indexCube, faceCount = 0;
					ZeroMemory(indexCube, modelLenExt * 4);
					byte* faceCube = new byte[modelLenExt], *pFace = faceCube;
					ZeroMemory(faceCube, modelLenExt);
					for(int z = 0; z < CubeResExt; z++)
					for(int y = 0; y < CubeResExt; y++)
					for(int x = 0; x < CubeResExt; x++)
					{
						p = IsVoxelFilled(modelData, x, y, z);
						a = IsVoxelFilled(modelData, x - 1, y, z);
						b = IsVoxelFilled(modelData, x, y - 1, z);
						c = IsVoxelFilled(modelData, x, y, z - 1);
						if(p != a)
						{
							if(p) *pFace |= 1;
							else *pFace |= 2;
							faceCount++;
							CheckVertex(indexCube, x, y, z, vertexCount);
							CheckVertex(indexCube, x, y + 1, z, vertexCount);
							CheckVertex(indexCube, x, y + 1, z + 1, vertexCount);
							CheckVertex(indexCube, x, y, z + 1, vertexCount);
						}
						if(p != b)
						{
							if(p) *pFace |= 4;
							else *pFace |= 8;
							faceCount++;
							CheckVertex(indexCube, x, y, z, vertexCount);
							CheckVertex(indexCube, x + 1, y, z, vertexCount);
							CheckVertex(indexCube, x + 1, y, z + 1, vertexCount);
							CheckVertex(indexCube, x, y, z + 1, vertexCount);
						}
						if(p != c)
						{
							if(p) *pFace |= 16;
							else *pFace |= 32;
							faceCount++;
							CheckVertex(indexCube, x, y, z, vertexCount);
							CheckVertex(indexCube, x, y + 1, z, vertexCount);
							CheckVertex(indexCube, x + 1, y, z, vertexCount);
							CheckVertex(indexCube, x + 1, y + 1, z, vertexCount);
						}
						pFace++;
					}
					delete [modelLen] modelData;

					//Buil model
					indexCount = faceCount * 2 * 3;
					vertices = new VertexPosition[vertexCount];
					indicies = new unsigned[indexCount];
					VertexPosition* pVertices = vertices;
					unsigned* pIndicies = indicies;
					pFace = faceCube;
					pIndex = indexCube;
					float cubeSize = TurntableOptions.CubeSize.x;
					float start = -cubeSize / 2.f;
					float cubeStep = cubeSize / CubeRes;
					for(int z = 0; z < CubeResExt; z++)
					for(int y = 0; y < CubeResExt; y++)
					for(int x = 0; x < CubeResExt; x++)
					{
						if(*pIndex > 0) pVertices[*pIndex - 1] = VertexPosition(start + x * cubeStep, z * cubeStep, start + y * cubeStep);
						if(*pFace & 1)
						{
							*pIndicies++ = GetIndex(indexCube, x, y, z);
							*pIndicies++ = GetIndex(indexCube, x, y + 1, z + 1);
							*pIndicies++ = GetIndex(indexCube, x, y, z + 1);

							*pIndicies++ = GetIndex(indexCube, x, y, z);
							*pIndicies++ = GetIndex(indexCube, x, y + 1, z);
							*pIndicies++ = GetIndex(indexCube, x, y + 1, z + 1);
						}
						if(*pFace & 2)
						{
							*pIndicies++ = GetIndex(indexCube, x, y, z + 1);							
							*pIndicies++ = GetIndex(indexCube, x, y + 1, z + 1);
							*pIndicies++ = GetIndex(indexCube, x, y, z);

							*pIndicies++ = GetIndex(indexCube, x, y + 1, z + 1);
							*pIndicies++ = GetIndex(indexCube, x, y + 1, z);
							*pIndicies++ = GetIndex(indexCube, x, y, z);							
						}
						if(*pFace & 4)
						{
							*pIndicies++ = GetIndex(indexCube, x, y, z);
							*pIndicies++ = GetIndex(indexCube, x, y, z + 1);
							*pIndicies++ = GetIndex(indexCube, x + 1, y, z + 1);

							*pIndicies++ = GetIndex(indexCube, x, y, z);
							*pIndicies++ = GetIndex(indexCube, x + 1, y, z + 1);
							*pIndicies++ = GetIndex(indexCube, x + 1, y, z);
						}
						if(*pFace & 8)
						{
							*pIndicies++ = GetIndex(indexCube, x + 1, y, z + 1);
							*pIndicies++ = GetIndex(indexCube, x, y, z + 1);
							*pIndicies++ = GetIndex(indexCube, x, y, z);							

							*pIndicies++ = GetIndex(indexCube, x + 1, y, z);
							*pIndicies++ = GetIndex(indexCube, x + 1, y, z + 1);							
							*pIndicies++ = GetIndex(indexCube, x, y, z);
						}
						if(*pFace & 16)
						{
							*pIndicies++ = GetIndex(indexCube, x, y, z);
							*pIndicies++ = GetIndex(indexCube, x + 1, y + 1, z);
							*pIndicies++ = GetIndex(indexCube, x, y + 1, z);

							*pIndicies++ = GetIndex(indexCube, x, y, z);
							*pIndicies++ = GetIndex(indexCube, x + 1, y, z);
							*pIndicies++ = GetIndex(indexCube, x + 1, y + 1, z);
						}
						if(*pFace & 32)
						{
							*pIndicies++ = GetIndex(indexCube, x, y + 1, z);
							*pIndicies++ = GetIndex(indexCube, x + 1, y + 1, z);							
							*pIndicies++ = GetIndex(indexCube, x, y, z);

							*pIndicies++ = GetIndex(indexCube, x + 1, y + 1, z);
							*pIndicies++ = GetIndex(indexCube, x + 1, y, z);							
							*pIndicies++ = GetIndex(indexCube, x, y, z);
						}
						pFace++;
						pIndex++;
					}					
					delete [modelLenExt] faceCube;
					delete [modelLenExt] indexCube;	

					//Smooth
					SmoothMesh(vertices, vertexCount, indicies, indexCount);
				}
				SafeDelete(SaveCubeTarget);
				CloseHandle(SaveEvent);
				return ok;
			}
			
#define SmoothingMaxConnections 12
#define SmoothingIterations 16
			void AddToMap(unsigned* map, unsigned source, unsigned target)
			{
				map += source * SmoothingMaxConnections;
				for(int i = 0; i < SmoothingMaxConnections; i++)
				{
					if(*map == MAXUINT)
					{
						*map = target;
						return;
					}
					if(*map == target) return;
					map++;
				}
				//This should newer happen
				throw 0;
			}

			void SmoothMesh(VertexPosition* vertices, unsigned vertexCount, const unsigned* const indicies, unsigned indexCount)
			{
				//Build map
				int mapLength = vertexCount * SmoothingMaxConnections;
				unsigned* map = new unsigned[mapLength];
				FillMemory(map, 4 * mapLength, 0xFF);

				const unsigned* pIndicies = indicies;
				for(int i = 0; i < indexCount; i += 3)
				{
					AddToMap(map, *pIndicies, *(pIndicies + 1));
					AddToMap(map, *(pIndicies + 1), *pIndicies);

					AddToMap(map, *pIndicies, *(pIndicies + 2));
					AddToMap(map, *(pIndicies + 2), *pIndicies);

					AddToMap(map, *(pIndicies + 1), *(pIndicies + 2));
					AddToMap(map, *(pIndicies + 2), *(pIndicies + 1));
					pIndicies += 3;
				}

				//Smooth
				VertexPosition *targetVertices, *sourceVertices = vertices, *tempVertices = new VertexPosition[vertexCount];
				unsigned* pMap, *connection;
				XMVECTOR position;
				int j;

				for(int pass = 0; pass < SmoothingIterations; pass++)
				{
					pMap = map;
					targetVertices = ( pass % 2 ? vertices : tempVertices);
					sourceVertices = ( pass % 2 ? tempVertices : vertices);
					for(int i = 0; i < vertexCount; i++)
					{
						connection = pMap;
						position = XMVectorZero();
						for(j = 0; j < SmoothingMaxConnections; j++)
						{
							if(*connection == MAXUINT) break;
							else position += XMLoadFloat3(&sourceVertices[*connection].Position);
							connection++;
						}
						XMStoreFloat3(&targetVertices->Position, position / j);

						targetVertices++;
						pMap += SmoothingMaxConnections;
					}
				}

				if(SmoothingIterations % 2)
				{
					memcpy(vertices, tempVertices, vertexCount * sizeof(VertexPosition));
				}
				delete [vertexCount] tempVertices;
				delete [mapLength] map;
			}
			
			VertexPositionNormal* GenerateSmoothNormalsForMesh(const VertexPosition* const vertices, unsigned vertexCount, const unsigned* const indicies, unsigned indexCount)
			{
				VertexPositionNormal *verticesWithNormals = new VertexPositionNormal[vertexCount], *pVerticesWithNormals = verticesWithNormals;
				const VertexPosition *pVertices = vertices;
				for(int i = 0; i < vertexCount; i++)
				{
					pVerticesWithNormals->Position = pVertices->Position;
					pVerticesWithNormals->Normal = XMFLOAT3();
					pVerticesWithNormals++;
					pVertices++;
				}

				int triangleCount = indexCount / 3;				
				XMVECTOR A, B, C, N, sumNa, sumNb, sumNc;
				unsigned indexA, indexB, indexC;
				const unsigned* pIndicies = indicies;
				for(int i = 0; i < triangleCount; i++)
				{
					indexA = *pIndicies++;
					indexB = *pIndicies++;
					indexC = *pIndicies++;
					A = XMLoadFloat3(&verticesWithNormals[indexA].Position);
					B = XMLoadFloat3(&verticesWithNormals[indexB].Position);
					C = XMLoadFloat3(&verticesWithNormals[indexC].Position);
					sumNa = XMLoadFloat3(&verticesWithNormals[indexA].Normal);
					sumNb = XMLoadFloat3(&verticesWithNormals[indexB].Normal);
					sumNc = XMLoadFloat3(&verticesWithNormals[indexC].Normal);
					N = XMVector3Normalize(XMVector3Cross(A - B, A - C));
					XMStoreFloat3(&verticesWithNormals[indexA].Normal, N + sumNa);
					XMStoreFloat3(&verticesWithNormals[indexB].Normal, N + sumNb);
					XMStoreFloat3(&verticesWithNormals[indexC].Normal, N + sumNc);
				}

				pVerticesWithNormals = verticesWithNormals;
				for(int i = 0; i < vertexCount; i++)
				{
					N = XMLoadFloat3(&pVerticesWithNormals->Normal);
					XMStoreFloat3(&pVerticesWithNormals->Normal, XMVector3Normalize(N));
					pVerticesWithNormals++;
				}

				return verticesWithNormals;
			}

			bool CalculateCubeMesh()
			{
				VertexPosition* vertices = nullptr;
				unsigned* indicies = nullptr;
				unsigned vertexCount = 0, indexCount = 0;

				bool ok = GetCube(vertices, vertexCount, indicies, indexCount);
				if(!ok) return false;

				VertexPositionNormal* verticesWithNormals = GenerateSmoothNormalsForMesh(vertices, vertexCount, indicies, indexCount);
				delete [vertexCount] vertices;

				SafeDelete(CubeMesh);
				if(vertexCount > 0) CubeMesh = new Mesh<VertexPositionNormal, unsigned>(Device, verticesWithNormals, vertexCount, indicies, indexCount, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				delete [vertexCount] verticesWithNormals;
				delete [indexCount] indicies;
			}
		public:
			bool SaveCube(LPWSTR path, ModelFormats format)
			{
				if(State != States::Processing || Mode != Modes::Volumetric) return false;

				VertexPosition* vertices = nullptr;
				unsigned* indicies = nullptr;
				unsigned vertexCount = 0, indexCount = 0;

				bool ok = GetCube(vertices, vertexCount, indicies, indexCount);
				if(!ok) return false;

				VertexPositionNormal* verticesWithNormals = GenerateSmoothNormalsForMesh(vertices, vertexCount, indicies, indexCount);
				delete [vertexCount] vertices;
				
				switch (format)
				{
				case ModelFormats::FBX:
					ok = FBXMeshSave(path, verticesWithNormals, vertexCount, indicies, indexCount, L"fbx");
					break;
				case ModelFormats::DXF:
					ok = FBXMeshSave(path, verticesWithNormals, vertexCount, indicies, indexCount, L"dxf");
					break;
				case ModelFormats::DAE:
					ok = FBXMeshSave(path, verticesWithNormals, vertexCount, indicies, indexCount, L"dae");
					break;
				case ModelFormats::OBJ:
					ok = FBXMeshSave(path, verticesWithNormals, vertexCount, indicies, indexCount, L"obj");
					break;
				default:
					ok = false;
					break;
				}		
				
				delete [vertexCount] verticesWithNormals;
				delete [indexCount] indicies;
				return ok;
			}

			virtual void Draw() override
			{
				if (RestartOnNextFrame)
				{
					RestartOnNextFrame = false;
					StartProcessing();
				}
				if (State != States::Processing) return;

				Device->SetAsRenderTarget();
				Constants->Update(&TurntableOptions);
				Constants->SetForVS(2);
				Constants->SetForGS(2);
				Constants->SetForPS(2);				

				bool drawCross = false;

				switch(Mode)
				{
				case Modes::OneAxis:
				case Modes::TwoAxis:
					{
						if (!SaveTextureReady)
						{
							SaveTextureReady = true;
							if (RawSave)
							{
								for(int i = 0; i < TargetCount; i++)
								{
									SaveModelTargets[i]->CopyToStage();
									SaveTextureTargets[i]->CopyToStage();
								}
							}
							else
							{
								RDefault->Set();
								SLinearClamp->SetForPS();
								BOpaque->Apply();

								Device->SetShaders(VSSimple, PSModelOutput);

								for(int i = 0; i < TargetCount; i++)
								{
									TurntableOptions.Side = i * 2 - 1;
									Constants->Update(&TurntableOptions);
									SaveModelTargets[i]->SetAsRenderTarget();
									ModelTargets[i]->SetForPS();
									QMain->Draw();
									SaveModelTargets[i]->CopyToStage();
								}

								Device->SetShaders(VSSimple, PSTextureOutput);

								for(int i = 0; i < TargetCount; i++)
								{
									SaveTextureTargets[i]->SetAsRenderTarget();
									TextureTargets[i]->SetForPS();
									QMain->Draw();
									SaveTextureTargets[i]->CopyToStage();
								}
							}
							SetEvent(SaveEvent);
						}

						Device->SetAsRenderTarget();
						switch (Params.View)
						{
						case AxialViews::DepthL:
						case AxialViews::DepthR:
							RDefault->Set();
							BAlpha->Apply();
							Device->SetShaders(VSSimple, PSPolarDepth);
							switch (Mode)
							{
							case Modes::OneAxis:
								ModelTargets[0]->SetForPS();
								break;
							case Modes::TwoAxis:
								if(Params.View == AxialViews::DepthL)
									ModelTargets[0]->SetForPS();
								else
									ModelTargets[1]->SetForPS();
								break;
							}					
							SLinearClamp->SetForPS();
							QMain->Draw();
							break;
						case AxialViews::TextureL:
						case AxialViews::TextureR:
							RDefault->Set();
							BAlpha->Apply();
							Device->SetShaders(VSSimple, PSPolarTexture);
							switch (Mode)
							{
							case Modes::OneAxis:
								TextureTargets[0]->SetForPS();
								break;
							case Modes::TwoAxis:
								if(Params.View == AxialViews::TextureL)
									TextureTargets[0]->SetForPS();
								else
									TextureTargets[1]->SetForPS();
								break;
							}
							SLinearClamp->SetForPS();
							QMain->Draw();
							break;
						case AxialViews::Model:
							RCullNone->Set();
							BOpaque->Apply();
							Device->SetShaders(VSModel, PSModel, GSModel);
							SLinearClamp->SetForPS();

							for(int i = 0; i < TargetCount; i++)
							{
								TurntableOptions.Side = i * 2 - 1;
								Constants->Update(&TurntableOptions);
								ModelTargets[i]->SetForVS();
								TextureTargets[i]->SetForPS();
								PMain->Draw();
							}
							break;
						default:
							drawCross = true;
							break;
						}			
					}
					break;
				case Modes::Volumetric:
					{
						if (!SaveTextureReady)
						{
							SaveTextureReady = true;
							SaveCubeTarget->CopyToStage();
							SetEvent(SaveEvent);
						}

						Device->SetAsRenderTarget();
						switch (Params.VolumetricView)
						{
						case VolumetricViews::Projection:
							RDefault->Set();
							BOpaque->Apply();
							Device->SetShaders(VSSimple, PSVolumetricDepth);
							FilterTarget->SetForPS();	
							SPointClamp->SetForPS();
							QMain->Draw();
							break;
						case VolumetricViews::Slice:
							RDefault->Set();
							BOpaque->Apply();
							Device->SetShaders(VSSimple, PSVolumetricSlice);
							CubeTarget->SetForPS();
							SPointClamp->SetForPS();
							QMain->Draw();
							break;
						case VolumetricViews::Model:
							if(!CubeMesh) break;

							RDefault->Set();
							BOpaque->Apply();
							Device->SetShaders(VSVolumetricModel, PSVolumetricModel);
							CubeMesh->Draw();
							break;
						default:
							drawCross = true;
							break;
						}						
					}
					break;
				}
				
				if(drawCross && Calibrated)
				{
					RDefault->Set();
					BOpaque->Apply();
					if (CrossChanged)
					{
						Cross->Load(CrossVertices, CrossVertexCount);
						CrossChanged = false;
					}					
					Cross->Set();
					Device->SetShaders(VSOverlay, PSOverlay);
					Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
					Context->Draw(CrossVertexCount, 0);
				}
			}

			void SetTurntable(float* turntableTransform)
			{
				Params.TurntableTransform = XMFLOAT4X4(turntableTransform);
				Calibrated = !IsIdentity(Params.TurntableTransform);
				SetTurntableOptions();
				DrawIfNeeded();
			}

			void SetAxial(AxialViews view, float height, float radius, float coreX, float coreY, int modelWidth, int modelHeight, int textureWidth, int textureHeight)
			{
				TurntableOptions.CorePosition = XMFLOAT2(coreX, coreY);
				TurntableOptions.ClipLimit = XMFLOAT2(radius, height);
				Params.View = view;
				NextModelWidth = modelWidth;
				NextModelHeight = modelHeight;
				NextTextureWidth = textureWidth;
				NextTextureHeight = textureHeight;
				SetOverlay();
				SetCross();
				if(!Scanning && State == States::Processing && (
					NextModelWidth != ModelWidth ||
					NextModelHeight != ModelHeight ||
					NextTextureWidth != TextureWidth ||
					NextTextureHeight != TextureHeight
					))
				{
					RestartOnNextFrame = true;
				}
				DrawIfNeeded();
			}

			void SetVolumetric(VolumetricViews view, float cubeSize, int cubeRes, float depth, float threshold, float gradientLimit)
			{
				TurntableOptions.CubeSize = XMFLOAT2(cubeSize, sqrt(2.f) * cubeSize);
				TurntableOptions.Slice = depth;
				TurntableOptions.Threshold = threshold;
				TurntableOptions.GradientLimit = gradientLimit;
				Params.VolumetricView = view;
				NextCubeRes = cubeRes;
				SetOverlay();
				SetCross();
				if(!Scanning && NextCubeRes != CubeRes && State == States::Processing)
				{
					RestartOnNextFrame = true;
				}
				DrawIfNeeded();
			}

			virtual void SetCameras(XMFLOAT4X4 depthIntrinsics, XMFLOAT4X4 depthInvIntrinsics, XMFLOAT4X4 worldToColorTransform, XMFLOAT4X4 depthToTexture, XMFLOAT2 textureMove, XMFLOAT2 textureScale) override
			{
				Params.DepthIntrinsics = depthIntrinsics;
				Params.DepthInvIntrinsics = depthInvIntrinsics;
				TurntableOptions.DepthToTextureTransform = depthToTexture;
				TurntableOptions.TextureMove = textureMove;
				TurntableOptions.TextureScale = textureScale;
				SetTurntableOptions();
				DrawIfNeeded();
			}

			virtual void SetView(XMFLOAT4X4 world, float transX, float transY, float transZ, float rotX, float rotY, float rotZ) override
			{ 
				Params.World = world;
				Params.TransX = transX;
				Params.TransY = transY;
				Params.TransZ = transZ;
				Params.RotX = rotX;
				Params.RotY = rotY;
				Params.RotZ = rotZ;			
				SetTurntableOptions();
				DrawIfNeeded();
			}

			void SetTurntablePosition(float angle)
			{
				Params.Rotation = angle;
				SetTurntableOptions();
			}

			void SetMode(Modes mode)
			{
				NextMode = mode;
				if(!Scanning && NextMode != Mode && State == States::Processing)
				{
					RestartOnNextFrame = true;
				}
			}

			Modes GetMode()
			{
				return Mode;
			}			

			void SetTurntableOptions()
			{
				XMMATRIX DepthIntrinsics = XMLoadFloat4x4(&Params.DepthIntrinsics);
				XMMATRIX DepthInvIntrinsics = XMLoadFloat4x4(&Params.DepthInvIntrinsics);
				XMMATRIX World = XMLoadFloat4x4(&Params.World);
				XMMATRIX TurntableTransform = XMLoadFloat4x4(&Params.TurntableTransform);
				XMMATRIX TurntableTransformWithRotation = TurntableTransform * XMMatrixRotationY(Params.Rotation);
				XMMATRIX TurntableToScreenTransform = DepthIntrinsics * World * TurntableTransformWithRotation;
				XMMATRIX WorldToTurntableTransform = XMMatrixInverse(0, TurntableTransformWithRotation);
				XMMATRIX DepthToTurntableTransform = WorldToTurntableTransform * DepthInvIntrinsics;
				XMMATRIX DepthToWorldTransform = XMMatrixInverse(0, TurntableTransform) * DepthInvIntrinsics;
				XMMATRIX T = XMMatrixTranspose(XMMatrixTranslation(Params.TransX, Params.TransY, Params.TransZ));
				XMMATRIX R = 
					XMMatrixRotationZ(XMConvertToRadians(Params.RotZ)) *
					XMMatrixRotationX(XMConvertToRadians(Params.RotX)) *
					XMMatrixRotationY(XMConvertToRadians(Params.RotY));				
				XMStoreFloat4x4(&TurntableOptions.ModelToScreenTransform, DepthIntrinsics * T * R);
				XMStoreFloat4x4(&TurntableOptions.TurntableToScreenTransform, TurntableToScreenTransform);
				XMStoreFloat4x4(&TurntableOptions.DepthToTurntableTransform, DepthToTurntableTransform);
				XMStoreFloat4x4(&TurntableOptions.WorldToTurntableTransform, XMMatrixRotationY(-Params.Rotation));
				XMStoreFloat4x4(&TurntableOptions.DepthToWorldTransform, DepthToWorldTransform);
				float rotY = -XMConvertToRadians(Params.RotY);
				float rotX = XMConvertToRadians(Params.RotX);
				TurntableOptions.CameraPosition = XMFLOAT4(sin(rotY) * cos(rotX), sin(rotX), -cos(rotY) * cos(rotX), 1);
			}
		};
	}
}