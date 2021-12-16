
#include "StdAfx.h"
#include "ObjectContainer.h"
#include "Reference.h"
#include "aisalthandle.h"
#include "AISaltBufferArray.h"

// Annoying...
#include "AIVehicle.h"  
#include "AIPlayer.h"  


int CObjectContainer::m_snObjectsRegistered = 0;
int CObjectContainer::m_snObjectsDeregistered = 0;

CObjectContainer::CObjectContainer(void) {}


// pObject is essential - have to register something of course
// ref is to set a strong reference if we have one, otherwise we accept it is unowned
bool CObjectContainer::RegisterObjectUntyped(CAIObject *pObject, CAbstractUntypedRef *ref )
{
	CCCPOINT(RegisterObjectUntyped);
	assert(pObject);
	assert(ref);

	m_snObjectsRegistered++;
	CAISaltHandle<> handle;

#if _DEBUG
	{
		// Debugging - check that no existing stub has this pointer (slow)
		for (int i=0; i<MAX_AI_OBJECTS; i++)
		{
			if (m_StubBuffer[i].m_pObject == pObject)
			 handle = m_SaltBuffer.GetValidHandleForIndex(i);

		}
		bool bExisted = (handle);		// Bool conversion tests validity
		if (bExisted)
		{
			gEnv->pLog->LogError("AI: CObjectContainer::RegisterObjectUntyped - Object already registered - %x @%6d \"%s\" ", (unsigned)(pObject), HandleToId(handle), pObject->GetName() );
			return false;
		}
	}
#endif

	handle = m_SaltBuffer.InsertDynamic();
	CStub *pStub = LookupValidStub( handle );
	pStub->m_pObject = pObject;

	// Assign to the strong reference
	tAIObjectID nID = HandleToId(handle);
	ref->Assign( nID );

	AILogComment("Registered object %x @%6d \"%s\" ", (unsigned)(pObject), nID, pObject->GetName() );

	return true;
}


bool CObjectContainer::DeregisterObjectUntyped( CAIObject *pObject )
{
	CWeakRef<CAIObject> ref(GetWeakRef( pObject ));
	return DeregisterObjectUntyped( &ref );
}


// Only strong refs should ever be passed in
bool CObjectContainer::DeregisterObjectUntyped( CAbstractUntypedRef *ref )
{
	CCCPOINT(DeregisterObjectUntyped);

	// (MATT) Checks for double-deregister in debug might be helpful here - but if the mechanisms are enforced it shouldn't be possible {2009/03/30}

	// (MATT) Perhaps this isn't the right place to increment - they are only pushed on a list, after all {2009/04/07}
	m_snObjectsDeregistered++;
	assert(m_snObjectsRegistered >= m_snObjectsDeregistered);
	
	tAIObjectID nID = ref->GetObjectID();
	CAISaltHandle<> handle = IdToHandle(nID);
	bool bValidHandle = CheckHandle(handle);
	assert(bValidHandle);

	m_DeregisteredBuffer.push_back(handle);
	ref->Assign(INVALID_AIOBJECTID);

	// For debug purposes, find out about the object itself
	CStub *pStub = LookupValidStub(handle);
	CAIObject *pObject = pStub->GetAIObject();

	AILogComment("Deregistered object %x @%6d \"%s\" ", (unsigned)(pObject), nID, pObject->GetName() );

	return true;
}

bool CObjectContainer::IsValid( const CAbstractUntypedRef &ref ) const
{
	CAISaltHandle<> handle = IdToHandle(ref.GetObjectID());
	return CheckHandle(handle);
}

CStub * CObjectContainer::GetStub( const CAbstractUntypedRef &ref ) const
{
	CAISaltHandle<> handle = IdToHandle(ref.GetObjectID());
	PrefetchLine( &(m_StubBuffer[handle.GetIndex()]), 0);
	if (CheckHandle(handle))
	{
		return LookupValidStub(handle);
	}
	return NULL;
}

CStub * CObjectContainer::GetValidStub( const CAbstractUntypedRef &ref ) const
{
	CAISaltHandle<> handle = IdToHandle(ref.GetObjectID());
	return LookupValidStub(handle);
}


// This implictly has to mark something for deletion
// Or, at least, it does usually....
// No point returning the pointer because that encourages people to delete it themselves, which is no use


// We (should!) call this at the end of each AI frame
int CObjectContainer::ReleaseDeregisteredObjects(void)
{
	CCCPOINT(ReleaseDeregisteredObjects);
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	int nReleased = 0;

	// We use a double-buffer approach
	// Currently, deleting objects currently triggers deregistration of any sub-objects
	// It could be made to work with one vector but this seems more debuggable
	int loopLimit = 100;
	while (!m_DeregisteredBuffer.empty() && loopLimit)
	{
		assert(--loopLimit > 0);
		m_DeregisteredBuffer.swap(m_DeregisteredWorkingBuffer);

		TVecAIObjects::iterator itO = m_DeregisteredWorkingBuffer.begin();
		TVecAIObjects::iterator itOEnd = m_DeregisteredWorkingBuffer.end();
		for ( ; itO != itOEnd ; itO++)
		{
			CStub *pStub = LookupValidStub(*itO);
			CAIObject *pObject = pStub->GetAIObject();

			// Before we start removing the object check for Proxy and release if necessary
			// This allows us to prepare for any proxy queries during remove procedure, such as checking health
			CAIActor *pActor = pObject->CastToCAIActor();
			IAIActorProxy *pProxy = pActor ? pActor->GetProxy() : NULL;

			tAIObjectID nID = HandleToId(*itO);
			AILogComment("Releasing object %x @%6d proxy %x \"%s\" ",
				reinterpret_cast<unsigned int>(pObject), nID, reinterpret_cast<unsigned int>(pProxy), pObject->GetName() );

			if (pProxy)
				pActor->GetProxy()->Release();

			// For transitional purposes (at least) call the remove code now
			GetAISystem()->RemoveObject(pObject); // use false as workaround to avoid recursing

			// Delete the object. Past this point, weak refs will still function and you can still fetch them for a given pointer, but the object itself is gone
			// It might be better to put off that delete until we wipe pointers from the stubs, but if GetWeakRef disappeared, so would the need for all that.
			pObject->Release();
			//m_DeregisteredObjects.push_back(*itO);

			// (KEVIN) Doing salt buffer remove inline, so as objects handling being removed, their weak references will not return invalid memory.
			assert(pStub);
			pStub->m_pObject = NULL;
			m_SaltBuffer.Remove(*itO);

			nReleased++;
		}
		m_DeregisteredWorkingBuffer.clear();
	}

	// (KEVIN) Doing salt buffer remove inline, so as objects handling being removed, their weak references will not return invalid memory.

	// (MATT) Here, we finally invalidate handle and wipe the pointer from the stub {2009/04/22}
	/*TVecAIObjects::iterator itO = m_DeregisteredObjects.begin();
	TVecAIObjects::iterator itOEnd = m_DeregisteredObjects.end();
	for ( ; itO != itOEnd ; itO++)
	{
		CStub *pStub = LookupValidStub(*itO);
		assert(pStub);
		pStub->m_pObject = NULL;
		m_SaltBuffer.Remove(*itO);
	}
	m_DeregisteredObjects.clear();*/

	return nReleased;
}

CStub * CObjectContainer::LookupValidStub( CAISaltHandle<> handle ) const
{
	unsigned short nIndex = handle.GetIndex();
	return &( m_StubBuffer[nIndex] );
}

tAIObjectID CObjectContainer::LookupObjectID( CAIObject * pObject )
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	tAIObjectID nID = INVALID_AIOBJECTID;
	if (pObject)
	{
		// Again, this is a workaround for transition purposes only	
		for ( int it = 0; it < MAX_AI_OBJECTS; it++ )
		{
			if (m_StubBuffer[it].GetAIObject() == pObject)
			{
				CAISaltHandle<> handle = m_SaltBuffer.GetValidHandleForIndex(it);
				nID = HandleToId(handle);
			}
		}
		// REF_TODO : (MATT) Couldn't find the object {2008/02/12:20:12:47}
		// If you hit this, the object probably wasn't created via the ObjectContainer system.
		// Comment out the assert and continue, it should cope, until you can find the problem.
		if (nID == INVALID_AIOBJECTID)
		{
			AIAssert(false);
			AIError("Failed to find weak reference for object %s", pObject->GetName());
		}
	}
	return nID;
}



CWeakRef <CAIObject> CObjectContainer::GetWeakRef( tAIObjectID nID )
{
	CAISaltHandle<> handle = IdToHandle(nID);
	if (CheckHandle(handle))
		return CWeakRef <CAIObject>(nID);
	return NILREF;
}


void CObjectContainer::DumpRegistered()
{
	int nCount = 0;
	for ( int it = 0; it < MAX_AI_OBJECTS; it++ )
		if (m_StubBuffer[it].m_pObject)
		{
			CAIObject *pObj = m_StubBuffer[it].m_pObject;
			CWeakRef<CAIObject> weakRef = GetWeakRef(pObj);
			gEnv->pLog->Log("Object %x @%6d \"%s\" ", (unsigned)(pObj), weakRef.GetObjectID(), pObj->GetName() );
			nCount++;
		}
	gEnv->pLog->Log("Total object count %d", nCount);
}


void CObjectContainer::Serialize( TSerialize ser, CObjectTracker& objectTracker )
{
	CAISystem *pAISystem = GetAISystem();

	// Deal with the deregistration list as it makes no sense to serialise those objects
	ReleaseDeregisteredObjects();

	// How many objects should be have right now?
	int nTotalObjects = m_snObjectsRegistered - m_snObjectsDeregistered;
	int nTotalSerialised = 0;

	{
		// Double-check that we have a consistent count
		int nCount = 0;
		for ( int it = 0; it < MAX_AI_OBJECTS; it++ )
			if (m_StubBuffer[it].m_pObject) 
				nCount++;
		if (nCount != nTotalObjects)
		{
			DumpRegistered();
		}
		CRY_ASSERT_MESSAGE(nCount == nTotalObjects, "Something has leaked AI objects! Check the log for details");
	}

	ser.BeginGroup("ObjectContainer");

	const bool bReading = ser.IsReading();

	if (bReading)
	{
		// Should first ensure we've already removed all objects!

		// Reset salt buffer
		m_SaltBuffer.Reset();
		// Erase all stubs
		for ( int it = 0; it < MAX_AI_OBJECTS; it++ )
			m_StubBuffer[it].Reset();
	}

	ser.Value("total",nTotalObjects);

	std::vector<IAIObject*> postSerializeObjects;

	for ( int it = 0 ; nTotalSerialised < nTotalObjects && it < MAX_AI_OBJECTS ; ++it )
	{
		CAIObject *pObj = NULL;
		int type = 0; // Indicates no object
		tAIObjectID nID = INVALID_AIOBJECTID;

		if (ser.IsWriting())
		{
			// First, is there a valid object here?
			pObj = m_StubBuffer[it].m_pObject;
			if (!pObj)
				continue;
		}

		// If writing, we've established this is a valid index
		// If reading, we will read the next valid index and skip ahead
		ser.BeginGroup("Entry");
		ser.Value("index",it);		// Reading may change the loop iterator

		// Serialise the handle / salt id
		if (ser.IsWriting())
		{
			CAISaltHandle<> handle = m_SaltBuffer.GetValidHandleForIndex(it);
			nID = HandleToId(handle);
		}

		ser.Value("id", nID);

		if (bReading)
		{
			CAISaltHandle<> handle = IdToHandle(nID);
			m_SaltBuffer.InsertKnownHandle( handle );
		}

		if (ser.IsWriting())
		{
			// Figure out the object type value, always >=1
			if (pObj)
			{
				// Working from child to parent through the hierarchy is essential here - each object can be many types
				// This could all be much neater with a different type system to fastcast.
				// I'm deliberately _not_ adding yet another enum
				if      (pObj->CastToCAIVehicle())  type = 7;
				else if (pObj->CastToCPuppet())     type = 6;
				else if (pObj->CastToCPipeUser())   type = 5;
				else if (pObj->CastToCAIPlayer())   type = 4;
				else if (pObj->CastToCLeader())     type = 3;
				else if (pObj->CastToCAIActor())    type = 2;
				else              /* CAIObject */   type = 1;
			}
		}

		// Serialise type value
		ser.Value("type",type);

		if (bReading)
		{
			//Read type for creation
			// I'm deliberately _not_ adding yet another enum
			switch(type)
			{
			case 7: pObj = new CAIVehicle; break;
			case 6: pObj = new CPuppet;    break;
			case 5: pObj = new CPipeUser;  break;
			case 4: pObj = new CAIPlayer;  break;
			case 3: pObj = new CLeader(0); break;	// Groupid is reqd
			case 2: pObj = new CAIActor;   break;
			case 1: pObj = new CAIObject;  break;
			default: assert(false);
			}

			m_StubBuffer[it].m_pObject = pObj;
			m_snObjectsRegistered++;
		}

		// (MATT) Note that this call may reach CAIActor, which currently handles serialising the proxies {2009/04/30}
		if (pObj->Serialize( ser, objectTracker ))
		{
			// [AlexMc|03.02.10] This object has signaled that it needs a PostSerialize call once we've created all objects
			postSerializeObjects.push_back(pObj);
		}
		nTotalSerialised++;
		
		if (bReading)
			AILogComment("Serialisation created object %x @%6d \"%s\" ", (unsigned)(pObj), nID, pObj->GetName() );

		// Simple back-and-forth test, should be valid at this point
		assert(GetWeakRef(pObj).IsValid());

		ser.EndGroup();
	}

	// [AlexMc|03.02.10] Give objects a chance to fixup pointers.
	for (size_t i = 0, nPostSerializeCount = postSerializeObjects.size(); i < nPostSerializeCount; ++i)
	{
		postSerializeObjects[i]->PostSerialize(bReading);
	}

	ser.EndGroup();
}

