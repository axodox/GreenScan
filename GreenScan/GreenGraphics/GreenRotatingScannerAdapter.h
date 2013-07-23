#pragma once
#pragma unmanaged
#include "GreenRotatingScanner.h"
#include "GreenGraphicsAdapter.h"
#include "GreenKinectAdapter.h"
#pragma managed
using namespace System;
using namespace Green::Kinect;
using namespace Turntables;

namespace Green
{
	namespace Graphics
	{
		public ref class RotatingScanner : public INotifyPropertyChanged
		{
		private:
			Turntable^ Table;
			KinectManager^ Kinect;
			GraphicsCanvas^ Canvas;
			RotatingScannerModule* ScannerModule;
			bool connected;
			void OnTableConnected(Object^ sender, EventArgs^ e)
			{
				connected = true;
				PropertyChanged(this, gcnew PropertyChangedEventArgs("Connected"));

				ScannerModule = new RotatingScannerModule();
				Canvas->GetDirectXWindow()->LoadModule(ScannerModule);
			}

			void OnTableDisconnected(Object^ sender, EventArgs^ e)
			{
				if(Turntable::DeviceCount != 0) return;

				connected = false;
				PropertyChanged(this, gcnew PropertyChangedEventArgs("Connected"));

				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
				SafeDelete(ScannerModule);
			}
		public:
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			property bool Connected { bool get() { return connected; }}
			RotatingScanner(KinectManager^ kinectManager, GraphicsCanvas^ graphicsCanvas)
			{
				Kinect = kinectManager;
				Canvas = graphicsCanvas;
				ScannerModule = nullptr;
				Turntable::DeviceConnected += gcnew EventHandler(this, &RotatingScanner::OnTableConnected);
				Turntable::DeviceDisconnected += gcnew EventHandler(this, &RotatingScanner::OnTableDisconnected);
			}

			~RotatingScanner()
			{
				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
			}
		};
	}
}