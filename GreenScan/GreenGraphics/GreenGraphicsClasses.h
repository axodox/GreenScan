#pragma once
#pragma unmanaged
#include "stdafx.h"
#include "Helper.h"
#include "GreenGraphicsVertexDefinitions.h"
using namespace DirectX;

namespace Green
{
	namespace Graphics
	{
		class VertexBufferBase
		{
			friend class VertexShader;
		protected:
			VertexDefinition* Definition;
		};

		class VertexShader
		{
		private:
			void* Source;
			int SourceLength;
			ID3D11Device* Device;
			ID3D11DeviceContext* Context;
			ID3D11InputLayout* InputLayout;
		public:
			ID3D11VertexShader* Shader;			
			VertexShader(ID3D11Device* device, LPWSTR path)
			{
				Source = 0;
				SourceLength = 0;
				LoadFile(path, Source, SourceLength);
				InputLayout = 0;
				Error(device->CreateVertexShader(Source, SourceLength, 0, &Shader));
				Device = device;
				device->GetImmediateContext(&Context);
			}
			void SetInputLayout(VertexBufferBase* vertexBuffer)
			{
				if(InputLayout != 0) InputLayout->Release();
				Error(Device->CreateInputLayout(
					vertexBuffer->Definition->Description, 
					vertexBuffer->Definition->ElementCount,
					Source, SourceLength, &InputLayout));
			}
			void Apply()
			{
				Context->VSSetShader(Shader, 0, 0);
				Context->IASetInputLayout(InputLayout);
			}
			~VertexShader()
			{
				Shader->Release();
				Context->Release();
				if(InputLayout != 0) InputLayout->Release();
			}
		};

		template <class T>
		class VertexBuffer : public VertexBufferBase
		{
		private:
			ID3D11DeviceContext* Context;
			ID3D11Buffer* Buffer;
		public:			
			VertexBuffer(ID3D11Device* device, int size, VertexDefinition* definition)
			{
				D3D11_BUFFER_DESC bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.Usage = D3D11_USAGE_DYNAMIC;
				bd.ByteWidth = sizeof(T) * size;
				bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				Error(device->CreateBuffer(&bd, 0, &Buffer));
				Definition = definition;
				device->GetImmediateContext(&Context);
			}

			void Load(T* vertices, int count)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Context->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
				memcpy(ms.pData, vertices, sizeof(T) * count);
				Context->Unmap(Buffer, 0);
			}

			void Set()
			{
				UINT stride = sizeof(T);
				UINT offset = 0;
				Context->IASetVertexBuffers(0, 1, &Buffer, &stride, &offset);
			}

			~VertexBuffer()
			{
				Buffer->Release();
				Context->Release();
			}
		};

		class PixelShader
		{
		private:
			ID3D11PixelShader* Shader;
			ID3D11DeviceContext* Context;
		public:
			
			PixelShader(ID3D11Device* device, LPWSTR path)
			{
				void* source = 0;
				int sourceLength = 0;
				LoadFile(path, source, sourceLength);
				Error(device->CreatePixelShader(source, sourceLength, 0, &Shader));
				free(source);
				device->GetImmediateContext(&Context);
			}
			void Apply()
			{
				Context->PSSetShader(Shader, 0, 0);
			}
			~PixelShader()
			{
				Shader->Release();
				Context->Release();
			}
		};

		class Sampler
		{
		private:
			ID3D11SamplerState *SamplerState;
			ID3D11DeviceContext *Context;
		public:
			Sampler(ID3D11Device* device, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
			{
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
				Error(device->CreateSamplerState(&sd, &SamplerState));
				device->GetImmediateContext(&Context);
			}

			void SetForPS(int slot)
			{
				Context->PSSetSamplers(slot, 1, &SamplerState);
			}

			~Sampler()
			{
				SamplerState->Release();
				Context->Release();
			}
		};

		class Texture2D
		{
		private:
			ID3D11Texture2D* Texture;
			ID3D11ShaderResourceView* ResourceView;
			ID3D11DeviceContext* Context;
			int Width, Height;
		public:
			Texture2D(ID3D11Device* device, int width, int height, DXGI_FORMAT format, D3D11_USAGE usage) : Width(width), Height(height)
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
				desc.Usage = usage;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				desc.MiscFlags = 0;

				Error(device->CreateTexture2D(&desc, 0, &Texture));

				D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDesc;
				ZeroMemory(&textureViewDesc, sizeof(textureViewDesc));
				textureViewDesc.Format = format; 
				textureViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; 
				textureViewDesc.Texture2D.MipLevels = 1; 
				textureViewDesc.Texture2D.MostDetailedMip = 0; 

				Error(device->CreateShaderResourceView(Texture, &textureViewDesc, &ResourceView));

				device->GetImmediateContext(&Context);
			}

			template <class T> void Load(T* data)
			{
				D3D11_MAPPED_SUBRESOURCE ms;
				Error(Context->Map(Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
				memcpy(ms.pData, data, Width * Height);
				Context->Unmap(Texture, 0);
			}

			void SetForPS(int slot)
			{
				Context->PSSetShaderResources(slot, 1, &ResourceView);
			}

			~Texture2D()
			{
				Texture->Release();
				ResourceView->Release();
				Context->Release();
			}
		};

		class Quad
		{
		private:
			ID3D11DeviceContext* Context;
		public:
			VertexBuffer<VertexPositionTexture>* VBuffer;
			Quad(ID3D11Device* device)
			{
				VBuffer = new VertexBuffer<VertexPositionTexture>(device, 4, VertexDefinition::VertexPositionTexture);
				
				VertexPositionTexture* vertices = new VertexPositionTexture[4];
				vertices[0].Position = XMFLOAT3(-1.f, -1.f, 0.f);
				vertices[0].Texture = XMFLOAT2(0.f, 0.f);
				vertices[1].Position = XMFLOAT3(-1.f, 1.f, 0.f);
				vertices[1].Texture = XMFLOAT2(0.f, 1.f);
				vertices[2].Position = XMFLOAT3(1.f, -1.f, 0.f);
				vertices[2].Texture = XMFLOAT2(1.f,0.f);
				vertices[3].Position = XMFLOAT3(1.f, 1.f, 0.f);
				vertices[3].Texture = XMFLOAT2(1.f, 1.f);
				VBuffer->Load(vertices, 4);
				delete [4] vertices;
				device->GetImmediateContext(&Context);
			}

			~Quad()
			{
				delete VBuffer;
				Context->Release();
			}

			void Draw()
			{
				VBuffer->Set();
				Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				Context->Draw(4, 0);
			}
		};
	}
}