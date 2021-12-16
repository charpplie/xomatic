#ifndef __CRYDXPSRDJCLEARDEPTHSTENCIL__
#define __CRYDXPSRDJCLEARDEPTHSTENCIL__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJClearDepthStencil
{
protected:
	uint32			m_ClearValue;
	uint32			m_ClearFlags;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJClearDepthStencilSPU	:	public CDXPSRDJClearDepthStencil
{
public:
	uint32			ClearFlags()const{return m_ClearFlags;}
	uint32			ClearValue()const{return m_ClearValue;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJClearDepthStencilPPU	:	public CDXPSRDJClearDepthStencil
{
public:
	uint32			Setup(uint32 ClearFlags,float Depth,uint8 Stencil)
							{
								m_ClearValue	=	static_cast<int32>(Depth*static_cast<float>(1<<24));
								m_ClearValue	=	((m_ClearValue>0xffffff?0xffffff:m_ClearValue<0?0:m_ClearValue)<<8)|Stencil;
								m_ClearFlags	=	ClearFlags;
								JOBRETURN(EDXPSJ_CLEARDEPTHSTENCIL);
							}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

