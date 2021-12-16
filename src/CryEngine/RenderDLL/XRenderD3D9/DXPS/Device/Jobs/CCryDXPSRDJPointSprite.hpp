#ifndef __CRYDXPSRDJPOINTSPRITE__
#define __CRYDXPSRDJPOINTSPRITE__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJPointSprite
{
protected:
	uint32				m_Enable;
	uint32				m_RMode;
	uint32				m_TexCoord;
} _ALIGN(DXPS_DEVICECMDBALLIGN);


class CDXPSRDJPointSpriteSPU	:	public CDXPSRDJPointSprite
{
public:
	uint32				Enable()	const{return m_Enable;}
	uint32				RMode()		const{return m_RMode;}
	uint32				TexCoord()const{return m_TexCoord;}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJPointSpritePPU	:	public CDXPSRDJPointSprite
{
public:
	uint32				Setup(uint32 Enable, uint32	RMode, uint32 TexCoord)
								{
									m_Enable = Enable; 
									m_RMode = RMode; 
									m_TexCoord = TexCoord;

									JOBRETURN(EDXPSJ_POINTSPRITE);
								}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

