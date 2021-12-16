//////////////////////////////////////////////////////////////////////
//
//  CryFont Source Code
//
//  File: FFont.cpp
//  Description: Font class.
//
//  History:
//  - August 20, 2001: d by Alberto Demichelis
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FFont.h"
#include "FBitmap.h"
#include "CryFont.h"

#include "FontTexture.h"
#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif


static ColorB g_cColorTable[FONTCOLORS]=
{
	ColorB(0x00,0x00,0x00),		// black
	ColorB(0xff,0xff,0xff),		// white
	ColorB(0x00,0x00,0xff),		// blue
	ColorB(0x00,0xff,0x00),		// green
	ColorB(0xff,0x00,0x00),		// red
	ColorB(0x00,0xff,0xff),		// cyan
	ColorB(0xff,0xff,0x00),		// yellow
	ColorB(0xff,0x00,0xff),		// purple
	ColorB(0xff,0x80,0x00),		// 
	ColorB(0x8f,0x8f,0x8f),		// grey
};

const int g_iTabCharCount=4;


///////////////////////////////////////////////
CFFont::CFFont(struct ISystem *pISystem, class CCryFont *pCryFont, const char *pszName)
{
	m_bSizeIn800x600=true;
	m_fWidthScale = 1.0f;
	m_bProportional = true;
	m_pISystem = pISystem;
	m_pCryFont = pCryFont;
	m_sName = pszName;
	m_vSize.set(16, 16);
	m_iTextureID = -1;

	m_fClipX = m_fClipY = 0.0f;
	m_fClipR = m_fClipB = 0.0f;

	m_bClipEnabled = 0;
	m_pFontBuffer = 0;
	m_bFramed = false;

	// create the default effect
	SEffect *pEffect = NewEffect();
	pEffect->strName = "default";
	pEffect->NewPass();
	SetEffect("default");

  m_pFontTexture = new CFontTexture();
	m_pRenderContext = new SFontRenderContext;
}


///////////////////////////////////////////////
CFFont::~CFFont()
{
	SAFE_DELETE(m_pRenderContext);
	if (m_pCryFont)
	{
		m_pCryFont->UnregisterFont(m_sName);
		m_pCryFont = 0;
	}
	Free();
}

void CFFont::Reset()
{
	m_fWidthScale = 1.0f;
	m_bClipEnabled = 0;
	m_vSize = vector2f(16.0f, 16.0f);
	
	m_bProportional = true;
	m_bSizeIn800x600 = true;
}


///////////////////////////////////////////////
// Release the memory...
void CFFont::Release()
{
	delete this;
}

///////////////////////////////////////////////
// Load a font from a TTF file
bool CFFont::Load(const char *szFile, unsigned long nWidth, unsigned long nHeight, unsigned long nTTFFlags)
{
	Free();

	ICryPak *pPak = gEnv->pCryPak;

	string szFullFile;
	if (pPak->IsAbsPath(szFile))
		szFullFile = szFile;
	 else
		szFullFile = m_sCurPath + szFile;	

	int iSmoothMethod = (nTTFFlags & TTFFLAG_SMOOTH_MASK) >> TTFFLAG_SMOOTH_SHIFT;
	int iSmoothAmount = (nTTFFlags & TTFFLAG_SMOOTH_AMOUNT_MASK) >> TTFFLAG_SMOOTH_AMOUNT_SHIFT;

	FILE *pFile = pPak->FOpen(szFullFile.c_str(),"rb");

	if (!pFile)
		return false;

	pPak->FSeek(pFile, 0, SEEK_END); 
	int nSize = pPak->FTell(pFile); 
	pPak->FSeek(pFile, 0, SEEK_SET); 

	if (!nSize)
	{
		pPak->FClose(pFile); 

		return false;
	}	

#ifdef USE_VIRT_MEM
	uint32 h;
	unsigned char *pBuffer = (unsigned char*)NVirtualMem::AllocVirtualMem(nSize,h);
#else
	unsigned char *pBuffer = new unsigned char[nSize];
#endif

	if (!pPak->FReadRaw(pBuffer, nSize, 1, pFile))
	{
		pPak->FClose(pFile);
#ifdef USE_VIRT_MEM
		NVirtualMem::FreeVirtualMem(pBuffer);
#else
		delete[] pBuffer;
#endif
		return false;
	}

	pPak->FClose(pFile);

  if (m_pFontTexture == 0)
    m_pFontTexture = new CFontTexture();

	if (!m_pFontTexture->CreateFromMemory(pBuffer, nSize, nWidth, nHeight, iSmoothMethod, iSmoothAmount))
	{
#ifdef USE_VIRT_MEM
		NVirtualMem::FreeVirtualMem(pBuffer);
#else
		delete[] pBuffer;
#endif

		return false;
	}

	m_pFontBuffer = pBuffer;
	m_nFontBufferSize = nSize;
	//------------------------------------------------------------------------------------------------- 

	m_bOK = true;
	return true;
}


void CFFont::Free()
{
	RenderCleanup();
#ifdef USE_VIRT_MEM
	NVirtualMem::FreeVirtualMem(m_pFontBuffer);
#else
	delete[] m_pFontBuffer;
#endif
	m_pFontBuffer = 0;
  
	delete m_pFontTexture;
	m_pFontTexture = 0;

	m_bOK = false;
}

///////////////////////////////////////////////
// Set the current effect to use
void CFFont::SetEffect(const char *szEffect)
{
	m_pCurrentEffect = NULL;
	if(!szEffect)
		szEffect = "default";

	for(int i = 0; i < (int)m_vEffects.size(); ++i)
	{
		if(strcmp(m_vEffects[i].strName.c_str(), szEffect) == 0)
		{
			m_pCurrentEffect = &m_vEffects[i];
			return;
		}
	}

	if(!m_pCurrentEffect)
		m_pCurrentEffect = &m_vEffects[0];
}


void CFFont::SetClippingRect(float fX, float fY, float fX2, float fY2)
{
	m_fClipX = fX;
	m_fClipY = fY;
	m_fClipR = fX2;
	m_fClipB = fY2;
}


void CFFont::EnableClipping(bool bEnable)
{
	m_bClipEnabled = bEnable;
}


///////////////////////////////////////////////
void CFFont::SetColor(const ColorB col, int nPass)
{
	SRenderingPass *pPass;
	if(nPass < 0)
	{
		std::vector<SRenderingPass>::iterator i, end=m_pCurrentEffect->vPass.end();

		for(i=m_pCurrentEffect->vPass.begin();i!=end;++i)
		{
			pPass = &(*i);
			pPass->m_cColor=col;
		}
	}
	else
	{
		if(nPass >= (int)m_pCurrentEffect->vPass.size())
			return;
		pPass = &m_pCurrentEffect->vPass[nPass];
		pPass->m_cColor=col;
	}
}

///////////////////////////////////////////////
// Set the characters base size
void CFFont::SetSize(const vector2f &vSize)
{
	m_vSize = vSize;
}


void CFFont::SetProportional(bool bProportional)
{
	m_bProportional = bProportional;
}

bool CFFont::GetProportional() const
{
	return m_bProportional;
}

vector2f &CFFont::GetSize()
{
	return m_vSize;
}


float CFFont::GetCharWidth()
{
	IRenderer *pRenderer = gEnv->pRenderer;
	assert(pRenderer);

	float fMaxW = m_vSize.x;
/*
	if (m_pCurrentEffect)
	{			
		for(int i = m_pCurrentEffect->vPass.size()-1; i >= 0; --i)
		{
			SRenderingPass *Pass = &m_pCurrentEffect->vPass[i];

			float fOffset = Pass->vPosOffset.x;
			float fPassW = m_vSize.x + fOffset;

			if (m_bSizeIn800x600)
				fPassW = pRenderer->ScaleCoordX(m_vSize.x) + fOffset;

			if (!m_bProportional)
				fPassW *= m_fWidthScale;

			if (fPassW > fMaxW)
				fMaxW = fPassW;
		}
	}
*/	
	return fMaxW;
}

float CFFont::GetCharHeight()
{
	IRenderer *pRenderer = gEnv->pRenderer;
	assert(pRenderer);

	float fMaxH = m_vSize.y;
/*
	if (m_pCurrentEffect)
	{
		for(int i = m_pCurrentEffect->vPass.size()-1; i >= 0; --i)
		{
			SRenderingPass *Pass = &m_pCurrentEffect->vPass[i];

			float fOffset = Pass->vPosOffset.y;
			float fPassH = m_vSize.y + fOffset;

			if (m_bSizeIn800x600)
				fPassH = pRenderer->ScaleCoordY(m_vSize.y) + fOffset;

			if (fPassH > fMaxH)
				fMaxH = fPassH;
		}
	}
*/
	return fMaxH;
}

///////////////////////////////////////////////
void CFFont::SetCharWidthScale(float fScale)
{
	m_fWidthScale = fScale;
}

///////////////////////////////////////////////
float CFFont::GetCharWidthScale()
{
	return m_fWidthScale;
}

ILINE DWORD COLCONV (DWORD clr)
{
	return ((clr & 0xff00ff00) | ((clr & 0xff0000)>>16) | ((clr & 0xff)<<16));
}

///////////////////////////////////////////////
// Draw a formated string
void CFFont::DrawString( float fBaseX, float fBaseY, const char *szMsg, const bool bASCIIMultiLine )
{
	if (!szMsg)
	{
		return;
	}

	int iSize = min(1023, (int)strlen(szMsg));

	/*static */wchar_t szwMsg[1024];

	szwMsg[iSize] = 0;
	while (iSize--)
	{
		szwMsg[iSize] = (unsigned char)szMsg[iSize];
	}

	DrawStringW(fBaseX, fBaseY, szwMsg, bASCIIMultiLine);
}

///////////////////////////////////////////////
// Draw a formated string, taking z into account
void CFFont::DrawString( float fBaseX, float fBaseY, float fBaseZ, const char *szMsg, const bool bASCIIMultiLine )
{
	if (!szMsg)
	{
		return;
	}

	int iSize = min(1023, (int)strlen(szMsg));

	/*static */wchar_t szwMsg[1024];

	szwMsg[iSize] = 0;
	while (iSize--)
	{
		szwMsg[iSize] = (unsigned char)szMsg[iSize];
	}

	DrawStringW(fBaseX, fBaseY, fBaseZ, szwMsg, bASCIIMultiLine);
}

void CFFont::DrawStringW(float fBaseX, float fBaseY, const wchar_t *szMsg, const bool bASCIIMultiLine)
{
	DrawStringW(fBaseX,fBaseY,1.0f,szMsg,bASCIIMultiLine);
}

//////////////////////////////////////////////////////////////////////////
void CFFont::FillRenderContext( SFontRenderContext &Context )
{
	Context.m_iTextureID = m_iTextureID;
	Context.m_pFontTexture = m_pFontTexture;
	Context.m_bProportional = m_bProportional;
	Context.m_bSizeIn800x600 = m_bSizeIn800x600;
	Context.m_bFramed = m_bFramed;
	Context.m_bClipEnabled = m_bClipEnabled;
	Context.m_fClipB = m_fClipB;
	Context.m_fClipR = m_fClipR;
	Context.m_fClipX = m_fClipX;
	Context.m_fClipY = m_fClipY;
	Context.m_fWidthScale = m_fWidthScale;
	Context.m_pCurrentEffect = m_pCurrentEffect;
	Context.m_vSize = m_vSize;
	Context.m_pFont = this;
	if (m_pCurrentEffect)
	{
		for (int i = 0,num = min(2,(int)m_pCurrentEffect->vPass.size()); i < num; i++)
			Context.m_cPassColor[i] = m_pCurrentEffect->vPass[i].m_cColor;
	}

}

//////////////////////////////////////////////////////////////////////////
SFontRenderContext *CFFont::GetRenderContext(int& nSize)
{
  static SFontRenderContext Context;
	FillRenderContext(Context);
  nSize = sizeof(SFontRenderContext);
  return &Context;
}

//////////////////////////////////////////////////////////////////////////
int CFFont::SetRenderContext(SFontRenderContext *pContext)
{
	*m_pRenderContext = *pContext;
  return sizeof(SFontRenderContext);
}

void CFFont::DrawStringW(float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine)
{
	// please terminate your strings with a '\0'
	// if you want to draw a string with more than 682 char, tell me (marcio)
	// and will allocate two buffers
	//assert(wcslen(szMsg) <= 682);

	if (!m_pCurrentEffect || m_pCurrentEffect->vPass.empty())
		return;

	IRenderer *pRenderer = gEnv->pRenderer;
	assert(pRenderer);

	if (!szMsg)
	{
		return;
	}
  if (m_iTextureID <= 0)
    RenderInit();

  Prepare(szMsg);

	FillRenderContext(*m_pRenderContext); // This is need if renderer is not multithreaded and do not call set/get render context.

  pRenderer->DrawStringW(this, fBaseX, fBaseY, fBaseZ, szMsg, bASCIIMultiLine);
}

void CFFont::RT_DrawStringW(float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine)
{
	assert(m_pRenderContext);
	m_pRenderContext->RT_DrawStringW( this,fBaseX,fBaseY,fBaseZ,szMsg,bASCIIMultiLine);
}

void SFontRenderContext::RT_DrawStringW(CFFont *pFont,float fBaseX, float fBaseY, float fBaseZ, const wchar_t *szMsg, const bool bASCIIMultiLine)
{
  IRenderer *pRenderer = gEnv->pRenderer;
  assert(pRenderer);

//	float fTexHeight = m_pFontTexture->GetCellHeight() / (float)m_pFontTexture->GetHeight();
	bool	bRGB = (pRenderer->GetFeatures() & RFT_RGBA) != 0;

	SVF_P3F_C4B_T2F *pVertex = 0;
	int		iVertexOffset = 0;
	int		iTextLength = pFont->GetTextLengthW(szMsg);
	float fHalfTexelShift = pRenderer->EF_Query(EFQ_HalfPixelShiftNeeded) ? 0.0f : 0.5f;		// in pixels

	uint32 dwAlphaBlend = m_cPassColor[0].a;		
	{
		if(dwAlphaBlend>128)
			++dwAlphaBlend;		// 0..256 for proper blending
	}

	pRenderer->FontSetTexture(m_iTextureID, FILTER_TRILINEAR);
	pRenderer->FontSetRenderingState(0, 0);

	vector2f vSize;				// in pixel
	if(m_bSizeIn800x600)
	{
		vSize.x = pRenderer->ScaleCoordX(m_vSize.x);
		vSize.y = pRenderer->ScaleCoordY(m_vSize.y);
	}
	else vSize = m_vSize;

	float fRcpCellWidth;
	vector2f vScale;

	if(m_bProportional)
	{
		// to have backward compatible behavior (can be refactored later)
		fRcpCellWidth = (1.0f / (512.0f/16.0f)) * vSize.x;
		vScale = vector2f(fRcpCellWidth*m_fWidthScale,vSize.y/32.0f);
	}
	else
	{
		fRcpCellWidth = vSize.x/16.0f;
		vScale = vector2f(fRcpCellWidth*m_fWidthScale,vSize.y*m_fWidthScale/16.0f);
	}

	vector2f vBaseXY;				// in pixels

	if(m_bSizeIn800x600)
	{
		vBaseXY.x = pRenderer->ScaleCoordX(fBaseX);
		vBaseXY.y = pRenderer->ScaleCoordY(fBaseY);
	}
	else
		vBaseXY = vector2f(fBaseX,fBaseY);

	// snap for pixel perfect rendering (better quality for text)
	{
		vBaseXY.x = floor(vBaseXY.x);
		vBaseXY.y = floor(vBaseXY.y);

		// for smaller fonts (half res or less) it's better to average multiple pixels (we don't miss lines)
		if(vScale.x<0.9f)
			vBaseXY.x += 0.5f;			// try to average two columns (for exact half res)
		if(vScale.y<0.9f)
			vBaseXY.y += 0.25f;			// hand tweaked value to get a good result with tiny font (640x480 underscore in console)
	}

	for(int i = m_pCurrentEffect->vPass.size()-1; i >= 0; --i)
	{
		if (!i)
			dwAlphaBlend = 256;

		ColorB cPassColor = m_cPassColor[((i<2)?i:1)]; // Only 2 pass colors supported

		CFFont::SRenderingPass *Pass = &m_pCurrentEffect->vPass[i];
		
		// gather pass data
		vector2f vOffset = Pass->vPosOffset;		// in pixels

		float fCharX = vBaseXY.x + vOffset.x;			// in pixels
		float fCharY = vBaseXY.y + vOffset.y;			// in pixels

		ColorB cColor = cPassColor;

		bool bDrawFrame = m_bFramed && i==m_pCurrentEffect->vPass.size()-1;

		pRenderer->FontSetBlending(Pass->blendSrc, Pass->blendDest);
		pVertex = (SVF_P3F_C4B_T2F *)pRenderer->GetDynVBPtr(iTextLength * 6 + (bDrawFrame ? 6 : 0), iVertexOffset, 0); //vertex buffer size: text length + additional vertices for the frame when necessary
		assert(pVertex);

		int iVBLen = 0;

		if (bDrawFrame)
		{

			DWORD dwColor = 2768701191U;	//dark grey, 65% opacity
			//{
			//	ColorB cTempColor(7, 7, 7, 165);	//dark grey, 65% opacity
			//	dwColor = cTempColor.pack_abgr8888();
			//	if(!bRGB)
			//		dwColor = COLCONV(dwColor);
			//}

			vector2f vTextSize = m_pFont->GetTextSizeW(szMsg);

			Vec3 v0(fBaseX-3, fBaseY, fBaseZ);
			Vec3 v2(fBaseX+vTextSize.x+2, fBaseY+vTextSize.y, fBaseZ);
			Vec3 v1(v2.x, v0.y, v0.z);		// to avoid float->half conversion
			Vec3 v3(v0.x, v2.y, v0.z);		// to avoid float->half conversion

			Vec2 vGradientUvMin, vGradientUvMax;
			m_pFont->GetGradientTextureCoord(vGradientUvMin.x, vGradientUvMin.y, vGradientUvMax.x, vGradientUvMax.y);

			// define the frame quad
			pVertex[0].xyz = v0;
			pVertex[0].color.dcolor = dwColor;
			pVertex[0].st = Vec2(vGradientUvMin.x,vGradientUvMax.y);

			pVertex[1].xyz = v1;
			pVertex[1].color.dcolor = dwColor;
			pVertex[1].st = pVertex[0].st;

			pVertex[2].xyz = v2;
			pVertex[2].color.dcolor = dwColor;
			pVertex[2].st = pVertex[0].st;

			pVertex[3].xyz = v2;
			pVertex[3].color.dcolor = dwColor;
			pVertex[3].st = pVertex[0].st;

			pVertex[4].xyz = v3;
			pVertex[4].color.dcolor = dwColor;
			pVertex[4].st = pVertex[0].st;

			pVertex[5].xyz = v0;
			pVertex[5].color.dcolor = dwColor;
			pVertex[5].st = pVertex[0].st;

			++iVBLen;
		}

		wchar_t *pcChar = (wchar_t *)szMsg;
		wchar_t ch;

		// parse the string, ignoring control characters
		while (ch = *pcChar++)
		{
			switch(ch)
			{
			case L'\\':
				{
					if (*pcChar != L'n' || !bASCIIMultiLine)
					{
						break;
					}
					++pcChar;
				}
			case L'\n':
				{
					fCharX = vBaseXY.x + vOffset.x;
					fCharY += vSize.y;
					continue;
				}
				break;
			case L'\r':
				{
					fCharX = vBaseXY.x + vOffset.x;

					continue;
				}
				break;
			case L'\t':
				{
					if (m_bProportional)
						fCharX += g_iTabCharCount * vSize.x * FONT_SPACE_SIZE;
					 else
						fCharX += g_iTabCharCount * vSize.x * m_fWidthScale;

					continue;
				}
				break;
			case L' ':
				{
					if (m_bProportional)
						fCharX += vSize.x * FONT_SPACE_SIZE;
					 else
						fCharX += vSize.x * m_fWidthScale;
					
					continue;
				}
				break;
			case L'$':
				{
					if (*pcChar == L'$')
					{
						++pcChar;
					}
					else if(isdigit(*pcChar))
					{
						if (!i)
						{
							int iColorIndex = (*pcChar) - L'0';

							ColorB newColor = g_cColorTable[iColorIndex];
							cColor.r = newColor.r;
							cColor.g = newColor.g;
							cColor.b = newColor.b;
							// Leave alpha at original value!
						}

						++pcChar;

						continue;
					}
					else if (*pcChar == L'O' || *pcChar == L'o')
					{
						if (!i)
							cColor = cPassColor;

						++pcChar;

						continue;
					}
					else if (*pcChar)
					{
						//++pcChar;
						//continue;
					}
				}
				break;
			default:
				break;
			}

			int iCharWidth = m_pFontTexture->GetCharacterWidth(ch);
			float fWidth = iCharWidth * fRcpCellWidth;

			// get texture coordinates
			float vTexCoord[4];

			int iCharOffsetX,iCharOffsetY;		// in font texels
			int iCharSizeX, iCharSizeY;				// in font texels
			m_pFontTexture->GetTextureCoord(m_pFontTexture->GetCharSlot(ch),vTexCoord[0],vTexCoord[1],vTexCoord[2],vTexCoord[3],iCharSizeX,iCharSizeY,iCharOffsetX,iCharOffsetY);

			float fAdvance;

			if(m_bProportional)
			{
				fAdvance = (iCharWidth + FONT_GLYPH_PROP_SPACING) * vScale.x;			// FONT_GLYPH_PROP_SPACING pixel space between characters for proportianl fonts
				iCharOffsetX=0;
			}
			else fAdvance = vSize.x * m_fWidthScale;

			float fX = fCharX + iCharOffsetX*vScale.x;			// in pixels
			float fY = fCharY + iCharOffsetY*vScale.y;			// in pixels


//			float fR = fX + fWidth;
//			float fB = fY + vSize.y;
			float fR = fX + iCharSizeX*vScale.x;
			float fB = fY + iCharSizeY*vScale.y;

			// compute clipping
			float fNewX = fX;			// in pixels
			float fNewY = fY;			// in pixels
			float fNewR = fR;			// in pixels
			float fNewB = fB;			// in pixels
			
			if (m_bClipEnabled)
			{
				// clip non visible 
				if ((fX >= m_fClipR) || (fY >= m_fClipB) || (fR < m_fClipX) || (fB < m_fClipY))
				{
					fCharX += fAdvance;

					continue;
				}
				// clip partially visible
				else
				{
					if ((fWidth <= 0.0f) || (vSize.y <= 0.0f))
					{
						fCharX += fAdvance;

						continue;
					}

					// clip the image to the scissor rect
					fNewX = max(m_fClipX, fX);
					fNewY = max(m_fClipY, fY);
					fNewR = min(m_fClipR, fR);
					fNewB = min(m_fClipB, fB);

					float fRcpWidth = 1.0f / fWidth;
					float fRcpHeight = 1.0f / vSize.y;

					float	fTexW = vTexCoord[2] - vTexCoord[0];
					float fTexH = vTexCoord[3] - vTexCoord[1];

					// clip horizontal
					vTexCoord[0] = vTexCoord[0] + (fTexW * ((fNewX - fX) * fRcpWidth));
					vTexCoord[2] = vTexCoord[2] + (fTexW * ((fNewR - (fX + fWidth)) * fRcpWidth));

					// clip vertical
					vTexCoord[1] = vTexCoord[1] + (fTexH * ((fNewY - fY) * fRcpHeight));
					vTexCoord[3] = vTexCoord[3] + (fTexH * ((fNewB - (fY + vSize.y)) * fRcpHeight));
				}
			}

			fNewX += fHalfTexelShift;
			fNewY += fHalfTexelShift;
			fNewR += fHalfTexelShift;
			fNewB += fHalfTexelShift;

			int iOffset = iVBLen * 6;

      Vec3 v0(fNewX, fNewY, fBaseZ);
      Vec3 v2(fNewR, fNewB, fBaseZ);
      Vec3 v1(v2.x, v0.y, v0.z);		// to avoid float->half conversion
      Vec3 v4(v0.x, v2.y, v0.z);		// to avoid float->half conversion

      Vec2 tc0(vTexCoord[0], vTexCoord[1]);
      Vec2 tc2(vTexCoord[2], vTexCoord[3]);
      Vec2 tc1(tc2.x,tc0.y);		// to avoid float->half conversion
      Vec2 tc3(tc0.x,tc2.y);		// to avoid float->half conversion

			DWORD dwColor = 0;
			{
				ColorB cTempColor = cColor;
				cTempColor.a = ((uint32)cTempColor.a*dwAlphaBlend)>>8;
				dwColor = cTempColor.pack_abgr8888();

				if(!bRGB)
					dwColor = COLCONV(dwColor);
			}

			// define char quad
			pVertex[iOffset].xyz = v0;
			pVertex[iOffset].color.dcolor = dwColor;
			pVertex[iOffset].st = tc0;

			pVertex[iOffset+1].xyz = v1;
			pVertex[iOffset+1].color.dcolor = dwColor;
			pVertex[iOffset+1].st = tc1;

			pVertex[iOffset+2].xyz = v2;
			pVertex[iOffset+2].color.dcolor = dwColor;
			pVertex[iOffset+2].st = tc2;

			pVertex[iOffset+3].xyz = v2;
			pVertex[iOffset+3].color.dcolor = dwColor;
			pVertex[iOffset+3].st = tc2;

			pVertex[iOffset+4].xyz = v4;
			pVertex[iOffset+4].color.dcolor = dwColor;
			pVertex[iOffset+4].st = tc3;

			pVertex[iOffset+5].xyz = v0;
			pVertex[iOffset+5].color.dcolor = dwColor;
			pVertex[iOffset+5].st = tc0;
      iOffset += 6;

			if (iVBLen >= 682)
			{
				break;
			}

			++iVBLen;

			fCharX += fAdvance;
		}

		// draw this pass
		pRenderer->DrawDynVB(iVertexOffset, 0, iVBLen * 6);
	}

	// restore the old states	
	pRenderer->FontRestoreRenderingState();	
}

void CFFont::DrawWrappedStringW( float fBaseX, float fBaseY, float w, const wchar_t *szMsg, const bool bASCIIMultiLine )
{
	wstring szWrapped;

	WrapText(szWrapped, w, szMsg);
	DrawStringW(fBaseX, fBaseY, szWrapped.c_str(), bASCIIMultiLine);
}

vector2f CFFont::GetWrappedTextSizeW(const wchar_t *swStr, float w, const bool bASCIIMultiLine)
{
	wstring szWrapped;

	WrapText(szWrapped, w, swStr);
	return GetTextSizeW(szWrapped.c_str(), bASCIIMultiLine);
}

vector2f CFFont::GetTextSize(const char *szMsg, const bool bASCIIMultiLine)
{
	if (!szMsg)
	{
		return vector2f(0.0f, 0.0f);
	}

	int iSize = min(1023, (int)strlen(szMsg));

	static wchar_t szwMsg[1024];

	szwMsg[iSize] = 0;
	while (iSize--)
	{
		szwMsg[iSize] = (unsigned char)szMsg[iSize];
	}

	return GetTextSizeW(szwMsg,bASCIIMultiLine);
}

///////////////////////////////////////////////
// Compute the text size
vector2f CFFont::GetTextSizeW(const wchar_t *szMsg, const bool bASCIIMultiLine)
{
	IRenderer *pRenderer = gEnv->pRenderer;
	assert(pRenderer);

	if (!szMsg)
		return vector2f(0,0);

  if (m_iTextureID <= 0)
    RenderInit();

  Prepare(szMsg);

	float fMaxW = 0.0f;
	float fMaxH = 0.0f;

	vector2f vSize;						// in pixels
	if(m_bSizeIn800x600)
	{
		vSize.x = pRenderer->ScaleCoordX(m_vSize.x);
		vSize.y = pRenderer->ScaleCoordY(m_vSize.y);
	}
	else vSize = m_vSize;

	float fRcpCellWidth;
	vector2f vScale;

	if(m_bProportional)
	{
		// to have backward compatible behavior (can be refactored later)
		fRcpCellWidth = (1.0f / (512.0f/16.0f)) * vSize.x;
		vScale=vector2f(fRcpCellWidth,vSize.y/32.0f);
	}
	else
	{
		fRcpCellWidth = vSize.x/16.0f;
		vScale=vector2f(fRcpCellWidth*m_fWidthScale,vSize.y*m_fWidthScale/16.0f);
	}

	for(int i = m_pCurrentEffect->vPass.size()-1; i >= 0; --i)
	{
		SRenderingPass *Pass = &m_pCurrentEffect->vPass[i];

		// gather pass data
		vector2f	vOffset = Pass->vPosOffset;

		float			fCharX = vOffset.x;
		float			fCharY = vOffset.y + vSize.y;

		if (fCharY > fMaxH)
			fMaxH = fCharY;

		wchar_t *pcChar = (wchar_t *)szMsg;
		wchar_t ch;

		// parse the string, ignoring control characters
		while (ch = *pcChar++)
		{
			switch(ch)
			{
			case L'\\':
				{
					if (*pcChar != L'n' || !bASCIIMultiLine)
					{
						break;
					}
					++pcChar;
				}
			case L'\n':
				{
					if (fCharX > fMaxW)
					{
						fMaxW = fCharX;
					}

					fCharX = vOffset.x;
					fCharY += vSize.y;

					if (fCharY > fMaxH)
					{
						fMaxH = fCharY;
					}

					continue;
				}
				break;
			case L'\r':
				{
					if (fCharX > fMaxW)
						fMaxW = fCharX;

					fCharX = vOffset.x;
					continue;
				}
				break;
			case L'\t':
				{
					if (m_bProportional)
						fCharX += g_iTabCharCount * vSize.x * FONT_SPACE_SIZE;
					 else
						fCharX += g_iTabCharCount * vSize.x * m_fWidthScale;

					continue;
				}
				break;
			case L' ':
				{
					fCharX += FONT_SPACE_SIZE * vSize.x;
					continue;
				}
				break;
			case L'$':
				{
					if (*pcChar == L'$')
					{
						++pcChar;
					}
					else if (*pcChar)
					{
						++pcChar;

						continue;
					}
				}
				break;
			default:
				break;
			}

			float fAdvance;

			if(m_bProportional)
			{
				int iCharWidth = m_pFontTexture->GetCharacterWidth(ch);
				fAdvance = (iCharWidth + FONT_GLYPH_PROP_SPACING) * vScale.x;			// FONT_GLYPH_PROP_SPACING pixel space between characters for proportianl fonts
			}
			else fAdvance = vSize.x * m_fWidthScale;

			fCharX += fAdvance;
		}

		if (fCharX > fMaxW)
			fMaxW = fCharX;
	}

	return vector2f(fMaxW, fMaxH);
}

///////////////////////////////////////////////
int CFFont::GetTextLengthW(const wchar_t *szMsg, const bool bASCIIMultiLine)
{
	int iLength = 0;

	wchar_t *pcChar = (wchar_t *)szMsg;
	wchar_t ch;

	// parse the string, ignoring control characters
	while (ch = *pcChar++)
	{
		switch(ch)
		{
			case L'\\':
				{
					if (*pcChar != L'n' || !bASCIIMultiLine)
					{
						break;
					}
					++pcChar;
				}
			case L'\n':
			case L'\r':
			case L'\t':
				{
					continue;
				}
				break;
			case L'$':
				{
					if (*pcChar == L'$')
					{
						++pcChar;
					}
					else if (*pcChar)
					{
						++pcChar;

						continue;
					}
				}
				break;
			default:
				break;
		}
		++iLength;
	}

	return iLength;
}

///////////////////////////////////////////////
int CFFont::GetTextLength(const char *szMsg, const bool bASCIIMultiLine)
{
	int iLength = 0;

	char *pcChar = (char *)szMsg;
	char ch;

	// parse the string, ignoring control characters
	while (ch = *pcChar++)
	{
		switch(ch)
		{
		case '\\':
			{
				if (*pcChar != L'n' || !bASCIIMultiLine)
				{
					break;
				}
				++pcChar;
			}
		case '\n':
		case '\r':
		case '\t':
			{
				continue;
			}
			break;
		case '$':
			{
				if (*pcChar == L'$')
				{
					++pcChar;
				}
				else if (*pcChar)
				{
					++pcChar;

					continue;
				}
			}
			break;
		default:
			break;
		}
		++iLength;
	}

	return iLength;
}

///////////////////////////////////////////////
// Push a new effect in the vector and return a pointer to it
CFFont::SEffect* CFFont::NewEffect()
{
	SEffect effect;
	m_vEffects.push_back(effect);
	return &m_vEffects[m_vEffects.size()-1];
}

///////////////////////////////////////////////
CFFont::SEffect* CFFont::GetCurrentEffect() const
{
	return m_pCurrentEffect;
}

///////////////////////////////////////////////
bool CFFont::RenderInit()
{
	m_iTextureID = gEnv->pRenderer->FontCreateTexture(
		m_pFontTexture->GetWidth(), m_pFontTexture->GetHeight(), (byte*)m_pFontTexture->GetBuffer(), eTF_A8);

	if (m_iTextureID < 0)
		return false;

	m_pFontTexture->CreateGradientSlot();

	// precache (not required but for faster printout later)
	{
		wchar_t szCH[256],*p=szCH;

		int i = (wchar_t)' ';			// even space should be there - otherwise it's created later on demand

		// precache all [normal] printable characters to the string (missing once are updated on demand)
		for(; i<=(wchar_t)'~'; i++)
			*p++ = i;

		i+=35;

		for(; i<=(int)255; i++)
			*p++ = i;

		*p = 0;
		Prepare(szCH);
	}

	return true;
}

///////////////////////////////////////////////
void CFFont::RenderCleanup()
{
	if (m_iTextureID > -1 && gEnv->pRenderer)
		gEnv->pRenderer->RemoveTexture(m_iTextureID);

	SAFE_RELEASE(m_pFontTexture);
}

void CFFont::GetMemoryUsage (class ICrySizer* pSizer) const
{
	pSizer->AddObject(m_sName);
	pSizer->AddObject(m_sCurPath);
	pSizer->AddObject(m_pRenderContext);
	pSizer->AddObject(m_pFontTexture);
#ifndef USE_VIRT_MEM
	pSizer->AddObject(m_pFontBuffer, m_nFontBufferSize);
#endif
	pSizer->AddObject(m_vEffects);	
}

//------------------------------------------------------------------------------------------------- 
void CFFont::Prepare(const wchar_t *szString)
{
	if (m_pFontTexture->PreCacheString(szString) == 1)
	{
		gEnv->pRenderer->FontUpdateTexture(m_iTextureID, 0, 0, m_pFontTexture->GetWidth(), m_pFontTexture->GetHeight(), (unsigned char *)m_pFontTexture->GetBuffer());
	}
}

//------------------------------------------------------------------------------------------------- 
void CFFont::WrapText(wstring &szResult, float fMaxWidth, const wchar_t *szString)
{
	szResult = szString;

	if (m_bSizeIn800x600)
	{
		fMaxWidth = gEnv->pRenderer->ScaleCoordX(fMaxWidth);
	}

	vector2f vStringSize = GetTextSizeW(szResult.c_str());

	if (vStringSize.x <= fMaxWidth)
	{
		return;
	}

	int		iLastSpace = -1;
	float	fLastSpaceWidth = 0.0f;

	float fCurrentCharWidth = 0.0f;
	float fCurrentLineWidth = 0.0f;
	float fBiggestLineWidth = 0.0f;
	float fWidthSum = 0.0f;

	int				iCurrentChar = 0;
	wchar_t		*pChar = (wchar_t *)szResult.c_str();
	wchar_t		szChar[2] = {0, 0};

	while(szChar[0] = *pChar++)
	{
		// ignore color codes
		if (szChar[0] == L'$')
		{
			if (*pChar)
			{
				++pChar;
				++iCurrentChar;

				if ((*pChar) != L'$')
				{
					++iCurrentChar;

					continue;
				}
				szChar[0] = *pChar;
			}
		}

		// get char width and sum it to the line width
		fCurrentCharWidth = GetTextSizeW(szChar).x;

		// keep track of spaces
		// they are good for spliting the string :D
		if (szChar[0] == L' ')
		{
			iLastSpace = iCurrentChar;
			fLastSpaceWidth = fCurrentLineWidth + fCurrentCharWidth;
		}

		// if line exceed allowed width, split it
		if ((fCurrentLineWidth + fCurrentCharWidth >= fMaxWidth) && (*pChar))
		{
			if ((iLastSpace > 0) && ((iCurrentChar - iLastSpace) < 16) && (iCurrentChar - iLastSpace > 0)) // 16 is the default treshold
			{
#if 0 // this inserts a newline, but the next line will start with a space...
				szResult.insert(iLastSpace + 1, 1, L'\n');

				++iCurrentChar;
#else // this replaces the space with a newline
				pChar = (wchar_t *)(szResult.c_str());
				pChar[iLastSpace] = L'\n';
#endif
				pChar = (wchar_t *)(szResult.c_str() + iCurrentChar + 1);

				if (fLastSpaceWidth > fBiggestLineWidth)
				{
					fBiggestLineWidth = fLastSpaceWidth;
				}

				fCurrentLineWidth = fCurrentLineWidth - fLastSpaceWidth + fCurrentCharWidth;
				fWidthSum += fCurrentLineWidth;
			}
			else
			{
#if 0 // this inserts a newline, but the next line will start with a space...
				szResult.insert(iCurrentChar, 1, L'\n');

				++iCurrentChar;
#else // this replaces the space with a newline
				pChar = (wchar_t *)(szResult.c_str());
				pChar[iCurrentChar] = L'\n';
#endif
				pChar = (wchar_t *)(szResult.c_str() + iCurrentChar + 1);

				if (fCurrentLineWidth > fBiggestLineWidth)
				{
					fBiggestLineWidth = fCurrentLineWidth;
				}

				fWidthSum += fCurrentLineWidth;
				fCurrentLineWidth = fCurrentCharWidth;
			}

			// if we don't need any more line breaks, then just stop
			if (vStringSize.x - fWidthSum <= fMaxWidth)
			{
				break;
			}

			fLastSpaceWidth = 0;
			iLastSpace = 0;
		}
		else
		{
			fCurrentLineWidth += fCurrentCharWidth;
		}

		++iCurrentChar;
	}
}


void CFFont::GetGradientTextureCoord( float &fMinU, float &fMinV, float &fMaxU, float &fMaxV ) const
{
	const CTextureSlot *pSlot = m_pFontTexture->GetGradientSlot();		assert(pSlot);

	float fInvWidth = 1.0f/(float)m_pFontTexture->GetWidth();
	float fInvHeight = 1.0f/(float)m_pFontTexture->GetHeight();

	// deflate by one pixel to avoid bilinear filtering on the borders
	fMinU = pSlot->vTexCoord[0]+fInvWidth;
	fMinV = pSlot->vTexCoord[1]+fInvHeight;
	fMaxU = pSlot->vTexCoord[0]+(pSlot->iCharWidth-1)*fInvWidth;
	fMaxV = pSlot->vTexCoord[1]+(pSlot->iCharHeight-1)*fInvHeight;
}

