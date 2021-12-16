#ifndef __STAMPPERK_H__
#define __STAMPPERK_H__

#include "IPerk.h"

enum EStampState
{
	ESS_none = 0,
	ESS_jumping,
	ESS_stamping,
};

class StampPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(StampPerk);

	StampPerk();
	virtual ~StampPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual const void* GetData(EPlayerPlugInData dataType);

	virtual void Update(const float dt);
	virtual void NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);

protected:
	int CalculateDamage(float fallspeed);
	float CalculateRadius(float fallspeed);
	void SetState(EStampState state);

	EStampState m_state;
	bool m_stamping;

	float m_rechargeTimer;
	int m_timesToDisplayPrompt;
	int m_damage;
	float m_radius;
};

#endif