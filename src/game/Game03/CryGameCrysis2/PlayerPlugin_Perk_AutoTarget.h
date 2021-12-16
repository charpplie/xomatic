#ifndef __PLAYERPLUGIN_PERK_AUTOTARGET_H__
#define __PLAYERPLUGIN_PERK_AUTOTARGET_H__

#include "IPerk.h"
#include "TeamPerks.h"

struct SAutoTargetBestVictim;
struct SMovementState;

class CPlayerPlugin_Perk_AutoTarget : public IPerk
{
	public:
	SET_PLAYER_PLUGIN_NAME(CPlayerPlugin_Perk_AutoTarget);

	CPlayerPlugin_Perk_AutoTarget();

	virtual void HandleEvent(EPlayerPlugInEvent perkEvent, void* data);
	virtual void Update(const float dt);
	virtual void Enter();
	virtual void Leave();

	private:
	EntityId GetBestAutoDirection(const SMovementState * state, SAutoTargetBestVictim & bestOut);

	bool m_ironSightOn;
	bool m_disableIcon;
	bool m_iconIsLit;
};

#endif	// __PLAYERPLUGIN_PERK_AUTOTARGET_H__