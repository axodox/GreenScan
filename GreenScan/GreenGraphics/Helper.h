#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>
#include <DirectXMath.h>
#define SafeDelete(p) { if(p) { delete (p); (p)=NULL; } }
using namespace DirectX;

void Error(HRESULT hr)
{
	if(FAILED(hr))
	{
		_com_error ce = _com_error(hr);
	    //MessageBox(NULL, ce.ErrorMessage(), L"Error", MB_OK);
		FILE* hF = nullptr;
		_wfopen_s(&hF, L"error.txt", L"a");
		
		fputws(ce.ErrorMessage(), hF);
		fclose(hF);
	}
}

void LoadFile(LPWSTR path, void* &data, int& length)
{
	FILE* hF = nullptr;
	_wfopen_s(&hF, path, L"rb");
	fseek(hF, 0, SEEK_END);
	length = ftell(hF);
	fseek(hF, 0, SEEK_SET);
	data = malloc(length);
	fread_s(data, length, length, 1, hF);
	fclose(hF);
}

XMFLOAT4X4 Invert(XMFLOAT4X4 matrix)
{
	XMFLOAT4X4 matrixInverse;
	XMMATRIX m = XMLoadFloat4x4(&matrix);
	XMMATRIX i = XMMatrixInverse(0, m);
	XMStoreFloat4x4(&matrixInverse, i);
	return matrixInverse;
}

XMFLOAT4X4 md(XMMATRIX* matrix)
{
	XMFLOAT4X4 s;
	XMStoreFloat4x4(&s, *matrix);
	return s;
}

#pragma managed
#include <vcclr.h>
LPWSTR StringToLPWSTR(System::String^ str)
{
	pin_ptr<const wchar_t> wch =  PtrToStringChars(str);
	size_t origsize = wcslen(wch) + 1;
	const size_t newsizew = origsize;
    wchar_t *wcstring = new wchar_t[newsizew];
    wcscpy_s(wcstring, newsizew, wch);
	return wcstring;
}

bool Is4x4(array<float, 2>^ array)
{
	return array->GetLength(0) == 4 && array->GetLength(1) == 4;
}

bool Is3x3(array<float, 2>^ array)
{
	return array->GetLength(0) == 3 && array->GetLength(1) == 3;
}

array<float, 2>^ To4x4(array<float, 2>^ input)
{
	array<float, 2>^ o = gcnew array<float, 2>(4, 4);
	for(int j = 0; j < 3; j++)
	for(int i = 0; i < 3; i++)
		o[j, i] = input[j, i];
	o[3, 3] = 1.f;
	return o;
}

array<float, 2>^ Expand4x4(array<float, 2>^ input)
{
	array<float, 2>^ output = gcnew array<float, 2>(4, 4);
	for(int j = 0; j < 3; j++)
	{
		for(int i = 0; i < 2; i++)
			output[j, i] = input[j, i];
		if(j != 2) output[j, 3] = input[j, 2];
	}
	output[2, 2] = input[2, 2];
	output[3, 3] = 1.f;
	return output;
}