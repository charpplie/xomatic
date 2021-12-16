#ifndef __HUDNAVPATH_H__
#define __HUDNAVPATH_H__

#include "HUDObject.h"


//////////////////////////////////////////////////////////////////////////


class CHUDMissionObjective;


//////////////////////////////////////////////////////////////////////////


class CHUD_NavPath : public CHUDObject
{

public:

	CHUD_NavPath();
	virtual ~CHUD_NavPath();

	virtual void		Update	(float frameTime);
	virtual void		Draw	();

	virtual void		OnHUDEvent( const SHUDEvent& event );

private:

	void ObjectiveChanged(CHUDMissionObjective* pObjective);

	bool m_enabled;
};


//////////////////////////////////////////////////////////////////////////

#endif

