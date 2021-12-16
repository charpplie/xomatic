#include "StdAfx.h"

#include "MenuData.h"
#include "IFlashPlayer.h"
#include "IItemSystem.h"
#include "ProfileOptions.h"

//////////////////////////////////////////////////////////////////////////
static TKeyValuePair<EFlashFrontEndScreens,const char*>
gFrontEndScreens[] = {
	{	eFFES_unknown,				"unknown"						},
	{	eFFES_main,						"main"							},
	{	eFFES_profile_login,	"profile_login"			},
	{	eFFES_play_online,		"play_online"				},
	{	eFFES_play_lan,				"play_lan"					},
	{	eFFES_lobby,					"join_game"					},
	{	eFFES_reputation,			"reputation"				},
	{	eFFES_leaderboards,		"leaderboards"			}
};

CMenuButton::CMenuButton()
{
}



CMenuButton::~CMenuButton()
{
}



void CMenuButton::Initialize(const IItemParamsNode* paramNode)
{
	if(paramNode)
	{
		m_caption = paramNode->GetAttribute("caption");
		m_tooltip = paramNode->GetAttribute("tooltip");
		
		CMenuButton::SAction action;
		action.m_action = paramNode->GetAttribute("action");
		if (!action.m_action.empty())
		{
			action.m_param = paramNode->GetAttribute("param");
			m_actions.push_back(action);
		}

		const int numChildren = paramNode->GetChildCount();
		m_actions.reserve(numChildren);
		for(int i = 0; i < numChildren; ++i)
		{
			const IItemParamsNode* childNode = paramNode->GetChild(i);
			const char *name = childNode->GetName();
			if (name && !stricmp(name,"Action"))
			{
				CMenuButton::SAction action;
				action.m_action = childNode->GetAttribute("action");
				action.m_param = childNode->GetAttribute("param");

				m_actions.push_back(action);
			}
		}

		int disabled = 0;
		paramNode->GetAttribute("disabled", disabled);
		m_disabled = disabled==1;
	}
}



void CMenuButton::Announce(IFlashPlayer* pPlayer) const
{
	if(pPlayer)
	{
		SFlashVarValue argsButton[5] = {
			m_caption.c_str()
			, m_tooltip.c_str()
			, (m_actions.size()>0)?m_actions[0].m_action.c_str():""
			, (m_actions.size()>0)?m_actions[0].m_param.c_str():""
			, m_disabled
		};

		pPlayer->Invoke("addButton", argsButton, 5);

		SFlashVarValue argsButtonAction[3] = {"", "", ""};
		CMenuButton::TActions::const_iterator it = m_actions.begin();
		CMenuButton::TActions::const_iterator end = m_actions.end();
		if (it!=end)
		{
			++it;	// Ignore the first;
			for(; it!=end; ++it)
			{
				argsButtonAction[0] = m_caption.c_str();
				argsButtonAction[1] = it->m_action.c_str();
				argsButtonAction[2] = it->m_param.c_str();

				pPlayer->Invoke("addButtonAction", argsButtonAction, 3);
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CMenuSlider::CMenuSlider()
{
}



CMenuSlider::~CMenuSlider()
{
}



void CMenuSlider::Initialize(const IItemParamsNode* paramNode)
{
	if(paramNode)
	{
		m_caption = paramNode->GetAttribute("caption");
		m_tooltip = paramNode->GetAttribute("tooltip");
		m_action = paramNode->GetAttribute("action");
		paramNode->GetAttribute("min", m_min);
		paramNode->GetAttribute("max", m_max);
		paramNode->GetAttribute("step", m_step);
		int disabled = 0;
		paramNode->GetAttribute("disabled", disabled);
		m_disabled = disabled==1;
	}
}



void CMenuSlider::Announce(IFlashPlayer* pPlayer) const
{
	if(pPlayer)
	{
		SFlashVarValue args[8] = {
			m_caption.c_str()
			, m_tooltip.c_str()
			, m_action.c_str()
			, m_min
			, m_max
			, m_step
			, m_disabled
			, g_pGame->GetProfileOptions()->GetOptionValue(m_action.c_str()).c_str()
		};

		pPlayer->Invoke("addSlider", args, 8);
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CMenuSwitch::CMenuSwitch()
{
}



CMenuSwitch::~CMenuSwitch()
{
}



void CMenuSwitch::Initialize(const IItemParamsNode* paramNode)
{
	if(paramNode)
	{
		m_caption = paramNode->GetAttribute("caption");
		m_tooltip = paramNode->GetAttribute("tooltip");
		m_action = paramNode->GetAttribute("action");

		const int numChildren = paramNode->GetChildCount();
		m_options.reserve(numChildren);
		for(int i = 0; i < numChildren; ++i)
		{
			CMenuSwitch::COption option;
			const IItemParamsNode* childNode = paramNode->GetChild(i);
			option.m_caption = childNode->GetAttribute("caption");

			const int type = childNode->GetAttributeType("value");
			switch (type)
			{
			case eIPT_Float:
				{
					float val = 0.0f;
					childNode->GetAttribute("value", val);
					option.m_value.Format("%f",val);
				}
				break;
			case eIPT_Int:
				{
					int val = 0;
					childNode->GetAttribute("value", val);
					option.m_value.Format("%d",val);
				}
				break;
			default:
				option.m_value = childNode->GetAttribute("param");
				break;
			}

			m_options.push_back(option);
		}
		int disabled = 0;
		paramNode->GetAttribute("disabled", disabled);
		m_disabled = disabled==1;
	}
}



void CMenuSwitch::Announce(IFlashPlayer* pPlayer) const
{
	if(pPlayer)
	{
		SFlashVarValue args[5] = {
			m_caption.c_str()
			, m_tooltip.c_str()
			, m_action.c_str()
			, m_disabled
			, g_pGame->GetProfileOptions()->GetOptionValue(m_action.c_str()).c_str()
		};

		pPlayer->Invoke("addSwitch", args, 5);

		CMenuSwitch::TOptions::const_iterator it = m_options.begin();
		CMenuSwitch::TOptions::const_iterator end = m_options.end();
		for(; it!=end; ++it)
		{
			SFlashVarValue args[3] = {
				m_action.c_str()
				, it->m_caption.c_str()
				, it->m_value.c_str()
			};

			pPlayer->Invoke("addSwitchOption", args, 3);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CMenuSubScreen::CMenuSubScreen()
{
}



CMenuSubScreen::~CMenuSubScreen()
{
}



void CMenuSubScreen::Initialize(const IItemParamsNode* paramNode)
{
	if(paramNode)
	{
		m_path = paramNode->GetAttribute("path");
	}
}



void CMenuSubScreen::Announce(IFlashPlayer* pPlayer) const
{
	if(pPlayer)
	{
		pPlayer->Invoke1("addSubScreen", m_path.c_str());
	}
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CMenuScreen::CMenuScreen() : m_screen(eFFES_unknown), m_currentHighlight(0), m_flags(DEFAULT_MENUSCREEN_FLAGS)
{
}



CMenuScreen::~CMenuScreen()
{
}

void CMenuScreen::ReadFlag(const IItemParamsNode* paramNode, const char* name, EFlashMenuScreenFlags flag)
{
	int flagValue = 0;
	if (paramNode->GetAttribute(name, flagValue))
	{
		if (flagValue)
			m_flags |= flag;
		else
			m_flags &= ~flag;
	}
}

void CMenuScreen::Initialize(const IItemParamsNode* paramNode)
{
	if(paramNode)
	{
		m_name = paramNode->GetNameAttribute();
		m_screen = KEY_BY_VALUE(m_name.c_str(), gFrontEndScreens);

		ReadFlag(paramNode, "keepHighlight", eFMSF_RememberHighlight);
		ReadFlag(paramNode, "keepInStack", eFMSF_KeepInStack);

		const int numChildren = paramNode->GetChildCount();
		m_objects.reserve(numChildren);
		for(int i = 0; i < numChildren; ++i)
		{
			const IItemParamsNode* childNode = paramNode->GetChild(i);
			if(!stricmp(childNode->GetName(), "Button"))
			{
				CMenuButton* pObject = new CMenuButton();
				pObject->Initialize(childNode);
				m_objects.push_back(pObject);
			}
			else if(!stricmp(childNode->GetName(), "Slider"))
			{
				CMenuSlider* pObject = new CMenuSlider();
				pObject->Initialize(childNode);
				m_objects.push_back(pObject);
			}
			else if(!stricmp(childNode->GetName(), "Switch"))
			{
				CMenuSwitch* pObject = new CMenuSwitch();
				pObject->Initialize(childNode);
				m_objects.push_back(pObject);
			}
			else if(!stricmp(childNode->GetName(), "SubScreen"))
			{
				CMenuSubScreen* pObject = new CMenuSubScreen();
				pObject->Initialize(childNode);
				m_objects.push_back(pObject);
			}
		}
	}
}



void CMenuScreen::Announce(IFlashPlayer* pPlayer) const
{
	if(pPlayer)
	{
		TObjects::const_iterator it = m_objects.begin();
		TObjects::const_iterator end = m_objects.end();
		for(; it!=end; ++it)
		{
			IMenuObject* pObject = (*it);
			pObject->Announce(pPlayer);
		}
	}
}


//////////////////////////////////////////////////////////////////////////