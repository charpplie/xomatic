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

*************************************************************************/

#ifndef __DAMAGE_EFFECT_CONTROLLER_H__
#define __DAMAGE_EFFECT_CONTROLLER_H__

#define MAX_NUM_DAMAGE_EFFECTS (8) //This needs to match the size of activeEffectsBitfield and effectsResetSwitchBitfield

struct HitInfo;
class CDamageEffect;
class CActor;
struct IItemParamsNode;
struct IAttachment;

#include "IGameObject.h"
#include "NanoSuitDefs.h"
#include "IMaterialEffects.h"

class CDamageEffectController
{
public:

	void Init(CActor* m_ownerActor);
	void OnHit(const HitInfo* hitInfo);
	void OnKill(const HitInfo* hitInfo);
	void OnRevive();
	void UpdateEffects(SEntityUpdateContext& ctx);
	void NetSerialiseEffects(TSerialize ser, EEntityAspects aspect);
	uint8 GetActiveEffects() const { return m_activeEffectsBitfield; }
	uint8 GetEffectResetSwitch() const { return m_effectsResetSwitchBitfield; }
	uint8 GetEffectsKilled() const { return m_effectsKillBitfield; }
	void SetActiveEffects(uint8 active);
	void SetEffectResetSwitch(uint8 reset);
	void SetEffectsKilled(uint8 killed);

	static uint32 CreateHash(const char* string);

protected:

	CDamageEffect* m_effectList[MAX_NUM_DAMAGE_EFFECTS];
	int m_associatedHitType[MAX_NUM_DAMAGE_EFFECTS];
	float m_minDamage[MAX_NUM_DAMAGE_EFFECTS];

	CActor* m_ownerActor;

	uint8 m_activeEffectsBitfield;
	uint8 m_effectsResetSwitchBitfield;
	uint8 m_effectsKillBitfield;
	bool  m_allowSerialise;
};

class CDamageEffect
{
public:
	CDamageEffect() { m_ownerActor = NULL; }
	virtual ~CDamageEffect() {};

	virtual void Init(CActor* actor, const IItemParamsNode* params) { m_ownerActor = actor; }
	virtual void Enter() {};
	virtual void Leave() {};
	virtual void Reset() {};
	virtual void OnKill() {};
	virtual bool Update(SEntityUpdateContext& ctx) { return true; };

protected:
	CActor* m_ownerActor;
};

class CKVoltEffect : public CDamageEffect
{
private:
	typedef CDamageEffect inherited;

public:
	static uint32 s_hashId;

	virtual ~CKVoltEffect() { if(m_particleEmitter) m_particleEmitter->Release(); }

	void Init(CActor* actor, const IItemParamsNode* params);
	void Enter();
	void Leave();
	void Reset(); 
	bool Update(SEntityUpdateContext& ctx);
	
protected:
	void ResetScreenEffect();

	IParticleEffect* m_screenEffect;
	IParticleEffect* m_particleEffect;
	IParticleEmitter* m_particleEmitter;
	IAttachment* m_screenAttachment;
	float m_timer;
	float m_effectTime;
};

class CAttachedParticleDamageEffect : public CDamageEffect
{
private:
	typedef CDamageEffect inherited;

public:
	static uint32 s_hashId;

	void Init(CActor* actor, const IItemParamsNode* params);
	void Enter();
	void Leave();
	void Reset(); 
	bool Update(SEntityUpdateContext& ctx);

protected:
	void ResetParticleEffect();

	IParticleEffect* m_particleEffect;
	float m_timer;
	float m_effectTime;
	int m_particleSlot;
};

class CNanoSuitEventDamageEffect : public CDamageEffect
{
private:
	typedef CDamageEffect inherited;

public:
	static uint32 s_hashId;
	static uint32 s_kvoltEventId;
	static uint32 s_empEventId;
	static uint32 s_cloakInterferenceId;

	void Init(CActor* actor, const IItemParamsNode* params);
	void Enter();
	void Reset();
	void Leave();

protected:
	void SendEvent(ENanoSuitEvent eventType, float fParam, bool bParam);
	ENanoSuitEvent FindEvent(const char* eventName);

	float m_enterFloat;
	float m_exitFloat;
	ENanoSuitEvent m_enterEventType;
	ENanoSuitEvent m_exitEventType;
	bool m_enterBool;
	bool m_exitBool;
};

class CVolatileDamageEffect : public CDamageEffect
{
private:
	typedef CDamageEffect inherited;

public:
	static uint32 s_hashId;

	void Init(CActor* actor, const IItemParamsNode* params);
	void Enter();
	void Reset();
	void Leave();
	void OnKill();
	bool Update(SEntityUpdateContext& ctx);

protected:
	void SpawnScreenEffect();
	void PlayAcidSplashSound();

	float m_damageTime;
	float m_timer;
	float m_cumulativeDamage;
	float m_startDamage;
	float m_endDamage;
	int	  m_minHealth;
	TMFXEffectId m_screenEffectId;
	IParticleEffect* m_killParticle;
};

class CTinnitusEffect : public CDamageEffect
{
private:
	typedef CDamageEffect inherited;

public:
	static uint32 s_hashId;

	void Init(CActor* actor, const IItemParamsNode* params);
	void Enter();
	void Reset();
	void Leave();
	bool Update(SEntityUpdateContext& ctx);

protected:
	float m_timer;
	float m_tinnitusTime;
};

class CEntityTimerEffect : public CDamageEffect
{
private:
	typedef CDamageEffect inherited;

public:
	static uint32 s_hashId;

	void Init(CActor* actor, const IItemParamsNode* params);
	void Enter();
	bool Update(SEntityUpdateContext& ctx);

protected:
	int m_entityTimerID;
	float m_initialTime;
	float m_timer;
};

#endif