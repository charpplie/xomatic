/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Damage Effect Controller

-------------------------------------------------------------------------
History:
- 02:09:2009   15:00 : Created by Claire Allan

***********************************************************************/

#include "StdAfx.h"
#include "ActorDamageEffectController.h"
#include "Game.h"
#include "GameRules.h"
#include "ParticleParams.h"
#include "Actor.h"
#include "Player.h"
#include "Audio/AudioSignalPlayer.h"
#include "HUD/HUD_Radar.h"
#include "HUD/HUD.h"

uint32 CKVoltEffect::s_hashId = 0;
uint32 CAttachedParticleDamageEffect::s_hashId = 0;
uint32 CNanoSuitEventDamageEffect::s_hashId = 0;
uint32 CVolatileDamageEffect::s_hashId = 0;
uint32 CTinnitusEffect::s_hashId = 0;
uint32 CEntityTimerEffect::s_hashId = 0;

uint32 CNanoSuitEventDamageEffect::s_kvoltEventId = 0;
uint32 CNanoSuitEventDamageEffect::s_empEventId = 0;
uint32 CNanoSuitEventDamageEffect::s_cloakInterferenceId = 0;

uint32 CDamageEffectController::CreateHash(const char* string)
{
	return GetISystem()->GetCrc32Gen()->GetCRC32Lowercase(string);
}

void CDamageEffectController::Init(CActor* actor)
{
	if (CKVoltEffect::s_hashId == 0) //once initialised it can't be 0
	{
		CKVoltEffect::s_hashId = CreateHash("KVoltFX");
		CAttachedParticleDamageEffect::s_hashId = CreateHash("ParticleFx");
		CNanoSuitEventDamageEffect::s_hashId = CreateHash("SuitEvent");
		CVolatileDamageEffect::s_hashId = CreateHash("VolatileFx");
		CTinnitusEffect::s_hashId = CreateHash("TinnitusFx");
		CEntityTimerEffect::s_hashId = CreateHash("TimerFX");

		CNanoSuitEventDamageEffect::s_kvoltEventId = CreateHash("KVOLT");
		CNanoSuitEventDamageEffect::s_empEventId = CreateHash("EMP");
		CNanoSuitEventDamageEffect::s_cloakInterferenceId = CreateHash("INTERFERENCE");
	}

	IGameRules* pGameRules = g_pGame->GetGameRules();
	m_ownerActor = actor;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		m_effectList[i] = NULL;
		m_associatedHitType[i] = -1;
		m_minDamage[i] = -1.0f;
	}

	m_activeEffectsBitfield = 0;
	m_effectsResetSwitchBitfield = 0;
	m_effectsKillBitfield = 0;

	const IItemParamsNode* actorParams = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActorParams(actor->GetActorClass());

	if (actorParams)
	{
		const IItemParamsNode* damageEffectParams = actorParams->GetChild("DamageEffectParams");

		int numChildren = damageEffectParams->GetChildCount();
		int allowSerialise = 1;

		damageEffectParams->GetAttribute("allowSerialise", allowSerialise);
		m_allowSerialise = allowSerialise ? true : false;

		CRY_ASSERT_MESSAGE(numChildren <= MAX_NUM_DAMAGE_EFFECTS, "Too many damage effects found. Increase the MAX_NUM_DAMAGE_EFFECTS and size of activeEffects and effectsResetSwitch");

		for (int i = 0; i < numChildren; i++)
		{
			const IItemParamsNode* child = damageEffectParams->GetChild(i);
			const IItemParamsNode* effect = child->GetChild(0);

			const char* hittype = child->GetAttribute("hittype");
			const char* name = effect->GetName();

			m_associatedHitType[i] = pGameRules->GetHitTypeId(hittype);

			child->GetAttribute("minDamage", m_minDamage[i]);	
			
			uint64 hashcode = CreateHash(name);

			if (hashcode == CKVoltEffect::s_hashId)
			{
				m_effectList[i] = new CKVoltEffect();
			}
			else if (hashcode == CAttachedParticleDamageEffect::s_hashId)
			{
				m_effectList[i] = new CAttachedParticleDamageEffect();
			}
			else if (hashcode == CNanoSuitEventDamageEffect::s_hashId)
			{
				m_effectList[i] = new CNanoSuitEventDamageEffect();
			}
			else if (hashcode == CVolatileDamageEffect::s_hashId)
			{
				m_effectList[i] = new CVolatileDamageEffect();
			}
			else if (hashcode == CTinnitusEffect::s_hashId)
			{
				m_effectList[i] = new CTinnitusEffect();
			}
			else if (hashcode == CEntityTimerEffect::s_hashId)
			{
				m_effectList[i] = new CEntityTimerEffect();
			}
			else
			{
				CRY_ASSERT_MESSAGE(0,"INVALID DAMAGE EFFECT SERIALISED");
			}
			
			m_effectList[i]->Init(actor, effect);
		}
	}
}

void CDamageEffectController::OnHit(const HitInfo *hitInfo) 
{
	uint8 bitCheck = 1;

	bool netSync = false;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		if (m_associatedHitType[i] == hitInfo->type && m_minDamage[i] < hitInfo->damage)
		{
			netSync = true;
			CRY_ASSERT_MESSAGE(m_effectList[i], "The effect should not be null if a hit type is associated with it");
			if (m_activeEffectsBitfield & bitCheck)
			{
				m_effectList[i]->Reset();
				m_effectsResetSwitchBitfield = m_effectsResetSwitchBitfield & bitCheck 
											? m_effectsResetSwitchBitfield & ~bitCheck 
											: m_effectsResetSwitchBitfield | bitCheck;
			}
			else
			{
				m_effectList[i]->Enter();

				m_effectsResetSwitchBitfield &= ~bitCheck;
				m_activeEffectsBitfield |= bitCheck;
			}			
		}
		bitCheck = bitCheck << 1;
	}

	if (netSync)
	{
		CHANGED_NETWORK_STATE(m_ownerActor, eEA_GameServerDynamic);
	}
}

void CDamageEffectController::OnKill(const HitInfo* hitInfo) 
{
	uint8 bitCheck = 1;

	bool netSync = false;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		if (m_associatedHitType[i] == hitInfo->type)
		{
			netSync = true;
			CRY_ASSERT_MESSAGE(m_effectList[i], "The effect should not be null if a hit type is associated with it");
			if (!(m_activeEffectsBitfield & bitCheck))
			{
				m_effectList[i]->Enter();
				m_effectsResetSwitchBitfield &= ~bitCheck;
				m_activeEffectsBitfield |= bitCheck;
			}
			
			m_effectList[i]->OnKill();
			m_effectsKillBitfield |= bitCheck;		
		}
		bitCheck = bitCheck << 1;
	}

	if (netSync)
	{
		CHANGED_NETWORK_STATE(m_ownerActor, eEA_GameServerDynamic);
	}
}

void CDamageEffectController::OnRevive() 
{
	uint8 bitCheck = 1;

	bool netSync = false;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		if (m_activeEffectsBitfield & bitCheck)
		{
			netSync = true;
			CRY_ASSERT_MESSAGE(m_effectList[i], "The effect should not be null if it is active");
			
			m_effectList[i]->Leave();
		}
		bitCheck = bitCheck << 1;
	}

	m_activeEffectsBitfield = 0;
	m_effectsKillBitfield = 0;

	if (netSync)
	{
		CHANGED_NETWORK_STATE(m_ownerActor, eEA_GameServerDynamic);
	}
}

void CDamageEffectController::UpdateEffects(SEntityUpdateContext& ctx) 
{
	uint8 bitCheck = 1;

	bool netSync = false;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		if (m_activeEffectsBitfield & bitCheck)
		{
			CRY_ASSERT_MESSAGE(m_effectList[i], "The effect should not be null if it is active");

			if (!m_effectList[i]->Update(ctx) && gEnv->bServer)
			{
				netSync = true;
				m_effectList[i]->Leave();
				m_activeEffectsBitfield &= ~bitCheck;
			}
		}
		bitCheck = bitCheck << 1;
	}

	if (netSync)
	{
		CHANGED_NETWORK_STATE(m_ownerActor, eEA_GameServerDynamic);
	}
}

void CDamageEffectController::NetSerialiseEffects(TSerialize ser, EEntityAspects aspect)
{
	if(m_allowSerialise && aspect == eEA_GameServerDynamic)
	{
		ser.Value("activeEffects", m_ownerActor, &CActor::GetActiveDamageEffects, &CActor::SetActiveDamageEffects, 'ui8');
		ser.Value("effectReset", m_ownerActor, &CActor::GetDamageEffectsResetSwitch, &CActor::SetDamageEffectsResetSwitch, 'ui8');
		ser.Value("effectKilled", m_ownerActor, &CActor::GetDamageEffectsKilled, &CActor::SetDamageEffectsKilled, 'ui8');
	}
}

void CDamageEffectController::SetActiveEffects(uint8 active)
{
	uint8 bitCheck = 1;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		bool serverActive = !((active & bitCheck) == 0);
		bool locallyActive = !((m_activeEffectsBitfield & bitCheck) == 0);
		if ( serverActive != locallyActive )
		{
			CRY_ASSERT_MESSAGE(m_effectList[i], "The effect should not be null");
			if (serverActive)
			{
				m_effectList[i]->Enter();

				m_effectsResetSwitchBitfield &= ~bitCheck;
				m_activeEffectsBitfield |= bitCheck;
			}
			else
			{
				m_effectList[i]->Leave();
				m_activeEffectsBitfield &= ~bitCheck;
			}
		}
		bitCheck = bitCheck << 1;
	}
}

void CDamageEffectController::SetEffectResetSwitch(uint8 reset)
{
	uint8 bitCheck = 1;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		if ((reset & bitCheck) != (m_effectsResetSwitchBitfield & bitCheck))
		{
			CRY_ASSERT_MESSAGE((m_activeEffectsBitfield & bitCheck), "Might not leave the effect and shouldn't happen");
			CRY_ASSERT_MESSAGE(m_effectList[i], "The effect should not be null");
			m_effectList[i]->Reset();
		}
		bitCheck = bitCheck << 1;
	}

	m_effectsResetSwitchBitfield = reset;
}

void CDamageEffectController::SetEffectsKilled(uint8 killed)
{
	unsigned int bitCheck = 1;

	for(int i = 0; i < MAX_NUM_DAMAGE_EFFECTS; i++)
	{
		if ((killed & bitCheck) && !(m_effectsKillBitfield & bitCheck))
		{
			CRY_ASSERT_MESSAGE(m_effectList[i], "The effect should not be null");
			m_effectList[i]->OnKill();
		}
		bitCheck = bitCheck << 1;
	}

	m_effectsKillBitfield = killed;
}

void CKVoltEffect::Init(CActor* actor, const IItemParamsNode* params)
{
	inherited::Init(actor, params);

	params->GetAttribute("timer", m_effectTime);
	const char* effect = params->GetAttribute("effect");
	const char* screenEffect = params->GetAttribute("screenEffect");

	m_particleEffect = gEnv->pParticleManager->FindEffect(effect);
	m_screenEffect = gEnv->pParticleManager->FindEffect(screenEffect);

	m_screenAttachment = NULL;
	m_particleEmitter = NULL;
	
	if (!m_particleEffect || !m_screenEffect)
	{
		GameWarning("Invalid Particle Effect for KVolt Effect");
	}
}

void CKVoltEffect::Enter()
{
	m_timer = m_effectTime;

	SHUDEvent eventTempAddToRadar(eHUDEvent_TemporarilyTrackEntity);
	eventTempAddToRadar.AddData(SHUDEventData((int)m_ownerActor->GetEntityId()));
	eventTempAddToRadar.AddData(SHUDEventData(m_effectTime));
	CHUD::CallEvent(eventTempAddToRadar);
	
	if(m_particleEffect)
	{
		IEntity* entity = m_ownerActor->GetEntity();

		const ParticleParams& particleParams = m_particleEffect->GetParticleParams();

		EGeomType geomType = particleParams.eAttachType;
		EGeomForm geomForm = particleParams.eAttachForm;
		SpawnParams	spawnParams( geomType,geomForm );

		m_particleEmitter = m_particleEffect->Spawn(spawnParams.bIndependent,entity->GetWorldTM());
		m_particleEmitter->AddRef();

		// Set spawn params so that the emitter spawns particles all over the character's physics mesh
		ICharacterInstance* characterInstance = entity->GetCharacter(0);
		if(characterInstance)
		{
			GeomRef geomRef;
			geomRef.m_pChar = characterInstance;
			geomRef.m_pPhysEnt = characterInstance->GetISkeletonPose()->GetCharacterPhysics();
			m_particleEmitter->SetSpawnParams(spawnParams,geomRef);
		}
	}
	ResetScreenEffect();
}

void CKVoltEffect::ResetScreenEffect()
{
	if(m_screenEffect && m_ownerActor->IsClient())
	{
		if(!m_screenAttachment)
		{
			IAttachmentManager* pAttachmentMan = m_ownerActor->GetEntity()->GetCharacter(0)->GetIAttachmentManager();

			if(pAttachmentMan)
			{
				m_screenAttachment = pAttachmentMan->GetInterfaceByName("#camera");
			}
		}
		
		if(m_screenAttachment)
		{
			CEffectAttachment* pEffectAttachment = new CEffectAttachment(m_screenEffect->GetName(), Vec3(0,0,0), Vec3(0,1,0), 1.f);
			pEffectAttachment->CreateEffect(Matrix34(m_screenAttachment->GetAttWorldAbsolute()));
			if (pEffectAttachment->GetEmitter())
			{
				m_screenAttachment->AddBinding(pEffectAttachment);
			}
			else
			{
				delete pEffectAttachment;
			}
		}
		else
		{
			GameWarning("Failed to find #camera attachment");
		}
	}
}

void CKVoltEffect::Reset()
{ 
	m_timer = m_effectTime; 
	ResetScreenEffect();
}

void CKVoltEffect::Leave()
{
	if(m_particleEmitter)
	{
		gEnv->pParticleManager->DeleteEmitter(m_particleEmitter);
		m_particleEmitter->Release();
		m_particleEmitter = NULL;
	}

	if(m_screenAttachment)
	{
		m_screenAttachment->ClearBinding();
	}
}

bool CKVoltEffect::Update(SEntityUpdateContext& ctx)
{
	if(m_timer > ctx.fFrameTime)
	{
		m_timer -= ctx.fFrameTime;
		return true;
	}

	m_timer = 0.f;
	return false;
}


void CAttachedParticleDamageEffect::Init(CActor* actor, const IItemParamsNode* params)
{
	inherited::Init(actor, params);

	params->GetAttribute("timer", m_effectTime);
	const char* effect = params->GetAttribute("effect");

	m_particleEffect = gEnv->pParticleManager->FindEffect(effect);
	m_particleSlot = -1;


	if (!gEnv->pSystem->IsDedicated())
	{
		CRY_ASSERT_TRACE(m_particleEffect, ("Invalid particle effect '%s' (read from XML tag '%s' for %s \"%s\")", effect, params->GetName(), actor->GetEntity()->GetClass()->GetName(), actor->GetEntity()->GetName()));
	}
}

void CAttachedParticleDamageEffect::ResetParticleEffect()
{
	IEntity* pEntity = m_ownerActor->GetEntity();

	SpawnParams spawnParams;
	spawnParams.fSizeScale = 1.0f;
	spawnParams.eAttachType = GeomType_Render;
	spawnParams.eAttachForm = GeomForm_Surface;

	m_particleSlot = pEntity->LoadParticleEmitter(m_particleSlot, m_particleEffect, &spawnParams);
}

void CAttachedParticleDamageEffect::Enter()
{
	m_timer = m_effectTime;
	m_particleSlot = -1;

	ResetParticleEffect();
}

void CAttachedParticleDamageEffect::Reset()
{ 
	m_timer = m_effectTime; 

	ResetParticleEffect();
}

void CAttachedParticleDamageEffect::Leave()
{
	IEntity* pEntity = m_ownerActor->GetEntity();

	pEntity->FreeSlot(m_particleSlot);
}

bool CAttachedParticleDamageEffect::Update(SEntityUpdateContext& ctx)
{
	if(m_timer > ctx.fFrameTime)
	{
		m_timer -= ctx.fFrameTime;
		return true;
	}

	m_timer = 0.f;
	return false;
}

void CNanoSuitEventDamageEffect::Init(CActor* actor, const IItemParamsNode* params)
{
	inherited::Init(actor, params);

	const char* eventType = NULL;
	eventType = params->GetAttribute("enterEvent");
	m_enterEventType = FindEvent(eventType);
	eventType = NULL;
	eventType = params->GetAttribute("exitEvent");
	m_exitEventType = FindEvent(eventType);

	int boolParam = 0;
	
	params->GetAttribute("enterFloat", m_enterFloat);
	params->GetAttribute("enterBool", boolParam);
	m_enterBool = boolParam ? true : false;
	params->GetAttribute("exitFloat", m_exitFloat);
	params->GetAttribute("exitBool", boolParam);
	m_exitBool = boolParam ? true : false;
}

ENanoSuitEvent CNanoSuitEventDamageEffect::FindEvent(const char* eventName)
{
	ENanoSuitEvent eventType = eNanoSuitEvent_LAST;

	if(eventName && eventName[0])
	{
		uint64 hashcode = CDamageEffectController::CreateHash(eventName);

		if(hashcode == s_empEventId)
		{
			eventType = eNanoSuitEvent_EMP_DISCHARGE;
		}
		else if(hashcode == s_kvoltEventId)
		{
			eventType = eNanoSuitEvent_KVOLT_CHARGE;
		}
		else if(hashcode == s_cloakInterferenceId)
		{
			eventType = eNanoSuitEvent_CLOAK_INTERFERENCE;
		}
		else
		{
			CRY_ASSERT_MESSAGE(0,"Invalid event serialised for Nano Suit Event");
		}
	}
	return eventType;
}

void CNanoSuitEventDamageEffect::Enter()
{
	SendEvent(m_enterEventType, m_enterFloat, m_enterBool);
}

void CNanoSuitEventDamageEffect::Reset()
{ 
	SendEvent(m_enterEventType, m_enterFloat, m_enterBool);
}

void CNanoSuitEventDamageEffect::Leave()
{ 
	SendEvent(m_exitEventType, m_exitFloat, m_exitBool);
}

void CNanoSuitEventDamageEffect::SendEvent(ENanoSuitEvent eventType, float fParam, bool bParam)
{
	if (eventType != eNanoSuitEvent_LAST)
	{
		SNanoSuitEvent event;
		event.event = eventType;
		event.fParam = fParam;
		event.bParam = bParam;
		m_ownerActor->SendActorSuitEvent(event);
	}
}

void CVolatileDamageEffect::Init(CActor* actor, const IItemParamsNode* params)
{
	inherited::Init(actor, params);

	const char* killEffect = params->GetAttribute("killEffect");
	const char* screenEffectLib = params->GetAttribute("screenEffectLib");
	const char* screenEffect = params->GetAttribute("screenEffectName");
	
	params->GetAttribute("damageTime", m_damageTime);
	params->GetAttribute("startDamage", m_startDamage);
	params->GetAttribute("endDamage", m_endDamage);
	params->GetAttribute("minPlayerHealth", m_minHealth);

	m_killParticle = gEnv->pParticleManager->FindEffect(killEffect);
	m_screenEffectId = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects()->GetEffectIdByName(screenEffectLib, screenEffect);
	
	if (!killEffect)
	{
		GameWarning("Invalid Particle Effect '%s' for Volatile Kill Effect. This will crash once activated", killEffect);
	}
}

void CVolatileDamageEffect::Enter()
{
	Reset();
}

void CVolatileDamageEffect::Reset()
{ 	
	m_timer = m_damageTime;
	m_cumulativeDamage = 0.f;

	PlayAcidSplashSound();
	SpawnScreenEffect();
}

bool CVolatileDamageEffect::Update(SEntityUpdateContext& ctx)
{
	//Do the damage on the server as it is health authoritative and on the local client so it doesn't start regenerating too early
	if ((gEnv->bServer || g_pGame->GetIGameFramework()->GetClientActor() == m_ownerActor) && m_timer > 0.f)
	{
		float frameDamage = m_endDamage + ((m_startDamage - m_endDamage) * (m_timer/m_damageTime));
		m_timer -= ctx.fFrameTime;
		m_cumulativeDamage += (frameDamage * ctx.fFrameTime);
		int health = m_ownerActor->GetHealth();

		m_cumulativeDamage = min((float)(health - m_minHealth), m_cumulativeDamage);

		if (m_cumulativeDamage >= 1.f)
		{
			int damage = (int) m_cumulativeDamage;

			health -= damage;
			m_ownerActor->SetHealth(health);
			m_cumulativeDamage -= damage;
		}	
	}

	return true;
}

void CVolatileDamageEffect::Leave()
{
	IEntity* pEntity = m_ownerActor->GetEntity();
	pEntity->Hide(false);

}

void CVolatileDamageEffect::SpawnScreenEffect()
{
	if(m_ownerActor == g_pGame->GetIGameFramework()->GetClientActor())
	{
		IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();

		SMFXRunTimeEffectParams effectParams;
		effectParams.pos = m_ownerActor->GetEntity()->GetWorldPos();
		effectParams.soundSemantic = eSoundSemantic_HUD;

		pMaterialEffects->StopEffect(m_screenEffectId);
		pMaterialEffects->ExecuteEffect(m_screenEffectId, effectParams);
	}
}

void CVolatileDamageEffect::PlayAcidSplashSound()
{
	CAudioSignalPlayer::JustPlay("VolatileSpike_AcidSplash", m_ownerActor->GetEntityId());
}

void CVolatileDamageEffect::OnKill()
{
	IEntity* pEntity = m_ownerActor->GetEntity();

	pEntity->Hide(true);
	
	if(m_killParticle)
	{
		m_killParticle->Spawn(true, IParticleEffect::ParticleLoc(pEntity->GetWorldPos(), Vec3(0.0f,0.0f,1.0f), 1.0f));
	}	
}

void CTinnitusEffect::Init(CActor* actor, const IItemParamsNode* params)
{
	inherited::Init(actor, params);

	params->GetAttribute("timer", m_tinnitusTime);
	m_timer = 0;
}

void CTinnitusEffect::Enter()
{
	m_timer = m_tinnitusTime;

	if(m_ownerActor == g_pGame->GetIGameFramework()->GetClientActor())
	{
		static_cast<CPlayer*>(m_ownerActor)->StartTinnitus();	
	}
}

void CTinnitusEffect::Reset()
{ 
	Enter();
}

void CTinnitusEffect::Leave()
{
	if(m_ownerActor == g_pGame->GetIGameFramework()->GetClientActor())
	{
		static_cast<CPlayer*>(m_ownerActor)->StopTinnitus();
	}
}

bool CTinnitusEffect::Update(SEntityUpdateContext& ctx)
{
	if(m_timer > ctx.fFrameTime)
	{
		m_timer -= ctx.fFrameTime;
		
	if(m_ownerActor == g_pGame->GetIGameFramework()->GetClientActor())
	{
		static_cast<CPlayer*>(m_ownerActor)->UpdateTinnitus(m_timer/m_tinnitusTime);
	}

		return true;
	}

	m_timer = 0.f;
	return false;
}

void CEntityTimerEffect::Init(CActor* actor, const IItemParamsNode* params)
{
	inherited::Init(actor, params);

	params->GetAttribute("timerID", m_entityTimerID);
	params->GetAttribute("time", m_initialTime);
	m_timer = 0.f;
}

void CEntityTimerEffect::Enter()
{
	m_timer = m_initialTime;

	if(gEnv->bServer)
	{
		m_ownerActor->GetEntity()->SetTimer(m_entityTimerID, (int)(m_initialTime*1000.f));
	}
}

bool CEntityTimerEffect::Update(SEntityUpdateContext& ctx)
{
	if(m_timer > ctx.fFrameTime)
	{
		m_timer -= ctx.fFrameTime;

		return true;
	}

	m_timer = 0.f;
	return false;
}
