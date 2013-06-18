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
		private:
			int deviceCount;
			bool deviceOpened;
		public:
			virtual event PropertyChangedEventHandler^ PropertyChanged;
			property int DeviceCount
			{
				int get()
				{
					return deviceCount;
				}
			}
			property bool DeviceOpened
			{
				bool get()
				{
					return deviceOpened;
				}
			}
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
				Device->StartKinect(KinectDevice::Modes::Depth);
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