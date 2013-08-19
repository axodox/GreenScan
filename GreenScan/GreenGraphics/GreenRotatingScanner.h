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
				DepthL,
				DepthR,
				TextureL,
				TextureR,
				Model
			};
			bool Processing;
		private:
			GraphicsDevice* Device;
			RenderTarget *ModelTargetL, *ModelTargetR, *TextureTargetL, *TextureTargetR;
			RenderTargetGroup *PolarTargetGroup;
			ReadableRenderTarget *SaveModelTargetL, *SaveModelTargetR, *SaveTextureTargetL, *SaveTextureTargetR;
			int NextModelWidth, NextModelHeight, NextTextureWidth, NextTextureHeight;
			bool SaveTextureReady, Scanning, ClearNext, StaticInputNext;
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
			VertexShader *VSOverlay, *VSPolar, *VSCommon, *VSModel, *VSSimple;
			GeometryShader *GSPolar, *GSModel;
			PixelShader *PSOverlay, *PSPolar, *PSPolarDepth, *PSPolarTexture, *PSTest, *PSModel, *PSModelOutput, *PSTextureOutput, *PSSimple;
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
				VSModel = new VertexShader(Device, L"TurntableModelVertexShader.cso");
				VSModel->SetInputLayout(VertexDefinition::VertexPositionTexture);
				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(VertexDefinition::VertexPositionTexture);

				GSPolar = new GeometryShader(Device, L"TurntablePolarGeometryShader.cso");
				GSModel = new GeometryShader(Device, L"TurntableModelGeometryShader.cso");
				
				PSOverlay = new PixelShader(Device, L"TurntableOverlayPixelShader.cso");
				PSPolar = new PixelShader(Device, L"TurntablePolarPixelShader.cso");
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
				delete VSPolar;
				delete VSCommon;
				delete VSModel;
				delete VSSimple;
				delete GSPolar;
				delete GSModel;
				delete PSOverlay;
				delete PSPolar;
				delete PSPolarDepth;
				delete PSPolarTexture;
				delete PSModel;
				delete PSTest;
				delete PSSimple;
				delete PSModelOutput;
				delete PSTextureOutput;
				Context->Release();
			}

			void PrepareForStaticInput()
			{
				StaticInputNext = true;
				StartProcessing();
			}

			virtual void StartProcessing() override
			{
				if (Processing) return;
				if (StaticInput) EndProcessing();
				Processing = true;
				StaticInput = StaticInputNext;
				StaticInputNext = false;
				TurntableOptions.ModelResolution = XMINT2(NextModelWidth, NextModelHeight);
				ModelTargetL = new RenderTarget(Device, NextModelWidth, NextModelHeight, DXGI_FORMAT_R32G32_FLOAT);
				ModelTargetR = new RenderTarget(Device, NextModelWidth, NextModelHeight, DXGI_FORMAT_R32G32_FLOAT);
				TextureTargetL = new RenderTarget(Device, NextTextureWidth, NextTextureHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
				TextureTargetR = new RenderTarget(Device, NextTextureWidth, NextTextureHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
				RenderTarget* targets[] = {ModelTargetL, ModelTargetR, TextureTargetL, TextureTargetR};

				PolarTargetGroup = new RenderTargetGroup(Device, 4, targets);

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
			}

			virtual void EndProcessing() override
			{
				if (!Processing) return;
				Processing = false;
				delete ModelTargetL;
				delete ModelTargetR;
				delete TextureTargetL;
				delete TextureTargetR;
				delete PolarTargetGroup;
				delete Cross;
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
					Device->SetShaders(VSPolar, PSPolar, GSPolar);
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
					if(version == SaveVersion)
					{
						int modelWidth, modelHeight, texWidth, texHeight;
						fread(&modelWidth, 4, 1, file);
						fread(&modelHeight, 4, 1, file);
						int modelLen = modelWidth * modelHeight;
						XMFLOAT2* modelData = new XMFLOAT2[modelLen];

						fread(modelData, sizeof(XMFLOAT2), modelLen, file);
						Texture2D* modelL = new Texture2D(Device, modelWidth, modelHeight, DXGI_FORMAT_R32G32_FLOAT, modelData, modelWidth * sizeof(XMFLOAT2));
						fread(modelData, sizeof(XMFLOAT2), modelLen, file);
						Texture2D* modelR = new Texture2D(Device, modelWidth, modelHeight, DXGI_FORMAT_R32G32_FLOAT, modelData, modelWidth * sizeof(XMFLOAT2));
						delete [modelLen] modelData;

						fread(&texWidth, 4, 1, file);
						fread(&texHeight, 4, 1, file);
						int texLen = texWidth * texHeight;
						XMFLOAT4* textureData = new XMFLOAT4[texLen];

						fread(textureData, sizeof(XMFLOAT4), texLen, file);
						Texture2D* textureL = new Texture2D(Device, texWidth, texHeight, DXGI_FORMAT_R32G32B32A32_FLOAT, textureData, texWidth * sizeof(XMFLOAT4));
						fread(textureData, sizeof(XMFLOAT4), texLen, file);
						Texture2D* textureR = new Texture2D(Device, texWidth, texHeight, DXGI_FORMAT_R32G32B32A32_FLOAT, textureData, texWidth * sizeof(XMFLOAT4));
						delete [texLen] textureData;

						RDefault->Set();
						BOpaque->Apply();
						SLinearClamp->SetForPS();
						Device->SetShaders(VSSimple, PSSimple);
						
						ModelTargetL->SetAsRenderTarget();
						modelL->SetForPS();
						QMain->Draw();

						ModelTargetR->SetAsRenderTarget();
						modelR->SetForPS();
						QMain->Draw();

						TextureTargetL->SetAsRenderTarget();
						textureL->SetForPS();
						QMain->Draw();

						TextureTargetR->SetAsRenderTarget();
						textureR->SetForPS();
						QMain->Draw();

						SafeDelete(modelL);
						SafeDelete(modelR);
						SafeDelete(textureL);
						SafeDelete(textureR);

						fclose(file);
						ok = true;
						DrawIfNeeded();
					}
				}
				return ok;
			}

			bool SaveRaw(LPWSTR path)
			{
				if (!Processing) return false;
				RawSave = true;
				SaveEvent = CreateEvent(0, 0, 0, 0);
				SaveModelTargetL = new ReadableRenderTarget(ModelTargetL);
				SaveModelTargetR = new ReadableRenderTarget(ModelTargetR);
				SaveTextureTargetL = new ReadableRenderTarget(TextureTargetL);
				SaveTextureTargetR = new ReadableRenderTarget(TextureTargetR);
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
					
						fwrite(&Params.SaveWidth, 4, 1, file);
						fwrite(&Params.SaveHeight, 4, 1, file);
						int modelLen = Params.SaveWidth * Params.SaveHeight;
						XMFLOAT2* modelData = new XMFLOAT2[modelLen];

						SaveModelTargetL->GetData<XMFLOAT2>(modelData);
						fwrite(modelData, sizeof(XMFLOAT2), modelLen, file);
						SaveModelTargetR->GetData<XMFLOAT2>(modelData);
						fwrite(modelData, sizeof(XMFLOAT2), modelLen, file);
						delete [modelLen] modelData;

						fwrite(&Params.SaveTextureWidth, 4, 1, file);
						fwrite(&Params.SaveTextureHeight, 4, 1, file);
						int texLen = Params.SaveTextureWidth * Params.SaveTextureHeight;
						XMFLOAT4* textureData = new XMFLOAT4[texLen];

						SaveTextureTargetL->GetData<XMFLOAT4>(textureData);
						fwrite(textureData, sizeof(XMFLOAT4), texLen, file);
						SaveTextureTargetR->GetData<XMFLOAT4>(textureData);
						fwrite(textureData, sizeof(XMFLOAT4), texLen, file);

						delete [texLen] textureData;
						fclose(file);
					}
				}

				SafeDelete(SaveModelTargetL);
				SafeDelete(SaveModelTargetR);
				SafeDelete(SaveTextureTargetL);
				SafeDelete(SaveTextureTargetR);
				CloseHandle(SaveEvent);
				return ok;
			}

			bool SaveModel(LPWSTR path, ModelFormats format)
			{
				if(!Processing) return false;
				RawSave = false;
				SaveEvent = CreateEvent(0, 0, 0, 0);

				SaveModelTargetL = new ReadableRenderTarget(Device, Params.SaveWidth, Params.SaveHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
				SaveModelTargetR = new ReadableRenderTarget(Device, Params.SaveWidth, Params.SaveHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);

				SaveTextureTargetL = new ReadableRenderTarget(Device, Params.SaveTextureWidth, Params.SaveTextureHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
				SaveTextureTargetR = new ReadableRenderTarget(Device, Params.SaveTextureWidth, Params.SaveTextureHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
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
					WCHAR textureFilenameL[MAX_PATH], textureFilenameR[MAX_PATH];
					WCHAR filenameL[MAX_PATH], filenameR[MAX_PATH];

					wcscpy_s(filenameL, path);
					wcscat_s(filenameL, L"_l");
					wcscpy_s(filenameR, path);
					wcscat_s(filenameR, L"_r");
					
					wcscpy_s(textureFilenameL, path);
					wcscat_s(textureFilenameL, L"_l.png");
					wcscpy_s(textureFilenameR, path);
					wcscat_s(textureFilenameR, L"_r.png");

					int dataLen = (Params.SaveWidth + 1) * Params.SaveHeight;
					XMFLOAT4* dataL = new XMFLOAT4[dataLen];
					XMFLOAT4* dataR = new XMFLOAT4[dataLen];
					GetPolarData(SaveModelTargetL, dataL);
					GetPolarData(SaveModelTargetR, dataR);

					switch (format)
					{
					case ModelFormats::STL:
						ok &= STLSave(filenameL, dataL, Params.SaveWidth + 1, Params.SaveHeight);
						ok &= STLSave(filenameR, dataR, Params.SaveWidth + 1, Params.SaveHeight);
						break;
					case ModelFormats::FBX:
						ok &= FBXSave(filenameL, dataL, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameL, L'\\') + 1, L"fbx");
						ok &= FBXSave(filenameR, dataR, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameR, L'\\') + 1, L"fbx");
						break;
					case ModelFormats::DXF:
						ok &= FBXSave(filenameL, dataL, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameL, L'\\') + 1, L"dxf");
						ok &= FBXSave(filenameR, dataR, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameR, L'\\') + 1, L"dxf");
						break;
					case ModelFormats::DAE:
						ok &= FBXSave(filenameL, dataL, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameL, L'\\') + 1, L"dae");
						ok &= FBXSave(filenameR, dataR, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameR, L'\\') + 1, L"dae");
						break;
					case ModelFormats::OBJ:
						ok &= FBXSave(filenameL, dataL, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameL, L'\\') + 1, L"obj");
						ok &= FBXSave(filenameR, dataR, Params.SaveWidth + 1, Params.SaveHeight, wcsrchr(textureFilenameR, L'\\') + 1, L"obj");
						break;
					case ModelFormats::FL4:
						ok &= FL4Save(filenameL, dataL, Params.SaveWidth + 1, Params.SaveHeight);
						ok &= FL4Save(filenameR, dataR, Params.SaveWidth + 1, Params.SaveHeight);
						break;
					default:
						ok = false;
						break;
					}
					delete [dataLen] dataL;
					delete [dataLen] dataR;

					ok &= PNGSave(textureFilenameL, SaveTextureTargetL->GetStagingTexture());
					ok &= PNGSave(textureFilenameR, SaveTextureTargetR->GetStagingTexture());
				}
				SafeDelete(SaveModelTargetL);
				SafeDelete(SaveModelTargetR);
				SafeDelete(SaveTextureTargetL);
				SafeDelete(SaveTextureTargetR);
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
						SaveModelTargetL->CopyToStage();
						SaveModelTargetR->CopyToStage();
						SaveTextureTargetL->CopyToStage();
						SaveTextureTargetR->CopyToStage();
					}
					else
					{
						RDefault->Set();
						SLinearClamp->SetForPS();
						BOpaque->Apply();
					
						Device->SetShaders(VSSimple, PSModelOutput);

						SaveModelTargetL->SetAsRenderTarget();
						ModelTargetL->SetForPS();
						QMain->Draw();
						SaveModelTargetL->CopyToStage();

						SaveModelTargetR->SetAsRenderTarget();
						ModelTargetR->SetForPS();
						QMain->Draw();
						SaveModelTargetR->CopyToStage();

						Device->SetShaders(VSSimple, PSTextureOutput);

						SaveTextureTargetL->SetAsRenderTarget();
						TextureTargetL->SetForPS();
						QMain->Draw();
						SaveTextureTargetL->CopyToStage();

						SaveTextureTargetR->SetAsRenderTarget();
						TextureTargetR->SetForPS();
						QMain->Draw();
						SaveTextureTargetR->CopyToStage();
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
					if(Params.View == Views::DepthL)
						ModelTargetL->SetForPS();
					else
						ModelTargetR->SetForPS();
					SLinearClamp->SetForPS();
					QMain->Draw();
					break;
				case Views::TextureL:
				case Views::TextureR:
					RDefault->Set();
					BAlpha->Apply();
					Device->SetShaders(VSCommon, PSPolarTexture);
					if(Params.View == Views::TextureL)
						TextureTargetL->SetForPS();
					else
						TextureTargetR->SetForPS();
					SLinearClamp->SetForPS();
					QMain->Draw();
					break;
				case Views::Model:
					RCullNone->Set();
					BOpaque->Apply();
					Device->SetShaders(VSModel, PSModel, GSModel);
					SLinearClamp->SetForPS();

					TurntableOptions.Side = -1.f;
					Constants->Update(&TurntableOptions);
					ModelTargetL->SetForVS();
					TextureTargetL->SetForPS();
					PMain->Draw();

					TurntableOptions.Side = 1.f;
					Constants->Update(&TurntableOptions);
					ModelTargetR->SetForVS();
					TextureTargetR->SetForPS();
					PMain->Draw();
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