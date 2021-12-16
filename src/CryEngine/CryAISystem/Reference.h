////////////////////////////////////////////////////////////////////////////
//
//  StalTech Engine Source File.
//  Copyright (C), StalTech Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   Reference.h
//  Created:     11/02/2008 by Matthew
//  Description: References to AI objects
// -------------------------------------------------------------------------
//  History:
//
//  Notes:
//    - this-> is used when referring to members to compile on GCC - due to specialization rules compiler
//      cannot assume any inherited members exist in a templated base (Meyer, Item 43, Effective C++)
////////////////////////////////////////////////////////////////////////////

/**
 * References to AI objects.
 *
 * (More accurately: smartpointers)
 * These references can be preserved across frames and dereferenced to give an AIStub * or AIObject *, which should
 * always be discarded between frames.
 * There are four main types:
 * tAIObjectID -  A simple integer ID. This is the only type that should pass outside the AI system.
 * WeakRef -      Wraps an ID with templated type. For use within the AI system.
 * StrongRef -    Wraps an ID with templated type and auto_ptr semantics. Uniquely owns the object, controlling its validity. Within AI.
 * CountedRef -   Wraps a StrongRef for reference-counting semantics. Within AI.
 *
 * Notes:
 * Strong should be preferred but are not compatible (and will not compile) with STL containers. Counted can be used in this case.
 * Counted is rather bolted on and should be properly integrated!
 */

#ifndef __AI_REFERENCE_H_
#define __AI_REFERENCE_H_

#pragma once

#include "ObjectContainer.h"
#include "aisalthandle.h"
#include "Stub.h"

//#define DEBUG_REFERENCES



/**
* The simple integer AI object ID, for use outside the AI system.
*/
#
// (MATT) This should really live in a minimal AI include, which right now we don't have  {2009/04/08}
#ifndef INVALID_AIOBJECTID
typedef uint32	tAIObjectID;
#define INVALID_AIOBJECTID ((tAIObjectID)(0))
#endif

static ILINE CAISaltHandle<> IdToHandle( const tAIObjectID id ) { return CAISaltHandle<>(id>>16,id&0xffff); }
static ILINE tAIObjectID HandleToId( const CAISaltHandle<> id ) { return (((uint32)id.GetSalt())<<16) | ((uint32)id.GetIndex()); }

template <class T> class CWeakRef;
template <class T> class CStrongRef;
template <class T> class CCountedRef;

/**
* Enum whose single member, NILREF, represents an empty, untyped reference.
* Only for syntactic sugar, to avoid the use of CWeakRef<CMyClass>() where, with pointers, NULL would have sufficed.
* It is silently converted into a Nil weak reference of any type, hence very useful as a parameter when calling a function.
*/
enum type_nil_ref { NILREF };

/**
 * An abstract typeless base class for references.
 * Some functionality of the references is independent of type and so is defined here. 
 */
class CAbstractUntypedRef
{
friend class CObjectContainer;

public:
	/**
	 * Test if reference is currently unassigned.
	 * Being assigned does not imply validity. Test for this with IsValid().
 	 * @return True if currently assigned to an object.
	 */
	bool IsNil(void) const { return (m_nID == INVALID_AIOBJECTID); }

	/**
	* Test if reference is currently assigned.
	* Being assigned does not imply validity. Test for this with IsValid().
	* This method is simply the converse of IsNil - but avoids potential double-negation confusion in its usage.
	* @return True if currently assigned to an object.
	*/
	bool IsSet(void) const { return (m_nID != INVALID_AIOBJECTID); }


	/**
	 * Return a simple AI object ID.
	 * @return the tAIObjectID wrapped, regardless of validity. INVALID_AIOBJECTID if unassigned;
	 */	
	tAIObjectID GetObjectID(void) const { return m_nID; }

	/**
	 * Return the AI object, if reference is valid.
	 * The object may have been removed since the reference was assigned.
	 * @return Pointer to a valid AI object or NULL.
	 */	
	IAIObject * GetIAIObject(void) const
	{
		if (m_nID == INVALID_AIOBJECTID)
			return NULL;
		CRY_ASSERT(gAIEnv.pObjectContainer);
		CStub * pStub = gAIEnv.pObjectContainer->GetStub(*this);
		if (!pStub)
			return NULL;

		// (MATT) A workaround - I don't want to have to include the object hierarchy {2009/02/03}
		return reinterpret_cast<IAIObject*>(pStub->GetAIObject());
	}

	/**
	 * Tests whether a reference and a pointer refer to the same object.
	 * An invalid reference and a NULL pointer compare false for now, but that is debatable.
	 * @return true iff reference is Valid and refers to the same object as the pointer, or Nil and pointer is NULL
	 */
	bool operator==(const IAIObject * const pThatObject) const
	{
		// (MATT) UNTESTED!!! {2009/01/29}
		// If our reference is Nil, which is equivalent to NULL...
		if (IsNil())
			return (pThatObject == NULL);

		// If not, this might be a weak reference so it might be invalid
		IAIObject* pThisObject = GetIAIObject();
		// If invalid, GetIAIObject will return NULL, in which case return false, as equality with something invalid is a property of dubious merit
		if (!pThisObject)
			return false;

		// Otherwise we should have two IAIObject pointers which, even with the vagaries of multiple inheritance, should be comparable
		bool bEqual = (pThisObject==pThatObject);
		return bEqual;
	}

	/**
	* Negation of operator== for a pointer
	*/
	bool operator!=(const IAIObject * const pThatObject) const
	{
		return ! (*this == pThatObject);
	}

	/**
	* Tests whether two references refer to the same object.
	* For now, if either reference is invalid we return false, even if both are invalid or even both refer to the same invalid object, which is debatable
	* @return true iff references are to the same Valid object or are both Nil
	*/
	bool operator==(const CAbstractUntypedRef &that) const
	{
		// (MATT) UNTESTED!!! {2009/01/29}
		// This could be optimised for comparing two strong refs - but you should be able to see why that would never be used.

		// If our reference is Nil, then only Nil is considered equal
		if (IsNil())
			return that.IsNil();

		// (MATT) Refactor this to avoid the fetch where possible {2009/02/15}

		// If invalid, GetIAIObject will return NULL, in which case return false, as equality with something invalid is a property of dubious merit
		// In principle, this lookup can be optimised out when comparing a strong with a weak
		if (!GetIAIObject())
			return false;

		// Compare IDs - if one was valid and they are equal then the other should be valid too!
		bool bEqual = (m_nID == that.m_nID);
		return bEqual;
	}

	/**
	* Negation of operator== for another reference
	*/
	bool operator!=(const CAbstractUntypedRef &that) const
	{
		return !(*this == that);
	}

	/**
	* Comparison operator to allow use in sorted containers
	*/
	bool operator< (const CAbstractUntypedRef &that) const
	{
		return this->m_nID < that.m_nID;
	}

protected:
	// Allow just CObjectContainer to create these, and we trust it to have ensured the type is appropriate
	void Assign( tAIObjectID nID ) { m_nID = nID; }

	tAIObjectID m_nID;
};



/**
 * An abstract typed base class for references.
 * Some functionality of the references is independent of type and so is defined here. 
 */
template <class T> class CAbstractRef : public CAbstractUntypedRef
{
public:

	/**
	 * Get a typed weak reference instance from any existing reference.
	 * Weak references can be created from both strong and weak references.
	 * @return A weak reference to the same object, if any.
	 */	
	CWeakRef<T> GetWeakRef(void) const {	return CWeakRef<T>( m_nID ); }
};



/**
 * Typed strong reference for defining ownership, with auto_ptr semantics.
 * An AI object can only be owned by just one strong reference. 
 * They are by definition always valid until Released.
 * Strong references can only be assigned centrally by the system when the object is created and only passes around by single-transferrable ownership.
 * If you need more - for instance STL-container compatability - see CCountedRef below.
 */
template <class T = CAIObject> class CStrongRef : public CAbstractRef<T>
{
public:
	/**
	* Construct an unassigned (nil) reference.
	*/
	CStrongRef<T>(void) { this->m_nID = INVALID_AIOBJECTID; }  // See note at top of file

	/**
	* Single transferable ownership constructor.
	*/
	CStrongRef ( CStrongRef<T> &ref ) 
	{
		this->m_nID = ref.GiveOwnership();
	}

	/**
	* Convert a NilRef into an unassigned strong reference.
	* Technically this is a copy-constructor but it's really just syntactic sugar
	*/
	CStrongRef<T>(type_nil_ref) { this->m_nID = INVALID_AIOBJECTID; }

	/**
	* Destructor automatically releases any object owned. 
	*/
	~CStrongRef<T>(void) { Release(); }

	/**
	* Single transferable ownership assignment.
	*/
	CStrongRef<T> &operator= ( CStrongRef<T> &ref ) 
	{
		// Do nothing if assigned to self
		if (this != &ref)
		{
			// Release any object currently owned
			Release(); 

			// Take ownership of object
			this->m_nID = ref.GiveOwnership();
		}

		return *this;
	}

	// Don't perform any stub checking as this is strong
	T * GetAIObject(void) const
	{ 
		T *pObject = static_cast<T*>(this->GetValidIAIObject());
		return pObject;
	}	

	// Don't perform any stub checking as this is strong
	IAIObject * GetIAIObject(void) const
	{
		IAIObject *pObject = this->GetValidIAIObject();
		return pObject;
	}

	/**
	* Release any object owned.
	* Deliberately named differently to the Reset method in weak references, because Releasing
	* a strong reference causes object deregistration - which is a much more significant event.
	* @return true iff this reference owned an object
	*/
	bool Release(void) 
	{ 
		if (this->m_nID != INVALID_AIOBJECTID)
		{
			CRY_ASSERT(gAIEnv.pObjectContainer);
			gAIEnv.pObjectContainer->DeregisterObject( *this );
			this->m_nID = INVALID_AIOBJECTID;
			return true;
		}
		return false;
	}

	operator bool() const { return !this->IsNil(); }

	void Serialize( TSerialize ser, const char * sName = NULL )
	{		
		// We are strong, so if we're reading from a serialization stream then must release any object we own
		// (MATT) No, the object container should already have dealt with all this, so we don't have to do anything, and releasing would be bad {2009/03/25}
		//if (ser.IsReading()) 
		//	Release();
			
		// Will the compiler coalesce all these? Or, would #T be useful?
		ser.Value( (sName ? sName : "strongRef") , this->m_nID );  
	}

	// Could define deref here - semantics are same as pointer for strong
	// Conversely, removing this is helpful in finding dodgy places in the code!
	T * operator->() const { return this->GetAIObject(); }


protected:
	// In principle, we could cache the CAIObject pointer here - but whether it's worth the bloat is debatable, especially with non-trivial stub

	/**
	* Return the AI object with no validity test
	* This is intended for use by strong references, who have ownership of objects, not general use
	* @return Pointer to a valid AI object
	*/
	IAIObject * GetValidIAIObject(void) const
	{
		if (this->m_nID == INVALID_AIOBJECTID)
			return NULL;
		CRY_ASSERT(gAIEnv.pObjectContainer);
		CStub * pStub = gAIEnv.pObjectContainer->GetValidStub(*this);

		// (MATT) A workaround - I don't want to have to include the object hierarchy {2009/02/03}
		return reinterpret_cast<IAIObject*>(pStub->GetAIObject());
	}

	tAIObjectID GiveOwnership(void)
	{
		// Here, obviously, we quietly revert to an empty reference without destroying the object we own. 
		// It is not a problem if we are Nil. Callers should be responsible new owners
		tAIObjectID nID = this->m_nID;
		this->m_nID = INVALID_AIOBJECTID;
		return nID;
	}
};

/**
 * Template function to convert a typed weak reference to another type.
 * Typed references can be cast implicitly to another type if the bare pointer could be cast implicitly.
 * Where that is not the case, StaticCast can explicitly cast to another type, if static_cast would accomplish this for a bare pointer.
 * This method, like static_cast, does not perform dynamic type checking.
 */
template <class S, class U> CWeakRef<S> StaticCast( const CAbstractRef<U> &ref )
{
	// Allow any cast that static_cast would allow on a bare pointer
	S *pDummy = static_cast<S*>( (U*)0 );	(void)pDummy;
	return CWeakRef<S>(ref.GetObjectID());
}


/**
* Typed weak reference.
* Weak references allow AI code to refer to an AI object that it does not own.
* A weak reference may be invalidated by the object being Released at _any_ time.
* However, any pointer obtained from is guaranteed valid until the end of the frame.
* Weak references are easily obtained from other strong or weak references.
* They may be passed around and stored freely.
*/
template <class T = CAIObject> class CWeakRef : public CAbstractRef<T>
{
friend class CObjectContainer;
friend class CAbstractRef<T>;
friend class CCountedRef<T>;
template <class S, class U> friend CWeakRef<S> StaticCast( const CAbstractRef<U> &ref );

public:
	/**
	* Construct an unassigned weak reference.
	*/
	CWeakRef<T>( void ) { this->m_nID = INVALID_AIOBJECTID; }

	/**
	* Construct a weak reference from any typed reference.
	*/
	template <class S> CWeakRef<T> ( const CAbstractRef<S> &ref ) 
	{
		// Allow this for anywhere an implicit pointer cast would succeed.
		T * pDummy = (S*) 0;(void)pDummy;
		this->m_nID = ref.GetObjectID();
	}

	/**
	* Convert a NilRef into an unassigned weak reference.
	* Technically this is a copy-constructor but it's really just syntactic sugar
	*/
	CWeakRef<T>(type_nil_ref) { this->m_nID = INVALID_AIOBJECTID; }

	/**
	* Deassign this weak reference.
	* This will not affect the object itself or any other references to it.
	*/
	void Reset( void ) { this->m_nID = INVALID_AIOBJECTID; }

	/**
	* Deassign this weak reference.
	* This will not affect the object itself or any other references to it.
	*/
	bool IsReset( void ) const { return( this->m_nID == INVALID_AIOBJECTID ); }

	/**
	* If this reference is invalid, reset it to Nil.
	* Returns whether it was valid or not.
	* Often useful to resolve an argument, ensuring it is Valid or Nil before storing it.
	*/
	bool ValidateOrReset( void ) 
	{ 
		if (IsValid())
			return true;
		Reset(); 
		return false;
	}	

	/**
	* Acts like GetAIObject(), but if invalid Reset to Nil.
	* Often useful to check update status of stored references while checking their validity.
	*/
	T * GetAIObjectOrReset(void)
	{ 
		T *pObject = this->GetAIObject();
		if (!pObject)
			Reset;
		return pObject;
	}	

	T * GetAIObject(void) const
	{ 
		T *pObject = static_cast<T*>(this->GetIAIObject());
		return pObject;
	}	

	bool IsValid( void ) const { return gAIEnv.pObjectContainer->IsValid(*this); }

	/**
	* Assign a weak weak reference from any typed reference.
	*/
	void Assign( const CAbstractRef<T> &ref ) { this->m_nID = ref.GetObjectID(); }

	void Serialize( TSerialize ser, const char * sName = NULL )
	{		
		ser.Value( (sName ? sName : "weakRef") , this->m_nID );  // Will the compiler coalesce all these? Or, would #T be useful?
	}

protected:
	// Allow just CObjectManager to create in this way and we trust it to have ensured the type is appropriate
	// This can probably go away eventually
	// Note that when converting over from pointers, use of 0 as an argument can cause complaints of protected member here
	CWeakRef<T>( tAIObjectID nID ) { this->m_nID = nID; }
};

/**
 * Get a weak reference to the given object, of the same type as the object pointer.
 * Convenience method, currently with incredibly slow implementation.
 */
template <class T> CWeakRef<T> GetWeakRef( T * pObject )
{
	if (!pObject)
		return NILREF;
	return StaticCast<T>( pObject->GetSelfReference() );
}

template <class T> void Serialisationworkaround( TSerialize ser, const char * sName, T ** pObj)
{
	CWeakRef<T> ref;
	if (ser.IsReading())
	{
		ref.Serialize(ser, sName);
		*pObj = ref.GetAIObject();
	}
	if (ser.IsWriting())
	{
		if (*pObj) 
			ref = GetWeakRef(*pObj);
		ref.Serialize(ser, sName);
	}
}



#ifdef DEBUG_REFERENCES
	#define SET_DEBUG_OBJ( x ) pObj = x
#else
	#define SET_DEBUG_OBJ( x )
#endif

/**
* Typed counted reference for defining ownership in objects while being STL-compatible.
* Can only be initialised from strong references or other counted references.
* They are by definition always valid until Released.
* Strong references can only be assigned centrally by the system when the object is created and cannot be passed around.
*/

// Implementation notes: 
// Example reference counting classes often must be assigned and assume a valid counter object. Here we can be unassigned, like Strong.
// This _isn't_ an abstract ref! It shouldn't directly contain an ID. A bit of restructuring might be required here.
template <class T = CAIObject> class CCountedRef
{
public:

#ifdef DEBUG_REFERENCES
	CAIObject *pObj;
#endif
	
	/**
	* Construct an unassigned (nil) reference.
	*/
	CCountedRef(void) { this->m_pCounter = NULL; SET_DEBUG_OBJ(NULL); }

	/**
	* Construction from a strong ref (which is loses its single-transferable ownership)
	*/
	CCountedRef(CStrongRef<T> &ref) 
	{
		this->m_pCounter = NULL;
		// Remember there's no point counting Nil references. We also don't check validity, just like Strong.
		if (!ref.IsNil())
		{
			ObtainCounter();    // Get a reference counting object (starts at 1)
			this->m_pCounter->m_strongRef = ref;   // Acquire the ownership - note we don't need to be friends
			SET_DEBUG_OBJ(ref.GetAIObject());
		}
	}

	/**
	* Copy constructor.
	*/
	CCountedRef(const CCountedRef<T> &ref) 
	{
		this->m_pCounter = NULL;
		// If the other instance is empty, we don't need to do anything.
		if (ref.m_pCounter)
		{
			this->m_pCounter = ref.m_pCounter; // Share the counter
			++(this->m_pCounter->m_nRefs);     // Increment the count
			SET_DEBUG_OBJ(ref.GetAIObject());
		}
	}

	/**
	* Convert a NilRef into an unassigned counted reference.
	* Technically this is a copy-constructor but it's really just syntactic sugar
	*/
	CCountedRef(type_nil_ref) { this->m_pCounter = NULL; }

	/**
	* Counted reference assignment.
	*/
	CCountedRef &operator= ( const CCountedRef &ref ) 
	{
		// Do nothing if assigned to same counter
		if (m_pCounter == ref.m_pCounter)
			return *this;

		// Release one count on any object currently owned and possibly release object itself
		Release();

		if (ref.m_pCounter != NULL)
		{
			this->m_pCounter = ref.m_pCounter;
			++(this->m_pCounter->m_nRefs);
			SET_DEBUG_OBJ(ref.GetAIObject());
		}

		return *this;
	}

	/**
	* Destructor, of course, decrements the reference count and possibly deregisters the object.
	*/
	~CCountedRef(void) 
	{
		Release();
	}


	bool IsNil(void) const { return (this->m_pCounter == NULL); }

	/**
	* Decrement the count on any object owned, possibly causing the object to be deregistered.
	* Same name convention as Strong ref as it can have the same consequences.
	* @return true iff this reference was counting an object.
	*/
	bool Release(void) 
	{ 
		if (!this->m_pCounter)
			return false;

		--(this->m_pCounter->m_nRefs);
		if (this->m_pCounter->m_nRefs == 0)
			ReleaseCounter();
		return true;
	}

	operator bool() const { return this->m_pCounter != NULL; }

	void Serialize( TSerialize ser, const char * sName = NULL )
	{
		// (MATT) Perhaps this could be tidied up {2009/04/02}

		// Here, for now, I have to workaround it and just handle the case of a count of 1 when serialisation occurs.
		// Given that the multiple references are really only needed for STL compatability, that's safe for now.
		// (MATT) No it's not, when counted refs are being copied _during_ serisation to help implement the process {2009/04/01}
		//assert(!this->m_pCounter || this->m_pCounter->m_nRefs <= 1);

		// I think that when reading, an initial count greater than one is a very bad thing
		if (ser.IsReading())
			assert(!this->m_pCounter || this->m_pCounter->m_nRefs == 1);

		// Easiest efficient way to do this: a local copy of our Strong ref
		// We temporarily transfer any ownership to this strong ref
		CStrongRef<T> strongRef;
		if (this->m_pCounter)
		{
			strongRef = this->m_pCounter->m_strongRef; // Note this takes ownership
		}

		// Serialise that, which of course will write it out or read it in, which will replace ownership if need be
		strongRef.Serialize( ser, (sName ? sName : "countedRef") );

		// Now we must translate back into a counter object, if any is needed
		// This is true whether we wrote out, and need our counter working again to continue play,
		// or whether we read in and so need to make a counter object to start working with the new value.

		// If we need a counter now and we don't already have one, create one
		if (strongRef.IsSet() && !this->m_pCounter)
			ObtainCounter();

		if (this->m_pCounter)
		{
			// If we had a counter but don't need it anymore, because the object we owned has been replaced by Nil, we must delete the counter
			if (strongRef.IsNil())
				ForceReleaseCounter();
			else
				this->m_pCounter->m_strongRef = strongRef; // Note this passes any ownership back
		}

		// We shouldn't have local ownership when we finish
		assert(strongRef.IsNil());
	}

	bool operator==(const IAIObject * const pThatObject) const
	{
		// No reference is equivalent to Nil is equivalent to a NULL pointer
		if (!m_pCounter)
			return (pThatObject == NULL);
		
		return m_pCounter->m_strongRef == pThatObject;
	}

	bool operator==(const CAbstractUntypedRef &that) const
	{
		// No reference is equivalent to Nil
		if (!m_pCounter)
			return (that.IsNil());

		return m_pCounter->m_strongRef == that;
	}

	/**
	* Get a typed weak reference instance from any existing reference.
	* Weak references can be created from both strong and weak references.
	* @return A weak reference to the same object, if any.
	*/	
	CWeakRef<T> GetWeakRef(void) const {	return ( m_pCounter ? CWeakRef<T>( m_pCounter->m_strongRef.GetObjectID() ) : NILREF ); }

	T * GetAIObject(void) const
	{
		return ( m_pCounter ? m_pCounter->m_strongRef.GetAIObject() : NULL );
	}


	// Could define deref here - semantics are same as pointer for strong
	// Conversely, removing this is helpful in finding dodgy places in the code!
	T * operator->() const { return this->m_pCounter->m_strongRef.GetAIObject(); }

	/**
	* Return a simple AI object ID.
	* @return the tAIObjectID wrapped, regardless of validity. INVALID_AIOBJECTID if unassigned;
	*/	
	tAIObjectID GetObjectID(void) const
	{
		return ( m_pCounter ? m_pCounter->m_strongRef.GetObjectID() : INVALID_AIOBJECTID );
	}

protected:
	// In principle, we could cache the m_nID or CAIObject pointer here - but whether it's worth the bloat is debatable, especially with non-trivial stub

	struct SRefCounter
	{
		SRefCounter() : m_nRefs(1) {} // Start at 1
		~SRefCounter() {}             // Strong ref released automatically
		CStrongRef<T> m_strongRef;
		int m_nRefs;                  // Since this isn't in the object itself, it needn't be mutable
	};

	SRefCounter * m_pCounter;

	// For now, we just use new and delete, but a pool seems sensible
	void ObtainCounter()
	{
		assert(!m_pCounter);
		m_pCounter = new SRefCounter();
	}

	void ReleaseCounter()
	{
		assert(m_pCounter && m_pCounter->m_nRefs == 0);
		SAFE_DELETE(m_pCounter);
		SET_DEBUG_OBJ(NULL);
	}

	// (MATT) Force a release to a strong ref. Only makes sense when count is 1 - or we will leave dangling pointers to a deleted counter {2009/03/30}
	void ForceReleaseCounter()
	{
		assert(m_pCounter && m_pCounter->m_nRefs == 1);
		SAFE_DELETE(m_pCounter);
		SET_DEBUG_OBJ(NULL);
	}
};

#endif // __AI_REFERENCE_H_