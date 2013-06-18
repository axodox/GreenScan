#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN  
#include <Windows.h>
#include <ShlObj.h>
//#include <Ole2.h>
#include <NuiApi.h>
#pragma comment(lib, "Kinect10.lib")
namespace Green
{
	namespace Kinect
	{
		typedef void (*KinectCountChangedCallback)(int count);

		class KinectDevice
		{
		public:
			static const int ColorWidth = 640;
			static const int ColorHeight = 640;
			static const int DepthWidth = 640;
			static const int DepthHeight = 640;
			static int GetDeviceCount() 
			{
				int count;
				NuiGetSensorCount(&count);
				return count;
			}

			enum Modes 
			{
				Depth = 1,
				Color = 2,
				DepthAndColor = 3,
				Infrared = 4
			};

			typedef void (*KinectStartingCallback)(Modes mode, void* obj);
			typedef void (*ColorFrameReadyCallback)(void* data, void* obj);
			
		private:
			void* CallbackObject;
			KinectStartingCallback KinectStarting;
			KinectCountChangedCallback KinectCountChanged;
			ColorFrameReadyCallback ColorFrameReady;
			INuiSensor* Sensor;
			static void CALLBACK StatusChangedCallback(
				HRESULT hrStatus, const OLECHAR* instanceName, 
				const OLECHAR* uniqueDeviceName, void* pUserData)
			{
				KinectDevice* kd = (KinectDevice*)pUserData;
				if(kd->KinectCountChanged!=0)
				{					
					kd->KinectCountChanged(GetDeviceCount());
				}
			}
			HANDLE* NextFrameEvents;
			HANDLE ColorStream, DepthStream;
			bool WorkerThreadOn;
			static DWORD WINAPI WorkerThread(LPVOID o)
			{
				HRESULT hr;
				NUI_IMAGE_FRAME frame;
				NUI_LOCKED_RECT lockedRect;
				INuiFrameTexture* texture;

				KinectDevice* device = (KinectDevice*)o;
				INuiSensor* sensor = device->Sensor;
				while(device->WorkerThreadOn)
				{
					WaitForMultipleObjects(2, device->NextFrameEvents, 0, 100);
					if(device->ColorStream != 0)
					{
						hr = sensor->NuiImageStreamGetNextFrame(device->ColorStream, 0, &frame);
						if(SUCCEEDED(hr))
						{
							texture = frame.pFrameTexture;
							texture->LockRect(0, &lockedRect, 0, 0);
							if(device->ColorFrameReady != nullptr) 
								device->ColorFrameReady(lockedRect.pBits, device->CallbackObject);
							texture->UnlockRect(0);
							sensor->NuiImageStreamReleaseFrame(device->ColorStream, &frame);
						}
					}
					if(device->DepthStream != 0)
					{
						hr = sensor->NuiImageStreamGetNextFrame(device->DepthStream, 0, &frame);
						if(SUCCEEDED(hr))
						{
							texture = frame.pFrameTexture;
							texture->LockRect(0, &lockedRect, 0, 0);
							//copy data here...
							texture->UnlockRect(0);
							sensor->NuiImageStreamReleaseFrame(device->DepthStream, &frame);
						}
					}
				}
				return 0;
			}
		public:			
			void SetCountChangedCallback(KinectCountChangedCallback callback)
			{
				KinectCountChanged = callback;
			}

			void SetFrameReadyCallback(ColorFrameReadyCallback callback)
			{
				ColorFrameReady = callback;
			}

			bool OpenKinect(int index)
			{
				CloseKinect();
				HRESULT hr;
				hr = NuiCreateSensorByIndex(index, &Sensor);
				if(FAILED(hr)) return false;
				hr = Sensor->NuiStatus();
				if(S_OK == hr) return true;
				Sensor->Release();
				Sensor = nullptr;
				return false;
			}

			void CloseKinect()
			{
				if(Sensor == nullptr) return;
				Sensor->Release();
				Sensor = nullptr;
			}

			void SetCallbackObject(void* obj)
			{
				CallbackObject = obj;
			}

			void SetKinectStartingCallback(KinectStartingCallback callback)
			{
				KinectStarting = callback;
			}

			bool StartKinect(Modes mode)
			{
				if(Sensor == nullptr) return false;
				HRESULT hr;
				
				ColorStream = 0;
				DepthStream = 0;
				DWORD initalizeFlags = 0;
				if(mode & (Modes::Color | Modes::Infrared))
					initalizeFlags |= NUI_INITIALIZE_FLAG_USES_COLOR;	
				if(mode & Modes::Depth)
					initalizeFlags |= NUI_INITIALIZE_FLAG_USES_DEPTH;
				hr = Sensor->NuiInitialize(initalizeFlags);
				if(FAILED(hr)) return false;
				
				if(mode & (Modes::Color | Modes::Infrared))
				{
					NUI_IMAGE_TYPE imageType;
					if(mode & Modes::Color)
						imageType = NUI_IMAGE_TYPE_COLOR;
					else
						imageType = NUI_IMAGE_TYPE_COLOR_INFRARED;
					
					hr = Sensor->NuiImageStreamOpen(
						imageType, NUI_IMAGE_RESOLUTION_640x480, 0, 2,
						NextFrameEvents[0], &ColorStream);
					if(FAILED(hr)) return false;
				}
				
				if(mode & Modes::Depth)
				{
					hr = Sensor->NuiImageStreamOpen(
						NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_640x480, 0, 2,
						NextFrameEvents[1], &DepthStream);
					if(FAILED(hr)) return false;
				}
				
				if(KinectStarting != nullptr) KinectStarting(mode, CallbackObject);
				WorkerThreadOn = true;
				CreateThread(0, 0, &WorkerThread, this, 0, 0);
				return true;				
			}

			void StopKinect()
			{
				if(Sensor == nullptr) return;
				WorkerThreadOn = false;
				Sensor->NuiShutdown();
			}

			KinectDevice()
			{
				Sensor = nullptr;
				NextFrameEvents = new HANDLE[2];
				NextFrameEvents[0] = CreateEvent(0, 1, 0, 0);
				NextFrameEvents[1] = CreateEvent(0, 1, 0, 0);
				KinectCountChanged = nullptr;
				KinectStarting = nullptr;
				ColorFrameReady = nullptr;
				NuiSetDeviceStatusCallback(&StatusChangedCallback, this);
			}

			~KinectDevice()
			{
				CloseHandle(NextFrameEvents[0]);
				CloseHandle(NextFrameEvents[1]);
				delete NextFrameEvents;
			}
		private:

		};
	}
}
