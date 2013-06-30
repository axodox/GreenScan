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
		public ref class DirectXCanvas : HwndHost
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
				switch (msg)
				{
				case WM_SIZE:
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
				//delete XWindow;
			}

			void SetView(
				float transX, float transY, float transZ, 
				float rotX, float rotY, float rotZ, 
				float scale, float moveX, float moveY, float rotation)
			{
				XWindow->SetView(transX, transY, transZ, rotX, rotY, rotZ, scale, moveX, moveY, rotation);
			}

			void SetCameras(array<float, 2>^ infraredIntrinsics, array<float, 2>^ depthToIR)
			{
				if(Is3x3(infraredIntrinsics) && Is3x3(depthToIR))
				{
					pin_ptr<float> pDepth = &To4x4(infraredIntrinsics)[0, 0];
					pin_ptr<float> pDepthToIR = &Expand4x4(depthToIR)[0, 0];
					XWindow->SetCameras(pDepth, pDepthToIR);
				}
			}

			void SetShading(float depthLimit, float triangleLimit)
			{
				XWindow->SetShading(depthLimit, triangleLimit);
			}

			void Draw()
			{
				if(XWindow!=nullptr) 
					XWindow->Draw();
			}
		};

	}
}

