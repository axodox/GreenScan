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
		public ref class RotatingScanner : public INotifyPropertyChanged, IModelSaver
		{
		private:
			Turntable^ Table;
			KinectManager^ Kinect;
			GraphicsCanvas^ Canvas;
			DispatcherTimer^ ProgressTimer;
			RotatingScannerModule* ScannerModule;
			bool isConnected, isScanning;

			void OnPropertyChanged(String^ name)
			{
				PropertyChanged(this, gcnew PropertyChangedEventArgs(name));
			}

			void ToOrigin()
			{
				Table->ToOrigin();
				StatusChanged(this, gcnew StatusEventArgs("Finding origin...", true));
			}

			void OnTableConnected(Object^ sender, EventArgs^ e)
			{
				ScannerModule = new RotatingScannerModule();
				Canvas->GetDirectXWindow()->LoadModule(ScannerModule);

				Table = Turntable::DefaultDevice;
				Table->MotorStopped += gcnew EventHandler(this, &RotatingScanner::OnMotorStopped);				
				Table->PositionChanged += gcnew EventHandler(this, &RotatingScanner::OnPositionChanged);
				ToOrigin();

				isConnected = true;
				OnPropertyChanged("IsConnected");				
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
				EndScan();
				OnPropertyChanged("IsAtOrigin");
			}

			void OnTableDisconnected(Object^ sender, EventArgs^ e)
			{
				if(Turntable::DeviceCount != 0)
				{
					StatusChanged(this, gcnew StatusEventArgs("Turntable disconnected.", true));
					return;
				}

				isConnected = false;
				OnPropertyChanged("IsConnected");

				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
				SafeDelete(ScannerModule);
			}

			void EndScan()
			{
				if (!isScanning) return;
				if (isConnected) Table->Stop();
				if (ScannerModule) ScannerModule->Stop();
				isScanning = false;
				OnPropertyChanged("IsScanning");
			}
		public:
			enum class Views {
				Overlay,
				DepthL,
				DepthR,
				TextureL,
				TextureR,
				Model
			};

			void SetShading(Views view)
			{
				if (ScannerModule)
					ScannerModule->SetShading((RotatingScannerModule::Views)view);
			}

			virtual event StatusEventHandler^ StatusChanged;
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			property bool IsConnected { bool get() { return isConnected; }}
			property bool IsScanning { bool get() { return isScanning; }}
			property bool IsAtOrigin 
			{ 
				bool get() 
				{
					return isConnected && Table->AtOrigin;
				}
			}
			property bool CanSave
			{ 
				bool get() 
				{
					return isConnected && ScannerModule->Processing;
				}
			}
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
				if (ScannerModule) ScannerModule->SetPerformance(modelWidth, modelHeight, textureWidth, textureHeight);
			}

			void Scan()
			{
				if(!isConnected || isScanning) return;
				isScanning = true;
				Table->TurnOnce();
				ProgressTimer->Start();
				if (ScannerModule) ScannerModule->Scan();
				OnPropertyChanged("IsScanning");
			}

			virtual bool SaveModel(String^ path, SaveFormats format)
			{
				if (!ScannerModule) return false;
				LPWSTR npath = StringToLPWSTR(path);
				bool ok = ScannerModule->SaveModel(npath, (ModelFormats)format);
				LPWSTRDelete(npath);
				return ok;
			}

			virtual bool OpenRaw(String^ path)
			{
				if (!ScannerModule) return false;
				LPWSTR npath = StringToLPWSTR(path);
				ScannerModule->PrepareForStaticInput();
				bool ok = ScannerModule->OpenRaw(npath);
				if (!ok) ScannerModule->EndProcessing();
				LPWSTRDelete(npath);
				return ok;
			}

			virtual bool SaveRaw(String^ path)
			{
				if (!ScannerModule) return false;
				LPWSTR npath = StringToLPWSTR(path);
				bool ok = ScannerModule->SaveRaw(npath);
				LPWSTRDelete(npath);
				return ok;
			}

			void Stop()
			{
				if(!isConnected || !isScanning) return;
				EndScan();
			}

			void ReturnToOrigin()
			{
				if(!isConnected || isScanning || Table->AtOrigin) return;
				ToOrigin();
			}

			void SetCalibration(
				array<float, 2>^ turntableTransform,
				float height, float radius, 
				float coreX, float coreY)
			{
				pin_ptr<float> pTurntableTransform = &turntableTransform[0, 0];
				if (ScannerModule) ScannerModule->SetCalibration(
					pTurntableTransform, height, radius, coreX, coreY);
			}

			~RotatingScanner()
			{
				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
				if (Table)
				{
					delete Table;
				}
			}
		};
	}
}