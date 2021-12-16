#ifndef __HUDLOOKATUPDATER_H__
#define __HUDLOOKATUPDATER_H__

#include "Player.h"
#include "HUDObject.h"


//////////////////////////////////////////////////////////////////////////


class CHUD_LookAtUpdater : public CHUDObject
{

public:

	CHUD_LookAtUpdater();
	virtual ~CHUD_LookAtUpdater();

	virtual void Update	(float frameTime);

private:

	CDeferredRaycastHelper m_raycastHelper;

	EntityId			m_lookAtEntity;
	EntityId			m_usableEntity;
	int						m_viewDistance;
	float					m_spottingTimer;

	CPlayer::EInteractionType m_interactionType;

};


//////////////////////////////////////////////////////////////////////////

#endif

