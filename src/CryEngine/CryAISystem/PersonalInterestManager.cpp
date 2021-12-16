/********************************************************************
StalTech Source File.
Copyright  , StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   PersonalInterestManager.cpp
$Id$
$DateTime$
Description: Interest Manager (tracker) for interested individuals

Notes:       Consider converting interest values float->int 
-------------------------------------------------------------------------
History:
- 2 Mar 2009	: Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext
- 15 Feb 2010 : Heavily refactored by Ricardo Pillosu
*********************************************************************/

#include "StdAfx.h"

#include "PersonalInterestManager.h"
#include "CentralInterestManager.h"
#include "CAISystem.h"
#include "Puppet.h"
#include "AIActions.h"
#include "VisionMap.h"
// For persistent debugging
#include "IGameFramework.h"

CPersonalInterestManager::CPersonalInterestManager(CAIActor* pAIActor)
{
	m_pCIM = NULL;
	m_Settings = SActorInterestSettings();
	Reset();
	if (pAIActor)
	{
		Assign(pAIActor);
	}
}

//------------------------------------------------------------------------------------------------------------------------

CPersonalInterestManager::~CPersonalInterestManager(void)
{
	m_refInterestDummy.Release();
}

//------------------------------------------------------------------------------------------------------------------------

CPersonalInterestManager::CPersonalInterestManager(const CPersonalInterestManager& pim)
{
	m_pCIM = CCentralInterestManager::GetInstance();
	m_refAIActor = pim.m_refAIActor;
	GetAISystem()->CreateDummyObject(m_refInterestDummy, "InterestDummy");
	m_refInterestDummy->SetPos(pim.GetInterestingPos());
	m_IdInterestingEntity = pim.m_IdInterestingEntity;
	m_Settings = pim.m_Settings;
}

//------------------------------------------------------------------------------------------------------------------------

CPersonalInterestManager &CPersonalInterestManager::operator =(const CPersonalInterestManager &pim)
{
	m_pCIM = CCentralInterestManager::GetInstance();
	m_refAIActor = pim.m_refAIActor;
	GetAISystem()->CreateDummyObject(m_refInterestDummy, "InterestDummy");
	m_refInterestDummy->SetPos(pim.GetInterestingPos());
	m_IdInterestingEntity = pim.m_IdInterestingEntity;
	m_Settings = pim.m_Settings;

	return(*this);
}

//------------------------------------------------------------------------------------------------------------------------

// Clear tracking cache, don't clear assignment
void CPersonalInterestManager::Reset(void)
{
	m_refAIActor.Reset();
	m_refInterestDummy.Release();
	m_IdInterestingEntity = 0;
	m_Settings.Reset();
}

//------------------------------------------------------------------------------------------------------------------------

void CPersonalInterestManager::Assign( CAIActor *pAIActor )
{
	Assign( GetWeakRef(pAIActor) );
}

//------------------------------------------------------------------------------------------------------------------------

// You must also ensure the PIM pointer in the CAIActor is set to this object
void CPersonalInterestManager::Assign( CWeakRef<CAIActor> refAIActor )
{
	// Check for redundant calls
	if (m_refAIActor == refAIActor) return;

	// Grab a cached pointer to the CIM. We won't need it before we're first assigned.
	m_pCIM = CCentralInterestManager::GetInstance();

	CAIActor* pOldAIActor = m_refAIActor.GetAIObject();
	CAIActor* pNewAIActor = refAIActor.GetAIObject();

	if (pOldAIActor) gAIEnv.pSmartObjectManager->RemoveSmartObjectState( pOldAIActor->GetEntity(), "RegisteredActor" );
	if (pNewAIActor) gAIEnv.pSmartObjectManager->AddSmartObjectState( pNewAIActor->GetEntity(), "RegisteredActor" );

	Reset();

	// Assign
	m_refAIActor = refAIActor;

	// Create Observer

	if (m_refAIActor.IsNil() == false && m_refInterestDummy.IsNil() == true)
	{
		GetAISystem()->CreateDummyObject(m_refInterestDummy, "InterestDummy");
	}

	// Debug draw
	if (m_pCIM->IsDebuggingEnabled())
	{
		if (pOldAIActor) m_pCIM->AddDebugTag(pOldAIActor->GetEntityID(), "No longer interested");
		if (pNewAIActor) m_pCIM->AddDebugTag(pNewAIActor->GetEntityID(), "Interested");
	}
}

//------------------------------------------------------------------------------------------------------------------------

bool CPersonalInterestManager::Update( bool bCloseToCam )
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool bRet = false;
	SEntityInterest const * pMostInteresting = NULL;
	IEntity* pUser = m_refAIActor.GetAIObject()->GetEntity();

	if( bCloseToCam == true )
		pMostInteresting = PickMostInteresting();

	if (pMostInteresting != NULL)
	{
		bRet = true;
		EntityId IdMostInteresting = pMostInteresting->GetEntity()->GetId();
		if (m_IdInterestingEntity != IdMostInteresting)
		{
			if(m_IdInterestingEntity > 0)
				m_pCIM->OnEntityInteresting(pUser->GetId(), m_IdInterestingEntity, false);
			m_pCIM->OnEntityInteresting(pUser->GetId(), IdMostInteresting, true);
			m_IdInterestingEntity = IdMostInteresting;

			if (!pMostInteresting->actionName.empty())
			{
				IEntity const * pObject = pMostInteresting->GetEntity();
				gAIEnv.pAIActionManager->ExecuteAIAction( pMostInteresting->actionName.c_str(), pUser, const_cast<IEntity*>(pObject), 101, 0 );
			}
		}
	}
	else if(m_IdInterestingEntity > 0)
	{
		m_pCIM->OnEntityInteresting(pUser->GetId(), m_IdInterestingEntity, false);
		m_IdInterestingEntity = 0;
		m_pCIM->AddDebugTag(GetAssigned()->GetEntityID(), "Nothing around");
		bRet = true;
	}

	return(bRet);
}

//------------------------------------------------------------------------------------------------------------------------

void CPersonalInterestManager::SetSettings(bool bEnablePIM, float fInterestFilter, float fAngle )
{
	m_Settings.bEnablePIM = bEnablePIM;

	if(fInterestFilter >= 0.0f)
	{
		m_Settings.fInterestFilter = fInterestFilter;
	}

	if(fAngle >= 0.0f)
	{
		m_Settings.SetAngle(fAngle);
	}

	if(m_pCIM->IsDebuggingEnabled())
	{
		CryFixedStringT<32> sText;
		sText.Format("%s: Filter %0.1f Angle %0.1f", (m_Settings.bEnablePIM ? "Enabled" : "Disabled"), m_Settings.fInterestFilter, 2.0f * (RAD2DEG(acos(m_Settings.fAngle))));
		m_pCIM->AddDebugTag(GetAssigned()->GetEntityID(), sText);
	}
}

//------------------------------------------------------------------------------------------------------------------------
SEntityInterest const * CPersonalInterestManager::PickMostInteresting(void) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	Vec3 vActorPos = m_refAIActor.GetAIObject()->GetPos();
	SEntityInterest const * pRet = NULL;
	float fMostInterest = m_Settings.fInterestFilter;

	CCentralInterestManager::TVecInteresting::const_iterator it = m_pCIM->GetInterestingEntities()->begin();
	CCentralInterestManager::TVecInteresting::const_iterator itEnd = m_pCIM->GetInterestingEntities()->end();
	for (;it != itEnd; ++it)
	{
		if((*it).entityId == 0 || (*it).fInterest < fMostInterest)
			continue;

		Vec3 vToTarget = (*it).GetEntity()->GetPos() - vActorPos;
		float fDistTo = vToTarget.GetLengthSquared();
		float fRadius = (*it).fRadius;

		if (fDistTo > (fRadius*fRadius))
			continue;

		//distance aspect - close objects have higher interest
		float fMaxDist = fRadius;
		float fDist = clamp(cry_sqrtf(fDistTo), 0.1f, fMaxDist) ;
		float fDistanceAspect = (fMaxDist - fDist) / fMaxDist;

		// Basic accumulation of interest
		float fAccumulated = (*it).fInterest * fDistanceAspect;

		// Is this the most interesting so far?
		if (fMostInterest < fAccumulated)
		{
			// Check viewing angle
			float fDot = vToTarget.normalized().Dot(m_refAIActor.GetAIObject()->GetMoveDir());
			if (fDot > m_Settings.fAngle) 
			{
				fMostInterest = fAccumulated;
				pRet = &(*it);
			}
		}
	}

	return(pRet);
}

//------------------------------------------------------------------------------------------------------------------------

IEntity * CPersonalInterestManager::GetInterestEntity(void) const
{
	if(m_IdInterestingEntity == 0) return NULL;

	return gEnv->pEntitySystem->GetEntity(m_IdInterestingEntity);
}

//------------------------------------------------------------------------------------------------------------------------
bool CPersonalInterestManager::IsPointInsideFOVFromMoveDirection( const Vec3 &vPoint)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	CAIActor *pAIActor = m_refAIActor.GetAIObject();
	// Check range
	Vec3 vActorPos = pAIActor->GetPos();
	Vec3 vToTarget = vPoint - vActorPos;

	// Check viewing angle
	float fDot = vToTarget.normalized().Dot(pAIActor->GetMoveDir());
	if (fDot < m_Settings.fAngle) return false;

	return true;
}

//------------------------------------------------------------------------------------------------------------------------
Vec3 CPersonalInterestManager::GetInterestingPos() const
{
	// It is often appropriate to test against visible range before calling this (depending on interest type)
	CAIActor *pAIActor = m_refAIActor.GetAIObject();
	IEntity *pEntity = GetInterestEntity();

	if(pAIActor != NULL && pEntity != NULL)
	{
		return(pEntity->GetPos());
	}
	return(ZERO);
}

//------------------------------------------------------------------------------------------------------------------------

CWeakRef<CAIObject> CPersonalInterestManager::GetInterestDummyPoint(void)
{
	if (IsInterested() == false)
	{
		return NILREF;
	}

	CAIObject *pInterestDummy = m_refInterestDummy.GetAIObject();
	if(pInterestDummy)
		pInterestDummy->SetPos( GetInterestingPos() );
	return m_refInterestDummy;
}

//------------------------------------------------------------------------------------------------------------------------
ELookStyle CPersonalInterestManager::GetLookingStyle() const
{
	ELookStyle eStyle = LOOKSTYLE_HARD;

	/* When this works we can really use a better selection of those values:
	LOOKSTYLE_HARD,
	LOOKSTYLE_HARD_NOLOWER,
	LOOKSTYLE_SOFT,
	LOOKSTYLE_SOFT_NOLOWER,
	*/

	return(eStyle);
}

//-----------------------------------------------------------------------------------------------------

#include UNIQUE_VIRTUAL_WRAPPER(IPersonalInterestManager)
