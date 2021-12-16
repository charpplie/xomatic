#include "stdafx.h"
#include "ColorChartBase.h"
#include "../ImageObject.h"

//////////////////////////////////////////////////////////////////////////
// helper

static bool CheckImageFormat(const ImageObject* pImg)
{
	SPixelFormats* pFmt = CPixelFormats::GetPixelFormatInfo(pImg->GetPixelFormat());
	if (!pFmt)
		return false;

	D3DFORMAT dxFmt = pFmt->DxtNo;
	return dxFmt == D3DFMT_A8R8G8B8 || dxFmt == D3DFMT_X8R8G8B8; // || dxFmt == D3DFMT_R8G8B8;
}


static inline uint32 GetAt(uint32 x, uint32 y, void* pData, uint32 pitch)
{
	uint32* p = (uint32*) ((size_t) pData + pitch * y + x * sizeof(uint32));
	return *p;
}


static bool ScanColorChartAt(uint32 x, uint32 y, void* pData, uint32 pitch)
{
	uint32 colRef[2];
	colRef[0] = GetAt(x, y, pData, pitch);
	colRef[1] = GetAt(x+2, y, pData, pitch);

	if (colRef[0] == colRef[1])
		return false;

	int refIdx = 0;
	for (int i=0; i<CHART_IMAGE_WIDTH; i+=2)
	{
		if (colRef[refIdx] != GetAt(x+i, y, pData, pitch) && colRef[refIdx] != GetAt(x+i+1, y, pData, pitch))
			return false;
		refIdx ^= 1;
	}

	refIdx = 0;
	for (int i=0; i<CHART_IMAGE_WIDTH; i+=2)
	{
		if (colRef[refIdx] != GetAt(x+i, y+CHART_IMAGE_HEIGHT-1, pData, pitch) && colRef[refIdx] != GetAt(x+i+1, y+CHART_IMAGE_HEIGHT-1, pData, pitch))
			return false;
		refIdx ^= 1;
	}

	refIdx = 0;
	for (int i=0; i<CHART_IMAGE_HEIGHT; i+=2)
	{
		if (colRef[refIdx] != GetAt(x, y+i, pData, pitch) && colRef[refIdx] != GetAt(x, y+i+1, pData, pitch))
			return false;
		refIdx ^= 1;
	}

	refIdx = 0;
	for (int i=0; i<CHART_IMAGE_HEIGHT; i+=2)
	{
		if (colRef[refIdx] != GetAt(x+CHART_IMAGE_WIDTH-1, y+i, pData, pitch) && colRef[refIdx] != GetAt(x+CHART_IMAGE_WIDTH-1, y+i+1, pData, pitch))
			return false;
		refIdx ^= 1;
	}

	return true;
}


static bool ScanColorChart(uint32 width, uint32 height, void* pData, uint32 pitch, uint32& resx, uint32& resy)
{
	int found = 0;

	uint32 y = 0;
	while (y <= height - CHART_IMAGE_HEIGHT)
	{
		uint32 x = 0;
		while (x <= width - CHART_IMAGE_WIDTH)
		{
			if (ScanColorChartAt(x, y, pData, pitch))
			{
				++found;
				if (found == 1)
				{
					resx = x;
					resy = y;
				}
				break;
			}
			++x;
		}
		if (found > 1)
			break;
		++y;
	}

	return found == 1;
}


static bool FindColorChart(ImageObject* pImg, uint32& x, uint32& y)
{
	uint32 w = pImg->GetWidth(0);
	uint32 h = pImg->GetHeight(0);

	if (w < CHART_IMAGE_WIDTH || h < CHART_IMAGE_HEIGHT)
		return false;

	CRAWImage* pRawImg = pImg->GetCRAWImage();
	if (!pRawImg)
		return false;

	char* pData = 0;
	uint32 pitch = 0;
	if (!pRawImg->Lock(0, 0, pData, pitch))
		return false;

	bool found = ScanColorChart(w, h, pData, pitch, x, y);

	pRawImg->Unlock(0);

	return found;
}

//////////////////////////////////////////////////////////////////////////
// CColorChartBase impl

CColorChartBase::CColorChartBase()
{
}


CColorChartBase::~CColorChartBase()
{
}


bool CColorChartBase::GenerateFromInput(ImageObject* pImg)
{
	if (!pImg)
		return false;

	if (!CheckImageFormat(pImg))
	{
		RCLogError("Cannot generate color chart from input image. Input image format unsupported.");
		return false;
	}

	if (!ExtractFromImage(pImg))
		return false;

	return true;
}


bool CColorChartBase::ExtractFromImage(ImageObject* pImg)
{
	assert(pImg);
	assert(CheckImageFormat(pImg));

	uint32 x = 0, y = 0;
	if (!FindColorChart(pImg, x, y))
	{
		RCLogError("Cannot extract color chart from input image. Either the color chart wasn't detected or there is more than one chart included.");
		return false;
	}

	return ExtractFromImageAt(pImg, x, y);
}
