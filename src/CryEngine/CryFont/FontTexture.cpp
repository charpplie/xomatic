//-------------------------------------------------------------------------------------------------
// Author: Márcio Martins
//
// Purpose:
//  - Create and update a texture with the most recently used glyphs
//
// History:
//  - [6/6/2003] created the file
//
//-------------------------------------------------------------------------------------------------
#include "StdAfx.h"
#include "FontTexture.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetCharWidth
#endif

//-------------------------------------------------------------------------------------------------
CFontTexture::CFontTexture()
: m_wSlotUsage(1), m_iWidth(0), m_iHeight(0), m_iCellWidth(0), m_iCellHeight(0), m_fTextureCellWidth(0),
  m_fTextureCellHeight(0), m_iWidthCellCount(0), m_iHeightCellCount(0), m_nTextureSlotCount(0), m_pBuffer(0),
  m_iSmoothMethod(FONT_SMOOTH_NONE), m_iSmoothAmount(FONT_SMOOTH_AMOUNT_NONE)
{
}

//-------------------------------------------------------------------------------------------------
CFontTexture::~CFontTexture()
{
	Release();
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::CreateFromFile(const string &szFileName, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio, int iWidthCellCount, int iHeightCellCount)
{
	if (!m_pGlyphCache.LoadFontFromFile(szFileName))
	{
		Release();

		return 0;
	}

	if (!Create(iWidth, iHeight, iSmoothMethod, iSmoothAmount, fSizeRatio, iWidthCellCount, iHeightCellCount))
		return 0;

	return 1;
}

//------------------------------------------------------------------------------------------------- 
int CFontTexture::CreateFromMemory(unsigned char *pFileData, int iDataSize, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio, int iWidthCellCount, int iHeightCellCount)
{
	if (!m_pGlyphCache.LoadFontFromMemory(pFileData, iDataSize))
	{
		Release();

		return 0;
	}

	if (!Create(iWidth, iHeight, iSmoothMethod, iSmoothAmount, fSizeRatio, iWidthCellCount, iHeightCellCount))
		return 0;

	return 1;
}

//------------------------------------------------------------------------------------------------- 
int CFontTexture::Create(int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio, int iWidthCellCount, int iHeightCellCount)
{
#ifdef USE_VIRT_MEM
	uint32 h;
	m_pBuffer = (FONT_TEXTURE_TYPE*)NVirtualMem::AllocVirtualMem(sizeof(FONT_TEXTURE_TYPE)*iWidth * iHeight,h);
#else
	m_pBuffer = new FONT_TEXTURE_TYPE[iWidth * iHeight];
#endif
	if (!m_pBuffer)
		return 0;

	memset(m_pBuffer, 0, iWidth * iHeight * sizeof(FONT_TEXTURE_TYPE));

	if (!(iWidthCellCount * iHeightCellCount))
	{
		return 0;
	}

	m_iWidth = iWidth;
	m_iHeight = iHeight;

	m_iWidthCellCount = iWidthCellCount;
	m_iHeightCellCount = iHeightCellCount;
	m_nTextureSlotCount = m_iWidthCellCount * m_iHeightCellCount;

	m_iSmoothMethod = iSmoothMethod;
	m_iSmoothAmount = iSmoothAmount;

	m_iCellWidth = m_iWidth / m_iWidthCellCount;
	m_iCellHeight = m_iHeight / m_iHeightCellCount;

	m_fTextureCellWidth = m_iCellWidth / (float)m_iWidth;
	m_fTextureCellHeight = m_iCellHeight / (float)m_iHeight;

	if (!m_pGlyphCache.Create(FONT_GLYPH_CACHE_SIZE, m_iCellWidth, m_iCellHeight, iSmoothMethod, iSmoothAmount, fSizeRatio))
	{
		Release();

		return 0;
	}

	if (!CreateSlotList(m_nTextureSlotCount))
	{
		Release();

		return 0;
	}

	return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::Release()
{
#ifdef USE_VIRT_MEM
	NVirtualMem::FreeVirtualMem(m_pBuffer);
#else
	delete[] m_pBuffer;
#endif
	m_pBuffer = 0;

	ReleaseSlotList();

	m_pSlotTable.clear();

	m_pGlyphCache.Release();

	m_iWidthCellCount = 0;
	m_iHeightCellCount = 0;
	m_nTextureSlotCount = 0;

	m_iWidth = 0;
	m_iHeight = 0;

	m_iCellWidth = 0;
	m_iCellHeight = 0;

	m_iSmoothMethod = 0;
	m_iSmoothAmount = 0;

	m_fTextureCellWidth = 0.0f;
	m_fTextureCellHeight = 0.0f;

	m_wSlotUsage = 1;

	return 1;
}

//-------------------------------------------------------------------------------------------------
wchar_t CFontTexture::GetSlotChar(int iSlot) const
{
	return m_pSlotList[iSlot]->cCurrentChar;
}

//-------------------------------------------------------------------------------------------------
CTextureSlot *CFontTexture::GetCharSlot(wchar_t cChar)
{
	CTextureSlotTableItor pItor = m_pSlotTable.find(cChar);

	if (pItor != m_pSlotTable.end())
		return pItor->second;

	return 0;
}

//-------------------------------------------------------------------------------------------------
CTextureSlot *CFontTexture::GetLRUSlot()
{
	uint16 wMinSlotUsage = 0xffff;
	CTextureSlot	*pLRUSlot = 0;
	CTextureSlot	*pSlot;

	CTextureSlotListItor pItor = m_pSlotList.begin();

	while (pItor != m_pSlotList.end())
	{
		pSlot = *pItor;

		if (pSlot->wSlotUsage == 0)
		{
			return pSlot;
		}
		else
		{
			if (pSlot->wSlotUsage < wMinSlotUsage)
			{
				pLRUSlot = pSlot;
				wMinSlotUsage = pSlot->wSlotUsage;
			}
		}

		pItor++;
	}

	return pLRUSlot;
}

//-------------------------------------------------------------------------------------------------
CTextureSlot *CFontTexture::GetMRUSlot()
{
	uint16 wMaxSlotUsage = 0;
	CTextureSlot *pMRUSlot = 0;
	CTextureSlot *pSlot;

	CTextureSlotListItor pItor = m_pSlotList.begin();

	while (pItor != m_pSlotList.end())
	{
		pSlot = *pItor;

		if (pSlot->wSlotUsage != 0)
		{
			if (pSlot->wSlotUsage > wMaxSlotUsage)
			{
				pMRUSlot = pSlot;
				wMaxSlotUsage = pSlot->wSlotUsage;
			}
		}

		pItor++;
	}

	return pMRUSlot;
}

//------------------------------------------------------------------------------------------------- 
int CFontTexture::PreCacheString(const wchar_t *szString, int *pUpdated)
{
	uint16 wSlotUsage = m_wSlotUsage++;
	int iLength = wcslen(szString);
	int iUpdated = 0;

	for (int i = 0; i < iLength; i++)
	{
		wchar_t cChar = szString[i];

		CTextureSlot *pSlot = GetCharSlot(cChar);

		if (!pSlot)
		{
			pSlot = GetLRUSlot();

			if (!pSlot)
				return 0;

			if (!UpdateSlot(pSlot->iTextureSlot, wSlotUsage, cChar))
				return 0;

			++iUpdated;
		}
		else
		{
			pSlot->wSlotUsage = wSlotUsage;
		}
	}

	if (pUpdated)
		*pUpdated = iUpdated;

	if (iUpdated)
		return 1;

	return 2;
}

//------------------------------------------------------------------------------------------------- 
void CFontTexture::GetTextureCoord( CTextureSlot *pSlot, float &fMinU, float &fMinV, float &fMaxU, float &fMaxV,
	int &iCharSizeX, int &iCharSizeY, int &iCharOffsetX, int &iCharOffsetY ) const
{
	if(!pSlot)
		return;			// expected behavior

	float fInvWidth = 1.0f/(float)m_iWidth;
	float fInvHeight = 1.0f/(float)m_iHeight;

	fMinU = pSlot->vTexCoord[0]-fInvWidth;											// extra pixel for nicer bilinear filter
	fMinV = pSlot->vTexCoord[1]-fInvHeight;										// extra pixel for nicer bilinear filter
	fMaxU = pSlot->vTexCoord[0]+pSlot->iCharWidth*fInvWidth;
	fMaxV = pSlot->vTexCoord[1]+pSlot->iCharHeight*fInvHeight;

	iCharSizeX = (int)pSlot->iCharWidth+1;										// extra pixel for nicer bilinear filter
	iCharSizeY = (int)pSlot->iCharHeight+1;										// extra pixel for nicer bilinear filter
	iCharOffsetX = pSlot->iCharOffsetX;
	iCharOffsetY = pSlot->iCharOffsetY;
}

//------------------------------------------------------------------------------------------------- 
int CFontTexture::GetCharacterWidth(wchar_t cChar) const
{
	CTextureSlotTableItorConst pItor = m_pSlotTable.find(cChar);

	if(pItor==m_pSlotTable.end())
		return 0;

	const CTextureSlot &rSlot = *pItor->second;

	return rSlot.iCharWidth+1;										// extra pixel for nicer bilinear filter
}

//------------------------------------------------------------------------------------------------- 
/*
int CFontTexture::GetCharHeightByChar(wchar_t cChar)
{
	CTextureSlotTableItor pItor = m_pSlotTable.find(cChar);

	if (pItor != m_pSlotTable.end())
	{
		return pItor->second->iCharHeight;
	}

	return 0;
}
*/

//------------------------------------------------------------------------------------------------- 
int CFontTexture::WriteToFile(const string &szFileName)
{
#ifdef WIN32
	FILE *hFile = fopen(szFileName.c_str(), "wb");

	if(!hFile)
		return 0;

	BITMAPFILEHEADER pHeader;
	BITMAPINFOHEADER pInfoHeader;

	memset(&pHeader, 0, sizeof(BITMAPFILEHEADER));
	memset(&pInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	
	pHeader.bfType = 0x4D42;
	pHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + m_iWidth * m_iHeight * 3;
	pHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	pInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	pInfoHeader.biWidth = m_iWidth;
	pInfoHeader.biHeight = m_iHeight;
	pInfoHeader.biPlanes = 1;
	pInfoHeader.biBitCount = 24;
	pInfoHeader.biCompression = 0;
	pInfoHeader.biSizeImage = m_iWidth * m_iHeight * 3;

	fwrite(&pHeader, 1, sizeof(BITMAPFILEHEADER), hFile);
	fwrite(&pInfoHeader, 1, sizeof(BITMAPINFOHEADER), hFile);

	unsigned char cRGB[3];

	for(int i = m_iHeight - 1; i >= 0; i--)
	{
		for(int j = 0; j < m_iWidth; j++)
		{
			cRGB[0] = m_pBuffer[(i * m_iWidth) + j];
			cRGB[1] = *cRGB;

			cRGB[2] = *cRGB;

			fwrite(cRGB, 1, 3, hFile);
		}
	}

	fclose(hFile);
#endif
	return 1;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int CFontTexture::CreateSlotList(int iListSize)
{
	int y, x;

	for (int i = 0; i < iListSize; i++)
	{
		CTextureSlot *pTextureSlot = new CTextureSlot;

		if (!pTextureSlot)
			return 0;

		pTextureSlot->iTextureSlot = i;
		pTextureSlot->Reset();

		y = i / m_iWidthCellCount;
		x = i % m_iWidthCellCount;

		pTextureSlot->vTexCoord[0] = (float)(x * m_fTextureCellWidth)+(0.5f/(float)m_iWidth);
		pTextureSlot->vTexCoord[1] = (float)(y * m_fTextureCellHeight)+(0.5f/(float)m_iHeight);

		m_pSlotList.push_back(pTextureSlot);
	}

	return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::ReleaseSlotList()
{
	CTextureSlotListItor pItor = m_pSlotList.begin();

	while (pItor != m_pSlotList.end())
	{
		delete (*pItor);

		pItor = m_pSlotList.erase(pItor);
	}

	return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::UpdateSlot(int iSlot, uint16 wSlotUsage, wchar_t cChar)
{
	CTextureSlot *pSlot = m_pSlotList[iSlot];

	if (!pSlot)
		return 0;

	CTextureSlotTableItor pItor = m_pSlotTable.find(pSlot->cCurrentChar);

	if (pItor != m_pSlotTable.end())
		m_pSlotTable.erase(pItor);

	m_pSlotTable.insert(std::pair<wchar_t, CTextureSlot *>(cChar, pSlot));

	pSlot->wSlotUsage = wSlotUsage;
	pSlot->cCurrentChar = cChar;

	int iWidth = 0;
	int iHeight = 0;
	
	// blit the char glyph into the texture
	int x = pSlot->iTextureSlot % m_iWidthCellCount;
	int y = pSlot->iTextureSlot / m_iWidthCellCount;

	CGlyphBitmap *pGlyphBitmap;

	if (!m_pGlyphCache.GetGlyph(&pGlyphBitmap, &iWidth, &iHeight, pSlot->iCharOffsetX, pSlot->iCharOffsetY, cChar))
		return 0;

	pSlot->iCharWidth = iWidth;
	pSlot->iCharHeight = iHeight;

	pGlyphBitmap->BlitTo8(m_pBuffer, 0, 0, 
		iWidth, iHeight, x * m_iCellWidth, y * m_iCellHeight, m_iWidth);

	return 1;
}

//------------------------------------------------------------------------------------------------- 
void CFontTexture::CreateGradientSlot()
{
	CTextureSlot *pSlot = GetGradientSlot();				assert(pSlot->cCurrentChar==(uint16)~0);		// 0 needs to be unused spot

	pSlot->Reset();
	pSlot->iCharWidth = m_iCellWidth-2;
	pSlot->iCharHeight = m_iCellHeight-2;
	pSlot->SetNotReusable();

	int x = pSlot->iTextureSlot % m_iWidthCellCount;
	int y = pSlot->iTextureSlot / m_iWidthCellCount;

	assert(sizeof(*m_pBuffer)==sizeof(uint8));
	uint8 *pBuffer = &m_pBuffer[ x*m_iCellWidth + y*m_iCellHeight*m_iWidth];

	for(uint32 dwY=0;dwY<pSlot->iCharHeight;++dwY)
	for(uint32 dwX=0;dwX<pSlot->iCharWidth;++dwX)
		pBuffer[dwX+dwY*m_iWidth] = dwY*255/(pSlot->iCharHeight-1);
}

//------------------------------------------------------------------------------------------------- 
CTextureSlot *CFontTexture::GetGradientSlot()
{
	return m_pSlotList[0];
}

void CFontTexture::GetMemoryUsage( class ICrySizer* pSizer ) const
{	
	pSizer->AddObject(this, sizeof(*this));
#ifndef USE_VIRT_MEM
	pSizer->AddObject( m_pBuffer, m_iWidth*m_iHeight );
#endif
}

