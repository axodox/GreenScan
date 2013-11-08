#pragma once
#pragma unmanaged
#include "Stdafx.h"
#include "GreenGraphicsVertexDefinitions.h"
#include <fbxsdk.h>
#define SaveBufferSize 128
using namespace std;
using namespace DirectX;

bool PNGSave(LPWSTR path, ID3D11Texture2D* texture)
{
	ID3D11Device* device;
	texture->GetDevice(&device);

	ID3D11DeviceContext* deviceContext;
	device->GetImmediateContext(&deviceContext);

	D3D11_TEXTURE2D_DESC texDesc;
	texture->GetDesc(&texDesc);
	
	Bitmap bitmap(texDesc.Width, texDesc.Height, PixelFormat32bppARGB);
	Gdiplus::Rect lockRect(0, 0, texDesc.Width, texDesc.Height);
	BitmapData bitmapData;
	bitmap.LockBits(&lockRect, Gdiplus::ImageLockMode::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);
					
	byte *target, *source;
	D3D11_MAPPED_SUBRESOURCE ms;
	Error(deviceContext->Map(texture, 0, D3D11_MAP_READ, 0, &ms));
	for(int row = 0; row < texDesc.Height; row++)
	{
		source = (byte*)ms.pData + ms.RowPitch * row + 2;
		target = (byte*)bitmapData.Scan0 + bitmapData.Stride * row;
		for(int col = 0; col < texDesc.Width; col++)
		{
			*target++ = *source--;
			*target++ = *source--;
			*target++ = *source;
			*target++ = *(source + 3);
			source += 6;
		}
	}
	deviceContext->Unmap(texture, 0);
	bitmap.UnlockBits(&bitmapData);
	deviceContext->Release();
	device->Release();

	CLSID pngClsid;
	GetEncoderClsid(L"image/png", &pngClsid);
	return bitmap.Save(path, &pngClsid, NULL) == Ok;
}

int STLCheckAndWriteTriangle(const XMFLOAT4* a, const XMFLOAT4* b, const XMFLOAT4* c, byte* &p)
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

bool STLSave(LPWSTR path, const XMFLOAT4* const sVertices, int width, int height)
{
	WCHAR filename[MAX_PATH];
	wcscpy_s(filename, path);
	wcscat_s(filename, L".stl");

	FILE* file = _wfopen(filename, L"wb");
	if(!file) return false;
	
	char header[] = "Created with GreenScan STL Export Module, Peter Major (c) 2013";
	
	fwrite(header, 1, strlen(header), file);
	fseek(file, 84, SEEK_SET);

	unsigned int i = 0u, e = 0u, bufferLength = SaveBufferSize * 100u;
    long lenPos;
    byte* sBuffer = new byte[bufferLength];

	const XMFLOAT4
		*pVertices = sVertices, 
		*lVertices = sVertices + width - 1, 
		*eVertices = sVertices + (height - 1) * width;
	const XMFLOAT4 *A, *B, *C;
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

bool FL4Save(LPWSTR path, const XMFLOAT4* const sVertices, int width, int height)
{
	WCHAR filename[MAX_PATH];
	wcscpy_s(filename, path);
	wcscat_s(filename, L".fl4");

	FILE* file = _wfopen(filename, L"wb");
	if(!file) return false;
		
	fwrite(&width, 4, 1, file);
	fwrite(&height, 4, 1, file);
	fwrite(sVertices, 16, width * height, file);
	fclose(file);

	return true;
}

int FBXCheckAndCalculateTriangle(const XMFLOAT4* const pVertices, XMFLOAT3* const pNormals, byte* const pCounts, int a, int b, int c, int &triangleCount)
{
	const XMFLOAT4 *vA = pVertices + a, *vB = pVertices + b, *vC = pVertices + c;
	XMFLOAT3 *nA = pNormals + a, *nB = pNormals + b, *nC = pNormals + c;
	byte *cA = pCounts + a, *cB = pCounts + b, *cC = pCounts + c;

	XMVECTOR A, B, C, N, NA, NB, NC;
	if (vA->w && vB->w && vC->w)
	{
		triangleCount++;
		A = XMLoadFloat4(vA);
		B = XMLoadFloat4(vB);
		C = XMLoadFloat4(vC);
		N = XMVector3Cross(B - A, C - A);
		XMVector3Normalize(N);
		NA = XMLoadFloat3(nA);
		NB = XMLoadFloat3(nB);
		NC = XMLoadFloat3(nC);
		NA += N;
		NB += N;
		NC += N;
		XMStoreFloat3(nA, NA);
		XMStoreFloat3(nB, NB);
		XMStoreFloat3(nC, NC);
		(*cA)++;
		(*cB)++;
		(*cC)++;
		return (*cA == 1) + (*cB == 1) + (*cC == 1);
	}
	else 
	{
		return 0;
	}
}

void FBXAddVertex(
	FbxVector4* const sVectors, 
	FbxVector4* &pVectors, 
	FbxLayerElementArrayTemplate<FbxVector4>* fbxNormals,	
	XMFLOAT4* vector, 
	XMFLOAT3* normal,
	XMFLOAT2* texture,
	FbxLayerElementArrayTemplate<FbxVector2>* fbxTexture,
	int *index)
{
	if(*index == -1) 
	{
		*pVectors = FbxVector4(vector->x, vector->y, vector->z);
		*index = pVectors - sVectors;
		pVectors++;
		fbxNormals->SetAt(*index, FbxVector4(normal->x, normal->y, normal->z));
		fbxTexture->SetAt(*index, FbxVector2(texture->x, texture->y));
	}
}

void FBXAddTriangle(
	FbxMesh* const mesh, 
	FbxVector4* const sVectors, 
	FbxVector4* &pVectors, 
	FbxLayerElementArrayTemplate<FbxVector4>* const fbxNormals,
	XMFLOAT4* const pVertices, 
	XMFLOAT3* const pNormals, 
	XMFLOAT2* const pTexture,
	FbxLayerElementArrayTemplate<FbxVector2>* const fbxTexture,
	int* const pIndicies, 
	int a, int b, int c)
{
	XMFLOAT4 *vA = pVertices + a, *vB = pVertices + b, *vC = pVertices + c;
	XMFLOAT3 *nA = pNormals + a, *nB = pNormals + b, *nC = pNormals + c;
	XMFLOAT2 *tA = pTexture + a, *tB = pTexture + b, *tC = pTexture + c;
	int *iA = pIndicies + a, *iB = pIndicies + b, *iC = pIndicies + c;

	if (vA->w && vB->w && vC->w)
	{
		FBXAddVertex(sVectors, pVectors, fbxNormals, vA, nA, tA, fbxTexture, iA);
		FBXAddVertex(sVectors, pVectors, fbxNormals, vB, nB, tB, fbxTexture, iB);
		FBXAddVertex(sVectors, pVectors, fbxNormals, vC, nC, tC, fbxTexture, iC);

		mesh->BeginPolygon(-1, -1, false);
		mesh->AddPolygon(*iA);
		mesh->AddPolygon(*iB);
		mesh->AddPolygon(*iC);
		mesh->EndPolygon();
	}
}

bool FBXSave(LPWSTR path, XMFLOAT4* const sVertices, int width, int height, LPWSTR wTextureFilename, LPWSTR wFormat)
{
	WCHAR wfilename[MAX_PATH];
	wcscpy_s(wfilename, path);
	wcscat_s(wfilename, L".");
	wcscat_s(wfilename, wFormat);
	char* filename = LPWSTRToLPSTR(wfilename);
	char* textureFilename = LPWSTRToLPSTR(wTextureFilename);
	char* format = LPWSTRToLPSTR(wFormat);

	//Create objects
	FbxManager* fbxManager = FbxManager::Create();	
	FbxIOSettings* fbxIOSettings = FbxIOSettings::Create(fbxManager, IOSROOT);
	FbxScene* fbxScene = FbxScene::Create(fbxManager, "");
	FbxExporter* fbxExporter = FbxExporter::Create(fbxManager, "");
	FbxNode* fbxNode = FbxNode::Create(fbxManager, "");
	FbxMesh* fbxMesh = FbxMesh::Create(fbxManager, "");

	XMFLOAT4 *pVertices, *lVertices, *eVertices = sVertices + (height - 1) * width, *qVertices = sVertices + width * height;
	XMFLOAT4 *A, *B, *C;

	//Count triangles, calculate normals and UVs
	int vertexCount = 0, triangleCount = 0, gridSize = width * height;
	XMFLOAT3 *pNormals, *sNormals = new XMFLOAT3[gridSize];
	byte *pCounts, *sCounts = new byte[gridSize];	
	XMFLOAT2 *pTexture, *sTexture = new XMFLOAT2[gridSize];
	float maxRow = height - 1, maxCol = width - 1;

	pTexture = sTexture;
	pNormals = sNormals;
	pCounts = sCounts;
	for (int row = 0; row < height; row++)
	for (int col = 0; col < width; col++)
	{
		*pNormals++ = XMFLOAT3(0.f, 0.f, 0.f);
		*pCounts++ = 0;
		*pTexture++ = XMFLOAT2(col / maxCol,  1.f - row / maxRow);
	}

	pVertices = sVertices;
	pNormals = sNormals;
	pCounts = sCounts;
	lVertices = pVertices + width - 1;
	while (pVertices < eVertices)
    {
		vertexCount += FBXCheckAndCalculateTriangle(pVertices, pNormals, pCounts, 0, 1, width, triangleCount);
		vertexCount += FBXCheckAndCalculateTriangle(pVertices, pNormals, pCounts, width + 1, width, 1, triangleCount);
		pVertices++;
		pNormals++;
		pCounts++;
        if (pVertices == lVertices)
        {
            pVertices++;
			pNormals++;
			pCounts++;
            lVertices = pVertices + width - 1;
        }
	}

	XMVECTOR N;
	for (pVertices = sVertices, pNormals = sNormals; pVertices < qVertices; pVertices++, pNormals++)
	{
		N = XMLoadFloat3(pNormals);
		XMVector3Normalize(N);
		XMStoreFloat3(pNormals, N);
	}

	delete [width * height] sCounts;

	//Build model
	FbxNode* fbxRootNode = fbxScene->GetRootNode();
	fbxRootNode->AddChild(fbxNode);
	
	fbxMesh->InitControlPoints(vertexCount);
	FbxVector4* pVectors, *fbxVectors = fbxMesh->GetControlPoints();

	FbxGeometryElementNormal* fbxGeometryElementNormal = fbxMesh->CreateElementNormal();
	fbxGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	fbxGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);
	FbxLayerElementArrayTemplate<FbxVector4>* fbxNormals = &fbxGeometryElementNormal->GetDirectArray();
	fbxNormals->SetCount(vertexCount);

	char uvName[] = "texUV";
	FbxGeometryElementUV* fbxGeometryElementUV = fbxMesh->CreateElementUV(uvName);
	fbxGeometryElementUV->SetMappingMode(FbxGeometryElement::eByControlPoint);
	fbxGeometryElementUV->SetReferenceMode(FbxGeometryElement::eDirect);
	FbxLayerElementArrayTemplate<FbxVector2>* fbxTexture = &fbxGeometryElementUV->GetDirectArray();
	fbxTexture->SetCount(vertexCount);

	int *pIndicies, *sIndicies = new int[gridSize];
	for (pVertices = sVertices, pIndicies = sIndicies; pVertices < qVertices; pVertices++, pIndicies++)
	{
		*pIndicies = -1;
	}

	pVertices = sVertices;
	pNormals = sNormals;
	pVectors = fbxVectors;
	pIndicies = sIndicies;
	pTexture = sTexture;
	lVertices = sVertices + width - 1;
	while (pVertices < eVertices)
    {
		FBXAddTriangle(fbxMesh, fbxVectors, pVectors, fbxNormals, pVertices, pNormals, pTexture, fbxTexture, pIndicies, 0, 1, width);
		FBXAddTriangle(fbxMesh, fbxVectors, pVectors, fbxNormals, pVertices, pNormals, pTexture, fbxTexture, pIndicies, width + 1, width, 1);
		pVertices++;
		pNormals++;
		pIndicies++;
		pTexture++;
        if (pVertices == lVertices)
        {
            pVertices++;
			pNormals++;
			pIndicies++;
			pTexture++;
            lVertices = pVertices + width - 1;
        }
	}

	fbxNode->SetNodeAttribute(fbxMesh);

	delete [gridSize] sNormals;	
	delete [gridSize] sIndicies;	
	delete [gridSize] sTexture;

	//Add texture
	FbxDouble3 black = FbxDouble3(0.0, 0.0, 0.0);
	FbxDouble3 white = FbxDouble3(1.0, 1.0, 1.0);

	fbxNode->SetShadingMode(FbxNode::eTextureShading);
	FbxSurfaceLambert* fbxSurface = FbxSurfaceLambert::Create(fbxManager, "Lambert Material");
	fbxSurface->Emissive.Set(black);
	fbxSurface->EmissiveFactor.Set(1.);
	fbxSurface->Ambient.Set(white);
	fbxSurface->AmbientFactor.Set(1.);
	fbxSurface->Diffuse.Set(white);
	fbxSurface->DiffuseFactor.Set(1.);
	fbxSurface->TransparencyFactor.Set(1.);
	fbxSurface->ShadingModel.Set("Lambert");
	fbxNode->AddMaterial(fbxSurface);

	FbxFileTexture* fbxFileTexture = FbxFileTexture::Create(fbxScene, "Diffuse Texture");
    fbxFileTexture->SetFileName(textureFilename);
    fbxFileTexture->SetTextureUse(FbxTexture::eStandard);
    fbxFileTexture->SetMappingType(FbxTexture::eUV);
    fbxFileTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
    fbxFileTexture->SetSwapUV(false);
    fbxFileTexture->SetTranslation(0.0, 0.0);
    fbxFileTexture->SetScale(1.0, 1.0);
    fbxFileTexture->SetRotation(0.0, 0.0);
    fbxFileTexture->UVSet.Set(FbxString(uvName));
	fbxSurface->Diffuse.ConnectSrcObject(fbxFileTexture);

	//Save
	fbxManager->SetIOSettings(fbxIOSettings);
	int formatIndex = fbxManager->GetIOPluginRegistry()->FindWriterIDByExtension(format);
	fbxExporter->Initialize(filename, formatIndex, fbxManager->GetIOSettings());
	bool ok = fbxExporter->Export(fbxScene);

	//Destroy objects
	fbxSurface->Destroy();
	fbxFileTexture->Destroy();
	fbxMesh->Destroy();
	fbxNode->Destroy();
	fbxExporter->Destroy();
	fbxScene->Destroy();
	fbxIOSettings->Destroy();
	fbxManager->Destroy();

	LPSTRDelete(filename);
	LPSTRDelete(textureFilename);
	LPSTRDelete(format);
	return ok;
}

bool FBXMeshSave(LPWSTR path, const VertexPositionNormal* const vertices, unsigned vertexCount, unsigned* indicies, unsigned indexCount, LPWSTR wFormat)
{
	WCHAR wfilename[MAX_PATH];
	wcscpy_s(wfilename, path);
	wcscat_s(wfilename, L".");
	wcscat_s(wfilename, wFormat);
	char* filename = LPWSTRToLPSTR(wfilename);
	char* format = LPWSTRToLPSTR(wFormat);

	//Create objects
	FbxManager* fbxManager = FbxManager::Create();	
	FbxIOSettings* fbxIOSettings = FbxIOSettings::Create(fbxManager, IOSROOT);
	FbxScene* fbxScene = FbxScene::Create(fbxManager, "");
	FbxExporter* fbxExporter = FbxExporter::Create(fbxManager, "");
	FbxNode* fbxNode = FbxNode::Create(fbxManager, "");
	FbxMesh* fbxMesh = FbxMesh::Create(fbxManager, "");

	//Build model
	FbxNode* fbxRootNode = fbxScene->GetRootNode();
	fbxRootNode->AddChild(fbxNode);
	
	fbxMesh->InitControlPoints(vertexCount);
	FbxVector4* fbxVectors = fbxMesh->GetControlPoints();
	const VertexPositionNormal* pVectors = vertices;

	FbxGeometryElementNormal* fbxGeometryElementNormal = fbxMesh->CreateElementNormal();
	fbxGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	fbxGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);
	FbxLayerElementArrayTemplate<FbxVector4>* fbxNormals = &fbxGeometryElementNormal->GetDirectArray();
	fbxNormals->SetCount(vertexCount);

	for(int i = 0; i < vertexCount; i++)
	{
		*fbxVectors++ = FbxVector4(pVectors->Position.x, pVectors->Position.y, pVectors->Position.z);
		fbxNormals->SetAt(i, FbxVector4(pVectors->Normal.x, pVectors->Normal.y, pVectors->Normal.z));
		pVectors++;
	}
	fbxNode->SetNodeAttribute(fbxMesh);

	int triangleCount = indexCount / 3;
	unsigned* pIndicies = indicies;
	for(int i = 0; i < triangleCount; i++)
	{
		fbxMesh->BeginPolygon(-1, -1, false);
		fbxMesh->AddPolygon(*pIndicies++);
		fbxMesh->AddPolygon(*pIndicies++);
		fbxMesh->AddPolygon(*pIndicies++);
		fbxMesh->EndPolygon();
	}

	//Save
	fbxManager->SetIOSettings(fbxIOSettings);
	int formatIndex = fbxManager->GetIOPluginRegistry()->FindWriterIDByExtension(format);
	fbxExporter->Initialize(filename, formatIndex, fbxManager->GetIOSettings());
	bool ok = fbxExporter->Export(fbxScene);

	//Destroy objects
	fbxMesh->Destroy();
	fbxNode->Destroy();
	fbxExporter->Destroy();
	fbxScene->Destroy();
	fbxIOSettings->Destroy();
	fbxManager->Destroy();

	LPSTRDelete(filename);
	LPSTRDelete(format);
	return ok;
}
