/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Script Binding for MusicLogic

-------------------------------------------------------------------------
History:
- 28:08:2006 : Created by TomasN
*************************************************************************/
#ifndef __SCRIPTBIND_MUSICLOGIC_H__
#define __SCRIPTBIND_MUSICLOGIC_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IScriptSystem.h>
#include <ScriptHelpers.h>


struct ISystem;
class CMusicLogic;

class CScriptBind_MusicLogic : public CScriptableBase
{
public:
	CScriptBind_MusicLogic(CMusicLogic *pMusicLogic);
	virtual ~CScriptBind_MusicLogic();

protected:
	//int SetMusicState(IFunctionHandler *pH, float fIntensity, float fBoredom);

	// <title SetEvent>
	// Syntax: MusicLogic.SetEvent( int eMusicEvent )
	// Arguments:
	//		eMusicEvent - One of this values:
	//				eMUSICLOGICEVENT_VEHICLE_ENTER
	//				eMUSICLOGICEVENT_VEHICLE_LEAVE
	//				eMUSICLOGICEVENT_WEAPON_MOUNT
	//				eMUSICLOGICEVENT_WEAPON_UNMOUNT
	//				eMUSICLOGICEVENT_ENEMY_SPOTTED
	//				eMUSICLOGICEVENT_ENEMY_KILLED
	//				eMUSICLOGICEVENT_ENEMY_HEADSHOT
	//				eMUSICLOGICEVENT_ENEMY_OVERRUN
	//				eMUSICLOGICEVENT_PLAYER_WOUNDED
	//				eMUSICLOGICEVENT_MAX
	// Description:
	//		Sets an event in the music logic.
	int SetEvent(IFunctionHandler *pH, int eMusicEvent);
	// <title StartLogic>
	// Syntax: MusicLogic.StartLogic()
	// Description:
	//		Starts the music logic.
	int StartLogic(IFunctionHandler *pH);
	// <title StopLogic>
	// Syntax: MusicLogic.StopLogic()
	// Description:
	//		Stops the music logic.
	int StopLogic(IFunctionHandler *pH);
	//int PauseGame(IFunctionHandler *pH, bool pause);

private:
	void RegisterGlobals();
	void RegisterMethods();

	ISystem						*m_pSystem;
	IScriptSystem			*m_pSS;
	CMusicLogic       *m_pMusicLogic;
};

#endif //__SCRIPTBIND_GAME_H__