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

#ifndef __CRYSISPERCEPTIONHANDLER_H__
#define __CRYSISPERCEPTIONHANDLER_H__

#include "BasePerceptionHandler.h"

class CCrysisPerceptionHandler : public CBasePerceptionHandler
{
public:
	CCrysisPerceptionHandler();
	virtual ~CCrysisPerceptionHandler();

	enum { TypeId = ePHT_Crysis };
	virtual int GetType() const { return TypeId; }

	virtual bool Serialize(TSerialize ser, CObjectTracker& objectTracker);
	virtual void PostSerialize(bool bReading);

	// Get best target
	virtual bool GetBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased);

	// Event handling
	virtual void HandleSoundEvent(SAIEVENT* pEvent);
	virtual void HandleVisualStimulus(SAIEVENT* pEvent);

private:
	// Get the priority multiplier based on species
	virtual float GetPriorityMultiplier(CAIObject* pTarget, const AgentParameters& parameters) const;

	void AffectSOM(CPuppet* pOwner, CAIObject*	pTarget, float exposure, float threat);

	typedef std::pair<CWeakRef<CAIObject>, SAIPotentialTarget> TPendingEvent;
	typedef std::vector<TPendingEvent> TPendingEvents;
	TPendingEvents m_pendingEvents;
};

#endif //__CRYSISPERCEPTIONHANDLER_H__
