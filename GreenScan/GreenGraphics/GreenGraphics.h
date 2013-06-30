#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include "Helper.h"
#include "GreenGraphicsClasses.h"
#include "GreenKinect.h"

#pragma comment (lib, "d3d11.lib")
using namespace std;
using namespace DirectX;
using namespace Green::Kinect;

namespace Green
{
	namespace Graphics
	{
		class DirectXWindow
		{
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

			bool KinectReady, ResizeNeeded;
			KinectDevice::Modes KinectMode;
			Quad *QMain;
			Plane *PMain;
			Sampler *SLinearWrap;
			VertexShader *VSSimple, *VSCommon, *VSReprojection;
			GeometryShader *GSReprojection;
			PixelShader *PSInfrared, *PSColor, *PSSinusoidal;
			Texture2D *TColor, *TDepth;
			
			struct RenderingParameters
			{
				float TransX, TransY, TransZ, RotX, RotY, RotZ;
				XMFLOAT4X4 DepthIntrinsics, DepthInvIntrinsics;
				int Rotation;
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
				XMFLOAT2 DepthStep;
				XMINT2 DepthSize;
				float DepthLimit;
				float TriangleLimit;
			} DepthAndColorOptions;

			ConstantBuffer<DepthAndColorConstants>* CBDepthAndColor;
			ConstantBuffer<CommonConstants>* CBCommon;

			void CreateResources()
			{
				QMain = new Quad(Device);
				PMain = new Plane(Device, 640, 480);

				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(QMain->GetVertexDefinition());
				
				VSCommon = new VertexShader(Device, L"CommonVertexShader.cso");
				VSCommon->SetInputLayout(QMain->GetVertexDefinition());

				VSReprojection = new VertexShader(Device, L"ReprojectionVertexShader.cso");
				VSReprojection->SetInputLayout(PMain->GetVertexDefinition());

				GSReprojection = new GeometryShader(Device, L"ReprojectionGeometryShader.cso");

				PSInfrared = new PixelShader(Device, L"InfraredPixelShader.cso");
				PSColor = new PixelShader(Device, L"ColorPixelShader.cso");
				PSSinusoidal = new PixelShader(Device, L"SinusoidalPixelShader.cso");
				
				SLinearWrap = new Sampler(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_BORDER);

				ZeroMemory(&Params, sizeof(RenderingParameters));				
				CBDepthAndColor = new ConstantBuffer<DepthAndColorConstants>(Device);
				CBCommon = new ConstantBuffer<CommonConstants>(Device);
				
				KinectReady = false;

				TColor = TDepth = 0;
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
				delete PSInfrared;
				delete PSColor;
				delete PSSinusoidal;
				delete SLinearWrap;
				delete CBDepthAndColor;
				delete CBCommon;
			}

			void DrawScene()
			{
				if(!KinectReady) return;
				CBCommon->Update(&CommonOptions);
				CBCommon->SetForVS(0);

				switch (KinectMode)
				{
				case KinectDevice::Depth:
					VSCommon->Apply();
					PSInfrared->Apply();
					TDepth->SetForPS();
					SLinearWrap->SetForPS();				
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
					
					CBDepthAndColor->Update(&DepthAndColorOptions);
					VSReprojection->Apply();
					
					CBDepthAndColor->SetForVS(1);
					TDepth->SetForVS();

					GSReprojection->Apply();
					CBDepthAndColor->SetForGS(1);

					PSSinusoidal->Apply();
								
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

			static void OnKinectStarting(KinectDevice::Modes mode, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				dxw->KinectMode = mode;
				switch (mode)
				{
				case KinectDevice::Depth:
					dxw->TDepth = new Texture2D(dxw->Device,
						KinectDevice::DepthWidth, KinectDevice::DepthHeight,
						DXGI_FORMAT_R16_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				case KinectDevice::Color:
					dxw->TColor = new Texture2D(dxw->Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				case KinectDevice::DepthAndColor:
					dxw->TDepth = new Texture2D(dxw->Device,
						KinectDevice::DepthWidth, KinectDevice::DepthHeight,
						DXGI_FORMAT_R16_SINT, D3D11_USAGE_DYNAMIC);
					dxw->TColor = new Texture2D(dxw->Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				case KinectDevice::Infrared:
					dxw->TColor = new Texture2D(dxw->Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R16_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				default:
					break;
				}

				dxw->SetDepthAndColorOptions();
				dxw->KinectReady = true;
			}

			static void OnColorFrameReady(void* data, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				switch (dxw->KinectMode)
				{
				case KinectDevice::Infrared:
					dxw->TColor->Load<short>((short*)data);
					break;
				case KinectDevice::Color:
					dxw->TColor->Load<int>((int*)data);
				default:
					break;
				}
				dxw->Draw();
			}

			static void OnDepthFrameReady(void* data, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				dxw->TDepth->Load<short>((short*)data);
				dxw->Draw();
			}

			static void OnKinectStopping(void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				dxw->KinectReady = false;
				SafeDelete(dxw->TColor);
				SafeDelete(dxw->TDepth);
				dxw->ClearScreen();
			}
		public:
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
			}

			void SetCameras(float* infraredIntrinsics, float* depthToIRMapping)
			{
				XMMATRIX mInfraredIntrinsics = XMLoadFloat4x4(&XMFLOAT4X4(infraredIntrinsics));
				XMMATRIX mDepthToIRMapping = XMLoadFloat4x4(&XMFLOAT4X4(depthToIRMapping));
				XMMATRIX mDepthIntrinsics = XMMatrixInverse(0, mDepthToIRMapping) * mInfraredIntrinsics;
				XMStoreFloat4x4(&Params.DepthIntrinsics, mDepthIntrinsics);
				Params.DepthInvIntrinsics = Invert(Params.DepthIntrinsics);
				SetDepthAndColorOptions();
			}

			void SetShading(float depthLimit, float triangleLimit)
			{
				DepthAndColorOptions.DepthLimit = depthLimit;
				DepthAndColorOptions.TriangleLimit = triangleLimit;
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
				InitD3D(hWnd);
				CreateResources();
				BackgroundColor[0] = 1.f;
				BackgroundColor[1] = 1.f;
				BackgroundColor[2] = 1.f;
				BackgroundColor[3] = 1.f;
				ResizeNeeded = false;
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
				
				MainViewport->Width = td.Width;
				MainViewport->Height = td.Height;
				DeviceContext->RSSetViewports(1, MainViewport);
				tex->Release();
			}

			void Resize()
			{
				ResizeNeeded = true;
			}

			~DirectXWindow()
			{
				DestroyResources();
				BackBuffer->Release();
				DeviceContext->Release();
				Device->Release();
				SwapChain->Release();
				delete DepthBackBuffer;
				delete MainViewport;
			}
		private:
			IDXGISwapChain *SwapChain;
			ID3D11Device *Device;
			ID3D11DeviceContext *DeviceContext;
			ID3D11RenderTargetView *BackBuffer;
			DepthBuffer *DepthBackBuffer;
			D3D11_VIEWPORT *MainViewport;
			HWND Window;

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

			void Draw()
			{
				if(ResizeNeeded)
				{
					BackBuffer->Release();
					if (DepthBackBuffer != 0) delete DepthBackBuffer;
					Error(SwapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
					QueryBackBuffer();
					ResizeNeeded = false;
					SetAspectRatio();
				}
				static float f = 0;
				//DeviceContext->OMSetRenderTargets(1, &BackBuffer, NULL);
				float bgcolor[4] = { (sin(f)+1)/2, 0.2f, 0.4f, 1.0f };
				f+=0.1;
				DepthBackBuffer->Clear();
				DeviceContext->ClearRenderTargetView(BackBuffer, bgcolor);
				DrawScene();
				SwapChain->Present(0, 0);
			}
		};
	}
}                                                                                                                                                               