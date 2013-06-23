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

			bool KinectReady;
			KinectDevice::Modes KinectMode;
			Quad *QMain;
			Plane *PMain;
			Sampler *SLinearWrap;
			VertexShader *VSSimple;
			PixelShader *PSInfrared, *PSColor;
			Texture2D *TColor, *TDepth;
			struct DepthAndColorData
			{
				float TransX, TransY, TransZ, RotX, RotY, RotZ;
			} DepthAndColorConstants;
			ConstantBuffer<DepthAndColorData>* CBDepthAndColor;

			void CreateResources()
			{
				QMain = new Quad(Device);
				PMain = new Plane(Device, 640, 480);

				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(QMain->GetVertexDefinition());

				PSInfrared = new PixelShader(Device, L"InfraredPixelShader.cso");
				PSColor = new PixelShader(Device, L"ColorPixelShader.cso");
				
				SLinearWrap = new Sampler(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_BORDER);

				ZeroMemory(&DepthAndColorConstants, sizeof(DepthAndColorConstants));
				CBDepthAndColor = new ConstantBuffer<DepthAndColorData>(Device);
				CBDepthAndColor->Update(&DepthAndColorConstants);
				

				KinectReady = false;

				TColor = TDepth = 0;
			}

			void DestroyResources()
			{
				KinectReady = false;
				delete QMain;
				delete PMain;
				delete VSSimple;
				delete PSInfrared;
				delete PSColor;
				delete SLinearWrap;
				delete CBDepthAndColor;
			}

			void DrawScene()
			{
				if(!KinectReady) return;
				switch (KinectMode)
				{
				case KinectDevice::Depth:
					VSSimple->Apply();
					PSInfrared->Apply();
					TDepth->SetForPS(0);
					SLinearWrap->SetForPS(0);				
					QMain->Draw();
					break;
				case KinectDevice::Color:
					VSSimple->Apply();
					PSColor->Apply();
					TColor->SetForPS(0);
					SLinearWrap->SetForPS(0);				
					QMain->Draw();
					break;
				case KinectDevice::DepthAndColor:
					break;
				case KinectDevice::Infrared:
					VSSimple->Apply();
					PSInfrared->Apply();
					TColor->SetForPS(0);
					SLinearWrap->SetForPS(0);				
					QMain->Draw();
					break;
				default:
					break;
				}
				
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
						DXGI_FORMAT_R16_UNORM, D3D11_USAGE_DYNAMIC);
					dxw->TColor = new Texture2D(dxw->Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R16_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				case KinectDevice::Infrared:
					dxw->TColor = new Texture2D(dxw->Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R16_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				default:
					break;
				}

				/*byte* d = new byte[640*480];
					byte* pd = d;
					for(int i=0;i <640*480;i++)
					{
						*pd = i%100;
						
							pd++;
					}
					dxw->TColor->Load<byte>(d);
					delete [640*480] d;*/
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
			void SetView(float transX, float transY, float transZ, float rotX, float rotY, float rotZ)
			{
				DepthAndColorConstants.TransX = transX;
				DepthAndColorConstants.TransY = transY;
				DepthAndColorConstants.TransZ = transZ;
				DepthAndColorConstants.RotX = rotX;
				DepthAndColorConstants.RotY = rotY;
				DepthAndColorConstants.RotZ = rotZ;
				CBDepthAndColor->Update(&DepthAndColorConstants);
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
			}

			~DirectXWindow()
			{
				DestroyResources();
				BackBuffer->Release();
				DeviceContext->Release();
				Device->Release();
				SwapChain->Release();
				delete MainViewport;
			}
		private:
			IDXGISwapChain *SwapChain;
			ID3D11Device *Device;
			ID3D11DeviceContext *DeviceContext;
			ID3D11RenderTargetView *BackBuffer;
			D3D11_VIEWPORT *MainViewport;

			void InitD3D(HWND hWnd)
			{
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

				LPRECT clientRect;
				GetClientRect(hWnd, clientRect);
				
				ID3D11Texture2D *tex;
				Error(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&tex));
				Error(Device->CreateRenderTargetView(tex, NULL, &BackBuffer));
				tex->Release();
				MainViewport = new D3D11_VIEWPORT();
				MainViewport->TopLeftX = 0;
				MainViewport->TopLeftY = 0;
				MainViewport->Width = clientRect->right - clientRect->left;
				MainViewport->Height = clientRect->bottom - clientRect->top;
				DeviceContext->RSSetViewports(1, MainViewport);				
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
				static float f = 0;
				DeviceContext->OMSetRenderTargets(1, &BackBuffer, NULL);
				float bgcolor[4] = { (sin(f)+1)/2, 0.2f, 0.4f, 1.0f };
				f+=0.1;
				DeviceContext->ClearRenderTargetView(BackBuffer, bgcolor);
				DrawScene();
				SwapChain->Present(0, 0);
			}
		};
	}
}                                                                                                                                                               