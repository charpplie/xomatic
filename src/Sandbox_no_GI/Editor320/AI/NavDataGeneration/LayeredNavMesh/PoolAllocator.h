//
//  PoolAllocator.h - Fast allocator for fixed sized items.
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

#ifndef POOLALLOCATOR_H
#define POOLALLOCATOR_H

namespace LayeredNavMesh {

template <typename T, int N = 256>
class PoolAllocator
{
	struct Item
	{
		union
		{
			Item* next;
			T data;
		};
	};
	
	struct Page
	{
		Page* next;
		Item items[1];
	};
	
	Item* m_freelist;
	Page* m_pages;
	
	inline bool CreatePage()
	{
		// Allocate memory for the page
		const int size = (sizeof(Page)-sizeof(Item)) + sizeof(Item)*N;
		Page* page = reinterpret_cast<Page*>(new unsigned char[size]);
		if (!page) return false;
		page->next = 0;
		
		// Add the page into the list of pages.
		page->next = m_pages;
		m_pages = page;
		
		// Add new items to the free list.
		Item* freelist = m_freelist;
		Item* head = &page->items[0];
		Item* it = &page->items[N];
		do
		{
			--it;
			it->next = freelist;
			freelist = it;
		}
		while (it != head);
		m_freelist = it;
		
		return true;
	}
	
	inline void DeletePages()
	{
		Page* b = m_pages;
		while (b)
		{
			Page* next = b->next;
			delete [] reinterpret_cast<unsigned char*>(b);
			b = next;			
		}		
		m_freelist = 0;
		m_pages = 0;
	}
	
public:
		
	inline PoolAllocator() :
		m_freelist(0),
		m_pages(0)
	{
	}
	
	inline ~PoolAllocator()
	{
		DeletePages();
	}
	
	inline T* Alloc()
	{
		// If running out of memory, allocate new page and update the freelist.
		if (!m_freelist || !m_freelist->next)
		{
			if (!CreatePage())
				return 0;
		}
		
		// Pop item from in front of the free list.
		Item* it = m_freelist;
		m_freelist = m_freelist->next;
		
		return reinterpret_cast<T*>(it);
	}
	
	inline void Free(T* ptr)
	{
		if (!ptr) return;
		// Add the node in front of the free list.
		Item* it = reinterpret_cast<Item*>(ptr);
		it->next = m_freelist;
		m_freelist = it;
	}
	
	inline int GetMemSize() const
	{
		int pages = 0;
		for (Page* b = m_pages; b; b = b->next)
			pages++;
		int size = sizeof(this);
		size += pages*(sizeof(Page)-sizeof(Item)) + sizeof(Item)*N;
		return size;
	}
	
/*	inline void Dump()
	{
		int pages = 0;
		for (Page* b = m_pages; b; b = b->next)
			pages++;
		int free = 0;
		for (Item* it = m_freelist; it; it = it->next)
			free++;
		printf("pool alloc:\n");
		printf(" - pages: %d\n", pages);
		printf(" - capacity: %d\n", pages*N);
		printf(" - used: %d\n", pages*N-free);
		printf(" - free: %d\n", free);
		printf("\n", free);
	}*/
	
};

} // namespace LayeredNavMesh

#endif
