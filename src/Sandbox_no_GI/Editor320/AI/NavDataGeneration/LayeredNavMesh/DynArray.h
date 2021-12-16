//
//  DynArray.h - Simple resizeable array.
//
//  Copyright (C) 2007-2008 Mikko Mononen
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Mikko Mononen memon@inside.org
//

#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <string.h> // memcpy()
#include <assert.h>
#include <stdio.h>
#include <algorithm>


namespace LayeredNavMesh {

class MemBlock
{
	unsigned m_size;
	unsigned m_capacity;
	unsigned char* m_ptr;

	// NOTE Feb 18, 2010: <pvl> disable copying
	MemBlock (const MemBlock & );
	MemBlock & operator= (const MemBlock & );
public:
	inline MemBlock() : m_size(0), m_capacity(0), m_ptr(0) {}
	inline ~MemBlock() { delete [] m_ptr; }
	bool Resize(unsigned size);
	inline unsigned Size() const { return m_size; }
	inline unsigned Capacity() const { return m_capacity; }
	inline unsigned char* Ptr() { return m_ptr; }
	inline const unsigned char* Ptr() const { return m_ptr; }
	inline unsigned char* Grow(unsigned n)
	{
		if (m_size+n > m_capacity)
		{
			if (!Resize(m_size+n))
				return 0;
		}
		else
			m_size += n;
		return m_ptr + m_size-n;
	}
	inline void Shrink(unsigned n) { m_size = n >= m_size ? 0 : m_size-n; }
	inline void Clear() { m_size = 0; }

	void Swap (MemBlock & rhs);
};

inline void MemBlock::Swap (MemBlock & rhs)
{
	std::swap (m_size, rhs.m_size);
	std::swap (m_capacity, rhs.m_capacity);
	std::swap (m_ptr, rhs.m_ptr);
}

template <typename T>
class DynArray
{	
	MemBlock m_mem;
public:

	typedef T* Iter;
	typedef const T* ConstIter;

	inline DynArray() {}
	inline DynArray(unsigned n) { m_mem.Resize(n); }
	
	inline void Resize(unsigned n) { m_mem.Resize(sizeof(T)*n); }
	inline void Clear() { m_mem.Resize(0); }
	inline unsigned Size() const { return m_mem.Size()/sizeof(T); }
	inline bool Empty() const { return m_mem.Size() == 0; }

	inline T& First() { return *reinterpret_cast<T*>(m_mem.Ptr()); }
	inline const T& First() const { return *reinterpret_cast<const T*>(m_mem.Ptr()); }
	inline T& Last() { return *reinterpret_cast<T*>(m_mem.Ptr()+m_mem.Size()-sizeof(T)); }
	inline const T& Last() const { return *reinterpret_cast<const T*>(m_mem.Ptr()+m_mem.Size()-sizeof(T)); }

	inline Iter Begin() { return reinterpret_cast<T*>(m_mem.Ptr()); }
	inline ConstIter Begin() const { return reinterpret_cast<const T*>(m_mem.Ptr()); }
	inline Iter End() { return reinterpret_cast<T*>(m_mem.Ptr()+m_mem.Size()); }
	inline ConstIter End() const { return reinterpret_cast<const T*>(m_mem.Ptr()+m_mem.Size()); }

	inline T& operator[](unsigned i) { assert(i < Size()); return *(reinterpret_cast<T*>(m_mem.Ptr())+i); }
	inline const T& operator[](unsigned i) const { assert(i < Size()); return *(reinterpret_cast<const T*>(m_mem.Ptr())+i); }

	inline void Push(const T& v) { memcpy(m_mem.Grow(sizeof(T)), &v, sizeof(T)); }
	inline void Append(const T* arr, unsigned n) { memcpy(m_mem.Grow(sizeof(T)*n), arr, sizeof(T)*n); }
	inline T& Push() { return *reinterpret_cast<T*>(m_mem.Grow(sizeof(T))); }
	inline void Grow(unsigned n) { m_mem.Grow(n*sizeof(T)); }
	inline void Pop() { m_mem.Shrink(sizeof(T)); }

	void Swap (DynArray & rhs) { m_mem.Swap (rhs.m_mem); }
};

} // namespace LayeredNavMesh

#endif
