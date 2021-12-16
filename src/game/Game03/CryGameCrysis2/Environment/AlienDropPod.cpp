/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: A pod dropped from a floating ship to transport an Alien
				into the combat zone
  
 -------------------------------------------------------------------------
  History:
  - 02:15:2009: Created by Kevin Kirst

*************************************************************************/

#include "StdAfx.h"
#include "AlienDropPod.h"
#include "../GameRules.h"

//////////////////////////////////////////////////////////////////////////
SAlienPodLaunchProperties::SAlienPodLaunchProperties()
: vDestination(ZERO)
, fMaxForce(0.0f)
, fBounceForce(1.0f)
, nBounceCount(0)
, idHost(0)
, idLinkedAI(0)
, bComeToRest(false)
{

}

//////////////////////////////////////////////////////////////////////////
CAlienDropPod::CAlienDropPod()
: m_fMaxForce(0.0f)
, m_fBounceForce(1.0f)
, m_nBounceLimit(1)
, m_nCurrentBounces(0)
, m_idHost(0)
, m_idLinkedAI(0)
, m_eState(eState_Invalid)
, m_bComeToRest(false)
{

}

//////////////////////////////////////////////////////////////////////////
CAlienDropPod::~CAlienDropPod()
{
	if (CGameRules *pGameRules = g_pGame->GetGameRules())
		pGameRules->RemoveHitListener(this);

	const EntityId myId = GetEntityId();
	TListeners::iterator itListener = m_Listeners.begin();
	TListeners::iterator itListenerEnd = m_Listeners.end();
	for (; itListener != itListenerEnd; ++itListener)
	{
		CAlienDropPodListener *pListener = *itListener;
		CRY_ASSERT(pListener);

		pListener->OnPodDestroyed(myId);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAlienDropPod::Init(IGameObject *pGameObject)
{
	SetGameObject(pGameObject);

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::Release()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::FullSerialize(TSerialize ser)
{
	ser.Value("m_fMaxForce", m_fMaxForce);
	ser.Value("m_fBounceForce", m_fBounceForce);
	ser.Value("m_nBounceLimit", m_nBounceLimit);
	ser.Value("m_nCurrentBounces", m_nCurrentBounces);
	ser.Value("m_idHost", m_idHost);
	ser.Value("m_idLinkedAI", m_idLinkedAI);
	ser.EnumValue("m_eState", m_eState, eState_Invalid, eState_COUNT);
	ser.Value("m_bComeToRest", m_bComeToRest);

	if (ser.IsReading())
	{
		SetHostEntity(m_idHost);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	// Monitor speed to detect when we stop rolling
	if (eState_Rolling == m_eState)
	{
		IEntity *pEntity = GetEntity();
		CRY_ASSERT(pEntity);

		pe_status_dynamics dyn;
		IPhysicalEntity *pPhysics = pEntity->GetPhysics();
		if (pPhysics && pPhysics->GetStatus(&dyn))
		{
			if (dyn.v.GetLengthSquared() <= 1.0f)
			{
				OnAtRestingPoint();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::HandleEvent(const SGameObjectEvent& event)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	if (event.event == eGFE_OnCollision && (eState_Launching == m_eState || eState_Bouncing == m_eState))
	{
		EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;
		
		const bool bFinalBounce = (++m_nCurrentBounces >= m_nBounceLimit);

		// Consider a bounce if we're not on our final bounce or if we need to come to a rest
		if (!bFinalBounce || m_bComeToRest)
		{
			OnBounce(pCollision);
		}

		if (bFinalBounce)
		{
			OnFinishedBouncing();
		}
		else
		{
			m_eState = eState_Bouncing;

			// Direction based on ricochet of hit
			Vec3 vMovementDir = pCollision->n;
			const Vec3 vCollisionDir = pCollision->vloc[0].GetNormalizedSafe();
			const float fDot = pCollision->n.Dot(vCollisionDir);
			if (fDot < 0.0f)
			{
				vMovementDir = -2.0f * fDot * pCollision->n + vCollisionDir;
				vMovementDir.NormalizeSafe();
			}

			const float fForce = 1.0f - ((float)m_nCurrentBounces / (float)m_nBounceLimit);
			ApplyMovement(vMovementDir, fForce);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::GetMemoryUsage(ICrySizer *pSizer) const
{
	pSizer->Add(*this);
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::OnServerExplosion(const ExplosionInfo& explosionInfo)
{
	if (eState_Resting == m_eState && explosionInfo.weaponId == GetEntityId())
	{
		OnSpawnAlien();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAlienDropPod::LaunchPod(const SAlienPodLaunchProperties& properties)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	bool bResult = false;

	IGameObject *pGameObject = GetGameObject();
	CRY_ASSERT(pGameObject);

	IEntity *pEntity = GetEntity();
	CRY_ASSERT(pEntity);

	const Vec3 vPos = pEntity->GetWorldPos();

	if (gEnv->bServer && eState_Invalid == m_eState)
	{
		SetHostEntity(properties.idHost);

		const Vec3 vDir = (properties.vDestination - vPos).GetNormalizedSafe();
		Matrix34 mWorldTM = Matrix33::CreateRotationVDir(vDir);
		mWorldTM.SetTranslation(vPos);
		pEntity->SetWorldTM(mWorldTM);

		pGameObject->DisableUpdateSlot(this, 0);
		pGameObject->EnablePhysicsEvent(true, eEPE_OnCollisionLogged);

		m_sTerritory = properties.sTerritory.empty() ? "<None>" : properties.sTerritory;
		m_sWave = properties.sWave.empty() ? "<None>" : properties.sWave;
		
		m_nBounceLimit = max(properties.nBounceCount, 0);
		m_nCurrentBounces = 0;
		m_idLinkedAI = properties.idLinkedAI;

		IEntity *pLinkedAIEntity = GetLinkedAIEntity();
		if (pLinkedAIEntity)
		{
			pLinkedAIEntity->Hide(true);
			pLinkedAIEntity->Activate(false);
		}

		// Process the initial movement
		m_fMaxForce = properties.fMaxForce;
		m_fBounceForce = properties.fBounceForce;
		m_bComeToRest = properties.bComeToRest;
		ApplyMovement(vDir, 1.0f);

		m_eState = eState_Launching;
		bResult = true;
	}

	TListeners::iterator itListener = m_Listeners.begin();
	TListeners::iterator itListenerEnd = m_Listeners.end();
	for (; itListener != itListenerEnd; ++itListener)
	{
		CAlienDropPodListener *pListener = *itListener;
		CRY_ASSERT(pListener);

		pListener->OnPodLaunched(bResult, vPos, properties);
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CAlienDropPod::GetTableFromProperties(const char* szTableName, SmartScriptTable &pOutTable) const
{
	bool bResult = false;

	IEntity *pEntity = GetEntity();
	CRY_ASSERT(pEntity);

	SmartScriptTable pPropertiesTable;
	IScriptTable* pScriptTable = pEntity->GetScriptTable();
	if (pScriptTable && pScriptTable->GetValue("Properties", pPropertiesTable))
	{
		bResult = pPropertiesTable->GetValue(szTableName, pOutTable);
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
IEntity* CAlienDropPod::GetLinkedAIEntity() const
{
	IEntity *pResult = NULL;

	if (m_idLinkedAI > 0)
	{
		IActor *pLinkedAI = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(m_idLinkedAI);
		if (pLinkedAI && !pLinkedAI->IsDead())
		{
			pResult = pLinkedAI->GetEntity();
		}
	}

	return pResult;
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::SetHostEntity(EntityId idHost)
{
	if (m_idHost > 0)
	{
		IgnoreEntityCollision(m_idHost, false);
	}

	m_idHost = idHost;

	if (m_idHost > 0)
	{
		IgnoreEntityCollision(m_idHost, true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::IgnoreEntityCollision(EntityId idEntity, bool bIgnore) const
{
	CRY_ASSERT(idEntity > 0);

	IEntity *pMyEntity = GetEntity();
	CRY_ASSERT(pMyEntity);

	IPhysicalEntity *pMyPhysics = pMyEntity->GetPhysics();
	if (pMyPhysics)
	{
		if (!bIgnore)
		{
			pe_action_update_constraint uc;
			uc.bRemove = 1;

			pMyPhysics->Action(&uc);
		}
		else
		{
			IEntity *pEntity = gEnv->pEntitySystem->GetEntity(idEntity);
			IPhysicalEntity *pPhysics = pEntity ? pEntity->GetPhysics() : NULL;
			if (pPhysics)
			{
				pe_action_add_constraint ac;
				ac.flags = constraint_inactive|constraint_ignore_buddy;
				ac.pBuddy = pPhysics;
				ac.pt[0].Set(0,0,0);

				pMyPhysics->Action(&ac);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::ApplyMovement(const Vec3& vDir, float fForceRatio) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	IEntity *pEntity = GetEntity();
	CRY_ASSERT(pEntity);

	IPhysicalEntity *pPhysics = pEntity->GetPhysics();
	if (pPhysics)
	{
		const float fForceMod = fForceRatio * (m_nCurrentBounces > 0 ? (float)__fsel(-m_fBounceForce, 1.0f, m_fBounceForce) : 1.0f);

		pe_action_set_velocity action;
		action.v = vDir * m_fMaxForce * fForceMod;
		pPhysics->Action(&action);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::CreateExplosion(const SmartScriptTable& pExplosionTable, const Vec3& vPos, const Vec3& vNormal) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CRY_ASSERT(pExplosionTable.GetPtr());

	IEntity *pEntity = GetEntity();
	CRY_ASSERT(pEntity);

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (pGameRules && pExplosionTable.GetPtr())
	{
		const char* szEffect = 0;
		float fEffectScale = 0.0f;
		float fDamage = 0.0f;
		float fPressure = 0.0f;
		float fMinRadius = 0.0f;
		float fRadius = 0.0f;
		float fMinPhysRadius = 0.0f;
		float fPhysRadius = 0.0f;
		float fHoleSize = 0.0f;

		pExplosionTable->GetValue("Effect", szEffect);
		pExplosionTable->GetValue("EffectScale", fEffectScale);
		pExplosionTable->GetValue("Damage", fDamage);
		pExplosionTable->GetValue("Pressure", fPressure);
		pExplosionTable->GetValue("MinRadius", fMinRadius);
		pExplosionTable->GetValue("Radius", fRadius);
		pExplosionTable->GetValue("MinPhysRadius", fMinPhysRadius);
		pExplosionTable->GetValue("PhysRadius", fPhysRadius);
		pExplosionTable->GetValue("HoleSize", fHoleSize);

		ExplosionInfo info(0, pEntity->GetId(), 0, fDamage, vPos, vNormal, fMinRadius, fRadius, 
			fMinPhysRadius, fPhysRadius, 0.0f, fPressure, fHoleSize, 0);
		info.type = pGameRules->GetHitTypeId("alienDropPodBounce");
		info.SetEffect(szEffect, fEffectScale, 0.0f);

		pGameRules->QueueExplosion(info);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::OnBounce(const EventPhysCollision *pCollision)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	SmartScriptTable pExplosionTable;
	if (GetTableFromProperties("BounceExplosion", pExplosionTable))
	{
		bool bEnabled = true;
		if (!pExplosionTable->GetValue("bEnabled", bEnabled) || bEnabled)
		{
			CreateExplosion(pExplosionTable, pCollision->pt, pCollision->n);
		}
	}

	TListeners::iterator itListener = m_Listeners.begin();
	TListeners::iterator itListenerEnd = m_Listeners.end();
	for (; itListener != itListenerEnd; ++itListener)
	{
		CAlienDropPodListener *pListener = *itListener;
		CRY_ASSERT(pListener);

		pListener->OnPodBounced(m_nCurrentBounces+1, pCollision->pt);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::OnFinishedBouncing()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	IGameObject *pGameObject = GetGameObject();
	CRY_ASSERT(pGameObject);

	pGameObject->EnablePhysicsEvent(false, eEPE_OnCollisionLogged);

	if (m_bComeToRest)
	{
		m_eState = eState_Rolling;

		// Start updating to detect when we stop rolling
		pGameObject->EnableUpdateSlot(this, 0);
	}
	else
	{
		m_eState = eState_Resting;

		OnAtRestingPoint();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::OnAtRestingPoint()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	IGameObject *pGameObject = GetGameObject();
	CRY_ASSERT(pGameObject);

	IEntity *pEntity = GetEntity();
	CRY_ASSERT(pEntity);

	pGameObject->DisableUpdateSlot(this, 0);

	m_eState = eState_Resting;

	const Vec3 vPos = pEntity->GetWorldPos();
	TListeners::iterator itListener = m_Listeners.begin();
	TListeners::iterator itListenerEnd = m_Listeners.end();
	for (; itListener != itListenerEnd; ++itListener)
	{
		CAlienDropPodListener *pListener = *itListener;
		CRY_ASSERT(pListener);

		pListener->OnPodLanded(vPos);
	}

	// Create the pod open effect
	SmartScriptTable pExplosionTable;
	if (GetTableFromProperties("PodOpenExplosion", pExplosionTable))
	{
		bool bEnabled = true;
		if (!pExplosionTable->GetValue("bEnabled", bEnabled) || bEnabled)
		{
			// Start listening for explosions so we know when its safe to spawn the alien
			if (CGameRules *pGameRules = g_pGame->GetGameRules())
				pGameRules->AddHitListener(this);

			CreateExplosion(pExplosionTable, vPos, Vec3Constants<float>::fVec3_OneZ);
		}
		else
		{
			OnSpawnAlien();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAlienDropPod::OnSpawnAlien()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	IEntity *pEntity = GetEntity();
	CRY_ASSERT(pEntity);

	IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
	CRY_ASSERT(pEntitySystem);

	const Vec3 vPos = pEntity->GetWorldPos();

	// Spawn the alien or beam the associated one
	IEntity *pNewAlienEntity = GetLinkedAIEntity();
	if (!pNewAlienEntity)
	{
		char const* szArchetype = NULL;
		SmartScriptTable pAlienSpawnTable;
		if (GetTableFromProperties("AlienSpawn", pAlienSpawnTable))
		{
			pAlienSpawnTable->GetValue("es_entity_archetypes", szArchetype);
		}

		SEntitySpawnParams params;
		if (szArchetype && szArchetype[0])
		{
			params.sName = szArchetype;
			params.pArchetype = pEntitySystem->LoadEntityArchetype(szArchetype);
		}
		else
		{
			params.sName = "AlienFromDropPod";
		}
		params.pClass = pEntitySystem->GetClassRegistry()->GetDefaultClass();
		params.vPosition = vPos;

		pNewAlienEntity = params.pArchetype ? pEntitySystem->SpawnEntity(params, false) : NULL;
		if (pNewAlienEntity)
		{
			SmartScriptTable pPropertiesInstance;
			IScriptTable *pScriptTable = pNewAlienEntity->GetScriptTable();
			if (pScriptTable && pScriptTable->GetValue("PropertiesInstance", pPropertiesInstance))
			{
				SmartScriptTable pAITerritoryAndWave;
				if (pPropertiesInstance->GetValue("AITerritoryAndWave", pAITerritoryAndWave))
				{
					pAITerritoryAndWave->SetValue("aiterritory_Territory", m_sTerritory.c_str());
					pAITerritoryAndWave->SetValue("aiwave_Wave", m_sWave.c_str());
				}
			}

			pEntitySystem->InitEntity(pNewAlienEntity, params);
		}

		if (!pNewAlienEntity)
		{
			GameWarning("Alien drop pod failed to spawn the alien using archetype \'%s\'"
				"- if an alien was linked to the pod, it's possible it was killed before the pod was launched", 
				szArchetype ? szArchetype : "UNKNOWN");
		}
	}

	EntityId newAlienEntityId = 0;
	if (pNewAlienEntity)
	{
		newAlienEntityId = pNewAlienEntity->GetId();

		pNewAlienEntity->SetPos(vPos);
		pNewAlienEntity->Hide(false);
		pNewAlienEntity->Activate(true);
	}

	TListeners::iterator itListener = m_Listeners.begin();
	TListeners::iterator itListenerEnd = m_Listeners.end();
	for (; itListener != itListenerEnd; ++itListener)
	{
		CAlienDropPodListener *pListener = *itListener;
		CRY_ASSERT(pListener);

		pListener->OnPodOpened(newAlienEntityId);
	}

	m_eState = eState_Finished;

	// Remove the pod
	pEntitySystem->RemoveEntity(GetEntityId());
}

//////////////////////////////////////////////////////////////////////////
bool CAlienDropPod::AddListener(CAlienDropPodListener *pListener)
{
	return stl::push_back_unique(m_Listeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
bool CAlienDropPod::RemoveListener(CAlienDropPodListener *pListener)
{
	return stl::find_and_erase(m_Listeners, pListener);
}
