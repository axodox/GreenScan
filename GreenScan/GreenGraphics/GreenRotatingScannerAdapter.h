#pragma once
#pragma unmanaged
#include "GreenRotatingScanner.h"
#include "GreenGraphicsAdapter.h"
#include "GreenKinectAdapter.h"
#include "GreenResources.h"
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
		public:
			enum class Modes
			{
				OneAxis,
				TwoAxis,
				Volumetric
			};

			enum class AxialViews {
				Overlay,
				DepthL,
				DepthR,
				TextureL,
				TextureR,
				Model
			};

			enum class VolumetricViews {
				Overlay,
				Projection,
				Slice,
				Model
			};			
		private:
			Turntable^ table;
			KinectManager^ Kinect;
			GraphicsCanvas^ Canvas;
			DispatcherTimer^ ProgressTimer;
			RotatingScannerModule* ScannerModule;
			bool isConnected, isScanning, toOrigin;
			Modes Mode;
			bool HasMirror;

			void OnPropertyChanged(String^ name)
			{
				PropertyChanged(this, gcnew PropertyChangedEventArgs(name));
			}

			void ToOrigin()
			{
				toOrigin = true;
				table->ToOrigin();
				StatusChanged(this, gcnew StatusEventArgs(GreenResources::GetString("TurntableFindingOrigin"), true));
			}

			void OnTableConnected(Object^ sender, EventArgs^ e)
			{
				if(isConnected) return;
				isConnected = true;
				ScannerModule = new RotatingScannerModule();
				ScannerModule->SetMode((RotatingScannerModule::Modes)Mode);
				OnPropertyChanged("IsConnected");
				Canvas->GetDirectXWindow()->LoadModule(ScannerModule);

				table = Turntable::DefaultDevice;
				table->MotorStopped += gcnew EventHandler(this, &RotatingScanner::OnMotorStopped);				
				table->PositionChanged += gcnew EventHandler(this, &RotatingScanner::OnPositionChanged);
				ToOrigin();
			}

			void OnPositionChanged(Object^ sender, EventArgs^ e)
			{
				if(HasMirror)
					ScannerModule->SetTurntablePosition(table->PositionInRadians);
				else
					ScannerModule->SetTurntablePosition(-table->PositionInRadians);
			}

			void OnProgressChanged(Object^ sender, EventArgs^ e)
			{
				if(!isConnected || toOrigin) return;
				double progress = table->PositionInUnits;
				StatusChanged(this, gcnew StatusEventArgs(GreenResources::GetString("TurntableScanningInProgress") + " " + (progress*100.0).ToString("F2") + "%", true, progress));
			}

			void OnMotorStopped(Object^ sender, EventArgs^ e)
			{
				toOrigin = false;
				ProgressTimer->Stop();
				StatusChanged(this, gcnew StatusEventArgs(GreenResources::GetString("TurntableReady")));
				EndScan();
				OnPropertyChanged("IsAtOrigin");
			}

			void OnTableDisconnected(Object^ sender, EventArgs^ e)
			{
				StatusChanged(this, gcnew StatusEventArgs(GreenResources::GetString("TurntableDisconnected")));
				if(Turntable::DeviceCount != 0) return;

				isConnected = false;
				OnPropertyChanged("IsConnected");

				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
				SafeDelete(ScannerModule);
			}

			void EndScan()
			{
				if (!isScanning) return;
				if (isConnected) table->Stop();
				if (ScannerModule) ScannerModule->Stop();
				isScanning = false;
				OnPropertyChanged("IsScanning");
			}
		public:
			void SetMode(Modes mode)
			{
				Mode = mode;
				if (ScannerModule)
					ScannerModule->SetMode((RotatingScannerModule::Modes)mode);
			}

			Modes GetMode()
			{
				if (ScannerModule)
					return (Modes)ScannerModule->GetMode();
				else
					return Mode;
			}

			void SetTurntable(array<float, 2>^ turntableTransform, int piSteps, bool hasMirror)
			{
				pin_ptr<float> pTurntableTransform = &turntableTransform[0, 0];
				if (ScannerModule) ScannerModule->SetTurntable(pTurntableTransform);
				if(table) table->PiSteps = piSteps;
				HasMirror = hasMirror;
			}

			void SetAxial(
				AxialViews view,
				float height, float radius, float coreX, float coreY, 
				int modelWidth, int modelHeight, int textureWidth, int textureHeight)
			{
				if (ScannerModule) ScannerModule->SetAxial(
					(RotatingScannerModule::AxialViews)view, height, radius, coreX, coreY,
					modelWidth, modelHeight, textureWidth, textureHeight);
			}

			void SetVolumetric(VolumetricViews view, float cubeSize, int cubeRes, float depth, float threshold, float gradientLimit)
			{
				if (ScannerModule)
					ScannerModule->SetVolumetric((RotatingScannerModule::VolumetricViews)view, cubeSize, cubeRes, depth, threshold, gradientLimit);
			}

			virtual event StatusEventHandler^ StatusChanged;
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			property bool IsConnected { bool get() { return isConnected; }}
			property bool IsScanning { bool get() { return isScanning; }}
			property bool IsAtOrigin 
			{ 
				bool get() 
				{
					return isConnected && table->AtOrigin;
				}
			}
			property bool CanSave
			{ 
				bool get() 
				{
					return isConnected && ScannerModule->State == RotatingScannerModule::States::Processing;
				}
			}
			property Turntable^ Table 
			{ 
				Turntable^ get() 
				{
					if(IsConnected)
						return table;
					else
						return nullptr;
				}
			}
			RotatingScanner(KinectManager^ kinectManager, GraphicsCanvas^ graphicsCanvas)
			{
				Kinect = kinectManager;
				Canvas = graphicsCanvas;
				ScannerModule = nullptr;
				table = nullptr;
				Turntable::DeviceConnected += gcnew EventHandler(this, &RotatingScanner::OnTableConnected);
				Turntable::DeviceDisconnected += gcnew EventHandler(this, &RotatingScanner::OnTableDisconnected);
				ProgressTimer = gcnew DispatcherTimer(DispatcherPriority::Render);
				ProgressTimer->Interval = TimeSpan(0, 0, 0, 0, 100);
				ProgressTimer->Tick += gcnew EventHandler(this, &RotatingScanner::OnProgressChanged);
			}

			void Scan()
			{
				if(!isConnected || isScanning) return;
				isScanning = true;
				table->TurnOnce();
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

			virtual bool OpenRaw(String^ path, String^ %metadata)
			{
				if (!ScannerModule) return false;
				metadata = nullptr;
				LPWSTR nMetadata = 0;
				LPWSTR npath = StringToLPWSTR(path);
				bool ok = ScannerModule->OpenRaw(npath, nMetadata);
				if (ok)
				{
					metadata = gcnew String(nMetadata);
					LPWSTRDelete(npath);
				}
				else
					ScannerModule->EndProcessing();
				return ok;
			}

			virtual bool SaveRaw(String^ path, String^ metadata)
			{
				if (!ScannerModule) return false;
				LPWSTR npath = StringToLPWSTR(path);
				LPWSTR nMetadata = StringToLPWSTR(metadata);
				bool ok = ScannerModule->SaveRaw(npath, nMetadata);
				LPWSTRDelete(npath);
				LPWSTRDelete(nMetadata);
				return ok;
			}

			void Stop()
			{
				if(!isConnected || !isScanning) return;
				EndScan();
			}

			void ReturnToOrigin()
			{
				if(!isConnected || isScanning || table->AtOrigin) return;
				ToOrigin();
			}			

			~RotatingScanner()
			{
				Canvas->GetDirectXWindow()->UnloadModule(ScannerModule);
				if (table)
				{
					delete table;
				}
			}
		};
	}
}