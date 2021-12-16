#include "StdAfx.h"


#ifndef EXCLUDE_SCALEFORM_SDK


#include "GAllocatorCryMem.h"


#if defined(PS3)
static const uint32 PageSizePS3 = 0x10000U;
#endif


static inline size_t GetSize(void* pMem)
{
#if !defined(_DEBUG) && !defined(NOT_USE_CRY_MEMORY_MANAGER)
	return CryGetMemSize(pMem, 0);
#else
	return 0;
#endif
}


GSysAllocCryMem::GSysAllocCryMem()
{
	memset(&m_stats, 0, sizeof(m_stats));
}


GSysAllocCryMem::~GSysAllocCryMem()
{
}


void GSysAllocCryMem::GetInfo(Info* i) const
{
	if (i)
	{
#if defined(PS3)
		i->MinAlign = PageSizePS3;
		i->MaxAlign = PageSizePS3;
		i->Granularity = PageSizePS3;
#else
		i->MinAlign = 1;
		i->MaxAlign = 1;
		i->Granularity = 128 * 1024;
#endif
		i->HasRealloc = false;
	}
}


void* GSysAllocCryMem::Alloc(UPInt size, UPInt align)
{
#if defined(PS3)
	size = (size + PageSizePS3 - 1) & ~(PageSizePS3 - 1);
#endif
	void* ptr = malloc(size); // ignore align since MaxAlign is 1
#if !defined(_DEBUG) && !defined(NOT_USE_CRY_MEMORY_MANAGER)
	++m_stats.AllocCount;
	if (ptr)
		m_stats.Allocated += ::GetSize(ptr);
#endif
#if defined(PS3_CRYSIZER_HEAP_TRAVERSAL)
	m_memops.insert( MemOp(ptr,size) );
#endif
	return ptr;
}


bool GSysAllocCryMem::Free(void* ptr, UPInt size, UPInt align)
{
#if !defined(_DEBUG) && !defined(NOT_USE_CRY_MEMORY_MANAGER)
	++m_stats.FreeCount;
	if (ptr)
		m_stats.Allocated -= ::GetSize(ptr);
#endif
#if defined(PS3_CRYSIZER_HEAP_TRAVERSAL)
	std::set<MemOp>::iterator it = m_memops.find( MemOp(ptr) );
	m_memops.erase(it);
#endif
	free(ptr);
	return true;
}


const GSysAllocCryMem::Stats& GSysAllocCryMem::GetStats() const
{
	return m_stats;
}

void GSysAllocCryMem::GetMemoryUsage( ICrySizer *pSizer ) const
{
#if defined(PS3_CRYSIZER_HEAP_TRAVERSAL)
	pSizer->AddObject(m_memops);
#else
	pSizer->AddObject(this, m_stats.Allocated);
#endif
}

#endif // #ifndef EXCLUDE_SCALEFORM_SDK