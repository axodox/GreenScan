// Float4DepthStatistics.cpp : main project file.
#include "stdafx.h"
#define MAX_DEPTH 8
#define DISTANCE_HISTOGRAM_BINS 30
#define DISTANVE_DEVIATION_BINS 30
using namespace System;
using namespace MathNet::Numerics;
using namespace MathNet::Numerics::Algorithms::LinearAlgebra;
using namespace std;

#pragma unmanaged
list<WIN32_FIND_DATAW>* ListFiles(LPCWSTR path, LPCWSTR filter)
{
	WIN32_FIND_DATAW file;
	SetCurrentDirectory(path);
	HANDLE search = FindFirstFile(filter, &file);
	if (search == INVALID_HANDLE_VALUE)	return nullptr;

	list<WIN32_FIND_DATAW>* files = new list<WIN32_FIND_DATAW>();
	do
	{
		files->push_back(file);
	} while (FindNextFile(search, &file));
	FindClose(search);
	return files;
}

struct float4
{
	float X, Y, Z, W;
};

bool FL4Load(LPCWSTR path, float4* &buffer, unsigned &size)
{
	FILE* file;
	if (_wfopen_s(&file, path, L"rb"))
	{
		puts("No float4 file found!");
		return false;
	}
	unsigned width, height, newSize;
	fread(&width, sizeof(width), 1, file);
	fread(&height, sizeof(height), 1, file);
	
	newSize = width * height;
	if(size != newSize && buffer)
	{
		fclose(file);
		return false;
	}

	if(!buffer)
	{
		buffer = new float4[newSize];
		size = newSize;
	}
	
	fread(buffer, sizeof(float4), size, file);
	fclose(file);
	return true;
}

struct double3
{
	double X, Y, Z;
	double3() : X(0), Y(0), Z(0) { }
	double3(const float4* a) : X(a->X), Y(a->Y), Z(a->Z) { }
	double3(double x, double y, double z) : X(x), Y(y), Z(z) { }
	double3& operator/=(const double &a) 
	{
		X /= a;
		Y /= a;
		Z /= a;
		return *this;
	}

	double3 operator-(const double3 &a) 
	{
		return double3(X - a.X, Y - a.Y, Z - a.Z);
	}

	double3 operator+(const double3 &a) 
	{
		return double3(X + a.X, Y + a.Y, Z + a.Z);
	}

	double operator*(const double3 &a) 
	{
		return X * a.X + Y * a.Y + Z * a.Z;
	}

	double3 operator*(const double &a) 
	{
		return double3(X * a, Y * a, Z * a);
	}

	double3 operator-() 
	{
		return double3(-X, -Y, -Z);
	}
};

struct covariance3x3
{
	double X2, Y2, Z2, XY, XZ, YZ;
	covariance3x3() : X2(0), Y2(0), Z2(0), XY(0), XZ(0), YZ(0) {}
};

double3 CalculateOrigin(
	const float4* const buffer,
	unsigned size)
{
	double3 origin;
	unsigned count = 0;
	const float4* pBuffer = buffer;
	for(int i = 0; i < size; i++)
	{
		if(pBuffer->W)
		{
			origin.X += pBuffer->X;
			origin.Y += pBuffer->Y;
			origin.Z += pBuffer->Z;
			count++;
		}
		pBuffer++;
	}
	origin /= count;	
	return origin;
}

covariance3x3 CalculateCovariance(
	const float4* const buffer,
	unsigned size,
	double3 origin)
{
	//Calculate covariance
	const float4* pBuffer = buffer;
	double3 pos;
	covariance3x3 cov;
	for(int i = 0; i < size; i++)
	{
		if(pBuffer->W)
		{
			pos = double3(pBuffer->X - origin.X, pBuffer->Y - origin.Y, pBuffer->Z - origin.Z);
			cov.X2 += pos.X * pos.X;
			cov.Y2 += pos.Y * pos.Y;
			cov.Z2 += pos.Z * pos.Z;
			cov.XY += pos.X * pos.Y;
			cov.XZ += pos.X * pos.Z;
			cov.YZ += pos.Y * pos.Z;
		}
		pBuffer++;
	}
	return cov;
}

void CalculateDistances(
	const float4* const buffer,
	unsigned size,
	double3 origin,
	double3 normal,
	double* &distances,
	double &min,
	double &max)
{
	if(!distances) distances = new double[size];
	min = MAX_DEPTH;
	max = 0;
	double* pDistances = distances;
	const float4* pBuffer = buffer;
	for(int i = 0; i < size; i++)
	{
		if(pBuffer->W)
		{
			*pDistances = (double3(pBuffer) - origin) * normal;
			if(min > *pDistances) min = *pDistances;
			if(max < *pDistances) max = *pDistances;
		}
		else
			*pDistances = 0;
		pBuffer++;
		pDistances++;
	}
}

unsigned* CalculateDistanceHistogram(
	const float4* const buffer,
	const double* const distances,
	unsigned size,
	double min,
	double max,
	unsigned* &bins,
	double &width,
	double &deviation)
{
	if(!bins) bins = new unsigned[DISTANCE_HISTOGRAM_BINS];
	ZeroMemory(bins, sizeof(*bins) * DISTANCE_HISTOGRAM_BINS);
	width = (max - min) / DISTANCE_HISTOGRAM_BINS;
	const double* pDistances = distances;
	unsigned bin, count;
	double sumError2 = 0;
	for(int i = 0; i < size; i++)
	{
		if(pDistances)
		{
			bin = min(max((*pDistances - min) / width, 0), DISTANCE_HISTOGRAM_BINS - 1);
			bins[bin]++;
			count++;
			sumError2 += *pDistances * *pDistances;
		}
		pDistances++;
	}
	deviation = sqrt(sumError2 / (count - 1));
	return bins;
}

void SaveFileResults(
	WIN32_FIND_DATAW sourceFile,
	const unsigned* const distanceBins,
	double min,
	double width)
{
	wchar_t path[MAX_PATH];
	wcscpy(path, sourceFile.cFileName);
	wcscat(path, L"Results.csv");

	FILE* file = nullptr;
	if (_wfopen_s(&file, path, L"w")) return;
	
	const unsigned* pDistanceBins = distanceBins;
	for(int i = 0; i < DISTANCE_HISTOGRAM_BINS; i++)
	{
		fprintf(file, "%f, %i\r\n", min + width * (i + 0.5), *pDistanceBins);
		pDistanceBins++;
	}
	fclose(file);
}

void PrepareGlobalProcessing(
	const float4* const buffer,
	unsigned size,
	double3 origin, 
	double3 normal,
	double &min,
	double &max)
{
	const float4* pBuffer = buffer;
	double3 r;
	double depth;
	for(int i = 0; i < size; i++)
	{
		if(pBuffer->W)
		{
			r = double3(pBuffer) - origin;
			depth = (origin + r - normal * (normal * r)).Z;
			if(depth < min) min = depth;
			if(depth > max) max = depth;
		}
		pBuffer++;
	}
}

struct distanceDeviationBin
{
	double Distance, Error2Sum;
	unsigned Count;
	distanceDeviationBin() : Distance(0), Error2Sum(0), Count(0) { }
	double Deviation()
	{
		return sqrt(Error2Sum / (Count - 1));
	}
};

distanceDeviationBin* CreateGlobalHistogram(
	double min,
	double max,
	double &width)
{
	distanceDeviationBin* bins = new distanceDeviationBin[DISTANVE_DEVIATION_BINS];
	width = (max - min) / DISTANVE_DEVIATION_BINS;
	for(int i = 0; i < DISTANVE_DEVIATION_BINS; i++)
	{
		bins[i].Distance = min + width * (i + 0.5);
	}
	return bins;
}

void AddToGlobalHistogram(
	const float4* const buffer,
	unsigned size,
	distanceDeviationBin* const bins,
	double min,
	double width,
	double3 origin,
	double3 normal)
{
	const float4* pBuffer = buffer;
	double3 r;
	double depth, distance;
	unsigned bin;
	for(int i = 0; i < size; i++)
	{
		if(pBuffer->W)
		{
			r = double3(pBuffer) - origin;
			distance = normal * r;
			depth = (origin + r - normal * distance).Z;
			bin = min(max((depth - min) / width, 0), DISTANVE_DEVIATION_BINS - 1);
			bins[bin].Count++;
			bins[bin].Error2Sum += distance * distance;
		}
		pBuffer++;
	}
}

void SaveGlobalResults(
	LPCWSTR path,
	distanceDeviationBin* const bins)
{
	FILE* file = nullptr;
	if (_wfopen_s(&file, path, L"w")) return;
	
	distanceDeviationBin* pBins = bins;
	for(int i = 0; i < DISTANCE_HISTOGRAM_BINS; i++)
	{
		fprintf(file, "%f, %f, %i\r\n", pBins->Distance, pBins->Deviation(), pBins->Count);
		pBins++;
	}
	fclose(file);
}

#pragma managed

double3 CalculateNormal(
	ManagedLinearAlgebraProvider^ linear,
	covariance3x3 cov)
{	
	array<double>^ matrix = gcnew array<double>(9) {
		cov.X2, cov.XY, cov.XZ,
		cov.XY, cov.Y2, cov.YZ,
		cov.XZ, cov.YZ, cov.Z2};
	array<double>^ eigenVectors = gcnew array<double>(9);
    array<Complex>^ eigenValues = gcnew array<Complex>(3);
    array<double>^  matrixD = gcnew array<double>(9);
	linear->EigenDecomp(true, 3, matrix, eigenVectors, eigenValues, matrixD);
	double3 normal = double3(eigenVectors[0], eigenVectors[1], eigenVectors[2]);
	if(normal.Y > 0) normal = -normal;
	return normal;
}

struct imageData
{
	double3 Origin, Normal;
	imageData(double3 origin, double3 normal) : Origin(origin), Normal(normal) { }
};

void ProcessImages(
	const list<WIN32_FIND_DATAW>* const files)
{
	ManagedLinearAlgebraProvider^ linear = gcnew ManagedLinearAlgebraProvider();
	float4* buffer = nullptr;
	unsigned size = 0, *distanceBins = nullptr;
	double3 origin, normal;
	covariance3x3 cov;
	double maxDistance, minDistance, *distances = nullptr, distanceBinWidth, distanceDeviation;
	double maxGlobalDistance = 0, minGlobalDistance = MAX_DEPTH;

	unsigned imageCount = files->size();
	imageData** imageProperties = new imageData*[imageCount], **pImageProperties = imageProperties;
	for(const WIN32_FIND_DATAW &file : *files)
	{
		if(FL4Load(file.cFileName, buffer, size))
		{
			origin = CalculateOrigin(buffer, size);
			cov = CalculateCovariance(buffer, size, origin);
			normal = CalculateNormal(linear, cov);

			CalculateDistances(buffer, size, origin, normal, distances, minDistance, maxDistance);
			CalculateDistanceHistogram(buffer, distances, size, minDistance, maxDistance, distanceBins, distanceBinWidth, distanceDeviation);
			SaveFileResults(file, distanceBins, minDistance, distanceBinWidth);

			PrepareGlobalProcessing(buffer, size, origin, normal, minGlobalDistance, maxGlobalDistance);
			*pImageProperties++ = new imageData(origin, normal);
		}
	}
	if(distances) delete[size] distances;
	if(distanceBins) delete[DISTANCE_HISTOGRAM_BINS] distanceBins;

	pImageProperties = imageProperties;
	double globalWidth;
	distanceDeviationBin* ddBins = CreateGlobalHistogram(minGlobalDistance, maxGlobalDistance, globalWidth);
	for(const WIN32_FIND_DATAW &file : *files)
	{
		if(FL4Load(file.cFileName, buffer, size))
		{
			origin = (*pImageProperties)->Origin;
			normal = (*pImageProperties)->Normal;
			AddToGlobalHistogram(buffer, size, ddBins, minGlobalDistance, globalWidth, origin, normal);
		}
		delete *pImageProperties;
		pImageProperties++;
	}
	if(buffer) delete[size] buffer;
	delete[imageCount] imageProperties;

	SaveGlobalResults(L"globalResults.csv", ddBins);
	delete[DISTANVE_DEVIATION_BINS] ddBins;
}


void Float4DataTest(LPCWSTR path)
{
	//Build file list
	list<WIN32_FIND_DATAW>* files = ListFiles(path, L"*.fl4");
	if (files == nullptr)
	{
		printf("No files found!\r\n");
		return;
	}

	//Process images
	ProcessImages(files);

	delete files;
}

int main(array<System::String ^> ^args)
{
	Float4DataTest(L"E:\\Teszt");

	//Console::ReadLine();
    return 0;
}
