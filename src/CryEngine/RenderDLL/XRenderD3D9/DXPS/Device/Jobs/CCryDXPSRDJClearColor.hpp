#ifndef __CRYDXPSRDJCLEARCOLOR__
#define __CRYDXPSRDJCLEARCOLOR__

#include "CCryDXPSRDJob.hpp"

////////////////////////////////////////////////////
// clear using RSX
////////////////////////////////////////////////////
class CDXPSRDJClearColor
{
protected:
	uint32					m_Color;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJClearColorSPU	:	public CDXPSRDJClearColor
{
public:
	const uint32		Color()const{return m_Color;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJClearColorPPU	:	public CDXPSRDJClearColor
{
public:
	uint32					Setup(const float Color[4])
									{
										int R=static_cast<int>(Color[0]*255.f);
										int G=static_cast<int>(Color[1]*255.f);
										int B=static_cast<int>(Color[2]*255.f);
										int A=static_cast<int>(Color[3]*255.f);
										R	=	R>255?255:(R<0)?0:R;
										G	=	G>255?255:(G<0)?0:G;
										B	=	B>255?255:(B<0)?0:B;
										A	=	A>255?255:(A<0)?0:A;
										m_Color=B|(G<<8)|(R<<16)|(A<<24);

										JOBRETURN(EDXPSJ_CLEARCOLOR);
									}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif

////////////////////////////////////////////////////
// clear using SPU
////////////////////////////////////////////////////
class CDXPSRDJClearTexture
{
protected:
	const	CCryDXPSTexture2D*	m_pTex;
	uint32										m_Color;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJClearTextureSPU	:	public CDXPSRDJClearTexture
{
public:
	const CCryDXPSTexture2D*	Tex()const{return m_pTex;}
	const uint32							Color()const{return m_Color;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJClearTexturePPU	:	public CDXPSRDJClearTexture
{
public:
	uint32				Setup(const CCryDXPSTexture2D* pTex,uint32 Color)
								{
									m_pTex	=	pTex;
									m_Color	=	Color;
									JOBRETURN(EDXPSJ_CLEARTEXTURE);
								}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif

#endif

