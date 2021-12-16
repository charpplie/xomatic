#ifndef __FlashMainMenu_H__
#define __FlashMainMenu_H__

#include "IFlashPlayer.h"
#include "IHardwareMouse.h"
#include "ILevelSystem.h"
#include "IInput.h"

class CFlashMainMenu :	public IHardwareMouseEventListener,
						public IFSCommandHandler,
						public ILevelSystemListener
{
private:
	enum EScreenMode
	{
		eSM_None = 0,
		eSM_Menu,
		eSM_Loading,
	};
public:
	CFlashMainMenu();
	virtual ~CFlashMainMenu();

	void Draw(float fDeltaTime);

	// IHardwareMouseEventListener
	void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta );
	// ~IHardwareMouseEventListener

	// IFSCommandHandler
	void HandleFSCommand( const char *szCommand,const char *strArgs, void* pUserData );
	// ~IFSCommandHandler

	// ILevelSystemListener
	VIRTUAL void OnLevelNotFound(const char* levelName) {}
	VIRTUAL void OnLoadingStart(ILevelInfo* pLevel);
	VIRTUAL void OnLoadingComplete(ILevel* pLevel);
	VIRTUAL void OnLoadingError(ILevelInfo* pLevel, const char* error) {}
	VIRTUAL void OnLoadingProgress(ILevelInfo* pLevel, int progressAmount);
	// ~ILevelSystemListener


private:
	IFlashPlayer* m_pFlashPlayer;
	bool m_bCursor;
	int m_iWidth;
	int m_iHeight;
	std::list<string> m_RegisteredCVars;

	void updateState();
};

#endif // #ifndef __FlashMainMenu_H__