#pragma once
#pragma unmanaged
#include "GreenRotatingScanner.h"
#include "GreenGraphicsAdapter.h"
#include "GreenKinectAdapter.h"
#pragma managed
using namespace System;
using namespace System::Windows::Threading;
using namespace Green::Kinect;
using namespace Green::Extensions;
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
			DispatcherTimer^ ProgressTimer;
			RotatingScannerModule* ScannerModule;
			bool connected;
			void OnTableConnected(Object^ sender, EventArgs^ e)
			{
				ScannerModule = new RotatingScannerModule();
				Canvas->GetDirectXWindow()->LoadModule(ScannerModule);

				Table = Turntable::DefaultDevice;
				Table->MotorStopped += gcnew EventHandler(this, &RotatingScanner::OnMotorStopped);
				Table->ToOrigin();
				Table->PositionChanged += gcnew EventHandler(this, &RotatingScanner::OnPositionChanged);
				StatusChanged(this, gcnew StatusEventArgs("Finding origin...", true));

				connected = true;
				PropertyChanged(this, gcnew PropertyChangedEventArgs("Connected"));				
			}

			void OnPositionChanged(Object^ sender, EventArgs^ e)
			{
				ScannerModule->SetTurntablePosition(Table->PositionInRadians);
			}

			void OnProgressChanged(Object^ sender, EventArgs^ e)
			{
				double progress = Table->PositionInUnits;
				StatusChanged(this, gcnew StatusEventArgs("Scanning in progress: " + (progress*100.0).ToString("F2") + "%", true, progress));
			}

			void OnMotorStopped(Object^ sender, EventArgs^ e)
			{
				ProgressTimer->Stop();
				StatusChanged(this, gcnew StatusEventArgs("Turntable ready."));
			}

			void OnTableDisconnected(Object^ sender, EventArgs^ e)
			{
				if(Turntable::DeviceCount != 0)
				{
					StatusChanged(this, gcnew StatusEventArgs("Turntable disconnected.", true));
					return;
				}

				connected = false;
				PropertyChanged(this, gcnew PropertyChangedEventArgs("Connected"));

				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
				SafeDelete(ScannerModule);
			}
		public:
			enum class Views {
				Overlay,
				Depth,
				TextureL,
				TextureR
			};

			void SetShading(Views view)
			{
				if(ScannerModule)
					ScannerModule->SetShading((RotatingScannerModule::Views)view);
			}

			virtual event StatusEventHandler^ StatusChanged;
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			property bool Connected { bool get() { return connected; }}
			RotatingScanner(KinectManager^ kinectManager, GraphicsCanvas^ graphicsCanvas)
			{
				Kinect = kinectManager;
				Canvas = graphicsCanvas;
				ScannerModule = nullptr;
				Table = nullptr;
				Turntable::DeviceConnected += gcnew EventHandler(this, &RotatingScanner::OnTableConnected);
				Turntable::DeviceDisconnected += gcnew EventHandler(this, &RotatingScanner::OnTableDisconnected);
				ProgressTimer = gcnew DispatcherTimer(DispatcherPriority::Render);
				ProgressTimer->Interval = TimeSpan(0, 0, 0, 0, 100);
				ProgressTimer->Tick += gcnew EventHandler(this, &RotatingScanner::OnProgressChanged);
			}

			void SetPerformance(int modelWidth, int modelHeight, int textureWidth, int textureHeight)
			{
				if(ScannerModule) ScannerModule->SetPerformance(modelWidth, modelHeight, textureWidth, textureHeight);
			}

			void Scan()
			{
				if(!connected) return;
				Table->TurnOnce();
				ProgressTimer->Start();
			}

			void SetCalibration(
				array<float, 2>^ turntableTransform,
				float height, float radius, 
				float coreX, float coreY)
			{
				pin_ptr<float> pTurntableTransform = &turntableTransform[0, 0];
				if(ScannerModule) ScannerModule->SetCalibration(
					pTurntableTransform, height, radius, coreX, coreY);
			}

			~RotatingScanner()
			{
				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
				if(Table)
				{
					delete Table;
				}
			}
		};
	}
}