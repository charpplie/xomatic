#ifndef __EMPSTRIKEPERK_H__
#define __EMPSTRIKEPERK_H__

#include "IPerk.h"
#include "Perk.h"
#include "AutoEnum.h"

#define StrikeStateList(f)  \
	f(ESPS_uninit)            \
	f(ESPS_none)              \
	f(ESPS_pointer)           \
	f(ESPS_strike)

AUTOENUM_BUILDENUMWITHTYPE(ESP_State, StrikeStateList);

class EMPStrikePerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(EMPStrikePerk);

	EMPStrikePerk();
	virtual ~EMPStrikePerk() {}

	virtual void Enter();
	virtual void Leave();

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data = NULL);
	virtual void Update(const float dt);
	virtual void NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);

protected:
	void InitTargetingPointer();
	void UpdatePointer();
	bool GetStrikePointerPos(Vec3 &outPos);
	void ActivateStrikeFromPointer();
	void ActivateStrike(Vec3 *pos);
	void DoStrike(Vec3 firePos);
	void SetState(ESP_State newState);

	// Overloaded in SatStrikePerk.h
	virtual void FireStrike(Vec3 firePos);

	// m_state is now only used by local player... for remote players, this stays set to ESPS_none
	ESP_State m_state;

	ColorF m_color;
	Vec3 m_mostRecentStrikePos;
	IParticleEffect* m_targetingRingsEffect;
	EntityId m_pointerEntityId;
	const char* m_announcement;
	const char* m_audioSignal;
	uint8 m_numTimesFired;
	bool m_everRead;
};

#endif