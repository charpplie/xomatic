/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A wrapper around Scaleform's GSysAlloc interface to delegate memory allocations to CryMemoryManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _GALLOCATOR_CRYMEM_H_
#define _GALLOCATOR_CRYMEM_H_

#pragma once


#ifndef EXCLUDE_SCALEFORM_SDK


#pragma warning(push)
#pragma warning(disable : 6326) // Potential comparison of a constant with another constant
#pragma warning(disable : 6011) // Dereferencing NULL pointer
#include <GSysAlloc.h>
#pragma warning(pop)


class GSysAllocCryMem : public GSysAlloc
{
public:
	// GSysAlloc interface
	virtual void GetInfo(Info* i) const;
	virtual void* Alloc(UPInt size, UPInt align);
	virtual bool Free(void* ptr, UPInt size, UPInt align);

public:
	struct Stats
	{
		uint32 AllocCount;
		uint32 FreeCount;
		size_t Allocated;
	};

public:
	GSysAllocCryMem();
	~GSysAllocCryMem();

	const Stats& GetStats() const;

	void GetMemoryUsage(ICrySizer *pSizer ) const;
private:
	Stats m_stats;

#if defined(PS3_CRYSIZER_HEAP_TRAVERSAL)
	struct MemOp
	{
		MemOp(void *p_ptr, uint32 size ) : ptr(p_ptr), nSize(size){}
		MemOp(void *p_ptr) : ptr(p_ptr), nSize(0){}
		void *ptr;
		uint32 nSize;
		void GetMemoryUsage(ICrySizer *pSizer ) const
		{
			pSizer->AddObject(ptr, nSize);
		}
		bool operator<( const MemOp &rOther ) const { return (uint32)ptr < (uint32)rOther.ptr; }
	};

	std::set<MemOp> m_memops;
#endif
};


#endif // #ifndef EXCLUDE_SCALEFORM_SDK


#endif // #ifndef _GALLOCATOR_CRYMEM_H_