////////////////////////////////////////////////////////////////////////////
//
//  StalTech Engine Source File.
//  Copyright (C), StalTech Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   ObjectContainer.h
//  Created:     11/02/2008 by Matthew
//  Description: Manages the stubs and pointers to all AI objects
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AIOBJECTCONTAINER
#define __AIOBJECTCONTAINER

#pragma once

#include "Stub.h"
#include "AISaltBufferArray.h"
#include <vector>

#include "IAIObjectManager.h"

struct IAIObject;
class CAIObject;
class CAbstractUntypedRef;
template <class T> class CAbstractRef;
template <class T> class CStrongRef;
template <class T> class CWeakRef;

#define MAX_AI_OBJECTS 10240


/**
* Container for AI Objects.
* All AI objects should be registered through the the ObjectContainer, which is the only way strong references
* can be assigned. The container manages the objects themselves, their stubs, their validity status 
* (though the use of salts) and the destruction of the objects at the end of an AI frame.
*/
class CObjectContainer
{
public:

	CObjectContainer(void);

	bool IsValid( const CAbstractUntypedRef &ref ) const;

	// Try to fetch the stub associated with a reference
	CStub * GetStub(  const CAbstractUntypedRef &ref ) const;

	// Fetch the stub associated with a reference, without checking validity
	CStub * GetValidStub(  const CAbstractUntypedRef &ref ) const;

	// Add a new AI Object to the manager, which will then own it
	// In theory, this might fail due to lack of buffer space
	// Might need to pass in an existing stub...
	// Also, the Object should keep a pointer to its stub... hmm...
	//template <class T> bool RegisterObject( T *pObject, CStrongRef<T> &ref );
	template <class T> bool RegisterObject( T *pObject, CStrongRef<T> &ref )
	{
		// First, check and release the reference if it is already used
		// Recreating an object like this usually isn't necessary but the semantics make sense
		if (!ref.IsNil())
		{
			ref.Release();
		}

		// The templating is really just a wrapper, so call an untyped private function to do the real work
		bool bResult = RegisterObjectUntyped(pObject, &ref);
		pObject->SetSelfReference(ref);
		return bResult;
	}

	// Remove an object from the manager via its handle
	// Returns false if no object was associated with that handle
	template <class T> bool DeregisterObject( CStrongRef<T> &ref )
	{
		// The templating is really just a wrapper, so call an untyped private function to do the real work	
		return DeregisterObjectUntyped( &ref );
	}

	int ReleaseDeregisteredObjects(void);

	// Slow!!!
	template <class T> CWeakRef<T> GetWeakRef( T * pObject )
	{
		// We return a weak reference based on the same type as the pointer
		// Thus we assume the user knew what he was doing when he cast that pointer and follow suit
		return CWeakRef<T>( LookupObjectID( (CAIObject*) pObject) );
	}

	// Get a weak reference corresponding to a handle
	CWeakRef <CAIObject> GetWeakRef( tAIObjectID nID );

	int GetNumRegistered() { return m_snObjectsRegistered - m_snObjectsDeregistered; }

	void DumpRegistered();

	// Serialise the Object Container and all the objects it contains
	// Note that without this call, no objects would be serialised.
	void Serialize( TSerialize ser, CObjectTracker& objectTracker );

protected:
	// Perform much of the work of registering an object ignoring the type info
	bool RegisterObjectUntyped(CAIObject *pObject, CAbstractUntypedRef *ref);

	// Perform much of the work of deregistering an object ignoring the type info
	bool DeregisterObjectUntyped(CAIObject *pObject);
	bool DeregisterObjectUntyped(CAbstractUntypedRef * pRef);


	// Check if a handle is valid
	ILINE bool CheckHandle( CAISaltHandle<> handle ) const
	{
		// checking this in handle get function also
		return ( m_SaltBuffer.IsValid( handle ) );
	}

	// Look up a stub, assuming valid handle
	CStub * LookupValidStub( CAISaltHandle<> handle ) const;

	tAIObjectID LookupObjectID( CAIObject * pObject );


	typedef CAISaltBufferArray<unsigned short, unsigned short, MAX_AI_OBJECTS> TSaltBuffer;
	typedef std::vector<CAISaltHandle<> > TVecAIObjects;

	TSaltBuffer m_SaltBuffer;
	mutable CStub m_StubBuffer[MAX_AI_OBJECTS];			// er, yeah, bit of a workaround :-)
	TVecAIObjects m_DeregisteredBuffer;                 // When deregistered, CAIObjects are added here
	TVecAIObjects m_DeregisteredWorkingBuffer;          // Working buffer during deletion (consider re-entrant deletes)
	TVecAIObjects m_DeregisteredObjects;								// Final collection of all objects deleted that frame
	static int m_snObjectsRegistered, m_snObjectsDeregistered;
};




#endif // __AIOBJECTCONTAINER