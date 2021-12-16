/*************************************************************************
  StalTech Source File.
  Copyright (C), StalTech Studios, 2001-2008.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Perception handling for puppets using Warface method
  
 -------------------------------------------------------------------------
  History:
  - 04:30:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __WARFACEPERCEPTIONHANDLER_H__
#define __WARFACEPERCEPTIONHANDLER_H__

#include "BasePerceptionHandler.h"

class CWarfacePerceptionExtraData;

class CWarfacePerceptionHandler : public CBasePerceptionHandler
{
public:
	CWarfacePerceptionHandler();
	virtual ~CWarfacePerceptionHandler();

	enum { TypeId = ePHT_Warface };
	virtual int GetType() const { return TypeId; }

	// Extra data
	virtual IPerceptionExtraData* GetExtraData() { return (IPerceptionExtraData*)m_pExtraData; }

	// Target management
	virtual bool RemoveEvent(CWeakRef<CAIObject> refObject);
	virtual void ClearPotentialTargets();
	virtual bool GetPotentialTargets(PotentialTargetMap &targetMap) const;
	virtual void UpTargetPriority(CWeakRef<CAIObject> refTarget, float fPriorityIncrement);
	virtual bool AddAggressiveTarget(CWeakRef<CAIObject> refTarget);
	virtual bool DropTarget(CWeakRef<CAIObject> refTarget);
	virtual bool SetTempTargetPriority(ETempTargetPriority priority);
	virtual bool UpdateTempTarget(const Vec3& vPosition);
	virtual bool ClearTempTarget();

	// Get best target
	virtual bool GetBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased);

	// Event handling
	virtual void HandleSoundEvent(SAIEVENT* pEvent);
	virtual void HandleVisualStimulus(SAIEVENT* pEvent);
	virtual void HandleBulletRain(SAIEVENT* pEvent);

protected:
	bool IsDebugging() const;
	void ClearSoundTarget();
	void ClearPotentialTarget(SAIPotentialTarget &target);

	bool GetTempTarget_Priority(ETempTargetPriority matchPriority, CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo);

	// Update target lifetime to look for stationary events
	void ResetTargetLifetime(EntityId targetOwnerId, SAIPotentialTarget &target);
	void UpdateTargetLifetime(EntityId targetOwnerId, SAIPotentialTarget &target);
	void NotifyOwnerStationaryTarget(bool bStationary, EntityId targetOwnerId, const SAIPotentialTarget &target) const;
	void NotifyOwnerLongMemoryTarget(bool bLongMemory, EntityId targetOwnerId, const SAIPotentialTarget *pTarget) const;

	// Returns if currTarget is better than our bestTarget for target selection
	bool CompareBestTarget(SAIPotentialTarget *pBestInfo, SAIPotentialTarget *pCurrInfo) const;

	// Handles a heard sound
	void HandleHeardSoundEvent(SAIEVENT *pEvent, float fSoundDistance, EAITargetThreat &iSoundThreat, bool bForceToAggressive = false);
	void UpdateSoundOnlyTarget(CWeakRef<CAIObject> refEventOwner, const Vec3& vPos, EAITargetThreat iSoundThreat);

	// Handles a seen visual event
	void HandleSeenVisualStimulus(CWeakRef<CAIObject> refEventOwner, float fSightRange, float fDistToTarget, bool bForceToAggressive = false);
	void GetSightRange(CWeakRef<CAIObject> refEventOwner, float &fSightRange, float &fSightRangeMax, float &fDistToTarget) const;
	void GetVisualThreatLevel(SAIPotentialTarget *pEvent, CWeakRef<CAIObject> refEventOwner, EAITargetThreat iBaseLevel, float &fRatioToNextThreat, float fDistToTarget = FLT_MAX);

	// Calculate the visual threat level of a target
	float CalculateVisualThreat(float fSightRange, float fDistToTarget) const;

	CWarfacePerceptionExtraData *m_pExtraData;
	bool m_bLastLongMemory;

	// Temporary target
	ETempTargetPriority m_iTempTargetPriority;
	SAIPotentialTarget m_TempTarget;

	// Our one and only sound-only target
	EntityId m_SoundOnlyOwnerId;
	SAIPotentialTarget m_SoundOnlyTarget;
};

#endif //__WARFACEPERCEPTIONHANDLER_H__
