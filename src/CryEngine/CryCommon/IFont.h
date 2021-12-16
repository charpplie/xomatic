//////////////////////////////////////////////////////////////////////
//
//  CryFont Source Code
//
//  File: IFont.h
//  Description: CryFont interface.
//
//  History:
//  - August 17, 2001: Created by Alberto Demichelis
//
//////////////////////////////////////////////////////////////////////

#ifndef CRYFONT_ICRYFONT_H
#define CRYFONT_ICRYFONT_H

#include <Cry_Color.h>
#include <Cry_Math.h>

#ifdef GetCharWidth
#undef GetCharWidth
#undef GetUserName
#endif

struct ISystem;

//////////////////////////////////////////////////////////////////////////////////////////////
// THE Only exported function of the DLL

// export for the dll, very clear ;=)
extern "C"
#ifdef CRYFONT_EXPORTS
	DLL_EXPORT
#else
	DLL_IMPORT
#endif
struct ICryFont* CreateCryFontInterface(ISystem *pSystem);

typedef ICryFont *(*PFNCREATECRYFONTINTERFACE)(ISystem *pSystem);

//////////////////////////////////////////////////////////////////////////////////////////////

// Summary:
//	 Rendering interfaces.
enum CRYFONT_RENDERINGINTERFACE
{
	CRYFONT_RI_OPENGL = 0,		// pRIData is ignored
	CRYFONT_RI_LAST
};

//////////////////////////////////////////////////////////////////////////////////////////////
struct ICryFont
{
	//
	virtual void Release() = 0;
	// Summary:
	//	 Creates a named font (case sensitive)
	virtual struct IFFont *NewFont(const char *pszName)=0;
	// Summary:
	//	 Gets a named font (case sensitive)
	virtual struct IFFont *GetFont(const char *pszName)=0;
	// Summary:
	//	 Puts the objects used in this module into the sizer interface
	virtual void GetMemoryUsage (class ICrySizer* pSizer)=0;
	// Summary:
	//	 All font names separated by , 
	// Example:
	//	 "console,default,hud"
	virtual string GetLoadedFontNames(){ return string(""); }	// not =0 for simpler CCryNullFont
};

//////////////////////////////////////////////////////////////////////////////////////////////
#define TTFFLAG_SMOOTH_NONE					0x00000000		// No smooth.
#define TTFFLAG_SMOOTH_BLUR					0x00000001		// Smooth by blurring it.
#define TTFFLAG_SMOOTH_SUPERSAMPLE	0x00000002				// Smooth by rendering the characters into a bigger texture, 
															// and then resize it to the normal size using bilinear filtering.

#define TTFFLAG_SMOOTH_MASK					0x0000000f		// Mask for retrieving.
#define TTFFLAG_SMOOTH_SHIFT				0				// Shift amount for retrieving.

#define TTFLAG_SMOOTH_AMOUNT_2X			0x00010000			// Blur / supersample [2x]
#define TTFLAG_SMOOTH_AMOUNT_4X			0x00020000			// Blur / supersample [4x]

#define TTFFLAG_SMOOTH_AMOUNT_MASK	0x000f0000				// Mask for retrieving.
#define TTFFLAG_SMOOTH_AMOUNT_SHIFT	16						// Shift amount for retrieving.


// Creates a ttflag
#define TTFFLAG_CREATE(smooth, amount)		((((smooth) << TTFFLAG_SMOOTH_SHIFT) & TTFFLAG_SMOOTH_MASK) | (((amount) << TTFFLAG_SMOOTH_AMOUNT_SHIFT) & TTFFLAG_SMOOTH_AMOUNT_MASK))
#define TTFFLAG_GET_SMOOTH(flag)					(((flag) & TTFLAG_SMOOTH_MASK) >> TTFFLAG_SMOOTH_SHIFT)
#define TTFFLAG_GET_SMOOTH_AMOUNT(flag)		(((flag) & TTFLAG_SMOOTH_SMOUNT_MASK) >> TTFFLAG_SMOOTH_AMOUNT_SHIFT)


#define FONTRF_HCENTERED						0x80000000		// The font will be centered horizontally around the x coo
#define FONTRF_VCENTERED						0x40000000		// The font will be centered vertically around the y coo
#define FONTRF_FILTERED							0x20000000		// The font will be drawn with bilinear filtering

struct SFontRenderContext;

//////////////////////////////////////////////////////////////////////////////////////////////
struct IFFont
{
	// Summary:
	//	 Resets the font to the default state.
	virtual void Reset() = 0;

	virtual void Release() = 0;

	// Summary:
	//	 Loads a font from a TTF file.
	virtual bool Load(const char *szFile, unsigned long nWidth, unsigned long nHeight, unsigned long nTTFFlags) = 0;

	// Summary:
	//	 Loads a font from a XML file.
	virtual bool Load(const char *szFile) = 0;

	// Summary:
	//	 Frees the memory.
	virtual void Free() = 0;

	// Summary:
	//	 Sets the current effect to use.
	virtual void SetEffect(const char *szEffect) = 0;

	// Summary:
	//	 Sets clipping rectangle.
	virtual void SetClippingRect(float fX, float fY, float fW, float fH) = 0;

	// Summary:
	//	 Enables/Disables clipping (off by default).
	virtual void EnableClipping(bool bEnable) = 0;

	virtual SFontRenderContext *GetRenderContext(int& nSize) = 0;
	virtual int SetRenderContext(SFontRenderContext *pContext) = 0;

	// Summary:
	//	Sets the color of the current effect.
	virtual void SetColor(const ColorB col, int nPass = 0) = 0;
	virtual void SetSizeIn800x600(bool bValue=false)=0;
	virtual bool GetSizeIn800x600() const=0;

	virtual void UseFrame(bool bFrame=true)=0;

	// Summary:
	//	 Sets the characters base size.
	virtual void SetSize(const vector2f &size) = 0;

	// Summary:
	//	 Returns the set size.
	virtual vector2f &GetSize() = 0;

	// Summary:
	//	 Returns the char width.
	virtual float GetCharWidth() = 0;

	// Summary:
	//	 Returns the char height.
	virtual float GetCharHeight() = 0;

	// Summary:
	//	 Sets the proportional flag.
	virtual void SetProportional(bool bValue) = 0;

	// Summary:
	//	 Gets the proportional flag.
	virtual bool GetProportional() const= 0;

	// Summary:
	//	 Sets the width scaling.
	virtual void SetCharWidthScale(float fScale = 1.0f) = 0;

	// Summary:
	//	 Gets the width scaling.
	virtual float GetCharWidthScale() = 0;

	// Summary:
	//	 Draws a formated string.
	// Arguments:
	//	 bASCIIMultiLine - true='\','n' is a valid return, false=it's not.
	virtual void DrawString( float x, float y, const char *szMsg, const bool bASCIIMultiLine=true ) = 0;

	// Summary:
	//	 Draws a formated string, taking z into account.
	// Arguments:
	//	 bASCIIMultiLine - true='\','n' is a valid return, false=it's not.
	virtual void DrawString( float x, float y, float z, const char *szMsg, const bool bASCIIMultiLine=true ) = 0;

	// Summary:
	//	 Computes the text size.
	// Arguments:
	//	 bASCIIMultiLine - true='\','n' is a valid return, false=it's not.
	virtual vector2f GetTextSize(const char *szMsg, const bool bASCIIMultiLine=true ) = 0;

	// Summary:
	//	 Draws a formated string (Unicode)
	// Arguments:
	//	 bASCIIMultiLine - true='\','n' is a valid return, false=it's not.
	virtual void DrawStringW( float x, float y, const wchar_t *swStr, const bool bASCIIMultiLine=true ) = 0;

	// Summary:
	//	 Draws a formated string (Unicode), taking z into account.
	// Arguments:
	//	 bASCIIMultiLine - true='\','n' is a valid return, false=it's not
	virtual void DrawStringW( float x, float y, float z, const wchar_t *swStr, const bool bASCIIMultiLine=true ) = 0;
	virtual void RT_DrawStringW( float x, float y, float z, const wchar_t *swStr, const bool bASCIIMultiLine=true ) = 0;

	// Summary:
	//	 Draws a formated string (Unicode)
	virtual void DrawWrappedStringW( float x, float y, float w, const wchar_t *swStr, const bool bASCIIMultiLine=true ) = 0;

	// Summary:
	//	 Computes the text size (Unicode)
	// Arguments:
	//	 bASCIIMultiLine - true='\','n' is a valid return, false=it's not
	virtual vector2f GetTextSizeW(const wchar_t *swStr, const bool bASCIIMultiLine=true ) = 0;	

	// Summary:
	//	 Computes the text size (Unicode)
	virtual vector2f GetWrappedTextSizeW(const wchar_t *swStr, float w, const bool bASCIIMultiLine=true ) = 0;

	// Description:
	//	 Computes virtual text-length (because of special chars...).
	virtual int GetTextLength(const char *szMsg, const bool bASCIIMultiLine=true) = 0;

	// Description:
	//	 Computes virtual text-length (Unicode) (because of special chars...).
	virtual int GetTextLengthW(const wchar_t *szwMsg, const bool bASCIIMultiLine=true) = 0;

	// Description:
	//	 Puts the memory used by this font into the given sizer.
	virtual void GetMemoryUsage (class ICrySizer* pSizer) const= 0;

	// Description:
	//   useful for special feature rendering interleaved with fonts (e.g. box behind the text)
	virtual void GetGradientTextureCoord( float &fMinU, float &fMinV, float &fMaxU, float &fMaxV ) const {};
};


#endif // CRYFONT_ICRYFONT_H
