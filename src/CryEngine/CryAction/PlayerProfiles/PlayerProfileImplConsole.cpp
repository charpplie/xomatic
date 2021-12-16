////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   PlayerProfileImplConsole.cpp
//  Created:     21/12/2009 by Alex Weighell.
//  Description: Player profile implementation for consoles which manage
//               the profile data via the OS and not via a file system 
//               which may not be present.
//
//               Note:
//               Currently only the user login code is implemented though.
//               The save data is still (incorrectly) using the file system.
//               
//               TODO:
//               Change the save data to use the platforms OS save data API.
//               Probably best to do this through the IPLatformOS interface.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <IXml.h>
#include <StringUtils.h>
#include <IPlatformOS.h>
#include "PlayerProfileImplConsole.h"
#include "PlayerProfile.h"
#include "PlayerProfileImplRSFHelper.h"
#include "CryActionCVars.h"

using namespace PlayerProfileImpl;

void CPlayerProfileImplConsole::Release()
{
	delete this;
}

// Profile Implementation which stores most of the stuff ActionMaps/Attributes in separate files

//------------------------------------------------------------------------
CPlayerProfileImplConsole::CPlayerProfileImplConsole() : m_pMgr(0)
{
}

//------------------------------------------------------------------------
CPlayerProfileImplConsole::~CPlayerProfileImplConsole()
{
}

//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::Initialize(CPlayerProfileManager* pMgr)
{
	m_pMgr = pMgr;
	return true;
}

//------------------------------------------------------------------------
void CPlayerProfileImplConsole::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}

//------------------------------------------------------------------------
void CPlayerProfileImplConsole::InternalMakeFSPath(SUserEntry* pEntry, const char* profileName, string& outPath)
{
	if (CCryActionCVars::Get().g_usePlatformSavingAPI)
	{
		outPath.clear();
	}
	else
	{
		if (pEntry)
		{
			outPath = ("%USER%/Profiles/");
		}
		else
		{
			// default profile always in game folder
			outPath = PathUtil::GetGameFolder();
			outPath.append("/Libs/Config/Profiles/");
		}
		if (profileName && *profileName)
		{
			outPath.append(profileName);
			outPath.append("/");
		}
	}
}

//------------------------------------------------------------------------
void CPlayerProfileImplConsole::InternalMakeFSSaveGamePath(SUserEntry* pEntry, const char* profileName, string& outPath, bool bNeedFolder)
{
	assert (pEntry != 0);
	assert (profileName != 0);

	if (CCryActionCVars::Get().g_usePlatformSavingAPI)
	{
		outPath.clear();
	}
	else
	{
		if (m_pMgr->IsSaveGameFolderShared())
		{
			outPath = m_pMgr->GetSharedSaveGameFolder();
			outPath.append("/");
			if (!bNeedFolder)
			{
				outPath.append(profileName);
				outPath.append("_");
			}
		}
		else
		{
			outPath = "%USER%/Profiles/";
			outPath.append(profileName);
			outPath.append("/SaveGames/");
		}
	}
}


//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::SaveProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name)
{
	// save the profile into a specific location

	// check if it's a valid filename
	if (IsValidFilename(name) == false)
		return false;

	string path;
	InternalMakeFSPath(pEntry, name, path);

	XmlNodeRef rootNode = GetISystem()->CreateXmlNode(PROFILE_ROOT_TAG);
	rootNode->setAttr(PROFILE_NAME_TAG, name);

	// save profile.xml 
	bool ok = SaveXMLFile(path+"profile.xml", rootNode);
	if (ok)
	{
		CSerializerXML serializer(rootNode, false);
		ok = pProfile->SerializeXML(&serializer);

		// save action map and attributes into separate files
		if (ok)
		{
//			ok &= SaveXMLFile(path+"attributes.xml", serializer.GetSection(CPlayerProfile::ATTRIBUTES_TAG));
//			ok &= SaveXMLFile(path+"actionmaps.xml", serializer.GetSection(CPlayerProfile::ACTIONMAPS_TAG));
			ok &= SaveXMLFile(path+"attributes.xml", serializer.GetSection(CPlayerProfileManager::ePPS_Attribute));
			ok &= SaveXMLFile(path+"actionmaps.xml", serializer.GetSection(CPlayerProfileManager::ePPS_Actionmap));
		}
	}
	return ok;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::LoadProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name)
{
	// load the profile from a specific location

	// XML for now
	string path;
	InternalMakeFSPath(pEntry, name, path);

	XmlNodeRef rootNode = GetISystem()->CreateXmlNode(PROFILE_ROOT_TAG);
	CSerializerXML serializer(rootNode, true);
	XmlNodeRef attrNode = LoadXMLFile(path+"attributes.xml");
	XmlNodeRef actionNode = LoadXMLFile(path+"actionmaps.xml");
//	serializer.AddSection(attrNode);
//	serializer.AddSection(actionNode);
	serializer.SetSection(CPlayerProfileManager::ePPS_Attribute, attrNode);
	serializer.SetSection(CPlayerProfileManager::ePPS_Actionmap, actionNode);

	bool ok = pProfile->SerializeXML(&serializer);
	return ok;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::LoginUser(SUserEntry* pEntry)
{
	// lookup stored profiles of the user (pEntry->userId) and fill in the pEntry->profileDesc
	// vector 
	pEntry->profileDesc.clear();

	IPlatformOS *os = gEnv->pSystem->GetPlatformOS();

	bool signedIn = os->UserIsSignedIn(0);
	CryLogAlways("LoginUser::UserIsSignedIn %d\n", signedIn);

	if (!signedIn)
	{
#if !defined(_RELEASE)
		if (!CCryActionCVars::Get().g_userNeverAutoSignsIn)
#endif
		{
			signedIn = os->UserDoSignIn();	// disable this on a cvar to remove the signin box during autotests
			CryLogAlways("LoginUser::UserDoSignIn %d\n", signedIn);
		}
	}
	if (signedIn)
	{
		IPlatformOS::TUserName profileName;

		if (os->UserGetName(0, profileName))
		{	
			pEntry->profileDesc.push_back(SLocalProfileInfo(profileName.c_str()));

			// TODO: We still use the file system to store the profile info
			// This should use the OS specific profile load & save
	
			// Check the profile data exists - if not create it
			ICryPak *pCryPak = gEnv->pCryPak;
			string path;
			InternalMakeFSPath(pEntry, profileName, path);
			if (!pCryPak->IsFileExist(path))
			{
				// Create new profile based on the defaults
				SaveProfile(pEntry, m_pMgr->GetDefaultCPlayerProfile(), profileName.c_str());
			}
		}
		else
		{
			printf( "OS FAILED to get signed in User name\n" );
		}
	}
	else
	{
		printf( "OS No User signed in\n" );
	}

	return signedIn;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::DeleteProfile(SUserEntry* pEntry, const char* name)
{
	string path;
	InternalMakeFSPath(pEntry, name, path);  // no profile name -> only path
	bool bOK = gEnv->pCryPak->RemoveDir(path.c_str(), true);
	// in case the savegame folder is shared, we have to delete the savegames from the shared folder
	if (bOK && m_pMgr->IsSaveGameFolderShared())
	{
		CPlayerProfileManager::TSaveGameInfoVec saveGameInfoVec;
		if (GetSaveGames(pEntry, saveGameInfoVec, name)) // provide alternate profileName, because pEntry->pCurrentProfile points to the active profile
		{
			CPlayerProfileManager::TSaveGameInfoVec::iterator iter = saveGameInfoVec.begin();
			CPlayerProfileManager::TSaveGameInfoVec::iterator iterEnd = saveGameInfoVec.end();
			while (iter != iterEnd)
			{
				DeleteSaveGame(pEntry, iter->name);
				++iter;
			}
		}
	}
	return bOK;
}


//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::RenameProfile(SUserEntry* pEntry, const char* newName)
{
	// TODO: only rename in the filesystem. for now save new and delete old
	const char* oldName = pEntry->pCurrentProfile->GetName();
	bool ok = SaveProfile(pEntry, pEntry->pCurrentProfile, newName);
	if (!ok)
		return false;

	// move the save games
	if (CPlayerProfileManager::sUseRichSaveGames)
	{
		CRichSaveGameHelper sgHelper(this);
		sgHelper.MoveSaveGames(pEntry, oldName, newName);
	}
	else
	{
		CCommonSaveGameHelper sgHelper(this);
		sgHelper.MoveSaveGames(pEntry, oldName, newName);
	}

	DeleteProfile(pEntry, oldName);
	return true;
}


//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::LogoutUser(SUserEntry* pEntry)
{
	return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::GetSaveGames(SUserEntry* pEntry, CPlayerProfileManager::TSaveGameInfoVec& outVec, const char* altProfileName)
{
	if (CPlayerProfileManager::sUseRichSaveGames)
	{
		CRichSaveGameHelper sgHelper(this);
		return sgHelper.GetSaveGames(pEntry, outVec, altProfileName);
	}
	else
	{
		CCommonSaveGameHelper sgHelper(this);
		return sgHelper.GetSaveGames(pEntry, outVec, altProfileName);
	}
}

//------------------------------------------------------------------------
ISaveGame* CPlayerProfileImplConsole::CreateSaveGame(SUserEntry* pEntry)
{
	if (CPlayerProfileManager::sUseRichSaveGames)
	{
		CRichSaveGameHelper sgHelper(this);
		return sgHelper.CreateSaveGame(pEntry);
	}
	else
	{
		CCommonSaveGameHelper sgHelper(this);
		return sgHelper.CreateSaveGame(pEntry);
	}
}

//------------------------------------------------------------------------
ILoadGame* CPlayerProfileImplConsole::CreateLoadGame(SUserEntry* pEntry)
{
	if (CPlayerProfileManager::sUseRichSaveGames)
	{
		CRichSaveGameHelper sgHelper(this);
		return sgHelper.CreateLoadGame(pEntry);
	}
	else
	{
		CCommonSaveGameHelper sgHelper(this);
		return sgHelper.CreateLoadGame(pEntry);
	}
}

//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::DeleteSaveGame(SUserEntry* pEntry, const char* name)
{
	if (CPlayerProfileManager::sUseRichSaveGames)
	{
		CRichSaveGameHelper sgHelper(this);
		return sgHelper.DeleteSaveGame(pEntry, name);
	}
	else
	{
		CCommonSaveGameHelper sgHelper(this);
		return sgHelper.DeleteSaveGame(pEntry, name);
	}
}

//------------------------------------------------------------------------
bool CPlayerProfileImplConsole::GetSaveGameThumbnail(SUserEntry* pEntry, const char* saveGameName, SThumbnail& thumbnail)
{
	if (CPlayerProfileManager::sUseRichSaveGames)
	{
		CRichSaveGameHelper sgHelper(this);
		return sgHelper.GetSaveGameThumbnail(pEntry, saveGameName, thumbnail);
	}
	else
	{
		CCommonSaveGameHelper sgHelper(this);
		return sgHelper.GetSaveGameThumbnail(pEntry, saveGameName, thumbnail);
	}
}

ILevelRotationFile* CPlayerProfileImplConsole::GetLevelRotationFile(SUserEntry* pEntry, const char* name)
{
  CCommonSaveGameHelper sgHelper(this);
  return sgHelper.GetLevelRotationFile(pEntry,name);
}

