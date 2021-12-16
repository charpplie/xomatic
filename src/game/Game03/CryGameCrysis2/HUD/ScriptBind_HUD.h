/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2006.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Script Binding for HUD
  
 -------------------------------------------------------------------------
  History:
  - 14:02:2006   11:30 : Created by AlexL
	- 04:04:2006	 17:30 : Extended by Jan Müller

*************************************************************************/
#ifndef __SCRIPTBIND_HUD_H__
#define __SCRIPTBIND_HUD_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

struct IItemSystem;
struct IGameFramework;

class CScriptBind_HUD :
	public CScriptableBase
{
public:
	CScriptBind_HUD(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_HUD();

	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

protected:
	virtual int SetObjectiveStatus(IFunctionHandler *pH,const char* pObjectiveID, int status, bool silent);
	virtual int GetObjectiveStatus(IFunctionHandler* pH,const char* pObjectiveID);
	virtual int SetMainObjective(IFunctionHandler* pH, const char* pObjectiveID);
	virtual int GetMainObjective(IFunctionHandler* pH);
	virtual int SetObjectiveEntity(IFunctionHandler *pH,const char* pObjectiveID, ScriptHandle entityID);
	virtual int SetUsability(IFunctionHandler *pH, int objId, const char* pMessage);
	virtual int DrawStatusText(IFunctionHandler *pH, const char* pText);
	virtual int ReloadLevel(IFunctionHandler *pH);
	virtual int ReloadLevelSavegame(IFunctionHandler *pH);
	virtual int TacWarning(IFunctionHandler *pH);
	virtual int DamageIndicator(IFunctionHandler *pH, ScriptHandle weaponId, ScriptHandle shooterId, Vec3 direction, bool onVehicle);
	virtual int HitIndicator(IFunctionHandler *pH);
	virtual int EnteredBuyZone(IFunctionHandler *pH, ScriptHandle zoneId, bool entered);
	virtual int EnteredServiceZone(IFunctionHandler *pH, ScriptHandle zoneId, bool entered);
	virtual int UpdateBuyList(IFunctionHandler *pH);
	virtual int RadarShowVehicleReady(IFunctionHandler *pH, ScriptHandle vehicleId);
	virtual int AddEntityToRadar(IFunctionHandler *pH, ScriptHandle entityId);
	virtual int AddEntityToRadarAsBinocularTag(IFunctionHandler *pH, ScriptHandle shooterId, ScriptHandle targetId);
	virtual int UpdateFlagState(IFunctionHandler *pH, ScriptHandle flag1_id, ScriptHandle base1_id, int state1, /*Vec3 pos1, float alpha1,*/ ScriptHandle carrierId1,  
	                                                  ScriptHandle flag2_id, ScriptHandle base2_id, int state2/*, Vec3 pos2, float alpha2*/, ScriptHandle carrierId2 );

	virtual int RemoveEntityFromRadar(IFunctionHandler *pH, ScriptHandle entityId);
	virtual int ShowKillZoneTime(IFunctionHandler *pH, bool active, int seconds);
	virtual int StartPlayerFallAndPlay(IFunctionHandler *pH);
	virtual int OnPlayerVehicleBuilt(IFunctionHandler *pH, ScriptHandle playerId, ScriptHandle vehicleId);
	virtual int OnItemBought(IFunctionHandler *pH, bool success, const char* itemName);
	virtual int ShowDeathFX(IFunctionHandler *pH, int type);
	virtual int BattleLogEvent(IFunctionHandler *pH, int type, const char *msg);
	virtual int FakeDeath(IFunctionHandler *pH);
	virtual int ShowWarningMessage(IFunctionHandler *pH, int warning, const char* text = 0);
	virtual int GetMapGridCoord(IFunctionHandler *pH, float x, float y);
	virtual int OpenPDA(IFunctionHandler *pH, bool show, bool buyMenu);

	virtual int SetCurrentCaptureSite(IFunctionHandler *pH, ScriptHandle site);
	virtual int ShowCaptureProgress(IFunctionHandler *pH, bool show);
	virtual int SetCaptureProgress(IFunctionHandler *pH, float progress);
	virtual int SetCaptureContested(IFunctionHandler *pH, bool contested);
	virtual int ShowConstructionProgress(IFunctionHandler *pH, bool show, bool queued, float constructionTime);
	virtual int ShowReviveCycle(IFunctionHandler *pH, bool show);
	virtual int SpawnGroupInvalid(IFunctionHandler *pH);

	virtual int SetProgressBar(IFunctionHandler *pH, bool show, int percent, const char *text);
	virtual int DisplayBigOverlayFlashMessage(IFunctionHandler *pH, const char *msg, float duration, int posX, int posY, Vec3 color);
	virtual int FadeOutBigOverlayFlashMessage(IFunctionHandler *pH);
	virtual int DisplayFunMessagePostfix(IFunctionHandler *pH, const char* msg, const char* postfix);
	virtual int DisplayBonusMessage(IFunctionHandler *pH, const char* msg, int teamId);
	virtual int GetLastInGameSave(IFunctionHandler *pH);
	virtual int TextMessage(IFunctionHandler *pH, int type, float lifetime, const char *text);

	virtual int AddEntitySilhouette(IFunctionHandler *pH, ScriptHandle entityId, float r, float g, float b, float a);
	virtual int RemoveEntitySilhouette(IFunctionHandler *pH, ScriptHandle entityId);
	virtual int RequestLoadout(IFunctionHandler *pH);
	virtual int DisplayFunMessage(IFunctionHandler *pH, const char* msg);

	virtual int ClearPrototypeWeapons(IFunctionHandler *pH);
	virtual int AddPrototypeWeapon(IFunctionHandler *pH, ScriptHandle entityId);
	virtual int RemovePrototypeWeapon(IFunctionHandler *pH, ScriptHandle entityId);

	virtual int ClearActivePayloads(IFunctionHandler *pH);
	virtual int AddActivePayload(IFunctionHandler *pH, ScriptHandle entityId);
	virtual int RemoveActivePayload(IFunctionHandler *pH, ScriptHandle entityId);
	virtual int SetPayloadBank(IFunctionHandler *pH, ScriptHandle entityId);

	virtual int NewMissionObjective(IFunctionHandler *pH, ScriptHandle targetId, int objectiveType, float progress, int priority);

	virtual int GetLookAtEntity(IFunctionHandler *pH);

	int SetCaptureBar(IFunctionHandler *pH, float progress);
	int TriggerCaptureBarWarning(IFunctionHandler *pH);

private:
	void RegisterGlobals();
	void RegisterMethods();

	ISystem						*m_pSystem;
	IScriptSystem			*m_pSS;
	IGameFramework		*m_pGameFW;
};

#endif //__SCRIPTBIND_HUD_H__
