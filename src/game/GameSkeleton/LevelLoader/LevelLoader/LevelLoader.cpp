
#include "StdAfx.h"
#include "LevelLoader.h"

#include <ICryPak.h>
#include <IConsole.h>
#include <I3DEngine.h>
#include <IPhysics.h>
#include <IEntitySystem.h>
#include <IScriptSystem.h>
#include <IAISystem.h>
#include <IMusicSystem.h>
#include <IMovieSystem.h>

#define DEFAULT_GAME_TYPE "Mission0"

/// Used by console auto completion.
struct SLevelNameAutoComplete : public IConsoleArgumentAutoComplete
{
	std::vector<string> levels;
	virtual int GetCount() const { return levels.size(); };
	virtual const char* GetValue( int nIndex ) const { return levels[nIndex].c_str(); };
};
// definition and declaration must be separated for devirtualization
SLevelNameAutoComplete g_LevelNameAutoComplete;

static void CmdMap(IConsoleCmdArgs* args);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CLevelInfo::CLevelInfo() : 
	m_heightmapSize(0),
	m_cgfCount(0)
{

}

CLevelInfo::~CLevelInfo()
{

}



bool CLevelInfo::ReadInfo()
{
	string levelPath = m_levelPath;
	string paks = levelPath + string("/*.pak");

	if (!gEnv->pCryPak->OpenPacks(paks.c_str(), (unsigned int)0))
	{
		return false;
	}

	string xmlFile = levelPath + string("/LevelInfo.xml");
	XmlNodeRef rootNode = GetISystem()->LoadXmlFile(xmlFile.c_str());

	if (rootNode)
	{
		string name = m_levelName;

		m_heightmapSize = atoi(rootNode->getAttr("HeightmapSize"));

		string dataFile = levelPath + string("/LevelDataAction.xml");
		XmlNodeRef dataNode = GetISystem()->LoadXmlFile(dataFile.c_str());
		if(!dataNode)
		{
			dataFile = levelPath + string("/LevelData.xml");
			dataNode = GetISystem()->LoadXmlFile(dataFile.c_str());
		}

		if (dataNode)
		{
			XmlNodeRef gameTypesNode = dataNode->findChild("Missions");

			if ((gameTypesNode!=0) && (gameTypesNode->getChildCount() > 0))
			{
				for (int i = 0; i < gameTypesNode->getChildCount(); i++)
				{
					XmlNodeRef gameTypeNode = gameTypesNode->getChild(i);

					if (gameTypeNode->isTag("Mission"))
					{
						m_xmlFile = gameTypeNode->getAttr("File");
						m_cgfCount = 0;
						gameTypeNode->getAttr("CGFCount", m_cgfCount);
					}
				}

				XmlNodeRef musicLibraryNode = dataNode->findChild("MusicLibrary");

				if ((musicLibraryNode!=0) && (musicLibraryNode->getChildCount() > 0))
				{
					for (int i = 0; i < musicLibraryNode->getChildCount(); i++)
					{
						XmlNodeRef musicLibrary = musicLibraryNode->getChild(i);

						if (musicLibrary->isTag("Library"))
						{
							const char* musicLibraryName = musicLibrary->getAttr("File");

							if (musicLibraryName)
							{
								m_musicLibs.push_back(string("music/") + musicLibraryName);
							}
						}
					}
				}
			}
		}
	}

	gEnv->pCryPak->ClosePacks(paks.c_str());
	return rootNode != 0;
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::ReadMetaData()
{
	string fullPath(GetPath());
	int slashPos = fullPath.rfind('\\');
	if(slashPos == -1)
	slashPos = fullPath.rfind('/');
	string mapName = fullPath.substr(slashPos+1, fullPath.length()-slashPos);
	fullPath.append("/");
	fullPath.append(mapName);
	fullPath.append(".xml");

	if (!gEnv->pCryPak->IsFileExist(fullPath.c_str()))
		return;

	XmlNodeRef mapInfo = GetISystem()->LoadXmlFile(fullPath.c_str());
	//retrieve the coordinates of the map
	if(mapInfo)
	{
		for(int n = 0; n < mapInfo->getChildCount(); ++n)
		{
			XmlNodeRef rulesNode = mapInfo->getChild(n);
			const char* name = rulesNode->getTag();
			if(!stricmp(name, "Gamerules"))
			{
				for(int a = 0; a < rulesNode->getNumAttributes(); ++a)
				{
					const char* key,* value;
					rulesNode->getAttributeByIndex(a, &key, &value);
					m_gamerules.push_back(value);
				}
			}
			if(!stricmp(name,"Display"))
			{
				XmlString v;
				if(rulesNode->getAttr("Name",v))
					m_levelDisplayName = v.c_str();
			}
		}
	}
}


//------------------------------------------------------------------------
CLevelLoader::CLevelLoader(ISystem* pSystem, const char* levelsFolder): 
	m_pSystem(pSystem),
	m_pLoadingLevelInfo(0)
{
	assert(pSystem);

	Rescan(levelsFolder);

	// register with system to get loading progress events
	m_pSystem->SetLoadingProgressListener(this);
	m_fLastLevelLoadTime = 0;
	m_fFilteredProgress = 0;
	m_fLastTime = 0;
	m_bLevelLoaded = false;
	m_bRecordingFileOpens = false;

	m_tLevelLoadingStart.SetValue(0);

	m_nLoadedLevelsCount = 0;

	gEnv->pConsole->RegisterAutoComplete( "map", &g_LevelNameAutoComplete );
	REGISTER_COMMAND("map", CmdMap, VF_RESTRICTEDMODE, "Loads the level given as argument");
}

//------------------------------------------------------------------------
CLevelLoader::~CLevelLoader()
{
	// register with system to get loading progress events
	m_pSystem->SetLoadingProgressListener(0);
	gEnv->pConsole->RemoveCommand("map");
}

//------------------------------------------------------------------------
CLevelInfo* CLevelLoader::GetLevelInfo(const char* levelName)
{
	// If level not found by full name try comparing with only filename
	for (std::vector<CLevelInfo>::iterator it = m_levelInfos.begin(); it != m_levelInfos.end(); it++)
	{
		if (!strcmpi(it->GetName(), levelName))
		{
			return &(*it);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	for (std::vector<CLevelInfo>::iterator it = m_levelInfos.begin(); it != m_levelInfos.end(); it++)
	{
		if (!strcmpi(PathUtil::GetFileName(it->GetName()), levelName))
		{
			return &(*it);
		}
	}

	return 0;
}

//------------------------------------------------------------------------
void CLevelLoader::Rescan(const char* levelsFolder)
{
	m_levelsFolder = levelsFolder;
	ScanFolder(0);
	g_LevelNameAutoComplete.levels.clear();
	for (int i = 0; i < (int)m_levelInfos.size(); i++)
	{
		g_LevelNameAutoComplete.levels.push_back( PathUtil::GetFileName(m_levelInfos[i].GetName()) );
	}
}

//------------------------------------------------------------------------
void CLevelLoader::ScanFolder(const char* subfolder)
{
	string folder;
	if (subfolder && subfolder[0])
		folder = subfolder;

	string search(m_levelsFolder);
	if (!folder.empty())
		search += string("/") + folder;
	search += "/*.*";

	ICryPak* pPak = gEnv->pCryPak;

	_finddata_t fd;
	intptr_t handle = 0;

	handle = pPak->FindFirst(search.c_str(), &fd);

	if (handle > -1)
	{
		do
		{
			if (!(fd.attrib & _A_SUBDIR) || !strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
			{
				continue;
			}

			CLevelInfo levelInfo;

			string levelFolder = (folder.empty() ? "" : (folder + "/")) + string(fd.name);
			string levelPath = m_levelsFolder + "/" + levelFolder;
			string paks = levelPath + string("/*.pak");

			if (!pPak->IsFileExist( levelPath + "/level.pak" ))
			{
				ScanFolder(levelFolder.c_str());
				continue;
			}

			levelInfo.m_levelPath = levelPath;
			levelInfo.m_levelPaks = paks;
			levelInfo.m_levelName = levelFolder;
			levelInfo.m_levelName = PathUtil::ToUnixPath(levelInfo.m_levelName);

			levelInfo.ReadMetaData();

			m_levelInfos.push_back(levelInfo);
		
		} while (pPak->FindNext(handle, &fd) >= 0);

		pPak->FindClose(handle);
	}
}


//------------------------------------------------------------------------
bool CLevelLoader::LoadLevel(const char* levelName)
{
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	assert(levelName);
	
	m_tLevelLoadingStart = gEnv->pTimer->GetAsyncTime();

	CLevelInfo* pLevelInfo = GetLevelInfo(levelName);

	if (!pLevelInfo)
	{
		// alert the listener
		OnLevelNotFound(levelName);

		return 0;
	}

	CryLog( "-----------------------------------------------------" );
	CryLog( "*LOADING: Loading Level %s",levelName );
	CryLog( "-----------------------------------------------------" );

	const bool bLoadingSameLevel = m_lastLevelName.compareNoCase(levelName) == 0;
	m_lastLevelName = levelName;

	//////////////////////////////////////////////////////////////////////////
	// Read main level info.
	if (!pLevelInfo->ReadInfo())
	{
		OnLoadingError(pLevelInfo, "Failed to read level info (level.pak might be corrupted)!");
		return 0;
	}
	//////////////////////////////////////////////////////////////////////////

	gEnv->pConsole->SetScrollMax(600);
	gEnv->pConsole->ShowConsole(true);

	//CCryAction::GetCryAction()->ClearBreakHistory();
	m_pLoadingLevelInfo = pLevelInfo;
	OnLoadingStart(pLevelInfo);

	// ensure a physical global area is present
	IMaterialManager* pMatMan = gEnv->p3DEngine->GetMaterialManager();	
	IPhysicalWorld* pPhysicalWorld = gEnv->pPhysicalWorld;
	IPhysicalEntity* pGlobalArea = pPhysicalWorld->AddGlobalArea();

	ICryPak* pPak = gEnv->pCryPak;
	pPak->Notify( ICryPak::EVENT_BEGIN_LOADLEVEL );

	if (!pPak->OpenPacks(pLevelInfo->GetPaks(), (unsigned int)0))
	{
		OnLoadingError(pLevelInfo, "");

		return 0;
	}
/*
	ICVar* pFileCache = gEnv->pConsole->GetCVar("sys_FileCache");		assert(pFileCache);

	if(pFileCache->GetIVal())
	{
		if(pPak->OpenPack("",pLevelInfo->GetPath()+string("/FileCache.dat")))
			gEnv->pLog->Log("FileCache.dat loaded");
		 else
			gEnv->pLog->Log("FileCache.dat not loaded");
	}
*/

	m_pSystem->SetThreadState(ESubsys_Physics, false);

	/*
	// Load game tokens.
	IGameTokenSystem* pGameTokenSystem = CCryAction::GetCryAction()->GetIGameTokenSystem();
	// if we're loading a new (different) level, we keep all non Level gametokens in the system
	if (bLoadingSameLevel == false)
	{
		// clear Level library
		pGameTokenSystem->RemoveLibrary("Level");
	}
	else
	{
		// someone wants to load the same level again (e.g. map level -> map level)
		// make sure we reset the gametokens and start with default values
		pGameTokenSystem->Reset();
	}
	// load all GameToken libraries this level uses incl. LevelLocal
	pGameTokenSystem->LoadLibs( pLevelInfo->GetPath() + string("/GameTokens/*.xml"));
	*/

	if (!gEnv->p3DEngine->LoadLevel(pLevelInfo->GetPath(), DEFAULT_GAME_TYPE))
	{
		OnLoadingError(pLevelInfo, "");

		return 0;
	}
	if (gEnv->pEntitySystem)
	{
		int nTerrainSize = gEnv->p3DEngine->GetTerrainSize();
		gEnv->pEntitySystem->ResizeProximityGrid( nTerrainSize,nTerrainSize );
	}

	// reset all the script timers
	gEnv->pScriptSystem->ResetTimers();

	// Reset sound system.
	if (gEnv->pSoundSystem)
	{
		gEnv->pSoundSystem->Silence(true, false);
		gEnv->pSoundSystem->Update(eSoundUpdateMode_All); // update once so sounds actually stop
	}

	if (gEnv->pAISystem)
	{
		gEnv->pAISystem->FlushSystem();
		bool loadAI = true;
		if (gEnv->bMultiplayer)
			if (ICVar*  pSvAI = gEnv->pConsole->GetCVar("sv_AISystem"))
				if (!pSvAI->GetIVal())
					loadAI = false;
		if (loadAI)
			gEnv->pAISystem->LoadNavigationData(pLevelInfo->GetPath(), DEFAULT_GAME_TYPE);
	}
  
	
	if (gEnv->pMusicSystem)
	{
		const ILevelInfo::TStringVec& musicLibs = pLevelInfo->GetMusicLibs();
		for (ILevelInfo::TStringVec::const_iterator i = musicLibs.begin(); i!= musicLibs.end(); ++i)
		{
			gEnv->pMusicSystem->LoadFromXML(*i, true, false);
		}
	}

	string missionXml = pLevelInfo->m_xmlFile;
	string xmlFile = string(pLevelInfo->GetPath()) + "/" + missionXml;

	XmlNodeRef rootNode = m_pSystem->LoadXmlFile(xmlFile.c_str());

	if (rootNode)
	{
		const char* script = rootNode->getAttr("Script");
		
		if (script && script[0])
			gEnv->pScriptSystem->ExecuteFile(script, true, true);

		XmlNodeRef objectsNode = rootNode->findChild("Objects");

		if (objectsNode)
			gEnv->pEntitySystem->LoadEntities(objectsNode);
	}

	// Now that we've registered our AI objects, we can init
	gEnv->pAISystem->Reset(IAISystem::RESET_ENTER_GAME);

	//////////////////////////////////////////////////////////////////////////
	// Movie system must be loaded after entities.
	//////////////////////////////////////////////////////////////////////////
	string movieXml = pLevelInfo->GetPath() + string("/moviedata.xml");
	IMovieSystem* movieSys = gEnv->pMovieSystem;
	if (movieSys != NULL)
	{
		movieSys->Load( movieXml, DEFAULT_GAME_TYPE );
		movieSys->Reset(true,true);
	}

	OnLoadingComplete( pLevelInfo );

	m_pSystem->SetThreadState(ESubsys_Physics, true);

	gEnv->pConsole->SetScrollMax(600/2);

	pPak->GetRecorderdResourceList(ICryPak::RFOM_NextLevel)->Clear();
	pPak->Notify( ICryPak::EVENT_END_LOADLEVEL );

	m_bLevelLoaded = true;

	return true;
}

//------------------------------------------------------------------------
void CLevelLoader::OnLoadingStart(ILevelInfo* pLevelInfo)
{
	m_fFilteredProgress = 0.f;
	m_fLastTime = gEnv->pTimer->GetAsyncCurTime();

	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_START,0,0 );
}

//------------------------------------------------------------------------
void CLevelLoader::OnLoadingComplete(ILevelInfo* pLevelInfo)
{
	//////////////////////////////////////////////////////////////////////////
	// Notify 3D engine that loading finished.
	//////////////////////////////////////////////////////////////////////////
	gEnv->pRenderer->BeginFrame();
	gEnv->pRenderer->EndFrame();

	gEnv->p3DEngine->PostLoadLevel();

	if (gEnv->pScriptSystem)
		gEnv->pScriptSystem->ForceGarbageCollection();

  //PrecacheLevelRenderData();
	
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_END,0,0 );
	
	// Hide console after loading.
	gEnv->pConsole->ShowConsole(false);
}

//------------------------------------------------------------------------
void CLevelLoader::OnLoadingProgress(int steps) 
{
	float fProgress = (float)gEnv->p3DEngine->GetLoadedObjectCount();
  m_fFilteredProgress = min(m_fFilteredProgress, fProgress);
  float fFrameTime = gEnv->pTimer->GetAsyncCurTime() - m_fLastTime;
  float t = CLAMP(fFrameTime*.25f, 0.0001f, 1.0f);
  m_fFilteredProgress = fProgress*t + m_fFilteredProgress*(1.f-t);
  m_fLastTime = gEnv->pTimer->GetAsyncCurTime();

	//OnLoadingProgress( m_pLoadingLevelInfo, (int)m_fFilteredProgress );

	// Report progress somehow here.
}

void CLevelLoader::OnLevelNotFound(const char* levelName)
{
	CryLogAlways( "Level %s not found",levelName );
}

void CLevelLoader::OnLoadingError(ILevelInfo* pLevel, const char* error)
{
	CryLogAlways( "Level loading failed, %s",error );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define	ERR_NO_LEVEL_PARAM	(CryLogAlways( "WARNING: No level parameter received" ))
#define ERR_IN_GAME_ONLY	(CryLogAlways( "WARNING: Works in Game mode only" ))
#define ERR_LEVEL_LOAD_FAIL	(CryLogAlways( "WARNING: Failed to load level" ))
extern CLevelLoader* g_pLevelLoader;

static void CmdMap(IConsoleCmdArgs* args)
{
	CRY_ASSERT(args);
	CRY_SAFE_RETURN(!args, ERR_NO_LEVEL_PARAM);
	CRY_SAFE_RETURN(!args->GetArgCount(), ERR_NO_LEVEL_PARAM);

	const char* levelName = args->GetArg(1);
	CRY_ASSERT(levelName);
	CRY_SAFE_RETURN(!levelName, ERR_NO_LEVEL_PARAM);

	bool bRes = g_pLevelLoader->LoadLevel(levelName);
	CRY_ASSERT(bRes);
	CRY_SAFE_RETURN(!levelName, ERR_LEVEL_LOAD_FAIL);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////