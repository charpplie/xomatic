/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   ProfileDictionary.cpp
$Id$
$DateTime$
Description: Every puppet can select (using an Scriptbind) his own
Behavior Tree + Activation Conditions profile to use. All this data
is loaded here (and reloaded when needed)
---------------------------------------------------------------------
History:
- 06:09:2007 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#include "StdAfx.h"
#include "ProfileDictionary.h"
#include "BehaviorTree.h"
#include "PersonalBehaviorTree.h"
#include "ActivationConditions.h"
#include "GlobalActivationConditions.h"
#include "TacticsManager.h"
#include "StatsManager.h"

// Root path to where BT xmls are stored
#define BT_BTROOT_PATH "Scripts/BehaviorTree/"

// Relative paths to useful info
#define BT_PROFILE_RELPATH "Profiles/"
#define BT_AC_RELPATH "ActivationConditions/"
#define BT_TREE_RELPATH "Trees/"

// Names for special files located in root folder
#define BT_TACTICS_FILE "Tactics.xml"
#define BT_GLOBALS_FILE "Globals.xml"

#define TACTICS_MANAGER_UPDATE_TIME 1.0f

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SBTProfileUser::SBTProfileUser()
: m_pPersonalTree(NULL)
, m_pUser(NULL)
, m_pProfile(NULL)
, m_userId(g_uBTUserId_Invalid)
{
	
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SBTProfileUser::~SBTProfileUser()
{
	SAFE_DELETE(m_pPersonalTree);
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfileUser::Init(IBSSProfileUser *pUser, SBTProfile *pProfile, uint32 uUserCount)
{
	bool bResult = OnNewProfile(pProfile, uUserCount);

	if (pUser && bResult)
	{
		m_pUser = pUser;

		// Generate new personal
		if (!m_pPersonalTree)
		{
			m_pPersonalTree = new CPersonalBehaviorTree;
		}

		bResult = true;
	}

	return bResult;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfileUser::OnNewProfile(SBTProfile *pProfile, uint32 uUserCount)
{
	bool bResult = false;

	if (pProfile)
	{
		m_pProfile = pProfile;
		m_userId = (pProfile->m_uProfileId << 16) | (uUserCount & 0xFF);
		bResult = true;
	}

	return bResult;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfileUser::PostInit()
{
	CRY_ASSERT(m_pPersonalTree);

	if (m_pPersonalTree)
		m_pPersonalTree->SetProfile(m_pProfile, m_userId);
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfileUser::SendEvent(EBTUserEvents event, const string& sData, const ScriptAnyValue *pArgValue) const
{
	if (m_pUser)
	{
		m_pUser->OnBTUserEvent(event, m_pPersonalTree, sData, pArgValue);
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfileUser::CanTreeRun() const
{
	return (m_pUser ? m_pUser->CanBTUserTreeRun() : false);
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfileUser::RunTree(bool bForce)
{
	CRY_ASSERT(m_pPersonalTree);

	if (m_pPersonalTree)
	{
		if (bForce)
			m_pPersonalTree->RunTreeNow();
		else
			m_pPersonalTree->RunTree();
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfileUser::CheckCondition(int iPreCompiledCode, bool bDebug) const
{
	bool bResult = false;

	if (m_pPersonalTree)
	{
		bResult = m_pPersonalTree->CheckCondition(iPreCompiledCode, bDebug);
	}

	return bResult;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfileUser::CreateConditionsTable( SmartScriptTable &pTable ) const
{
	bool bResult = false;

	if (m_pPersonalTree)
	{
		bResult = m_pPersonalTree->CreateConditionsTable(pTable);
	}

	return bResult;
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SBTProfile::SBTProfile( uint32 uProfileId )
: m_uProfileId(uProfileId)
, m_uUserCountGen(g_uBTUserId_Invalid)
, m_pBehaviorTree(NULL)
, m_BTTimeSlice(0.0003f)
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
SBTProfile::~SBTProfile()
{
	TUserMap::iterator itUser = m_Users.begin();
	TUserMap::iterator itUserEnd = m_Users.end();
	for (; itUser != itUserEnd; ++itUser)
	{
		SBTProfileUser *pUser = (itUser->second);
		SAFE_DELETE(pUser);
	}
	m_Users.clear();

	m_vecActivationConditions.clear();
	m_listPersonals.clear();
	SAFE_DELETE(m_pBehaviorTree);
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfile::Init()
{
	m_pBehaviorTree->Init();
}

// Description:
//
// Arguments:
//
// Return:
//
bool SBTProfile::LoadBehaviorTree( const string& sFile )
{
	bool bRet = false;

	m_pBehaviorTree = new CBehaviorTree( "root", m_uProfileId );
	bRet = m_pBehaviorTree->ReLoad( sFile );

	if( bRet == false )
	{
		SAFE_DELETE( m_pBehaviorTree );

		CryWarning( VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "[ProfileDictionary] Could not load Behavior Tree %s", sFile.c_str() );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool SBTProfile::LoadActivationConditions( XmlNodeRef &pNode )
{
	assert( pNode != NULL );

	bool bRet = false;

	if (pNode)
	{
		const int iCount = pNode->getChildCount();
		m_vecActivationConditions.clear();
		m_vecActivationConditions.reserve(iCount);

		for (int i = 0; i < iCount; ++i)
		{
			XmlNodeRef pChild = pNode->getChild(i);
			if (!pChild || stricmp(pChild->getTag(), "ActivationCondition") != 0)
				continue;

			SActivationConditionDef conditionDef;
			if (conditionDef.Init(pChild))
			{
				m_vecActivationConditions.push_back(conditionDef);
				bRet = true;
			}
			else
			{
				CryWarning( VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "[ProfileDictionary] Profile %u failed when creating definition", m_uProfileId );
			}
		}
	}

	if (!bRet)
	{
		CryWarning( VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "[ProfileDictionary] Profile %u has no valid activation conditions!", m_uProfileId );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool SBTProfile::LoadTactics( CTacticsManager *pTacticsManager )
{
	CRY_ASSERT( pTacticsManager );
	CRY_ASSERT( m_pBehaviorTree );

	return m_pBehaviorTree->LoadTactics( pTacticsManager );
}

// Description:
//
// Arguments:
//
// Return:
//
void SBTProfile::AddPersonalBT( CPersonalBehaviorTree* pPersonal )
{
	assert( pPersonal != NULL );

	if (!m_listPersonals.empty())
	{
		m_listPersonals.push_back( pPersonal );		
	}
	else
	{
		m_listPersonals.push_back( pPersonal );
		m_sNextOne = m_listPersonals.begin();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void SBTProfile::DelPersonalBT( CPersonalBehaviorTree* pPersonal )
{
	assert( pPersonal != NULL );

	TPersonalList::iterator it;
	it = std::find( m_listPersonals.begin(), m_listPersonals.end(), pPersonal );

	if (it == m_sNextOne)
	{
		if (m_sNextOne == m_listPersonals.end())
		{
			m_sNextOne = m_listPersonals.begin();
		}
		else
		{
			++m_sNextOne;
		}
	}

	if( it != m_listPersonals.end() )
	{
		m_listPersonals.erase( it );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void SBTProfile::RunAllTrees( bool bForceRealUpdate )
{
	TPersonalList::iterator it;
	for( it = m_listPersonals.begin(); it != m_listPersonals.end(); ++it )
	{
		CPersonalBehaviorTree*	pPersonal = *it;

		if( bForceRealUpdate == true )
		{
			pPersonal->RunTreeNow();
		}
		else
		{
			pPersonal->RunTree();
		}
	}
}

void SBTProfile::Update( )
{
	CTimeValue starttime = gEnv->pTimer->GetAsyncTime();
	CTimeValue endtime = starttime;

	if (!m_listPersonals.empty())
	{
		uint32 count = m_listPersonals.size();
		uint32 updatesi = 0;

		do
		{
			if (m_sNextOne == m_listPersonals.end())
			{
				m_sNextOne = m_listPersonals.begin();
			}

			CPersonalBehaviorTree *tree = (*m_sNextOne);

			if (tree->Enabled() && tree->Update())
			{
				//tree->Update();
				endtime = gEnv->pTimer->GetAsyncTime();
			}

			++m_sNextOne;

			++updatesi;
		} while( (endtime - starttime) < m_BTTimeSlice && (updatesi < count) );
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfile::InitNewUser(IBSSProfileUser *pUser, TBSSProfileUserId &outUserId)
{
	CRY_ASSERT(pUser);

	bool bResult = false;

	// Test if there isn't already a user
	TUserMap::iterator itUser = m_Users.begin();
	TUserMap::iterator itUserEnd = m_Users.end();
	for (; itUser != itUserEnd; ++itUser)
	{
		SBTProfileUser *pProfileUser = (itUser->second);
		CRY_ASSERT(pProfileUser);

		if (pProfileUser && pProfileUser->m_pUser == pUser)
		{
			// Already added
			outUserId = pProfileUser->m_userId;
			bResult = true;
			break;
		}
	}

	if (!bResult)
	{
		// Create new entry
		SBTProfileUser *pNewProfileUser = new SBTProfileUser;
		CRY_ASSERT(pNewProfileUser);

		if (pNewProfileUser)
		{
			bResult = pNewProfileUser->Init(pUser, this, ++m_uUserCountGen);
		}

		if (bResult)
		{
			outUserId = pNewProfileUser->m_userId;
			m_Users[outUserId] = pNewProfileUser;

			// Set the profile last
			pNewProfileUser->PostInit();
		}
		else
		{
			SAFE_DELETE(pNewProfileUser);
		}
	}

	return bResult;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfile::RemoveUser(TBSSProfileUserId userId)
{
	TUserMap::iterator itUser = m_Users.find(userId);
	if (itUser != m_Users.end())
	{
		SBTProfileUser *pUser = itUser->second;

		if (pUser)
		{
			// Remove tactic group Id info
			CTacticsManager *pTacticsManager = GetAISystem()->GetBTProfileDictionary()->GetTacticsManager();
			if (pTacticsManager)
			{
				pTacticsManager->RemoveUserGroupid(userId);
			}
		}

		SAFE_DELETE(pUser);
		m_Users.erase(itUser);
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfile::ResetUser(SBTProfileUser *pUser)
{
	CRY_ASSERT(pUser);

	// Reset personal tree
	CRY_ASSERT(pUser->m_pPersonalTree);
	pUser->m_pPersonalTree->Reset();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfile::ResetUser(TBSSProfileUserId userId)
{
	TUserMap::iterator itUser = m_Users.find(userId);
	if (itUser != m_Users.end())
	{
		SBTProfileUser *pUser = itUser->second;
		CRY_ASSERT(pUser);

		ResetUser(pUser);
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfile::ResetAllUsers()
{
	TUserMap::iterator itUser = m_Users.begin();
	TUserMap::iterator itUserEnd = m_Users.end();
	for (; itUser != itUserEnd; ++itUser)
	{
		SBTProfileUser *pUser = itUser->second;
		CRY_ASSERT(pUser);

		ResetUser(pUser);
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfile::EnableUser(TBSSProfileUserId userId, bool bEnable)
{
	TUserMap::iterator itUser = m_Users.find(userId);
	if (itUser != m_Users.end())
	{
		SBTProfileUser *pUser = itUser->second;
		CRY_ASSERT(pUser);

		CRY_ASSERT(pUser->m_pPersonalTree);
		if (bEnable)
			pUser->m_pPersonalTree->Enable();
		else
			pUser->m_pPersonalTree->Disable();

	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void SBTProfile::RunUserTree(TBSSProfileUserId userId, bool bForce)
{
	TUserMap::iterator itUser = m_Users.find(userId);
	if (itUser != m_Users.end())
	{
		SBTProfileUser *pUser = itUser->second;
		CRY_ASSERT(pUser);

		pUser->RunTree(bForce);
	}
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
SBTProfileUser* SBTProfile::GetUser(TBSSProfileUserId userId) const
{
	SBTProfileUser *pUser = NULL;

	TUserMap::const_iterator itUser = m_Users.find(userId);
	if (itUser != m_Users.end())
	{
		pUser = itUser->second;
		CRY_ASSERT(pUser);
	}

	return pUser;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
SBTProfileUser* SBTProfile::GetUser(IBSSProfileUser *pProfileUser) const
{
	SBTProfileUser *pUser = NULL;

	TUserMap::const_iterator itUser = m_Users.begin();
	TUserMap::const_iterator itUserEnd = m_Users.end();
	for (; itUser != itUserEnd; ++itUser)
	{
		SBTProfileUser *pThisUser = itUser->second;
		CRY_ASSERT(pThisUser);

		if (pThisUser && pThisUser->m_pUser == pProfileUser)
		{
			pUser = pThisUser;
			break;
		}
	}

	return pUser;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfile::SendUserInput(TBSSProfileUserId userId, const char* szSignal, const ScriptAnyValue &argValue)
{
	bool bResult = false;

	TUserMap::const_iterator itUser = m_Users.find(userId);
	if (itUser != m_Users.end())
	{
		SBTProfileUser *pThisUser = itUser->second;
		CRY_ASSERT(pThisUser);

		bResult = (pThisUser && pThisUser->m_pPersonalTree && pThisUser->m_pPersonalTree->ReceiveSignal(szSignal, argValue));
	}

	return bResult;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
bool SBTProfile::AddProfileUser(SBTProfileUser* &pProfileUser, TBSSProfileUserId &outUserId)
{
	bool bResult = false;

	if (pProfileUser)
	{
		bResult = pProfileUser->OnNewProfile(this, ++m_uUserCountGen);
	}

	if (bResult)
	{
		outUserId = pProfileUser->m_userId;
		m_Users[outUserId] = pProfileUser;

		// Set the profile last
		pProfileUser->PostInit();
	}
	else
	{
		SAFE_DELETE(pProfileUser);
	}

	return bResult;
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
SBTProfileUser* SBTProfile::ReleaseProfileUser(TBSSProfileUserId userId)
{
	SBTProfileUser* pResult = NULL;

	TUserMap::iterator itUser = m_Users.find(userId);
	if (itUser != m_Users.end())
	{
		// Release it from ownership
		pResult = itUser->second;
		m_Users.erase(itUser);
	}

	return pResult;
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SBTProfile::SActivationConditionDef::SActivationConditionDef()
: m_bMustReset(false)
{
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
bool SBTProfile::SActivationConditionDef::Init(XmlNodeRef &pNode)
{
	CRY_ASSERT(pNode);

	bool bResult = false;

	if (pNode)
	{
		// Must always have a name
		XmlString szName;
		if (pNode->getAttr("name", szName))
		{
			szName.MakeLower();
			m_sName = szName;

			XmlString szTable;
			if (pNode->getAttr("table", szTable))
			{
				m_sTable = szTable;
			}
			else
			{
				m_sTable = m_sName;
			}

			if (!pNode->getAttr("doReset", m_bMustReset))
				m_bMustReset = false;

			bResult = true;
		}
	}

	return bResult;
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CProfileDictionary::CProfileDictionary()
: m_bIsEditing( false )
, m_bProfilesInit( false )
, m_bDebugTactics( false )
, m_uProfileIdGen( g_uBTProfileId_Invalid+1 )
, m_fUpdateTacticsTime( TACTICS_MANAGER_UPDATE_TIME )
, m_pTacticsManager( NULL )
, m_pGlobalConditions( NULL )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CProfileDictionary::~CProfileDictionary()
{
	for( TProfileMap::iterator iter = m_Profiles.begin(); iter != m_Profiles.end(); ++iter )
	{
		SBTProfile *pProfile = iter->second;
		SAFE_DELETE( pProfile );
	}

	for( TActivationConditionsMap::iterator iter = m_ActivationConditions.begin(); iter != m_ActivationConditions.end(); ++iter )
	{
		CActivationConditions *pConditions = iter->second;
		SAFE_DELETE( pConditions );
	}

	m_Profiles.clear();
	m_ActivationConditions.clear();

	SAFE_DELETE(m_pTacticsManager);
	SAFE_DELETE(m_pGlobalConditions);
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::OnEditorSetGameMode( bool bMode )
{
	m_bIsEditing = !bMode;

	for( TProfileMap::iterator iter = m_Profiles.begin(); iter != m_Profiles.end(); ++iter )
	{
		SBTProfile *pProfile = iter->second;

		pProfile->m_pBehaviorTree->OnEditorSetGameMode( bMode );
	}

	for( TActivationConditionsMap::iterator iter = m_ActivationConditions.begin(); iter != m_ActivationConditions.end(); ++iter )
	{
		CActivationConditions *pConditions = iter->second;
		
		pConditions->OnEditorSetGameMode( bMode );
	}

	if (m_pGlobalConditions)
	{
		m_pGlobalConditions->OnEditorSetGameMode(bMode);
	}

	if (m_pTacticsManager)
	{
		m_pTacticsManager->OnEditorSetGameMode(bMode);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::InitProfiles()
{
	// Init global activation conditions first
	if (m_pGlobalConditions)
	{
		if (m_bProfilesInit)
			m_pGlobalConditions->Reset();
		else
			m_pGlobalConditions->InitGlobals();
	}

	for( TActivationConditionsMap::iterator iter = m_ActivationConditions.begin(); iter != m_ActivationConditions.end(); ++iter )
	{
		CActivationConditions *pConditions = iter->second;

		if (m_bProfilesInit)
			pConditions->Reset();
		else
			pConditions->Init();
	}

	if (m_bProfilesInit)
	{
		ResetAllUsers();
	}
	else
	{
		for( TProfileMap::iterator iter = m_Profiles.begin(); iter != m_Profiles.end(); ++iter )
		{
			SBTProfile *pProfile = iter->second;

			pProfile->Init();
		}
	}

	// Init tactics last
	if (m_pTacticsManager)
	{
		if (m_bProfilesInit)
			m_pTacticsManager->Reset();
		else
			m_pTacticsManager->Init();
	}

	m_bProfilesInit = true;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::LoadData()
{
	bool bRet = true;

	// Load global activation conditions first
	if ( bRet )
	{
		bRet = LoadGlobals( BT_GLOBALS_FILE );
		if (!bRet)
		{
			AIError( "[ProfileDictionary] Could not read BT Globals %s", BT_GLOBALS_FILE );
		}
	}

	// Load activation conditions
	if ( bRet )
	{
		string sPath = PathUtil::AddSlash(BT_BTROOT_PATH) + PathUtil::AddSlash(BT_AC_RELPATH);
		bRet = LoadActivationConditions( sPath );
		if (!bRet)
		{
			AIError( "[ProfileDictionary] Failed when reading activation conditions in \'%s\'", sPath.c_str());
		}
	}

	// Load profiles
	if ( bRet )
	{
		string sPath = PathUtil::AddSlash(BT_BTROOT_PATH) + PathUtil::AddSlash(BT_PROFILE_RELPATH);
		bRet = LoadProfiles( sPath );
		if (!bRet)
		{
			AIError( "[ProfileDictionary] Failed when reading profiles in \'%s\'", sPath.c_str());
		}
	}

	// Load tactics last
	if ( bRet )
	{
		bRet = LoadTactics( BT_TACTICS_FILE );
		if (!bRet)
		{
			AIError( "[ProfileDictionary] Could not read BT Tactics %s", BT_TACTICS_FILE );
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::LoadActivationConditions( const string& sDir )
{
	bool bRet = false;

	ICryPak *pCryPak = gEnv->pCryPak;
	_finddata_t fd;

	string sSearchPath = PathUtil::Make(sDir, string("*"), string("xml"));
	intptr_t iHandle = pCryPak->FindFirst(sSearchPath, &fd);
	if (iHandle >= 0)
	{
		do
		{
			string sName = PathUtil::GetFileName(fd.name);
			sName.MakeLower();

			// Make sure not already added
			TActivationConditionsMap::iterator itEntry = m_ActivationConditions.find(sName);
			CRY_ASSERT(itEntry == m_ActivationConditions.end());
			if (itEntry == m_ActivationConditions.end())
			{
				string sFile = PathUtil::Make(sDir, string(fd.name), string("xml"));
				CActivationConditions *pConditions = new CActivationConditions(sName);
				CRY_ASSERT(pConditions);

				bRet = pConditions->Reload(sFile);
				if (!bRet)
				{
					SAFE_DELETE(pConditions);
					CryWarning(VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "[ProfileDictionary] Could not load Activation Condition %s", sFile.c_str());
					break;
				}

				// Set in map
				m_ActivationConditions[sName] = pConditions;
			}
		} while( pCryPak->FindNext(iHandle, &fd) >= 0 );

		pCryPak->FindClose(iHandle);
	}

	return bRet;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::LoadProfiles( const string& sDir )
{
	bool bRet = false;

	ICryPak *pCryPak = gEnv->pCryPak;
	_finddata_t fd;

	string sSearchPath = PathUtil::Make(sDir, string("*"), string("xml"));
	intptr_t iHandle = pCryPak->FindFirst(sSearchPath, &fd);
	if (iHandle >= 0)
	{
		do
		{
			string sFile = PathUtil::Make(sDir, string(fd.name), string("xml"));
			bRet = LoadProfile(sFile);
			if (!bRet)
			{
				CryWarning(VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "[ProfileDictionary] Could not load Profile %s", sFile.c_str());
				break;
			}
		} while( pCryPak->FindNext(iHandle, &fd) >= 0 );

		pCryPak->FindClose(iHandle);
	}

	return bRet;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::LoadProfile( const string& sFile )
{
	bool bRet = false;

	// Attempt to load it
	XmlNodeRef pRoot = gEnv->pSystem->LoadXmlFile(sFile.c_str());
	if (pRoot && stricmp(pRoot->getTag(), "BTProfile") == 0)
	{
		SBTProfile *pProfile = new SBTProfile( m_uProfileIdGen++ );
		string sName = PathUtil::GetFileName(sFile);
		sName.MakeLower();	// Go lower case for profile name, regardless of file casing
		m_Profiles.insert( TProfileMap::value_type(sName, pProfile) );

		// Attempt to load the tree
		XmlString szTreeName;
		if (pRoot->getAttr("tree", szTreeName))
		{
			string sTreePath = PathUtil::AddSlash(BT_BTROOT_PATH) + PathUtil::AddSlash(BT_TREE_RELPATH);
			string sTree = PathUtil::Make(sTreePath, szTreeName, "xml");
			
			bRet = pProfile->LoadBehaviorTree(sTree);
		}

		// Attempt to load the activation condition list
		XmlNodeRef pACList = (bRet ? pRoot->findChild("ActivationConditions") : XmlNodeRef(0));
		if (pACList)
		{
			bRet = pProfile->LoadActivationConditions(pACList);
		}
	}

	return bRet;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::LoadGlobals( const string& sFile )
{
	bool bRet = false;

	if (!m_pGlobalConditions)
	{
		m_pGlobalConditions = new CGlobalActivationConditions("_GLOBALS");
		bRet = m_pGlobalConditions->ReLoad( PathUtil::Make(string(BT_BTROOT_PATH), sFile) );
	}

	return bRet;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::LoadTactics( const string& sFile )
{
	bool bRet = false;

	if (!m_pTacticsManager)
	{
		m_pTacticsManager = new CTacticsManager(this);
		CRY_ASSERT(m_pTacticsManager);
		bRet = (m_pTacticsManager && m_pTacticsManager->ReLoad( PathUtil::Make(string(BT_BTROOT_PATH), sFile) ));
	}

	if (bRet)
	{
		TProfileMap::iterator itI = m_Profiles.begin();
		TProfileMap::iterator itEnd = m_Profiles.end();
		for (; itI != itEnd; ++itI)
		{
			itI->second->LoadTactics( m_pTacticsManager );
		}
	}

	return bRet;
}

// Description:
//
// Arguments:
//
// Return:
//
CActivationConditions* CProfileDictionary::GetActivationConditions( const string& sName )
{
	CActivationConditions* pResult = NULL;

	string sNameLower(sName);
	sNameLower.MakeLower();

	TActivationConditionsMap::iterator itEntry = m_ActivationConditions.find(sNameLower);
	if (itEntry != m_ActivationConditions.end())
	{
		pResult = itEntry->second;
	}

	return pResult;
}

// Description:
//
// Arguments:
//
// Return:
//
CActivationConditions const* CProfileDictionary::GetActivationConditions( const string& sName ) const
{
	CActivationConditions const* pResult = NULL;

	string sNameLower(sName);
	sNameLower.MakeLower();

	TActivationConditionsMap::const_iterator itEntry = m_ActivationConditions.find(sNameLower);
	if (itEntry != m_ActivationConditions.end())
	{
		pResult = itEntry->second;
	}

	return pResult;
}

// Description:
//
// Arguments:
//
// Return:
//
SBTProfile* CProfileDictionary::GetProfile( const string& sProfile ) const
{
	SBTProfile *pProfile = NULL;
	string sPro( sProfile );
	sPro.MakeLower();

	TProfileMap::const_iterator	iter = m_Profiles.find( sPro );

	if( iter != m_Profiles.end() )
	{
		pProfile = iter->second;
	}

	return( pProfile );
}

// Description:
//
// Arguments:
//
// Return:
//
SBTProfile* CProfileDictionary::GetProfile( uint32 uProfileId ) const
{
	SBTProfile*		pProfile = NULL;
	TProfileMap::const_iterator	iter = m_Profiles.begin();
	TProfileMap::const_iterator	iterEnd = m_Profiles.end();

	for( ; iter != iterEnd; ++iter )
	{
		SBTProfile *pThisProfile = iter->second;
		if( pThisProfile->m_uProfileId == uProfileId )
		{
			pProfile = pThisProfile;
			break;
		}
	}

	return( pProfile );
}

// Description:
//
// Arguments:
//
// Return:
//
uint32 CProfileDictionary::GetProfileId( const string& sProfile ) const
{
	uint32 uProfileId = g_uBTProfileId_Invalid;
	string	sPro( sProfile );
	sPro.MakeLower();

	TProfileMap::const_iterator	iter = m_Profiles.find( sPro );

	if( iter != m_Profiles.end() )
	{
		SBTProfile *pProfile = iter->second;
		uProfileId = pProfile->m_uProfileId;
	}

	return( uProfileId );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::GetProfileName( uint32 uProfileId, string& sProfile ) const
{
	bool bResult = false;
	TProfileMap::const_iterator	iter = m_Profiles.begin();
	TProfileMap::const_iterator	iterEnd = m_Profiles.end();

	for( ; iter != iterEnd; ++iter )
	{
		SBTProfile *pProfile = iter->second;
		if( pProfile->m_uProfileId == uProfileId )
		{
			sProfile = iter->first;
			bResult = true;
			break;
		}
	}

	return( bResult );
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::Update( float fDelta )
{
	if (fDelta > 0.0f)
	{
		for( TActivationConditionsMap::iterator iter = m_ActivationConditions.begin(); iter != m_ActivationConditions.end(); ++iter )
		{
			CActivationConditions *pConditions = iter->second;

			pConditions->Update( fDelta );
		}

		for( TProfileMap::iterator iter = m_Profiles.begin(); iter != m_Profiles.end(); ++iter )
		{
			SBTProfile *pProfile = iter->second;

			// Update this profile's personal behavior trees
			pProfile->Update();
		}

		// Update tactics
		if (m_pTacticsManager)
		{
			m_fUpdateTacticsTime -= fDelta;
			if( m_fUpdateTacticsTime <= 0.0f )
			{
				m_pTacticsManager->Update();
				m_fUpdateTacticsTime = TACTICS_MANAGER_UPDATE_TIME;
			}
		}
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::UpdateDraw()
{
	for( TProfileMap::iterator iter = m_Profiles.begin(); iter != m_Profiles.end(); ++iter )
	{
		( (SBTProfile*) iter->second )->m_pBehaviorTree->UpdateDraw();
	}

	if ( m_bDebugTactics && m_pTacticsManager )
	{
		m_pTacticsManager->DebugDraw();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::SetDebugTree( const char* sProfile )
{
	assert( sProfile != NULL );

	SBTProfile *pSelectedProfile = GetProfile( sProfile );

	for( TProfileMap::iterator iter = m_Profiles.begin(); iter != m_Profiles.end(); ++iter )
	{
		SBTProfile*		pProfile = iter->second;
		pProfile->m_pBehaviorTree->SetDebugTree( (pProfile == pSelectedProfile) );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::SetDebugTactics( bool bDebug )
{
	m_bDebugTactics = bDebug;
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::SetDebugActivationConditions( const char* sProfile )
{
	assert( sProfile != NULL );

	SBTProfile *pSelectedProfile = GetProfile( sProfile );

	for( TActivationConditionsMap::iterator iter = m_ActivationConditions.begin(); iter != m_ActivationConditions.end(); ++iter )
	{
		CActivationConditions *pConditions = iter->second;
		const bool bDebug = (stricmp(pConditions->GetName(), sProfile) == 0);
		pConditions->SetDebug( bDebug );
	}
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::SendSignalToAllActors( const char* szProfileName, const char* szSignal, const ScriptAnyValue &argValue )
{
	assert( szProfileName != NULL );
	assert( szSignal != NULL );

	bool bRet = false;

	SBTProfile *pProfile = GetProfile( szProfileName );

	SBTProfile::TActivationConditionsVec::iterator itCondition = pProfile->m_vecActivationConditions.begin();
	SBTProfile::TActivationConditionsVec::iterator itConditionEnd = pProfile->m_vecActivationConditions.end();
	for (; itCondition != itConditionEnd; ++itCondition)
	{
		const SBTProfile::SActivationConditionDef &conditionDef = *itCondition;
		CActivationConditions *pCondition = GetActivationConditions(conditionDef.m_sName);
		if (pCondition)
		{
			pCondition->ReceiveAnonymousSignal( szSignal, argValue );
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::SendGlobalSignal( const char* szSignal, const ScriptAnyValue &argValue )
{
	assert( szSignal != NULL );

	bool bSent = ( m_pGlobalConditions && m_pGlobalConditions->SetGlobalSignal(szSignal, argValue) );

	// Run all trees if global was received
	if ( bSent )
	{
		TProfileMap::iterator iter = m_Profiles.begin();
		TProfileMap::iterator iterEnd = m_Profiles.end();
		for( ; iter != iterEnd; ++iter )
		{
			iter->second->RunAllTrees();
		}
	}

	return bSent;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::InitNewUser(IBSSProfileUser *pUser, const char* szProfileName, TBSSProfileUserId &outUserId)
{
	CRY_ASSERT(pUser);
	CRY_ASSERT(szProfileName && szProfileName[0]);

	bool bResult = false;
	outUserId = g_uBTUserId_Invalid;

	SBTProfile *pProfile = GetProfile(szProfileName);
	if (pProfile)
	{
		bResult = pProfile->InitNewUser(pUser, outUserId);
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::SwitchUserProfile(TBSSProfileUserId &userId, const char* szProfileName)
{
	CRY_ASSERT(szProfileName && szProfileName[0]);

	bool bResult = false;

	SBTProfile *pNewProfile = GetProfile(szProfileName);
	if (pNewProfile)
	{
		const uint32 uProfileId = GET_PROFILE_FROM_USERID(userId);
		SBTProfile *pOldProfile = GetProfile(uProfileId);

		// Don't bother if we're trying to switch to the same profile
		bResult = (pOldProfile == pNewProfile);
		if (!bResult && pOldProfile)
		{
			// Change ownership of this profile
			SBTProfileUser *pProfileUser = pOldProfile->ReleaseProfileUser(userId);
			if (pProfileUser)
			{
				userId = g_uBTProfileId_Invalid;
				bResult = pNewProfile->AddProfileUser(pProfileUser, userId);
			}
		}
	}

	return bResult;
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::RemoveUser(TBSSProfileUserId userId)
{
	const uint32 uProfileId = GET_PROFILE_FROM_USERID(userId);
	SBTProfile *pProfile = GetProfile(uProfileId);
	if (pProfile)
	{
		pProfile->RemoveUser(userId);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::ResetUser(TBSSProfileUserId userId)
{
	const uint32 uProfileId = GET_PROFILE_FROM_USERID(userId);
	SBTProfile *pProfile = GetProfile(uProfileId);
	if (pProfile)
	{
		pProfile->ResetUser(userId);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::ResetAllUsers()
{
	TProfileMap::iterator itProfile = m_Profiles.begin();
	TProfileMap::iterator itProfileEnd = m_Profiles.end();
	for(; itProfile != itProfileEnd; ++itProfile)
	{
		SBTProfile *pThisProfile = itProfile->second;
		CRY_ASSERT(pThisProfile);

		pThisProfile->ResetAllUsers();
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::EnableUser(TBSSProfileUserId userId, bool bEnable)
{
	const uint32 uProfileId = GET_PROFILE_FROM_USERID(userId);
	SBTProfile *pProfile = GetProfile(uProfileId);
	if (pProfile)
	{
		pProfile->EnableUser(userId, bEnable);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
void CProfileDictionary::RunUserTree(TBSSProfileUserId userId, bool bForce)
{
	const uint32 uProfileId = GET_PROFILE_FROM_USERID(userId);
	SBTProfile *pProfile = GetProfile(uProfileId);
	if (pProfile)
	{
		pProfile->RunUserTree(userId, bForce);
	}
}

// Description:
//
// Arguments:
//
// Return:
//
SBTProfileUser* CProfileDictionary::GetProfileUser(TBSSProfileUserId userId) const
{
	SBTProfileUser *pUser = NULL;

	const uint32 uProfileId = GET_PROFILE_FROM_USERID(userId);
	SBTProfile *pProfile = GetProfile(uProfileId);
	if (pProfile)
	{
		pUser = pProfile->GetUser(userId);
	}

	return pUser;
}

// Description:
//
// Arguments:
//
// Return:
//
bool CProfileDictionary::SendUserInput(TBSSProfileUserId userId, const char* szSignal, const ScriptAnyValue &argValue)
{
	bool bResult = false;

	const uint32 uProfileId = GET_PROFILE_FROM_USERID(userId);
	SBTProfile *pProfile = GetProfile(uProfileId);
	if (pProfile)
	{
		bResult = pProfile->SendUserInput(userId, szSignal, argValue);
	}

	return bResult;
}

#include UNIQUE_VIRTUAL_WRAPPER(IBSSProfileManager)
