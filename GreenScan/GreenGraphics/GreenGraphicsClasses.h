#pragma once
#pragma unmanaged
#include "stdafx.h"
#include "Helper.h"
#include "GreenGraphicsVertexDefinitions.h"
using namespace DirectX;
using namespace Gdiplus;

namespace Green
{
	namespace Graphics
	{
		class GraphicsDevice
		{
			friend class BlendState;
			friend class RasterizerState;
			friend class VertexShader;
			template <class T> friend class VertexBuffer;
			template <class T> friend class IndexBuffer;
			template <class T> friend class ConstantBuffer;
			friend class GeometryShader;
			friend class PixelShader;
			friend class SamplerState;
			friend class Texture1D;
			friend class Texture2D;
			friend class Texture3D;
			friend class RenderTarget2D;
			friend class RenderTarget3D;
			friend class RenderTarget2DGroup;
			friend class ReadableRenderTarget2D;
			friend class ReadableRenderTarget3D;
			template <class T, class U> friend class Mesh;
			friend class Quad;
			friend class Line;
			friend class Plane;
		private:
			IDXGISwapChain* SwapChain;
			ID3D11Device* Device;
			ID3D11DeviceContext* DeviceContext;
			ID3D11Texture2D *BackBufferTexture, *DepthBufferTexture;
			ID3D11RenderTargetView* RenderTargetView;
			ID3D11DepthStencilView* DepthStencilView;
			D3D11_VIEWPORT* Viewport;
			DXGI_FORMAT Format;
			int BackBufferWidth, BackBufferHeight;

			void PrepareBackBuffer()
			{
				Error(SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBufferTexture));
				Error(Device->CreateRenderTargetView(BackBufferTexture, NULL, &RenderTargetView));

				D3D11_TEXTURE2D_DESC td;
				BackBufferTexture->GetDesc(&td);
				BackBufferWidth = td.Width;
				BackBufferHeight = td.Height;

				//Create depth buffer
				ZeroMemory(&td, sizeof(td));
				td.Width = BackBufferWidth;
				td.Height = BackBufferHeight;
				td.MipLevels = 1;
				td.ArraySize = 1;
				td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				td.SampleDesc.Count = 1;
				td.SampleDesc.Quality = 0;
				td.Usage = D3D11_USAGE_DEFAULT;
				td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				td.CPUAccessFlags = 0;
				td.MiscFlags = 0;

				Error(Device->CreateTexture2D(&td, 0, &DepthBufferTexture));

				D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
				ZeroMemory(&dsvd, sizeof(dsvd));
				dsvd.Format = td.Format;
				dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Texture2D.MipSlice = 0;

				Error(Device->CreateDepthStencilView(DepthBufferTexture, &dsvd, &DepthStencilView));

				Viewport->Width = td.Width;
				Viewport->Height = td.Height;
				DeviceContext->RSSetViewports(1, Viewport);
			}
		public:
			GraphicsDevice(HWND hWnd)
			{
				DXGI_SWAP_CHAIN_DESC scd;
				ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
				scd.BufferCount = 1;
				scd.BufferDesc.Format = Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				scd.OutputWindow = hWnd;
				scd.SampleDesc.Count = 1;
				scd.Windowed = TRUE;

				Error(D3D11CreateDeviceAndSwapChain(
					NULL,
					D3D_DRIVER_TYPE_HARDWARE,
					NULL,
					NULL,
					NULL,
					NULL,
					D3D11_SDK_VERSION,
					&scd,
					&SwapChain,
					&Device,
					NULL,
					&DeviceContext));

				LPRECT clientRect = 0;
				GetClientRect(hWnd, clientRect);
				Viewport = new D3D11_VIEWPORT();
				Viewport->MaxDepth = D3D11_MAX_DEPTH;
				Viewport->MinDepth = D3D11_MIN_DEPTH;
				PrepareBackBuffer();

				DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
			}

			void SetShaders(VertexShader* vs, PixelShader* ps, GeometryShader* gs = 0);

			ID3D11DeviceContext* GetImmediateContext()
			{
				DeviceContext->AddRef();
				return DeviceContext;
			}

			float GetAspectRatio()
			{
				return Viewport->Width / Viewport->Height;
			}

			void SetAsRenderTarget()
			{
				DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
				DeviceContext->RSSetViewports(1, Viewport);
			}

			void Clear(float backgroundColor[4])
			{
				DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
				DeviceContext->ClearRenderTargetView(RenderTargetView, backgroundColor);
			}

			void Resize()
			{
				DepthStencilView->Release();
				DepthBufferTexture->Release();
				RenderTargetView->Release();
				BackBufferTexture->Release();
				Error(SwapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_UNKNOWN, 0));
				PrepareBackBuffer();
			}

			void Present()
			{
				SwapChain->Present(0, 0);
			}

			~GraphicsDevice()
			{
				SwapChain->Release();
				Device->Release();
				DeviceContext->Release();
				BackBufferTexture->Release();
				DepthBufferTexture->Release();
				RenderTargetView->Release();
				DepthStencilView->Release();
				delete Viewport;				
			}
		};

		class BlendState
		{
		private:
			GraphicsDevice* Host;
			ID3D11BlendState* State;
		public:
			enum BlendType
			{
				Opaque,
				Additive,
				AlphaBlend
			};

			BlendState(GraphicsDevice* graphicsDevice, BlendType type)
			{
				Host = graphicsDevice;

				D3D11_RENDER_TARGET_BLEND_DESC rtbd;
				ZeroMemory(&rtbd, sizeof(rtbd));
				rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				switch (type)
				{
				case BlendType::Opaque:
					rtbd.BlendEnable = false;
					rtbd.SrcBlend = D3D11_BLEND_ONE;
					rtbd.DestBlend = D3D11_BLEND_ZERO;
					rtbd.BlendOp = D3D11_BLEND_OP_ADD;
					rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
					rtbd.DestBlendAlpha = D3D11_BLEND_ZERO;
					rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
					break;
				case BlendType::Additive:
					rtbd.BlendEnable = true;
					rtbd.SrcBlend = D3D11_BLEND_ONE;
					rtbd.DestBlend = D3D11_BLEND_ONE;
					rtbd.BlendOp = D3D11_BLEND_OP_ADD;
					rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
					rtbd.DestBlendAlpha = D3D11_BLEND_ONE;
					rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
					break;
				case BlendType::AlphaBlend:
					rtbd.BlendEnable = true;
					rtbd.SrcBlend = D3D11_BLEND_ONE;
					rtbd.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
					rtbd.BlendOp = D3D11_BLEND_OP_ADD;
					rtbd.SrcBlendAlpha = D3D11_BLEND_ONE;
					rtbd.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
					rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
					break;
				}
				
				D3D11_BLEND_DESC bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.RenderTarget[0] = rtbd;

				Host->Device->CreateBlendState(&bd, &State);
			}

			void Apply()
			{
				Host->DeviceContext->OMSetBlendState(State, 0, 0xffffffff);
			}

			~BlendState()
			{
				State->Release();
			}
		};

		class RasterizerState
		{
		private:
			GraphicsDevice* Host;
			ID3D11RasterizerState* State;
		public:
			enum RasterizerType
			{
				Default,
				CullNone,
				CullClockwise,
				CullCounterClockwise,
				Wireframe
			};

			RasterizerState(GraphicsDevice* graphicsDevice, RasterizerType type)
			{
				Host = graphicsDevice;

				D3D11_RASTERIZER_DESC rd;
				ZeroMemory(&rd, sizeof(rd));
				rd.FillMode = D3D11_FILL_SOLID;
				rd.CullMode = D3D11_CULL_BACK;
				rd.FrontCounterClockwise = false;
				rd.DepthBias = 0;
				rd.SlopeScaledDepthBias = 0.f;
				rd.DepthBiasClamp = 0.f;
				rd.DepthClipEnable = true;
				rd.ScissorEnable = false;
				rd.MultisampleEnable = false;
				rd.AntialiasedLineEnable = false;
				
				switch(type)
				{
				case RasterizerType::CullNone:
					rd.CullMode = D3D11_CULL_NONE;
					break;
				case RasterizerType::CullClockwise:
					rd.CullMode = D3D11_CULL_FRONT;
					break;
				case RasterizerType::CullCounterClockwise:
					rd.CullMode = D3D11_CULL_BACK;
					break;
				case RasterizerType::Wireframe:
					rd.CullMode = D3D11_CULL_NONE;
					rd.FillMode = D3D11_FILL_WIREFRAME;
					break;
				}

				Error(Host->Device->CreateRasterizerState(&rd, &State));
			}

			void Set()
			{
				Host->DeviceContext->RSSetState(State);
			}

			~RasterizerState()
			{
				State->Release();
			}
		};

		class VertexBufferBase
		{
		protected:
			VertexDefinition* Definition;
		public:
			VertexDefinition* GetDefinition()
			{
				return Definition;
			}
		};

		class VertexShader
		{
			friend class GraphicsDevice;
		private:
			void* Source;
			int SourceLength;
			GraphicsDevice* Host;
			ID3D11InputLayout* InputLayout;

			void Apply()
			{
				Host->DeviceContext->VSSetShader(Shader, 0, 0);
				Host->DeviceContext->IASetInputLayout(InputLayout);
			}
		public:
			ID3D11VertexShader* Shader;			
			VertexShader(GraphicsDevice* graphicsDevice, LPWSTR path)
			{
				Host = graphicsDevice;
				Source = 0;
				SourceLength = 0;
				LoadFile(path, Source, SourceLength);
				InputLayout = 0;
				Error(Host->Device->CreateVertexShader(Source, SourceLength, 0, &Shader));
			}

			void SetInputLayout(VertexDefinition* vertexDefinition)
			{
				if(InputLayout != 0) InputLayout->Release();
				Error(Host->Device->CreateInputLayout(
					vertexDefinition->Description, 
					vertexDefinition->ElementCount,
					Source, SourceLength, &InputLayout));
			}
			
			~VertexShader()
			{
				Shader->Release();
				if(InputLayout != 0) InputLayout->Release();
				free(Source);
			}
		};

		template <class T>
		class VertexBuffer : public VertexBufferBase
		{
			template <class T, class U> friend class Mesh;
		private:
			GraphicsDevice* Host;
			ID3D11Buffer* Buffer;
			int Size;
		public:			
			VertexBuffer(GraphicsDevice* graphicsDevice, int size, VertexDefinition* definition, T* vertices = 0)
			{
				Host = graphicsDevice;

				D3D11_BUFFER_DESC bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.ByteWidth = sizeof(T) * size;
				bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				if(vertices == 0)
				{
					bd.Usage = D3D11_USAGE_DYNAMIC;
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					Error(Host->Device->CreateBuffer(&bd, 0, &Buffer));
				}
				else
				{
					bd.Usage = D3D11_USAGE_IMMUTABLE;
					bd.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = vertices;

					Error(Host->Device->CreateBuffer(&bd, &sd, &Buffer));
				}
				
				Definition = definition;
				Size = size;
			}

			int GetSize()
			{
				return Size;
			}

			void Load(T* vertices, int count)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Host->DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				memcpy(ms.pData, vertices, sizeof(T) * count);
				Host->DeviceContext->Unmap(Buffer, 0);
			}

			void Set()
			{
				UINT stride = sizeof(T);
				UINT offset = 0;
				Host->DeviceContext->IASetVertexBuffers(0, 1, &Buffer, &stride, &offset);
			}

			~VertexBuffer()
			{
				Buffer->Release();
			}
		};

		template <class T>
		class IndexBuffer
		{
			template <class T, class U> friend class Mesh;
		private:
			GraphicsDevice* Host;
			ID3D11Buffer* Buffer;
			int Size;
		public:			
			IndexBuffer(GraphicsDevice* graphicsDevice, int size, T* indicies = 0)
			{
				Host = graphicsDevice;

				D3D11_BUFFER_DESC bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.ByteWidth = sizeof(T) * size;
				bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
				if(indicies == 0)
				{
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					bd.Usage = D3D11_USAGE_DYNAMIC;
					Error(Host->Device->CreateBuffer(&bd, 0, &Buffer));
				}
				else
				{
					bd.Usage = D3D11_USAGE_IMMUTABLE;
					bd.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = indicies;

					Error(Host->Device->CreateBuffer(&bd, &sd, &Buffer));
				}
				Size = size;
			}

			int GetSize()
			{
				return Size;
			}

			void Load(T* indicies, int count)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Host->DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				memcpy(ms.pData, vertices, sizeof(T) * count);
				Host->DeviceContext->Unmap(Buffer, 0);
			}

			void Set()
			{
				if(sizeof(T) == 4)
					Host->DeviceContext->IASetIndexBuffer(Buffer, DXGI_FORMAT_R32_UINT, 0);
				else
					Host->DeviceContext->IASetIndexBuffer(Buffer, DXGI_FORMAT_R16_UINT, 0);
			}

			~IndexBuffer()
			{
				Buffer->Release();
			}
		};

		template <class T>
		class ConstantBuffer
		{
		private:
			GraphicsDevice* Host;
			ID3D11Buffer* Buffer;
		public:			
			ConstantBuffer(GraphicsDevice* graphicsDevice)
			{
				Host = graphicsDevice;

				D3D11_BUFFER_DESC bd;
				int size = sizeof(T);
				bd.ByteWidth = (size % 16 == 0 ? size : size + 16 - size % 16);
				bd.Usage = D3D11_USAGE_DYNAMIC;				
				bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				bd.MiscFlags = 0;
				bd.StructureByteStride = 0;
				
				Error(Host->Device->CreateBuffer(&bd, 0, &Buffer));
			}

			void Update(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Host->DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				memcpy(ms.pData, data, sizeof(T));
				Host->DeviceContext->Unmap(Buffer, 0);
			}

			void SetForVS(int slot = 0)
			{
				Host->DeviceContext->VSSetConstantBuffers(slot, 1, &Buffer);
			}

			void SetForPS(int slot = 0)
			{
				Host->DeviceContext->PSSetConstantBuffers(slot, 1, &Buffer);
			}

			void SetForCS(int slot = 0)
			{
				Host->DeviceContext->CSSetConstantBuffers(slot, 1, &Buffer);
			}

			void SetForGS(int slot = 0)
			{
				Host->DeviceContext->GSSetConstantBuffers(slot, 1, &Buffer);
			}

			~ConstantBuffer()
			{
				Buffer->Release();
			}
		};

		class GeometryShader
		{
			friend class GraphicsDevice;
		private:
			GraphicsDevice* Host;
			ID3D11GeometryShader* Shader;

			void Apply()
			{
				Host->DeviceContext->GSSetShader(Shader, 0, 0);
			}
		public:			
			GeometryShader(GraphicsDevice* graphicsDevice, LPWSTR path)
			{
				Host = graphicsDevice;

				void* source = 0;
				int sourceLength = 0;
				LoadFile(path, source, sourceLength);
				Error(Host->Device->CreateGeometryShader(source, sourceLength, 0, &Shader));
				free(source);
			}

			~GeometryShader()
			{
				Shader->Release();
			}
		};

		class PixelShader
		{
			friend class GraphicsDevice;
		private:
			GraphicsDevice* Host;
			ID3D11PixelShader* Shader;

			void Apply()
			{
				Host->DeviceContext->PSSetShader(Shader, 0, 0);
			}
		public:			
			PixelShader(GraphicsDevice* graphicsDevice, LPWSTR path)
			{
				Host = graphicsDevice;

				void* source = 0;
				int sourceLength = 0;
				LoadFile(path, source, sourceLength);
				Error(Host->Device->CreatePixelShader(source, sourceLength, 0, &Shader));
				free(source);
			}

			~PixelShader()
			{
				Shader->Release();
			}
		};

		class SamplerState
		{
		private:
			GraphicsDevice* Host;
			ID3D11SamplerState *State;
		public:
			SamplerState(GraphicsDevice* graphicsDevice, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
			{
				Host = graphicsDevice;

				D3D11_SAMPLER_DESC sd;
				ZeroMemory(&sd, sizeof(sd));
				sd.Filter = filter;
				sd.MaxAnisotropy = 0;
				sd.AddressU = addressMode;
				sd.AddressV = addressMode;
				sd.AddressW = addressMode;
				sd.MipLODBias = 0.f;
				sd.MinLOD = 0.f;
				sd.MaxLOD = D3D11_FLOAT32_MAX;
				sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
				for(int i = 0; i < 4; i++)
					sd.BorderColor[i] = 0.f;
				Error(Host->Device->CreateSamplerState(&sd, &State));
			}

			void SetForVS(int slot = 0)
			{
				Host->DeviceContext->VSSetSamplers(slot, 1, &State);
			}

			void SetForPS(int slot = 0)
			{
				Host->DeviceContext->PSSetSamplers(slot, 1, &State);
			}

			~SamplerState()
			{
				State->Release();
			}
		};

		class Texture1D
		{
		private:
			GraphicsDevice* Host;
			ID3D11Texture1D* Texture;
			ID3D11ShaderResourceView* ResourceView;
			int Length;
		public:
			Texture1D(GraphicsDevice* graphicsDevice, int length, DXGI_FORMAT format, void* data = 0, int stride = 0) : Length(length)
			{
				Host = graphicsDevice;

				D3D11_TEXTURE1D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = length;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = format;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;				
				desc.MiscFlags = 0;
				if(data == 0)
				{
					desc.Usage = D3D11_USAGE_DYNAMIC;
					desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					Error(Host->Device->CreateTexture1D(&desc, 0, &Texture));
				}
				else
				{
					desc.Usage = D3D11_USAGE_IMMUTABLE;
					desc.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = data;
					sd.SysMemPitch = stride;
					sd.SysMemSlicePitch = 0;
					Error(Host->Device->CreateTexture1D(&desc, &sd, &Texture));
				}				

				D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc;
				ZeroMemory(&textureViewDesc, sizeof(textureViewDesc));
				textureViewDesc.Format = format; 
				textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D; 
				textureViewDesc.Texture2D.MipLevels = 1; 
				textureViewDesc.Texture2D.MostDetailedMip = 0; 

				Error(Host->Device->CreateShaderResourceView(Texture, &textureViewDesc, &ResourceView));
			}

			static Texture1D* FromFile(GraphicsDevice* graphicsDevice, LPWSTR path)
			{
				Bitmap bitmap(path);
				int width = bitmap.GetWidth();
				int height = bitmap.GetHeight();
				Gdiplus::Rect lockRect(0, 0, width, height);
				BitmapData bitmapData;
				bitmap.LockBits(&lockRect, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
				Texture1D* texture = new Texture1D(graphicsDevice, width, DXGI_FORMAT_B8G8R8A8_UNORM, bitmapData.Scan0, bitmapData.Stride);
				bitmap.UnlockBits(&bitmapData);
				return texture;
			}

			template <class T> void Load(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Host->DeviceContext->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				memcpy(ms.pData, data, Length * sizeof(T));
				Host->DeviceContext->Unmap(Texture, 0);
			}

			void Load(Texture1D* source)
			{
				Host->DeviceContext->CopyResource(Texture, source->Texture);
			}

			void SetForVS(int slot = 0)
			{
				Host->DeviceContext->VSSetShaderResources(slot, 1, &ResourceView);
			}

			void SetForPS(int slot = 0)
			{
				Host->DeviceContext->PSSetShaderResources(slot, 1, &ResourceView);
			}

			~Texture1D()
			{
				Texture->Release();
				ResourceView->Release();
			}
		};

		class Texture2D
		{
			friend class RenderTarget2D;
			friend class ReadableRenderTarget2D;
		protected:
			GraphicsDevice* Host;
			ID3D11Texture2D* Texture;
			ID3D11ShaderResourceView* ResourceView;
			DXGI_FORMAT Format;
			int Width, Height;

			Texture2D(GraphicsDevice* graphicsDevice)
			{
				Host = graphicsDevice;
				Texture = Host->BackBufferTexture;
				Texture->AddRef();
				Format = graphicsDevice->Format;
				ResourceView = nullptr;
				Width = Host->BackBufferWidth;
				Height = Host->BackBufferHeight;
			}

			Texture2D(Texture2D* texture)
			{
				Host = texture->Host;
				Texture = texture->Texture;
				Texture->AddRef();
				ResourceView = texture->ResourceView;
				ResourceView->AddRef();
				Width = texture->Width;
				Height = texture->Height;
				Format = texture->Format;
			}
		public:
			Texture2D(GraphicsDevice* graphicsDevice, int width, int height, DXGI_FORMAT format, void* data = 0, int stride = 0) : Width(width), Height(height)
			{
				Host = graphicsDevice;
				Format = format;

				D3D11_TEXTURE2D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = width;
				desc.Height = height;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = format;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;				
				desc.MiscFlags = 0;
				switch ((int)data)
				{
				case -1: //Render target
					desc.Usage = D3D11_USAGE_DEFAULT;
					desc.CPUAccessFlags = 0;
					desc.BindFlags |= D3D11_BIND_RENDER_TARGET;	
					Error(Host->Device->CreateTexture2D(&desc, 0, &Texture));
					break;
				case 0: //Dynamic texture
					desc.Usage = D3D11_USAGE_DYNAMIC;
					desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					Error(Host->Device->CreateTexture2D(&desc, 0, &Texture));
					break;
				default: //Static texture
					desc.Usage = D3D11_USAGE_IMMUTABLE;
					desc.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = data;
					sd.SysMemPitch = stride;
					sd.SysMemSlicePitch = 0;
					Error(Host->Device->CreateTexture2D(&desc, &sd, &Texture));
					break;
				}			

				D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc;
				ZeroMemory(&textureViewDesc, sizeof(textureViewDesc));
				textureViewDesc.Format = format; 
				textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; 
				textureViewDesc.Texture2D.MipLevels = 1; 
				textureViewDesc.Texture2D.MostDetailedMip = 0; 

				Error(Host->Device->CreateShaderResourceView(Texture, &textureViewDesc, &ResourceView));
			}

			static Texture2D* FromFile(GraphicsDevice* graphicsDevice, LPWSTR path)
			{
				Bitmap bitmap(path);
				int width = bitmap.GetWidth();
				int height = bitmap.GetHeight();
				Gdiplus::Rect lockRect(0, 0, width, height);
				BitmapData bitmapData;
				bitmap.LockBits(&lockRect, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
				Texture2D* texture = new Texture2D(graphicsDevice, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, bitmapData.Scan0, bitmapData.Stride);
				bitmap.UnlockBits(&bitmapData);
				return texture;
			}

			int GetWidth() { return Width; }
			int GetHeight() { return Height; }
			DXGI_FORMAT GetFormat() { return Format; }

			template <class T> void Load(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Host->DeviceContext->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				memcpy(ms.pData, data, Width * Height * sizeof(T));
				Host->DeviceContext->Unmap(Texture, 0);
			}

			void Load(Texture2D* source)
			{
				Host->DeviceContext->CopyResource(Texture, source->Texture);
			}

			void Load24bit(void* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Host->DeviceContext->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				int size = Width * Height;
				byte* pTarget = (byte*)ms.pData;
				byte* pSource = (byte*)data;
				for(int i = 0; i < size; i++)
				{
					*pTarget++ = *pSource++;
					*pTarget++ = *pSource++;
					*pTarget++ = *pSource++;
					*pTarget++ = 255;
				}
				Host->DeviceContext->Unmap(Texture, 0);
			}

			void SetForVS(int slot = 0)
			{
				Host->DeviceContext->VSSetShaderResources(slot, 1, &ResourceView);
			}

			void SetForPS(int slot = 0)
			{
				Host->DeviceContext->PSSetShaderResources(slot, 1, &ResourceView);
			}

			~Texture2D()
			{
				Texture->Release();
				SafeRelease(ResourceView);
			}
		};

		class Texture3D
		{
			friend class RenderTarget3D;
			friend class ReadableRenderTarget3D;
		protected:
			GraphicsDevice* Host;
			ID3D11Texture3D* Texture;
			ID3D11ShaderResourceView* ResourceView;
			DXGI_FORMAT Format;
			int Width, Height, Depth;

			Texture3D(Texture3D* texture)
			{
				Host = texture->Host;
				Texture = texture->Texture;
				Texture->AddRef();
				ResourceView = texture->ResourceView;
				ResourceView->AddRef();
				Width = texture->Width;
				Height = texture->Height;
				Depth = texture->Depth;
				Format = texture->Format;
			}
		public:
			Texture3D(GraphicsDevice* graphicsDevice, int width, int height, int depth, DXGI_FORMAT format, void* data = 0, int stride = 0, int slicelength = 0) : Width(width), Height(height), Depth(depth)
			{
				Host = graphicsDevice;
				Format = format;

				D3D11_TEXTURE3D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = width;
				desc.Height = height;
				desc.Depth = depth;
				desc.MipLevels = 1;
				desc.Format = format;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;				
				desc.MiscFlags = 0;
				switch ((int)data)
				{
				case -1: //Render target
					desc.Usage = D3D11_USAGE_DEFAULT;
					desc.CPUAccessFlags = 0;
					desc.BindFlags |= D3D11_BIND_RENDER_TARGET;	
					Error(Host->Device->CreateTexture3D(&desc, 0, &Texture));
					break;
				case 0: //Dynamic texture
					desc.Usage = D3D11_USAGE_DYNAMIC;
					desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					Error(Host->Device->CreateTexture3D(&desc, 0, &Texture));
					break;
				default: //Static texture
					desc.Usage = D3D11_USAGE_IMMUTABLE;
					desc.CPUAccessFlags = 0;

					D3D11_SUBRESOURCE_DATA sd;
					ZeroMemory(&sd, sizeof(sd));
					sd.pSysMem = data;
					sd.SysMemPitch = stride;
					sd.SysMemSlicePitch = slicelength;
					Error(Host->Device->CreateTexture3D(&desc, &sd, &Texture));
					break;
				}			

				D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc;
				ZeroMemory(&textureViewDesc, sizeof(textureViewDesc));
				textureViewDesc.Format = format; 
				textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D; 
				textureViewDesc.Texture3D.MipLevels = 1; 
				textureViewDesc.Texture3D.MostDetailedMip = 0;

				Error(Host->Device->CreateShaderResourceView(Texture, &textureViewDesc, &ResourceView));
			}			

			int GetWidth() { return Width; }
			int GetHeight() { return Height; }
			int GetDepth() { return Depth; }
			DXGI_FORMAT GetFormat() { return Format; }

			template <class T> void Load(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Host->DeviceContext->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				memcpy(ms.pData, data, Depth * Width * Height * sizeof(T));
				Host->DeviceContext->Unmap(Texture, 0);
			}

			void Load(Texture3D* source)
			{
				Host->DeviceContext->CopyResource(Texture, source->Texture);
			}

			void SetForVS(int slot = 0)
			{
				Host->DeviceContext->VSSetShaderResources(slot, 1, &ResourceView);
			}

			void SetForPS(int slot = 0)
			{
				Host->DeviceContext->PSSetShaderResources(slot, 1, &ResourceView);
			}

			~Texture3D()
			{
				Texture->Release();
				SafeRelease(ResourceView);
			}
		};

		class RenderTarget2D : public Texture2D
		{
			friend class RenderTarget2DGroup;
		protected:
			ID3D11RenderTargetView* RenderTargetView;
			D3D11_VIEWPORT* Viewport;

			RenderTarget2D(GraphicsDevice* device) : Texture2D(device)
			{
				RenderTargetView = device->RenderTargetView;
				RenderTargetView->AddRef();
				Viewport = new D3D11_VIEWPORT(*device->Viewport);
			}

			RenderTarget2D(RenderTarget2D* target) : Texture2D(target)
			{
				RenderTargetView = target->RenderTargetView;
				RenderTargetView->AddRef();
				Viewport = new D3D11_VIEWPORT(*target->Viewport);
			}
		public:
			RenderTarget2D(GraphicsDevice* graphicsDevice, int width, int height, DXGI_FORMAT format) 
				: Texture2D(graphicsDevice, width, height, format, (void*)-1, 0)
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvd;
				ZeroMemory(&rtvd, sizeof(rtvd));
				rtvd.Format = format;
				rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				rtvd.Texture2D.MipSlice = 0;

				Error(Host->Device->CreateRenderTargetView(Texture, &rtvd, &RenderTargetView));

				Viewport = new D3D11_VIEWPORT();
				Viewport->TopLeftX = 0;
				Viewport->TopLeftY = 0;
				Viewport->Width = width;
				Viewport->Height = Height;
				Viewport->MinDepth = D3D11_MIN_DEPTH;
				Viewport->MaxDepth = D3D11_MAX_DEPTH;
			}

			void Clear()
			{
				float bg[] = {0, 0, 0, 0};
				Host->DeviceContext->ClearRenderTargetView(RenderTargetView, bg);
			}

			void SetAsRenderTarget()
			{
				Host->DeviceContext->OMSetRenderTargets(1, &RenderTargetView, 0);
				Host->DeviceContext->RSSetViewports(1, Viewport);
			}

			~RenderTarget2D()
			{
				RenderTargetView->Release();
				delete Viewport;
			}
		};

		class RenderTarget3D : public Texture3D
		{
		protected:
			ID3D11RenderTargetView* RenderTargetView;
			D3D11_VIEWPORT* Viewport;

			RenderTarget3D(RenderTarget3D* target) : Texture3D(target)
			{
				RenderTargetView = target->RenderTargetView;
				RenderTargetView->AddRef();
				Viewport = new D3D11_VIEWPORT(*target->Viewport);
			}
		public:
			RenderTarget3D(GraphicsDevice* graphicsDevice, int width, int height, int depth, DXGI_FORMAT format) 
				: Texture3D(graphicsDevice, width, height, depth, format, (void*)-1, 0)
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvd;
				ZeroMemory(&rtvd, sizeof(rtvd));
				rtvd.Format = format;
				rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
				rtvd.Texture3D.MipSlice = 0;
				rtvd.Texture3D.FirstWSlice = 0;
				rtvd.Texture3D.WSize = -1;

				Error(Host->Device->CreateRenderTargetView(Texture, &rtvd, &RenderTargetView));

				Viewport = new D3D11_VIEWPORT();
				Viewport->TopLeftX = 0;
				Viewport->TopLeftY = 0;
				Viewport->Width = width;
				Viewport->Height = Height;
				Viewport->MinDepth = D3D11_MIN_DEPTH;
				Viewport->MaxDepth = D3D11_MAX_DEPTH;
			}

			void Clear()
			{
				float bg[] = {0, 0, 0, 0};
				Host->DeviceContext->ClearRenderTargetView(RenderTargetView, bg);
			}

			void SetAsRenderTarget()
			{
				Host->DeviceContext->OMSetRenderTargets(1, &RenderTargetView, 0);
				Host->DeviceContext->RSSetViewports(1, Viewport);
			}

			~RenderTarget3D()
			{
				RenderTargetView->Release();
				delete Viewport;
			}
		};

		class RenderTarget2DGroup
		{
		private:
			GraphicsDevice* Host;
			ID3D11RenderTargetView** RenderTargetViews;
			D3D11_VIEWPORT* ViewPorts;
			int Count;
		public:
			RenderTarget2DGroup(GraphicsDevice* device, int count, RenderTarget2D* targets[])
			{
				Host = device;
				Count = count;
				RenderTargetViews = new ID3D11RenderTargetView*[count];
				ViewPorts = new D3D11_VIEWPORT[count];
				for(int i = 0; i < count; i++)
				{
					RenderTargetViews[i] = targets[i]->RenderTargetView;
					ViewPorts[i] = *targets[i]->Viewport;
				}
			}

			void Clear()
			{
				float color[] =  {0.f, 0.f, 0.f, 0.f};
				for(int i = 0; i < Count; i++)
				{
					Host->DeviceContext->ClearRenderTargetView(RenderTargetViews[i], color);
				}
			}

			void SetRenderTargets()
			{
				Host->DeviceContext->OMSetRenderTargets(Count, RenderTargetViews, 0);
				Host->DeviceContext->RSSetViewports(Count, ViewPorts);
			}

			~RenderTarget2DGroup()
			{
				delete [Count] ViewPorts;
				delete [Count] RenderTargetViews;
			}
		};

		class RenderTargetPair
		{
		private:
			byte Tick;
			RenderTarget2D **Targets;
		public:
			RenderTargetPair(GraphicsDevice* graphicsDevice, int width, int height, DXGI_FORMAT format)
			{
				Targets = new RenderTarget2D*[2];
				for(int i = 0; i < 2; i++)
					Targets[i] = new RenderTarget2D(graphicsDevice, width, height, format);
				Tick = 0;
			}

			void Clear()
			{
				for(int i = 0; i < 2; i++)
					Targets[i]->Clear();
			}

			void SetAsRenderTarget()
			{
				Targets[Tick]->SetAsRenderTarget();
			}

			void SetForPS(int slot = 0)
			{
				Targets[(Tick == 0 ? 1 : 0)]->SetForPS(slot);
			}

			void SetForVS(int slot = 0)
			{
				Targets[(Tick == 0 ? 1 : 0)]->SetForVS(slot);
			}

			void Swap()
			{
				Tick++;
				if(Tick == 2) Tick = 0;
			}

			~RenderTargetPair()
			{
				for(int i = 0; i < 2; i++)
					delete Targets[i];
				delete [2] Targets;
			}
		};

		class ReadableRenderTarget2D : public RenderTarget2D
		{
		private:
			ID3D11Texture2D* StagingTexture;
			void CreateStagingTexture(int width, int height, DXGI_FORMAT format)
			{
				D3D11_TEXTURE2D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = width;
				desc.Height = height;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = format;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.BindFlags = 0;				
				desc.MiscFlags = 0;
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				Error(Host->Device->CreateTexture2D(&desc, 0, &StagingTexture));
			}
		public:
			ReadableRenderTarget2D(GraphicsDevice* device)
				: RenderTarget2D(device)
			{
				CreateStagingTexture(Width, Height, device->Format);
			}

			ReadableRenderTarget2D(RenderTarget2D* target)
				: RenderTarget2D(target)
			{
				CreateStagingTexture(Width, Height, target->Format);
			}

			ReadableRenderTarget2D(GraphicsDevice* graphicsDevice, int width, int height, DXGI_FORMAT format) 
				: RenderTarget2D(graphicsDevice, width, height, format)
			{
				CreateStagingTexture(width, height, format);
			}

			void CopyToStage()
			{
				Host->DeviceContext->OMSetRenderTargets(0, 0, 0);
				Host->DeviceContext->CopyResource(StagingTexture, Texture);
				Host->DeviceContext->Flush();
			}

			ID3D11Texture2D* GetStagingTexture()
			{
				return StagingTexture;
			}

			template <class T> void GetData(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				ZeroMemory(&ms, sizeof(ms));
				Error(Host->DeviceContext->Map(StagingTexture, 0, D3D11_MAP_READ, 0, &ms));
				for(int row = 0; row < Height; row++)
					memcpy(data + row * Width, (byte*)ms.pData + row * ms.RowPitch, Width * sizeof(T));
				Host->DeviceContext->Unmap(StagingTexture, 0);
			}

			~ReadableRenderTarget2D()
			{
				StagingTexture->Release();
			}
		};

		class ReadableRenderTarget3D : public RenderTarget3D
		{
		private:
			ID3D11Texture3D* StagingTexture;
			void CreateStagingTexture(int width, int height, int depth, DXGI_FORMAT format)
			{
				D3D11_TEXTURE3D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = width;
				desc.Height = height;
				desc.Depth = depth;
				desc.MipLevels = 1;
				desc.Format = format;
				desc.BindFlags = 0;				
				desc.MiscFlags = 0;
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				Error(Host->Device->CreateTexture3D(&desc, 0, &StagingTexture));
			}
		public:
			ReadableRenderTarget3D(RenderTarget3D* target)
				: RenderTarget3D(target)
			{
				CreateStagingTexture(Width, Height, Depth, target->Format);
			}

			ReadableRenderTarget3D(GraphicsDevice* graphicsDevice, int width, int height, int depth, DXGI_FORMAT format) 
				: RenderTarget3D(graphicsDevice, width, height, depth, format)
			{
				CreateStagingTexture(width, height, depth, format);
			}

			void CopyToStage()
			{
				Host->DeviceContext->OMSetRenderTargets(0, 0, 0);
				Host->DeviceContext->CopyResource(StagingTexture, Texture);
				Host->DeviceContext->Flush();
			}

			ID3D11Texture3D* GetStagingTexture()
			{
				return StagingTexture;
			}

			template <class T> void GetData(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Host->DeviceContext->Map(StagingTexture, 0, D3D11_MAP_READ, 0, &ms));
				int sliceSize = Width * Height, slRam, slVram;
				for(int slice = 0; slice < Depth; slice++)
				{
					slRam = slice * sliceSize;
					slVram = slice * ms.DepthPitch;
					for(int row = 0; row < Height; row++)
						memcpy(data + row * Width + slRam, (byte*)ms.pData + row * ms.RowPitch + slVram, Width * sizeof(T));
				}
				Host->DeviceContext->Unmap(StagingTexture, 0);
			}

			~ReadableRenderTarget3D()
			{
				StagingTexture->Release();
			}
		};

		class DepthBuffer
		{
		private:
			ID3D11Texture2D* Texture;
			ID3D11DepthStencilView* DepthStencilView;
			ID3D11DeviceContext* DeviceContext;
			ID3D11RenderTargetView* RenderTargetView;
		public:
			DepthBuffer(ID3D11RenderTargetView* target)
			{
				target->AddRef();
				ID3D11Device* device;
				target->GetDevice(&device);
				RenderTargetView = target;

				ID3D11Resource* resource;
				target->GetResource(&resource);
				ID3D11Texture2D* tex2D = (ID3D11Texture2D*)resource;
				D3D11_TEXTURE2D_DESC tex2Dd;
				tex2D->GetDesc(&tex2Dd);
				resource->Release();

				D3D11_TEXTURE2D_DESC td;
				ZeroMemory(&td, sizeof(td));
				td.Width = tex2Dd.Width;
				td.Height = tex2Dd.Height;
				td.MipLevels = 1;
				td.ArraySize = 1;
				td.Format = DXGI_FORMAT_D32_FLOAT;
				td.SampleDesc.Count = 1;
				td.SampleDesc.Quality = 0;
				td.Usage = D3D11_USAGE_DEFAULT;
				td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
				td.CPUAccessFlags = 0;
				td.MiscFlags = 0;
				
				Error(device->CreateTexture2D(&td, 0, &Texture));

				D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
				ZeroMemory(&dsvd, sizeof(dsvd));
				dsvd.Format = td.Format;
				dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Texture2D.MipSlice = 0;

				Error(device->CreateDepthStencilView(Texture, &dsvd, &DepthStencilView));
				
				device->GetImmediateContext(&DeviceContext);	
				device->Release();
			}

			void Set()
			{
				DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
			}

			void Clear()
			{
				DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
			}

			~DepthBuffer()
			{
				DeviceContext->Release();
				DepthStencilView->Release();
				Texture->Release();
				RenderTargetView->Release();
			}
		};

		class Texture2DDoubleBuffer
		{
		private:
			int TextureCount, TextureInRead, TextureInWrite, FrontTextureIndex;
			int Width, Height;
			Texture2D** Textures;
		public:			
			Texture2DDoubleBuffer(GraphicsDevice* graphicsDevice, int width, int height, DXGI_FORMAT format, int size = 2) :
				TextureCount(size), Width(width), Height(height), TextureInRead(-1), TextureInWrite(-1), 
				FrontTextureIndex(-1)
			{
				Textures = new Texture2D*[size];
				for(int i = 0; i < size; i++)
					Textures[i] = new Texture2D(graphicsDevice, width, height, format);
			}

			int GetSlotCount()
			{
				return TextureCount;
			}

			template <class T> void Load(T* data)
			{
				if(TextureInWrite != -1) return;
				int id = FrontTextureIndex;
                do
                {
                    id = (id + 1) % TextureCount;
                }
                while (id == TextureInRead);
                TextureInWrite = id;
				Textures[TextureInWrite]->Load<T>(data);
                FrontTextureIndex = TextureInWrite;
                TextureInWrite = -1;
			}

			Texture2D* BeginTextureUse(int id)
			{
				if (id == TextureInWrite || TextureInRead != -1 || id < 0 || id > TextureCount)
					return 0;
				TextureInRead = id;
				return Textures[id];
			}

			void EndTextureUse()
			{
				TextureInRead = -1;
			}

			Texture2D* GetFrontTexture()
			{
				if (FrontTextureIndex == -1 || TextureInRead != -1 || TextureInWrite == FrontTextureIndex)
					return 0;
				TextureInRead = FrontTextureIndex;
				return Textures[FrontTextureIndex];
			}

			void SetForVS(int slot = 0)
			{
				if(FrontTextureIndex != -1 && TextureInRead == -1 && TextureInWrite != FrontTextureIndex) 
				{
					TextureInRead = FrontTextureIndex;
					Textures[FrontTextureIndex]->SetForVS(slot);
				}
			}

			void SetForPS(int slot = 0)
			{
				if(FrontTextureIndex != -1 && TextureInRead == -1 && TextureInWrite != FrontTextureIndex) 
				{
					TextureInRead = FrontTextureIndex;
					Textures[FrontTextureIndex]->SetForPS(slot);
				}
			}

			~Texture2DDoubleBuffer()
			{
				for(int i = 0; i < TextureCount; i++)
					delete Textures[i];
				delete [TextureCount] Textures;
				FrontTextureIndex = -1;
			}
		};

		template <class T, class U>
		class Mesh
		{
		private:
			GraphicsDevice* Host;
			VertexBuffer<T>* VB;
			IndexBuffer<U>* IB;
			D3D11_PRIMITIVE_TOPOLOGY Topology;
		public:
			Mesh(GraphicsDevice* graphicsDevice, T* vertices, unsigned vertexCount, VertexDefinition* definition, U* indicies, unsigned indexCount, D3D11_PRIMITIVE_TOPOLOGY topology)
			{
				Host = graphicsDevice;
				VB = new VertexBuffer<T>(graphicsDevice, vertexCount, definition, vertices);
				IB = new IndexBuffer<U>(graphicsDevice, indexCount, indicies);
				Topology = topology;
			}

			void Draw()
			{
				VB->Set();
				IB->Set();
				Host->DeviceContext->IASetPrimitiveTopology(Topology);
				Host->DeviceContext->DrawIndexed(IB->Size, 0, 0);
			}

			~Mesh()
			{
				delete VB;
				delete IB;
			}
		};

		class Quad
		{
		private:
			GraphicsDevice* Host;
			VertexBuffer<VertexPositionTexture>* VB;
		public:
			Quad(GraphicsDevice* graphicsDevice)
			{	
				Host = graphicsDevice;

				VertexPositionTexture* vertices = new VertexPositionTexture[4];
				vertices[0].Position = XMFLOAT3(-1.f, -1.f, 0.f);
				vertices[0].Texture = XMFLOAT2(0.f, 1.f);
				vertices[1].Position = XMFLOAT3(-1.f, 1.f, 0.f);
				vertices[1].Texture = XMFLOAT2(0.f, 0.f);
				vertices[2].Position = XMFLOAT3(1.f, -1.f, 0.f);
				vertices[2].Texture = XMFLOAT2(1.f, 1.f);
				vertices[3].Position = XMFLOAT3(1.f, 1.f, 0.f);
				vertices[3].Texture = XMFLOAT2(1.f, 0.f);
				VB = new VertexBuffer<VertexPositionTexture>(Host, 4, VertexDefinition::VertexPositionTexture, vertices);
				delete [4] vertices;
			}

			~Quad()
			{
				delete VB;
			}

			VertexDefinition* GetVertexDefinition()
			{
				return VB->GetDefinition();
			}

			void Draw()
			{
				VB->Set();
				Host->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				Host->DeviceContext->Draw(4, 0);
			}
		};

		class Line
		{
		private:
			GraphicsDevice* Host;
			VertexBuffer<VertexPositionTexture>* VB;
			int VertexCount;
		public:
			Line(GraphicsDevice* graphicsDevice, int width)
			{
				Host = graphicsDevice;

				float xstep = 2.f / (width - 1), xtexstep = 1.f / (width - 1), xstart = -1.f;

				VertexCount = width;
				VertexPositionTexture* vertices = new VertexPositionTexture[VertexCount];
				for (int i = 0; i < width; i++)
				{
					vertices[i] = VertexPositionTexture(
						XMFLOAT3(xstart + i * xstep, 0.f, 0.f),
						XMFLOAT2(i * xtexstep, 0.f));
				}
				
				VB = new VertexBuffer<VertexPositionTexture>(Host, width, VertexDefinition::VertexPositionTexture, vertices);
				delete [VertexCount] vertices;
			}

			VertexDefinition* GetVertexDefinition()
			{
				return VB->GetDefinition();
			}

			void Draw()
			{
				VB->Set();
				Host->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
				Host->DeviceContext->Draw(VertexCount, 0);
			}

			void DrawInstanced(int count)
			{
				VB->Set();
				Host->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
				Host->DeviceContext->DrawInstanced(VertexCount, count, 0, 0);
			}

			~Line()
			{
				delete VB;
			}
		};

		class Plane
		{
		private:
			GraphicsDevice* Host;
			VertexBuffer<VertexPositionTexture>* VB;
			IndexBuffer<unsigned int>* IB;
			int IndexCount;
		public:
			Plane(GraphicsDevice* graphicsDevice, int width, int height)
			{
				Host = graphicsDevice;

				float xstep = 2.f / (width - 1), xtexstep = 1.f / (width - 1), xstart = -1.f;
				float ystep = -2.f / (height - 1), ytexstep = 1.f / (height - 1), ystart = 1.f;

				int vertexCount = width * height;
				VertexPositionTexture* vertices = new VertexPositionTexture[vertexCount];
				for (int j = 0; j < height; j++)
				{
					for (int i = 0; i < width; i++)
					{
						vertices[j * width + i] = VertexPositionTexture(
							XMFLOAT3(xstart + i * xstep, ystart + j * ystep, 0.f),
							XMFLOAT2(i * xtexstep, j * ytexstep));
					}
				}
				VB = new VertexBuffer<VertexPositionTexture>(Host, width * height, VertexDefinition::VertexPositionTexture, vertices);
				delete [vertexCount] vertices;				

				unsigned int triangleWidth = width - 1, triangleHeight = height - 1;
				IndexCount = triangleWidth * triangleHeight * 6;
				unsigned int* indicies = new unsigned int[IndexCount];
				unsigned int triangleIndex;
				for (unsigned int j = 0; j < triangleHeight; j++)
				{
					for (unsigned int i = 0; i < triangleWidth; i++)
					{
						triangleIndex = (j * triangleWidth + i) * 6;
						indicies[triangleIndex] = j * width + i;
						indicies[triangleIndex + 1] = j * width + i + 1;
						indicies[triangleIndex + 2] = (j + 1) * width + i;
						indicies[triangleIndex + 3] = j * width + i + 1;
						indicies[triangleIndex + 4] = (j + 1) * width + i + 1;
						indicies[triangleIndex + 5] = (j + 1) * width + i;
					}
				}
				IB = new IndexBuffer<unsigned int>(Host, IndexCount, indicies);
				delete [IndexCount] indicies;
			}

			VertexDefinition* GetVertexDefinition()
			{
				return VB->GetDefinition();
			}

			void Draw()
			{
				VB->Set();
				IB->Set();
				Host->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Host->DeviceContext->DrawIndexed(IndexCount, 0, 0);
			}

			void DrawInstanced(int count)
			{
				VB->Set();
				IB->Set();
				Host->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Host->DeviceContext->DrawIndexedInstanced(IndexCount, count, 0, 0, 0);
			}

			~Plane()
			{
				delete VB;
				delete IB;
			}
		};

		void GraphicsDevice::SetShaders(VertexShader* vs, PixelShader* ps, GeometryShader* gs)
		{
			vs->Apply();
			if(gs) gs->Apply();
			else DeviceContext->GSSetShader(0, 0, 0);
			ps->Apply();
		}
	}
}