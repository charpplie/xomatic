
#include "StdAfx.h"

#include "ProfileOptions.h"

#include "Option.h"
#include "Game.h"
#include "IPlayerProfiles.h"


//////////////////////////////////////////////////////////////////////////


CProfileOptions::CProfileOptions()
{
	m_allOptions.resize(0);

	XmlNodeRef root = GetISystem()->LoadXmlFile("libs/config/profiles/default/attributes.xml");
	if(!root)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Failed loading attributes.xml");
		return;
	}

	for (int i = 0; i < root->getChildCount(); ++i)
	{
		AddOption(root->getChild(i));
	}
}

CProfileOptions::~CProfileOptions()
{
	while(m_allOptions.size()>0)
	{
		COption* pOption = m_allOptions.back(); m_allOptions.pop_back();
		SAFE_DELETE(pOption);
	}
}

void CProfileOptions::UpdatePlayerProfile(IPlayerProfile *profile)
{
	std::vector<COption*>::iterator it = m_allOptions.begin();
	std::vector<COption*>::iterator end = m_allOptions.end();
	for(; it!=end; ++it)
	{
		CRY_TODO(09, 02, 2010, "Look at clearing or resetting to default all options. If an option has a changed value before switching profile, if the new profile doesn't have it, it will inherit the changed option and not the default.");
		(*it)->SetPlayerProfile(profile);
		(*it)->InitializeFromProfile();
	}
}

bool CProfileOptions::IsCVar(const char* cVarName)
{
	if(!cVarName || !cVarName[0])
		return false;

	ICVar *pCVar = gEnv->pConsole->GetCVar(cVarName);
	return pCVar!=NULL;
}



bool CProfileOptions::IsOption(const char* option)
{
	bool returnValue = false;
	std::vector<COption*>::const_iterator it = m_allOptions.begin();
	std::vector<COption*>::const_iterator end = m_allOptions.end();
	for(; it!=end; ++it)
	{
		if ((*it)->GetName().compare(option)==0)
		{
			returnValue = true;
			break;
		}
	}
	return returnValue;
}



void CProfileOptions::AddOption(const XmlNodeRef node)
{
	AddOption(node->getAttr("name"), node->getAttr("value"), node->getAttr("cvar"));
}



void CProfileOptions::AddOption(const char* name, const char* value, const char* cvar)
{
	COption* pOption = NULL;
	if(IsCVar(cvar))
	{
		pOption = new CCVarOption(name, value, cvar);
	}
	else
	{
		pOption = new COption(name,value);
	}

	if(pOption)
	{
		if(IPlayerProfileManager* profileManager = g_pGame->GetIGameFramework()->GetIPlayerProfileManager())
		{
			if(IPlayerProfile *profile = profileManager->GetCurrentProfile(profileManager->GetCurrentUser()))
			{
				pOption->SetPlayerProfile(profile);
				pOption->InitializeFromProfile();
			}
		}
	}

	m_allOptions.push_back(pOption);
}



string CProfileOptions::GetOptionValue(const char* command)
{
	if(!IsOption(command))
	{
		AddOption(command, "");
	}

	std::vector<COption*>::const_iterator it = m_allOptions.begin();
	std::vector<COption*>::const_iterator end = m_allOptions.end();
	for(; it!=end; ++it)
	{
		if((*it)->GetName().compare(command)==0)
		{
			return (*it)->Get();
		}
	}
	return "";
}



void CProfileOptions::SetOptionValue(const char* command, const char* param)
{
	if(!IsOption(command))
	{
		AddOption(command, param);
	}

	std::vector<COption*>::const_iterator it = m_allOptions.begin();
	std::vector<COption*>::const_iterator end = m_allOptions.end();
	for(; it!=end; ++it)
	{
		if((*it)->GetName().compare(command)==0)
		{
			(*it)->Set(param);
		}
	}
}



void CProfileOptions::SetOptionValue(const char* command, int value)
{
			char charValue[32];
			sprintf(charValue,"%d",value);
			SetOptionValue(command,charValue);
}



void CProfileOptions::InitializeFromProfile()
{
	IPlayerProfileManager* const profileManager = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
	if(!profileManager)
	{
		return;
	}

	IPlayerProfile* const playerProfile = profileManager->GetCurrentProfile(profileManager->GetCurrentUser());
	if (!playerProfile)
	{
		return;
	}

	std::vector<COption*>::const_iterator it = m_allOptions.begin();
	std::vector<COption*>::const_iterator end = m_allOptions.end();
	for(; it!=end; ++it)
	{
		(*it)->InitializeFromProfile();
	}
}



void CProfileOptions::InitializeFromCVar()
{
	IPlayerProfileManager* const profileManager = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
	if(!profileManager)
	{
		return;
	}

	IPlayerProfile* const playerProfile = profileManager->GetCurrentProfile(profileManager->GetCurrentUser());
	if (!playerProfile)
	{
		return;
	}

	std::vector<COption*>::const_iterator it = m_allOptions.begin();
	std::vector<COption*>::const_iterator end = m_allOptions.end();
	for(; it!=end; ++it)
	{
		(*it)->SetPlayerProfile(playerProfile);
		(*it)->InitializeFromCVar();
	}
}



void CProfileOptions::SaveProfile()
{
	IPlayerProfileManager* profileManager = g_pGame->GetIGameFramework()->GetIPlayerProfileManager();
	if(!profileManager)
		return;

	IPlayerProfileManager::EProfileOperationResult result;
	profileManager->SaveProfile(profileManager->GetCurrentUser(), result);
}


//////////////////////////////////////////////////////////////////////////
