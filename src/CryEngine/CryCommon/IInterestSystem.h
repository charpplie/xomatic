////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   IInterestSystem.h
//  Version:     v1.00
//  Created:     08/03/2007 by Matthew Jack
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include DEVIRTUALIZE_HEADER_FIX(IInterestSystem.h)

#ifndef __IInterestSystem_h__
#define __IInterestSystem_h__
#pragma once

#include "IEntity.h"

// Forward declarations
struct SEntityInterest;
struct SActorInterestSettings;
class CAIActor;

// AI event listener
struct IInterestListener
{
	virtual void OnInterested(EntityId idInterestedActor, EntityId idInterestingEntity) = 0;
	virtual void OnLoseInterest(EntityId idInterestedActor, EntityId idInterestingEntity) = 0;
};

// Settings for ai actors
struct SActorInterestSettings
{

	SActorInterestSettings(bool _bEnablePIM, float _fInterestFilter, float _fAngle)
	: bEnablePIM(_bEnablePIM), fInterestFilter(_fInterestFilter)
	{
		SetAngle(_fAngle);
	}

	SActorInterestSettings(void) { Reset(); }									// Set to defaults

	void Reset() {
		bEnablePIM = true;
		fInterestFilter = 0.0f;
		SetAngle(270.0f);
	}

	void SetAngle(float fDegrees)
	{
		if (fDegrees < 0.0f)
			fDegrees = 0.0f;
		else if(fDegrees > 360.0f)
			fDegrees = 360.0f;

		fAngle = cos(DEG2RAD(fDegrees*0.5f));
	}

	float fInterestFilter;	// What minimum interest must something have for this actor?
	float fAngle;						// If the angle between our movement direction and the interest Object is beyond this range, it will be ignored
	bool bEnablePIM;				// Can this actor use the interest system at all?
};

//---------------------------------------------------------------------//

UNIQUE_IFACE class ICentralInterestManager
{
public:
	// Reset the CIM system
	// Do this when all caches should be emptied, e.g. on loading levels
	// Resetting during game should do no serious harm
	virtual void Reset() = 0;

	// Enable or disable (suppress) the interest system
	virtual bool Enable(bool bEnable) = 0;

	// Is the Interest System enabled?
	virtual bool IsEnabled(void) = 0;

	// Is the Debug info enabled?
	virtual bool IsDebuggingEnabled(void) = 0;

	// Update the CIM
	virtual void Update( float fDelta ) = 0;

	// Register/update an interesting entity
	// Returns true if this was accepted as a useful interest target
	// If the object is already registered, it will be updated and still return true
	virtual bool RegisterInterestingEntity( IEntity *pEntity, float fRadius, float fBaseInterest, const char* actionName = 0 ) = 0;

	// Deregister an interesting entity
	virtual void DeregisterInterestingEntity( IEntity *pEntity ) = 0;

	// Register a potential interested AIActor
	// Returns true if this was accepted as a potentially useful interested entity
	// If the object is already registered, it will be updated and still return true
	virtual bool RegisterInterestedAIActor( IEntity *pEntity, bool bEnablePIM, float fInterestFilter, float fAngle ) = 0;

	// Deregister a potential interested AIActor
	virtual bool DeregisterInterestedAIActor( IEntity *pEntity ) = 0;

	// Add a class to be notified when an entity is interested or interesting
	virtual void RegisterListener( IInterestListener* pClass, EntityId idInterestingEntity ) = 0;
	virtual void UnRegisterListener( IInterestListener* pClass, EntityId idInterestingEntity ) = 0;
};

//---------------------------------------------------------------------//

UNIQUE_IFACE class IPersonalInterestManager
{
public:
	// Clear tracking cache, don't clear assignment
	virtual void Reset(void) = 0;

	// Re(assign) to an actor (helps in object re-use)
	// NULL is acceptable, to leave unassigned
	// You must also ensure the PIM pointer in the CAIActor is set to this object
	virtual void Assign( CAIActor* pAIActor ) = 0;

	// Get the currently assigned AIActor
	virtual CAIActor *GetAssigned(void) const = 0;

	// Update
	virtual bool Update( bool bCloseToCam ) = 0;

	// Do we have any interest target right now?
	// Allows us to move on as quickly as possible for the common (uninterested) case
	virtual bool IsInterested(void) const = 0;

	// Which entity are we currently interested in?
	// Returns NULL if not currently interested in anything, or that target is not an entity
	virtual IEntity * GetInterestEntity(void) const = 0;

	virtual Vec3 GetInterestingPos() const = 0;
};

#endif //__IInterestSystem_h__