#ifndef __CRYDXPSRDJCOPYRESOURCE__
#define __CRYDXPSRDJCOPYRESOURCE__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJCopyResource
{
protected:
	ID3D11Resource*		m_pDstResource;
	ID3D11Resource*		m_pSrcResource;
	uint32						m_DstSubresource;
	uint32						m_SrcSubresource;
	uint32						m_DstX;
	uint32						m_DstY;
	uint32						m_DstZ;
	uint32						m_bHasSrcBox;
	D3D11_BOX					m_SrcBox;
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32						m_SrcVBIdx;
	uint32						m_DstVBIdx;
#endif
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJCopyResourceSPU	:	public CDXPSRDJCopyResource
{
public:
	const D3D11_BOX*				SrcBox()const {return m_bHasSrcBox ? &m_SrcBox : NULL;}
	const ID3D11Resource*		DstResource()const {return m_pDstResource;}
	const ID3D11Resource*		SrcResource()const {return m_pSrcResource;}
	uint32									DstSubresource()const {return m_DstSubresource;}
	uint32									SrcSubresource()const {return m_SrcSubresource;}
	uint32									DstX()const {return m_DstX;}
	uint32									DstY()const {return m_DstY;}
	uint32									DstZ()const {return m_DstZ;}
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32									SrcVBIdx()const{return m_SrcVBIdx;}
	uint32									DstVBIdx()const{return m_DstVBIdx;}
#endif
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJCopyResourcePPU	:	public CDXPSRDJCopyResource
{
public:
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32						Setup(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox,uint32 SrcVBIdx,uint32 DstVBIdx)
										{
											m_SrcVBIdx					=	SrcVBIdx;
											m_DstVBIdx					=	DstVBIdx;
#else
	uint32						Setup(ID3D11Resource *pDstResource,uint32 DstSubresource,uint32 DstX,uint32 DstY,uint32 DstZ,ID3D11Resource *pSrcResource,uint32 SrcSubresource,const D3D11_BOX *pSrcBox)
										{
#endif
											if(pSrcBox)
												m_SrcBox					=	*pSrcBox;
											m_bHasSrcBox				= pSrcBox != NULL;
											m_pDstResource			=	pDstResource;
											m_pSrcResource			=	pSrcResource;
											m_DstSubresource		=	DstSubresource;
											m_SrcSubresource		=	SrcSubresource;
											m_DstX							=	DstX;
											m_DstY							=	DstY;
											m_DstZ							=	DstZ;
											JOBRETURN(EDXPSJ_COPYRESOURCE);
										}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

