#ifndef __CRYDXPSRDJSWAP__
#define __CRYDXPSRDJSWAP__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJSwap
{
protected:
	uint32					m_Frame;
	APWeakTexture2D m_pTexBack;
	APWeakTexture2D m_pTexFront;
	int							m_FlipMode;
	uint32					m_FlipFunc;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJSwapSPU	:	public CDXPSRDJSwap
{
public:
	const uint32		Frame()const{return m_Frame;}
	APWeakTexture2D TexBack()const{return m_pTexBack;}
	APWeakTexture2D TexFront()const{return m_pTexFront;}
	int						FlipMode()const{return m_FlipMode;}
	uint32				FlipFunc()const{return m_FlipFunc;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJSwapPPU	:	public CDXPSRDJSwap
{
public:
	uint32				Setup(uint32 F,APWeakTexture2D pTexBack,APWeakTexture2D pTexFront, int FlipMode, uint32 FlipFunc)
								{
									m_Frame					=	F;
									m_pTexBack			=	pTexBack;
									m_pTexFront			=	pTexFront;
									m_FlipMode			= FlipMode;
									m_FlipFunc			= FlipFunc;
									JOBRETURN(EDXPSJ_SWAP);
								}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

