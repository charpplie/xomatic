/*
	SaveLoad Icon/Text hud object, Jan Mueller, 2010
*/

#ifndef __HUD_SAVELOADDISPLAY_H__
#define __HUD_SAVELOADDISPLAY_H__

#include "HUDObject.h"
#include "HUD/HUD.h"

//////////////////////////////////////////////////////////////////////////

class CHUD_SaveLoadDisplay : public CHUDObject
{
public :

	CHUD_SaveLoadDisplay();
	virtual ~CHUD_SaveLoadDisplay();

	//CHUDObject
	virtual void Update	(float frameTime);
	virtual void Draw		( void );
	void Init           ( void );
	void OnHUDEvent			( const SHUDEvent& event );
	void PreDelete      ( void );
	//~CHUDObject

private :

	float	m_fSaveLoadTimer;
};

#endif //__HUD_SAVELOADDISPLAY_H__

