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
			VertexShader *VSSimple, *VSReprojection;
			GeometryShader *GSReprojection;
			PixelShader *PSInfrared, *PSColor, *PSSinusoidal;
			Texture2D *TColor, *TDepth;
			struct RenderingParameters
			{
				float TransX, TransY, TransZ, RotX, RotY, RotZ;
				XMFLOAT4X4 DepthIntrinsics, DepthInvIntrinsics;
				int Rotation;
			} Params;

			struct DepthAndColorConstants 
			{
				XMFLOAT4X4 ReprojectionTransform;
				XMFLOAT4X4 ModelTransform;
				XMFLOAT4X4 WorldTransform;
				XMFLOAT4X4 NormalTransform;
				XMFLOAT2 ModelScale;
				XMINT2 DepthSize;
				float DepthLimit;
				float Scale;
				XMFLOAT2 Move;
				XMFLOAT4X4 SceneRotation;
			} DepthAndColorOptions;

			ConstantBuffer<DepthAndColorConstants>* CBDepthAndColor;

			void CreateResources()
			{
				QMain = new Quad(Device);
				PMain = new Plane(Device, 640, 480);

				VSSimple = new VertexShader(Device, L"SimpleVertexShader.cso");
				VSSimple->SetInputLayout(QMain->GetVertexDefinition());
				
				VSReprojection = new VertexShader(Device, L"ReprojectionVertexShader.cso");
				VSReprojection->SetInputLayout(PMain->GetVertexDefinition());

				GSReprojection = new GeometryShader(Device, L"ReprojectionGeometryShader.cso");

				PSInfrared = new PixelShader(Device, L"InfraredPixelShader.cso");
				PSColor = new PixelShader(Device, L"ColorPixelShader.cso");
				PSSinusoidal = new PixelShader(Device, L"SinusoidalPixelShader.cso");
				
				SLinearWrap = new Sampler(Device, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_BORDER);

				ZeroMemory(&Params, sizeof(RenderingParameters));				
				CBDepthAndColor = new ConstantBuffer<DepthAndColorConstants>(Device);
				
				KinectReady = false;

				TColor = TDepth = 0;
			}

			void DestroyResources()
			{
				KinectReady = false;
				delete QMain;
				delete PMain;
				delete VSSimple;
				delete VSReprojection;
				delete GSReprojection;
				delete PSInfrared;
				delete PSColor;
				delete PSSinusoidal;
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
					TDepth->SetForPS();
					SLinearWrap->SetForPS();				
					QMain->Draw();
					break;
				case KinectDevice::Color:
					VSSimple->Apply();
					PSColor->Apply();
					TColor->SetForPS();
					SLinearWrap->SetForPS();				
					QMain->Draw();
					break;
				case KinectDevice::DepthAndColor:
					CBDepthAndColor->Update(&DepthAndColorOptions);

					VSReprojection->Apply();
					CBDepthAndColor->SetForVS();
					TDepth->SetForVS();

					GSReprojection->Apply();
					CBDepthAndColor->SetForGS();

					PSSinusoidal->Apply();
								
					PMain->Draw();
					break;
				case KinectDevice::Infrared:
					VSSimple->Apply();
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
				XMMATRIX R2 = XMMatrixRotationZ(-Params.Rotation * XM_PIDIV2);
				XMMATRIX world = Ti * R * T * T2;
				XMMATRIX reproj = DepthIntrinsics * world * DepthInverseIntrinsics;
				XMMATRIX reprojModel = DepthIntrinsics * Ti * R *T;
				XMMATRIX reprojNormals = XMMatrixTranspose(XMMatrixInverse(0, world));
				XMStoreFloat4x4(&DepthAndColorOptions.ReprojectionTransform, reproj);
				XMStoreFloat4x4(&DepthAndColorOptions.ModelTransform, reprojModel);
				XMStoreFloat4x4(&DepthAndColorOptions.WorldTransform, world);
				XMStoreFloat4x4(&DepthAndColorOptions.NormalTransform, reprojNormals);
				XMStoreFloat4x4(&DepthAndColorOptions.SceneRotation, R2);

				XMFLOAT2 modelScale;
				float depthAspectRatio = (float)KinectDevice::DepthWidth / (float)KinectDevice::DepthHeight;
				float aspectRatio = MainViewport->Width / MainViewport->Height;
				if(Params.Rotation % 2 == 0)
				{
					if(aspectRatio >= depthAspectRatio)
					{
						modelScale.y = 1.f;
						modelScale.x = depthAspectRatio / aspectRatio;
					}
					else
					{
						modelScale.x = 1.f;
						modelScale.y = aspectRatio / depthAspectRatio;
					}
				}
				else
				{
					if(aspectRatio >= depthAspectRatio)
					{
						modelScale.y = 1.f / depthAspectRatio;
						modelScale.x = 1.f / aspectRatio;
					}
					else
					{
						modelScale.x = depthAspectRatio;
						modelScale.y = aspectRatio;
					}
				}
				DepthAndColorOptions.ModelScale = modelScale;
				DepthAndColorOptions.DepthSize = XMINT2(KinectDevice::DepthWidth, KinectDevice::DepthHeight);
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

				DepthAndColorOptions.Scale = scale;
				DepthAndColorOptions.Move = XMFLOAT2(moveX, moveY);
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

			void SetShading(float depthLimit)
			{
				DepthAndColorOptions.DepthLimit = depthLimit;
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

			void Resize()
			{
				/*LPRECT clientRect;
				GetClientRect(Window, clientRect);

				Error(SwapChain->ResizeBuffers(1, clientRect->right - clientRect->left, clientRect->bottom - clientRect->top, DXGI_FORMAT_R8G8B8A8_UNORM, 0)); 
				MainViewport->Width = clientRect->right - clientRect->left;
				MainViewport->Height = clientRect->bottom - clientRect->top;
				DeviceContext->RSSetViewports(1, MainViewport);	*/
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