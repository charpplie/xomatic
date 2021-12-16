#pragma once

#include <vector> //Diesel cut VS2010


#define DONT_USE_CRY_MEMORY_MANAGER //to avoid the dependency to CryEngine
#if defined(_DEBUG)
//	#define _CRTDBG_MAP_ALLOC
	#define CRTDBG_MAP_ALLOC
#endif
