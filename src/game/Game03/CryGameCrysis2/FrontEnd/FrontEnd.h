#ifndef __FRONTEND_H__
#define __FRONTEND_H__

//////////////////////////////////////////////////////////////////////////


#include "IGameFramework.h"
#include "IHardwareMouse.h"
#include "ILevelSystem.h"
#include "Audio/AudioSignalPlayer.h"

//////////////////////////////////////////////////////////////////////////


struct IMusicSystem;
class CProfileOptions;
class CFrontEndScreen;


//////////////////////////////////////////////////////////////////////////


template<class T, class U>
struct TFrontEndKeyValuePair
{
	T key;
	U value;
};

template<class T, class U, class V>
T GetKeyByValue(const V& value, TFrontEndKeyValuePair<T,U> list[], size_t list_size)
{
	for(int i=0;i<int(list_size);++i)
		if(value == list[i].value)
			return list[i].key;
	return list[0].key;
}

template<class T, class U, class V>
U GetValueByKey(V key, TFrontEndKeyValuePair<T,U> list[], size_t list_size)
{
	for(int i=0;i<list_size;++i)
		if(key == list[i].key)
			return list[i].value;
	return list[0].value;
}


//////////////////////////////////////////////////////////////////////////


enum EFrontEndCommand
{
	eFECMD_none,
	eFECMD_goto,
	eFECMD_goto_options,
	eFECMD_goto_exit,
	eFECMD_release_and_goto_main,
	eFECMD_new_game,
	eFECMD_volume,
	eFECMD_brightness,
	eFECMD_invert,
	eFECMD_vibration,
	eFECMD_god_mode,
	eFECMD_sensitivity,
	eFECMD_continue,
	eFECMD_restart,
	eFECMD_quit,
	eFECMD_returnToMain,
	eFECMD_exit,
	eFECMD_screen_back,
	eFECMD_load,
};

enum EFrontEndAlignment
{
	eFEA_Center,
	eFEA_Left,
	eFEA_Right
};

enum EFrontEndInputType
{
	eFEIT_Up,
	eFEIT_Down,
	eFEIT_Left,
	eFEIT_Right,
	eFEIT_Ok,
	eFEIT_Start,
	eFEIT_Back
};

enum EFrontEndInputState
{
	eFEIS_Press,
	eFEIS_Release,
};

enum EFrontEndSound
{
	eFES_Start,
	eFES_Confirm,
	eFES_Cancel,
	eFES_Move,
	eFES_Ambient,
	eFES_Last
};


//////////////////////////////////////////////////////////////////////////



class CFrontEnd : public IGameFrameworkListener, IHardwareMouseEventListener, IInputEventListener, ILevelSystemListener, ISystemEventListener
{
public:

	void GetMemoryStatistics( ICrySizer * );

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime);
	virtual void OnSaveGame(ISaveGame* pSaveGame);
	virtual void OnLoadGame(ILoadGame* pLoadGame);
	virtual void OnLevelEnd(const char* pNextLevel);
  virtual void OnActionEvent(const SActionEvent& event);
	// ~IGameFrameworkListener

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &rInputEvent);
	virtual bool OnInputEventUI(const SInputEvent &rInputEvent);
	// ~IInputEventListener

	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName);
	virtual void OnLoadingStart(ILevelInfo *pLevel);
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error);
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount);
		//~ILevelSystemListener

	// IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0);
	// ~IHardwareMouseEventListener

	// ISystemEventListener
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	// ~ISystemEventListener

	static CFrontEnd* Get()
	{
		if(!m_pInst)
			m_pInst = new CFrontEnd();
		return m_pInst;
	}

	static void Kill()
	{
		if(m_pInst)
			SAFE_DELETE(m_pInst);
		m_pInst = NULL;
	}

						CFrontEnd();
	virtual ~	CFrontEnd();

	void Initialize();
	void InitializeFromFile(const char* path = NULL);
	void Clear();

	// Automatically prepends asset name with cw2 in a multiplayer game 
	void InitializeAsset(const char* const path, const char* const asset );

	void SetActive(bool active) { m_isActive = active; }
	const bool IsActive() { return m_isActive; }

	bool IsGameActive();
	void ToGame();
	void FromGame();

	void SetFont(const char* font);
	void DrawFont(const char* text, float posX, float posY, float widthspan, float heightspan, int alignment = eFEA_Center, float fontSize = 24.0f, ColorF color = ColorF(1.0f));

	void DrawTexture(ITexture* texture, float posX, float posY, float width = -1.0f, float height = -1.0f, float opacity = 1.0f, float u0 = 0.0f, float w0 = 1.0f, float u1 = 1.0f, float w1 = 0.0f);
	void DrawLine(float x1, float y1, float x2, float y2);

	void PlaySound(EFrontEndSound eSound,bool bPlay);

	bool		Execute(string command, const char* param = NULL);
	bool		Execute(EFrontEndCommand command, const char* param = NULL);
	string	GetCurrentValue(string command);

	CFrontEndScreen* GetCurrentScreen();
	void SetCurrentScreen(int screenIndex, float updateFrameTime = 0.0f);

	void ScreenForward(const char* name, bool releaseOld = false);
	void ScreenBack();

	int	m_debugFrontEnd;

private:

	static CFrontEnd*	m_pInst;

	std::vector<CFrontEndScreen*> m_screens;
	int m_soundListener;
	int m_currentScreenIndex;
	std::vector<string> m_screenStack;
	IFFont *m_font;
	string m_currentXML;
	bool m_isActive;
	int	m_maxLevelProgress;
	int m_curLevelProgress;

	CryFixedStringT<64>				m_nextLoadLevel;

	bool m_clearOnUpdate;
	bool m_initializeOnUpdate;
	bool m_reloadOnUpdate;
	bool m_reloading;
	bool m_postLoadingProgress;
	CAudioSignalPlayer m_soundPlayers[eFES_Last];

	CProfileOptions* m_options;

};


//////////////////////////////////////////////////////////////////////////

#endif
