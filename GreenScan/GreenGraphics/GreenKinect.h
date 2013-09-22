#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN  
#include <Windows.h>
#include <ShlObj.h>
#include "Helper.h"
#include <NuiApi.h>
#pragma comment(lib, "Kinect10.lib")
#include "GreenGraphicsClasses.h"

using namespace Green::Graphics;

namespace Green
{
	namespace Kinect
	{
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
			typedef void (*ArgumentlessCallback)();
			typedef void (*FrameReadyCallback)(void* data, void* obj);
			typedef void (*KinectCountChangedCallback)(int count);
			KinectCountChangedCallback KinectCountChanged;
			KinectStartingCallback KinectStarting;
			Callback KinectStopping;
			FrameReadyCallback ColorFrameReady, DepthFrameReady;
			ArgumentlessCallback KinectDisconnected;

		private:
			void* CallbackObject;			
			INuiSensor* Sensor;
			static void CALLBACK StatusChangedCallback(
				HRESULT hrStatus, const OLECHAR* instanceName, 
				const OLECHAR* uniqueDeviceName, void* pUserData)
			{
				KinectDevice* kd = (KinectDevice*)pUserData;
				if(kd->Sensor && kd->KinectDisconnected && !wcscmp(instanceName, kd->Sensor->NuiDeviceConnectionId()))
				{
					kd->KinectDisconnected();
				}	
				if(kd->KinectCountChanged)
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
			bool CapturedColor, CapturedDepth, EmitterEnabled, NearMode;
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
				device->DepthStream = nullptr;
				device->ColorStream = nullptr;
				SetEvent(device->KinectStopped);
				return 0;
			}
		public:
			void SetEmitter(bool enabled)
			{
				EmitterEnabled = enabled;
				if(Sensor) Sensor->NuiSetForceInfraredEmitterOff(!EmitterEnabled);
			}

			void SetNearMode(bool enabled)
			{
				NearMode = enabled;
				if(DepthStream)
				{
					Sensor->NuiImageStreamSetImageFrameFlags(DepthStream, NearMode ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0);
				}
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
						if(NearMode)
							Sensor->NuiImageStreamSetImageFrameFlags(DepthStream, NUI_IMAGE_FRAME_FLAG_NEAR_MODE_ENABLED);
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
			bool SaveRaw(LPWSTR path, LPWSTR metadata)
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

						unsigned metalen = wcslen(metadata) + 1;
						fwrite(&metalen, 4, 1, file);
						fwrite((char*)metadata, 2, metalen, file);
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

			void CloseFile()
			{
				if(KinectWorking) return;
				if(KinectStopping != nullptr) KinectStopping(CallbackObject);
			}

			bool Import(LPWSTR path, float* depthIntrinsics)
			{
				enum class importTypes { Unknown, Vector4, Float4 } type;
				if(KinectWorking) return false;
				if(KinectStopping != nullptr) KinectStopping(CallbackObject);
				
				wchar_t* ext = wcsrchr(path, L'.') + 1;
				type = importTypes::Unknown;
				if(wcscmp(ext, L"vector4") == 0) type = importTypes::Vector4;
				if(wcscmp(ext, L"fl4") == 0) type = importTypes::Float4;
				if(type == importTypes::Unknown) return false;

				FILE* file = _wfopen(path, L"rb");
				if(!file) return false;
		
				struct ImportConstants
				{
					XMFLOAT4X4 DepthIntrinsics;
					XMFLOAT4 Scale;
					XMFLOAT2 DepthCoeffs;
					XMINT2 DepthSize;
				} importConstants;

				int width, height;				 
				if(type == importTypes::Float4)
				{
					fread(&width, 4, 1, file);
					fread(&height, 4, 1, file);
					importConstants.Scale = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
				}
				else
				{
					width = 640;
					height = 480;
					importConstants.Scale = XMFLOAT4(1.f, -1.f, 1.f, 1.f);
				}

				XMFLOAT4* vertices = new XMFLOAT4[width * height];
				fread(vertices, 16, width * height, file);
				fclose(file);							

				importConstants.DepthIntrinsics = XMFLOAT4X4(depthIntrinsics);
				importConstants.DepthCoeffs = XMFLOAT2(0.125e-3f, 0.f);
				importConstants.DepthSize = XMINT2(DepthWidth, DepthHeight);				

				GraphicsDevice* device = new GraphicsDevice();
				Texture2D* source = new Texture2D(device, width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, vertices, width * 16);
				delete [width * height] vertices;
				
				ReadableRenderTarget2D* target = new ReadableRenderTarget2D(device, DepthWidth, DepthHeight, DXGI_FORMAT_R16_UINT);
				VertexShader* vs = new VertexShader(device, L"VectorImportVertexShader.cso");
				GeometryShader* gs = new GeometryShader(device, L"VectorImportGeometryShader.cso");
				PixelShader* ps = new PixelShader(device, L"VectorImportPixelShader.cso");
				ConstantBuffer<ImportConstants>* cb = new ConstantBuffer<ImportConstants>(device);				
				Plane* plane = new Plane(device, width, height);

				cb->Update(&importConstants);
				cb->SetForVS();
				vs->SetInputLayout(plane);
				target->SetAsRenderTarget();
				source->SetForVS();
				device->SetShaders(vs, ps, gs);
				plane->Draw();

				target->CopyToStage();
				unsigned short* depthMap = new unsigned short[DepthWidth * DepthHeight];
				target->GetData<unsigned short>(depthMap);

				delete plane;
				delete cb;
				delete ps;
				delete gs;
				delete vs;				
				delete target;
				delete source;
				delete device;

				if(KinectStarting != nullptr) 
					KinectStarting((Modes)(Modes::DepthAndColor | Modes::Virtual), CallbackObject);	

				if(DepthFrameReady != nullptr)
					DepthFrameReady(depthMap, CallbackObject);
				delete [DepthWidth * DepthHeight] depthMap;

				return true;
			}

			bool OpenRaw(LPWSTR path, Modes &mode, LPWSTR &metadata)
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

						unsigned metalen;
						fread(&metalen, 4, 1, file);
						metadata = new WCHAR[metalen];
						fread(metadata, 2, metalen, file);
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
				KinectDisconnected = nullptr;
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
				DepthStream = 0;
				ColorStream = 0;
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
