/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2008.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Perception handling for puppets using Crysis method
  
 -------------------------------------------------------------------------
  History:
  - 04:30:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __CRYSIS2PERCEPTIONHANDLER_H__
#define __CRYSIS2PERCEPTIONHANDLER_H__

#include "BasePerceptionHandler.h"

class CCrysis2PerceptionHandler : public CBasePerceptionHandler
{
public:
	CCrysis2PerceptionHandler();
	virtual ~CCrysis2PerceptionHandler();

	enum { TypeId = ePHT_Crysis2 };
	virtual int GetType() const { return TypeId; }

	virtual bool Serialize(TSerialize ser, CObjectTracker& objectTracker);
	virtual void PostSerialize(bool bReading);

	// Get best target
	virtual bool GetBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased);

	// Event handling
	virtual void HandleSoundEvent(SAIEVENT* pEvent);
	virtual void HandleVisualStimulus(SAIEVENT* pEvent);
	virtual void HandleBulletRain(SAIEVENT* pEvent);

private:
	// Get the priority multiplier based on species
	virtual float GetPriorityMultiplier(CAIObject* pTarget, const AgentParameters& parameters) const;

	void AffectSOM(CPuppet* pOwner, CAIObject*	pTarget, float exposure, float threat);
	void ResetPerceptionScale(CPuppet* puppet);

	typedef std::pair<CWeakRef<CAIObject>, SAIPotentialTarget> TPendingEvent;
	typedef std::vector<TPendingEvent> TPendingEvents;
	TPendingEvents m_pendingEvents;
};

#endif //__CRYSIS2PERCEPTIONHANDLER_H__
