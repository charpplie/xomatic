#ifndef __CRYDXPSRDJCOPY__
#define __CRYDXPSRDJCOPY__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJCopy
{
protected:
			void*				m_pDst;
			uint32 			m_DstSizeX;
			uint32 			m_DstSizeY;
			uint32 			m_DstPitch;
			const void* m_pSrc;
			uint32 			m_SrcSizeX;
			uint32 			m_SrcSizeY;
			uint32 			m_SrcPitch;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJCopySPU	:	public CDXPSRDJCopy
{
public:
			const void*	Dst()const{return m_pDst;}
			uint32 			DstSizeX()const{return m_DstSizeX;}
			uint32 			DstSizeY()const{return m_DstSizeY;}
			uint32 			DstPitch()const{return m_DstPitch;}
			const void* Src()const{return m_pSrc;}
			uint32 			SrcSizeX()const{return m_SrcSizeX;}
			uint32 			SrcSizeY()const{return m_SrcSizeY;}
			uint32 			SrcPitch()const{return m_SrcPitch;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJCopyPPU	:	public CDXPSRDJCopy
{
public:
	uint32						Setup(			void* pDst,uint32 DstSizeX,uint32 DstSizeY,uint32 DstPitch,
													const void* pSrc,uint32 SrcSizeX,uint32 SrcSizeY,uint32 SrcPitch)
										{
											m_pDst			=	pDst;
											m_DstSizeX	=	DstSizeX;
											m_DstSizeY	=	DstSizeY;
											m_DstPitch	=	DstPitch;
											m_pSrc			=	pSrc;
											m_SrcSizeX	=	SrcSizeX;
											m_SrcSizeY	=	SrcSizeY;
											m_SrcPitch	=	SrcPitch;
											JOBRETURN(EDXPSJ_COPY);
										}
	uint32						Setup(void* pDst,const void* pSrc,uint32 Size)
										{
											m_pDst			=	pDst;
											m_pSrc			=	pSrc;
											m_SrcPitch	=	Size;
											m_DstPitch	=	
											m_DstSizeX	=	
											m_DstSizeY	=	
											m_SrcSizeX	=	
											m_SrcSizeY	=	0;
											JOBRETURN(EDXPSJ_COPY);
										}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

