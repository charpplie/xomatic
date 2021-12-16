#ifndef __FlashMenu_H__
#define __FlashMenu_H__

#include "IInput.h"

class CFlashMainMenu;
class CFlashIngameMenu;

class CFlashMenu : public IInputEventListener
{
public:
	CFlashMenu();
	~CFlashMenu();

	void Draw(float fDeltaTime);

	// IInputEventListener
	virtual bool OnInputEvent( const SInputEvent &event );
	// ~IInputEventListener

private:
	CFlashMainMenu* m_pMainMenu;
	CFlashIngameMenu* m_pIngameMenu;
};

#endif // #ifndef __FlashMenu_H__