#ifndef __CRYDXPSRDJDUMMY__
#define __CRYDXPSRDJDUMMY__

#include "CCryDXPSRDJob.hpp"

class CDXPSRDJDummy
{
protected:
	uint32 dummy;
} _ALIGN(DXPS_DEVICECMDBALLIGN);

class CDXPSRDJDummySPU	:	public CDXPSRDJDummy
{
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#if !defined(__SPU__)
class CDXPSRDJDummyPPU	:	public CDXPSRDJDummy
{
public:
	uint32				Setup(EDXPSJob Job)
								{
									JOBRETURN(Job);
								}
} _ALIGN(DXPS_DEVICECMDBALLIGN);

#endif


#endif

