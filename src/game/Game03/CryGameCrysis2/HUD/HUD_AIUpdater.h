#ifndef __HUDAIUPDATER_H__
#define __HUDAIUPDATER_H__

#include "HUDObject.h"


//////////////////////////////////////////////////////////////////////////


class CHUD_AIUpdater : public CHUDObject
{

public:

	CHUD_AIUpdater();
	virtual ~CHUD_AIUpdater();

	virtual void Update	(float frameTime);

private:

	int	m_currentAwarenessLevel;
};


//////////////////////////////////////////////////////////////////////////

#endif

