/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2008.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Base perception handling for puppets
  
 -------------------------------------------------------------------------
  History:
  - 04:30:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __BASEPERCEPTIONHANDLER_H__
#define __BASEPERCEPTIONHANDLER_H__

#include "IPerceptionHandler.h"

class CBasePerceptionHandler : public IPerceptionHandler
{
public:
	CBasePerceptionHandler();
	virtual ~CBasePerceptionHandler();

	enum { TypeId = ePHT_Dummy };
	virtual int GetType() const { return TypeId; }

	virtual void SetOwner(CWeakRef<CPuppet> refOwner) { m_refOwner = refOwner; }
	virtual bool Serialize(TSerialize ser, CObjectTracker& objectTracker) { return false; }

	// Event management
	virtual bool IsEventManagementEnabled() const;
	virtual SAIPotentialTarget* AddEvent(CWeakRef<CAIObject> refObject, SAIPotentialTarget& targetInfo);
	virtual bool RemoveEvent(CWeakRef<CAIObject> refObject);
	virtual CAIObject* GetEventOwner(CWeakRef<CAIObject> refOwner) const;

	// Target management
	virtual void ClearPotentialTargets();
	virtual bool GetPotentialTargets(PotentialTargetMap &targetMap) const;
	virtual void UpTargetPriority(CWeakRef<CAIObject> refTarget, float fPriorityIncrement);
	virtual bool AddAggressiveTarget(CWeakRef<CAIObject> refTarget) { return false; }
	virtual bool DropTarget(CWeakRef<CAIObject> refTarget) { return false; }
	virtual bool SetTempTargetPriority(ETempTargetPriority priority) { return false; }
	virtual bool UpdateTempTarget(const Vec3& vPosition) { return false; }
	virtual bool ClearTempTarget() { return false; }

	// Get best target
	virtual bool GetBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased);

	// Event handling
	virtual void HandleSoundEvent(SAIEVENT* pEvent);
	virtual void HandleVisualStimulus(SAIEVENT* pEvent);
	virtual void HandleBulletRain(SAIEVENT* pEvent);

protected:
	CWeakRef<CPuppet> m_refOwner;
	PotentialTargetMap m_TargetMap;
};

#endif //__BASEPERCEPTIONHANDLER_H__
