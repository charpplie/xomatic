#ifndef __CRYDXPSRDJSRENDERTARGET__
#define __CRYDXPSRDJSRENDERTARGET__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJRenderTarget
{
protected:
	APWeakTexture2D		m_pRenderTargetViews0;
	APWeakTexture2D		m_pRenderTargetViews1;
	APWeakTexture2D		m_pRenderTargetViews2;
	APWeakTexture2D		m_pRenderTargetViews3;
	APWeakTexture2D		m_pDepthStencilView;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJRenderTargetSPU	:	public CDXPSRDJRenderTarget
{
public:
	APWeakTexture2D		RenderTargetViews0()const{return m_pRenderTargetViews0;}
	APWeakTexture2D		RenderTargetViews1()const{return m_pRenderTargetViews1;}
	APWeakTexture2D		RenderTargetViews2()const{return m_pRenderTargetViews2;}
	APWeakTexture2D		RenderTargetViews3()const{return m_pRenderTargetViews3;}
	APWeakTexture2D		DepthStencilView()const{return m_pDepthStencilView;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJRenderTargetPPU	:	public CDXPSRDJRenderTarget
{
public:
	uint32				Setup(APWeakTexture2D	pRenderTargetViews0,
											APWeakTexture2D	pRenderTargetViews1,
											APWeakTexture2D	pRenderTargetViews2,
											APWeakTexture2D	pRenderTargetViews3,
											APWeakTexture2D	pDepthStencilView,
											uint32 AAMode,uint32 ATC)
								{
										m_pRenderTargetViews0	=	pRenderTargetViews0;
										m_pRenderTargetViews1	=	pRenderTargetViews1;
										m_pRenderTargetViews2	=	pRenderTargetViews2;
										m_pRenderTargetViews3	=	pRenderTargetViews3;
										m_pDepthStencilView		=	pDepthStencilView;
									JOBRETURN(static_cast<EDXPSJob>(EDXPSJ_RENDERTARGET1X+AAMode+(ATC*3)));
								}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

