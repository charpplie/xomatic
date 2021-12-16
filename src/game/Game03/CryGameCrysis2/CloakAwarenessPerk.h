#ifndef __CLOAKAWARENESSPERK_H__
#define __CLOAKAWARENESSPERK_H__

#include "IPerk.h"

class CloakAwarenessPerk:
	public IPerk
{
public:
	SET_PLAYER_PLUGIN_NAME(CloakAwarenessPerk);

	CloakAwarenessPerk(const float* range, const float* timeTillFlicker1, const float* timeTillFlicker2, const float* timeTillFlicker3, const float* flickerTime);
	virtual ~CloakAwarenessPerk() {}

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);

	virtual void Update(const float dt);

protected:
	bool ShouldGenerateParticles(CPlayer* pPlayer);
	void GenerateParticles(CPlayer* pPlayer, int amount);

	const float* m_timeTillFlicker[eTierMax];
	const float* m_range;
	const float* m_suitFlickerTime;
	float m_flickerTimer;
};

#endif
