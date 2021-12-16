#pragma once

#include "CriticalSection.h"
#include "Utility.h"

template <typename T, size_t ItemsPerPage>
class PoolAllocator
{
private:
	struct Alloc
	{
		Alloc(Alloc* next, size_t uninitLength)
			: next(next)
			, uninitLength(uninitLength) {}
		Alloc* next;
		size_t uninitLength;
	};

private:
	static const size_t ValueSize = MAX(sizeof(T), sizeof(Alloc));
	static const size_t PageSize = ValueSize * ItemsPerPage;

public:
	PoolAllocator()
		: m_lock(4000)
		, m_numAllocs(0)
		, m_potentialAllocs(0)
		, m_garbageCollectThresh(0)
	{
		AddPage();
	}

	~PoolAllocator()
	{
		for (std::vector<u8*>::iterator it = m_pages.begin(), itEnd = m_pages.end();
			it != itEnd;
			++ it)
		{
			delete [] *it;
		}
	}

public:
	void* Allocate()
	{
		SpinCriticalSectionLock lock(m_lock);

		if (m_freeList == NULL)
		{
			AddPage();
		}

		Alloc* next;

		if (m_freeList->next)
		{
			next = m_freeList->next;
		}
		else if (m_freeList->uninitLength)
		{
			assert (m_freeList->uninitLength <= ItemsPerPage);
			next = reinterpret_cast<Alloc*>(reinterpret_cast<u8*>(m_freeList) + ValueSize);
			new (next) Alloc(NULL, m_freeList->uninitLength - 1);
		}
		else
		{
			next = NULL;
		}

		void* p = m_freeList;
		m_freeList = next;

		++ m_numAllocs;

		return p;
	}

	void Free(void* p)
	{
		SpinCriticalSectionLock lock(m_lock);

		Alloc* al = reinterpret_cast<Alloc*>(p);
		new (al) Alloc (m_freeList, 0);
		m_freeList = al;

		-- m_numAllocs;

		size_t alignedAllocCount = ( (m_numAllocs + (ItemsPerPage - 1)) / ItemsPerPage);

		if (alignedAllocCount <= m_garbageCollectThresh)
		{
			if (!GarbageCollect())
			{
				m_garbageCollectThresh >>= 1;
			}
			else
			{
				m_garbageCollectThresh = alignedAllocCount >> 1;
			}
		}
	}

	size_t GetNumAllocs() const { return m_numAllocs; }

	bool GarbageCollect()
	{
		SpinCriticalSectionLock lock(m_lock);

		SortFreeList();
		std::sort(m_pages.begin(), m_pages.end());

		bool freed = false;

		for (std::vector<u8*>::iterator pageIt = m_pages.begin(); pageIt != m_pages.end(); )
		{
			size_t allocsInPage = 0;
			u8* page = *pageIt;

			Alloc* alloc = m_freeList;

			while (((u8*)alloc) >= page && ((u8*)alloc) < (page + PageSize))
			{
				if (alloc->uninitLength)
					allocsInPage += alloc->uninitLength + 1;
				else
					++ allocsInPage;

				alloc = alloc->next;
			}

			if (allocsInPage == ItemsPerPage)
			{
				delete [] page;
				m_freeList = alloc;
				pageIt = m_pages.erase(pageIt);

				freed = true;
			}
			else
			{
				++ pageIt;
			}
		}

		return freed;
	}

private:
	void AddPage()
	{
		u8* page = new u8[PageSize];

		Alloc* headAlloc = reinterpret_cast<Alloc*>(page);
		new (headAlloc) Alloc(NULL, ItemsPerPage - 1);

		m_freeList = headAlloc;
		m_potentialAllocs += ItemsPerPage;

		m_pages.push_back(page);

		m_garbageCollectThresh = m_pages.size() >> 1;
	}

	void SortFreeList()
	{
		SortLL<Alloc, AllocLess, AllocNext>(m_freeList);
	}

	private:
		struct AllocLess
		{
			bool operator () (const Alloc& a, const Alloc& b) const { return &a < &b; }
		};

		struct AllocNext
		{
			Alloc*& operator () (Alloc& a) const { return a.next; }
		};

private:
	PoolAllocator(const PoolAllocator&);
	PoolAllocator& operator = (const PoolAllocator&);

private:
	SpinCriticalSection m_lock;

	std::vector<u8*> m_pages;
	Alloc* m_freeList;

	size_t m_numAllocs;
	size_t m_potentialAllocs;
	size_t m_garbageCollectThresh;
};

template <typename T>
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
		typedef STLPoolAllocator<U> other;
	};

	STLPoolAllocator() throw() { }
	STLPoolAllocator(const STLPoolAllocator&) throw() { }
	template <class U> STLPoolAllocator(const STLPoolAllocator<U>&) throw() { }

	~STLPoolAllocator() throw() { }

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
		if (n == 1)
			return reinterpret_cast<T*>(Allocator().Allocate());
		else
			return reinterpret_cast<T*>(malloc(sizeof(T) * n));
	}

	void deallocate(pointer p, size_type n = 1)
	{
		if (n == 1)
			Allocator().Free(p);
		else
			free(p);
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
		p->~T();
	}

	friend bool operator == (const STLPoolAllocator<T>& a, const STLPoolAllocator<T>& b)
	{
		return true;
	}

	friend bool operator != (const STLPoolAllocator<T>& a, const STLPoolAllocator<T>& b)
	{
		return false;
	}

private:
	typedef PoolAllocator<T, 16384> PoolAllocatorT;

public:
	static PoolAllocatorT& Allocator()
	{
		static PoolAllocatorT alloc;
		return alloc;
	}
};