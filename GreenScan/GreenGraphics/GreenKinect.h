#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN  
#include <Windows.h>
#include <ShlObj.h>
#include "Helper.h"
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
			static const int ColorHeight = 480;
			static const int DepthWidth = 640;
			static const int DepthHeight = 480;
			static const int ColorSize = ColorWidth * ColorHeight * 4;
			static const int InfraredSize = ColorWidth * ColorHeight * 2;
			static const int DepthSize = DepthWidth * DepthHeight * 2;
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
				Infrared = 4,
				Virtual = 8,
				NonFlags = 7
			};

			typedef void (*KinectStartingCallback)(Modes mode, void* obj);
			typedef void (*Callback)(void* obj);
			typedef void (*FrameReadyCallback)(void* data, void* obj);
			
		private:
			void* CallbackObject;
			KinectStartingCallback KinectStarting;
			Callback KinectStopping;
			KinectCountChangedCallback KinectCountChanged;
			FrameReadyCallback ColorFrameReady, DepthFrameReady;
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
			HANDLE KinectStopped;
			bool KinectWorking;
			Modes Mode;
			byte *DepthImage, *ColorImage;
			bool CapturedColor, CapturedDepth, EmitterEnabled;
			HANDLE CapturedAll;
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
							if(!device->CapturedColor)
							{
								if(device->Mode & Modes::Color)
									memcpy(device->ColorImage, lockedRect.pBits, ColorSize);
								if(device->Mode & Modes::Infrared)
									memcpy(device->ColorImage, lockedRect.pBits, InfraredSize);
								device->CapturedColor = true;
							}
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
							if(device->DepthFrameReady != nullptr) 
								device->DepthFrameReady(lockedRect.pBits, device->CallbackObject);
							if(!device->CapturedDepth)
							{
								memcpy(device->DepthImage, lockedRect.pBits, DepthSize);
								device->CapturedDepth = true;
							}
							texture->UnlockRect(0);
							sensor->NuiImageStreamReleaseFrame(device->DepthStream, &frame);
						}
					}

					if(device->CapturedAll != nullptr && device->CapturedColor && device->CapturedDepth)
					{
						SetEvent(device->CapturedAll);
					}
				}

				SetEvent(device->KinectStopped);
				return 0;
			}
		public:
			void SetCountChangedCallback(KinectCountChangedCallback callback)
			{
				KinectCountChanged = callback;
			}

			void SetColorFrameReadyCallback(FrameReadyCallback callback)
			{
				ColorFrameReady = callback;
			}

			void SetDepthFrameReadyCallback(FrameReadyCallback callback)
			{
				DepthFrameReady = callback;
			}

			void SetEmitter(bool enabled)
			{
				EmitterEnabled = enabled;
				if(Sensor) Sensor->NuiSetForceInfraredEmitterOff(!EmitterEnabled);
			}

			bool OpenKinect(int index)
			{
				CloseKinect();
				HRESULT hr;
				hr = NuiCreateSensorByIndex(index, &Sensor);
				if(FAILED(hr)) return false;
				hr = Sensor->NuiStatus();
				if(S_OK == hr) 
				{
					Sensor->NuiSetForceInfraredEmitterOff(!EmitterEnabled);
					return true;
				}
				Sensor->Release();
				Sensor = nullptr;
				return false;
			}

			void CloseKinect()
			{
				if(Sensor == nullptr) return;
				StopKinect();
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

			void SetKinectStoppingCallback(Callback callback)
			{
				KinectStopping = callback;
			}

			bool StartKinect(Modes mode)
			{
				if(Sensor == nullptr || KinectWorking) return false;
				try
				{
					HRESULT hr;
				
					ColorStream = 0;
					DepthStream = 0;
					DWORD initalizeFlags = 0;
					if(mode & (Modes::Color | Modes::Infrared))
						initalizeFlags |= NUI_INITIALIZE_FLAG_USES_COLOR;	
					if(mode & Modes::Depth)
						initalizeFlags |= NUI_INITIALIZE_FLAG_USES_DEPTH;
					hr = Sensor->NuiInitialize(initalizeFlags);
					if(FAILED(hr)) throw 0;
				
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
						if(FAILED(hr)) throw 0;
					}
				
					if(mode & Modes::Depth)
					{
						hr = Sensor->NuiImageStreamOpen(
							NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_640x480, 0, 2,
							NextFrameEvents[1], &DepthStream);
						if(FAILED(hr)) throw 0;
					}
				
					if(KinectStarting != nullptr) KinectStarting(mode, CallbackObject);
					Mode = mode;
					WorkerThreadOn = true;
					CreateThread(0, 0, &WorkerThread, this, 0, 0);
					KinectWorking = true;
					return true;		
				}
				catch(int i)
				{
					Sensor->NuiShutdown();
					return false;
				}
			}

			static const int SaveVersion = 1;
			bool SaveRaw(LPWSTR path)
			{
				bool ok = false;

				//Capture frames
				if(Mode & Modes::Color)
				{
					ColorImage = new byte[ColorSize];
					CapturedColor = false;
				}
				if(Mode & Modes::Infrared)
				{
					ColorImage = new byte[InfraredSize];
					CapturedColor = false;
				}
				if(Mode & Modes::Depth)
				{
					DepthImage = new byte[DepthSize];
					CapturedDepth = false;
				}
				CapturedAll = CreateEvent(0, 0, 0, 0);
				DWORD waitResult = WaitForSingleObject(CapturedAll, 1000);
				CloseHandle(CapturedAll);
				CapturedAll = nullptr;

				//Write to file
				if(waitResult == WAIT_OBJECT_0)
				{
					FILE* file = nullptr;
					if(_wfopen_s(&file, path, L"wb") == 0)
					{
						fwrite(&SaveVersion, sizeof(SaveVersion), 1, file);
						fwrite(&Mode, sizeof(Mode), 1, file);

						if(ColorImage)
						{
							fwrite(&ColorWidth, sizeof(ColorWidth), 1, file);
							fwrite(&ColorHeight, sizeof(ColorHeight), 1, file);
							if(Mode & Modes::Color)	
								fwrite(ColorImage, ColorSize, 1, file);
							if(Mode & Modes::Infrared)
								fwrite(ColorImage, InfraredSize, 1, file);
						}
				
						if(DepthImage)
						{
							fwrite(&DepthWidth, sizeof(DepthWidth), 1, file);
							fwrite(&DepthHeight, sizeof(DepthHeight), 1, file);
							fwrite(DepthImage, DepthSize, 1, file);
						}

						fclose(file);
						ok = true;
					}
				}

				//Clean up resources
				if(ColorImage)
				{
					if(Mode & Modes::Color)	
						delete [ColorSize] ColorImage;
					if(Mode & Modes::Infrared)
						delete [InfraredSize] ColorImage;
					ColorImage = nullptr;
				}

				if(DepthImage)
				{
					delete [DepthSize] DepthImage;
					DepthImage = nullptr;
				}
				
				return ok;
			}

			bool OpenRaw(LPWSTR path, Modes &mode)
			{
				if(KinectWorking) return false;
				if(KinectStopping != nullptr) KinectStopping(CallbackObject);
				bool ok = false;
				FILE* file = nullptr;
				if(_wfopen_s(&file, path, L"rb") == 0)
				{
					int version;
					int colorWidth, colorHeight, depthWidth, depthHeight; 
					fread(&version, sizeof(version), 1, file);
					if(version == SaveVersion)
					{
						fread(&mode, sizeof(mode), 1, file);
						if(KinectStarting != nullptr) 
							KinectStarting((Modes)(mode | Modes::Virtual), CallbackObject);
						
						if(mode & (Modes::Color | Modes::Infrared))
						{
							fread(&colorWidth, sizeof(colorWidth), 1, file);
							fread(&colorHeight, sizeof(colorHeight), 1, file);
							if(mode & Modes::Color)
							{
								ColorImage = new byte[ColorSize];
								fread(ColorImage, ColorSize, 1, file);
							}
							if(mode & Modes::Infrared)
							{
								ColorImage = new byte[InfraredSize];
								fread(ColorImage, InfraredSize, 1, file);
							}
							if(ColorFrameReady != nullptr)
								ColorFrameReady(ColorImage, CallbackObject);
							if(mode & Modes::Color)
								delete [ColorSize] ColorImage;
							if(mode & Modes::Infrared)
								delete [InfraredSize] ColorImage;
						}

						if(mode & Modes::Depth)
						{
							fread(&depthWidth, sizeof(depthWidth), 1, file);
							fread(&depthHeight, sizeof(depthHeight), 1, file);
							DepthImage = new byte[DepthSize];
							fread(DepthImage, DepthSize, 1, file);
							if(DepthFrameReady != nullptr)
								DepthFrameReady(DepthImage, CallbackObject);
							delete [DepthSize] DepthImage;
						}
					}
					fclose(file);
					ok = true;
				}
				return ok;
			}

			void StopKinect()
			{
				if(Sensor == nullptr || !KinectWorking) return;
				Sensor->NuiShutdown();
				WorkerThreadOn = false;
				WaitForSingleObject(KinectStopped, 1000);	
				KinectStopping(CallbackObject);
				KinectWorking = false;
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
				DepthFrameReady = nullptr;
				KinectStopping = nullptr;
				NuiSetDeviceStatusCallback(&StatusChangedCallback, this);
				KinectStopped = CreateEvent(0, 0, 0, 0);
				KinectWorking = false;
				EmitterEnabled = true;

				//Image capturing
				DepthImage = ColorImage = nullptr;
				CapturedColor = true;
				CapturedDepth = true;
				CapturedAll = 0;
			}

			~KinectDevice()
			{
				CloseKinect();
				CloseHandle(NextFrameEvents[0]);
				CloseHandle(NextFrameEvents[1]);
				CloseHandle(KinectStopped);
				delete NextFrameEvents;
			}
		};
	}
}
