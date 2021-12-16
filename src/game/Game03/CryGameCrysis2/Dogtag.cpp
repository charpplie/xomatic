/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 04:12:2009   10:30 : Created by Ben Parbury

*************************************************************************/
#include "StdAfx.h"
#include "Dogtag.h"

#include "ItemParamReader.h"
#include "Player.h"
#include "GameRules.h"
#include "PlayerProgression.h"
#include "HUD/HUD.h"
#include "Utility/StringUtils.h"

const static float k_dogtagLifetimeTimer = 30.0f;

//====================================================

//----------------------------------------------------
void CDogtag::STagInfo::Clear()
{
	memset(this, 0, sizeof(*this));
}

//----------------------------------------------------
void CDogtag::STagInfo::AssignFromEntity(EntityId eid)
{
	m_ownerId = eid;

	if (IEntity* e=gEnv->pEntitySystem->GetEntity(eid))
	{
		cry_strncpy(m_ownerName, e->GetName(), sizeof(m_ownerName));

		CRY_TODO(11,03,2010, "[tlh] Need to assign clan name/tag so can be displayed on the HUD dog tag popup.");
		//cry_strncpy(m_clanName, ???. sizeof(m_clanName));  // [tlh] TODO!!

		CPlayerProgression*  pp = CPlayerProgression::GetInstance();
		CRY_ASSERT(pp);

		m_rank = pp->GetData(EPP_Rank);

		m_progressTactical = pp->GetSuitModeData(eNanoSuitMode_Tactical, ePPS_Level);
		m_progressStealth = pp->GetSuitModeData(eNanoSuitMode_Stealth, ePPS_Level);
		m_progressArmour = pp->GetSuitModeData(eNanoSuitMode_Armor, ePPS_Level);
		m_progressPower = pp->GetSuitModeData(eNanoSuitMode_Power, ePPS_Level);
	}
}

//====================================================

//----------------------------------------------------
CDogtag::CDogtag()
{
	m_info.Clear();
	m_effectSlot = -1;
	m_timer = k_dogtagLifetimeTimer;
	m_pickedUp = false;
}

//----------------------------------------------------
CDogtag::~CDogtag()
{
}

//----------------------------------------------------
void CDogtag::PostInit( IGameObject * pGameObject )
{
	CItem::PostInit(pGameObject);

	GetEntity()->EnablePhysics(true);
	Physicalize(true, true);
	SetAspectProfile(eEA_Physics, eIPhys_PhysicalizedRigid);

	IEntityPhysicalProxy *pPhysics = GetPhysicalProxy();
	if (pPhysics)
	{
		Vec3 impulse;
		impulse.SetRandomDirection();
		const static float k_impulseScale = 0.2f;
		pPhysics->AddImpulse(-1, Vec3(0.0f, 0.0f, 0.0f), impulse, true, k_impulseScale);
	}

	EnableUpdate(true, eIUS_General);
	EnableUpdate(false, eIUS_Scheduler);

	if(!m_sharedparams->pDogTagParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "DOGTAG PARAMS: Item of type CDogtag is missing it's dogtag params!");
		return;
	}

	if(m_sharedparams->pDogTagParams->pEffect)
	{
		m_effectSlot = GetEntity()->LoadParticleEmitter(m_effectSlot, m_sharedparams->pDogTagParams->pEffect);
	}
}

//----------------------------------------------------
void CDogtag::PickUp(EntityId pickerId, bool sound, bool select, bool keepHistory, const char *setup)
{
	CRY_ASSERT(pickerId == gEnv->pGame->GetIGameFramework()->GetClientActorId());

	if (m_pickedUp)
	{
		return;
	}
	
	if(!m_sharedparams->pDogTagParams)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "DOGTAG PARAMS: Item of type CDogtag is missing it's dogtag params!");
		return;
	}
	
	CPlayer* pPlayer = static_cast<CPlayer*>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	if(pPlayer)
	{
		CPlayerProgression::GetInstance()->Event(EPP_DogtagCollection);
	}

	CAudioSignalPlayer::JustPlay(m_sharedparams->pDogTagParams->pickupAudio, GetEntity()->GetWorldPos());

	if(m_effectSlot != -1)
	{
		IParticleEmitter *pEmitter = GetEntity()->GetParticleEmitter(m_effectSlot);
		if(pEmitter)
		{
			pEmitter->Activate(false);
		}
	}

	SHUDEvent  eventCollected (eHUDEvent_OnDogTagCollected);
	eventCollected.AddData(SHUDEventData((void*)&m_info));
	CHUD::CallEvent(eventCollected);

	gEnv->pEntitySystem->RemoveEntity(GetEntityId());
	m_pickedUp = true;  // because the RemoveEntity() call won't necessarily be immediate
}

//-----------------------------------------------------
void CDogtag::PickUpAmmo(EntityId pickerId)
{
	PickUp(pickerId);
}

//----------------------------------------------------
void CDogtag::Update( SEntityUpdateContext& ctx, int slot)
{
	CRY_ASSERT_MESSAGE(slot == eIUS_General, "CDogtag::Update should only be general");
	m_timer -= ctx.fFrameTime;
	if(m_timer < 0.0f)
	{
		RemoveEntity(true);
	}
}

//----------------------------------------------------
//Static
void CDogtag::SpawnDogtag(EntityId shooterId, EntityId victimId)
{
	if(ShouldSpawnDogtag(shooterId, victimId))
	{
		IEntity *pSpawnAtEntity = gEnv->pEntitySystem->GetEntity(victimId);
		if (pSpawnAtEntity)
		{
			// Spawn entity
			CryFixedStringT<16> sName;
			sName.Format("Dogtag_%i", victimId);

			SEntitySpawnParams params;
			static IEntityClass* pDogtag = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Dogtag");
			params.pClass = pDogtag;
			params.sName = sName.c_str();
			AABB aabb;
			pSpawnAtEntity->GetWorldBounds(aabb);
			params.vPosition = aabb.GetCenter();
			Ang3 rotation = pSpawnAtEntity->GetWorldAngles();
			params.qRotation = Quat(rotation);
			params.nFlags |= ENTITY_FLAG_CLIENT_ONLY;

			IEntity *pSpawnedEntity = gEnv->pEntitySystem->SpawnEntity(params, true);
			if (pSpawnedEntity)
			{
				CryLog("Spawned dogtag - %s, %d", pSpawnedEntity->GetName(), pSpawnedEntity->GetId());

				CDogtag* pDogtag = static_cast<CDogtag*>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(pSpawnedEntity->GetId()));
				if(pDogtag)
				{
					pDogtag->m_info.AssignFromEntity(victimId);
				}
			}
		}
	}
}

//Static
bool CDogtag::ShouldSpawnDogtag(EntityId shooterId, EntityId victimId)
{
	EntityId clientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	if(shooterId == clientId && victimId != clientId && g_pGameCVars->g_dogtagsEnable == 1 && gEnv->bMultiplayer)
	{
		return true;
	}

	return false;
}
