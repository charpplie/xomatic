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
class CHUD;

class CScriptBind_HUD :
	public CScriptableBase
{
public:
	CScriptBind_HUD(ISystem *pSystem, IGameFramework *pGameFramework);
	virtual ~CScriptBind_HUD();

protected:
	// <title SetObjectiveStatus>
	// Syntax: HUD.SetObjectiveStatus( const char* pObjectiveID, int status, bool silent )
	// Arguments:
	//		pObjectiveID - Objective identifier.
	//		status		 - Objective status.
	//		silent		 - True if silent, false otherwise.
	//	Description:
	//		Sets the specified objective status.
	virtual int SetObjectiveStatus(IFunctionHandler *pH,const char* pObjectiveID, int status, bool silent);
	// <title GetObjectiveStatus>
	// Syntax: HUD.GetObjectiveStatus( const char* pObjectiveID )
	// Arguments:
	//		pObjectiveID - Objective identifier.
	// Description:
	//		Gets the specified objective status.
	virtual int GetObjectiveStatus(IFunctionHandler* pH,const char* pObjectiveID);
	// <title SetMainObjective>
	// Syntax: HUD.SetMainObjective( const char* pObjectiveID )
	// Arguments:
	//		pObjectiveID - Objective identifier.
	// Description:
	//		Sets the specified objective as the main objective.
	virtual int SetMainObjective(IFunctionHandler* pH, const char* pObjectiveID);
	// <title GetMainObjective>
	// Syntax: HUD.GetMainObjective()
	// Description:
	//		Gets the main objective.
	virtual int GetMainObjective(IFunctionHandler* pH);
	// <title SetObjectiveEntity>
	// Syntax: HUD.SetObjectiveEntity( const char* pObjectiveID, ScriptHandle entityID )
	// Arguments:
	//		pObjectiveId - Objective identifier.
	//		entityID	 - Entity identifier.
	// Description:
	//		Sets an objective entity.
	virtual int SetObjectiveEntity(IFunctionHandler *pH,const char* pObjectiveID, ScriptHandle entityID);
	// <title StewUsability>
	// Syntax: HUD.SetUsability( int objId, const char* pMessage )
	// Arguments:
	//		objId - Object identifier.
	//		pMessage - Message.
	// Description:
	//		Sets the usability message for the specified object.
	virtual int SetUsability(IFunctionHandler *pH, int objId, const char* pMessage);
	// <title DrawStatusText>
	// Syntax: HUD.DrawStatusText( const char* pText )
	// Arguments:
	//		pText - Status message.
	// Description:
	//		Draws the status text.
	virtual int DrawStatusText(IFunctionHandler *pH, const char* pText);
	// <title ReloadLevel>
	// Syntax: HUD.ReloadLevel()
	// Description:
	//		Reloads the current level.
	virtual int ReloadLevel(IFunctionHandler *pH);
	// <title ReloadLevelSavegame>
	// Syntax: HUD.ReloadLevelSavegame()
	// Description:
	//		Reloads the level saved game.
	virtual int ReloadLevelSavegame(IFunctionHandler *pH);
	// <title TacWarning>
	// Syntax: HUD.TacWarning(  )
	// Description:
	//		Plays the "tac" warning sound alarm.
	virtual int TacWarning(IFunctionHandler *pH);
	// <title DamageIndicator>
	// Syntax: HUD.DamageIndicator( ScriptHandle weaponId, ScriptHandle shooterId, Vec3 direction, bool onVehicle )
	// Arguments:
	//		weaponId	- Weapon identifier.
	//		shooterId	- Shooter identifier.
	//		direction	- Direction vector.
	//		onVehicle	- True if the damage is done to a character on a vehicle, false otherwise.
	// Description:
	//		Indicates the damage.
	virtual int DamageIndicator(IFunctionHandler *pH, ScriptHandle weaponId, ScriptHandle shooterId, Vec3 direction, bool onVehicle);
	// <title HitIndicator>
	// Syntax: HUD.HitIndicator()
	// Description:
	//		Indicates the hit.
	virtual int HitIndicator(IFunctionHandler *pH);
	// <title EnteredBuyZone>
	// Syntax: HUD.EnteredBuyZone()
	// Description:
	//		Calls the hud, tell him we entered a buy zone.
	//		The hud itself needs to keep track of which team owns the zone and enable/disable buying accordingly.
	virtual int EnteredBuyZone(IFunctionHandler *pH, ScriptHandle zoneId, bool entered);
	// <title EnteredServiceZone>
	// Syntax: HUD.EnteredServiceZone()
	// Description:
	//		Call the hud, tell him we entered a service zone.
	//		The hud itself needs to keep track of which team owns the zone and enable/disable services accordingly.
	virtual int EnteredServiceZone(IFunctionHandler *pH, ScriptHandle zoneId, bool entered);
	// <title UpdateBuyList>
	// Syntax: HUD.UpdateBuyList()
	// Description:
	//		Updates the buying list.
	virtual int UpdateBuyList(IFunctionHandler *pH);
	// <title RadarShowVehicleReady>
	// Syntax: HUD.RadarShowVehicleReady( ScriptHandle vehicleId )
	// Arguments:
	//		vehicleId - Vehicle identifier.
	// Description:
	//		Shows the vehicle in the radar as ready.
	virtual int RadarShowVehicleReady(IFunctionHandler *pH, ScriptHandle vehicleId);
	// <title AddEntityToRadar>
	// Syntax: HUD.AddEntityToRadar( ScriptHandle entityId )
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Shows the specified entity inside the radar.
	virtual int AddEntityToRadar(IFunctionHandler *pH, ScriptHandle entityId);
	// <title RemoveEntityFromRadar>
	// Syntax: HUD.RemoveEntityFromRadar( ScriptHandle entityId )
	// Arguments:
	//		entityId - Entity identifier.
	// Description:
	//		Removes the specified entity from the radar.
	virtual int RemoveEntityFromRadar(IFunctionHandler *pH, ScriptHandle entityId);
	// <title ShowKillZoneTime>
	// Syntax: HUD.ShowKillZoneTime(  )
	// Arguments:
	//		active	- True if the kill zone time is active, false otherwise.
	//		seconds - Countdown in seconds for the kill zone time.
	// Description:
	//		Show the countdown for the kill zone time.
	virtual int ShowKillZoneTime(IFunctionHandler *pH, bool active, int seconds);
	// <title StartPlayerFallAndPlay>
	// Syntax: HUD.StartPlayerFallAndPlay()
	// Description:
	//		When the player falls the timer is started.
	virtual int StartPlayerFallAndPlay(IFunctionHandler *pH);
	// <title OnPlayerVehicleBuilt>
	// Syntax: HUD.OnPlayerVehicleBuilt( ScriptHandle playerId, ScriptHandle vehicleId )
	// Arguments:
	//		playerId	- Player identifier.
	//		vehicleId	- Vehicle identifier.
	// Description:
	//		Event when the vehicle the player bought is built.
	virtual int OnPlayerVehicleBuilt(IFunctionHandler *pH, ScriptHandle playerId, ScriptHandle vehicleId);
	// <title OnItemBought>
	// Syntax: HUD.OnItemBought()
	// Arguments:
	//		success	 - True if the item was successfully bought, false otherwise.
	//		itemName - Name of the item bought.
	// Description:
	//		Event happening when an item is bought.
	virtual int OnItemBought(IFunctionHandler *pH, bool success, const char* itemName);
	// <title ShowDeathFX>
	// Syntax: HUD.ShowDeathFX( int type )
	// Arguments:
	//		type - Effect type.
	// Description:
	//		Show a death effect.
	virtual int ShowDeathFX(IFunctionHandler *pH, int type);
	// <title BattleLogEvent>
	// Syntax: HUD.BattleLogEvent( int type, const char *msg )
	// Arguments:
	//		type - .
	//		msg - .
	// Description:
	//		Event on logging battle information.
	virtual int BattleLogEvent(IFunctionHandler *pH, int type, const char *msg);
	// <title FakeDeath>
	// Syntax: HUD.FakeDeath()
	virtual int FakeDeath(IFunctionHandler *pH);
	// <title ShowWarningMessage>
	// Syntax: HUD.ShowWarningMessage( int warning, const char* text = 0 )
	// Arguments:
	//		warning - One of this value:
	//					EHUD_SPECTATOR,
	//					EHUD_SWITCHTOTAN,
	//					EHUD_SWITCHTOBLACK,
	//					EHUD_SUICIDE,
	//					EHUD_CONNECTION_LOST,
	//					EHUD_OK,
	//					EHUD_YESNO,
	//					EHUD_CANCEL.
	//		text - Warning message.
	// Description:
	//		Shows a warning message.
	virtual int ShowWarningMessage(IFunctionHandler *pH, int warning, const char* text = 0);
	// <title GetMapGridCoord>
	// Syntax: HUD.GetMapGridCoord()
	// Arguments:
	//		x - X value.
	//		y - Y value.
	// Description:
	//		Gets map grid coordinates.
	virtual int GetMapGridCoord(IFunctionHandler *pH, float x, float y);
	// <title OpenPDA>
	// Syntax: HUD.OpenPDA( bool show, bool buyMenu )
	// Arguments:
	//		show	- True to show the pda, false to hide.
	//		buyMenu - True to show the buy menu, false otherwise.
	// Description:
	//		Opens/closes the PDA.
	virtual int OpenPDA(IFunctionHandler *pH, bool show, bool buyMenu);

	// <title ShowCaptureProgress>
	// Syntax: HUD.ShowCaptureProgress( bool show )
	// Arguments:
	//		show - True to show the capture progress, false to hide.
	// Description:
	//		Shows/hides capture progress.
	virtual int ShowCaptureProgress(IFunctionHandler *pH, bool show);
	// <title SetCaptureProgress>
	// Syntax: HUD.SetCaptureProgress( float progress )
	// Arguments:
	//		progress - Amount of progress.
	// Description:
	//		Sets the capture progress value.
	virtual int SetCaptureProgress(IFunctionHandler *pH, float progress);
	// <title SetCaptureContested>
	// Syntax: HUD.SetCaptureContested( bool contested )
	// Arguments:
	//		contested - True if the capture is contested, false otherwise.
	// Description:
	//		Sets if the capture is contested.
	virtual int SetCaptureContested(IFunctionHandler *pH, bool contested);
	// <title ShowConstructionProgress>
	// Syntax: HUD.ShowConstructionProgress( bool show, bool queued, float constructionTime )
	// Arguments:
	//		show	- True to show construction progress, false to hide.
	//		queued	- True if the construction is queued, false otherwise.
	//		constructionTime - Contruction time.
	// Description:
	//		Show construction progress.
	virtual int ShowConstructionProgress(IFunctionHandler *pH, bool show, bool queued, float constructionTime);
	// <title ShowReviveCycle>
	// Syntax: HUD.ShowReviveCycle( bool show )
	// Arguments:
	//		show	- True to show revive cycle, false to hide.
	// Description:
	//		Shows the revive cycle.
	virtual int ShowReviveCycle(IFunctionHandler *pH, bool show);
	// <title SpawnGroupInvalid>
	// Syntax: HUD.SpawnGroupInvalid()
	// Description:
	//		Spawns group invalid.
	virtual int SpawnGroupInvalid(IFunctionHandler *pH);

	// <title SetProgressBar>
	// Syntax: HUD.SetProgressBar( bool show, int percent, const char *text )
	// Arguments:
	//		show	- True to show the progress bar, false to hide.
	//		percent - Progress percentage.
	//		text	- Text message.
	// Description:
	//		Sets the progress bar values.
	virtual int SetProgressBar(IFunctionHandler *pH, bool show, int percent, const char *text);
	// <title DisplayBigOverlayFlashMessage>
	// Syntax: HUD.DisplayBigOverlayFlashMessage( const char *msg, float duration, int posX, int posY, Vec3 color )
	// Arguments:
	//		msg			- Message text.
	//		duration	- Duration of the flash message.
	//		posX		- X position for the message.
	//		posY		- Y position for the message.
	//		color		- Color of the message.
	// Description:
	//		Displays a big overlay flash message.
	virtual int DisplayBigOverlayFlashMessage(IFunctionHandler *pH, const char *msg, float duration, int posX, int posY, Vec3 color);
	// <title FadeOutBigOverlayFlashMessage>
	// Syntax: HUD.FadeOutBigOverlayFlashMessage()
	// Description:
	//		Fades out the big overlay flash message.
	virtual int FadeOutBigOverlayFlashMessage(IFunctionHandler *pH);
	// <title GetLastInGameSave>
	// Syntax: HUD.GetLastInGameSave()
	// Description:
	//		Gets the last in game saved game.
	virtual int GetLastInGameSave(IFunctionHandler *pH);

private:
	void RegisterGlobals();
	void RegisterMethods();

	ISystem						*m_pSystem;
	IScriptSystem			*m_pSS;
	IGameFramework		*m_pGameFW;
};

#endif //__SCRIPTBIND_HUD_H__