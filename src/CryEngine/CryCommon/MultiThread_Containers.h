////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek, 2008.
// -------------------------------------------------------------------------
//  File name:   MultiThread_Containers.h
//  Version:     v1.00
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MultiThread_Containters_h__
#define __MultiThread_Containters_h__
#pragma once

#include "Pipe.h"

#include <queue>
#include <set>

namespace CryMT
{
	//////////////////////////////////////////////////////////////////////////
	// Thread Safe warppers on the standart STL containers.
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Multi-Thread safe queue container, can be used instead of std::vector.
	//////////////////////////////////////////////////////////////////////////
	template <class T>
	class queue
	{
	public:
		typedef	T	value_type;
		typedef	std::vector<T>	container_type;
		typedef	CryAutoCriticalSection AutoLock;

		//////////////////////////////////////////////////////////////////////////
		// std::queue interface
		//////////////////////////////////////////////////////////////////////////
		const T& front() const		{ AutoLock lock(m_cs); return v.front(); };
		const T& back() const { AutoLock lock(m_cs); 	return v.back(); }
		void	push(const T& x)	{ AutoLock lock(m_cs); return v.push_back(x); };
		void reserve(const size_t n) { AutoLock lock(m_cs); v.reserve(n); };
		// classic pop function of queue should not be used for thread safety, use try_pop instead
		//void	pop()							{ AutoLock lock(m_cs); return v.erase(v.begin()); };

		bool   empty() const { AutoLock lock(m_cs); return v.empty(); }
		int    size() const  { AutoLock lock(m_cs); return v.size(); }

		//////////////////////////////////////////////////////////////////////////
		bool try_pop( T& returnValue )
		{
			AutoLock lock(m_cs); 
			if (!v.empty())
			{
				returnValue = v.front();
				v.erase(v.begin());
				return true;
			}
			return false;
		};

		//////////////////////////////////////////////////////////////////////////
		bool try_remove( T& value )
		{
			AutoLock lock(m_cs);
			if(!v.empty())
			{
				typename container_type::iterator it = std::find(v.begin(), v.end(), value);
				if(it != v.end())
				{
					v.erase(it);
					return true;
				}
			}
			return false;
		};

	private:
		container_type v;
		mutable	CryCriticalSection m_cs;
	};

	//////////////////////////////////////////////////////////////////////////
	// Multi-Thread safe vector container, can be used instead of std::vector.
	//////////////////////////////////////////////////////////////////////////
	template <class T>
	class vector
	{
	public:
		typedef	T	value_type;
		typedef	CryAutoCriticalSection AutoLock;

		//////////////////////////////////////////////////////////////////////////
		// std::vector interface
		//////////////////////////////////////////////////////////////////////////
		bool   empty() const { AutoLock lock(m_cs); return v.empty(); }
		int    size() const  { AutoLock lock(m_cs); return v.size(); }
		void   resize( int sz ) { AutoLock lock(m_cs); v.resize( sz ); }
		void   reserve( int sz ) { AutoLock lock(m_cs); v.reserve( sz ); }
		size_t capacity() const { AutoLock lock(m_cs); return v.size(); }
		void   clear() { AutoLock lock(m_cs); v.clear(); }
		T&	   operator[]( size_t pos ) { AutoLock lock(m_cs); return v[pos]; }
		const T& operator[]( size_t pos ) const { AutoLock lock(m_cs); return v[pos]; }
		const T& front() const { AutoLock lock(m_cs); return v.front(); }
		const T& back() const { AutoLock lock(m_cs); 	return v.back(); }

		void push_back( const T& x ) { AutoLock lock(m_cs); return v.push_back(x); }
		void pop_back() { AutoLock lock(m_cs); 	return v.pop_back(); }

	private:
		std::vector<T> v;
		mutable	CryCriticalSection m_cs;
	};


	//////////////////////////////////////////////////////////////////////////
	// Multi-Thread safe set container, can be used instead of std::set.
	// It has limited functionality, but most of it is there.
	//////////////////////////////////////////////////////////////////////////
	template <class T>
	class set
	{
	public:
		typedef	T	value_type;
		typedef T Key;
		typedef typename std::set<T>::size_type	size_type;
		typedef	CryAutoCriticalSection	AutoLock;

		//////////////////////////////////////////////////////////////////////////
		// Methods
		//////////////////////////////////////////////////////////////////////////
		void								clear()																			{ AutoLock lock(m_cs); s.clear(); }
		size_type						count(const Key& _Key) const								{ AutoLock lock(m_cs); return s.count(_Key); }
		bool								empty() const																{ AutoLock lock(m_cs); return s.empty(); }
		size_type						erase(const Key& _Key)											{ AutoLock lock(m_cs); return s.erase(_Key); }

		bool								find(const Key& _Key)												{ AutoLock lock(m_cs); return (s.find(_Key)!=s.end()); }

		bool								pop_front(value_type&	rFrontElement)				{AutoLock lock(m_cs);if (s.empty()){return false;}rFrontElement=*s.begin();s.erase(s.begin());return true;}
		bool								pop_front()																	{AutoLock lock(m_cs);if (s.empty()){return false;}s.erase(s.begin());return true;}

		bool								front(value_type&	rFrontElement)						{AutoLock lock(m_cs);if (s.empty()){return false;}rFrontElement=*s.begin();return true;}

		bool								insert(const value_type& _Val)							{ AutoLock lock(m_cs); return s.insert(_Val).second; }
		size_type						max_size() const														{ AutoLock lock(m_cs); return s.max_size(); }
		size_type						size() const																{ AutoLock lock(m_cs); return s.size(); }
		void								swap(set& _Right)														{ AutoLock lock(m_cs); s.swap(_Right); }
	private:
		std::set<value_type>				s;
		mutable	CryCriticalSection	m_cs;
	};


	//////////////////////////////////////////////////////////////////////////
	// Class implementation are in the platform specific headers
	// ex Xenon_MT.h
	//////////////////////////////////////////////////////////////////////////

#if !defined(XENON)
	///////////////////////////////////////////////////////////////////////////////
	// 
	// Multi-thread safe lock-less FIFO queue container for passing pointers between threads.
	// The queue only stores pointers to T, it does not copy the contents of T.
	//
	//////////////////////////////////////////////////////////////////////////
	template <class T>
	class CLocklessPointerQueue
	{
	public:
		CLocklessPointerQueue() { m_lockFreeQueue.reserve(32); };
		~CLocklessPointerQueue() {};

		// Check's if queue is empty.
		bool	empty() const;

		// Pushes item to the queue, only pointer is stored, T contents are not copied.
		void	push( T* ptr );
		// pop can return NULL, always check for it before use.
		T*    pop();

	private:
		queue<T*> m_lockFreeQueue;
		mutable CryCriticalSection m_lock;
	};

	//////////////////////////////////////////////////////////////////////////
	template <class T>
	inline bool CLocklessPointerQueue<T>::empty() const
	{
		CryAutoCriticalSection lock(m_lock);
		return m_lockFreeQueue.empty();
	}

	//////////////////////////////////////////////////////////////////////////
	template <class T>
	inline void CLocklessPointerQueue<T>::push( T* ptr )
	{
		m_lockFreeQueue.push(ptr);
	}

	//////////////////////////////////////////////////////////////////////////
	template <class T>
	inline T* CLocklessPointerQueue<T>::pop()
	{
		T* val = NULL;
		m_lockFreeQueue.try_pop(val);
		return val;
	}

#endif
}; // namespace CryMT





#endif // __MultiThread_Containters_h__
