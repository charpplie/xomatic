/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Class to handle button regions and navigation on flash menus

-------------------------------------------------------------------------
History:
- 25:09:2009  : Created by Ben Johnson

*************************************************************************/

#ifndef __FLASH_MENU_BUTTON_HANDLER_H__
#define __FLASH_MENU_BUTTON_HANDLER_H__

# pragma once

#include "GameRules.h"

struct IFlashPlayer;

class CFlashMenuButtonHandler
{
public:
	typedef std::map<string,Vec2> ButtonPosMap;

	CFlashMenuButtonHandler();
	~CFlashMenuButtonHandler();

	void UpdateButtonSnap(IFlashPlayer *flashPlayer, const Vec2& mouse);
	void SnapToNextButton(IFlashPlayer *flashPlayer, const Vec2& dir);
	bool CheckSnapToNextButton(const char* direction);
	void PushButton(IFlashPlayer *flashPlayer, const ButtonPosMap::iterator& button, bool press, bool force);
	ButtonPosMap::iterator FindButton(const TKeyName &shortcut);

	void CallbackClear();
	void CallbackAddOnTop();
	void CallbackRemoveFromTop(IFlashPlayer *flashPlayer);
	void CallbackAddNewButton(IFlashPlayer *flashPlayer, const char *strDefinition);
	void CallbackModifyButton(const char *strDefinition);

	void SetLastInputDevice(EDeviceId lastInputId);

private:
	void HighlightButton(IFlashPlayer *flashPlayer, const ButtonPosMap::iterator& button);
	void GetButtonClientPos(IFlashPlayer *flashPlayer, const ButtonPosMap::iterator& button, Vec2 &pos);
	void HWMouse2Flash(IFlashPlayer *flashPlayer, Vec2 &vec);

	ButtonPosMap m_currentButtons;					// Current registered buttons
	std::vector<string> m_currentButtonNames;		// Keep name of the last selected button
	std::vector<ButtonPosMap> m_buttonPositions;	// List of layered buttons registered. To store past page buttons ready to switch back to.
	string m_sCurrentButton;

	float m_lastButtonSnapTime;
	EDeviceId m_lastPressedDeviceId;
};

#endif // ~__FLASH_MENU_BUTTON_HANDLER_H__