////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ParticleUtils.h
//  Version:     v1.00
//  Created:     11/03/2010 by Corey (split out from other files).
//  Compilers:   Visual Studio.NET
//  Description: Splitting out some of the particle specific containers to here.
//							 Will be moved to a proper home eventually.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __particleutils_h__
#define __particleutils_h__
#pragma once

#include "CryArray.h"
#include "CryList.h"

#undef PlaySound


//////////////////////////////////////////////////////////////////////////
// A variable-sized list of items with a pre-allocated maximum.
// Allows efficient element deletion during iteration.

template<class T>
class FixedArray
{
	// states used for the SPU-traverse to indicate the state of the dma
	// transfer with a associated buffer (was moved here because CryCG generated
	// wrong code from an enum nested in a nested class of a template class)
	enum BufferStates 
	{	
		Unused,
		Active,
		TransferingFromMain,
		TransferingToMain
	};

public:
	typedef int size_type;

	FixedArray()
	{
		reset();
	}

	size_type size() const
	{
		return m_nCount;
	}

	void reserve( size_type n )
	{
		m_ElemData.reserve(n);
		m_ElemList.reserve(n+1);
	}

	size_type capacity() const 
	{
		return m_ElemData.capacity();
	}

	bool empty() const
	{
		return size() == 0;
	}

	void clear()
	{
		// Destroy active elems in reverse creation order, then free array memory.
		for (traverser it(*this, false); it; --it)
			it->~T();
		m_ElemData.resize_raw(0, 0);
		reset();
	}

	T* push_back()
	{
		return insert( end_index() );
	}

	T* push_front()
	{
		return insert( head_index() );
	}

	T* insert( const T* next )
	{
		return insert( get_index(next) );
	}

	void erase( T *obj )
	{
		erase( get_index(obj) );
		validate();
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		AddArrayMemory(pSizer, m_ElemList);
		if (AddArrayMemory(pSizer, m_ElemData))
			for (traverser tr(non_const(*this)); tr; ++tr)
				::GetMemoryUsage(pSizer, *tr);
	}

	class traverser
	{
		/* Usage: 
			// Forward
			for (FixedArray::traverser t(array); t; ++t)
				t->Process();
			// Reverse
			for (FixedArray::traverser t(array, false); t; --t)
				t->Process();
		*/

	public:

#if !defined(__SPU__)
		// Default implementation

		traverser( FixedArray& array, bool bForward = true ) :
			m_pList(&array),
			m_nIndex(bForward ? array.head_index() : array.tail_index())
		{
		}

		ILINE void operator++()
		{
			m_nIndex = m_pList->next_index( m_nIndex );
		}
		
		ILINE void operator--()
		{
			m_nIndex = m_pList->prev_index( m_nIndex );
		}

		ILINE operator bool() const
		{
			return m_pList->valid_index( m_nIndex );
		}

		ILINE T* operator->() const
		{
			return m_pList->get_elem( m_nIndex );
		}

		ILINE operator T*() const
		{
			return m_pList->get_elem( m_nIndex );
		}

		ILINE T& operator*() const
		{
			return *m_pList->get_elem( m_nIndex );
		}

		void erase( bool bForward = true )
		{
			m_nIndex = m_pList->erase( m_nIndex, bForward );
		}





























































































































































































































#endif

		// Some common data.
	private:
		FixedArray*	m_pList;
		int					m_nIndex;
	};

private:

	friend class traverser;

	struct ListElem
	{
		int	next;
		int	prev;
	};

	T* get_elem( int index )
	{
		return &m_ElemData[index-1];
	}

	int get_index( const T* obj ) const
	{
		assert(obj >= m_ElemData.begin() && obj <= m_ElemData.end());
		int index = check_cast<int>(obj - m_ElemData.begin()) + 1;
		return index;
	}

	int head_index() const
	{
		return m_ElemList.front().next;
	}
	int tail_index() const
	{
		return m_ElemList.front().prev;
	}
	static int end_index()
	{
		return 0;
	}
	bool valid_index( int index ) const
	{
		assert(index >= 0 && index < m_ElemList.size());
		return index > 0;
	}

	int next_index( int index ) const
	{		
		return m_ElemList[index].next;
	}

	int prev_index( int index ) const
	{		
		return m_ElemList[index].prev;
	}

	void reset()
	{
		m_ElemList.resize_raw(1, 1);
		m_ElemList.front().next = m_ElemList.front().prev = end_index();

		m_nFirstFree = end_index();
		m_nCount = 0;
		validate();
	}

	SPU_NO_INLINE int alloc_new_index()
	{
		m_nCount++;

		// Is there an element in the free list?
		IF( valid_index(m_nFirstFree), true )
		{
			int index = m_nFirstFree;
			m_nFirstFree = next_index(m_nFirstFree);
			IF( !valid_index(index), false )
			{
				snPause();
			}
			return index;
		}
		else
		{
			// Increase number of active elements.
			m_ElemData.grow_raw();
			m_ElemList.push_back();		
			return m_ElemData.size();			
		}
	}

	SPU_NO_INLINE T* insert( int next_index )
	{
		int index = alloc_new_index();
		ListElem& rCurElem = m_ElemList[index];

		// Update links.
		ListElem& rNextElem = m_ElemList[next_index];

		rCurElem.next = next_index;
		rCurElem.prev = rNextElem.prev;

		m_ElemList[rCurElem.prev].next = index;
		m_ElemList[rCurElem.next].prev = index;

		validate();

		// initialise object
		T* obj = get_elem(index);
		return new(obj) T;
	}

	SPU_NO_INLINE int erase( int index, bool bForward = true )
	{
		T* obj = get_elem(index);
		obj->~T();		
		return erase_raw( index, bForward );
	}

	SPU_NO_INLINE int erase_raw( int index, bool bForward = true )
	{
		ListElem& elem = m_ElemList[index];		
		int ret = bForward ? elem.next : elem.prev;

		// Update links.
		m_ElemList[elem.prev].next = elem.next;
		m_ElemList[elem.next].prev = elem.prev;

		// Update FreeList.
		m_ElemList[index].next = m_nFirstFree;
		m_nFirstFree = index;

		m_nCount--;

		validate();
		return ret;
	}

	void validate()
	{
	#ifdef _DEBUG
		assert(size() <= m_ElemData.size());
		assert(m_ElemList.size() == m_ElemData.size()+1);
		assert(head_index() < m_ElemList.size());
		assert(tail_index() < m_ElemList.size());
		assert(prev_index(head_index()) == end_index());
		assert(next_index(tail_index()) == end_index());
		assert(m_nFirstFree < m_ElemList.size());

		int prev = end_index();
		int nUsed = 0;
		for (int index = head_index(); valid_index(index); index = next_index(index), nUsed++)
		{
			assert(prev_index(index) == prev);
			prev = index;
		}
		int nFree = 0;
		for (int index = m_nFirstFree; valid_index(index); index = next_index(index), nFree++)
			;
		assert(nUsed == size());
		assert(nUsed + nFree == m_ElemData.size());
	#endif
	}
	
	int													m_nCount;			// Count of used elements.
	FastDynArray<T>							m_ElemData;		// Actual elements.
	FastDynArray<ListElem>			m_ElemList;		// Bi-directional list to manager the order of elements.
																						// Has m_nCount+1 entries, first entry contains head/tail indices.
	int													m_nFirstFree;	// First free element.
};

//////////////////////////////////////////////////////////////////////////
// Reference-counting without automatic freeing, or virtual functions.
// Can be used as _smart_ptr<> target.

class DumbRefCount
{
public:
	DumbRefCount()
		: m_nRefs(0) {}
	~DumbRefCount()
		{ assert(m_nRefs == 0); }

	void AddRef()
		{ ++m_nRefs; }
	void Release()
		{ assert(m_nRefs > 0); --m_nRefs; }
	int GetRefCount() const
		{ return m_nRefs; }

protected:
	int m_nRefs;
};

//////////////////////////////////////////////////////////////////////////
// Util class for SPU, remembers on which objects Release needs to be called
// usses one global variable
class SpuDeferredReleaseObjects
{
public:

	// these are ifdefed for spu, since they use GetCurrentThreadId













	void ReleaseAll();

private:
	static const int NUM_SPUS = 4;

	// group per spu together and align, so that each spu has a cacheline for itself
	struct DeferredUpdateStacks
	{
		SpuDataStack<IStatObj*>							m_arrStatObj;
		SpuDataStack<ISound*>								m_arrSound;		
	} _ALIGN(128);

	DeferredUpdateStacks							m_deferredReleaseCalls[NUM_SPUS];
};

extern SpuDeferredReleaseObjects gSPUDeferredReleaseObjects;

//////////////////////////////////////////////////////////////////////////
// class to collect information if a ParticleContainer is updateable on SPU
class SpuUsage
{
public:
	SpuUsage( const string &sName, int nNumParticle ) : m_sName(sName), m_nNumParticle(nNumParticle) {}
	void SetCause( const string &sCause ) { m_sCause = sCause; }

	const char* Name() const { return m_sName.c_str(); }
	const char* Cause() const { return m_sCause.c_str(); }
	int NumParticle() const { return m_nNumParticle; }

	bool operator < ( const SpuUsage &other ) const { return m_nNumParticle > other.m_nNumParticle; }
private:
	string m_sName;
	string m_sCause;
	int m_nNumParticle;
};

typedef std::vector<SpuUsage> VecSpuUsageT;

#endif // __particleutils_h__
