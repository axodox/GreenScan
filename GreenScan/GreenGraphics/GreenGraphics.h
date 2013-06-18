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
			Quad *MainQuad;
			Sampler* SLinearWrap;
			VertexShader *VSSimple;
			PixelShader *PSInfrared, *PSColor;
			Texture2D *ColorTexture, *DepthTexture;

			void CreateResources()
			{
				MainQuad = new Quad(Device);
				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(MainQuad->VBuffer);

				PSInfrared = new PixelShader(Device, L"InfraredPixelShader.cso");
				PSColor = new PixelShader(Device, L"ColorPixelShader.cso");
				
				SLinearWrap = new Sampler(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_BORDER);

				KinectReady = false;
			}

			void DestroyResources()
			{
				KinectReady = false;
				delete MainQuad;
				delete VSSimple;
				delete PSInfrared;
				delete PSColor;
				delete SLinearWrap;
			}

			void DrawScene()
			{
				if(!KinectReady) return;
				switch (KinectMode)
				{
				case Green::Kinect::KinectDevice::Depth:
					VSSimple->Apply();
					PSInfrared->Apply();
					DepthTexture->SetForPS(0);
					SLinearWrap->SetForPS(0);				
					MainQuad->Draw();
					break;
				case Green::Kinect::KinectDevice::Color:
					VSSimple->Apply();
					PSColor->Apply();
					ColorTexture->SetForPS(0);
					SLinearWrap->SetForPS(0);				
					MainQuad->Draw();
					break;
				case Green::Kinect::KinectDevice::DepthAndColor:
					break;
				case Green::Kinect::KinectDevice::Infrared:
					VSSimple->Apply();
					PSInfrared->Apply();
					ColorTexture->SetForPS(0);
					SLinearWrap->SetForPS(0);				
					MainQuad->Draw();
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
				case Green::Kinect::KinectDevice::Depth:
					dxw->DepthTexture = new Texture2D(dxw->Device,
						KinectDevice::DepthWidth, KinectDevice::DepthHeight,
						DXGI_FORMAT_R16_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				case Green::Kinect::KinectDevice::Color:
					dxw->ColorTexture = new Texture2D(dxw->Device,
						KinectDevice::ColorWidth, KinectDevice::ColorHeight,
						DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DYNAMIC);
					break;
				case Green::Kinect::KinectDevice::DepthAndColor:
					break;
				case Green::Kinect::KinectDevice::Infrared:
					dxw->ColorTexture = new Texture2D(dxw->Device,
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
					dxw->ColorTexture->Load<byte>(d);
					delete [640*480] d;*/
				dxw->KinectReady = true;
			}

			static void OnColorFrameReady(void* data, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				switch (dxw->KinectMode)
				{
				case KinectDevice::Modes::Infrared:
					dxw->ColorTexture->Load<short>((short*)data);
					break;
				case KinectDevice::Modes::Color:
					dxw->ColorTexture->Load<int>((int*)data);
				default:
					break;
				}
				dxw->Draw();
			}

			static void OnDepthFrameReady(void* data, void* obj)
			{
				DirectXWindow* dxw = (DirectXWindow*)obj;
				dxw->DepthTexture->Load<short>((short*)data);
				dxw->Draw();
			}
		public:
			void InitKinect(KinectDevice* device)
			{
				device->SetCallbackObject(this);
				device->SetKinectStartingCallback(&OnKinectStarting);
				device->SetColorFrameReadyCallback(&OnColorFrameReady);
				device->SetDepthFrameReadyCallback(&OnDepthFrameReady);
			}

			DirectXWindow(HWND hWnd, LPWSTR root)
			{
				Root = root;
				InitD3D(hWnd);
				CreateResources();
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