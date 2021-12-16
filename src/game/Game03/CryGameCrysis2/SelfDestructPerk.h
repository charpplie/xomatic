#ifndef __SELFDESTRUCTPERK_H__
#define __SELFDESTRUCTPERK_H__

#include "IPerk.h"

class CAudioSignalPlayer;

class SelfDestructPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(SelfDestructPerk);

	SelfDestructPerk();
	virtual ~SelfDestructPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual const void* GetData(EPlayerPlugInData dataType);
	virtual void NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void Update(const float dt);

protected:
	void DrawDebugHUD();
	void SelfDestructExplode();
	void StartMinigame();
	void EnableInputs(bool enable);

	float m_timer;		//time in minigame state (server and local only)
	float m_power;		//power on meter (net serialized)
	bool m_minigame;	//if were in a minigame
	bool m_explode;		//if we wanted to explode (net serialized)
	CAudioSignalPlayer m_warningSignal;
	EntityId m_killerId;
};

#endif