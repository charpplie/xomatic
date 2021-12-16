//////////////////////////////////////////////////////////////////////
//
//  CryFont Source Code
//
//  File: FFont.h
//  Description: Font class.
//
//  History:
//  - August 20, 2001: Created by Alberto Demichelis
//	- January 29, 2004: Refactored by Márcio Martins
//
//////////////////////////////////////////////////////////////////////

#ifndef CRYFONT_FFONT_H
#define CRYFONT_FFONT_H


#include <vector>
#include <Cry_Math.h>
#include "FontTexture.h"

#define FONTCOLORS 10

#define FONT_SMOOTH_NONE					0
#define FONT_SMOOTH_BLUR					1
#define FONT_SMOOTH_SUPERSAMPLE		2

#define FONT_SMOOTH_AMOUNT_NONE		0
#define FONT_SMOOTH_AMOUNT_2X			1
#define FONT_SMOOTH_AMOUNT_4X			2

#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

static void DumpFontTexture( IConsoleCmdArgs *pArgs );

class CFFont : public IFFont
{
public:
	struct SRenderingPass
	{
		ColorB				m_cColor;
		vector2f			vPosOffset;
		int						blendSrc;
		int						blendDest;

		void ResetDefault()
		{
			vPosOffset.set(0,0);
			blendSrc = GS_BLSRC_SRCALPHA;
			blendDest = GS_BLDST_ONEMINUSSRCALPHA;

			m_cColor=0xffffffff;
		}

		SRenderingPass()
		{
			ResetDefault();
		}

		void GetMemoryUsage(ICrySizer *pSizer) const{}
	};

	// ---------------------

	struct SEffect
	{
		string											strName;
		std::vector<SRenderingPass> vPass;

		SRenderingPass* NewPass()
		{
			SRenderingPass Pass;

			vPass.push_back(Pass);

			return &vPass[vPass.size()-1];
		}

		void Clear()
		{
			vPass.clear();
		}

		void GetMemoryUsage (class ICrySizer* pSizer) const
		{
			pSizer->AddObject(strName);
			pSizer->AddObject(vPass);
		}
	};

	// ---------------------

	typedef std::vector<SEffect>	VecEffect;
	typedef VecEffect::iterator		VecEffectItor;

public:
	// interface IFFont -------------------------------------------------

	virtual void Reset();
	virtual void Release();
	virtual bool Load(const char *szFile, unsigned long nWidth, unsigned long nHeight, unsigned long nTTFFlags);
	virtual bool Load(const char *szFile);
	virtual void Free();
	virtual void SetEffect(const char *szEffect);
	virtual void SetClippingRect(float fX, float fY, float fW, float fH);
	virtual void EnableClipping(bool bEnable);
  virtual SFontRenderContext *GetRenderContext(int& nSize);
  virtual int SetRenderContext(SFontRenderContext *pContext);
	virtual void SetColor(const ColorB col, int nPass);
	virtual void SetSizeIn800x600(bool bValue=false) { m_bSizeIn800x600=bValue; }
	virtual bool GetSizeIn800x600() const { return m_bSizeIn800x600; }
	virtual	void UseFrame(bool bFrame=true) { m_bFramed=bFrame; }
	virtual void SetSize(const vector2f &size);
	virtual vector2f &GetSize();
	virtual float GetCharWidth();
	virtual float GetCharHeight();
	virtual void SetCharWidthScale(float fScale);
	virtual float GetCharWidthScale();
	virtual void SetProportional(bool bValue);
	virtual bool GetProportional() const;
	virtual void DrawString( float x, float y, const char *szMsg, const bool bASCIIMultiLine=true );
	virtual void DrawString( float x, float y, float z, const char *szMsg, const bool bASCIIMultiLine=true );
	virtual vector2f GetTextSize(const char *szMsg, const bool bASCIIMultiLine=true );	
	virtual void DrawStringW( float x, float y, const wchar_t *swStr, const bool bASCIIMultiLine=true );
	virtual void DrawStringW( float x, float y, float z, const wchar_t *swStr, const bool bASCIIMultiLine=true );
	virtual void RT_DrawStringW( float x, float y, float z, const wchar_t *swStr, const bool bASCIIMultiLine=true );
	virtual void DrawWrappedStringW( float x, float y, float w, const wchar_t *swStr, const bool bASCIIMultiLine=true );
	virtual vector2f GetTextSizeW(const wchar_t *swStr, const bool bASCIIMultiLine=true );
	virtual vector2f GetWrappedTextSizeW(const wchar_t *swStr, float w, const bool bASCIIMultiLine=true );
	virtual int GetTextLength(const char *szMsg, const bool bASCIIMultiLine=true);
	virtual int GetTextLengthW(const wchar_t *szMsg, const bool bASCIIMultiLine=true);
	virtual void GetMemoryUsage (class ICrySizer* pSizer) const;
	virtual void GetGradientTextureCoord( float &fMinU, float &fMinV, float &fMaxU, float &fMaxV ) const;

	// -----------------------------------------------------------------------

	// Create a new effect
	SEffect* NewEffect();
	// Return the current effect
	SEffect* GetCurrentEffect() const;

	bool RenderInit();
	void RenderCleanup();

private: // -------------------------------------------------------------------------------

	// constructor
	CFFont(struct ISystem *pISystem, class CCryFont *pCryFont, const char *pszName);
	// destructor
	virtual ~CFFont();
	//
	void Prepare(const wchar_t *szString);
	//
	void WrapText(wstring &szResult, float fMaxWidth, const wchar_t *szString);
	//
	void FillRenderContext( SFontRenderContext &rcx );

private: // -------------------------------------------------------------------------------

	string								m_sName;							// e.g. "renderer_internal" or "hud"
	bool									m_bOK;								// false if the font has not been correctly initialised
	string								m_sCurPath;						// e.g. "Fonts/"

	vector2f							m_vSize;							// Base size of one character in texels e.g. (16,16), can be changed by SetSize()
	VecEffect							m_vEffects;						// Rendering effects
	SEffect *							m_pCurrentEffect;			// Current effect
	CFontTexture *				m_pFontTexture;				// can be 0
	uint32								m_nFontBufferSize;		// size of the font buffer to correct tracking
	unsigned char	*				m_pFontBuffer;				// in memory image of the font file (needed because of pack files)
	int										m_iTextureID;					// texture id, -1 if not created yet

//	bool									m_bRealPixels;			// false: width and height of character is defined in 800x600 resolution, true otherwise
	bool									m_bSizeIn800x600;			// true: width and height of character is defined in 800x600 resolution, false otherwise

	bool									m_bProportional;			// affects if m_fWidthScale is taken into account (complex: width = m_bProportional ? FONT_SPACE_SIZE*vSize.x : vSize.x)
	float									m_fWidthScale;				// Width scale, only used if m_bProportional==false
	bool									m_bFramed;						// when true, a transparent frame is drawn behind the text

	// clipping related
	bool									m_bClipEnabled;				// only affects if m_fClip... are used
	float									m_fClipX;							// only used if m_bClipEnabled==true
	float									m_fClipY;							// only used if m_bClipEnabled==true
	float									m_fClipR;							// only used if m_bClipEnabled==true
	float									m_fClipB;							// only used if m_bClipEnabled==true

	// multithreaded related ?
	SFontRenderContext *	m_pRenderContext;

	// global pointers
	struct ISystem *			m_pISystem;						// system interface
	CCryFont*							m_pCryFont;						// to free the given font


	friend struct SFontRenderContext;
	friend class CCryFont;
	friend void DumpFontTexture( IConsoleCmdArgs *pArgs );
};

struct SFontRenderContext
{
	int										m_iTextureID;															// copy from CFFont
	CFontTexture *				m_pFontTexture;														// copy from CFFont
	CFFont::SEffect *			m_pCurrentEffect;													// copy from CFFont
	vector2f							m_vSize;																	// copy from CFFont
	float									m_fWidthScale;														// copy from CFFont
	float 								m_fClipR, m_fClipB, m_fClipX, m_fClipY;		// copy from CFFont
	bool 									m_bProportional;													// copy from CFFont
	bool 									m_bSizeIn800x600;													// copy from CFFont
	bool									m_bClipEnabled;														// copy from CFFont
	ColorB 								m_cPassColor[2];													// Color for 2 passes: [0]=pass 0, [1]=pass 1
	bool									m_bFramed;
	CFFont *							m_pFont;
	
	void RT_DrawStringW(CFFont *pFont,float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine);

	void GetMemoryUsage (class ICrySizer* pSizer) const{}
};

#endif // CRYFONT_FFONT_H
