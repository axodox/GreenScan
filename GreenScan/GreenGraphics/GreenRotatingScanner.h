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
				TwoAxis
			};

			enum class Views {
				Overlay,
				DepthL,
				DepthR,
				TextureL,
				TextureR,
				Model
			};
			bool Processing;
		private:
			GraphicsDevice* Device;
			RenderTarget **ModelTargets, **TextureTargets;
			RenderTargetGroup *PolarTargetGroup;
			ReadableRenderTarget **SaveModelTargets, **SaveTextureTargets;
			int NextModelWidth, NextModelHeight, NextTextureWidth, NextTextureHeight, TargetCount;
			bool SaveTextureReady, Scanning, ClearNext, StaticInputNext;
			Modes NextMode, Mode;
			struct RenderingParameters
			{
				XMFLOAT4X4 DepthIntrinsics, DepthInvIntrinsics, World, TurntableTransform;
				float Rotation;
				Views View;
				float TransX, TransY, TransZ, RotX, RotY, RotZ;
				int SaveWidth, SaveHeight, SaveTextureWidth, SaveTextureHeight;
			} Params;

			struct TurntableConstants
			{
				XMFLOAT4X4 TurntableToScreenTransform;
				XMFLOAT4X4 DepthToTurntableTransform;
				XMFLOAT4X4 DepthToTextureTransform;
				XMFLOAT4X4 ModelToScreenTransform;
				XMFLOAT2 CorePosition;
				XMFLOAT2 ClipLimit;
				XMFLOAT2 TextureMove;
				XMFLOAT2 TextureScale;
				XMINT2 DepthResolution;
				XMINT2 ColorResolution;
				XMINT2 ModelResolution;
				float Side;
			} TurntableOptions;

			ConstantBuffer<TurntableConstants>* Constants;
			VertexPositionColor* CrossVertices;
			VertexBuffer<VertexPositionColor>* Cross;
			VertexShader *VSOverlay, *VSTwoAxisPolar, *VSOneAxisPolar, *VSCommon, *VSModel, *VSSimple;
			GeometryShader *GSTwoAxisPolar, *GSOneAxisPolar, *GSModel;
			PixelShader *PSOverlay, *PSTwoAxisPolar, *PSOneAxisPolar, *PSPolarDepth, *PSPolarTexture, *PSTest, *PSModel, *PSModelOutput, *PSTextureOutput, *PSSimple;
			BlendState *BOpaque, *BAdditive, *BAlpha;
			RasterizerState *RDefault, *RCullNone;
			ID3D11DeviceContext *Context;
			Plane *PMain;
			Quad *QMain;
			SamplerState *SLinearClamp;
			HANDLE SaveEvent;

			bool CrossChanged, RawSave;
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

			void DrawIfNeeded()
			{
				if(StaticInput)
				{
					RequestDraw(Host);
				}
			}
		public:
			RotatingScannerModule()
			{
				Processing = false;
				Scanning = false;
				ClearNext = false;
				DrawsOverlay = true;
				StaticInputNext = false;
				StaticInput = false;
				Device = nullptr;
			}

			virtual void CreateResources(GraphicsDevice* device) override
			{
				Device = device;
				VSOverlay = new VertexShader(Device, L"TurntableOverlayVertexShader.cso");
				VSOverlay->SetInputLayout(VertexDefinition::VertexPositionColor);
				VSTwoAxisPolar = new VertexShader(Device, L"TurntableTwoAxisPolarVertexShader.cso");
				VSTwoAxisPolar->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSOneAxisPolar = new VertexShader(Device, L"TurntableOneAxisPolarVertexShader.cso");
				VSOneAxisPolar->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSCommon = new VertexShader(Device, L"CommonVertexShader.cso");
				VSCommon->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSModel = new VertexShader(Device, L"TurntableModelVertexShader.cso");
				VSModel->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(VertexDefinition::VertexPositionTexture);

				GSTwoAxisPolar = new GeometryShader(Device, L"TurntableTwoAxisPolarGeometryShader.cso");
				GSOneAxisPolar = new GeometryShader(Device, L"TurntableOneAxisPolarGeometryShader.cso");
				GSModel = new GeometryShader(Device, L"TurntableModelGeometryShader.cso");
				
				PSOverlay = new PixelShader(Device, L"TurntableOverlayPixelShader.cso");
				PSTwoAxisPolar = new PixelShader(Device, L"TurntableTwoAxisPolarPixelShader.cso");
				PSOneAxisPolar = new PixelShader(Device, L"TurntableOneAxisPolarPixelShader.cso");
				PSPolarDepth = new PixelShader(Device, L"TurntablePolarDepthPixelShader.cso");
				PSPolarTexture = new PixelShader(Device, L"TurntablePolarTexturePixelShader.cso");
				PSModel = new PixelShader(Device, L"TurntableModelPixelShader.cso");
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
				delete VSCommon;
				delete VSModel;
				delete VSSimple;
				delete GSTwoAxisPolar;
				delete GSOneAxisPolar;
				delete GSModel;
				delete PSOverlay;
				delete PSTwoAxisPolar;
				delete PSOneAxisPolar;
				delete PSPolarDepth;
				delete PSPolarTexture;
				delete PSModel;
				delete PSTest;
				delete PSSimple;
				delete PSModelOutput;
				delete PSTextureOutput;
				Context->Release();
			}

			virtual void StartProcessing() override
			{
				if (!Device) return;
				if (Processing) EndProcessing();
				Processing = true;
				StaticInput = StaticInputNext;
				StaticInputNext = false;
				TurntableOptions.ModelResolution = XMINT2(NextModelWidth, NextModelHeight);				

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
				TurntableOptions.ColorResolution = XMINT2(KinectDevice::ColorWidth, KinectDevice::ColorHeight);

				BOpaque = new BlendState(Device, BlendState::Opaque);
				BAdditive = new BlendState(Device, BlendState::Additive);
				BAlpha = new BlendState(Device, BlendState::AlphaBlend);
				RDefault = new RasterizerState(Device, RasterizerState::Default);
				RCullNone = new RasterizerState(Device, RasterizerState::CullNone);
				PMain = new Plane(Device, KinectDevice::DepthWidth, KinectDevice::DepthHeight);

				QMain = new Quad(Device);
				SLinearClamp = new SamplerState(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
				SaveTextureReady = true;

				Mode = NextMode;
				switch (Mode)
				{
				case Modes::OneAxis:
					TargetCount = 1;
					break;
				case Modes::TwoAxis:
					TargetCount = 2;
					break;
				}
				ModelTargets = new RenderTarget*[TargetCount];
				TextureTargets = new RenderTarget*[TargetCount];
				RenderTarget** targets = new RenderTarget*[TargetCount * 2];
				for(int i = 0; i < TargetCount; i++)
				{
					ModelTargets[i] = new RenderTarget(Device, NextModelWidth, NextModelHeight, DXGI_FORMAT_R32G32_FLOAT);
					targets[2 * i] = ModelTargets[i];
					TextureTargets[i] = new RenderTarget(Device, NextTextureWidth, NextTextureHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
					targets[2 * i + 1] = TextureTargets[i];
				}
				PolarTargetGroup = new RenderTargetGroup(Device, TargetCount * 2, targets);
				delete [TargetCount * 2] targets;
			}

			virtual void EndProcessing() override
			{
				if (!Processing) return;
				Processing = false;
				for(int i = 0; i < TargetCount; i++)
				{
					delete ModelTargets[i];
					delete TextureTargets[i];
				}
				delete [TargetCount] ModelTargets;
				delete [TargetCount] TextureTargets;
				delete PolarTargetGroup;
				delete Cross;
				delete CrossVertices;
				delete Constants;				
				delete BOpaque;
				delete BAdditive;
				delete BAlpha;
				delete RDefault;
				delete RCullNone;
				delete PMain;
				delete QMain;
				delete SLinearClamp;
			}

			void Scan()
			{
				if(Mode != NextMode) StartProcessing();
				PolarTargetGroup->Clear();
				Scanning = true;
			}

			void Stop()
			{
				Scanning = false;
			}

			virtual void ProcessFrame(RenderTargetPair* depth, Texture2D* color) override
			{
				if (!Processing) return;

				Constants->Update(&TurntableOptions);
				Constants->SetForVS(1);
				Constants->SetForGS(1);
				Constants->SetForPS(1);

				if (ClearNext)
				{
					ClearNext = false;
					PolarTargetGroup->Clear();
				}

				if (Scanning)
				{
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
				}
			}

			virtual void SetSave(int width, int height, int texWidth, int texHeight) override
			{
				Params.SaveWidth = width;
				Params.SaveHeight = height;
				Params.SaveTextureWidth = texWidth;
				Params.SaveTextureHeight = texHeight;
			}

			template <class T> void GetPolarData(ReadableRenderTarget* target, T* data)
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
			bool OpenRaw(LPWSTR path)
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
						StartProcessing();
						int modelWidth, modelHeight, texWidth, texHeight;
						fread(&modelWidth, 4, 1, file);
						fread(&modelHeight, 4, 1, file);
						int modelLen = modelWidth * modelHeight;
						XMFLOAT2* modelData = new XMFLOAT2[modelLen];

						RDefault->Set();
						BOpaque->Apply();
						SLinearClamp->SetForPS();
						Device->SetShaders(VSSimple, PSSimple);
						for(int i = 0; i < TargetCount; i++)
						{
							fread(modelData, sizeof(XMFLOAT2), modelLen, file);
							Texture2D* model = new Texture2D(Device, modelWidth, modelHeight, DXGI_FORMAT_R32G32_FLOAT, modelData, modelWidth * sizeof(XMFLOAT2));
							ModelTargets[i]->SetAsRenderTarget();
							model->SetForPS();
							QMain->Draw();
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
							TextureTargets[i]->SetAsRenderTarget();
							texture->SetForPS();
							QMain->Draw();
							SafeDelete(texture);
						}
						delete [texLen] textureData;
						
						ok = true;
						DrawIfNeeded();
					}
					fclose(file);
				}
				return ok;
			}

			bool SaveRaw(LPWSTR path)
			{
				if (!Processing) return false;
				RawSave = true;
				SaveEvent = CreateEvent(0, 0, 0, 0);

				SaveModelTargets = new ReadableRenderTarget*[TargetCount];
				SaveTextureTargets = new ReadableRenderTarget*[TargetCount];
				for(int i = 0; i < TargetCount; i++)
				{
					SaveModelTargets[i] = new ReadableRenderTarget(ModelTargets[i]);
					SaveTextureTargets[i] = new ReadableRenderTarget(TextureTargets[i]);
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
					
						fwrite(&Params.SaveWidth, 4, 1, file);
						fwrite(&Params.SaveHeight, 4, 1, file);
						int modelLen = Params.SaveWidth * Params.SaveHeight;
						XMFLOAT2* modelData = new XMFLOAT2[modelLen];

						for(int i = 0; i < TargetCount; i++)
						{
							SaveModelTargets[i]->GetData<XMFLOAT2>(modelData);
							fwrite(modelData, sizeof(XMFLOAT2), modelLen, file);
						}
						delete [modelLen] modelData;

						fwrite(&Params.SaveTextureWidth, 4, 1, file);
						fwrite(&Params.SaveTextureHeight, 4, 1, file);
						int texLen = Params.SaveTextureWidth * Params.SaveTextureHeight;
						XMFLOAT4* textureData = new XMFLOAT4[texLen];

						for(int i = 0; i < TargetCount; i++)
						{
							SaveTextureTargets[i]->GetData<XMFLOAT4>(textureData);
							fwrite(textureData, sizeof(XMFLOAT4), texLen, file);
						}

						delete [texLen] textureData;
						fclose(file);
					}
				}

				for(int i = 0; i < TargetCount; i++)
				{
					SafeDelete(SaveModelTargets[i]);
					SafeDelete(SaveTextureTargets[i]);
				}
				delete [TargetCount] SaveModelTargets;
				delete [TargetCount] SaveTextureTargets;

				CloseHandle(SaveEvent);
				return ok;
			}

			bool SaveModel(LPWSTR path, ModelFormats format)
			{
				if(!Processing) return false;
				RawSave = false;
				SaveEvent = CreateEvent(0, 0, 0, 0);

				SaveModelTargets = new ReadableRenderTarget*[TargetCount];
				SaveTextureTargets = new ReadableRenderTarget*[TargetCount];
				for(int i = 0; i < TargetCount; i++)
				{
					SaveModelTargets[i] = new ReadableRenderTarget(Device, Params.SaveWidth, Params.SaveHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
					SaveTextureTargets[i] = new ReadableRenderTarget(Device, Params.SaveTextureWidth, Params.SaveTextureHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
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
				return ok;
			}

			virtual void Draw() override
			{
				if (!Processing) return;
				Device->SetAsRenderTarget();
				Constants->SetForVS(1);
				Constants->SetForGS(1);
				Constants->SetForPS(1);
				Constants->Update(&TurntableOptions);

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
				case Views::DepthL:
				case Views::DepthR:
					RDefault->Set();
					BAlpha->Apply();
					Device->SetShaders(VSCommon, PSPolarDepth);
					switch (Mode)
					{
					case Modes::OneAxis:
						ModelTargets[0]->SetForPS();
						break;
					case Modes::TwoAxis:
						if(Params.View == Views::DepthL)
							ModelTargets[0]->SetForPS();
						else
							ModelTargets[1]->SetForPS();
						break;
					}					
					SLinearClamp->SetForPS();
					QMain->Draw();
					break;
				case Views::TextureL:
				case Views::TextureR:
					RDefault->Set();
					BAlpha->Apply();
					Device->SetShaders(VSCommon, PSPolarTexture);
					switch (Mode)
					{
					case Modes::OneAxis:
						TextureTargets[0]->SetForPS();
						break;
					case Modes::TwoAxis:
						if(Params.View == Views::TextureL)
							TextureTargets[0]->SetForPS();
						else
							TextureTargets[1]->SetForPS();
						break;
					}
					SLinearClamp->SetForPS();
					QMain->Draw();
					break;
				case Views::Model:
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
					RDefault->Set();
					BOpaque->Apply();
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

			void SetShading(Views view)
			{
				Params.View = view;
				DrawsOverlay = view == Views::Overlay;
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
				if(!Scanning && NextMode != Mode)
				{
					StartProcessing();
				}
			}

			void SetTurntableOptions()
			{
				XMMATRIX DepthIntrinsics = XMLoadFloat4x4(&Params.DepthIntrinsics);
				XMMATRIX DepthInvIntrinsics = XMLoadFloat4x4(&Params.DepthInvIntrinsics);
				XMMATRIX World = XMLoadFloat4x4(&Params.World);
				XMMATRIX TurntableTransform = XMLoadFloat4x4(&Params.TurntableTransform) * XMMatrixRotationY(Params.Rotation);
				XMMATRIX TurntableToScreenTransform = DepthIntrinsics * World * TurntableTransform;
				XMMATRIX DepthToTurntableTransform = XMMatrixInverse(0, TurntableTransform) * DepthInvIntrinsics;
				XMMATRIX T = XMMatrixTranspose(XMMatrixTranslation(Params.TransX, Params.TransY, Params.TransZ));
				XMMATRIX R = 
					XMMatrixRotationZ(XMConvertToRadians(Params.RotZ)) *
					XMMatrixRotationX(XMConvertToRadians(Params.RotX)) *
					XMMatrixRotationY(XMConvertToRadians(Params.RotY));
				XMStoreFloat4x4(&TurntableOptions.ModelToScreenTransform, DepthIntrinsics * T * R);
				XMStoreFloat4x4(&TurntableOptions.TurntableToScreenTransform, TurntableToScreenTransform);
				XMStoreFloat4x4(&TurntableOptions.DepthToTurntableTransform, DepthToTurntableTransform);
			}
		};
	}
}