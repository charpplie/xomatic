#ifndef __FLASHFRONTEND_H__
#define __FLASHFRONTEND_H__


#include "IGameFramework.h"
#include "IHardwareMouse.h"
#include "ILevelSystem.h"
#include "IInput.h"
#include "Audio/AudioSignalPlayer.h"
#include "IFlashPlayer.h"
#include "FrontEnd/FrontEndParser.h"
#include "FrontEnd/FrontEndAudio.h"
#include "WarningsManager.h"

//////////////////////////////////////////////////////////////////////////

#define FRONTEND_SUBSCREEN_PATH "_root.SubScreen."
#define FRONTEND_SUBSCREEN_PATH_SET(a) "_root.SubScreen." a

class CMPMenuHub;

class CFlashFrontEnd	: public IGameFrameworkListener
											, IHardwareMouseEventListener
											, IInputEventListener
											, ILevelSystemListener
											, IFSCommandHandler
											, IHUDWarningsListener
											, ISystemEventListener
{

public:

	enum EFlashMenus
	{
		eFlM_NotSpecified,
		eFlM_GameSelect,
		eFlM_Menu,
		eFlM_IngameMenu,
		eFlM_LoadingScreen,
		eFlM_ResetScreen,
	};

	enum EFlashFrontEndCommand
	{
		eFECMD_none,
		eFECMD_command,
		eFECMD_goto,
		eFECMD_exit,
		eFECMD_back,
		eFECMD_load,
		eFECMD_resume,
		eFECMD_restart,
		eFECMD_back_to_mainmenu,
		eFECMD_quit,
		eFECMD_suicide,
		eFECMD_switch_game,
		eFECMD_set_highlight,
		eFECMD_verify_profile,
	};

	// IGameFrameworkListener
	virtual void OnActionEvent(const SActionEvent& event);
	virtual void OnLevelEnd(const char* pNextLevel);
	virtual void OnPostUpdate(float frameTime);
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	// ~IGameFrameworkListener

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);
	virtual bool OnInputEventUI(const SInputEvent &rInputEvent) { return false; }
	// ~IInputEventListener

	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName) {}
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error);
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount);
		//~ILevelSystemListener

	// IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0);
	// ~IHardwareMouseEventListener

	// IFSCommandHandler
	virtual void HandleFSCommand(const char* pCommand, const char* pArgs, void* pUserData = 0);
	// ~IFSCommandHandler

	// IHUDWarningsListener
	virtual bool OnWarningReturn(THUDWarningId id, const char* returnValue);
	virtual void OnWarningRemoved(THUDWarningId id) {}
	// ~IHUDWarningsListener

	// ISystemEventListener
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	// ~ISystemEventListener

private:

	typedef std::map<uint32, EFlashFrontEndCommand> TCommandMap;
	typedef std::vector<int> TScreenStack;

public:

						CFlashFrontEnd();
	virtual ~	CFlashFrontEnd();

	void ScheduleInitialize(EFlashMenus menu);
	void Initialize(EFlashMenus menu = eFlM_NotSpecified);
	void Clear();
	void Execute(EFlashFrontEndCommand command, const char* param);

	bool IsMenuLoaded() const { return m_pFlashUI!=NULL; }
	bool IsMenuActive(EFlashMenus menu) const { return ((m_pFlashUI!=NULL) && (m_currentMenu==menu) && (m_scheduleClear==false)); }
	bool IsGameActive() const;
	
	IFlashPlayer* GetFlash() { return m_pFlashUI; }
	CMPMenuHub*		GetMPMenu() { return m_mpMenuHub; }

	void InitIngameMenu();
	void DestroyIngameMenu();

private:

	void InitFrontEndMenu();
	void DestroyFrontEndMenu();
	void ShowLoadingScreen();

	void RegisterCommand(EFlashFrontEndCommand command, const char *name);
	EFlashFrontEndCommand GetCommand(const char *name);

	void CorrectFrameTime(float &frameTime);
	void WorkSchedules();
	void UpdateResolution(const int width, const int height);
	void LoadBackground();

	void ScreenForward(const char* name);
	void ScreenBack();
	void SetScreen(const int index);
	void ClearScreen();

	void EnterScreen(const EFlashFrontEndScreens screen);
	void LeaveScreen(const EFlashFrontEndScreens screen);

	void FromGame();
	void ToGame();

	void ResetBegin();
	void ResetEnd();
	void ResetProgress(int progress);

#if !defined(_RELEASE)
	void DebugWatches();
#endif

	CMenuData				m_menuData;

	CryFixedStringT<64>		m_nextLoadLevel;

	TCommandMap			m_commandMap;
	TScreenStack		m_screenStack;

	CFrontEndParser	m_parser;
	CFrontEndAudio	m_audio;

	// Key repeat
	SInputEvent			m_repeatEvent;

	Vec2i						m_currentResolution;

	IFlashPlayer*		m_pFlashUI;
	CMPMenuHub*			m_mpMenuHub;
	ITexture*				m_backgroundTexture;


	float						m_repeatTimer;
	float						m_lastUpdateTime;
	int							m_maxLevelProgress;
	int							m_currentScreenIndex;

	EFlashMenus			m_currentMenu;
	EFlashFrontEndScreens m_currentScreen;

	bool						m_scheduleClear;
	bool						m_scheduleInitialize;
	bool						m_scheduleReload;
	bool						m_scheduleQuit;
	bool						m_scheduleSwitchGame;
	bool						m_switchGameMultiplayer;

	bool						m_isFirstRun;
	bool						m_isReloading;

	bool						m_frontEndInited;
	bool						m_ingameInited;
	bool						m_hudHidePS3HackErmWorkaround;
};


//////////////////////////////////////////////////////////////////////////

#endif
