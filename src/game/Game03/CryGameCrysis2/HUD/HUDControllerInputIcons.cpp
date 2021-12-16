/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
*************************************************************************/

#include "StdAfx.h"
#include "HUDControllerInputIcons.h"

#include "HUD/HUD.h"
#include "HUD/UIElementManager.h"
#include "ActionMap.h"
#include "IGame.h"
#include "IGameFramework.h"
#include "Utility/CryWatch.h"
#include "Utility/StringUtils.h"

#if defined(_RELEASE)
#define WATCH_CONTROLLER_INPUT_ICON_FAILURES			(0)
#define ASSERT_ON_CONTROLLER_INPUT_ICON_FAILURES	(0)
#elif defined(PROFILE)
#define WATCH_CONTROLLER_INPUT_ICON_FAILURES			(CRY_WATCH_ENABLED && 1)
#define ASSERT_ON_CONTROLLER_INPUT_ICON_FAILURES	(0)
#else
#define WATCH_CONTROLLER_INPUT_ICON_FAILURES			(CRY_WATCH_ENABLED && 1)
#define ASSERT_ON_CONTROLLER_INPUT_ICON_FAILURES	(1)
#endif

void CControllerInputRenderInfo::Clear()
{
	m_type = kCITV_none;
	m_text[0] = '\0';
	m_texture = NULL;
}

void CControllerInputRenderInfo::operator=(const CControllerInputRenderInfo & fromThis)
{
	m_type = fromThis.m_type;
	memcpy (m_text, fromThis.m_text, sizeof(m_text));
	m_texture = fromThis.m_texture;
}

bool CControllerInputRenderInfo::SetIcon(const char * text)
{
	m_type = kCITV_icon;
	m_texture = g_pGame->GetHUD()->GetElementManager()->GetTexture( text );
	assert(m_texture);
	return (m_texture != NULL);
}

bool CControllerInputRenderInfo::SetText(const char * text)
{
	m_type = kCITV_text;
	return cry_strncpy(m_text, g_pGame->GetHUD()->LocalizeString(text, NULL, NULL), sizeof(m_text));
}

bool CControllerInputRenderInfo::CreateForInput(const char * mapName, const char * inputName)
{
	CRY_ASSERT(gEnv);
	CRY_ASSERT(gEnv->pGame);
	CRY_ASSERT(gEnv->pGame->GetIGameFramework());
	CRY_ASSERT(gEnv->pGame->GetIGameFramework()->GetIActionMapManager());

	IActionMap * actionMap = gEnv->pGame->GetIGameFramework()->GetIActionMapManager()->GetActionMap(mapName);
	if (actionMap)
	{
		const char * keysArray[KEYS_PER_ACTION];
		SActionMapBindInfo inputBindInfo = {NULL, keysArray, 0};
		actionMap->GetBindInfo(ActionId(inputName), inputBindInfo, KEYS_PER_ACTION);

		for (int i = 0; i < inputBindInfo.nKeys; ++ i)
		{
			// TODO: Make data driven!

#if defined(PS3)
			if (0 == strcmp(inputBindInfo.keys[i], "pad_square"))
			{
				return SetIcon("controller_face_left");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "pad_cross"))
			{
				return SetIcon("controller_face_down");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "pad_circle"))
			{
				return SetIcon("controller_face_right");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "pad_triangle"))
			{
				return SetIcon("controller_face_up");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "pad_r2"))
			{
				return SetText("@ui_inputName_rightTrigger");	// TODO: An icon would be better...
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "pad_l2"))
			{
				return SetText("@ui_inputName_leftTrigger");	// TODO: An icon would be better...
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "pad_r3"))
			{
				return SetText("@ui_inputName_clickRightStick");	// TODO: An icon would be better...
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "pad_l3"))
			{
				return SetText("@ui_inputName_clickLeftStick");	// TODO: An icon would be better...
			}
#else
			if (0 == strcmp(inputBindInfo.keys[i], "xi_x"))
			{
				return SetIcon("controller_face_left");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "xi_a"))
			{
				return SetIcon("controller_face_down");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "xi_b"))
			{
				return SetIcon("controller_face_right");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "xi_y"))
			{
				return SetIcon("controller_face_up");
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "xi_triggerr_btn"))
			{
				return SetText("@ui_inputName_rightTrigger");	// TODO: An icon would be better...
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "xi_triggerl_btn"))
			{
				return SetText("@ui_inputName_leftTrigger");	// TODO: An icon would be better...
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "xi_thumbr"))
			{
				return SetText("@ui_inputName_clickRightStick");	// TODO: An icon would be better...
			}
			else if (0 == strcmp(inputBindInfo.keys[i], "xi_thumbl"))
			{
				return SetText("@ui_inputName_clickLeftStick");	// TODO: An icon would be better...
			}
#endif
		}
	}

#if WATCH_CONTROLLER_INPUT_ICON_FAILURES || ASSERT_ON_CONTROLLER_INPUT_ICON_FAILURES
	string error;

	if (actionMap)
	{
		const char * keysArray[KEYS_PER_ACTION];
		SActionMapBindInfo inputBindInfo = {NULL, keysArray, 0};
		actionMap->GetBindInfo(ActionId(inputName), inputBindInfo, KEYS_PER_ACTION);

		if (inputBindInfo.nKeys > 0)
		{			
			error.Format("%d input(s) =", inputBindInfo.nKeys);

			for (int i = 0; i < inputBindInfo.nKeys; ++ i)
			{
				error = string(error + " '" + inputBindInfo.keys[i] + "'");
			}
		}
		else
		{
			error = "no such input";
		}
	}
	else
	{
		error.Format("\"%s\" action map not found", mapName);
	}

#if WATCH_CONTROLLER_INPUT_ICON_FAILURES
	CryWatch("No HUD prompt icon/text for \"%s.%s\": %s", mapName, inputName, error.c_str());
#endif

#if ASSERT_ON_CONTROLLER_INPUT_ICON_FAILURES
	CRY_ASSERT_MESSAGE(false, string().Format("No HUD prompt icon/text for \"%s.%s\": %s", mapName, inputName, error.c_str()));
#endif

#endif

	return false;
}

