/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Script Binding for Game

-------------------------------------------------------------------------
History:
- 14:08:2006   11:30 : Created by AlexL
*************************************************************************/
#ifndef __SCRIPTBIND_GAME_H__
#define __SCRIPTBIND_GAME_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IGameFramework;
struct ISystem;

class CScriptBind_Game :
	public CScriptableBase
{
public:
	CScriptBind_Game(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_Game();

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

protected:
	int ShowMainMenu(IFunctionHandler *pH);
	int ShowInGameMenu(IFunctionHandler *pH);
	int PauseGame(IFunctionHandler *pH, bool pause);
	int PlayFlashAnim(IFunctionHandler *pH);
	int PlayVideo(IFunctionHandler *pH);
	//!	Queries battle status, range from 0 (quiet) to 1 (full combat)
	//int	QueryBattleStatus(IFunctionHandler *pH);
	int SetPerksAllowed(IFunctionHandler *pH, bool allowed);
	int GetPerkIndex(IFunctionHandler *pH, const char * name);
	int GivePlayerPerk(IFunctionHandler *pH, ScriptHandle playerId, const char *perkName, bool enabled);
	int ClearPlayersPerks(IFunctionHandler *pH, ScriptHandle playerId );
	int CountPlayersInVehicleWithPerk(IFunctionHandler * pH, ScriptHandle vehicleId, int perkIndex);
	int IsMountedWeaponUsableWithTarget(IFunctionHandler *pH);

	int IsPlayer(IFunctionHandler *pH, ScriptHandle entityId);

	int AddTacticalEntity(IFunctionHandler *pH, ScriptHandle entityId, int type);
	int RemoveTacticalEntity(IFunctionHandler *pH, ScriptHandle entityId, int type);

	//Checkpoint System
	int SaveCheckpoint(IFunctionHandler *pH, ScriptHandle checkpointId, const char *fileName);
	int	LoadCheckpoint(IFunctionHandler *pH, const char *fileName);
	int QuickLoad(IFunctionHandler *pH);

	int DebugDrawCylinder(IFunctionHandler *pH, float x, float y, float z, float radius, float height, int r, int g, int b, int a);

private:
	void RegisterGlobals();
	void RegisterMethods();

	ISystem						*m_pSystem;
	IScriptSystem			*m_pSS;
	IGameFramework		*m_pGameFW;
};

#endif //__SCRIPTBIND_GAME_H__
