///////////////////////////////////////////////////////////////////////////////
// Use these two classes to bind two objects, when any of them breaking
// the relation for some reason, the other end are guaranteed to be signaled
// and have the chance to do some other cleanup work
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "SWFwd.h"


SW_NAMESPACE_BEGIN();
namespace utils{


struct TBindingPtr;

// this is a policy-like class that one can inherent
// that will give the subclass ability to be binded by a TBindingPtr
struct Bindable
{
private:
	typedef void THost;
private:
	TBindingPtr*	m_pBindingPtr;
	THost*			m_pBindingHost;		// the host object that holds the pointer, which we might want to dereference

public:
	Bindable()
		:m_pBindingPtr(NULL)
	{}

	bool IsBinded()const {return m_pBindingPtr;}
	inline void Unbind();
	void* GetBindingHost()const { return  (m_pBindingPtr)?m_pBindingHost:NULL;}
protected:
	virtual ~Bindable() { Unbind(); }
	inline void DoUnbind();

	// Override this method if you need to do anything before unbind
	virtual void BeforeUnbind(){}
private:
	void SetBinding(TBindingPtr* pPtr, void* pObj)
	{
		if (m_pBindingPtr)
			DoUnbind();		// but don't call beforeUnbind, because we'll rebind it right now
		m_pBindingPtr = pPtr;
		m_pBindingHost = pObj;
	}

	friend TBindingPtr;
};


struct TBindingPtr
{
private:
	Bindable* m_pBinded;

public:
	TBindingPtr()
		:m_pBinded(NULL)
	{}
	TBindingPtr(Bindable* pIn)
		:m_pBinded(pIn)
	{}

	~TBindingPtr()
	{
		Unbind();
	}

	operator Bindable*()const			{ return m_pBinded; }
	Bindable* operator ->()const		{ return m_pBinded; }
	operator bool()const				{ return m_pBinded; }
	bool operator !()const				{ return !m_pBinded; }


	bool IsBinded()const { return m_pBinded; }
	Bindable* Bind(Bindable* pNew, void* pBindingObj)
	{
		Bindable* pOld = m_pBinded;
		if (m_pBinded)
			m_pBinded->Unbind();

		pNew->SetBinding(this, pBindingObj);
		m_pBinded = pNew;
		return pOld;
	}

	void Unbind()
	{
		if (m_pBinded) m_pBinded->Unbind();
	}

protected:
	// Use Bind() instead
	TBindingPtr& operator =(TBindingPtr const&);
	//TBindingPtr const& operator =(TBindingPtr const&) const;
	Bindable* operator =(Bindable* );

	friend Bindable;
};


//////////////////////////////////////////////////////////////////////////

void Bindable::Unbind()
{
	if (m_pBindingPtr)
	{
		BeforeUnbind();
		DoUnbind();
	}
}
void Bindable::DoUnbind()
{
	m_pBindingPtr->m_pBinded = NULL;
	m_pBindingPtr = NULL;
}

}
SW_NAMESPACE_END();
