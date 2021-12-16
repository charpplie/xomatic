/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 9:2:2010	14:00	: Rewritten by Christian Helmich
  - 17:8:2005			: Modified - NickH: Factory registration moved to GameFactory.cpp
  - 3:8:2004	11:26	: Created by Márcio Martins
  

*************************************************************************/
#include "StdAfx.h"
#include "Game.h"

#include <ISystem.h>
#include <IConsole.h>
#include <IMovieSystem.h>
#include <IPlayerProfiles.h>

#if !defined(FINAL_RELEASE)
#include "../LiveCreate/GameRealtimeRemoteUpdate.h"
#endif //!(FINAL_RELEASE)	

CGame*	g_pGame = 0;	//very bad code practice, use at least static class member, which is as bad, but less prone to errors

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// static variables used hereafter
static const Ang3	s_angDefaultCam(DEG2RAD( -10 ), 0, DEG2RAD( 17 ));
static const Vec3	s_vDefaultCamPos(1326, 1346, 140);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CGame ctor/dtor

CGame::CGame():
#if !defined(FINAL_RELEASE)
	m_pRemoteUpdateListener(0),
#endif //!(FINAL_RELEASE)	
	m_pFramework(0),
	m_pConsole(0),
	m_bReload(false),
	m_inDevMode(false),
	m_angCam(s_angDefaultCam),
	m_vCamPos(s_vDefaultCamPos)
{	
	LOG_CODE_COVERAGE();
	CRY_ASSERT(g_pGame == 0);	//if there is already a global game instance, we did something wrong
	g_pGame = this;
	
#if !defined(FINAL_RELEASE)
	m_pRemoteUpdateListener = &CGameRealtimeRemoteUpdateListener::GetGameRealtimeRemoteUpdateListener();
#endif //!(FINAL_RELEASE)	
	
	GetISystem()->SetIGame( this );
}

//-----------------------------------

CGame::~CGame()
{
	LOG_CODE_COVERAGE();

	gEnv->pInput->RemoveEventListener(this);

	GetISystem()->SetIGame(0);
	CRY_ASSERT(m_pFramework);
	if(m_pFramework)
	{
		m_pFramework->EndGameContext();
		m_pFramework->UnregisterListener(this);
	}
	g_pGame = 0;
}

//-----------------------------------

IGame* CGame::CreateInstance(IGameFramework* pGameFramework)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, NULL);

	ModuleInitISystem(pGameFramework->GetISystem(), "CryGame");

	static char pGameBuffer[sizeof(CGame)];
	return new ((void*)pGameBuffer) CGame();
}

// CGame ctor/dtor/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// IGame implementation

//- game instance lifecycle

bool CGame::Init(IGameFramework *pFramework)
{
	LOG_CODE_COVERAGE();

	LOADING_TIME_PROFILE_SECTION(GetISystem());

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::Init start");
#endif

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	//gameplay foundation framework (CryAction) init
	CRY_ASSERT(pFramework);
	CRY_SAFE_RETURN(!pFramework, false);
	m_pFramework = pFramework;	
	m_pFramework->SetGameGUID(GAME_GUID);
	m_pFramework->RegisterListener(this, "Game", FRAMEWORKLISTENERPRIORITY_GAME);

	//command line init	
	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, false);
	m_pConsole = gEnv->pConsole;
	m_pConsole->CreateKeyBind("f12", "r_getscreenshot 2");

	//level system init
	ILevelSystem* pLevelSystem	= m_pFramework->GetILevelSystem();
	CRY_ASSERT(pLevelSystem);
	CRY_SAFE_RETURN(!pLevelSystem, false);
	pLevelSystem->AddListener(this);	

	m_cCmds.Register();
	m_cVars.Register();
// 	RegisterGameObjectEvents();

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::Init end");
#endif

	return true;
}

//-----------------------------------

bool CGame::CompleteInit()
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT_FUNCTION_CALLED(1);	//had some doubts about how often this function is called

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	CRY_ASSERT(gEnv->pInput);
	CRY_SAFE_RETURN(!gEnv->pInput, false);

#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::CompleteInit start");
#endif

	//finish loading subsystems
	gEnv->pInput->AddEventListener(this);
	
#ifdef GAME_DEBUG_MEM
	DumpMemInfo("CGame::CompleteInit end");
#endif
	return true;
}

//-----------------------------------

int CGame::Update(bool haveFocus, unsigned int updateFlags)
{
	LOG_CODE_COVERAGE();
	
	CRY_ASSERT(m_pFramework);
	CRY_SAFE_RETURN(!m_pFramework, 0);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, 0);

	CRY_ASSERT(gEnv->pRenderer);
	CRY_SAFE_RETURN(!gEnv->pRenderer, 0);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, 0);

	bool	bRun = m_pFramework->PreUpdate( true, updateFlags );

	float	fFrameTime = gEnv->pTimer->GetFrameTime();
	
	UpdateCamera();	//update camera/force it to some pos


	//do updates of game logic subsystems here	
	static const Vec4 scvTextColor(0.1f, 0.2f, 1.0f, 0.5f);
	gEnv->pRenderer->Draw2dLabel(0.0f, 0.0f, 10.0f, (const float*)&scvTextColor.x, false,
		"Hello world\n %s\n%s", 
		"Welcome to\n"
		"CryENGINE3\n",
		GetName());

	
	static ITexture* ptexTemp = NULL;
	if(!ptexTemp)
	{
		ptexTemp = gEnv->pRenderer->EF_LoadTexture("Textures/defaults/defaultnoUVs.tif", FT_NOMIPS|FT_FILESINGLE|FT_FILTER_LINEAR, eTT_2D);		
	}
	gEnv->pRenderer->Draw2dImage(50.0f, 50.0f, 360.0f, 240.0f, ptexTemp->GetTextureID());

// 	if (!m_pFramework->IsGamePaused())
// 	{
// #if !defined(FINAL_RELEASE)
// 		CRY_ASSERT(m_pRemoteUpdateListener);
// 		if(m_pRemoteUpdateListener)
// 			m_pRemoteUpdateListener->Update();
// #endif //!defined(FINAL_RELEASE)
// 	}
// 	else
// 	{
// 		gEnv->pRenderer->Draw2dLabel(0.0f, 0.0f, 5.0f, (const float*)&scvTextColor.x, false,
// 			"Paused");
// 	}

	m_pFramework->PostUpdate( true, updateFlags );

//	CheckReloadLevel();

	return bRun ? 1 : 0;
}

//-----------------------------------

void CGame::Shutdown()
{
	LOG_CODE_COVERAGE();
	this->~CGame();	//bad, but perfectly valid since CGame instance was allocated on static memory
}

//-----------------------------------

void CGame::EditorResetGame(bool bStart)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(gEnv->IsEditor());	
}

//- game instance lifecycle/

//////////////////////////////////////////////////////////////////////////
//- memory stats

void CGame::GetMemoryStatistics(ICrySizer * s)
{
	LOG_CODE_COVERAGE();
	s->Add(*this);
	//for member elements
	//CHILD_MEM_STATISTICS(x, s);
}

//- memory stats/
//////////////////////////////////////////////////////////////////////////
//- game channel

void CGame::ConfigureGameChannel(bool isServer, IProtocolBuilder *pBuilder)
{
	LOG_CODE_COVERAGE();
}

//- game channel/
//////////////////////////////////////////////////////////////////////////
//- player id

void CGame::PlayerIdSet(EntityId playerId)
{
	LOG_CODE_COVERAGE();

	m_uiPlayerID = playerId;	
}

//-----------------------------------

void CGame::OnClearPlayerIds()
{
	LOG_CODE_COVERAGE();
}

//- player id/
//////////////////////////////////////////////////////////////////////////
//- action maps
//-- CryAction dependency, therefore removed --

//- action maps/
//////////////////////////////////////////////////////////////////////////
//- save game

const char* CGame::CreateSaveGameName()
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(m_pFramework);
	CRY_SAFE_RETURN(!m_pFramework, NULL);

// 	IPlayerProfileManager* pPlayerProfileMgr = m_pFramework->GetIPlayerProfileManager();
// 	CRY_ASSERT(pPlayerProfileMgr);
// 	CRY_SAFE_RETURN(!pPlayerProfileMgr, NULL);
// 
// 	const char* pUser = pPlayerProfileMgr->GetCurrentUser();
// 	CRY_ASSERT(pUser);
// 	CRY_SAFE_RETURN(!pUser, NULL);
// 
// 	IPlayerProfile*	pPlayerProfile = pPlayerProfileMgr->GetCurrentProfile(pUser);
// 	CRY_ASSERT(pPlayerProfile);
// 	CRY_SAFE_RETURN(!pPlayerProfile, NULL);

	const char* pLevelName = m_pFramework->GetLevelName();
	CRY_ASSERT(pLevelName);
	CRY_SAFE_RETURN(!pLevelName, NULL);

	const char* pMappedLevelName = GetMappedLevelName(pLevelName);
	CRY_ASSERT(pMappedLevelName);
	CRY_SAFE_RETURN(!pMappedLevelName, NULL);

	static int currentId = 0;
//	pPlayerProfile->GetAttribute("Singleplayer.SaveRunningID", currentId);
//	pPlayerProfile->SetAttribute("Singleplayer.SaveRunningID", currentId+1);

// 	IPlayerProfileManager::EProfileOperationResult	eResult;
// 	pPlayerProfileMgr->SaveProfile(pUser, eResult);

	static char	sszSaveGame[1024];
	sprintf(sszSaveGame, "%s_%x.%s", pMappedLevelName, currentId, GAME_SAVE_EXT);

	currentId++;

	return sszSaveGame;
}

//- save game/
//////////////////////////////////////////////////////////////////////////
//- level name

const char* CGame::GetMappedLevelName(const char *levelName) const
{
	LOG_CODE_COVERAGE();

	TLevelMapMap::const_iterator iter = 
		m_mapNames.find(CONST_TEMP_STRING(levelName));
	
	return (iter == m_mapNames.end()) ? 
		levelName :
		iter->second.c_str();	
}

//- level name/
//////////////////////////////////////////////////////////////////////////
//- game state recorder

IGameStateRecorder* CGame::CreateGameStateRecorder(IGameplayListener* pL)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(pL);
	CRY_SAFE_RETURN(!pL, NULL);

	return NULL;
}

//- game state recorder/
// IGame implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// IGameFrameworkListener implementation

void CGame::OnPostUpdate(float fDeltaTime)
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------

void CGame::OnSaveGame(ISaveGame* pSaveGame)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(pSaveGame);
}

//-----------------------------------

void CGame::OnLoadGame(ILoadGame* pLoadGame)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(pLoadGame);
}

//-----------------------------------

void CGame::OnActionEvent(const SActionEvent& event)
{ 
	//LOG_CODE_COVERAGE();

	if (event.m_event != eAE_earlyPreUpdate)//to avoid console spam
	{
		CryLogAlways("[game] action event: type(%i) value(%i) - %s", 
			event.m_event, event.m_value, event.m_description);
	}
}

// IGameFrameworkListener implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// ILevelSystemListener implementation

void CGame::OnLoadingComplete(ILevel *pLevel)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	IRenderer* pRenderer = gEnv->pRenderer;
	CRY_ASSERT(pRenderer);
	CRY_SAFE_RETURN(!pRenderer, CRY_NO_RETURN_VALUE);

	bool isStandAlone = ( pRenderer->EF_Query(EFQ_RecurseLevel) <= 0 );
	if (isStandAlone)
	{
		gEnv->pRenderer->BeginFrame();
		gEnv->pRenderer->EndFrame();
	}
}

// ILevelSystemListener implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// IInputEventListener interface

bool	CGame::OnInputEvent( const SInputEvent &event )
{
	LOG_CODE_COVERAGE();

	return UpdateInputs(event);
}

bool	CGame::OnInputEventUI( const SInputEvent &event )
{
	LOG_CODE_COVERAGE();

	return OnInputEvent(event);
}

// IInputEventListener interface/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// more utilities

string CGame::InitMapReloading()
{
	LOG_CODE_COVERAGE();
	string levelFileName = m_pFramework->GetLevelName();
	levelFileName = PathUtil::GetFileName(levelFileName);
	
	const char* visibleName = GetMappedLevelName(levelFileName);
	if(visibleName)
		levelFileName = visibleName;

	//levelFileName.append(GAME_LEVEL_EXT);
#ifndef WIN32
	m_bReload = true; //using map command
#else
	m_bReload = false;
	levelFileName.clear();
#endif
	return levelFileName;
}


void CGame::CheckReloadLevel()
{
	LOG_CODE_COVERAGE();
	if(!m_bReload)
		return;

	if(gEnv->IsEditor() || gEnv->bMultiplayer)
	{
		if(m_bReload)
			m_bReload = false;
		return;
	}

	//load levelstart
	ILevelSystem*	pLevelSystem = m_pFramework->GetILevelSystem();
	CRY_ASSERT(pLevelSystem);
	CRY_SAFE_RETURN(!pLevelSystem, CRY_NO_RETURN_VALUE);

	string levelName( m_pFramework->GetLevelName() );

#ifdef WIN32
	// Restart interrupts cutscenes
	gEnv->pMovieSystem->StopAllCutScenes();

	GetISystem()->SerializingFile(1);
	
	ILevel*			pLevel = pLevelSystem->GetCurrentLevel();
	CRY_ASSERT(pLevel);
	CRY_SAFE_RETURN(!pLevel, CRY_NO_RETURN_VALUE);	

	ILevelInfo*		pLevelInfo = pLevelSystem->GetLevelInfo(levelName);
	CRY_ASSERT(pLevelInfo);
	CRY_SAFE_RETURN(!pLevelInfo, CRY_NO_RETURN_VALUE);

	pLevelSystem->OnLoadingStart(pLevelInfo);

	//get visible level name
	if(const char* visibleName = GetMappedLevelName(levelName))
		levelName = visibleName;
	
	//levelFileName.append(GAME_LEVEL_EXT);

	//player id
	EntityId playerID = m_pFramework->GetClientActorId();
	PlayerIdSet(playerID);
	
	//finally load game
	m_pFramework->LoadGame(levelName.c_str(), true, true);
	
	pLevelSystem->OnLoadingComplete(pLevel);
	m_bReload = false;	//if m_bReload is true - load at levelstart

	//if paused - start game
	m_pFramework->PauseGame(false, true);

	GetISystem()->SerializingFile(0);
#else
	string command("map ");
	command.append(levelName);
	m_pConsole->ExecuteString(command);
#endif	//WIN32
}

//-----------------------------------

void CGame::BlockingProcess(BlockingConditionFunction f)
{
	LOG_CODE_COVERAGE();
	INetwork* pNetwork = gEnv->pNetwork;
	CRY_ASSERT(pNetwork);
	CRY_SAFE_RETURN(!pNetwork, CRY_NO_RETURN_VALUE);

	ITimer * pTimer = gEnv->pTimer;
	CRY_ASSERT(pTimer);
	CRY_SAFE_RETURN(!pTimer, CRY_NO_RETURN_VALUE);
	CTimeValue startTime = pTimer->GetAsyncTime();

	bool ok = false;
	while (!ok)
	{
		pNetwork->SyncWithGame(eNGS_FrameStart);
		pNetwork->SyncWithGame(eNGS_FrameEnd);
		pTimer->UpdateOnFrameStart();
		ok = (*f)();
	}
}

//-----------------------------------

void CGame::GameChannelDestroyed(bool bIsServer)
{	
	LOG_CODE_COVERAGE();
}

//-----------------------------------

void CGame::DumpMemInfo(const char* format, ...)
{
	LOG_CODE_COVERAGE();
	CryModuleMemoryInfo memInfo;
	CryGetMemoryInfoForModule(&memInfo);

	va_list args;
	va_start(args,format);
	gEnv->pLog->LogV( ILog::eAlways,format,args );
	va_end(args);

	gEnv->pLog->LogWithType( ILog::eAlways, "Alloc=%I64d kb  String=%I64d kb  STL-alloc=%I64d kb  STL-wasted=%I64d kb", (memInfo.allocated - memInfo.freed) >> 10 , memInfo.CryString_allocated >> 10, memInfo.STL_allocated >> 10 , memInfo.STL_wasted >> 10);
}

//-----------------------------------

void CGame::UpdateCamera()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	CCamera cam = gEnv->pSystem->GetViewCamera();
	cam.SetAngles( m_angCam );
	cam.SetPosition( m_vCamPos );
	gEnv->pSystem->SetViewCamera( cam );
}

bool CGame::UpdateInputs(const SInputEvent& event)
{
	LOG_CODE_COVERAGE();

	Vec3 vPosUpdate(0,0,0);
	Ang3 vAngUpdate(0,0,0);
	float fPosIncrement = 1.0f;
	float fAngIncrement = 0.05f;
	
	if(event.state == eIS_Down && event.deviceId == eDI_Keyboard)
	{
		if(event.modifiers & eMM_LShift)
			fPosIncrement *= 10.0f;
		if(event.modifiers & eMM_RShift)
			fPosIncrement *= 0.1f;

		//pos update
		if(event.keyId == eKI_D)
		{
			vPosUpdate.x += fPosIncrement;
		}
		if(event.keyId == eKI_A)
		{
			vPosUpdate.x -= fPosIncrement;
		}
		if(event.keyId == eKI_W)
		{
			vPosUpdate.y += fPosIncrement;
		}
		if(event.keyId == eKI_S)
		{
			vPosUpdate.y -= fPosIncrement;
		}		
		if(event.keyId == eKI_Q)
		{
			vPosUpdate.z += fPosIncrement;
		}
		if(event.keyId == eKI_E)
		{
			vPosUpdate.z -= fPosIncrement;
		}

		//ang update
		if(event.keyId == eKI_NP_8)
		{
			vAngUpdate.x += fAngIncrement;
		}
		if(event.keyId == eKI_NP_2)
		{
			vAngUpdate.x -= fAngIncrement;
		}
		if(event.keyId == eKI_NP_7)
		{
			vAngUpdate.y += fAngIncrement;
		}
		if(event.keyId == eKI_NP_9)
		{
			vAngUpdate.y -= fAngIncrement;
		}
		if(event.keyId == eKI_NP_4)
		{
			vAngUpdate.z += fAngIncrement;
		}
		if(event.keyId == eKI_NP_6)
		{
			vAngUpdate.z -= fAngIncrement;
		}
		//reset with 5
		if(event.keyId == eKI_NP_5)
		{
			m_angCam = s_angDefaultCam;
		}
	}
	m_angCam	+= vAngUpdate;
	
	Quat qRot;
	qRot.SetRotationXYZ(m_angCam);
	
	vPosUpdate = qRot * vPosUpdate;
	m_vCamPos	+= vPosUpdate;
	
	return true;

}

// more utilities/
//////////////////////////////////////////////////////////////////////////

#include UNIQUE_VIRTUAL_WRAPPER(IGame)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////