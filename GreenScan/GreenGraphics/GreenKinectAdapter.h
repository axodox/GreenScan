#pragma once
#define WIN32_LEAN_AND_MEAN
#include "GreenKinect.h"
#include "Helper.h"
#pragma managed
using namespace System;
using namespace System::ComponentModel;
using namespace System::Runtime::InteropServices;
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
			bool deviceOpened, processing, providesData;
			Modes mode;
		public:
			property Modes Mode	{ Modes get() { return mode; }}
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			property int DeviceCount { int get() { return deviceCount; }}
			property bool DeviceOpened { bool get()	{ return deviceOpened; }}
			property bool Processing { bool get() { return processing; }}
			property bool ProvidesData { bool get() { return providesData; }}
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
				deviceCount = count;
				OnPropertyChanged("DeviceCount");
			}
		public:
			bool OpenKinect(int index)
			{
				deviceOpened = Device->OpenKinect(index);
				OnPropertyChanged("DeviceOpened");
				return deviceOpened;
			}

			bool StartKinect(Modes mode)
			{
				if(!deviceOpened) return false;
				StopKinect();	
				bool ok = false;
				ok = Device->StartKinect((KinectDevice::Modes)mode);
				if(ok)
				{
					this->mode = mode;
					OnPropertyChanged("Mode");
					processing = true;
					OnPropertyChanged("Processing");
					providesData = true;
					OnPropertyChanged("ProvidesData");
				}
				return ok;
			}

			bool SaveRaw(String^ path)
			{
				if(processing)
				{
					LPWSTR npath = StringToLPWSTR(path);
					bool ok = Device->SaveRaw(npath);
					LPWSTRDelete(npath);
					return ok;
				}
				else
					return false;
			}

			bool OpenRaw(String^ path)
			{
				if(processing)
					return false;
				else
				{
					LPWSTR npath = StringToLPWSTR(path);
					KinectDevice::Modes newmode;
					bool ok = Device->OpenRaw(npath, newmode);
					mode = (Modes)newmode;
					OnPropertyChanged("Mode");
					LPWSTRDelete(npath);
					if(ok)
					{
						providesData = true;
						OnPropertyChanged("ProvidesData");
					}
					return ok;
				}
			}

			void SetEmitter(bool enabled)
			{
				Device->SetEmitter(enabled);
			}

			void StopKinect()
			{
				if(!processing) return;
				processing = false;
				OnPropertyChanged("Processing");
				providesData = false;
				OnPropertyChanged("ProvidesData");
				Device->StopKinect();				
			}
			
			KinectManager()
			{
				Device = new KinectDevice();
				deviceOpened = false;
				deviceCount = KinectDevice::GetDeviceCount();
				KinectCountChanged = gcnew KinectCountChangedHandler(this, &KinectManager::CountChangedCallback);
				Device->SetCountChangedCallback((KinectCountChangedCallback)Marshal::GetFunctionPointerForDelegate(KinectCountChanged).ToPointer());
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