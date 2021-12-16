//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
// Modified 2008-06, Scott Peter
//	Refactored to utilise storage management of new HeapAllocator class
//---------------------------------------------------------------------------

#ifndef __POOLALLOCATOR_H__
#define __POOLALLOCATOR_H__

//---------------------------------------------------------------------------
// Memory allocation class. Allocates, frees, and reuses fixed-size blocks of 
// memory, a scheme sometimes known as Simple Segregated Memory.
//
// Allocation is amortized constant time. The normal case is very fast -
// basically just a couple of dereferences. If many blocks are allocated,
// the system may occasionally need to allocate a further bucket of blocks
// for itself. Deallocation is strictly fast constant time.
//
// Each PoolAllocator allocates blocks of a single size and alignment, specified 
// by template arguments. There is no per-block space overhead, except for 
// alignment. The free list mechanism uses the memory of the block itself 
// when it is deallocated. 
// 
// In this implementation memory claimed by the system is never deallocated,
// until the entire allocator is deallocated. This is to ensure fast
// allocation/deallocation - reference counting the bucket quickly would
// require a pointer to the bucket be stored, whereas now no memory is used
// while the block is allocated.
//
// This allocator is suitable for use with STL lists - see STLPoolAllocator
// for an STL-compatible interface.
//
// The class can optionally support multi-threading, using the second
// template parameter. By default it is multithread-safe.
// See Synchronization.h.
//
// The class is implemented using a HeapAllocator.
//---------------------------------------------------------------------------

#include "HeapAllocator.h"

namespace stl
{
	//////////////////////////////////////////////////////////////////////////
	// Fixed-size pool allocator, using a shared heap.
	template <typename THeap>
	class SharedSizePoolAllocator
	{
	protected:

		using_type(THeap, Lock);

		struct ObjectNode
		{
			ObjectNode* pNext;
		};

		static size_t AllocSize(size_t nSize)
		{
			return max(nSize, sizeof(ObjectNode));
		}
		static size_t AllocAlign(size_t nSize, size_t nAlign)
		{
			return nAlign > 0 ? nAlign : min(nSize, alignof(void*));
		}

	public:

		SharedSizePoolAllocator(THeap& heap, size_t& nMemUsed, size_t nSize, size_t nAlign = 0)
		: _pHeap(&heap), _pMemUsed(&nMemUsed),
			_nAllocSize(AllocSize(nSize)),
			_nAllocAlign(AllocAlign(nSize, nAlign)),
			_pFreeList(0)
		{
		}

		~SharedSizePoolAllocator()
		{
			// All allocated objects should be freed by now.
			Lock lock(*_pHeap);
			Validate(lock);
			for (ObjectNode* pFree = _pFreeList; pFree; )
			{
				ObjectNode* pNext = pFree->pNext;
				_pHeap->Deallocate(pFree, _nAllocSize, lock);
				pFree = pNext;
			}
		}

		// Raw allocation.
		void* Allocate()
		{
			{
				Lock lock(*_pHeap);
				*_pMemUsed += _nAllocSize;
				if (_pFreeList)
				{
					ObjectNode* pFree = _pFreeList;
					_pFreeList = _pFreeList->pNext;
					Validate(lock);
					return pFree;
				}
			}

			// No free pointer, allocate a new one.
			return (ObjectNode*)_pHeap->Allocate(_nAllocSize, _nAllocAlign);
		}

		void Deallocate(void* pObject)
		{
			if (pObject)
			{
				Lock lock(*_pHeap);
				assert(_pHeap->CheckPtr(pObject, lock));

				ObjectNode* pNode = static_cast<ObjectNode*>(pObject);

				// Add the object to the front of the free list.
				pNode->pNext = _pFreeList;
				_pFreeList = pNode;
				*_pMemUsed -= _nAllocSize;
				Validate(lock);
			}
		}

		size_t GetMemSize(const void* pObject) const
		{
			return Align(_nAllocSize, _nAllocAlign);
		}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			_pHeap->GetMemoryUsage(pSizer, _nAllocSize, _nAllocAlign);			
		}
	protected:

		void Validate(const Lock& lock) const
		{
			_pHeap->Validate(lock);
			assert(*_pMemUsed <= _pHeap->GetTotalMemory().nUsed);
		}
		
	protected:
		const size_t			_nAllocSize, _nAllocAlign;

		THeap*						_pHeap;
		size_t*						_pMemUsed;
		ObjectNode*				_pFreeList;
	};

	//////////////////////////////////////////////////////////////////////////
	// SizePoolAllocator with owned heap
	template <typename THeap>
	class SizePoolAllocator: public SharedSizePoolAllocator<THeap>
	{
		using_type(THeap, Lock);
		using SharedSizePoolAllocator<THeap>::AllocSize;
		using SharedSizePoolAllocator<THeap>::_pFreeList;

	public:

		SizePoolAllocator(size_t nSize, size_t nAlign = 0, size_t nBucketSize = 0)
		: _Heap(nBucketSize * AllocSize(nSize)), _MemUsed(0), SharedSizePoolAllocator<THeap>(_Heap, _MemUsed, nSize, nAlign)
		{
		}

		~SizePoolAllocator()
		{
			// Ignore the free list, as owned heap freed all at once.
			Lock lock(_Heap);
			assert(_MemUsed == 0);
			_pFreeList = 0;
		}

		void FreeMemory( bool bDeallocate = true)
		{
			{
				Lock lock(_Heap);
				_pFreeList = 0;
				_MemUsed = 0;
			}
			if (bDeallocate)
				_Heap.Clear();
			else
				_Heap.Reset();
		}

		SMemoryUsage GetTotalMemory() const
		{
			return SMemoryUsage(_Heap.GetTotalMemory().nAlloc, _MemUsed);
		}

		size_t GetTotalAllocatedMemory() const
		{
			return _Heap.GetTotalMemory().nAlloc;
		}
		size_t GetTotalAllocatedNodeSize() const
		{
			return _MemUsed;
		}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			SharedSizePoolAllocator<THeap>::GetMemoryUsage(pSizer);
		}

	protected:
		THeap				_Heap;
		size_t			_MemUsed;
	};

	//////////////////////////////////////////////////////////////////////////
	// Templated size version of SizePoolAllocator
	template <int S, typename L = PSyncMultiThread, int A = 0>
	class PoolAllocator: public SizePoolAllocator< HeapAllocator<L> >
	{
	public:
		PoolAllocator(size_t nBucketSize = 0)
		:	SizePoolAllocator< HeapAllocator<L> >(S, A, nBucketSize)
		{
		}
	};

	//////////////////////////////////////////////////////////////////////////
	template <int S, int A = 0>
	class PoolAllocatorNoMT : public SizePoolAllocator< HeapAllocator<PSyncNone> >
	{
	public:
		PoolAllocatorNoMT(size_t nBucketSize = 0)
		:	SizePoolAllocator< HeapAllocator<PSyncNone> >(S, A, nBucketSize)
		{
		}
	};

	//////////////////////////////////////////////////////////////////////////
	template<typename T, typename L = PSyncMultiThread, size_t A = 0>
	class TPoolAllocator: public SizePoolAllocator< HeapAllocator<L> >
	{
	public:
		TPoolAllocator(size_t nBucketSize = 0)
			: SizePoolAllocator< HeapAllocator<L> >(sizeof(T), max(alignof(T), A), nBucketSize)
		{}
	};

	// Legacy verbose typedefs.
	typedef PSyncNone PoolAllocatorSynchronizationSinglethreaded;
	typedef PSyncMultiThread PoolAllocatorSynchronizationMultithreaded;

	//////////////////////////////////////////////////////////////////////////
	// Allocator maintaining multiple type-specific pools, sharing a common heap source.
	// The additional TInstancer type provides a way of instantiating multiple instances
	// of this class, without static variables.
	//
	template<typename THeap, typename TInstancer = int>
	struct PoolCommonAllocator
	{
		template<class T>
			static void* Allocate(T*& p)
				{ return p = (T*)TypeAllocator<T>().Allocate(); }

		template<class T>
			static void Deallocate(T* p)
				{ return TypeAllocator<T>().Deallocate(p); }

		template<class T>
			static size_t GetMemSize(const T* p)
				{ return TypeAllocator<T>().GetMemSize(p); }

		static SMemoryUsage GetPoolMemory()
		{ 
			assert( StaticHeap().GetTotalMemory().nUsed >= MemUsed() );
			return SMemoryUsage( StaticHeap().GetTotalMemory().nUsed, MemUsed() );
		}

		static SMemoryUsage GetTotalMemory()
		{ 
			assert( StaticHeap().GetTotalMemory().nAlloc >= MemUsed() );
			return SMemoryUsage( StaticHeap().GetTotalMemory().nAlloc, MemUsed() );
		}

		template<typename T>
		void GetMemoryUsage( ICrySizer* pSizer ) const
		{
			pSizer->AddObject( TypeAllocator<T>() );
		}
	protected:

		template<class T>
			static SharedSizePoolAllocator<THeap>& TypeAllocator()
			{
				static SharedSizePoolAllocator<THeap> s_Pool(StaticHeap(), MemUsed(), sizeof(T), alignof(T));
				return s_Pool;
			}

		static THeap& StaticHeap()
		{
			static THeap s_Heap;
			return s_Heap;
		}
		static size_t& MemUsed()
		{
			static size_t s_Used = 0;
			return s_Used;
		}
	};
};


#endif //__POOLALLOCATOR_H__
