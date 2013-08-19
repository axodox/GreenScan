#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN
#define GaussCoeffCount 9
#include "stdafx.h"
#include "Helper.h"
#include "GreenGraphicsClasses.h"
#include "GreenGraphicsModules.h"
#include "GreenKinect.h"
#include "Export.h"
using namespace std;
using namespace DirectX;
using namespace Gdiplus;
using namespace Green::Kinect;

namespace Green
{
	namespace Graphics
	{
		class DirectXWindow
		{
		public:
			enum class ShadingModes {
				Zebra,
				Rainbow,
				ShadedRainbow,
				Scale,
				ShadedScale,
				Blinn,
				Textured
			} ShadingMode;
		private:
			static const int MaxModuleCount = 4;
			int ModuleCount;
			GraphicsModule** Modules;
			LPWSTR Root;
			LPWSTR ToRoot(LPCWSTR path)
			{
				static LPWSTR tempPath = new WCHAR[MAX_PATH];
				wcscpy(tempPath, Root);
				wcscat(tempPath, L"\\");
				wcscat(tempPath, path);
				return tempPath;
			}
			ULONG_PTR GdiPlusToken;

			bool KinectReady, ResizeNeeded, PreprocessingChanged;
			KinectDevice::Modes KinectMode;
			Quad *QMain;
			Plane *PMain;
			SamplerState *SLinearWrap, *SLinearClamp;
			VertexShader *VSSimple, *VSCommon, *VSReprojection;
			GeometryShader *GSReprojection;
			PixelShader *PSSimple, *PSInfrared, *PSDepth, *PSColor, *PSSine, *PSPeriodicScale, *PSPeriodicShadedScale, *PSScale, *PSShadedScale, *PSBlinn, *PSTextured;
			PixelShader *PSDepthSum, *PSDepthAverage, *PSDepthGaussH, *PSDepthGaussV, *PSVectorOutput, *PSTextureOutput, *PSDistortionCorrection;
			Texture2D *TColor, *TDepthCorrection;
			Texture1D *THueMap, *TScaleMap, *TGauss;
			Texture2DDoubleBuffer *TDBDepth;
			RenderTarget *RTDepthSum;
			RenderTargetPair *RTPDepth;
			ReadableRenderTarget *RRTSaveVertices, *RRTSaveTexture;
			BlendState *BAdditive, *BOpaque;
			RasterizerState *RDefault, *RCullNone, *RWireframe;
			int NextDepthBufferSize, DepthBufferSize;
			int NextTriangleGridWidth, NextTriangleGridHeight;
			bool StaticInput;
			static const int DistortionMapWidth = 160;
			static const int DistortionMapHeight = 120;
			char* DepthDistortionMap;
			
			struct RenderingParameters
			{
				XMFLOAT4X4 DepthIntrinsics, DepthInvIntrinsics, World;
				int Rotation, DepthGaussIterations, SaveWidth, SaveHeight, SaveTextureWidth, SaveTextureHeight;
				float GaussCoeffs[GaussCoeffCount];
				bool WireframeShading, UseModuleShading;
				float TransX, TransY, TransZ, RotX, RotY, RotZ;
			} Params;

			struct CommonConstants
			{
				XMFLOAT4X4 SceneRotation;
				XMFLOAT2 AspectScale;
				XMFLOAT2 Move;
				float Scale;			
			} CommonOptions;

			struct DepthAndColorConstants 
			{
				XMFLOAT4X4 DepthInvIntrinsics;
				XMFLOAT4X4 ReprojectionTransform;
				XMFLOAT4X4 WorldTransform;
				XMFLOAT4X4 NormalTransform;
				XMFLOAT4X4 DepthToColorTransform;
				XMFLOAT4X4 WorldToColorTransform;
				XMFLOAT4X4 SaveTransform;
				XMFLOAT2 DepthStep;
				XMFLOAT2 DepthSaveStep;
				XMFLOAT2 ColorMove;
				XMFLOAT2 ColorScale;
				XMINT2 DepthSize;
				XMINT2 ColorSize;
				XMINT2 SaveSize;
				float DepthMaximum;
				float DepthMinimum;
				float ShadingPeriode;
				float ShadingPhase;
				float TriangleLimit;
			} DepthAndColorOptions;

			ConstantBuffer<DepthAndColorConstants>* CBDepthAndColor;
			ConstantBuffer<CommonConstants>* CBCommon;

			void CreateResources()
			{
				QMain = new Quad(Device);
				PMain = nullptr;
				NextTriangleGridWidth = KinectDevice::DepthWidth;
				NextTriangleGridHeight = KinectDevice::DepthHeight;

				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(QMain->GetVertexDefinition());
				
				VSCommon = new VertexShader(Device, L"CommonVertexShader.cso");
				VSCommon->SetInputLayout(QMain->GetVertexDefinition());

				VSReprojection = new VertexShader(Device, L"ReprojectionVertexShader.cso");
				VSReprojection->SetInputLayout(VertexDefinition::VertexPositionTexture);

				GSReprojection = new GeometryShader(Device, L"ReprojectionGeometryShader.cso");

				PSSimple = new PixelShader(Device, L"SimplePixelShader.cso");
				PSInfrared = new PixelShader(Device, L"InfraredPixelShader.cso");
				PSColor = new PixelShader(Device, L"ColorPixelShader.cso");
				PSDepth = new PixelShader(Device, L"DepthPixelShader.cso");
				PSSine = new PixelShader(Device, L"SinePixelShader.cso");
				PSPeriodicScale = new PixelShader(Device, L"PeriodicScalePixelShader.cso");
				PSPeriodicShadedScale = new PixelShader(Device, L"PeriodicShadedScalePixelShader.cso");
				PSScale = new PixelShader(Device, L"ScalePixelShader.cso");
				PSShadedScale = new PixelShader(Device, L"ShadedScalePixelShader.cso");
				PSBlinn = new PixelShader(Device, L"BlinnPixelShader.cso");
				PSTextured = new PixelShader(Device, L"TexturedPixelShader.cso");

				PSDepthSum = new PixelShader(Device, L"DepthSumPixelShader.cso");
				PSDepthAverage = new PixelShader(Device, L"DepthAveragePixelShader.cso");
				PSDepthGaussH = new PixelShader(Device, L"DepthGaussHPixelShader.cso");
				PSDepthGaussV = new PixelShader(Device, L"DepthGaussVPixelShader.cso");
				PSVectorOutput = new PixelShader(Device, L"VectorOutputPixelShader.cso");
				PSTextureOutput = new PixelShader(Device, L"TextureOutputPixelShader.cso");
				PSDistortionCorrection = new PixelShader(Device, L"DistortionCorrectionPixelShader.cso");

				SLinearWrap = new SamplerState(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
				SLinearClamp = new SamplerState(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);

				ZeroMemory(&Params, sizeof(RenderingParameters));				
				CBDepthAndColor = new ConstantBuffer<DepthAndColorConstants>(Device);
				CBCommon = new ConstantBuffer<CommonConstants>(Device);
				
				KinectReady = false;

				TColor = nullptr;
				TDepthCorrection = nullptr;
				THueMap = Texture1D::FromFile(Device, L"hueMap.png");
				TScaleMap = Texture1D::FromFile(Device, L"scaleMap.png");
				TGauss = new Texture1D(Device, GaussCoeffCount, DXGI_FORMAT_R32_FLOAT);
				
				TDBDepth = nullptr;
				RTDepthSum = nullptr;
				RTPDepth = nullptr;
				RRTSaveVertices = nullptr;
				RRTSaveTexture = nullptr;
				NextDepthBufferSize = 1;
				DepthDistortionMap = nullptr;

				BAdditive = new BlendState(Device, BlendState::Additive);
				BOpaque = new BlendState(Device, BlendState::Opaque);

				RDefault = new RasterizerState(Device, RasterizerState::Default);
				RCullNone = new RasterizerState(Device, RasterizerState::CullNone);
				RWireframe = new RasterizerState(Device, RasterizerState::Wireframe);
				
				PreprocessingChanged = false;

				DepthAndColorOptions.DepthSize = XMINT2(KinectDevice::DepthWidth, KinectDevice::DepthHeight);
				DepthAndColorOptions.ColorSize = XMINT2(KinectDevice::ColorWidth, KinectDevice::ColorHeight);
			}

			void DestroyResources()
			{
				KinectReady = false;
				delete QMain;
				delete PMain;
				delete VSSimple;
				delete VSCommon;
				delete VSReprojection;
				delete GSReprojection;
				delete PSSimple;
				delete PSInfrared;
				delete PSColor;
				delete PSDepth;
				delete PSSine;
				delete PSPeriodicScale;
				delete PSPeriodicShadedScale;
				delete PSScale;
				delete PSShadedScale;
				delete PSBlinn;
				delete PSTextured;
				delete PSDepthSum;
				delete PSDepthAverage;
				delete PSDepthGaussH;
				delete PSDepthGaussV;
				delete PSVectorOutput;
				delete PSTextureOutput;
				delete PSDistortionCorrection;
				delete SLinearWrap;
				delete SLinearClamp;
				delete CBDepthAndColor;
				delete CBCommon;
				delete THueMap;
				delete TScaleMap;
				delete TGauss;
				delete BAdditive;
				delete BOpaque;
				delete RDefault;
				delete RCullNone;
				delete RWireframe;

				if(DepthDistortionMap)
				{
					delete [KinectDevice::DepthWidth * KinectDevice::DepthHeight * 2] DepthDistortionMap;
					DepthDistortionMap = nullptr;
				}
			}

			void DrawScene()
			{
				if(!KinectReady) return;
				CBDepthAndColor->Update(&DepthAndColorOptions);

				Device->SetAsRenderTarget();
				BOpaque->Apply();

				bool overlayMode = false;
				bool backgroundDone = false;
				switch (KinectMode)
				{
				case KinectDevice::Depth:
					Device->SetShaders(VSCommon, PSDepth);
					THueMap->SetForPS(1);
					TColor->SetForPS();
					SLinearWrap->SetForPS();	
					CBDepthAndColor->SetForPS(1);
					QMain->Draw();
					break;
				case KinectDevice::Color:
					Device->SetShaders(VSCommon, PSColor);
					TColor->SetForPS();
					SLinearWrap->SetForPS();	
					QMain->Draw();
					break;
				case KinectDevice::DepthAndColor:
					//Depth averaging
					RDefault->Set();
					RTDepthSum->SetAsRenderTarget();
					RTDepthSum->Clear();
					Device->SetShaders(VSSimple, PSDepthSum);
					CBDepthAndColor->SetForPS(1);
					BAdditive->Apply();
					Texture2D *depthTex;
					for(int i = 0; i < DepthBufferSize; i++)
					{
						depthTex = TDBDepth->BeginTextureUse(i);
						if(depthTex == 0) continue;
						depthTex->SetForPS();

						QMain->Draw();
						TDBDepth->EndTextureUse();
					}
					RTPDepth->SetAsRenderTarget();
					Device->SetShaders(VSSimple, PSDepthAverage);
					BOpaque->Apply();
					RTDepthSum->SetForPS();
					QMain->Draw();

					//Distortion Correction
					if(TDepthCorrection)
					{
						RTPDepth->Swap();
						RTPDepth->SetAsRenderTarget();
						Device->SetShaders(VSSimple, PSDistortionCorrection);
						RTPDepth->SetForPS(0);
						TDepthCorrection->SetForPS(1);
						QMain->Draw();
					}

					//Depth Gauss filtering
					if(PreprocessingChanged)
					{
						TGauss->Load<float>(Params.GaussCoeffs);
						PreprocessingChanged = false;
					}
					SLinearWrap->SetForPS();
					TGauss->SetForPS(1);
					for(int i = 0; i < Params.DepthGaussIterations; i++)
					{
						RTPDepth->Swap();
						RTPDepth->SetAsRenderTarget();
						Device->SetShaders(VSSimple, PSDepthGaussH);
						RTPDepth->SetForPS(0);
						QMain->Draw();

						RTPDepth->Swap();
						RTPDepth->SetAsRenderTarget();
						Device->SetShaders(VSSimple, PSDepthGaussV);
						RTPDepth->SetForPS(0);
						QMain->Draw();
					}
					RTPDepth->Swap();

					//Process with modules
					for(int i = 0; i < MaxModuleCount; i++)
					{
						if(Modules[i] != nullptr) Modules[i]->ProcessFrame(RTPDepth, TColor);
					}

					//Rendering
					Device->SetAsRenderTarget();
					if(Params.UseModuleShading && ModuleCount > 0)
					{
						for(int i = 0; i < MaxModuleCount; i++)
						{
							if(Modules[i] != nullptr)
							{
							   if(Modules[i]->DrawsOverlay)
							   {
								   overlayMode = true;
							   }
							   else
							   {
								   Modules[i]->Draw();
								   backgroundDone = true;
							   }
							}
						}
					}

					if(!Params.UseModuleShading || (overlayMode && !backgroundDone))
					{
						BOpaque->Apply();
						CBDepthAndColor->SetForVS(1);
						CBDepthAndColor->SetForGS(1);
						RTPDepth->SetForVS();

						if(Params.WireframeShading)
							RWireframe->Set();
						else
							RCullNone->Set();
						switch (ShadingMode)
						{
						case ShadingModes::Zebra:
							Device->SetShaders(VSReprojection, PSSine, GSReprojection);
							break;
						case ShadingModes::Rainbow:
							Device->SetShaders(VSReprojection, PSPeriodicScale, GSReprojection);
							SLinearWrap->SetForPS();
							THueMap->SetForPS();
							break;
						case ShadingModes::ShadedRainbow:
							Device->SetShaders(VSReprojection, PSPeriodicShadedScale, GSReprojection);
							SLinearWrap->SetForPS();
							THueMap->SetForPS();
							break;
						case ShadingModes::Scale:
							Device->SetShaders(VSReprojection, PSScale, GSReprojection);
							SLinearClamp->SetForPS();
							TScaleMap->SetForPS();
							break;
						case ShadingModes::ShadedScale:
							Device->SetShaders(VSReprojection, PSShadedScale, GSReprojection);
							SLinearClamp->SetForPS();
							TScaleMap->SetForPS();
							break;
						case ShadingModes::Blinn:
							Device->SetShaders(VSReprojection, PSBlinn, GSReprojection);
							break;
						case ShadingModes::Textured:
							Device->SetShaders(VSReprojection, PSTextured, GSReprojection);
							SLinearWrap->SetForPS();
							TColor->SetForPS();
							break;
						default:
							Device->SetShaders(VSReprojection, PSSine, GSReprojection);
							break;
						}
						CBDepthAndColor->SetForPS(1);
						PMain->Draw();

						RCullNone->Set();
					}

					if(Params.UseModuleShading && overlayMode)
					{
						for(int i = 0; i < MaxModuleCount; i++)
						{
							if(Modules[i] != nullptr && Modules[i]->DrawsOverlay)
								Modules[i]->Draw();
						}
					}

					//Save
					if(!SaveTextureReady && (RRTSaveVertices != nullptr || RRTSaveTexture != nullptr))
					{
						SaveTextureReady = true;
						SLinearWrap->SetForPS();
						BOpaque->Apply();

						//Vertices
						if(RRTSaveVertices != nullptr)
						{
							RRTSaveVertices->SetAsRenderTarget();
							Device->SetShaders(VSSimple, PSVectorOutput);
							RTPDepth->SetForPS();
							QMain->Draw();
							RRTSaveVertices->CopyToStage();
						}

						//Texture
						if(RRTSaveTexture != nullptr)
						{
							RRTSaveTexture->SetAsRenderTarget();
							Device->SetShaders(VSSimple, PSTextureOutput);
							RRTSaveVertices->SetForPS(0);
							TColor->SetForPS(1);
							QMain->Draw();
							RRTSaveTexture->CopyToStage();	
						}

						if(!StaticInput) SetEvent(SaveEvent);
					}
					break;
				case KinectDevice::Infrared:
					Device->SetShaders(VSCommon, PSInfrared);
					TColor->SetForPS();
					SLinearWrap->SetForPS();				
					QMain->Draw();
					break;
				default:
					break;
				}
			}

			void SetDepthAndColorOptions()
			{
				XMMATRIX world = XMLoadFloat4x4(&Params.World);
				XMMATRIX depthIntrinsics = XMLoadFloat4x4(&Params.DepthIntrinsics);
				XMMATRIX depthInverseIntrinsics = XMLoadFloat4x4(&Params.DepthInvIntrinsics);				
				XMMATRIX reproj = depthIntrinsics * world * depthInverseIntrinsics;
				XMMATRIX reprojNormals = XMMatrixTranspose(XMMatrixInverse(0, world));
				XMStoreFloat4x4(&DepthAndColorOptions.ReprojectionTransform, reproj);
				XMStoreFloat4x4(&DepthAndColorOptions.WorldTransform, world);
				XMStoreFloat4x4(&DepthAndColorOptions.NormalTransform, reprojNormals);
			}

			void SetAspectRatio()
			{
				XMFLOAT2 aspectScale;
				float depthAspectRatio = (float)KinectDevice::DepthWidth / (float)KinectDevice::DepthHeight;
				float aspectRatio = (Params.Rotation % 2 == 0 ? Device->GetAspectRatio() : 1.f / Device->GetAspectRatio());

				if(depthAspectRatio >= aspectRatio)
				{
					aspectScale.x = 1.f;
					aspectScale.y = aspectRatio / depthAspectRatio;
				}
				else
				{
					aspectScale.x = depthAspectRatio / aspectRatio;
					aspectScale.y = 1.f;
				}
				CommonOptions.AspectScale = aspectScale;
			}

			void GaussCoeffs(int count, float sigma, float* coeffs)
			{
				int pos;
				float coeff;
				float a = 1.f / sqrt(2 * XM_PI * sigma * sigma);
				float b = -0.5f / sigma / sigma;
				int ibase = count / 2;
				for (int i = 0; i < count; i++)
				{
					pos = (i - ibase);
					coeff = a * exp(pos * pos * b);
					coeffs[i] = coeff;
				}
			}

			char* GenerateDistortionMap(int width, int height, int imageWidth, int imageHeight, float k1, float k2, float p1, float p2, 
				float cx, float cy, float fx, float fy)
			{
				char* map = new char[width * height * 2], *pMap = map;
				float x, xd, y, yd, r2, r4, xi, yi, xo, yo;
				for (int j = 0; j < height; j++)
				{
					yi = j * imageHeight / height;
					y = (yi - cy) / fy;
					for (int i = 0; i < width; i++)
					{
						xi = i * imageWidth / width;
						x = (xi - cx) / fx;
						r2 = x * x + y * y;
						r4 = r2 * r2;
						xd = x * (1 + k1 * r2 + k2 * r4) + p2 * (r2 + 2 * x * x) + 2 * p1 * x * y;
						yd = y * (1 + k1 * r2 + k2 * r4) + p1 * (r2 + 2 * y * y) + 2 * p2 * x * y;
						xo = xd * fx + cx;
						yo = yd * fy + cy;
						*pMap++ = ClampToChar((xo - xi) / 32.f);
						*pMap++ = ClampToChar((yo - yi) / 32.f);
					}
				}
				return map;
			}

			void PrepareProcessing(KinectDevice::Modes mode)
			{
				KinectMode = (KinectDevice::Modes)(mode & KinectDevice::NonFlags);
				switch (KinectMode)
				{
				case KinectDevice::Depth:
					TColor = new Texture2D(Device, KinectDevice::DepthWidth, KinectDevice::DepthHeight, DXGI_FORMAT_R16_SINT);
					RDefault->Set();
					break;
				case KinectDevice::Color:
					TColor = new Texture2D(Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R8G8B8A8_UNORM);
					RDefault->Set();
					break;
				case KinectDevice::DepthAndColor:
					TColor = new Texture2D(Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R8G8B8A8_UNORM);
					TDBDepth = new Texture2DDoubleBuffer(Device, KinectDevice::DepthWidth, KinectDevice::DepthHeight, DXGI_FORMAT_R16_SINT, NextDepthBufferSize);
					RTDepthSum = new RenderTarget(Device, KinectDevice::DepthWidth, KinectDevice::DepthHeight, DXGI_FORMAT_R32G32_FLOAT);
					RTPDepth = new RenderTargetPair(Device, KinectDevice::DepthWidth, KinectDevice::DepthHeight, DXGI_FORMAT_R16_FLOAT);
					PMain = new Plane(Device, NextTriangleGridWidth, NextTriangleGridHeight);
					DepthAndColorOptions.DepthStep = XMFLOAT2(1.f / NextTriangleGridWidth, 1.f / NextTriangleGridHeight);
					if(DepthDistortionMap) 
						TDepthCorrection = new Texture2D(Device, DistortionMapWidth, DistortionMapHeight, DXGI_FORMAT_R8G8_SNORM, DepthDistortionMap, DistortionMapWidth * 2);
					break;
				case KinectDevice::Infrared:
					TColor = new Texture2D(Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R16_UNORM);
					RDefault->Set();
					break;
				}
				
				DepthBufferSize = NextDepthBufferSize;
				SetDepthAndColorOptions();
				KinectReady = true;
				StaticInput = mode & KinectDevice::Virtual;

				if(mode == KinectDevice::DepthAndColor)
				{
					for(int i = 0; i < MaxModuleCount; i++)
					{
						if(Modules[i]) Modules[i]->StartProcessing();
					}
				}
			}
			
			static void OnKinectStarting(KinectDevice::Modes mode, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				dxw->PrepareProcessing(mode);				
			}

			static void OnColorFrameReady(void* data, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				switch (dxw->KinectMode)
				{
				case KinectDevice::Infrared:
					dxw->TColor->Load<short>((short*)data);
					dxw->Draw();
					break;
				case KinectDevice::Color:
					dxw->TColor->Load<int>((int*)data);
					dxw->Draw();
					break;
				case KinectDevice::DepthAndColor:
					dxw->TColor->Load<int>((int*)data);
				default:
					break;
				}
			}

			static void OnDepthFrameReady(void* data, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				switch (dxw->KinectMode)
				{
				case KinectDevice::Depth:
					dxw->TColor->Load<short>((short*)data);
					dxw->Draw();
					break;
				case KinectDevice::DepthAndColor:
					dxw->TDBDepth->Load<short>((short*)data);
					dxw->Draw();
					break;
				}
			}

			void ShutdownProcessing()
			{
				KinectReady = false;
				if(KinectMode == KinectDevice::DepthAndColor && !StaticInput)
				{
					for(int i = 0; i < MaxModuleCount; i++)
					{
						if(Modules[i]) Modules[i]->EndProcessing();
					}
				}
				SafeDelete(TColor);
				SafeDelete(TDBDepth);				
				SafeDelete(RTDepthSum);
				SafeDelete(RTPDepth);
				SafeDelete(PMain);
				SafeDelete(TDepthCorrection);
				ClearScreen();
			}

			static void OnKinectStopping(void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				dxw->ShutdownProcessing();
			}

			void DrawIfNeeded()
			{
				if (StaticInput) Draw();
				if (!KinectReady && ModuleCount > 0)
				{
					bool draw = false;
					for(int i = 0; i < MaxModuleCount; i++)
					{
						if(Modules[i]) draw |= Modules[i]->StaticInput;
					}
					if(draw) Draw();
				}

			}
		public:
			void SetPreprocessing(int depthAveraging, int depthGaussIterations, float depthGaussSigma)
			{
				NextDepthBufferSize = depthAveraging;
				GaussCoeffs(GaussCoeffCount, depthGaussSigma, Params.GaussCoeffs);
				Params.DepthGaussIterations = depthGaussIterations;
				PreprocessingChanged = true;
				DrawIfNeeded();
			}

			void SetView(
				float transX, float transY, float transZ, 
				float rotX, float rotY, float rotZ, 
				float scale, float moveX, float moveY, float rotation)
			{
				Params.Rotation = rotation;
				Params.TransX = transX;
				Params.TransY = transY;
				Params.TransZ = transZ;
				Params.RotX = rotX;
				Params.RotY = rotY;
				Params.RotZ = rotZ;

				XMMATRIX T = XMMatrixTranspose(XMMatrixTranslation(0.f, 0.f, -transZ));
				XMMATRIX T2 = XMMatrixTranspose(XMMatrixTranslation(transX, transY, 0.f));
				XMMATRIX Ti = XMMatrixInverse(0, T);
				XMMATRIX R = 
					XMMatrixRotationZ(XMConvertToRadians(rotZ)) *
					XMMatrixRotationX(XMConvertToRadians(rotX)) *
					XMMatrixRotationY(XMConvertToRadians(rotY));
				XMMATRIX world = Ti * R * T2 * T;
				XMStoreFloat4x4(&Params.World, world);

				CommonOptions.Scale = scale;
				CommonOptions.Move = XMFLOAT2(moveX, moveY);
				XMMATRIX R2 = XMMatrixRotationZ(-Params.Rotation * XM_PIDIV2);
				XMStoreFloat4x4(&CommonOptions.SceneRotation, R2);
				SetAspectRatio();
				SetDepthAndColorOptions();

				for(int i = 0; i < MaxModuleCount; i++)
				{
					if(Modules[i] != nullptr)
					{
						Modules[i]->SetView(Params.World, Params.TransX, Params.TransY, Params.TransZ, Params.RotX, Params.RotY, Params.RotZ);
					}
				}

				DrawIfNeeded();
			}

			void SetCameras(
				float* infraredIntrinsics, float* infraredDistortion, float* depthToIRMapping,
				float* colorIntrinsics, float* colorRemapping, float* colorExtrinsics,
				int colorDispX, int colorDispY, float colorScaleX, float colorScaleY)
			{
				if(infraredDistortion)
					DepthDistortionMap = GenerateDistortionMap(
						DistortionMapWidth, DistortionMapHeight, KinectDevice::DepthWidth, KinectDevice::DepthHeight,
						infraredDistortion[0], infraredDistortion[1], infraredDistortion[2], infraredDistortion[3],
						infraredIntrinsics[2], infraredIntrinsics[6], infraredIntrinsics[0], infraredIntrinsics[5]);
				else if(DepthDistortionMap)
				{
					delete [KinectDevice::DepthWidth * KinectDevice::DepthHeight * 2] DepthDistortionMap;
					DepthDistortionMap = nullptr;
				}

				XMMATRIX mInfraredIntrinsics = Load4x4(infraredIntrinsics);
				XMMATRIX mDepthToIRMapping = Load4x4(depthToIRMapping);
				XMMATRIX mDepthIntrinsics = XMMatrixInverse(0, mDepthToIRMapping) * mInfraredIntrinsics;
				XMMATRIX mDepthInvIntrinsics = XMMatrixInverse(0, mDepthIntrinsics);
				
				XMMATRIX mColorIntrinsics = Load4x4(colorIntrinsics);
				XMMATRIX mColorRemapping = Load4x4(colorRemapping);
				XMMATRIX mColorRemappedIntrinsics = XMMatrixInverse(0, mColorRemapping) * mColorIntrinsics;
				
				XMMATRIX mColorExtrinsics = Load4x4(colorExtrinsics);
				XMMATRIX mWorldToColor = mColorRemappedIntrinsics * mColorExtrinsics;
				XMMATRIX mDepthToColor = mWorldToColor * mDepthInvIntrinsics;

				XMStoreFloat4x4(&Params.DepthIntrinsics, mDepthIntrinsics);
				XMStoreFloat4x4(&Params.DepthInvIntrinsics, mDepthInvIntrinsics);
				XMStoreFloat4x4(&DepthAndColorOptions.DepthToColorTransform, mDepthToColor);
				XMStoreFloat4x4(&DepthAndColorOptions.WorldToColorTransform, mWorldToColor);
				
				DepthAndColorOptions.ColorMove = XMFLOAT2((float)colorDispX / KinectDevice::ColorWidth, (float)colorDispY / KinectDevice::ColorHeight);
				DepthAndColorOptions.ColorScale = XMFLOAT2(colorScaleX, colorScaleY);
				DepthAndColorOptions.DepthInvIntrinsics = Params.DepthInvIntrinsics;
				SetDepthAndColorOptions();
				DrawIfNeeded();

				for(int i = 0; i < MaxModuleCount; i++)
				{
					if(Modules[i] != nullptr)
					{
						Modules[i]->SetCameras(Params.DepthIntrinsics, Params.DepthInvIntrinsics, DepthAndColorOptions.WorldToColorTransform, DepthAndColorOptions.DepthToColorTransform, DepthAndColorOptions.ColorMove, DepthAndColorOptions.ColorScale);
					}
				}
			}

			void SetShading(ShadingModes mode, float depthMaximum, float depthMinimum, float shadingPeriode, float shadingPhase, float triangleLimit, bool wireframeShading, bool useModuleShading)
			{
				ShadingMode = mode;
				DepthAndColorOptions.DepthMaximum = depthMaximum;
				DepthAndColorOptions.DepthMinimum = depthMinimum;
				DepthAndColorOptions.ShadingPeriode = shadingPeriode;
				DepthAndColorOptions.ShadingPhase = shadingPhase;
				DepthAndColorOptions.TriangleLimit = triangleLimit;
				Params.WireframeShading = wireframeShading;
				Params.UseModuleShading = useModuleShading;
				DrawIfNeeded();
			}

			void SetPerformance(int triangleGridWidth, int triangleGridHeight)
			{
				NextTriangleGridWidth = triangleGridWidth;
				NextTriangleGridHeight = triangleGridHeight;
			}

			void SetSave(int width, int height, int texWidth, int texHeight)
			{
				Params.SaveWidth = width;
				Params.SaveHeight = height;
				Params.SaveTextureWidth = texWidth;
				Params.SaveTextureHeight = texHeight;
				DepthAndColorOptions.SaveSize = XMINT2(width, height);
				DepthAndColorOptions.DepthSaveStep = XMFLOAT2(1.f / width, 1.f / height);

				for(int i = 0; i < MaxModuleCount; i++)
				{
					if(Modules[i]) Modules[i]->SetSave(width, height, texWidth, texHeight);
				}
			}

			void InitKinect(KinectDevice* device)
			{
				device->SetCallbackObject(this);
				device->SetKinectStartingCallback(&OnKinectStarting);
				device->SetColorFrameReadyCallback(&OnColorFrameReady);
				device->SetDepthFrameReadyCallback(&OnDepthFrameReady);
				device->SetKinectStoppingCallback(&OnKinectStopping);
			}

			float BackgroundColor[4];
			DirectXWindow(HWND hWnd, LPWSTR root)
			{
				Root = root;
				GdiplusStartupInput gsi;
				GdiplusStartup(&GdiPlusToken, &gsi, 0);
				InitD3D(hWnd);
				CreateResources();
				BackgroundColor[0] = 1.f;
				BackgroundColor[1] = 1.f;
				BackgroundColor[2] = 1.f;
				BackgroundColor[3] = 0.f;
				ResizeNeeded = false;
				StaticInput = false;
				ReadableBackBuffer = nullptr;
				SaveTextureReady = true;
				SaveEvent = CreateEvent(0, 0, 0, 0);
				Modules = new GraphicsModule*[MaxModuleCount];
				ModuleCount = 0;
				for(int i = 0; i < MaxModuleCount; i++)
				{
					Modules[i] = nullptr;
				}
			}

			void Resize()
			{
				ResizeNeeded = true;
				DrawIfNeeded();
			}

			~DirectXWindow()
			{
				for(int i = 0; i < MaxModuleCount; i++)
				{
					if(Modules[i]) Modules[i]->DestroyResources();
					SafeDelete(Modules[i]);
				}
				delete [MaxModuleCount] Modules;
				DestroyResources();
				delete Device;
				GdiplusShutdown(GdiPlusToken);
				CloseHandle(SaveEvent);
			}

			

			bool LoadModule(GraphicsModule* module)
			{
				for(int i = 0; i < MaxModuleCount; i++)
				{
					if(!Modules[i])
					{
						module->CreateResources(Device);
						module->SetView(Params.World, Params.TransX, Params.TransY, Params.TransZ, Params.RotX, Params.RotY, Params.RotZ);
						module->SetCameras(Params.DepthIntrinsics, Params.DepthInvIntrinsics, DepthAndColorOptions.WorldToColorTransform, DepthAndColorOptions.DepthToColorTransform, DepthAndColorOptions.ColorMove, DepthAndColorOptions.ColorScale);
						module->SetSave(Params.SaveWidth, Params.SaveHeight, Params.SaveTextureWidth, Params.SaveTextureHeight);
						module->Host = this;
						module->RequestDraw = &DirectXWindow::OnModuleDraw;
						Modules[i] = module;	
						ModuleCount++;
						return true;
					}
				}
				return false;
			}

			bool UnloadModule(GraphicsModule* module)
			{
				if (module)
				for(int i = 0; i < MaxModuleCount; i++)
				{
					if(Modules[i] == module)
					{
						Modules[i] = nullptr;
						module->DestroyResources();
						ModuleCount--;
						return true;
					}
				}
				return false;
			}
		private:
			GraphicsDevice* Device;
			ReadableRenderTarget* ReadableBackBuffer;
			HANDLE SaveEvent;
			bool SaveTextureReady;

			void InitD3D(HWND hWnd)
			{
				Device = new GraphicsDevice(hWnd);
			}

			static void OnModuleDraw(void* param)
			{
				DirectXWindow* dxw = (DirectXWindow*)param;
				if(!dxw->KinectReady)
				{
					dxw->Draw();
				}
			}
		public:
			void ClearScreen()
			{
				Device->SetAsRenderTarget();
				Device->Clear(BackgroundColor);
				Device->Present();
			}

			bool SaveImage(LPWSTR path)
			{
				bool ok = false;
				SaveTextureReady = true;
				ReadableBackBuffer = new ReadableRenderTarget(Device);							
				SaveTextureReady = false;
				ResetEvent(SaveEvent);

				if(StaticInput)
				{
					Draw();
					ok = true;
				}
				else
				{
					ok = WaitForSingleObject(SaveEvent, 1000) == WAIT_OBJECT_0;	
				}
				
				if(ok)
				{
					ok = PNGSave(path, ReadableBackBuffer->GetStagingTexture());
				}
				delete ReadableBackBuffer;
				return ok;
			}

			bool GetVertices(XMFLOAT4* &data, int &width, int &height)
			{
				if(KinectMode != KinectDevice::DepthAndColor) return false;
				XMStoreFloat4x4(&DepthAndColorOptions.SaveTransform, XMMatrixIdentity());
				width = KinectDevice::DepthWidth;
				height = KinectDevice::DepthHeight;
				RRTSaveVertices = new ReadableRenderTarget(Device, width, height, DXGI_FORMAT_R32G32B32A32_FLOAT);
				SaveTextureReady = false;

				bool ok = false;
				if(StaticInput)
				{
					Draw();
					ok = true;
				}
				else
				{
					ok = WaitForSingleObject(SaveEvent, 1000) == WAIT_OBJECT_0;
				}

				if(ok)
				{
					data = new XMFLOAT4[width * height];
					RRTSaveVertices->GetData<XMFLOAT4>(data);
				}

				return ok;
			}

			bool SaveModel(LPWSTR path, ModelFormats format)
			{
				if(KinectMode != KinectDevice::DepthAndColor) return false;
				XMStoreFloat4x4(&DepthAndColorOptions.SaveTransform, XMMatrixScaling(1.f, -1.f, 1.f));
				bool ok = false;
				RRTSaveVertices = new ReadableRenderTarget(Device, Params.SaveWidth, Params.SaveHeight, DXGI_FORMAT_R32G32B32A32_FLOAT);
				RRTSaveTexture = new ReadableRenderTarget(Device, Params.SaveTextureWidth, Params.SaveTextureHeight, DXGI_FORMAT_R8G8B8A8_UNORM);				
				SaveTextureReady = false;
				
				if(StaticInput)
				{
					Draw();
					ok = true;
				}
				else
				{
					ok = WaitForSingleObject(SaveEvent, 1000) == WAIT_OBJECT_0;
				}
				
				if(ok)
				{
					WCHAR textureFilename[MAX_PATH];
					wcscpy_s(textureFilename, path);
					wcscat_s(textureFilename, L".png");

					//Texture
					ok &= PNGSave(textureFilename, RRTSaveTexture->GetStagingTexture());

					//Geometry
					XMFLOAT4* data = new XMFLOAT4[Params.SaveWidth * Params.SaveHeight];
					RRTSaveVertices->GetData<XMFLOAT4>(data);
					
					switch (format)
					{
					case ModelFormats::STL:
						ok = STLSave(path, data, Params.SaveWidth, Params.SaveHeight);
						break;
					case ModelFormats::FBX:
						ok = FBXSave(path, data, Params.SaveWidth, Params.SaveHeight, wcsrchr(textureFilename, L'\\') + 1, L"fbx");
						break;
					case ModelFormats::DXF:
						ok = FBXSave(path, data, Params.SaveWidth, Params.SaveHeight, wcsrchr(textureFilename, L'\\') + 1, L"dxf");
						break;
					case ModelFormats::DAE:
						ok = FBXSave(path, data, Params.SaveWidth, Params.SaveHeight, wcsrchr(textureFilename, L'\\') + 1, L"dae");
						break;
					case ModelFormats::OBJ:
						ok = FBXSave(path, data, Params.SaveWidth, Params.SaveHeight, wcsrchr(textureFilename, L'\\') + 1, L"obj");
						break;
					case ModelFormats::FL4:
						ok = FL4Save(path, data, Params.SaveWidth, Params.SaveHeight);
						break;
					default:
						ok = false;
						break;
					}
					delete [Params.SaveWidth * Params.SaveHeight] data;

					
				}
				SafeDelete(RRTSaveVertices);
				SafeDelete(RRTSaveTexture);
				return ok;
			}

			void Draw()
			{
				if(ResizeNeeded)
				{
					Device->Resize();
					SetAspectRatio();
					ResizeNeeded = false;
				}
				
				CBCommon->Update(&CommonOptions);
				CBCommon->SetForVS(0);

				Device->Clear(BackgroundColor);
				if(KinectReady)
					DrawScene();
				else
				{
					if(ModuleCount > 0)
					{
						for(int i = 0; i < MaxModuleCount; i++)
						{
							if(Modules[i] != nullptr) Modules[i]->Draw();
						}
					}
				}

				Device->Present();
				if(ReadableBackBuffer != nullptr && !SaveTextureReady)
				{
					ReadableBackBuffer->CopyToStage();
					SaveTextureReady = true;
					if(!StaticInput) SetEvent(SaveEvent);
				}
			}
		};
	}
}