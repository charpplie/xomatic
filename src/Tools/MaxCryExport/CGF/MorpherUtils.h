#ifndef _MAX_EXPORTER_MORPHER_UTILS_HDR_
#define _MAX_EXPORTER_MORPHER_UTILS_HDR_

#undef IDC_LOAD
#undef IDC_SAVE
#undef IDC_DELETE

#if (MAX_PRODUCT_VERSION_MAJOR == 12)
#	include "../Morpher12/Include/wm3.h"
#elif (MAX_PRODUCT_VERSION_MAJOR == 11)
#	include "../Morpher11/Include/wm3.h"
#elif (MAX_PRODUCT_VERSION_MAJOR == 10)
#	include "../Morpher10/Include/wm3.h"
#elif (MAX_PRODUCT_VERSION_MAJOR == 9)
#	include "../Morpher9/Include/wm3.h"
#elif (MAX_PRODUCT_VERSION_MAJOR == 8)
#	include "../Morpher8/Include/wm3.h"
#else
#	error Unsupported Max version (not 8, 9, 10, 11 or 12)
#endif


#endif