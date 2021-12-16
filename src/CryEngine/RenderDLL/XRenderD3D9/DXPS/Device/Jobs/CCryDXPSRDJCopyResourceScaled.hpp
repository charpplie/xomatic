#ifndef __CRYDXPSRDJCOPYRESOURCESCALED__
#define __CRYDXPSRDJCOPYRESOURCESCALED__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJCopyResourceScaled
{
protected:
	CCryDXPSTexture2D*	m_pDst;
	CCryDXPSTexture2D*	m_pSrc;
	D3D11_BOX						m_DstBox;
	D3D11_BOX						m_SrcBox;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJCopyResourceScaledSPU	:	public CDXPSRDJCopyResourceScaled
{
public:
	const CCryDXPSTexture2D*	Dst()const{return m_pDst;}
	const CCryDXPSTexture2D*	Src()const{return m_pSrc;}
	const D3D11_BOX&					DstBox()const{return m_DstBox;}
	const D3D11_BOX&					SrcBox()const{return m_SrcBox;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJCopyResourceScaledPPU	:	public CDXPSRDJCopyResourceScaled
{
public:
	uint32							Setup(CCryDXPSTexture2D *pDst,const D3D11_BOX *pDstBox,CCryDXPSTexture2D *pSrc,const D3D11_BOX *pSrcBox)
											{
												m_pDst						=	pDst;
												m_pSrc						=	pSrc;
												m_DstBox					=	*pDstBox;
												m_SrcBox					=	*pSrcBox;
												JOBRETURN(EDXPSJ_COPYRESOURCESCALED);
											}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

