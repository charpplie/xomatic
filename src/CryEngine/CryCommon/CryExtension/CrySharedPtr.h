//////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2009.
// -------------------------------------------------------------------------
//  File name:   CrySharedPtr.h
//  Version:     v1.00
//  Created:     02/25/2009 by CarstenW
//  Description: Part of CryEngine's extension framework.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CRYSHAREDPTR_H_
#define _CRYSHAREDPTR_H_

#pragma once


#include "../CryThread.h"


//namespace crysp_details
//{

	class sp_counted_base
	{
	public:
		sp_counted_base() : useCount(1), weakCount(1)
		{
		}

		virtual ~sp_counted_base()
		{
		}

		virtual void dispose() = 0;

		virtual void destroy()
		{
			delete this;
		}

		void release()
		{
			if (CryInterlockedDecrement(&useCount) == 0)
			{
				dispose();
				weak_release();
			}
		}

		void add_ref_copy()
		{
			CryInterlockedIncrement(&useCount);
		}

		//bool add_ref_lock()
		//{
		//	for( ;; )
		//	{
		//		int tmp = static_cast<int const volatile&>(useCount);
		//		if (tmp == 0)
		//			return false;
		//		if (CryInterlockedCompareExchange(&useCount, tmp + 1, tmp) == tmp)
		//			return true;
		//	}
		//}

		void weak_add_ref()
		{
			CryInterlockedIncrement(&weakCount);
		}

		void weak_release()
		{
			if (CryInterlockedDecrement(&weakCount) == 0)
			{
				destroy();
			}
		}

	private:
		int useCount;
		int weakCount;
	};


	template <class X>
	class sp_counted_impl : public sp_counted_base
	{
	public:
		explicit sp_counted_impl(X* px) : px(px)
		{
		}

		virtual void dispose()
		{
			delete px;
		}

	private:
		X* px;
	};

	class shared_count
	{
	public:
		shared_count() : pi(0)
		{
		}

		~shared_count()
		{
			if (pi != 0)
				pi->release();
		}

		template <class Y>
		explicit shared_count(Y* p) : pi(0)
		{
			pi = new sp_counted_impl<Y>(p);
		}

		shared_count(const shared_count& r) : pi(r.pi)
		{
			if (pi != 0)
				pi->add_ref_copy();
		}

		void swap(shared_count & r)
		{
			sp_counted_base* tmp = r.pi;
			r.pi = pi;
			pi = tmp;
		}

	private:
		sp_counted_base* pi;
	};
//} // namespace crysp_details


template <typename T>
class cryshared_ptr
{
private:
	typedef cryshared_ptr<T> this_type;

public:
	cryshared_ptr() : px(0), pn()
	{
	}

	explicit cryshared_ptr(T* p) : px(p), pn(p)
	{
	}

	template <class Y>
	explicit cryshared_ptr(Y* p) : px(p), pn(p)
	{
	}

	cryshared_ptr(const cryshared_ptr& r) : px(r.px), pn(r.pn)
	{
	}

	template <class Y>
	cryshared_ptr(const cryshared_ptr<Y>& r) : px(r.px), pn(r.pn)
	{
	}

	template <class Y>
	cryshared_ptr(const cryshared_ptr<Y>& r, T* p) : px(p), pn(r.pn)
	{
	}

	T* get() const
	{
		return px;
	}

	T* operator->() const
	{
		return px;
	}

	operator bool () const
	{
		return px != 0;
	}

	void swap(cryshared_ptr<T>& other)
	{
		T* tmp = other.px;
		other.px = px;
		px = tmp;

		pn.swap(other.pn);
	}

	cryshared_ptr& operator=(const cryshared_ptr& r)
	{
		this_type(r).swap(*this);
		return *this;
	}

	template <class Y>
	cryshared_ptr& operator=(const cryshared_ptr<Y> & r)
	{
		this_type(r).swap(*this);
		return *this;
	}

	void reset()
	{
		this_type().swap(*this);
	}

	template <class Y>
	void reset(Y* p)
	{
		assert(p == 0 || p != px);
		this_type(p).swap(*this);
	}

private:
	template <class Y> friend class cryshared_ptr;

	T* px;
	/*crysp_details::*/shared_count pn;
};


template <class T, class U> inline bool operator==(cryshared_ptr<T> const & a, cryshared_ptr<U> const & b)
{
	return a.get() == b.get();
}

template <class T, class U> inline bool operator!=(cryshared_ptr<T> const & a, cryshared_ptr<U> const & b)
{
	return a.get() != b.get();
}


#endif // #ifndef _CRYSHAREDPTR_H_
