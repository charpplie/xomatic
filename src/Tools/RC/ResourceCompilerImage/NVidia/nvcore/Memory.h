// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_MEMORY_H
#define NV_CORE_MEMORY_H

#include <nvcore/nvcore.h>

#include <stdlib.h> // malloc(), realloc() and free()
#include <stddef.h>	// size_t

#include <new>	// new and delete

// Custom memory allocator
namespace nv
{
	namespace mem 
	{
		NVCORE_API void * malloc_nv(size_t size);
		NVCORE_API void * malloc_nv(size_t size, const char * file, int line);
		
		NVCORE_API void free_nv(const void * ptr);
		NVCORE_API void * realloc_nv(void * ptr, size_t size);
		
	} // mem namespace
	
} // nv namespace


// Override new/delete

inline void * operator new (size_t size) throw()
{
	return nv::mem::malloc_nv(size); 
}

inline void operator delete (void *p) throw()
{
	nv::mem::free_nv(p); 
}

inline void * operator new [] (size_t size) throw()
{
	return nv::mem::malloc_nv(size);
}

inline void operator delete [] (void * p) throw()
{
	nv::mem::free_nv(p); 
}

/*
#ifdef _DEBUG
#define new new(__FILE__, __LINE__)
#define malloc(i) malloc(i, __FILE__, __LINE__)
#endif
*/

































































































































#endif // NV_CORE_MEMORY_H
