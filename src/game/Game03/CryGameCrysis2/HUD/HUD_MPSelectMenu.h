#ifndef __HUD_MP_SELECT_MENU_H__
#define __HUD_MP_SELECT_MENU_H__

#include <ILevelSystem.h>
#include "HUDObject.h"

#include "IInput.h"
#include "IFlashPlayer.h"
#include "IHardwareMouse.h"
#include "Menus/FlashMenuButtonHandler.h"

//-----------------------------------------------------------------------------------------------------

struct IFlashPlayer;

class CHUD_MPSelectMenu : public CHUDObject, IHardwareMouseEventListener, IInputEventListener, IFSCommandHandler, ILevelSystemListener
{
private:
	typedef CryFixedStringT<32> TFixedString32;
	typedef CryFixedStringT<16> TFixedString16;

	struct SHostMigratingPlayerInfo
	{
		TFixedString32 m_playerName;
		bool m_hasMigrated;
	};
	typedef std::vector<SHostMigratingPlayerInfo> THostMigratingInfoVec;

	enum EMPSelectMenuState
	{
		eMS_None = 0,
		eMS_Scoreboard,
		eMS_Map,
		eMS_Loadout,
		eMS_Stats,
	};

	enum EOpenMenu
	{
		eOM_Main = 0,
		eOM_EndScoreboard,
		eOM_Map,
		eOM_HostMigration,
	};

public:

	CHUD_MPSelectMenu();
	virtual ~CHUD_MPSelectMenu();

	void Init();
	void Update(float frameTime);
	void Draw();

	void OnHUDEvent(const SHUDEvent& event);

	// IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0);
	// ~IHardwareMouseEventListener

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);
	virtual bool OnInputEventUI(const SInputEvent &rInputEvent);
	// ~IInputEventListener

	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName) {};
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel) {};
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error) {};
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) {};
	//~ILevelSystemListener

	// IFSCommandHandler
	void HandleFSCommand(const char *strCommand,const char *strArgs,void* pUserData = 0);
	// ~IFSCommandHandler

private:
	void SendEndStatsToFlash();

	void ShowMenu(bool onOff, bool force=false, EOpenMenu menu=eOM_Main);
	void ChangeMenuState(EMPSelectMenuState newState);
	void EnterMenuState(EMPSelectMenuState state);
	void LeaveMenuState(EMPSelectMenuState state);

	void InitHostMigration();
	void UpdateHostMigrationState();
	void FinishHostMigration();
	void HostMigrationOnNewPlayer(EntityId playerId);

	THostMigratingInfoVec m_hostMigratingPlayerInfoVec;

	bool m_bVisible;
	bool m_bGameEnded;
	EMPSelectMenuState m_state;

	CFlashMenuButtonHandler *m_pFlashButtonHandler;

	TFixedString16 m_tempString;

	TFixedString16 m_gameModeString;
	TFixedString16 m_levelNameString;

	bool m_bHostMigrating;
	bool m_bHostMigratingNeedsUpdate;
};


#endif // ~__HUD_MP_SELECT_MENU_H__

