#include "stdafx.h"
#define MIN_DEPTH 0.4
#define MAX_DEPTH 8 
#define MIN_RAW_DEPTH 3200
#define MAX_RAW_DEPTH 64000
#define AGGREGATED_BINS 100
#define DEPTH_BINS 8
#define MIN_COUNT 10
#define MIN_AGGREGATED_COUNT 10
#define SAVE_VERSION 1
using namespace std;
using namespace DirectX;

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

enum Modes
{
	Depth = 1,
	Color = 2,
	DepthAndColor = 3,
	Infrared = 4,
	Virtual = 8,
	NonFlags = 7
};

bool ReadRawDepthFile(
	LPCWSTR path, 
	USHORT* &depthImage,
	unsigned &size)
{
	bool ok = false;
	FILE* file = nullptr;
	if (_wfopen_s(&file, path, L"rb") == 0)
	{
		int version;
		int colorWidth, colorHeight, depthWidth, depthHeight;
		fread(&version, sizeof(version), 1, file);
		if (version == SAVE_VERSION)
		{
			Modes mode;
			fread(&mode, sizeof(mode), 1, file);

			if (mode & (Modes::Color | Modes::Infrared))
			{
				fread(&colorWidth, sizeof(colorWidth), 1, file);
				fread(&colorHeight, sizeof(colorHeight), 1, file);
				int colorSize = colorWidth * colorHeight;
				if (mode & Modes::Color) fseek(file, colorSize * 4, SEEK_CUR);
				if (mode & Modes::Infrared) fseek(file, colorSize, SEEK_CUR);
			}

			if (mode & Modes::Depth)
			{
				fread(&depthWidth, sizeof(depthWidth), 1, file);
				fread(&depthHeight, sizeof(depthHeight), 1, file);
				int depthSize = depthWidth * depthHeight;
				if (depthImage == nullptr)
				{
					depthImage = new USHORT[depthSize];
					size = depthSize;
				}
				if (size == depthSize)
				{
					fread(depthImage, sizeof(USHORT), depthSize, file);
					ok = true;
				}
			}
		}
		fclose(file);
	}
	return ok;
}

double inline ToDepth(USHORT raw)
{
	return raw * 0.000125;
}

void CalculateAverage(
	const list<WIN32_FIND_DATAW>* const files,
	unsigned &fileCount,
	unsigned &size,
	unsigned short *&count,
	double *&average	
	)
{
	//Compute sum
	unsigned short *image = nullptr, *pImage, *pCount;
	count = nullptr;
	average = nullptr;
	double *pAverage, depth;
	fileCount = 0;
	for (const WIN32_FIND_DATAW &file : *files)
	{
		if (ReadRawDepthFile(file.cFileName, image, size))
		{
			fileCount++;

			//Build buffers
			if (!count)
			{
				count = new unsigned short[size];
				ZeroMemory(count, sizeof(unsigned short)* size);
			}
			if (!average)
			{
				average = new double[size];
				ZeroMemory(average, sizeof(double)* size);
			}

			//Perform sum
			pImage = image;
			pCount = count;
			pAverage = average;
			for (int i = 0; i < size; i++)
			{
				depth = ToDepth(*pImage);
				if (depth > MIN_DEPTH && depth < MAX_DEPTH)
				{
					*pAverage += depth;
					(*pCount)++;
				}
				pImage++;
				pCount++;
				pAverage++;
			}
		}
	}
	if (image) delete[size] image;
	if (!fileCount) return;
	
	//Compute average
	pCount = count;
	pAverage = average;
	for (int i = 0; i < size; i++)
	{
		if (*pCount > 0)
		{
			*pAverage /= *pCount;
		}
		pCount++;
		pAverage++;
	}
}

void CalculateStandardDeviation(
	const list<WIN32_FIND_DATAW>* const files,
	unsigned size,
	const unsigned short* const count,
	const double* const average,	
	double* &deviation)
{
	//Compute standard deviation sum
	unsigned short *image = nullptr, *pImage;
	const unsigned short *pCount;
	deviation = new double[size];
	double *pDeviation, depth;
	const double *pAverage;
	ZeroMemory(deviation, sizeof(double)* size);
	for (const WIN32_FIND_DATAW &file : *files)
	{
		if (ReadRawDepthFile(file.cFileName, image, size))
		{
			pImage = image;
			pAverage = average;
			pDeviation = deviation;
			for (int i = 0; i < size; i++)
			{
				depth = ToDepth(*pImage);
				if (depth > MIN_DEPTH && depth < MAX_DEPTH)
				{
					*pDeviation += pow(depth - *pAverage, 2);
				}
				pImage++;
				pAverage++;
				pDeviation++;
			}
		}
	}
	if (image) delete[size] image;

	//Compute standard deviation
	pCount = count;
	pDeviation = deviation;
	for (int i = 0; i < size; i++)
	{
		if (*pCount > 1)
		{
			*pDeviation = sqrt(*pDeviation / (*pCount - 1));
		}
		pCount++;
		pDeviation++;
	}
}

template <class T> void FindLimits(
	const T* const data, 
	unsigned length,
	T &min,
	T &max)
{
	if (!length) return;
	min = data[0];
	max = data[0];
	const T* pData = data;
	for (int i = 1; i < length; i++)
	{
		if (*pData < min)
			min = *pData;
		if (*pData > max)
			max = *pData;
		pData++;
	}
}

void CalculateAggregatedDeviations(
	unsigned size,
	const unsigned short* const count,
	const double* const average,	
	const double* const deviation,
	double *&values,
	unsigned *&bins,
	double *&aggregated
	)
{
	const unsigned short *pCount = count;
	const double *pAverage = average, *pDeviation = deviation;;
	bins = new unsigned[AGGREGATED_BINS];
	aggregated = new double[AGGREGATED_BINS];
	ZeroMemory(bins, sizeof(unsigned)* AGGREGATED_BINS);
	ZeroMemory(aggregated, sizeof(double)* AGGREGATED_BINS);
	
	double minimum, maximum;
	FindLimits(average, size, minimum, maximum);
	double width = (maximum - minimum) / AGGREGATED_BINS;
	unsigned bin;
	for (int i = 0; i < size; i++)
	{
		if (*pCount > MIN_COUNT)
		{
			bin = min(max((*pAverage - minimum) / width, 0), AGGREGATED_BINS - 1);
			bins[bin]++;
			aggregated[bin] += *pDeviation;
		}
		pCount++;
		pAverage++;
		pDeviation++;
	}

	values = new double[AGGREGATED_BINS];
	unsigned *pBins = bins;
	double *pAggregated = aggregated, *pValues = values;
	for (int i = 0; i < AGGREGATED_BINS; i++)
	{
		*pValues = minimum + (0.5 + i) * width;
		if (*pBins > 0)
		{
			*pAggregated /= *pBins;
		}
		pValues++;
		pBins++;
		pAggregated++;
	}
}

void SingleDepthPixelTest(
	const list<WIN32_FIND_DATAW>* const files,
	unsigned size,
	unsigned fileCount,
	const unsigned short* const count,
	const double* const deviation,
	double *&values,
	unsigned short *&bins)
{
	//Selecting pixel to test
	unsigned index = 0;
	double maxDeviation = 0;
	const unsigned short*pCount = count;
	const double* pDeviation = deviation;

	for (int i = 0; i < size; i++)
	{
		if (*pCount == fileCount)
		{
			if (maxDeviation < *pDeviation)
			{
				maxDeviation = *pDeviation;
				index = i;
			}
		}
		pCount++;
		pDeviation++;
	}

	//Collect data
	unsigned short *image = nullptr;
	int depthCount = 0;
	double* depths = new double[fileCount], depth;
	for (const WIN32_FIND_DATAW &file : *files)
	{
		if (ReadRawDepthFile(file.cFileName, image, size))
		{
			depth = ToDepth(image[index]);
			if (depth > MIN_DEPTH && depth < MAX_DEPTH)
			{
				depths[depthCount++] = depth;
			}
		}
	}
	delete[size] image;

	//Create histogram	
	double minimum, maximum;
	FindLimits(depths, depthCount, minimum, maximum);
	double width = (maximum - minimum) / DEPTH_BINS;
	values = new double[DEPTH_BINS];
	bins = new unsigned short[DEPTH_BINS];
	ZeroMemory(bins, sizeof(unsigned short) * DEPTH_BINS);
	unsigned short bin;
	for (int i = 0; i < depthCount; i++)
	{
		bin = min(max((depths[i] - minimum) / width, 0), DEPTH_BINS - 1);
		bins[bin]++;
	}

	for (int i = 0; i < DEPTH_BINS; i++)
	{
		values[i] = minimum + width * (0.5 + i);
	}
}

void SaveGlobalResults(
	LPCSTR path, 
	unsigned size,
	const unsigned short* const count,
	const double* const average,	
	const double* const deviation)
{
	FILE* file = nullptr;
	if (fopen_s(&file, path, "w") == 0)
	{
		const unsigned short *pCount = count;
		const double *pAverage = average, *pDeviation = deviation;
		for (int i = 0; i < size; i++)
		{
			if (*pCount > MIN_COUNT)
			{
				fprintf(file, "%f, %f\r\n", *pAverage, *pDeviation);
			}
			pCount++;
			pAverage++;
			pDeviation++;
		}
		fclose(file);
	}
}

void SaveAggregatedResults(
	LPCSTR path,
	const double* const values,
	const unsigned* const bins,
	const double* const aggregated)
{
	FILE* file = nullptr;
	if (fopen_s(&file, "aggregatedResults.csv", "w") == 0)
	{
		const unsigned *pBins = bins;
		const double *pAggregated = aggregated, *pValues = values;
		for (int i = 0; i < AGGREGATED_BINS; i++)
		{
			if (*pBins > MIN_AGGREGATED_COUNT)
			{
				fprintf(file, "%f, %f, %i\r\n", *pValues, *pAggregated, *pBins);
			}
			pValues++;
			pBins++;
			pAggregated++;
		}
		fclose(file);
	}
	
}

void SaveSingleDepthPixelTestResults(
	LPCSTR path,
	const double* const values,
	const unsigned short* const bins)
{
	FILE* file = nullptr;
	if (fopen_s(&file, path, "w") == 0)
	{
		const unsigned short *pBins = bins;
		const double *pValues = values;
		for (int i = 0; i < DEPTH_BINS; i++)
		{
			fprintf(file, "%f, %i\r\n", *pValues, *pBins);
			pValues++;
			pBins++;
		}
		fclose(file);
	}
}

void RawDataTest(LPCWSTR path)
{
	//Build file list
	list<WIN32_FIND_DATAW>* files = ListFiles(path, L"*.gsr");
	if (files == nullptr)
	{
		printf("No files found!\r\n");
		return;
	}

	//Averages
	unsigned fileCount, size;
	double* average;
	unsigned short* count;
	CalculateAverage(files, fileCount, size, count, average);

	if (!fileCount) return ;

	//Standard deviations
	double* deviation;
	CalculateStandardDeviation(files, size, count, average, deviation);

	//Agregated deviations
	unsigned *aggregatedCounts;
	double *aggregatedDeviations, *aggregatedAverages;
	CalculateAggregatedDeviations(size, count, average, deviation, aggregatedAverages, aggregatedCounts, aggregatedDeviations);

	//Single-pixel depth test
	double* depthValues;
	unsigned short* depthCounts;
	SingleDepthPixelTest(files, size, fileCount, count, deviation, depthValues, depthCounts);

	//Save
	SaveGlobalResults("globalResults.csv", size, count, average, deviation);
	SaveAggregatedResults("aggregatedResults.csv", aggregatedAverages, aggregatedCounts, aggregatedDeviations);
	SaveSingleDepthPixelTestResults("depthTestResults.csv", depthValues, depthCounts);

	//Free memory
	delete[size] count;
	delete[size] average;
	delete[size] deviation;
	delete[AGGREGATED_BINS] aggregatedCounts;
	delete[AGGREGATED_BINS] aggregatedAverages;
	delete[AGGREGATED_BINS] aggregatedDeviations;
	delete[DEPTH_BINS] depthCounts;
	delete[DEPTH_BINS] depthValues;
	delete files;
}

XMFLOAT4* FL4Load(LPCWSTR path, unsigned &size)
{
	FILE* file;
	if (_wfopen_s(&file, path, L"rb"))
	{
		puts("No float4 file found!");
		return nullptr;
	}
	unsigned width, height;
	fread(&width, sizeof(width), 1, file);
	fread(&height, sizeof(height), 1, file);
	size = width * height;

	XMFLOAT4* vertices;
	fread(vertices, sizeof(*vertices), width * height, file);
	fclose(file);
	return vertices;
}

void Float4DataTest(LPCWSTR path)
{
	//Load data
	unsigned size;
	XMFLOAT4* vertices = FL4Load(path, size);
	if (!vertices) return;

	
}

int _tmain(int argc, _TCHAR* argv[])
{
	RawDataTest(L"G:\\Axodox\\Dokumentumok\\Visual Studio 2012\\Projects\\GreenScan\\Release");

	system("PAUSE");
	return 0;
}

