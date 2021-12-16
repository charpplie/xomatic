/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevelSystem interface. 
					MiniLevelSystem provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 24:2:2010	11:55 : Created by Christian Helmich

*************************************************************************/

#include "StdAfx.h"
#include "MiniLevelSystem.h"
#include <LoadSeq.h>

#include <IGame.h>
#include <IGameFramework.h>
#include <IGameRulesSystem.h>
#include <IGameTokens.h>
#include <IDialogSystem.h>
#include <IAISystem.h>
#include <IMaterial.h>
#include <IMaterialEffects.h>
#include <IMusicSystem.h>
#include <IMovieSystem.h>
#include <IPlayerProfiles.h>


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define SUBMIT_TO_LISTENERS(function, paramlist)	{	\
	FOR_EACH_LOOP(TLevelSystemListenerVector::const_iterator, it, m_listeners)	\
	{	(*it)->function paramlist;	}				}	\

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// static members

simpleframework::CMiniLevelSystem::CLevelNameAutoComplete	simpleframework::CMiniLevelSystem::s_LevelNameAutoComplete;

// static members/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- ctor & dtor

simpleframework::CMiniLevelSystem::CMiniLevelSystem(const char* levelsFolder):
	m_pCurrentLevel(0),
	m_pLoadingLevelInfo(0),
	m_fLastLevelLoadTime(0.0f),
	m_fFilteredProgress(0.0f),
	m_fLastTime(0.0f),
	m_bLevelLoaded(false),
	m_bRecordingFileOpens(false),
	m_nLoadedLevelsCount(0)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(levelsFolder);
	CRY_SAFE_RETURN(!levelsFolder, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	Rescan(levelsFolder);

	gEnv->pSystem->SetLoadingProgressListener(this);
	gEnv->pConsole->RegisterAutoComplete( "map", &s_LevelNameAutoComplete );
}

//-----------------------------------

simpleframework::CMiniLevelSystem::~CMiniLevelSystem()
{
	LOG_CODE_COVERAGE();
	
	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);
	
	gEnv->pSystem->SetLoadingProgressListener(0);
}

//- ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//- ILevelSystemListener (ILevelSystem::parent) implementation

void	simpleframework::CMiniLevelSystem::OnLevelNotFound(const char* levelName)
{
	LOG_CODE_COVERAGE();
	CRY_ASSERT(levelName);

	//submit message to all listeners
	SUBMIT_TO_LISTENERS(OnLevelNotFound, (levelName));
}

//-----------------------------------

void	simpleframework::CMiniLevelSystem::OnLoadingStart(ILevelInfo* pLevelInfo)
{
	LOG_CODE_COVERAGE();

//	CRY_ASSERT(pLevelInfo);//might be NULL
	CRY_SAFE_RETURN(!pLevelInfo, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pTimer);
	CRY_SAFE_RETURN(!gEnv->pTimer, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	ISystemEventDispatcher*	pSysEventDP = gEnv->pSystem->GetISystemEventDispatcher();
	CRY_ASSERT(pSysEventDP);
	CRY_SAFE_RETURN(!pSysEventDP, CRY_NO_RETURN_VALUE);

	m_fFilteredProgress = 0.f;
	m_fLastTime = gEnv->pTimer->GetAsyncCurTime();

	pSysEventDP->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_START, 0, 0 );

#ifdef WIN32 //||WIN64?
	m_bRecordingFileOpens = gEnv->pSystem->IsDevMode() && gEnv->pCryPak->GetRecordFileOpenList() == ICryPak::RFOM_Disabled;
	if (m_bRecordingFileOpens)
	{
		gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_Level)->Clear();
		gEnv->pCryPak->RecordFileOpen(ICryPak::RFOM_Level);
	}
#endif

	SUBMIT_TO_LISTENERS(OnLoadingStart, (pLevelInfo));
	
	BEGIN_LOADING(pLevelInfo->GetName());
}

//-----------------------------------

void	simpleframework::CMiniLevelSystem::OnLoadingComplete(ILevel* pLevel)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pLevel);
	CRY_SAFE_RETURN(!pLevel, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->p3DEngine);
	CRY_SAFE_RETURN(!gEnv->p3DEngine, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pScriptSystem);
	CRY_SAFE_RETURN(!gEnv->pScriptSystem, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pTimer);
	CRY_SAFE_RETURN(!gEnv->pTimer, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	ISystemEventDispatcher* pSysEvtDispatcher = gEnv->pSystem->GetISystemEventDispatcher();
	CRY_ASSERT(pSysEvtDispatcher);
	CRY_SAFE_RETURN(!pSysEvtDispatcher, CRY_NO_RETURN_VALUE);

	IGameFramework*	pGameFramework = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);

	ILevelInfo* pLevelInfo = pLevel->GetLevelInfo();
	CRY_ASSERT(pLevelInfo);
	CRY_SAFE_RETURN(!pLevelInfo, CRY_NO_RETURN_VALUE);

	// let gamerules precache anything needed
	//with MiniGameFramework, those pointers are likely to be NULL, hence this safe structure
	IGameRulesSystem* pGameRulesSys = pGameFramework->GetIGameRulesSystem();
	IGameRules*	pGameRules = NULL;	
	if(pGameRulesSys)
		pGameRules = pGameRulesSys->GetCurrentGameRules();
	if(pGameRules)
		pGameRules->PrecacheLevel();	
	
	// Notify 3D engine that loading finished.	
	gEnv->p3DEngine->PostLoadLevel();
	gEnv->pScriptSystem->ForceGarbageCollection();

	PrecacheLevelRenderData();

	//notify listeners
	SUBMIT_TO_LISTENERS(OnLoadingComplete, (pLevel));
	
	pSysEvtDispatcher->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_END, 0, 0 );
	
	if (m_bRecordingFileOpens)
	{		
		gEnv->pCryPak->RecordFileOpen(ICryPak::RFOM_Disabled);	// Stop recoding pLvlRotFile opening		
		SaveOpenedFilesList();									// Save recorded list.
	}

	CTimeValue	tLevelLoading	= gEnv->pTimer->GetAsyncTime() - m_tLevelLoadingStart;
	m_fLastLevelLoadTime		= tLevelLoading.GetSeconds();
	
	if (!gEnv->IsEditor())
	{
		CryLogAlways( "+++++++++++++++++++++++++++++++++++++++++++++++++++++" );
		CryLogAlways( "*LOADING: Level %s loading time: %.2f seconds",
			m_lastLevelName.c_str(),
			m_fLastLevelLoadTime );
		CryLogAlways( "_____________________________________________________" );
	}

	LogLoadingTime();

	++m_nLoadedLevelsCount;

	// Hide console after loading.
	gEnv->pConsole->ShowConsole(false);

	if (!gEnv->bServer)
		return;

	END_LOADING(pLevelInfo->GetName());
}

//-----------------------------------

void	simpleframework::CMiniLevelSystem::OnLoadingError(ILevelInfo* pLevelInfo, const char* error)
{
	LOG_CODE_COVERAGE();

	if (!pLevelInfo)
		pLevelInfo = m_pLoadingLevelInfo;
	CRY_ASSERT(pLevelInfo);
	if(!pLevelInfo)
		GameWarning("OnLoadingError without a currently loading level");
	
	CRY_ASSERT(error);
	CRY_SAFE_RETURN(!error, CRY_NO_RETURN_VALUE);

	SUBMIT_TO_LISTENERS(OnLoadingError, (pLevelInfo, error));

	END_LOADING(pLevelInfo->GetName());
}

//-----------------------------------

void	simpleframework::CMiniLevelSystem::OnLoadingProgress(ILevelInfo* pLevel, int progressAmount)
{
	LOG_CODE_COVERAGE();

	SUBMIT_TO_LISTENERS(OnLoadingProgress, (pLevel, progressAmount));
}

//- ILevelSystemListener (ILevelSystem::parent) implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- ILevelSystem implementation

//-- level information

void	simpleframework::CMiniLevelSystem::Rescan(const char* levelsFolder)
{
	LOG_CODE_COVERAGE();

	CryLogAlways("%s scanning folder '%s'", __FUNCTION__, levelsFolder);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	ICmdLine* pCmdLine = gEnv->pSystem->GetICmdLine();
	CRY_ASSERT(pCmdLine);
	CRY_SAFE_RETURN(!pCmdLine, CRY_NO_RETURN_VALUE);
	
	if(levelsFolder) //might be NULL
	{
		m_levelsFolder = levelsFolder;
	}

	CRY_ASSERT(!m_levelsFolder.empty());
	CRY_SAFE_RETURN(m_levelsFolder.empty(), CRY_NO_RETURN_VALUE);

	ScanFolder(0);

	//complete levels list for console auto-completion
	s_LevelNameAutoComplete.clear();
	FOR_EACH_LOOP(TLevelInfoVector::const_iterator, iter, m_levelInfos)
	{
		string levelName(iter->GetName() );
		string fileName( PathUtil::GetFileName( levelName ) );
		s_LevelNameAutoComplete.push_back( fileName );
	}

}

//-----------------------------------

void	simpleframework::CMiniLevelSystem::LoadRotation()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, CRY_NO_RETURN_VALUE);

	ICVar* pCVarLevelRotation = gEnv->pConsole->GetCVar("sv_levelrotation");
	CRY_SAFE_RETURN(!pCVarLevelRotation, CRY_NO_RETURN_VALUE);
	string sLevelRotation = pCVarLevelRotation->GetString();

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, CRY_NO_RETURN_VALUE);

	IGameFramework* pGameFramework = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, CRY_NO_RETURN_VALUE);
	
	IPlayerProfileManager* pPlayerProfileMgr = pGameFramework->GetIPlayerProfileManager();
	ILevelRotationFile* pLvlRotFile = NULL;
	
	if(pPlayerProfileMgr)
	{
		const char* userName = pPlayerProfileMgr->GetCurrentUser();
		CRY_ASSERT(userName);

		IPlayerProfile* pProfile = pPlayerProfileMgr->GetCurrentProfile(userName);
		if(!pProfile)
			pProfile = pPlayerProfileMgr->GetDefaultProfile();
		CRY_ASSERT(pProfile);

		if(pProfile)
		{
			pLvlRotFile = pProfile->GetLevelRotationFile( sLevelRotation );
		}		
	}

	bool bLevelRotLoadSuccess = false;
	if(pLvlRotFile)
	{
		bLevelRotLoadSuccess = m_levelRotation.Load(pLvlRotFile);
		pLvlRotFile->Complete();
	}

	if(!bLevelRotLoadSuccess)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, 
			"Failed to load '%s' as level rotation!", sLevelRotation.c_str());
	}

}

//-----------------------------------

int		simpleframework::CMiniLevelSystem::GetLevelCount()
{
	LOG_CODE_COVERAGE();
	return static_cast<int>(m_levelInfos.size());
}

//-----------------------------------

ILevelInfo*	simpleframework::CMiniLevelSystem::GetLevelInfo(int level)
{
	LOG_CODE_COVERAGE();
	return GetLevelInfoInternal(level);
}

//-----------------------------------

ILevelInfo*	simpleframework::CMiniLevelSystem::GetLevelInfo(const char* levelName)
{
	LOG_CODE_COVERAGE();
	return GetLevelInfoInternal(levelName);
}

//-- level information/
//////////////////////////////////////////////////////////////////////////
//-- listener

void	simpleframework::CMiniLevelSystem::AddListener(ILevelSystemListener* pListener)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pListener);
	CRY_SAFE_RETURN(!pListener, CRY_NO_RETURN_VALUE);

	//search listener (avoid double adding)
	CREATE_ITER_VARS(TLevelSystemListenerVector::iterator, it, m_listeners);
	it = std::find(itStart, itEnd, pListener);

	//add if not found
	CRY_ASSERT(it == itEnd);	//avoid double adding by not allowing it to begin with
	CRY_SAFE_RETURN(it != itEnd, CRY_NO_RETURN_VALUE);
	
	m_listeners.push_back(pListener);
}

//-----------------------------------

void	simpleframework::CMiniLevelSystem::RemoveListener(ILevelSystemListener* pListener)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pListener);
	CRY_SAFE_RETURN(!pListener, CRY_NO_RETURN_VALUE);

	//search listener (to be removed)
	CREATE_ITER_VARS(TLevelSystemListenerVector::iterator, it, m_listeners);
	it = std::find(itStart, itEnd, pListener);

	//remove if not found
	CRY_ASSERT(it != itEnd);	//avoid double removing by not allowing it to begin with
	CRY_SAFE_RETURN(it == itEnd, CRY_NO_RETURN_VALUE);

	m_listeners.erase(it);
}

//-- listener/
//////////////////////////////////////////////////////////////////////////
//-- level loading

ILevel*	simpleframework::CMiniLevelSystem::GetCurrentLevel() const
{
	LOG_CODE_COVERAGE();
	return m_pCurrentLevel;
}

//-----------------------------------

ILevel*	simpleframework::CMiniLevelSystem::LoadLevel(const char* levelName)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(levelName);
	CRY_SAFE_RETURN(!levelName, NULL);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, NULL);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, NULL);

	CRY_ASSERT(gEnv->pTimer);
	CRY_SAFE_RETURN(!gEnv->pTimer, NULL);

	CRY_ASSERT(gEnv->pConsole);
	CRY_SAFE_RETURN(!gEnv->pConsole, NULL);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, NULL);

	CRY_ASSERT(gEnv->p3DEngine);
	CRY_SAFE_RETURN(!gEnv->p3DEngine, NULL);

	CRY_ASSERT(gEnv->pPhysicalWorld);
	CRY_SAFE_RETURN(!gEnv->pPhysicalWorld, NULL);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, NULL);

	CRY_ASSERT(gEnv->pEntitySystem);
	CRY_SAFE_RETURN(!gEnv->pEntitySystem, NULL);

	CRY_ASSERT(gEnv->pScriptSystem);
	CRY_SAFE_RETURN(!gEnv->pScriptSystem, NULL);

// 	CRY_ASSERT(gEnv->pDialogSystem);
// 	CRY_SAFE_RETURN(!gEnv->pDialogSystem, NULL);

	CRY_ASSERT(gEnv->pSoundSystem);
	CRY_SAFE_RETURN(!gEnv->pSoundSystem, NULL);

	CRY_ASSERT(gEnv->pAISystem);
	CRY_SAFE_RETURN(!gEnv->pAISystem, NULL);

// 	CRY_ASSERT(gEnv->pFlowSystem);
// 	CRY_SAFE_RETURN(!gEnv->pFlowSystem, NULL);	

	IMaterialManager* pMaterialMgr = gEnv->p3DEngine->GetMaterialManager();    
	CRY_ASSERT(pMaterialMgr);
	CRY_SAFE_RETURN(!pMaterialMgr, NULL);

	IGameFramework* pGameFramework = gEnv->pGame->GetIGameFramework();
	CRY_ASSERT(pGameFramework);
	CRY_SAFE_RETURN(!pGameFramework, NULL);	

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Level load (%s)", levelName);
	LOADING_TIME_PROFILE_SECTION(GetISystem());

	m_tLevelLoadingStart = gEnv->pTimer->GetAsyncTime();

	//get level info
	CMiniLevelInfo* pLevelInfo = GetLevelInfoInternal(levelName);
	CRY_ASSERT(pLevelInfo);
	if(!pLevelInfo)
	{		
		OnLevelNotFound(levelName);	// alert the listeners
		return NULL;
	}

	//spam console
	CryLog( "+++++++++++++++++++++++++++++++++++++++++++++++++++++" );
	CryLog( "*LOADING: Loading Level %s", levelName );
	CryLog( "_____________________________________________________" );


	const bool bLoadingSameLevel = m_lastLevelName.compareNoCase(levelName) == 0;
	if(bLoadingSameLevel)
		CryLog( "reloading level %s", levelName );

	m_lastLevelName = levelName;
	
	//read main level info
	bool bReadLvlInfoSuccess = pLevelInfo->ReadInfo();
	CRY_ASSERT(bReadLvlInfoSuccess);
	if (!bReadLvlInfoSuccess)
	{
		OnLoadingError(pLevelInfo, "Failed to read level info (level.pak might be corrupted)!");
		return NULL;
	}

	//decide whether to show console or loading screen
	gEnv->pConsole->SetScrollMax(600);
	SwitchToConsoleLoadingScreen();

	//submit to listeners
	m_pLoadingLevelInfo = pLevelInfo;
	OnLoadingStart(pLevelInfo);

	//ensure a physical global area is present	
	IPhysicalEntity* pGlobalArea = gEnv->pPhysicalWorld->AddGlobalArea();
	CRY_ASSERT(pGlobalArea);
	CRY_SAFE_RETURN(!pGlobalArea, NULL)

	//open paks
	gEnv->pCryPak->Notify( ICryPak::EVENT_BEGIN_LOADLEVEL );
	if (!gEnv->pCryPak->OpenPacks(pLevelInfo->GetPaks(), 0u))
	{
		OnLoadingError(pLevelInfo, "failed opening PAK");
		return NULL;
	}

	//disable physics thread
	gEnv->pSystem->SetThreadState(ESubsys_Physics, false);

	//reset or not gametokens
	IGameTokenSystem* pGameTokenSys = pGameFramework->GetIGameTokenSystem();	//MiniGameFramework: this can be NULL	
	//keep all non Level-related gametokens in the system when loading new level
	// someone might want to load the same level again (e.g. map level -> map level)
	// make sure we reset the gametokens and start with default values
	if(pGameTokenSys)
	{
		if(bLoadingSameLevel)
			pGameTokenSys->Reset();	// reset level library
		else
			pGameTokenSys->RemoveLibrary("Level");	// clear level library

		//load all GameToken libraries this level uses incl. LevelLocal
		pGameTokenSys->LoadLibs( pLevelInfo->GetPath() + string("/GameTokens/*.xml"));
	}
	

	//loading level into engine
	const char* levelPath = pLevelInfo->GetPath();
	string		gameTypeName = pLevelInfo->GetDefaultGameType()->name;
	bool bEngineLoadLevelSuccess = gEnv->p3DEngine->LoadLevel(levelPath, gameTypeName);
	CRY_ASSERT(bEngineLoadLevelSuccess);
	if (!bEngineLoadLevelSuccess)
	{
		OnLoadingError(pLevelInfo, "3DEngine level loading failed");
		return NULL;
	}

	//submit terrain size to entity system
	int nTerrainSize = gEnv->p3DEngine->GetTerrainSize();
	gEnv->pEntitySystem->ResizeProximityGrid( nTerrainSize,nTerrainSize );
	
	//reset all the script timers
	gEnv->pScriptSystem->ResetTimers();

	//reset TimeOfDayScheduler
	//pGameFramework->GetTimeOfDayScheduler()->Reset(); //not implemented yet

	//reset dialog system
	//gEnv->pDialogSystem->Reset();

	//reset sound system.
	gEnv->pSoundSystem->Silence(true, false);
	gEnv->pSoundSystem->GetInterfaceExtended()->RecomputeSoundOcclusion(true,true,true);
	gEnv->pSoundSystem->Update(eSoundUpdateMode_All); // update once so sounds actually stop

	//AI system	
	gEnv->pAISystem->FlushSystem();
	pGameFramework->DestroyAllAIActorProxies();
	bool bAILoadSuccess = true;
	if(gEnv->bMultiplayer)
	{
		ICVar* pCVsv_AISystem = gEnv->pConsole->GetCVar("sv_AISystem");
		int sv_AISystem = 0;
		if(pCVsv_AISystem)
			sv_AISystem = pCVsv_AISystem->GetIVal();
		if (!sv_AISystem)
			bAILoadSuccess = false;
	}
	CRY_ASSERT(bAILoadSuccess);
	if(bAILoadSuccess)
	{
		gEnv->pAISystem->SetLevelPath( levelPath );
		gEnv->pAISystem->LoadNavigationData( levelPath, gameTypeName );
	}

	//load music libs
	bool bMusicLibsLoadSuccess = LoadLevelMusicLibs(pLevelInfo);
	CRY_ASSERT(bMusicLibsLoadSuccess);

	//load mission xml
	bool bMissionXmlLoadSuccess = LoadMissionXml(pLevelInfo);
	CRY_ASSERT(bMissionXmlLoadSuccess);

	//AI objects are now registered, init AI system
	gEnv->pAISystem->Reset(IAISystem::RESET_ENTER_GAME);

	//load movie system (must be done after entities)	
	bool bMovieSystemLoadSuccess = LoadMovieSystem(pLevelInfo);
	CRY_ASSERT(bMovieSystemLoadSuccess);

	//delete old and create new current level (actually just a holder of pLevelInfo)
	CMiniLevel::Recreate(&m_pCurrentLevel, *pLevelInfo);

	//re-enable physics
	gEnv->pSystem->SetThreadState(ESubsys_Physics, true);

	IMaterialEffects* pMFX = pGameFramework->GetIMaterialEffects();
	if(pMFX)	//MiniGameFramework: NULL
		pMFX->PreLoadAssets();

	gEnv->pConsole->SetScrollMax(600/2);

	gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_NextLevel)->Clear();
	gEnv->pCryPak->Notify(ICryPak::EVENT_END_LOADLEVEL );
	
//	gEnv->pFlowSystem->Reset();	
	m_bLevelLoaded = true;

	return m_pCurrentLevel;
}

//-----------------------------------

ILevel*	simpleframework::CMiniLevelSystem::SetEditorLoadedLevel(const char* levelName)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(levelName);
	CRY_SAFE_RETURN(!levelName, NULL);

	CMiniLevelInfo* pLevelInfo = GetLevelInfoInternal(levelName);
//	CRY_ASSERT(pLevelInfo);	//Editor: might be NULL
	if (!pLevelInfo)
	{		
		OnLevelNotFound(levelName);
		return NULL;
	}

	m_lastLevelName = levelName;
	
	//read main level info.
	bool bLevelReadInfoSuccess = pLevelInfo->ReadInfo();
	CRY_ASSERT(bLevelReadInfoSuccess);
	if(!bLevelReadInfoSuccess)
	{
		OnLoadingError(pLevelInfo, "Failed to read level info (level.pak might be corrupted)!");
		return NULL;
	}
	
	//delete old and create new current level (actually just a holder of pLevelInfo)
	CMiniLevel::Recreate(&m_pCurrentLevel, *pLevelInfo);

	m_bLevelLoaded = true;

	return m_pCurrentLevel;
}

//-----------------------------------

bool	simpleframework::CMiniLevelSystem::IsLevelLoaded()
{
	LOG_CODE_COVERAGE();
	return m_bLevelLoaded;
}

//-----------------------------------

void	simpleframework::CMiniLevelSystem::PrepareNextLevel(const char* levelName)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(levelName);
	CRY_SAFE_RETURN(!levelName, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, CRY_NO_RETURN_VALUE);	

	m_tLevelLoadingStart = gEnv->pTimer->GetAsyncTime();

	CMiniLevelInfo* pLevelInfo = GetLevelInfoInternal(levelName);
	CRY_ASSERT(pLevelInfo);
	CRY_SAFE_RETURN(!pLevelInfo, CRY_NO_RETURN_VALUE);

	bool bOpenPakSuccess = gEnv->pCryPak->OpenPacks( pLevelInfo->GetPaks(), 0u );
	CRY_ASSERT(bOpenPakSuccess);
	CRY_SAFE_RETURN(!bOpenPakSuccess, CRY_NO_RETURN_VALUE);
	
	string filename = PathUtil::Make( pLevelInfo->GetPath(), "resourcelist.txt" );
	
	IResourceList* pRecResourceList = gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_NextLevel);
	CRY_ASSERT(pRecResourceList);
	CRY_SAFE_RETURN(!pRecResourceList, CRY_NO_RETURN_VALUE);
	pRecResourceList->Load( filename.c_str() );
}

//-----------------------------------

float	simpleframework::CMiniLevelSystem::GetLastLevelLoadTime()
{
	LOG_CODE_COVERAGE();
	return m_fLastLevelLoadTime;
}

//-- level loading/
//////////////////////////////////////////////////////////////////////////
//-- level rotation

ILevelRotation*	simpleframework::CMiniLevelSystem::GetLevelRotation()
{
	LOG_CODE_COVERAGE();
	return &m_levelRotation;
}

//-- level rotation/

//- ILevelSystem implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- ISystem::ILoadingProgressListener implementation

void	simpleframework::CMiniLevelSystem::OnLoadingProgress(int steps)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->p3DEngine);
	CRY_SAFE_RETURN(!gEnv->p3DEngine, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pTimer);
	CRY_SAFE_RETURN(!gEnv->pTimer, CRY_NO_RETURN_VALUE);	

	int nLoadedObjects = gEnv->p3DEngine->GetLoadedObjectCount();
	float fProgress = static_cast<float>(nLoadedObjects);
	m_fFilteredProgress = min(m_fFilteredProgress, fProgress);

	float fFrameTime = gEnv->pTimer->GetAsyncCurTime() - m_fLastTime;
	float t = CLAMP(fFrameTime*.25f, 0.0001f, 1.0f);
	m_fFilteredProgress = LERP(fProgress, m_fFilteredProgress, t);

	m_fLastTime = gEnv->pTimer->GetAsyncCurTime();

	OnLoadingProgress( m_pLoadingLevelInfo, (int)m_fFilteredProgress );
}
//- ISystem::ILoadingProgressListener implementation/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- internal level info getters

simpleframework::CMiniLevelInfo* simpleframework::CMiniLevelSystem::GetLevelInfoInternal(int level)
{
	if (level >= 0 && level < GetLevelCount())
	{
		return ( &(m_levelInfos.at(level)) );
	}
	return NULL;
}

//-----------------------------------

simpleframework::CMiniLevelInfo* simpleframework::CMiniLevelSystem::GetLevelInfoInternal(const char* levelName)
{
	//iterators needed for search
	CREATE_ITER_VARS(TLevelInfoVector::iterator, iter, m_levelInfos);
	
	//try full name search
	for (iter = iterStart; iter != iterEnd; ++iter)
	{
		if (!strcmpi(iter->GetName(), levelName))
			return (&(*iter));
	}
	
	//try comparing with only filename
	for (iter = iterStart; iter != iterEnd; ++iter)
	{
		if (!strcmpi(PathUtil::GetFileName(iter->GetName()), levelName))
			return (&(*iter));
	}

	//strip of path and try again with raw filename
	string sLevelName(levelName);
	size_t lastSlash = sLevelName.find_last_of('\\');
	if(lastSlash != string::npos)
	{
		sLevelName = sLevelName.substr(lastSlash + 1, sLevelName.size() - lastSlash - 1);
		return GetLevelInfoInternal(sLevelName.c_str());
	}

	//not found
	return 0;
}

//- internal level info getters/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- public utility

void simpleframework::CMiniLevelSystem::PrecacheLevelRenderData()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->p3DEngine);
	CRY_SAFE_RETURN(!gEnv->p3DEngine, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pEntitySystem);
	CRY_SAFE_RETURN(!gEnv->pEntitySystem, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pGame);
	CRY_SAFE_RETURN(!gEnv->pGame, CRY_NO_RETURN_VALUE);

	CRY_SAFE_RETURN(gEnv->pSystem->IsDedicated(), CRY_NO_RETURN_VALUE);

	ICVar* pCVe_PrecacheLevel = gEnv->pConsole->GetCVar("e_PrecacheLevel");		
	CRY_ASSERT(pCVe_PrecacheLevel);
	CRY_SAFE_RETURN(!pCVe_PrecacheLevel, CRY_NO_RETURN_VALUE);
	CRY_SAFE_RETURN(!pCVe_PrecacheLevel->GetIVal(), CRY_NO_RETURN_VALUE);


	std::vector<Vec3>	arrCamOutdoorPositions;
	IEntityItPtr	pEntityIter = gEnv->pEntitySystem->GetEntityIterator();
	IEntityClass*	pPrecacheCameraClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass( "PrecacheCamera" );
	IEntity*		pEntity = NULL;
	while (pEntity = pEntityIter->Next())
	{
		if (pEntity && pEntity->GetClass() == pPrecacheCameraClass)
		{
			arrCamOutdoorPositions.push_back( pEntity->GetWorldPos() );
		}
	}
	Vec3*	pPoints = 0;
	if (arrCamOutdoorPositions.size() > 0)
	{
		pPoints = &arrCamOutdoorPositions[0];
	}

	gEnv->p3DEngine->PrecacheLevel(true, pPoints, arrCamOutdoorPositions.size() );
}

//-----------------------------------

void simpleframework::CMiniLevelSystem::GetMemoryStatistics( ICrySizer* s )
{
	LOG_CODE_COVERAGE();

	s->Add(*this);
	s->AddContainer(m_levelInfos);
	s->Add(m_levelsFolder);
	s->AddContainer(m_listeners);

	FOR_EACH_LOOP(TLevelInfoVector::iterator, iter, m_levelInfos)
	{
		CHILD_MEM_STATISTICS((&(*iter)), s);
	}
}

//-----------------------------------

void simpleframework::CMiniLevelSystem::SaveOpenedFilesList()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(m_pLoadingLevelInfo);
	CRY_SAFE_RETURN(!m_pLoadingLevelInfo, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, CRY_NO_RETURN_VALUE);

	// Write resource list to pFile.
	IResourceList* pResList = gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_Level);
	CRY_ASSERT(pResList);
	CRY_SAFE_RETURN(!pResList, CRY_NO_RETURN_VALUE);

	string	filename = PathUtil::Make( m_pLoadingLevelInfo->GetPath(), "resourcelist.txt" );
	FILE*	pFile = fxopen( filename.c_str(), "wt", true );
	CRY_ASSERT(pFile);
	CRY_SAFE_RETURN(!pFile, CRY_NO_RETURN_VALUE);
	
	for(const char* fname = pResList->GetFirst(); fname != NULL; fname = pResList->GetNext())
	{
		fprintf( pFile,"%s\n",fname );
	}
	fclose(pFile);	
}

//- public utility/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- utility

void simpleframework::CMiniLevelSystem::SwitchToConsoleLoadingScreen()
{
	LOG_CODE_COVERAGE();

	//show console on load
	ICVar* pCVcon_showonload = gEnv->pConsole->GetCVar("con_showonload");	
	int con_showonload = 0;
	if(pCVcon_showonload) 
		con_showonload = pCVcon_showonload->GetIVal();

	if(con_showonload)	
	{
		//if console showing, don't display loading screen
		gEnv->pConsole->ShowConsole(true);
		ICVar* pCVg_enableloadingscreen = gEnv->pConsole->GetCVar("g_enableloadingscreen");
		if (pCVg_enableloadingscreen)
			pCVg_enableloadingscreen->Set(0);
	}
}

//-----------------------------------

string& simpleframework::CMiniLevelSystem::UnifyName(string& name)
{
	LOG_CODE_COVERAGE();
	
	//name.MakeLower();
	name.replace('\\', '/');

	return name;
}

//-----------------------------------

void simpleframework::CMiniLevelSystem::ScanFolder(const char* subfolder)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pCryPak);
	CRY_SAFE_RETURN(!gEnv->pCryPak, CRY_NO_RETURN_VALUE);	

	string folder;
	if (subfolder && subfolder[0])
		folder = subfolder;

	string search(m_levelsFolder);
	if (!folder.empty())
		search += string("/") + folder;
	search += "/*.*";

	_finddata_t fd;
	intptr_t handle = 0;

	
	handle = gEnv->pCryPak->FindFirst(search.c_str(), &fd);
	CRY_SAFE_RETURN(handle < 0, CRY_NO_RETURN_VALUE);
 
	do
	{
		if (!(fd.attrib & _A_SUBDIR) || !strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
			continue;

		CMiniLevelInfo	levelInfo;
		string levelFolder	= (folder.empty() ? 
			"" :
			(folder + "/")) + string(fd.name);

		string levelPath	= m_levelsFolder + "/" + levelFolder;
		string paks			= levelPath + string("/*.pak");

		bool bFileExist = gEnv->pCryPak->IsFileExist( levelPath + "/level.pak" );
		if (!bFileExist)
		{
			ScanFolder(levelFolder.c_str());
			continue;
		}

		levelInfo.m_levelPath = levelPath;
		levelInfo.m_levelPaks = paks;
		levelInfo.m_levelName = levelFolder;
		levelInfo.m_levelName = UnifyName(levelInfo.m_levelName);

		levelInfo.ReadMetaData();

		m_levelInfos.push_back(levelInfo);	
	} while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

	gEnv->pCryPak->FindClose(handle);
}

//-----------------------------------

void simpleframework::CMiniLevelSystem::LogLoadingTime()
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, CRY_NO_RETURN_VALUE);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, CRY_NO_RETURN_VALUE);

	CRY_SAFE_RETURN(gEnv->IsEditor(), CRY_NO_RETURN_VALUE);
	CRY_SAFE_RETURN(!gEnv->pSystem->IsDevMode(), CRY_NO_RETURN_VALUE);

#if defined(WIN32) && !defined(XENON)
	string filename( gEnv->pSystem->GetRootFolder() );
	filename.append( "Game_LevelLoadTime.log" );

	FILE* pFile = fxopen(filename.c_str(), "at");
	//CRY_ASSERT(pFile);
	CRY_SAFE_RETURN(!pFile, CRY_NO_RETURN_VALUE);

	static char vers[128];
	gEnv->pSystem->GetFileVersion().ToString(vers);

	const char* sChain = "";
	if (m_nLoadedLevelsCount > 0)
		sChain = " (Chained)";

	string text;
	text.Format( "\n[%s] Level %s loaded in %d seconds%s", 
		vers,
		m_lastLevelName.c_str(),
		(int)m_fLastLevelLoadTime,
		sChain );
	fwrite( text.c_str(), text.length(), 1, pFile );
	fclose(pFile);
#endif // Only log on windows.
}

//-----------------------------------

bool simpleframework::CMiniLevelSystem::LoadLevelMusicLibs(CMiniLevelInfo* pLevelInfo)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pLevelInfo);
	CRY_SAFE_RETURN(!pLevelInfo, false);
	
	CRY_ASSERT(gEnv->pMusicSystem);
	CRY_SAFE_RETURN(!gEnv->pMusicSystem, false);

	const ILevelInfo::TStringVec& musicLibs = pLevelInfo->GetMusicLibs();
	FOR_EACH_LOOP(ILevelInfo::TStringVec::const_iterator, iter, musicLibs)
	{
		gEnv->pMusicSystem->LoadFromXML(iter->c_str(), true, false);
	}
	return true;
}

//-----------------------------------

bool simpleframework::CMiniLevelSystem::LoadMissionXml(CMiniLevelInfo* pLevelInfo)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pLevelInfo);
	CRY_SAFE_RETURN(!pLevelInfo, false);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	CRY_ASSERT(gEnv->pSystem);
	CRY_SAFE_RETURN(!gEnv->pSystem, false);

	CRY_ASSERT(gEnv->pScriptSystem);
	CRY_SAFE_RETURN(!gEnv->pScriptSystem, false);

	CRY_ASSERT(gEnv->pEntitySystem);
	CRY_SAFE_RETURN(!gEnv->pEntitySystem, false);

	const ILevelInfo::TGameTypeInfo* pGameInfo = pLevelInfo->GetDefaultGameType();
	CRY_ASSERT(pGameInfo);
	CRY_SAFE_RETURN(!pGameInfo, false);

	string missionXml = pGameInfo->xmlFile;
	string xmlFile(pLevelInfo->GetPath());
	xmlFile.append("/");
	xmlFile.append(missionXml);

	XmlNodeRef rootNode = gEnv->pSystem->LoadXmlFile(xmlFile.c_str());
	CRY_ASSERT(rootNode);
	CRY_SAFE_RETURN(!rootNode, false);

	const char* script = rootNode->getAttr("Script");
	if (script && script[0])
		gEnv->pScriptSystem->ExecuteFile(script, true, true);

	XmlNodeRef objectsNode = rootNode->findChild("Objects");
	if (objectsNode)
		gEnv->pEntitySystem->LoadEntities(objectsNode);

	return true;
}

//-----------------------------------

bool simpleframework::CMiniLevelSystem::LoadMovieSystem(CMiniLevelInfo* pLevelInfo)
{
	LOG_CODE_COVERAGE();

	CRY_ASSERT(pLevelInfo);
	CRY_SAFE_RETURN(!pLevelInfo, false);

	CRY_ASSERT(gEnv);
	CRY_SAFE_RETURN(!gEnv, false);

	CRY_ASSERT(gEnv->pMovieSystem);
	CRY_SAFE_RETURN(!gEnv->pMovieSystem, false);

	string movieXml(pLevelInfo->GetPath());
	movieXml.append("/moviedata.xml");
	gEnv->pMovieSystem->Load(movieXml, pLevelInfo->GetDefaultGameType()->name);
	gEnv->pMovieSystem->Reset(true, true);

	return true;
}

//- utility/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//- CMiniLevelSystem::SLevelNameAutoComplete
//-- ctor & dtor

simpleframework::CMiniLevelSystem::CLevelNameAutoComplete::CLevelNameAutoComplete():
	ILevelInfo::TStringVec()
{
	LOG_CODE_COVERAGE();
}

//-----------------------------------

simpleframework::CMiniLevelSystem::CLevelNameAutoComplete::~CLevelNameAutoComplete()
{
	LOG_CODE_COVERAGE();
}

//-- ctor & dtor/
//////////////////////////////////////////////////////////////////////////
//-- IConsoleArgumentAutoComplete implementation

int simpleframework::CMiniLevelSystem::CLevelNameAutoComplete::GetCount() const
{
	LOG_CODE_COVERAGE();
	return size();
}

//-----------------------------------

const char* simpleframework::CMiniLevelSystem::CLevelNameAutoComplete::GetValue(int idx) const
{
	LOG_CODE_COVERAGE();
	return at(idx).c_str();
}

//-- IConsoleArgumentAutoComplete implementation/
//- //- CMiniLevelSystem::SLevelNameAutoComplete/
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include UNIQUE_VIRTUAL_WRAPPER(ILevelSystem)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////