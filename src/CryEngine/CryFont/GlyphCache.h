//-------------------------------------------------------------------------------------------------
// Author: Márcio Martins
//
// Purpose:
//  - Manage and cache glyphs, retrieving them from the renderer as needed
//
// History:
//  - [6/6/2003] created the file
//
//-------------------------------------------------------------------------------------------------
#pragma once

#include <vector>
#include "GlyphBitmap.h"
#include "FontRenderer.h"
#include <StlUtils.h>


typedef struct CCacheSlot
{
	unsigned int	dwUsage;
	int				iCacheSlot;
	wchar_t			cCurrentChar;

	uint8					iCharWidth;					// size in pixel
	uint8					iCharHeight;				// size in pixel
	char					iCharOffsetX;
	char					iCharOffsetY;
    
	CGlyphBitmap	pGlyphBitmap;

	void			Reset()
	{
		dwUsage = 0;
		cCurrentChar = ~0;

		iCharWidth = 0;
		iCharHeight = 0;
		iCharOffsetX = 0;
		iCharOffsetY = 0;

		pGlyphBitmap.Clear();
	}

} CCacheSlot;


typedef stl::hash_map<uint16, CCacheSlot *>			CCacheTable;

typedef std::vector<CCacheSlot *>						CCacheSlotList;
typedef std::vector<CCacheSlot *>::iterator				CCacheSlotListItor;


#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class CGlyphCache
{
public:
	CGlyphCache();
	~CGlyphCache();

	int Create(int iCacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio = 0.8f);
	int Release();

	int LoadFontFromFile(const string &szFileName);
	int LoadFontFromMemory(unsigned char *pFileBuffer, int iDataSize);
	int ReleaseFont();

	int SetEncoding(FT_Encoding pEncoding) { return m_pFontRenderer.SetEncoding(pEncoding); };
	FT_Encoding GetEncoding() { return m_pFontRenderer.GetEncoding(); };

	int	GetGlyphBitmapSize(int *pWidth, int *pHeight);

	int PreCacheGlyph(wchar_t cChar);
	int UnCacheGlyph(wchar_t cChar);
	int GlyphCached(wchar_t cChar);

	CCacheSlot *GetLRUSlot();
	CCacheSlot *GetMRUSlot();

	int GetGlyph(CGlyphBitmap **pGlyph, int *piWidth, int *piHeight, char &iCharOffsetX, char &iCharOffsetY, wchar_t cChar);

private:

	int				CreateSlotList(int iListSize);
	int				ReleaseSlotList();

	CCacheSlotList	m_pSlotList;
	CCacheTable		m_pCacheTable;

	int				m_iGlyphBitmapWidth;
	int				m_iGlyphBitmapHeight;
	float			m_fSizeRatio;

	int				m_iSmoothMethod;
	int				m_iSmoothAmount;

	CGlyphBitmap	*m_pScaleBitmap;

	CFontRenderer	m_pFontRenderer;

	unsigned int	m_dwUsage;
};
