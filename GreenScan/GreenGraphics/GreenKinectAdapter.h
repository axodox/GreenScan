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
				Color,
				Depth,
				DepthAndColor,
				Infrared
			};
		private:
			int deviceCount;
			bool deviceOpened, processing;
			Modes mode;
		public:
			property Modes Mode	{ Modes get() { return mode; }}
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			property int DeviceCount { int get() { return deviceCount; }}
			property bool DeviceOpened { bool get()	{ return deviceOpened; }}
			property bool Processing { bool get() { return processing; }}
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
				switch (mode)
				{
				case Modes::Color:
					ok = Device->StartKinect(KinectDevice::Color);
					break;
				case Modes::Depth:
					ok = Device->StartKinect(KinectDevice::Depth);
					break;
				case Modes::DepthAndColor:
					ok = Device->StartKinect(KinectDevice::DepthAndColor);
					break;
				case Modes::Infrared:
					ok = Device->StartKinect(KinectDevice::Infrared);
					break;
				}
				if(ok)
				{
					this->mode = mode;
					OnPropertyChanged("Mode");
					processing = true;
					OnPropertyChanged("Processing");
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
					bool ok = Device->OpenRaw(npath);
					LPWSTRDelete(npath);
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