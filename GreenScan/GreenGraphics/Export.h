#pragma once
#pragma unmanaged
#include "Stdafx.h"
#define SaveBufferSize 128
using namespace std;
using namespace DirectX;

int STLCheckAndWriteTriangle(XMFLOAT4* a, XMFLOAT4* b, XMFLOAT4* c, byte* &p)
{
	XMFLOAT3* pVector = (XMFLOAT3*)p;
	XMVECTOR A, B, C, N;
	if(a->w && b->w && c->w)
	{
		A = XMLoadFloat4(a);
		B = XMLoadFloat4(b);
		C = XMLoadFloat4(c);
		N = XMVector3Cross(B - A, C - A);
		XMVector3Normalize(N);
		XMStoreFloat3(pVector++, N);
		XMStoreFloat3(pVector++, A);
		XMStoreFloat3(pVector++, B);
		XMStoreFloat3(pVector++, C);
		p += 50;
		return 1;
	}
	else 
	{
		return 0;
	}
}

bool STLSave(LPWSTR path, XMFLOAT4* const sVertices, int width, int height)
{
	WCHAR filename[MAX_PATH];
	wcscpy_s(filename, path);
	wcscat_s(filename, L".stl");

	FILE* file = _wfopen(filename, L"wb");
	if(!file) return false;
	
	char header[] = "Created with GreenScan STL Export Module, Peter Major (c) 2013";
	
	fwrite(header, 1, strlen(header), file);
	fseek(file, 80, SEEK_SET);

	unsigned int i = 0u, e = 0u, bufferLength = SaveBufferSize * 100u;
    long lenPos;
    byte* sBuffer = new byte[bufferLength];

	XMFLOAT4
		*pVertices = sVertices, 
		*lVertices = sVertices + width - 1, 
		*eVertices = sVertices + (height - 1) * width;
	XMFLOAT4 *A, *B, *C;
    byte* pBuffer = sBuffer;
    byte* eBuffer = sBuffer + bufferLength - 50;

	while (pVertices < eVertices)
    {
        A = pVertices;
        B = pVertices + 1;
        C = pVertices + width;
        i += STLCheckAndWriteTriangle(A, B, C, pBuffer);
        C = pVertices + 1;
        B = pVertices + width;
        A = pVertices + width + 1;
        i += STLCheckAndWriteTriangle(A, B, C, pBuffer);
        pVertices++;
        if (pVertices == lVertices)
        {
            pVertices++;
            lVertices = pVertices + width - 1;
        }
        if (pBuffer >= eBuffer || pVertices == eVertices)
        {
            fwrite(sBuffer, 50u, i, file);
            pBuffer = sBuffer;
            e += i;
            i = 0u;
        }
    }

	fseek(file, 80, SEEK_SET);
	fwrite(&e, sizeof(e), 1, file);
	fclose(file);

	delete [bufferLength] sBuffer;

	return true;
}