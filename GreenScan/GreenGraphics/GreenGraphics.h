#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN
#define GaussCoeffCount 9
#include "stdafx.h"
#include "Helper.h"
#include "GreenGraphicsClasses.h"
#include "GreenKinect.h"
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
			void UseMainRenderTarget()
			{
				DepthBackBuffer->Set();
				//DeviceContext->OMSetRenderTargets(1, &BackBuffer, NULL);
				DeviceContext->RSSetViewports(1, MainViewport);
			}

			bool KinectReady, ResizeNeeded, PreprocessingChanged;
			KinectDevice::Modes KinectMode;
			Quad *QMain;
			Plane *PMain;
			SamplerState *SLinearWrap;
			VertexShader *VSSimple, *VSCommon, *VSReprojection;
			GeometryShader *GSReprojection;
			PixelShader *PSSimple, *PSInfrared, *PSDepth, *PSColor, *PSSine, *PSPeriodicScale, *PSPeriodicShadedScale, *PSScale, *PSShadedScale, *PSBlinn, *PSTextured;
			PixelShader *PSDepthSum, *PSDepthAverage, *PSDepthGaussH, *PSDepthGaussV;
			Texture2D *TColor;
			Texture1D *THueMap, *TScaleMap, *TGauss;
			Texture2DDoubleBuffer *TDBDepth;
			RenderTarget *RTDepthSum;
			RenderTargetPair *RTPDepth;
			ReadableRenderTarget *RRTSave;
			Blend *BAdditive, *BOpaque;
			Rasterizer *RDefault, *RCullNone;
			int NextDepthBufferSize, DepthBufferSize;
			int NextTriangleGridWidth, NextTriangleGridHeight;
			bool StaticInput;
			
			struct RenderingParameters
			{
				float TransX, TransY, TransZ, RotX, RotY, RotZ;
				XMFLOAT4X4 DepthIntrinsics, DepthInvIntrinsics;
				int Rotation, DepthGaussIterations;
				float GaussCoeffs[GaussCoeffCount];
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
				XMFLOAT4X4 ModelTransform;
				XMFLOAT4X4 WorldTransform;
				XMFLOAT4X4 NormalTransform;
				XMFLOAT4X4 DepthToColorTransform;
				XMFLOAT2 DepthStep;
				XMFLOAT2 ColorMove;
				XMFLOAT2 ColorScale;
				XMINT2 DepthSize;
				float DepthLimit;
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

				SLinearWrap = new SamplerState(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);

				ZeroMemory(&Params, sizeof(RenderingParameters));				
				CBDepthAndColor = new ConstantBuffer<DepthAndColorConstants>(Device);
				CBCommon = new ConstantBuffer<CommonConstants>(Device);
				
				KinectReady = false;

				TColor = 0;
				THueMap = Texture1D::FromFile(Device, L"hueMap.png");
				TScaleMap = Texture1D::FromFile(Device, L"scaleMap.png");
				TGauss = new Texture1D(Device, GaussCoeffCount, DXGI_FORMAT_R32_FLOAT);
				
				TDBDepth = nullptr;
				RTDepthSum = nullptr;
				RTPDepth = nullptr;
				NextDepthBufferSize = 1;

				BAdditive = new Blend(Device, Blend::Additive);
				BOpaque = new Blend(Device, Blend::Opaque);

				RDefault = new Rasterizer(Device, Rasterizer::Default);
				RCullNone = new Rasterizer(Device, Rasterizer::CullNone);

				PreprocessingChanged = false;
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
				delete SLinearWrap;
				delete CBDepthAndColor;
				delete CBCommon;
				delete THueMap;
				delete TScaleMap;
				delete TGauss;
				delete BAdditive;
				delete BOpaque;
				delete RDefault;
				delete RCullNone;
			}

			void DrawScene()
			{
				if(!KinectReady) return;
				CBCommon->Update(&CommonOptions);
				CBCommon->SetForVS(0);
				CBDepthAndColor->Update(&DepthAndColorOptions);

				UseMainRenderTarget();
				BOpaque->Apply();

				switch (KinectMode)
				{
				case KinectDevice::Depth:
					VSCommon->Apply();
					PSDepth->Apply();
					THueMap->SetForPS(1);
					TColor->SetForPS();
					SLinearWrap->SetForPS();	
					CBDepthAndColor->SetForPS(1);
					QMain->Draw();
					break;
				case KinectDevice::Color:
					VSCommon->Apply();
					PSColor->Apply();
					TColor->SetForPS();
					SLinearWrap->SetForPS();	
					QMain->Draw();
					break;
				case KinectDevice::DepthAndColor:
					//Depth averaging
					RTDepthSum->SetAsRenderTarget();
					RTDepthSum->Clear();
					VSSimple->Apply();
					PSDepthSum->Apply();
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
					PSDepthAverage->Apply();
					BOpaque->Apply();
					RTDepthSum->SetForPS();
					QMain->Draw();

					
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
						PSDepthGaussH->Apply();
						RTPDepth->SetForPS(0);
						QMain->Draw();

						RTPDepth->Swap();
						RTPDepth->SetAsRenderTarget();
						PSDepthGaussV->Apply();
						RTPDepth->SetForPS(0);
						QMain->Draw();
					}

					//Reprojection
					RTPDepth->Swap();
					UseMainRenderTarget();
					
					VSReprojection->Apply();
					CBDepthAndColor->SetForVS(1);
					RTPDepth->SetForVS();
					
					GSReprojection->Apply();
					CBDepthAndColor->SetForGS(1);

					switch (ShadingMode)
					{
					case ShadingModes::Zebra:
						PSSine->Apply();
						break;
					case ShadingModes::Rainbow:
						PSPeriodicScale->Apply();
						SLinearWrap->SetForPS();
						THueMap->SetForPS();
						break;
					case ShadingModes::ShadedRainbow:
						PSPeriodicShadedScale->Apply();
						SLinearWrap->SetForPS();
						THueMap->SetForPS();
						break;
					case ShadingModes::Scale:
						PSScale->Apply();
						SLinearWrap->SetForPS();
						TScaleMap->SetForPS();
						break;
					case ShadingModes::ShadedScale:
						PSShadedScale->Apply();
						SLinearWrap->SetForPS();
						TScaleMap->SetForPS();
						break;
					case ShadingModes::Blinn:
						PSBlinn->Apply();
						break;
					case ShadingModes::Textured:
						PSTextured->Apply();
						SLinearWrap->SetForPS();
						TColor->SetForPS();
						break;
					default:
						PSSine->Apply();
						break;
					}
					CBDepthAndColor->SetForPS(1);
								
					PMain->Draw();
					break;
				case KinectDevice::Infrared:
					VSCommon->Apply();
					PSInfrared->Apply();
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
				float halfDepthWidth = KinectDevice::DepthWidth / 2.f;
				float halfDepthHeight = KinectDevice::DepthHeight / 2.f;
				XMMATRIX DepthIntrinsics = XMLoadFloat4x4(&Params.DepthIntrinsics);
				XMMATRIX DepthInverseIntrinsics = XMLoadFloat4x4(&Params.DepthInvIntrinsics);

				XMMATRIX T = XMMatrixTranspose(XMMatrixTranslation(0.f, 0.f, -Params.TransZ));
				XMMATRIX T2 = XMMatrixTranspose(XMMatrixTranslation(Params.TransX, Params.TransY, 0.f));
				XMMATRIX Ti = XMMatrixInverse(0, T);
				XMMATRIX R = 
					XMMatrixRotationZ(XMConvertToRadians(Params.RotZ)) *
					XMMatrixRotationX(XMConvertToRadians(Params.RotX)) *
					XMMatrixRotationY(XMConvertToRadians(Params.RotY));
				XMMATRIX world = Ti * R * T * T2;
				XMMATRIX reproj = DepthIntrinsics * world * DepthInverseIntrinsics;
				XMMATRIX reprojModel = DepthIntrinsics * Ti * R *T;
				XMMATRIX reprojNormals = XMMatrixTranspose(XMMatrixInverse(0, world));
				XMStoreFloat4x4(&DepthAndColorOptions.ReprojectionTransform, reproj);
				XMStoreFloat4x4(&DepthAndColorOptions.ModelTransform, reprojModel);
				XMStoreFloat4x4(&DepthAndColorOptions.WorldTransform, world);
				XMStoreFloat4x4(&DepthAndColorOptions.NormalTransform, reprojNormals);

				SetAspectRatio();
				DepthAndColorOptions.DepthSize = XMINT2(KinectDevice::DepthWidth, KinectDevice::DepthHeight);
				DepthAndColorOptions.DepthInvIntrinsics = Params.DepthInvIntrinsics;
				DepthAndColorOptions.DepthStep = XMFLOAT2(1.f / KinectDevice::DepthWidth, 1.f / KinectDevice::DepthHeight);
			}

			void SetAspectRatio()
			{
				XMFLOAT2 aspectScale;
				float depthAspectRatio = (float)KinectDevice::DepthWidth / (float)KinectDevice::DepthHeight;
				float aspectRatio = (Params.Rotation % 2 == 0 ? MainViewport->Width / MainViewport->Height : MainViewport->Height / MainViewport->Width);

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
					RCullNone->Set();
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
				SafeDelete(TColor);
				SafeDelete(TDBDepth);				
				SafeDelete(RTDepthSum);
				SafeDelete(RTPDepth);
				SafeDelete(PMain);
				ClearScreen();
			}

			static void OnKinectStopping(void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				dxw->ShutdownProcessing();
			}
		public:
			void SetPreprocessing(int depthAveraging, int depthGaussIterations, float depthGaussSigma)
			{
				NextDepthBufferSize = depthAveraging;
				GaussCoeffs(GaussCoeffCount, depthGaussSigma, Params.GaussCoeffs);
				Params.DepthGaussIterations = depthGaussIterations;
				PreprocessingChanged = true;
				if(StaticInput) Draw();
			}

			void SetView(
				float transX, float transY, float transZ, 
				float rotX, float rotY, float rotZ, 
				float scale, float moveX, float moveY, float rotation)
			{
				Params.TransX = transX;
				Params.TransY = transY;
				Params.TransZ = transZ;
				Params.RotX = rotX;
				Params.RotY = rotY;
				Params.RotZ = rotZ;
				Params.Rotation = rotation;
				SetDepthAndColorOptions();

				CommonOptions.Scale = scale;
				CommonOptions.Move = XMFLOAT2(moveX, moveY);
				XMMATRIX R = XMMatrixRotationZ(-Params.Rotation * XM_PIDIV2);
				XMStoreFloat4x4(&CommonOptions.SceneRotation, R);
				if(StaticInput) Draw();
			}

			void SetCameras(
				float* infraredIntrinsics, float* depthToIRMapping,
				float* colorIntrinsics, float* colorRemapping, float* colorExtrinsics,
				int colorDispX, int colorDispY, float colorScaleX, float colorScaleY)
			{
				XMMATRIX mInfraredIntrinsics = Load4x4(infraredIntrinsics);
				XMMATRIX mDepthToIRMapping = Load4x4(depthToIRMapping);
				XMMATRIX mDepthIntrinsics = XMMatrixInverse(0, mDepthToIRMapping) * mInfraredIntrinsics;
				XMMATRIX mDepthInvIntrinsics = XMMatrixInverse(0, mDepthIntrinsics);
				
				XMMATRIX mColorIntrinsics = Load4x4(colorIntrinsics);
				XMMATRIX mColorRemapping = Load4x4(colorRemapping);
				XMMATRIX mColorRemappedIntrinsics = XMMatrixInverse(0, mColorRemapping) * mColorIntrinsics;

				XMMATRIX mColorExtrinsics = Load4x4(colorExtrinsics);
				XMMATRIX mDepthToColor = mColorIntrinsics * mColorExtrinsics * mDepthInvIntrinsics;

				XMStoreFloat4x4(&Params.DepthIntrinsics, mDepthIntrinsics);
				XMStoreFloat4x4(&DepthAndColorOptions.DepthToColorTransform, mDepthToColor);
				Params.DepthInvIntrinsics = Invert(Params.DepthIntrinsics);
				DepthAndColorOptions.ColorMove = XMFLOAT2((float)colorDispX / KinectDevice::ColorWidth, (float)colorDispY / KinectDevice::ColorHeight);
				DepthAndColorOptions.ColorScale = XMFLOAT2(colorScaleX, colorScaleY);
				SetDepthAndColorOptions();
				if(StaticInput) Draw();
			}

			void SetShading(ShadingModes mode, float depthLimit, float shadingPeriode, float shadingPhase, float triangleLimit)
			{
				ShadingMode = mode;
				DepthAndColorOptions.DepthLimit = depthLimit;
				DepthAndColorOptions.ShadingPeriode = shadingPeriode;
				DepthAndColorOptions.ShadingPhase = shadingPhase;
				DepthAndColorOptions.TriangleLimit = triangleLimit;
				if(StaticInput) Draw();
			}

			void SetPerformance(int triangleGridWidth, int triangleGridHeight)
			{
				NextTriangleGridWidth = triangleGridWidth;
				NextTriangleGridHeight = triangleGridHeight;
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
				BackgroundColor[3] = 1.f;
				ResizeNeeded = false;
				StaticInput = false;
				SaveTexture = nullptr;
				SaveTextureReady = true;
				SaveEvent = CreateEvent(0, 0, 0, 0);
			}

			void QueryBackBuffer()
			{
				ID3D11Texture2D *tex;
				Error(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&tex));
				Error(Device->CreateRenderTargetView(tex, NULL, &BackBuffer));
				D3D11_TEXTURE2D_DESC td;
				tex->GetDesc(&td);
				
				DepthBackBuffer = new DepthBuffer(BackBuffer);
				DepthBackBuffer->Set();
				
				BackBufferWidth = td.Width;
				BackBufferHeight = td.Height;

				MainViewport->Width = td.Width;
				MainViewport->Height = td.Height;
				DeviceContext->RSSetViewports(1, MainViewport);
				//tex->Release();
				BackBufferTexture = tex;
			}

			void Resize()
			{
				ResizeNeeded = true;
				if(StaticInput) Draw();
			}

			~DirectXWindow()
			{
				DestroyResources();
				BackBufferTexture->Release();
				BackBuffer->Release();
				DeviceContext->Release();
				Device->Release();
				SwapChain->Release();
				delete DepthBackBuffer;
				delete MainViewport;
				GdiplusShutdown(GdiPlusToken);
				CloseHandle(SaveEvent);
			}
		private:
			IDXGISwapChain *SwapChain;
			ID3D11Device *Device;
			ID3D11DeviceContext *DeviceContext;
			ID3D11RenderTargetView *BackBuffer;
			ID3D11Texture2D *SaveTexture, *BackBufferTexture;
			DepthBuffer *DepthBackBuffer;
			D3D11_VIEWPORT *MainViewport;
			HWND Window;
			HANDLE SaveEvent;
			bool SaveTextureReady;
			unsigned int BackBufferWidth, BackBufferHeight;

			void InitD3D(HWND hWnd)
			{
				Window = hWnd;
				DXGI_SWAP_CHAIN_DESC scd;
				ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
				scd.BufferCount = 1;
				scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				scd.OutputWindow = hWnd;
				scd.SampleDesc.Count = 1;
				scd.Windowed = TRUE;

				Error(D3D11CreateDeviceAndSwapChain(
					NULL,
					D3D_DRIVER_TYPE_HARDWARE,
					NULL,
					NULL,
					NULL,
					NULL,
					D3D11_SDK_VERSION,
					&scd,
					&SwapChain,
					&Device,
					NULL,
					&DeviceContext));

				DepthBackBuffer = 0;

				LPRECT clientRect;
				GetClientRect(hWnd, clientRect);
				MainViewport = new D3D11_VIEWPORT();
				MainViewport->MaxDepth = D3D11_MAX_DEPTH;
				MainViewport->MinDepth = D3D11_MIN_DEPTH;
				QueryBackBuffer();
			}
		public:
			void ClearScreen()
			{
				DeviceContext->OMSetRenderTargets(1, &BackBuffer, NULL);
				DeviceContext->ClearRenderTargetView(BackBuffer, BackgroundColor);
				SwapChain->Present(0, 0);
			}

			bool SaveImage(LPWSTR path)
			{
				bool ok = false;
				SaveTextureReady = true;
				D3D11_TEXTURE2D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = BackBufferWidth;
				desc.Height = BackBufferHeight;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.BindFlags = 0;				
				desc.MiscFlags = 0;
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				Error(Device->CreateTexture2D(&desc, 0, &SaveTexture));				
				
				if(StaticInput)
				{
					SaveTextureReady = false;
					Draw();
					ok = true;
				}
				else
				{
					SaveTextureReady = false;
					ok = WaitForSingleObject(SaveEvent, 1000) == WAIT_OBJECT_0;	
				}
				
				if(ok)
				{
					Bitmap bitmap(BackBufferWidth, BackBufferHeight, PixelFormat24bppRGB);
					Gdiplus::Rect lockRect(0, 0, BackBufferWidth, BackBufferHeight);
					BitmapData bitmapData;
					bitmap.LockBits(&lockRect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat24bppRGB, &bitmapData);
					
					byte *target, *source;
					D3D11_MAPPED_SUBRESOURCE ms;
					Error(DeviceContext->Map(SaveTexture, 0, D3D11_MAP_READ, 0, &ms));
					for(int row = 0; row < BackBufferHeight; row++)
					{
						source = (byte*)ms.pData + ms.RowPitch * row + 2;
						target = (byte*)bitmapData.Scan0 + bitmapData.Stride * row;
						for(int col = 0; col < BackBufferWidth; col++)
						{
							*target++ = *source--;
							*target++ = *source--;
							*target++ = *source;
							source += 6;
						}
					}
					DeviceContext->Unmap(SaveTexture, 0);
					SaveTexture->Release();
					SaveTexture = nullptr;

					CLSID pngClsid;
					GetEncoderClsid(L"image/png", &pngClsid);
					bitmap.UnlockBits(&bitmapData);
					ok = bitmap.Save(path, &pngClsid, NULL) == Ok;
				}
				else
					SafeRelease(SaveTexture);

				return ok;
			}

			void Draw()
			{
				if(ResizeNeeded)
				{
					BackBufferTexture->Release();
					BackBuffer->Release();
					SafeDelete(DepthBackBuffer);
					Error(SwapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
					QueryBackBuffer();
					ResizeNeeded = false;
					SetAspectRatio();
				}
				DepthBackBuffer->Clear();
				DeviceContext->ClearRenderTargetView(BackBuffer, BackgroundColor);
				DrawScene();
				SwapChain->Present(0, 0);
				if(SaveTexture != nullptr && !SaveTextureReady)
				{
					DeviceContext->CopyResource(SaveTexture, BackBufferTexture);
					SaveTextureReady = true;
					SetEvent(SaveEvent);
				}
			}
		};
	}
}                                                                                                                                                               