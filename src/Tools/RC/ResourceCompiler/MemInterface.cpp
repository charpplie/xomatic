// ResourceCompiler.cpp : Memory management functions used by CryMemoryManager 
//												it looks for them first in the executable, then in CrySystem.dll

// TEMPORARY FIX - define this macro to allow app to be built without using VS2005 SP1 CRT libraries - we
// want to continue using the pre-SP1 libraries (at least for now), since non-programmers don't have the new
// libraries as yet.

#include <malloc.h>

extern "C" {
	__declspec(dllexport) void *CryMalloc(size_t size) { return malloc(size); }
	__declspec(dllexport) void *CryRealloc(void *memblock, size_t size) { return realloc(memblock,size); }
	__declspec(dllexport) void *CryReallocSize(void *memblock, size_t oldsize, size_t size) { return realloc(memblock,size); }
	__declspec(dllexport) void CryFree(void *p) { free(p); }
	__declspec(dllexport) void CryFreeSize(void *p, size_t size) { free(p); }
}