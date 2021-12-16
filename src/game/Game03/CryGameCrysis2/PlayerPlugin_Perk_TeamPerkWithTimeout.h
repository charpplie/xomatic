#ifndef __PLAYERPLUGIN_PERK_TEAMPERKWITHTIMEOUT_H__
#define __PLAYERPLUGIN_PERK_TEAMPERKWITHTIMEOUT_H__

#include "IPerk.h"
#include "TeamPerks.h"

class CPlayerPlugin_Perk_TeamPerkWithTimeout : public IPerk
{
	public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Perk_TeamPerkWithTimeout);

	CPlayerPlugin_Perk_TeamPerkWithTimeout(ETeamPerks teamPerkID, float timeOut, const char* startSignal, const char* stopSignal = NULL, const char* loopSignal = NULL);

	virtual void NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags);
	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual void Update(const float dt);
	virtual void Enter();
	virtual void Leave();
	void Activate();

	protected:
	const ETeamPerks m_teamPerkID;
	const float m_duration;
	TAudioSignalID m_startSignalID;
	TAudioSignalID m_stopSignalID;
	CAudioSignalPlayer m_loopSignal;

	float m_timeAlive;
	int m_activatedPerkForTeam;
	bool m_active;

	void ActivatePerkForTeam(int team, EntityId activatorId);
};

#endif	// __PLAYERPLUGIN_PERK_TEAMPERKWITHTIMEOUT_H__