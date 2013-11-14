#pragma once
#define WIN32_LEAN_AND_MEAN
#include "GreenKinect.h"
#include "Helper.h"
#pragma managed
using namespace System;
using namespace System::ComponentModel;
using namespace System::Runtime::InteropServices;
using namespace System::Windows::Threading;
namespace Green
{
	namespace Kinect
	{
		public ref class KinectManager : public INotifyPropertyChanged
		{
		public: 
			enum class Modes
			{
				Depth = 1,
				Color = 2,
				DepthAndColor = 3,
				Infrared = 4
			};
		private:
			int deviceCount;
			bool deviceOpened, processing, providesData, fileOpened;
			Modes mode;
		public:			
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			virtual event EventHandler^ DeviceCountChanged;
			virtual event EventHandler^ DeviceDisconnected;
			virtual event EventHandler^ DeviceStarting;
			property Modes Mode	
			{ 
				Modes get() { return mode; } 
			private: 
				void set(Modes value) { mode = value; OnPropertyChanged("Mode"); }
			}
			property int DeviceCount 
			{ 
				int get() { return deviceCount; } 
			private: 
				void set(int value) { deviceCount = value; OnPropertyChanged("DeviceCount"); }
			}
			property bool DeviceOpened 
			{ 
				bool get() { return deviceOpened; } 
			private: 
				void set(bool value) { deviceOpened = value; OnPropertyChanged("DeviceOpened"); }
			}
			property bool FileOpened
			{ 
				bool get() { return fileOpened; } 
			private: 
				void set(bool value) { fileOpened = value; OnPropertyChanged("FileOpened"); }
			}
			property bool Processing
			{ 
				bool get() { return processing; }
			private: 
				void set(bool value) { processing = value; OnPropertyChanged("Processing"); }
			}
			property bool ProvidesData
			{ 
				bool get() { return providesData; } 
			private: 
				void set(bool value) { providesData = value; OnPropertyChanged("ProvidesData"); }
			}
			property int Angle 
			{ 
				int get() { return Device->GetAngle(); } 
				void set(int value) { Device->SetAngle(value); }
			}
			static property int DepthWidth { int get() { return KinectDevice::DepthWidth; }}
			static property int DepthHeight { int get() { return KinectDevice::DepthHeight; }}
			static property int ColorWidth { int get() { return KinectDevice::ColorWidth; }}
			static property int ColorHeight { int get() { return KinectDevice::ColorHeight; }}
			KinectDevice* Device;
		private:
			void OnPropertyChanged(String^ name)
			{
				PropertyChanged(this, gcnew PropertyChangedEventArgs(name));
			}			
			delegate void KinectCountChangedHandler(int count);
			KinectCountChangedHandler^ KinectCountChanged;
			void CountChangedCallback(int count)
			{      
				DeviceCount = count;
				DeviceCountChanged(this, EventArgs::Empty);
			}

			delegate void KinectDisconnectedHandler();
			KinectDisconnectedHandler^ KinectDisconnected;
			void KinectDisconnectedCallback()
			{
				DeviceDisconnected(this, EventArgs::Empty);
			}
		public:
			bool OpenKinect(int index)
			{
				DeviceOpened = Device->OpenKinect(index);
				return DeviceOpened;
			}

			bool StartKinect(Modes mode)
			{
				if(!DeviceOpened) return false;
				StopKinect();	
				DeviceStarting(this, EventArgs::Empty);
				bool ok = false;
				ok = Device->StartKinect((KinectDevice::Modes)mode);
				if(ok)
				{
					this->Mode = mode;
					Processing = true;
					ProvidesData = true;	
					FileOpened = false;
				}
				return ok;
			}

			bool SaveRaw(String^ path, String^ metadata)
			{
				if(Processing)
				{
					LPWSTR nPath = StringToLPWSTR(path);
					LPWSTR nMetadata = StringToLPWSTR(metadata);
					bool ok = Device->SaveRaw(nPath, nMetadata);
					LPWSTRDelete(nPath);
					LPWSTRDelete(nMetadata);

					return ok;
				}
				else
					return false;
			}
		private:
			int continousShootingCounter;
			bool continousShootingEnabled;
			DispatcherTimer^ ContinousShootingTimer;
			void ContinousShootingTimer_Tick(Object^ sender, EventArgs^ e)
			{
				if(continousShootingCounter != Device->GetContinousShootingCount())
					ContinousShootingCounter = Device->GetContinousShootingCount();
			}
		public:
			property bool ContinousShootingEnabled
			{
				bool get() { return continousShootingEnabled; }
			private:
				void set(bool value) { continousShootingEnabled = value; OnPropertyChanged("ContinousShootingEnabled"); }
			}
			property unsigned ContinousShootingCounter
			{
				unsigned get() { return continousShootingCounter; }
			private:
				void set(unsigned value) { continousShootingCounter = value; OnPropertyChanged("ContinousShootingCounter"); }
			}
			bool StartContinousShooting(String^ pathWithoutExtension, String^ metadata, int interval)
			{
				if(ContinousShootingEnabled) return false;
				LPWSTR nPath = StringToLPWSTR(pathWithoutExtension);
				LPWSTR nMetadata = StringToLPWSTR(metadata);
					
				bool ok = Device->StartContinousShooting(nPath, nMetadata, interval);
				if(ok)
				{
					ContinousShootingCounter = 0;
					ContinousShootingEnabled = true;
					ContinousShootingTimer = gcnew DispatcherTimer(DispatcherPriority::DataBind);
					ContinousShootingTimer->Interval = TimeSpan(0, 0, 0, 0, 100);
					ContinousShootingTimer->Tick += gcnew EventHandler(this, &KinectManager::ContinousShootingTimer_Tick);
					ContinousShootingTimer->Start();
				}
				
				LPWSTRDelete(nPath);
				LPWSTRDelete(nMetadata);
				return ok;
			}

			void StopContinousShooting()
			{
				if(ContinousShootingEnabled)
				{
					Device->StopContinousShooting();
					ContinousShootingTimer->Stop();
					ContinousShootingEnabled = false;					
				}
			}

			void CloseFile()
			{
				if(FileOpened)
				{
					FileOpened = false;
					ProvidesData = false;
					Device->CloseFile();
				}
			}

			bool Import(String^ path, array<float, 2>^ infraredIntrinsics)
			{
				if(Processing || !Is3x3(infraredIntrinsics))
					return false;
				else
				{
					FileOpened = false;
					pin_ptr<float> pInfraredIntrinsics = &To4x4(infraredIntrinsics)[0, 0];
					LPWSTR nPath = StringToLPWSTR(path);
					bool ok = Device->Import(nPath, pInfraredIntrinsics);
					if(ok)
					{
						Mode = Modes::DepthAndColor;									
						ProvidesData = true;
						FileOpened = true;
					}
					LPWSTRDelete(nPath);
					return ok;
				}
			}

			bool OpenRaw(String^ path, String^ %metadata)
			{
				if(Processing)
					return false;
				else
				{
					FileOpened = false;
					LPWSTR nPath = StringToLPWSTR(path);
					LPWSTR nMetadata = 0;
					KinectDevice::Modes newmode;
					bool ok = Device->OpenRaw(nPath, newmode, nMetadata);
					if(ok)
					{
						metadata = gcnew String(nMetadata);						
						LPWSTRDelete(nMetadata);
						Mode = (Modes)newmode;					
						ProvidesData = true;
						FileOpened = true;
					}
					LPWSTRDelete(nPath);
					return ok;
				}
			}

			void SetEmitter(bool enabled)
			{
				Device->SetEmitter(enabled);
			}

			void SetNearMode(bool enabled)
			{
				Device->SetNearMode(enabled);
			}

			void StopKinect()
			{
				if(!Processing) return;
				StopContinousShooting();
				Processing = false;
				ProvidesData = false;
				FileOpened = false;
				Device->StopKinect();				
			}
			
			KinectManager()
			{
				Device = new KinectDevice();
				KinectDisconnected = gcnew KinectDisconnectedHandler(this, &KinectManager::KinectDisconnectedCallback);
				Device->KinectDisconnected = (KinectDevice::ArgumentlessCallback)Marshal::GetFunctionPointerForDelegate(KinectDisconnected).ToPointer();
				deviceOpened = false;
				deviceCount = KinectDevice::GetDeviceCount();
				KinectCountChanged = gcnew KinectCountChangedHandler(this, &KinectManager::CountChangedCallback);
				Device->KinectCountChanged = (KinectDevice::KinectCountChangedCallback)Marshal::GetFunctionPointerForDelegate(KinectCountChanged).ToPointer();
			}

			void CloseKinect()
			{
				SafeDelete(Device);
			}

			~KinectManager()
			{
				CloseKinect();
			}
		};
	}
}