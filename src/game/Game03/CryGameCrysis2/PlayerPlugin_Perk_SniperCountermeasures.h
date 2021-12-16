#ifndef __PERK_SNIPERCOUNTERMEASURES_H__
#define __PERK_SNIPERCOUNTERMEASURES_H__

#include "IPerk.h"

class CAudioSignalPlayer;

class CPlayerPlugin_Perk_SniperCountermeasures : public IPerk
{
	public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Perk_SniperCountermeasures);

	virtual void Enter();
	virtual void Leave();

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data = NULL);
	virtual void Update(const float dt);

	ILINE void InformBeingLookedAt()
	{
		m_cumulativePowerDrain += 1.f;
		m_beingTargetted = true;
	}

	ILINE float GetPowerRemaining() const
	{
		return m_perkAntiSniperEnergy;
	}

	static CPlayerPlugin_Perk_SniperCountermeasures * GetInstanceBelongingTo(EntityId who);

	private:
	CPlayerPlugin_Perk_SniperCountermeasures * m_prev;
	CPlayerPlugin_Perk_SniperCountermeasures * m_next;
	float m_perkAntiSniperEnergy;
	float m_cumulativePowerDrain;
	bool m_beingTargetted;
	CAudioSignalPlayer m_targettedSignal;
	bool m_isPlayingTargetted;
};

class CPlayerPlugin_TriggerSniperCountermeasures : public CPlayerPlugin
{
	public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_TriggerSniperCountermeasures);

	virtual void Enter();
	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data = NULL);
	virtual void Update(const float dt);
	virtual void NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);

	private:
	EntityId m_currentTarget;
	bool m_scopeActive;
	float m_feedbackAmount;
	CAudioSignalPlayer m_targettingSignal;
};

#endif	// __PERK_SNIPERCOUNTERMEASURES_H__