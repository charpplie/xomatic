//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Craig Tiller
//---------------------------------------------------------------------------
#ifndef __STLPOOLALLOCATOR_MANYELEMS_H__
#define __STLPOOLALLOCATOR_MANYELEMS_H__

//---------------------------------------------------------------------------
// STL-compatible interface for the pool allocator (see PoolAllocator.h).
//
// this class acts like STLPoolAllocator, but it is also usable for vectors
// which means that it can be used as a more efficient allocator for many
// implementations of hash_map (typically this uses internally a vector and
// a list with the same allocator)
//---------------------------------------------------------------------------

#include "STLPoolAllocator.h"

namespace stl
{
	template <class T, class L = PSyncMultiThread, size_t A = 0> 
	class STLPoolAllocator_ManyElems: public STLPoolAllocator<T,L,A>
	{
		typedef STLPoolAllocator<T,L,A> Super;

	public:

		typedef typename Super::pointer pointer;
		typedef typename Super::size_type size_type;

		template <class U> struct rebind
		{
			typedef STLPoolAllocator_ManyElems<U, L, A> other;
		};

		STLPoolAllocator_ManyElems() throw()
		{
		}

		template <class U, class M, size_t B> 
		STLPoolAllocator_ManyElems(const STLPoolAllocator_ManyElems<U,M,B>&) throw()
		{
		}

		pointer allocate(size_type n = 1, const void* hint = 0)
		{
			if (n > 1)
			{
			#if defined(NOT_USE_CRY_MEMORY_MANAGER)
				return static_cast<pointer>(CryModuleMalloc( n * sizeof(T) ));
			#else
				return static_cast<pointer>(CryModuleMalloc( n * sizeof(T), eCryM_Network ));
			#endif
			}
			return Super::allocate(1, hint);
		}

		void deallocate(pointer p, size_type n = 1)
		{
			if (n > 1)
			{
#ifndef NOT_USE_CRY_MEMORY_MANAGER
				CryModuleFree( p, eCryM_Network );
#else //NOT_USE_CRY_MEMORY_MANAGER
				CryModuleFree( p );
#endif
			}
			else
				Super::deallocate(p);
		}
	};
}

#endif //__STLPOOLALLOCATOR_H__
