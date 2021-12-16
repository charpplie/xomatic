//////////////////////////////////////////////////////////////////////////
//
// Precompiled header.
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

// Shell utility library
#pragma comment(lib, "Shlwapi.lib")

// even in Release mode, the editor will return its heap, because there's no Profile build configuration for the editor
#ifdef _RELEASE
#undef _RELEASE
#endif
#include <CrtDebugStats.h>
 

BOOL SetDefaultDllDirectories(
	DWORD DirectoryFlags
);

HMODULE LoadLibraryExA(
	LPCSTR lpLibFileName,
	HANDLE hFile,
	DWORD  dwFlags
);

#define LDR_IS_DATAFILE(handle)      (((ULONG_PTR)(handle)) &  (ULONG_PTR)1)
#define LDR_IS_IMAGEMAPPING(handle)  (((ULONG_PTR)(handle)) & (ULONG_PTR)2)
#define LDR_IS_RESOURCE(handle)      (LDR_IS_IMAGEMAPPING(handle) || LDR_IS_DATAFILE(handle))

#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800