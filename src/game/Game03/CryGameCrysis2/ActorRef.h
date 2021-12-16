/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Helper classes to handle weak refs for CActors

-------------------------------------------------------------------------
History:
- 12-2-2010		Benito Gangoso Rodriguez

*************************************************************************/

#pragma once

#ifndef _ACTORREF_H_
#define _ACTORREF_H_

#include "PoolAllocator.h"

//////////////////////////////////////////////////////////////////////////
// Helper structure shared between 'strong' reference, and weak ones
//////////////////////////////////////////////////////////////////////////
struct SActorRefCount
{

	SActorRefCount()
		: m_refCount(0)
		, m_validRef(false)
	{

	}

	~SActorRefCount()
	{
		CRY_ASSERT((m_refCount == 0) && (m_validRef == false));
	}

	ILINE void AddRef()
	{
		m_refCount ++;
	}

	ILINE void ReleaseRef()
	{
		m_refCount --;
		CRY_ASSERT(m_refCount >= 0);
	}

	ILINE void Validate()
	{
		m_validRef = true;
	}

	ILINE void Invalidate()
	{
		m_validRef = false;
	}

	ILINE bool IsValid() const
	{
		return m_validRef;
	}

	ILINE bool HasReferences() const
	{
		return (m_refCount > 0);
	}

private:

	int m_refCount;
	bool m_validRef;
};

struct SIActorRefCountWrapper;
struct SActorWeakRef;
class  CActor;

//////////////////////////////////////////////////////////////////////////
/// One of this will be contained per CActor instance, which will be
/// in charge for initialization. It works like a wrapper around CActor
/// itself. When actor gets destroyed, it will set valid flag to false
/// for weak references.
//////////////////////////////////////////////////////////////////////////
struct SActorRefWrapper
{
private:
	friend struct SActorWeakRef;
	friend class CActor;

public:

	SActorRefWrapper()
		: m_pActor (NULL)
		, m_pRefCount(NULL)
	{

	}

	~SActorRefWrapper()
	{
		ReleaseRef();
	}

private:

	SActorRefWrapper& operator = (CActor& actor);

	//This two are undefined explicitly 
	SActorRefWrapper (const SActorRefWrapper& otherWrapper);
	SActorRefWrapper& operator = (const SActorRefWrapper& otherRef);

	void ReleaseRef();

	CActor* m_pActor;
	SActorRefCount* m_pRefCount;
};

//////////////////////////////////////////////////////////////////////////
/// This ones should be used/stored from any other code which wants to 
/// keep a safe reference to CActor. Reference is invalidated once the actor
/// gets destroyed.
//////////////////////////////////////////////////////////////////////////

struct SActorWeakRef
{
	SActorWeakRef()
		: m_pActor(NULL)
		, m_pRefCount(NULL)
	{

	}

	~SActorWeakRef()
	{
		ReleaseRef();
	}

	SActorWeakRef(const SActorWeakRef& otherRef)
	{
		m_pActor = otherRef.m_pActor;
		m_pRefCount = otherRef.m_pRefCount;

		AddRef();
	}

	SActorWeakRef (const SActorRefWrapper& refWrapper) 
	{
		m_pActor = refWrapper.m_pActor;
		m_pRefCount = refWrapper.m_pRefCount;

		AddRef();
	}

	SActorWeakRef& operator = (const SActorWeakRef& otherRef) 
	{
		if (Get() != otherRef.Get()) 
		{
			ReleaseRef ();

			m_pActor = otherRef.m_pActor;
			m_pRefCount = otherRef.m_pRefCount;

			AddRef();
		}

		return *this;
	}

	void Reset()
	{
		ReleaseRef ();
	}

	SActorWeakRef& operator = (const SActorRefWrapper& refWrapper) 
	{
		if (Get() != refWrapper.m_pActor) 
		{
			ReleaseRef ();

			m_pActor = refWrapper.m_pActor;
			m_pRefCount = refWrapper.m_pRefCount;

			AddRef();
		}

		return *this;
	}

	CActor* operator -> () const 
	{
		return Get();
	}

	CActor& operator * () const
	{
		CRY_ASSERT(Get() != NULL);

		return *Get();
	}

	ILINE CActor* Get() const 
	{
		return ((m_pRefCount) && (m_pRefCount->IsValid())) ? m_pActor : NULL;
	}

private:

	void AddRef()
	{
		if (m_pActor)
		{
			m_pRefCount->AddRef();
		}
	}

	void ReleaseRef();

	CActor* m_pActor;
	SActorRefCount* m_pRefCount;
};

#endif
