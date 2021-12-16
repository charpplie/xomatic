/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Helper classes to handle weak refs for CActors

-------------------------------------------------------------------------
History:
- 12-2-2010		Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "ActorRef.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SActorRefCountAllocator
{
public:
	typedef stl::PoolAllocator<sizeof(SActorRefCount), stl::PoolAllocatorSynchronizationSinglethreaded> ActorRefCountAlloc;

	static SActorRefCount * Create()
	{
		return new (s_poolAlloc.Allocate()) SActorRefCount();
	}

	static void Release(SActorRefCount* pRefCount)
	{
		pRefCount->~SActorRefCount();
		s_poolAlloc.Deallocate(pRefCount);
	}

private:

	static ActorRefCountAlloc s_poolAlloc;
};

SActorRefCountAllocator::ActorRefCountAlloc SActorRefCountAllocator::s_poolAlloc;


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

SActorRefWrapper& SActorRefWrapper::operator=( CActor& actor )
{
	CRY_ASSERT(m_pActor == NULL);

	if (m_pActor == NULL)
	{
		m_pActor = &actor;
		m_pRefCount = SActorRefCountAllocator::Create();

		m_pRefCount->Validate();
		m_pRefCount->AddRef();
	}

	return *this;
}

void SActorRefWrapper::ReleaseRef()
{
	if (m_pActor)
	{
		CRY_ASSERT(m_pRefCount);

		m_pRefCount->Invalidate();
		m_pRefCount->ReleaseRef();

		if (!m_pRefCount->HasReferences())
		{
			SActorRefCountAllocator::Release(m_pRefCount);
		}

		m_pRefCount = NULL;

		//This pointer is not deleted, this structure is just a wrapper to help
		//tracking that the pointer is valid through m_pRefCount->m_validRef/IsValid()
		m_pActor = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void SActorWeakRef::ReleaseRef()
{
	if (m_pActor)
	{
		CRY_ASSERT(m_pRefCount);

		m_pRefCount->ReleaseRef();

		if (!m_pRefCount->HasReferences())
		{
			SActorRefCountAllocator::Release(m_pRefCount);
		}

		m_pRefCount = NULL;
		m_pActor = NULL;
	}
}