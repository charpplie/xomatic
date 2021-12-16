
#ifndef POLYGON_SET_OPS_UTIL_H
#define POLYGON_SET_OPS_UTIL_H

// TODO Aug 6, 2009: <pvl> consider removing the smart_ptr stuff - it's not used
// internally anymore, just client code uses it as a bit of convenience
#include "smartptr.h"

#include <cassert>
#include <cstddef>
#include <algorithm>

namespace PSO {

class Allocator : public _reference_target<int> {
public:
	virtual ~Allocator () = 0;
	virtual void * Allocate (unsigned size) = 0;
	virtual void Deallocate (void * mem) = 0;
};

inline Allocator::~Allocator ()
{
}

/*
 * NOTE Dez 4, 2008: <pvl> all classes in Polygon Set Ops library that have
 * a placement-new to support allocation from an Allocator instance also have
 * a matching placement-delete.  This placement-delete isn't supposed to be
 * invoked directly (I believe it's not even possible, from looking into the
 * Standard) but needs to be there to be invoked by the compiler-generated code
 * in case instance memory was obtained from a placement-new and subsequent
 * construction throwed.
 *
 * These placement-delete's were added to placate static code analyzer and are
 * empty apart from "assert (0)" to indicate that to my knowledge, they shouldn't
 * ever be called (none of the constructors in question is supposed to throw).
 */
class PoolAllocator : public Allocator {
	char * const m_memBase;
	const unsigned m_memSize;

	char * m_firstFree;
public:
	PoolAllocator (unsigned size);
	~PoolAllocator ();

	void * Allocate (unsigned size);
	void Deallocate (void * ) { }

	void Clear ();
};

inline PoolAllocator::PoolAllocator (unsigned size) :
		m_memBase (new char [size]), m_memSize (size), m_firstFree (m_memBase)
{
}

inline PoolAllocator::~PoolAllocator ()
{
	delete [] m_memBase;
}

inline void PoolAllocator::Clear ()
{
	m_firstFree = m_memBase;
}

inline void * PoolAllocator::Allocate (unsigned size)
{
	if (m_firstFree - m_memBase	+ size > m_memSize)
	{
		assert (0);
		return 0;
	}
	char * retval = m_firstFree;
	m_firstFree += size;
	return retval;
}

} // namespace PSO


#include <cstdlib>

namespace PSO {

class StdAllocator : public Allocator {
public:
	virtual void * Allocate (unsigned size);
	virtual void Deallocate (void * mem);
};

inline void * StdAllocator::Allocate (unsigned size)
{
	return ::operator new (size);
}

inline void StdAllocator::Deallocate (void * mem)
{
	::operator delete (mem);
}

} // namespace PSO

// ---

namespace PSO {

enum TraversalDirection { FORWARD, BACKWARD };

// NOTE Aug 28, 2008: <pvl> doubly-linked circular list for polygon edges and
// cross vertex descriptors.  No attempt at genericity here.
template <typename T>
class CircularList {
public:
	class iterator;
private:
	struct Node {
		T * m_data;
		Node * m_prev;
		Node * m_next;
		Node (T * data) : m_data(data), m_prev(0), m_next(0) { }

		static void * operator new (size_t size, Allocator * alloc)
		{
			return alloc->Allocate (size);
		}
		static void operator delete (void * , Allocator * )
		{
			assert (0);
		}
		static void Destroy (Node * node, Allocator * allocator)
		{
			if (node == 0) return;
			node->~Node ();
			allocator->Deallocate (node);
		}
	};
	Node * m_head;
	unsigned m_size;

	Allocator * m_allocator;

	iterator Insert (Node * prev, T * );

	// NOTE Sep 29, 2008: <pvl> dynamic allocation isn't needed - disallow it.  If
	// it ever becomes useful, pass an Allocator* as an additional argument to
	// operator new() and add an empty operator delete() so that no attempt is
	// made at deallocating the memory but ~CircularList() still runs.
	static void * operator new (size_t size);
public:

	class loop_iterator {
		Node * m_current;
		Node * m_end;
		unsigned m_timesToPassThroughEnd;
		bool m_forward;

		friend class CircularList;
		friend class iterator;
		Node * GetNode () const { return m_current; }

		void Next () { m_current = (m_forward ? m_current->m_next : m_current->m_prev); }
		void Prev () { m_current = (m_forward ? m_current->m_prev : m_current->m_next); }
	public:
		loop_iterator (Node * first, TraversalDirection dir = FORWARD);
		loop_iterator (const iterator & , TraversalDirection dir = FORWARD);
		loop_iterator & operator++ ();
		operator bool () const { return m_timesToPassThroughEnd > 0; }
		bool operator== (const loop_iterator & rhs) const;
		T * operator-> () const { return m_current->m_data; }
		T * operator* () const { return m_current->m_data; }

		// NOTE Sep 4, 2008: <pvl> creates a new iterator that points at the same
		// element as this one but its iteration state is reset, i.e. it's at the
		// beginning of the loop.  Direction of iteration can be changed, too.
		loop_iterator GetReset (bool switchDir = false) const;
		// NOTE Sep 2, 2008: <pvl> purely for convenience
		loop_iterator GetNext () const { loop_iterator next (*this); next.Next(); return next; }
		loop_iterator GetPrev () const { loop_iterator prev (*this); prev.Prev(); return prev; }
	};

	class iterator {
		Node * m_current;

		friend class CircularList;
	public:
		iterator (Node * first) : m_current (first) { }
		iterator (const loop_iterator & first) : m_current (first.GetNode()) { }
		iterator & operator++ () { m_current = m_current->m_next; return *this; }
		iterator & operator-- () { m_current = m_current->m_prev; return *this; }
		bool operator== (const loop_iterator & rhs) const { return m_current == rhs.m_current; }
		operator bool () const { return m_current != 0; }
		T * operator-> () const { return m_current->m_data; }
		T * operator* () const { return m_current->m_data; }

		// NOTE Dez 3, 2008: <pvl> made public since loop_iterator needs it.  It
		// shouldn't break encapsulation too much since nobody outside CircularList
		// will be able to dereference the pointer anyway.
		Node * GetNode () const { return m_current; }

		// NOTE Sep 2, 2008: <pvl> purely for convenience
		iterator GetNext () const { iterator next (*this); ++next; return next; }
		iterator GetPrev () const { iterator prev (*this); --prev; return prev; }
	};

	CircularList (Allocator * allocator) :
			m_head (0), m_size (0), m_allocator (allocator)
	{ }
	CircularList (const CircularList & rhs) :
			m_head (rhs.m_head), m_size (rhs.m_size), m_allocator (rhs.m_allocator)
	{ }
	~CircularList ();

	iterator Add (T * );

	template <typename Iterator>
	iterator Insert (Iterator previous, T * );

	template <typename Compare>
	iterator Insert (T * , const Compare & );

	template <typename Iterator>
	void Erase (Iterator );

	void Clear ();

	unsigned Size () const;

	loop_iterator CreateLoopIt (TraversalDirection dir = FORWARD) const { return loop_iterator (m_head, dir); }
	iterator CreateIt (TraversalDirection dir = FORWARD) const { return iterator (m_head, dir); }

	// NOTE Mar 27, 2009: <pvl> "placement delete" work-around as per
	// http://www.research.att.com/~bs/bs_faq2.html#placement-delete
	static void Destroy (CircularList & );
};

template <typename T>
inline CircularList<T>::loop_iterator::loop_iterator (Node * first, TraversalDirection dir) :
		m_current (first), m_end (first), m_timesToPassThroughEnd (1),
		m_forward (dir == FORWARD)
{
	if (m_current == 0)
		m_timesToPassThroughEnd = 0;
}

// NOTE Sep 2, 2008: <pvl> does the same as Node* ctor - a job for c++0x's
// forwarding ctor! ;-)
template <typename T>
inline CircularList<T>::loop_iterator::loop_iterator (const iterator & it, TraversalDirection dir) :
		m_current (it.GetNode()), m_end (it.GetNode()), m_timesToPassThroughEnd (1),
		m_forward (dir == FORWARD)
{
	if (m_current == 0)
		m_timesToPassThroughEnd = 0;
}

template <typename T>
inline typename CircularList<T>::loop_iterator &
CircularList<T>::loop_iterator::operator++ ()
{
	Next ();
	if (m_current == m_end)
		--m_timesToPassThroughEnd;
	return *this;
}

template <typename T>
inline bool CircularList<T>::loop_iterator::operator== (const loop_iterator & rhs) const
{
	// NOTE Sep 4, 2008: <pvl> we only compare the actual position meaning that
	// iterators looping over different ranges can compare equal (could that be
	// a bad thing?)
	return m_current == rhs.m_current/* && m_end == rhs.m_end
					&& m_timesToPassThroughEnd == rhs.m_timesToPassThroughEnd*/;
}

template <typename T>
inline typename CircularList<T>::loop_iterator
CircularList<T>::loop_iterator::GetReset (bool switchDir) const
{
	loop_iterator reset (*this);
	reset.m_end = reset.m_current;
	reset.m_timesToPassThroughEnd = 1;
	reset.m_forward ^= switchDir;
	return reset;
}

template <typename T>
inline CircularList<T>::~CircularList ()
{
	Clear ();
}

template <typename T>
inline typename CircularList<T>::iterator CircularList<T>::Add (T * data)
{
	Node * new_node = new (m_allocator) Node (data);

	if (m_head == 0)
	{
		new_node->m_prev = new_node;
		new_node->m_next = new_node;
		m_head = new_node;
	}
	else
	{
		new_node->m_next = m_head;
		new_node->m_prev = m_head->m_prev;
		m_head->m_prev->m_next = new_node;
		m_head->m_prev = new_node;
	}
	++m_size;
	return new_node;
}

template <typename T>
inline typename CircularList<T>::iterator CircularList<T>::Insert (Node * prev, T * data)
{
	Node * new_node = new (m_allocator) Node (data);

	new_node->m_next = prev->m_next;
	new_node->m_prev = prev;
	prev->m_next->m_prev = new_node;
	prev->m_next = new_node;

	++m_size;

	return new_node;
}

template <typename T>
template <typename Iterator>
inline typename CircularList<T>::iterator CircularList<T>::Insert (Iterator previous, T * data)
{
	return Insert (previous.GetNode (), data);
}

template <typename T>
template <typename Compare>
inline typename CircularList<T>::iterator CircularList<T>::Insert (T * data, const Compare & compare)
{
	if (m_head == 0)
		return Add (data);

	loop_iterator it = CreateLoopIt ();

	for ( ; it; ++it)
	{
		if ( ! compare (*it, data))
		{
			// add before the node pointed to by 'it'
			iterator new_node = Insert (it.GetNode()->m_prev, data);
			// if added before head it becomes the new head
			if (it.GetNode () == m_head)
				m_head = new_node.GetNode ();
			return new_node;
		}
	}

	// add at the end
	return Insert (m_head->m_prev, data);
}

template <typename T>
template <typename Iterator>
inline void CircularList<T>::Erase (Iterator doomed)
{
	assert (m_size != 0);
	Node * doomedNode = doomed.GetNode ();
	if (m_size == 1)
	{
		assert (doomedNode == m_head);
		m_head = 0;
		--m_size;
	}

	if (doomedNode == m_head)
		m_head = m_head->m_next;

	doomedNode->m_next->m_prev = doomedNode->m_prev;
	doomedNode->m_prev->m_next = doomedNode->m_next;

	Node::Destroy (doomedNode, m_allocator);

	--m_size;
}

template <typename T>
inline void CircularList<T>::Clear ()
{
	while (m_size)
	{
		Node * doomed = m_head;
		m_head = m_head->m_next;

		Node::Destroy (doomed, m_allocator);

		--m_size;
	}
	m_head = 0;
}

template <typename T>
inline unsigned CircularList<T>::Size () const
{
	return m_size;
}

template <typename T>
inline void CircularList<T>::Destroy (CircularList & list)
{
	Allocator * allocator = list.m_allocator;
	list.~CircularList<T> ();
	allocator->Deallocate ( & list);
}


// ---

template <typename T>
class FixedSizeArray {
	T * const m_array;
	const unsigned m_capacity;
	unsigned m_firstFree;

	// NOTE Mar 30, 2009: <pvl> disallow dynamic allocation - not for any fundamental
	// reason, it's just I'm not completely sure if it's handled properly
	static void * operator new (size_t );
	static void operator delete (void * );
public:
	// FIXME Mar 27, 2009: <pvl> find a way of passing Allocator* in as a parameter!
	FixedSizeArray (unsigned capacity, Allocator * allocator) :
			// NOTE Sep 15, 2008: <pvl> no much done for proper alignment here -
			// shouldn't be necessary anyway
			m_array (static_cast <T*> (PolygonSetOp::GetCurAllocator()->Allocate (capacity * sizeof(T)))),
			m_capacity (capacity), m_firstFree (0)
	{ }

	~FixedSizeArray ()
	{
		for (int i=0; i < m_firstFree; ++i)
			(&m_array[i])->~T();
		PolygonSetOp::GetCurAllocator ()->Deallocate (m_array);
	}

	T & operator[] (unsigned index) { assert (index < m_capacity); return m_array[index]; }

	void PushBack (const T & new_t)
	{
		assert (m_firstFree < m_capacity);
		new (& m_array[m_firstFree++]) T (new_t);
	}

	void TrimBack (T * new_last)
	{
		m_firstFree = new_last - m_array;
	}

	unsigned Size () const { return m_firstFree; }

	T * Begin () const { return & m_array[0]; }
	T * End () const { return & m_array[m_firstFree]; }

	bool IsOnList (const T & t) { return &(*this)[m_firstFree] != std::find ( & (*this)[0], & (*this)[m_firstFree], t); }

	static void Destroy (FixedSizeArray & array, Allocator * allocator)
	{
		array.~FixedSizeArray ();
		allocator->Deallocate ( & array);
	}
};

} // namespace PSO

#endif // POLYGON_SET_OPS_UTIL_H
