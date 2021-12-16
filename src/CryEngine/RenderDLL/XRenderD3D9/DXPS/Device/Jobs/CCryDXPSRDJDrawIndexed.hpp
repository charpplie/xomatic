#ifndef __CRYDXPSRDJDRAWINDEXED__
#define __CRYDXPSRDJDRAWINDEXED__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJDrawIndexed
{
protected:
	uint32							m_IndexCount;
	uint32							m_StartIndexLocation;
	uint32							m_InvalidFilteringFlag;
	int32								m_BaseVertexLocation;
	CCryDXPSBuffer*			m_pIndexBuffer;
	uint32							m_IBFormat;
	uint32							m_IBOffset;
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32							m_IBIdx;
#endif
	SRegisteredZWrite		m_ZWrite;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJDrawIndexedSPU	:	public CDXPSRDJDrawIndexed
{
public:
	uint32										IndexCount()const{return m_IndexCount;}
	uint32										StartIndexLocation()const{return m_StartIndexLocation;}
	int32											BaseVertexLocation()const{return m_BaseVertexLocation;}
	uint32										InvalidFilteringFlag()const{return m_InvalidFilteringFlag;}
	const CCryDXPSBuffer*			IndexBuffer()const {return m_pIndexBuffer;}
	uint32										IBFormat()const{return m_IBFormat;}
	uint32										IBOffset()const{return m_IBOffset;}
	const SRegisteredZWrite&	ZWrite()const{return m_ZWrite;}
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32										IBIdx()const{return m_IBIdx;}
#endif
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJDrawIndexedPPU	:	public CDXPSRDJDrawIndexed
{
public:
#if defined(CRY_DXPS_DEVICETHREAD_DOUBLEBUFFERING)
	uint32							Setup(uint32 IndexCount,uint32 StartIndexLocation,int32 BaseVertexLocation,uint32 InvalidFilteringFlag,CCryDXPSBuffer* pIndexBuffer,uint32 IBFormat,uint32 IBOffset,const SRegisteredZWrite& ZWrite,uint32 IBIdx)
											{
												m_IBIdx	=	IBIdx;
#else
	uint32							Setup(uint32 IndexCount,uint32 StartIndexLocation,int32 BaseVertexLocation,uint32 InvalidFilteringFlag,CCryDXPSBuffer* pIndexBuffer,uint32 IBFormat,uint32 IBOffset,const SRegisteredZWrite& ZWrite)
											{
#endif
												m_IndexCount					=	IndexCount;
												m_StartIndexLocation	=	StartIndexLocation;
												m_BaseVertexLocation	=	BaseVertexLocation;
												m_InvalidFilteringFlag=	InvalidFilteringFlag;
												m_pIndexBuffer				=	pIndexBuffer;
												m_IBFormat						=	IBFormat;
												m_IBOffset						=	IBOffset;
												m_ZWrite							= ZWrite;
												JOBRETURN(EDXPSJ_DRAWINDEXED);
											}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

