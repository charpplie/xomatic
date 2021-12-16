#ifndef __FlashIngameMenu_H__
#define __FlashIngameMenu_H__

#include "IFlashPlayer.h"
#include "IHardwareMouse.h"
#include "IInput.h"


class CFlashIngameMenu :		public IHardwareMouseEventListener,
								public IFSCommandHandler,
								public IInputEventListener 
{
public:
	CFlashIngameMenu();
	virtual ~CFlashIngameMenu();

	void Draw(float fDeltaTime);

	// IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY,EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0);
	// ~IHardwareMouseEventListener

	// IFSCommandHandler
	void HandleFSCommand( const char *szCommand,const char *strArgs, void* pUserData );
	// ~IFSCommandHandler

	// IInputEventListener
	virtual bool OnInputEvent( const SInputEvent &event );
	// ~IInputEventListener

private:
	IFlashPlayer* m_pFlashPlayer;
	bool m_bShow;
	int m_iWidth;
	int m_iHeight;

	void ShowMenu(bool bShow, bool bSetGamePause=true);
};

#endif // #ifndef __FlashIngameMenu_H__