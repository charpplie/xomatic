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
#pragma once

#include "GlyphCache.h"
#include "GlyphBitmap.h"


typedef uint8 FONT_TEXTURE_TYPE;

// the number of slots in the glyph cache
// each slot ocupies ((glyph_bitmap_width * glyph_bitmap_height) + 24) bytes
#define	FONT_GLYPH_CACHE_SIZE			(1)

// the glyph spacing in font texels between characters in proportional font mode (more correct would be to take the value in the character)
#define FONT_GLYPH_PROP_SPACING		(1)

// the size of a rendered space, this value gets multiplied by the default characted width
#define FONT_SPACE_SIZE						(0.5f)

// don't draw this char (used to avoid drawing color codes)
#define FONT_NOT_DRAWABLE_CHAR		(0xffff)

// smoothing methods
#define FONT_SMOOTH_NONE			0
#define FONT_SMOOTH_BLUR			1
#define FONT_SMOOTH_SUPERSAMPLE		2

// smoothing amounts
#define FONT_SMOOTH_AMOUNT_NONE		0
#define FONT_SMOOTH_AMOUNT_2X		1
#define FONT_SMOOTH_AMOUNT_4X		2


typedef struct CTextureSlot
{
	uint16				wSlotUsage;					// for LRU strategy, 0xffff is never released
	wchar_t				cCurrentChar;				// ~0 if not used for characters
	int						iTextureSlot;
	float					vTexCoord[2];				// character position in the texture (not yet half texel corrected)
	uint8					iCharWidth;					// size in pixel
	uint8					iCharHeight;				// size in pixel
	char					iCharOffsetX;
	char					iCharOffsetY;

	void Reset()
	{
		wSlotUsage = 0;
		cCurrentChar = ~0;
		iCharWidth = 0;
		iCharHeight = 0;
		iCharOffsetX = 0;
		iCharOffsetY = 0;
	}

	void SetNotReusable()
	{
		wSlotUsage = 0xffff;
	}

} CTextureSlot;


typedef std::vector<CTextureSlot *>														CTextureSlotList;
typedef std::vector<CTextureSlot *>::iterator									CTextureSlotListItor;

typedef stl::hash_map<uint16, CTextureSlot *>									CTextureSlotTable;
typedef stl::hash_map<uint16, CTextureSlot *>::iterator				CTextureSlotTableItor;
typedef stl::hash_map<uint16, CTextureSlot *>::const_iterator	CTextureSlotTableItorConst;

#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

class CFontTexture
{
public:
	CFontTexture();
	~CFontTexture();

	int CreateFromFile(const string &szFileName, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio = 0.8f, int iWidthCharCount = 16, int iHeightCharCount = 16);
	int CreateFromMemory(unsigned char *pFileData, int iDataSize, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio = 0.875f, int iWidthCharCount = 16, int iHeightCharCount = 16);
	int Create(int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio = 0.8f, int iWidthCharCount = 16, int iHeightCharCount = 16);
	int Release();

	int SetEncoding(FT_Encoding pEncoding) { return m_pGlyphCache.SetEncoding(pEncoding); }
	FT_Encoding GetEncoding() { return m_pGlyphCache.GetEncoding(); }

	int GetCellWidth() { return m_iCellWidth; }
	int GetCellHeight() { return m_iCellHeight; }

	int GetWidth() { return m_iWidth; }
	int GetHeight() { return m_iHeight; }

	int GetWidthCellCount() { return m_iWidthCellCount; }
	int GetHeightCellCount() { return m_iHeightCellCount; }

	float GetTextureCellWidth() { return m_fTextureCellWidth; }
	float GetTextureCellHeight() { return m_fTextureCellHeight; }

	FONT_TEXTURE_TYPE* GetBuffer() { return m_pBuffer; }

	wchar_t GetSlotChar(int iSlot) const;
	CTextureSlot *GetCharSlot(wchar_t cChar);
	CTextureSlot *GetGradientSlot();

	CTextureSlot *GetLRUSlot();
	CTextureSlot *GetMRUSlot();

	// returns 1 if texture updated, returns 2 if texture not updated, returns 0 on error
	// pUpdated is the number of slots updated
	int PreCacheString(const wchar_t *szString, int *pUpdated = 0);
	// Arguments:
	//   pSlot - function does nothing if this pointer is 0
	void GetTextureCoord( CTextureSlot *pSlot, float &fMinU, float &fMinV, float &fMaxU, float &fMaxV, int &iCharSizeX, int &iCharSizeY, int &iCharOffsetX, int &iCharOffsetY ) const;
	int GetCharacterWidth(wchar_t cChar) const;
//	int GetCharHeightByChar(wchar_t cChar);

	// useful for special feature rendering interleaved with fonts (e.g. box behind the text)
	void CreateGradientSlot();

	int WriteToFile(const string &szFileName);

	void GetMemoryUsage (class ICrySizer* pSizer) const;
	
private: // ---------------------------------------------------------------

	int CreateSlotList(int iListSize);
	int ReleaseSlotList();
	int UpdateSlot(int iSlot, uint16 wSlotUsage, wchar_t cChar);

	// --------------------------------

	int										m_iWidth;								// whole texture cache width
	int										m_iHeight;							// whole texture cache height

	int										m_iCellWidth;
	int										m_iCellHeight;

	float									m_fTextureCellWidth;
	float									m_fTextureCellHeight;

	int										m_iWidthCellCount;
	int										m_iHeightCellCount;

	int										m_nTextureSlotCount;

	int										m_iSmoothMethod;
	int										m_iSmoothAmount;

	CGlyphCache						m_pGlyphCache;
	CTextureSlotList			m_pSlotList;
	CTextureSlotTable			m_pSlotTable;

	FONT_TEXTURE_TYPE *		m_pBuffer;							// [y*iWidth * x] x=0..iWidth-1, y=0..iHeight-1

	uint16								m_wSlotUsage;
};

