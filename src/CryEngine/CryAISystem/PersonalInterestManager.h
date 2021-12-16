/********************************************************************
StalTech Source File.
Copyright  , StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   PersonalInterestManager.h
$Id$
$DateTime$
Description: Interest Manager (tracker) for interested individuals

Note:        PIMs are owned by the CentralInterestManager - delete there
---------------------------------------------------------------------
History:
- 07:03:2007 : Created by Matthew Jack
- 15:02:2010 : Heavily refactored by Ricardo Pillosu

*********************************************************************/

#ifndef __PersonalInterestManager_H__
#define __PersonalInterestManager_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IInterestSystem.h"

class CCentralInterestManager;

class CPersonalInterestManager : public IPersonalInterestManager
{
public:
	CPersonalInterestManager(CAIActor* pAIActor = NULL);
	~CPersonalInterestManager(void);

	// Copy constructors
	CPersonalInterestManager(const CPersonalInterestManager& pim);
	CPersonalInterestManager &operator =(const CPersonalInterestManager &pim);

	// Clear tracking cache, don't clear assignment
	void Reset(void);

	// Clear tracking cache, don't clear assignment
	bool IsReset(void) const { return(m_refAIActor.IsReset()); }

	// Required to implement external interface - might use adapter
	void Assign( CAIActor* pAIActor );

	// Re(assign) to an actor (helps in object re-use)
	// NILREF is acceptable, to leave unassigned
	// You must also ensure the PIM pointer in the CAIActor is set to this object
	void Assign( CWeakRef<CAIActor> refAIActor );

	// Get the currently assigned AIActor
	CAIActor *GetAssigned(void) const { return m_refAIActor.GetAIObject(); }

	// Update
	bool Update( bool bCloseToCam );

	// Update interest settings
	// Checks with the CIM for any new settings associated with this actor
	void SetSettings(bool bEnablePIM, float fInterestFilter = -1.0f, float fAngle = -1.0f );

	// Do we have any interest target right now?
	// Allows us to move on as quickly as possible for the common (uninterested) case
	bool IsInterested(void) const { return m_IdInterestingEntity > 0; }

	// Which entity are we currently interested in?
	// Returns NULL if not currently interested in anything, or that target is not an entity
	IEntity * GetInterestEntity(void) const;

	// Returns the dummy interest object if currently interested or NILREF
	// Works regardless of the type of the interesting object
	CWeakRef<CAIObject> GetInterestDummyPoint(void);

	Vec3 GetInterestingPos() const;

	ELookStyle GetLookingStyle() const;

protected:

	SEntityInterest const * PickMostInteresting(void) const;

	// Is this point in visible range of the actor (distance and FOV) 
	bool IsPointInVisibleRange( const Vec3 &vPoint, bool bIgnoreDist = false);			
	bool IsPointInsideFOVFromMoveDirection( const Vec3 &vPoint);

	CWeakRef<CAIActor>				m_refAIActor;								// The actor we are responsible for
	EntityId									m_IdInterestingEntity;			//
	CStrongRef<CAIObject>			m_refInterestDummy;					// Dummy that moves around with any interest target
	CCentralInterestManager		*m_pCIM;										// Cached pointer to CIM
	SActorInterestSettings		m_Settings;									// Interest settings for this actor
};


#endif // __PersonalInterestManager_H__