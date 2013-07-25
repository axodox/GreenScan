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
				ScannerModule = new RotatingScannerModule();
				Canvas->GetDirectXWindow()->LoadModule(ScannerModule);

				connected = true;
				PropertyChanged(this, gcnew PropertyChangedEventArgs("Connected"));				
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

			void SetPerformance(int modelWidth, int modelHeight, int textureWidth, int textureHeight)
			{
				if(ScannerModule) ScannerModule->SetPerformance(modelWidth, modelHeight, textureWidth, textureHeight);
			}

			void SetCalibration(
				float transX, float transY, float transZ,
				float rotX, float rotY, float rotZ,
				float height, float radius, 
				float coreX, float coreY)
			{
				if(ScannerModule) ScannerModule->SetCalibration(
					transX, transY, transZ,	rotX, rotY, rotZ, 
					height, radius, coreX, coreY);
			}

			~RotatingScanner()
			{
				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
			}
		};
	}
}