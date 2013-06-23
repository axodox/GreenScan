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
			property bool Processing { bool get()	{ return processing; }}
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
			void OpenKinect(int index)
			{
				deviceOpened = Device->OpenKinect(index);
				OnPropertyChanged("DeviceOpened");
			}

			void StartKinect(Modes mode)
			{
				StopKinect();				
				switch (mode)
				{
				case Modes::Color:
					Device->StartKinect(KinectDevice::Color);
					break;
				case Modes::Depth:
					Device->StartKinect(KinectDevice::Depth);
					break;
				case Modes::DepthAndColor:
					Device->StartKinect(KinectDevice::DepthAndColor);
					break;
				case Modes::Infrared:
					Device->StartKinect(KinectDevice::Infrared);
					break;
				}
				this->mode = mode;
				OnPropertyChanged("Mode");
				processing = true;
				OnPropertyChanged("Processing");
			}

			void StopKinect()
			{
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

			~KinectManager()
			{
				Device->StopKinect();
				delete Device;
			}

		private:

		};
	}
}