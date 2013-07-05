#pragma once
#pragma unmanaged
#include "GreenGraphics.h"
#include "GreenKinectAdapter.h"
#pragma managed
using namespace std;
using namespace System;
using namespace System::Windows;
using namespace System::Windows::Interop;
using namespace System::Runtime::InteropServices;
using namespace System::Reflection;
using namespace System::IO;
using namespace Green::Kinect;

namespace Green
{
	namespace Graphics
	{
		public ref class DirectXCanvas : public HwndHost
		{
		private:
			DirectXWindow* XWindow;
			HWND Host, Canvas;
			static DirectXCanvas()
			{
				VertexDefinition::Init();
			}

			void InitXWindowForKinect()
			{
				KinectManager^ KM = (KinectManager^)DataContext;
				if(KM != nullptr && XWindow != nullptr)
				{
					XWindow->InitKinect(KM->Device);
				}
			}
		protected:
			virtual HandleRef BuildWindowCore(HandleRef hwndParent) override
			{
				HWND parent = (HWND)hwndParent.Handle.ToPointer();

				Host = nullptr;
				Host = CreateWindowEx(
					0, L"static", L"",
					WS_CHILD,
					0, 0, (int)Width, (int)Height,
					parent, 
					0, nullptr, 0);
				String^ root = Path::GetDirectoryName(Assembly::GetExecutingAssembly()->Location);
				XWindow = new DirectXWindow(Host, StringToLPWSTR(root));
				InitXWindowForKinect();
				return HandleRef(this, (IntPtr)Host);
			}

			virtual void DestroyWindowCore(HandleRef hwnd) override
			{
				XWindow = nullptr;
				delete XWindow;
				DestroyWindow((HWND)hwnd.Handle.ToPointer());
			}

			virtual IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, bool %handled) override
			{
				static int resizeTimer = 0;
				switch (msg)
				{
				case WM_SIZE:
					resizeTimer = SetTimer((HWND)hwnd.ToPointer(), resizeTimer, 100, 0);
					break;
				case WM_TIMER:
					XWindow->Resize();
					break;
				}
				handled = false;
				return IntPtr::Zero;
			}

			void OnDataContextChanged(Object^ sender, 
	DependencyPropertyChangedEventArgs e)
			{
				InitXWindowForKinect();
			}

		public:
			DirectXCanvas()
			{
				XWindow = nullptr;
				DataContextChanged += gcnew DependencyPropertyChangedEventHandler(this, &DirectXCanvas::OnDataContextChanged);
			}

			~DirectXCanvas()
			{
				delete XWindow;
			}

			void SetView(
				float transX, float transY, float transZ, 
				float rotX, float rotY, float rotZ, 
				float scale, float moveX, float moveY, float rotation)
			{
				XWindow->SetView(transX, transY, transZ, rotX, rotY, rotZ, scale, moveX, moveY, rotation);
			}

			void SetCameras(
				array<float, 2>^ infraredIntrinsics, array<float, 2>^ depthToIRMapping,
				array<float, 2>^ colorIntrinsics, array<float, 2>^ colorRemapping,
				array<float, 2>^ colorExtrinsics, int colorDispX, int colorDispY, 
				float colorScaleX, float colorScaleY)
			{
				if(Is3x3(infraredIntrinsics) && Is3x3(depthToIRMapping))
				{
					pin_ptr<float> pInfraredIntrinsics = &To4x4(infraredIntrinsics)[0, 0];
					pin_ptr<float> pDepthToIRMapping = &Expand4x4(depthToIRMapping)[0, 0];
					pin_ptr<float> pColorIntrinsics = &To4x4(colorIntrinsics)[0, 0];
					pin_ptr<float> pColorRemapping = &Expand4x4(colorRemapping)[0, 0];
					pin_ptr<float> pColorExtrinsics = &colorExtrinsics[0, 0];
					XWindow->SetCameras(
						pInfraredIntrinsics, pDepthToIRMapping,
						pColorIntrinsics, pColorRemapping, pColorExtrinsics,
						colorDispX, colorDispY, colorScaleX, colorScaleY);
				}
			}

			void SetPreprocessing(int depthAveraging, int depthGaussIterations, float depthGaussSigma)
			{
				XWindow->SetPreprocessing(depthAveraging, depthGaussIterations, depthGaussSigma);
			}

			void SetPerformance(int triangleGridWidth, int triangleGridHeight)
			{
				XWindow->SetPerformance(triangleGridWidth, triangleGridHeight);
			}

			enum class ShadingModes {
				Zebra,
				Rainbow,
				ShadedRainbow,
				Scale,
				ShadedScale,
				Blinn,
				Textured
			};

			void SetShading(ShadingModes mode, float depthLimit, float shadingPeriode, float shadingPhase, float triangleLimit)
			{
				XWindow->SetShading((DirectXWindow::ShadingModes)mode, depthLimit, shadingPeriode, shadingPhase, triangleLimit);
			}

			void Draw()
			{
				if(XWindow!=nullptr) 
					XWindow->Draw();
			}
		};

	}
}

