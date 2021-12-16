#ifndef __HUDSCANNEDENEMIESUPDATER_H__
#define __HUDSCANNEDENEMIESUPDATER_H__

#include "HUDObject.h"
#include "NanoSuitDefs.h"


//////////////////////////////////////////////////////////////////////////


class CHUD_ScannedEnemiesUpdater : public CHUDObject
{

public:

	CHUD_ScannedEnemiesUpdater();
	virtual ~CHUD_ScannedEnemiesUpdater();

	virtual void		Update	(float frameTime);
	virtual void		Draw	();

	virtual void		OnHUDEvent( const SHUDEvent& event );

private:

	void						RemoveAllEnemies();
	void						AddAllEnemies();
	void						AddEnemy(EntityId entity);

	void						SetSuitMode(ENanoSuitMode mode);

	ENanoSuitMode		m_suitMode;

};


//////////////////////////////////////////////////////////////////////////

#endif

