// nvDXTLibrary.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "nvDXTLibrary.h"


// This is an example of an exported variable
NVDXTLIBRARY_API int nnvDXTLibrary=0;

// This is an example of an exported function.
NVDXTLIBRARY_API int fnnvDXTLibrary(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see nvDXTLibrary.h for the class definition
CnvDXTLibrary::CnvDXTLibrary()
{
	return;
}
