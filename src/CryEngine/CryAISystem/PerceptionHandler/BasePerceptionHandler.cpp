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

#include "StdAfx.h"
#include "BasePerceptionHandler.h"
#include "Puppet.h"

REGISTER_PERCEPTION_HANDLER(CBasePerceptionHandler)

//////////////////////////////////////////////////////////////////////////
CBasePerceptionHandler::CBasePerceptionHandler()
{

}

//////////////////////////////////////////////////////////////////////////
CBasePerceptionHandler::~CBasePerceptionHandler()
{

}

//////////////////////////////////////////////////////////////////////////
bool CBasePerceptionHandler::IsEventManagementEnabled() const
{
	return (gAIEnv.CVars.TargetTracking == 1);
}

//////////////////////////////////////////////////////////////////////////
SAIPotentialTarget* CBasePerceptionHandler::AddEvent(CWeakRef<CAIObject> refObject, SAIPotentialTarget& targetInfo)
{
	SAIPotentialTarget *pResult = NULL;

	if (IsEventManagementEnabled())
	{
		bool bFailed = false;
		CAIObject *pObject = refObject.GetAIObject();
		AIAssert(pObject);
		if (pObject == NULL)
		{
			bFailed = true;
		}

		if (!bFailed && gAIEnv.CVars.IgnorePlayer)
		{
			if (pObject->GetType() == AIOBJECT_PLAYER)
			{
				bFailed = true;
			}
		}
		if (!bFailed)
		{
			std::pair<PotentialTargetMap::iterator, bool> res = m_TargetMap.insert(PotentialTargetMap::iterator::value_type(refObject, targetInfo));
			if (res.second)
			{
				if (targetInfo.refDummyRepresentation)
				{
					CAIGroup* pGroup = GetAISystem()->GetAIGroup(m_refOwner.GetAIObject()->GetGroupId());
					if (pGroup)
						pGroup->OnUnitAttentionTargetChanged();
				}

				pResult = &(res.first->second);
			}
			else
			{
				bFailed = true;
			}
		}

		// Clean up
		if (bFailed)
		{
			targetInfo.refDummyRepresentation.Release();
		}
	}

	return pResult;
}

//////////////////////////////////////////////////////////////////////////
bool CBasePerceptionHandler::RemoveEvent(CWeakRef<CAIObject> refObject)
{
	bool bResult = !IsEventManagementEnabled();

	if (!bResult)
	{
		PotentialTargetMap::iterator itTarget = m_TargetMap.find(refObject);

		if (itTarget != m_TargetMap.end())
		{
			m_TargetMap.erase(itTarget);
			bResult = true;
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
CAIObject* CBasePerceptionHandler::GetEventOwner(CWeakRef<CAIObject> refOwner) const
{
	CAIObject *pResult = NULL;

	if (IsEventManagementEnabled())
	{
		CAIObject *pOwner = refOwner.GetAIObject();

		PotentialTargetMap::const_iterator itTarget = m_TargetMap.begin();
		PotentialTargetMap::const_iterator itTargetEnd = m_TargetMap.end();
		for (; itTarget != itTargetEnd; ++itTarget)
		{
			const SAIPotentialTarget& ed = itTarget->second;
			if (ed.refDummyRepresentation == pOwner || itTarget->first == pOwner)
			{
				pResult = itTarget->first.GetAIObject();
				break;
			}
		}
	}

	return pResult;
}

//////////////////////////////////////////////////////////////////////////
void CBasePerceptionHandler::ClearPotentialTargets()
{
	// The reason for this slightly weird deletion is that the RemoveDummyObject() will
	// call the OnObjectRemoved of this puppet which in turn tries to erase the event.
	while (!m_TargetMap.empty())
	{
		m_TargetMap.erase(m_TargetMap.begin());
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBasePerceptionHandler::GetPotentialTargets(PotentialTargetMap &targetMap) const
{
	// TODO Investigate returning a const ref, some (Warface) require inserting into the map
	targetMap = m_TargetMap;
	return (!m_TargetMap.empty());
}

//////////////////////////////////////////////////////////////////////////
void CBasePerceptionHandler::UpTargetPriority(CWeakRef<CAIObject> refTarget, float fPriorityIncrement)
{
	PotentialTargetMap::iterator itTarget = m_TargetMap.find(refTarget);
	// see if target is available
	if (itTarget == m_TargetMap.end())
		return;

	SAIPotentialTarget& ed = itTarget->second;

	// only existing targets can be upped
	// otherwise can get AttTarget set without OnEnemySeen sent
	if (ed.type == AITARGET_MEMORY || ed.type == AITARGET_NONE)
		return;

	float bestPriority = 0.0f;
	CAIObject* bestTarget = 0;

	// we want to skip the top one
	PotentialTargetMap::iterator itTargetBegin = m_TargetMap.begin();
	PotentialTargetMap::iterator itTargetEnd = m_TargetMap.end();
	for (; itTargetBegin != itTargetEnd; ++itTargetBegin)
	{
		SAIPotentialTarget& edLooping = itTargetBegin->second;

		// target selection based on priority
		if (edLooping.priority > bestPriority)
		{
			bestPriority = edLooping.priority;
			bestTarget = itTargetBegin->first.GetAIObject();
		}
	}
	if (bestTarget == refTarget.GetAIObject())
		return;

	ed.upPriority += fPriorityIncrement;
	if (ed.upPriority > 1.0f)
		ed.upPriority = 1.0f;
	ed.upPriorityTime = 2.0f;	// TODO: parameterize.
}

//////////////////////////////////////////////////////////////////////////
bool CBasePerceptionHandler::GetBestTarget(CWeakRef<CAIObject> &refBestTarget, SAIPotentialTarget* &pTargetInfo, bool &bCurrentTargetErased)
{
	refBestTarget.Reset();
	pTargetInfo = NULL;
	bCurrentTargetErased = true; // We must make sure any target we had before isn't there anymore when this is used

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CBasePerceptionHandler::HandleSoundEvent(SAIEVENT* pEvent)
{
	CRY_ASSERT(pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CBasePerceptionHandler::HandleVisualStimulus(SAIEVENT* pEvent)
{
	CRY_ASSERT(pEvent);
}

//////////////////////////////////////////////////////////////////////////
void CBasePerceptionHandler::HandleBulletRain(SAIEVENT* pEvent)
{
	CRY_ASSERT(pEvent);
	
	CPuppet *pOwner = CastToCPuppetSafe(m_refOwner.GetAIObject());
	if (pOwner)
		pOwner->SetAlarmed();
}
