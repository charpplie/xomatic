#ifndef __CREFHELPERS_H__
#define __CREFHELPERS_H__

#include <xsi_ref.h>

namespace CRefHelpers
{
	inline void* ToPointer(const XSI::CRef& ref) {return *(void**)&ref;}
	inline XSI::CRef ToRef(void* p) {return *(XSI::CRef*)&p;}
}

#endif //__CREFHELPERS_H__
