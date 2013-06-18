#pragma once
#pragma unmanaged
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>

void Error(HRESULT hr)
{
	if(FAILED(hr))
	{
		_com_error ce = _com_error(hr);
		MessageBox(NULL, ce.ErrorMessage(), L"Error", MB_OK);
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