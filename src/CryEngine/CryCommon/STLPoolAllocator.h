//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __STLPOOLALLOCATOR_H__
#define __STLPOOLALLOCATOR_H__

//---------------------------------------------------------------------------
// STL-compatible interface for the pool allocator (see PoolAllocator.h).
//
// This class is suitable for use as an allocator for STL lists. Note it will
// not work with vectors, since it allocates fixed-size blocks, while vectors
// allocate elements in variable-sized contiguous chunks.
//
// To create a list of type UserDataType using this allocator, use the
// following syntax:
//
// std::list<UserDataType, STLPoolAllocator<UserDataType> > myList;
//---------------------------------------------------------------------------

#include "PoolAllocator.h"
#include <stddef.h>
#include <climits>

namespace stl
{
	namespace STLPoolAllocatorHelper
	{
		inline void destruct(char *) {}
		inline void destruct(wchar_t*) {}
		template <typename T> 
			inline void destruct(T *t) {t->~T();}
	}

	template <size_t S, class L, size_t A, typename T> 
	struct STLPoolAllocatorStatic
	{
		static PoolAllocator<S, L, A> * allocator;
	};

	template <class T, class L, size_t A> 
	struct STLPoolAllocatorKungFu : public STLPoolAllocatorStatic<sizeof(T),L,A,T>
	{
	};

	template <class T, class L = PSyncMultiThread, size_t A = 0> 
	class STLPoolAllocator
	{
	public:
		typedef size_t    size_type;
		typedef ptrdiff_t difference_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef T         value_type;

		template <class U> struct rebind
		{
			typedef STLPoolAllocator<U, L, A> other;
		};

		STLPoolAllocator() throw()
		{
		}

		STLPoolAllocator(const STLPoolAllocator&) throw()
		{
		}

		template <class U, class M, size_t B> STLPoolAllocator(const STLPoolAllocator<U,M,B>&) throw()
		{
		}

		~STLPoolAllocator() throw()
		{
		}

		pointer address(reference x) const
		{
			return &x;
		}

		const_pointer address(const_reference x) const
		{
			return &x;
		}

		pointer allocate(size_type n = 1, const void* hint = 0)
		{
			assert(n == 1);
			if (!STLPoolAllocatorKungFu<T,L,A>::allocator)
				STLPoolAllocatorKungFu<T,L,A>::allocator = new PoolAllocator<sizeof(T),L,A>;
			return static_cast<T*>(STLPoolAllocatorKungFu<T,L,A>::allocator->Allocate());
		}

		void deallocate(pointer p, size_type n = 1)
		{
			assert(n == 1);
			STLPoolAllocatorKungFu<T,L,A>::allocator->Deallocate(p);
		}

		size_type max_size() const throw()
		{
			return INT_MAX;
		}

		void construct(pointer p, const T& val)
		{
			new(static_cast<void*>(p)) T(val);
		}

		void construct(pointer p)
		{
			new(static_cast<void*>(p)) T();
		}

		void destroy(pointer p)
		{
			STLPoolAllocatorHelper::destruct(p);
		}

		pointer new_pointer()
		{
			return new(allocate()) T();
		}

		pointer new_pointer(const T& val)
		{
			return new(allocate()) T(val);
		}

		void delete_pointer(pointer p)
		{
			p->~T();
			deallocate(p);
		}

		bool operator==(const STLPoolAllocator&) {return true;}
		bool operator!=(const STLPoolAllocator&) {return false;}

		static void GetMemoryUsage( ICrySizer *pSizer )
		{
			pSizer->AddObject(STLPoolAllocatorKungFu<T,L,A>::allocator);
		}
	};

	template <class T, size_t A = 0> 
	class STLPoolAllocatorNoMT: public STLPoolAllocator<T, PSyncNone, A>
	{
	};

	template <> class STLPoolAllocator<void>
	{
	public:
		typedef void* pointer;
		typedef const void* const_pointer;
		typedef void value_type;
		template <class U, class L> 
		struct rebind { typedef STLPoolAllocator<U> other; };
	};    

	template <size_t S, typename L, size_t A, typename T> 
	PoolAllocator<S,L,A>* STLPoolAllocatorStatic<S,L,A,T>::allocator;
}



#endif //__STLPOOLALLOCATOR_H__
