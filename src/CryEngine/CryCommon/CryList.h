#ifndef _CRY_LIST_H_
#define _CRY_LIST_H_
#pragma once

#include "Allocator.h"

#define for_all_ptrs(Type, p, cont) \
	for (Type* p = (cont).begin(), *_e = (cont).end(); p != _e; p = (cont).next(p))

#define for_rev_all_ptrs(Type, p, cont) \
	for (Type* p = (cont).rbegin(), *_e = (cont).rend(); p != _e; p = (cont).prev(p))

////////////////////////////////////////////////////////////////////////
// Bidirectional list

template< class T, class TAlloc = StdAllocator >
class List
{
protected:

	struct Node: T
	{
		Node*	pNext;
		Node*	pPrev;

		Node()
			{}

		template<typename I>
		Node(I const& i)
			: T(i) {}

		void remove()
		{
			validate();
			pPrev->pNext = pNext;
			pNext->pPrev = pPrev;
		}

		void insert_before(Node* node)
		{
			assert(node);
			pNext = node;
			pPrev = node->pPrev;
			node->pPrev->pNext = this;
			node->pPrev = this;
		}

		void insert_after(Node* node)
		{
			assert(node);
			pNext = node->pNext;
			pPrev = node;
			node->pNext->pPrev = this;
			node->pNext = this;
		}

		void validate() const
		{
			assert(pNext);
			assert(pNext->pPrev == this);
			assert(pPrev);
			assert(pPrev->pNext == this);
		}
	};

	TAlloc		m_Alloc;

public:

	typedef T value_type;

	// Constructors, using default allocator or allocator initialiser.
	List()
	{ reset(); }

	template<class I>
	List(const I& init)
		: m_Alloc(init)
		{ reset(); }

	~List()
		{ clear(); }

	bool operator +() const
		{ return head() != base(); }
	bool operator !() const
		{ return head() == base(); }
	bool empty() const
		{ return head() == base(); }

	CONST_VAR_FUNCTION( T& front(),
		{ assert(!empty()); return *head(); } )
	CONST_VAR_FUNCTION( T& back(),
		{ assert(!empty()); return *tail(); } )

	// Slow: do not call often
	int size() const
	{
		int nSize = 0;
		for_all_ptrs (const T, p, *this)
			nSize++;
		return nSize;
	}

	//
	// Iteration
	//
	CONST_VAR_FUNCTION( T* begin(),
		{ return head(); } )
	CONST_VAR_FUNCTION( T* end(),
		{ return base(); } )
	static T* next(const T* p)
		{ return get_node(p)->pNext; }

	CONST_VAR_FUNCTION( T* rbegin(),
		{ return tail(); } )
	CONST_VAR_FUNCTION( T* rend(),
		{ return base(); } )
	static T* prev(const T* p)
		{ return get_node(p)->pPrev; }

	//
	// Add elements
	//
	void* push_back_new()
	{
		Node* pNode = (Node*)m_Alloc.Allocate(pNode);
		pNode->insert_after(tail());
		validate();
		assert(tail() == pNode);
		return pNode;
	}
	T* push_back()
	{ 
		return new(push_back_new()) Node(); 
	}
	template<class I>
	T* push_back(const I& ini)
	{ 
		return new(push_back_new()) Node(ini); 
	}

	void* push_front_new()
	{
		Node* pNode = (Node*)m_Alloc.Allocate();
		pNode->insert_before(head());
		validate();
		assert(head() == pNode);
		return pNode;
	}
	T* push_front()
	{ 
		return new(push_front_new()) Node(); 
	}
	template<class I>
	T* push_front(const I& ini)
	{ 
		return new(push_front_new()) Node(ini); 
	}

	//
	// Remove elements
	//
	T* erase(T* p, bool bForward = true)
	{
		Node* pNode = get_node(p);
		p = bForward ? pNode->pPrev : pNode->pNext;
		pNode->remove();
		Delete(m_Alloc, pNode);
		validate();
		return p;
	}
	T* erase_rev(T* p)
		{ return erase(p, false); }

	void pop_back()
	{
		assert(!empty());
		erase(tail());
	}

	void pop_front()
	{
		assert(!empty());
		erase(head());
	}

	void clear()
	{
		// Destroy all elements, in reverse order
		for (Node* p = tail(); p != base(); )
		{
			Node* pPrev = p->pPrev;
			Delete(m_Alloc, p);
			p = pPrev;
		}
		reset();
	}

	// Move element within or between lists
	void move_back(T* p)
	{
		Node* pNode = get_node(p);
		pNode->remove();
		pNode->insert_after(tail());
	}
	void move_front(T* p)
	{
		Node* pNode = get_node(p);
		pNode->remove();
		pNode->insert_before(head());
	}

	template<class TSizer>
	void GetMemoryUsage( TSizer* pSizer ) const
	{
		for_all_ptrs (const T, p, *this)
		{
			if (pSizer->AddObject(p, m_Alloc.GetMemSize(get_node(p))))
				::GetMemoryUsage(pSizer, *p);
		}
	}

protected:

	Node*	m_pHead;
	Node* m_pTail;

protected:

	CONST_VAR_FUNCTION( Node* base(),
	{
		char* pVirtualBase = (char*)&m_pHead - offsetof(Node, pNext);
		return (Node*)pVirtualBase;
	})

	CONST_VAR_FUNCTION( Node* head(),
		{ return m_pHead; } )
	CONST_VAR_FUNCTION( Node* tail(),
		{ return m_pTail; } )

	void reset()
	{
		m_pHead = m_pTail = base();
		validate();
	}

	static Node* get_node(T* p)
	{
		assert(p);
		Node* pNode = static_cast<Node*>(p);
		pNode->validate();
		return pNode;
	}
	static const Node* get_node(const T* p)
	{
		assert(p);
		const Node* pNode = static_cast<const Node*>(p);
		pNode->validate();
		return pNode;
	}

	size_t validate()
	{
		size_t nSize = 0;
	#ifdef _DEBUG
		Node* n = head();
		while (n != base())
		{
			nSize++;
			assert (n->pNext->pPrev == n);
			n = n->pNext;
		}
	#endif
		return nSize;
	}
};

#endif
