#ifndef __CRYDXPSRDJVIEWPORT__
#define __CRYDXPSRDJVIEWPORT__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJViewPort
{
protected:
	D3D11_VIEWPORT		m_ViewPort;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJViewPortSPU	:	public CDXPSRDJViewPort
{
public:
	const D3D11_VIEWPORT&		ViewPort()const{return m_ViewPort;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJViewPortPPU	:	public CDXPSRDJViewPort
{
public:
	uint32				Setup(const D3D11_VIEWPORT& rViewPort)
								{
									m_ViewPort		=	rViewPort;
									JOBRETURN(EDXPSJ_VIEWPORT);
								}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

