/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 17:8:2004   18:12 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "LevelSystem.h"
#include "Level.h"
#include "ActorSystem.h"
#include "IMusicSystem.h"
#include "IMovieSystem.h"
#include "CryAction.h"
#include "IGameTokens.h"
#include "IDialogSystem.h"
#include "TimeOfDayScheduler.h"
#include "IGameRulesSystem.h"
#include "IMaterialEffects.h"
#include "IPlayerProfiles.h"
#include "IGameStatistics.h"
#include <IResourceManager.h>

#include "LoadSeq.h"

#ifdef WIN32
#include <windows.h>
#endif

//------------------------------------------------------------------------
CLevelRotation::CLevelRotation()
: m_next(0),
  m_random(false)
{
}

//------------------------------------------------------------------------
CLevelRotation::~CLevelRotation()
{
}

//------------------------------------------------------------------------
bool CLevelRotation::Load(ILevelRotationFile* file)
{
	XmlNodeRef rootNode = file->Load();
	if (rootNode)
	{
		if (!stricmp(rootNode->getTag(), "levelrotation"))
		{
			Reset();

      bool r;
      if(rootNode->getAttr("randomize",r))
        SetRandom(r);

			int n=rootNode->getChildCount();

			for (int i=0;i<n;i++)
			{
				XmlNodeRef child=rootNode->getChild(i);
				if (child && !stricmp(child->getTag(), "level"))
				{
					const char *levelName=child->getAttr("name");
					if (!levelName || !levelName[0])
						continue;

					if(!CCryAction::GetCryAction()->GetILevelSystem()->GetLevelInfo(levelName)) //skipping non-present levels
						continue;

					const char *gameRulesName=child->getAttr("gamerules");
					if (!gameRulesName || !gameRulesName[0])
						gameRulesName=0;

					int lvl = AddLevel(levelName, gameRulesName);
          for(int j=0;j<child->getChildCount();++j)
          {
            XmlNodeRef s = child->getChild(j);
            if(s && !stricmp(s->getTag(), "setting"))
            {
              const char *setting = s->getAttr("setting");
              AddSetting(lvl, setting);
            }
          }
				}
			}

      if(IsRandom())
        Shuffle();
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool CLevelRotation::Save(ILevelRotationFile* file)
{
	XmlNodeRef rootNode=gEnv->pSystem->CreateXmlNode("levelRotation");

  rootNode->setAttr("randomize",m_random);

	for (int i=0; i<m_rotation.size(); i++)
	{
		SLevelRotationEntry &entry=m_rotation[i];

		XmlNodeRef level=gEnv->pSystem->CreateXmlNode("level");
		level->setAttr("name", entry.levelName.c_str());
		if (!entry.gameRulesName.empty())
			level->setAttr("gameRules", entry.gameRulesName.c_str());
    
    for(int j=0;j<entry.settings.size();++j)
    {
      XmlNodeRef s=gEnv->pSystem->CreateXmlNode("setting");
      s->setAttr("setting",entry.settings[j].c_str());
      level->addChild(s);
    }

    rootNode->addChild(level);
	}

	return file->Save(rootNode);
}

//------------------------------------------------------------------------
void CLevelRotation::Reset()
{
  m_random = false;
	m_next = 0;
	m_rotation.resize(0);
  m_shuffle.resize(0);
}

//------------------------------------------------------------------------
int CLevelRotation::AddLevel(const char *level, const char *gamerules)
{
	SLevelRotationEntry entry;
	entry.levelName=level;
	if (gamerules)
		entry.gameRulesName=gamerules;

  int idx = m_rotation.size();
	m_rotation.push_back(entry);
  if(m_random)
    m_shuffle.push_back(idx);
  return idx;
}

void CLevelRotation::AddSetting(int level, const char* setting)
{
  CRY_ASSERT(level>=0 && level<m_rotation.size());
  m_rotation[level].settings.push_back(setting);
}

//------------------------------------------------------------------------
bool CLevelRotation::First()
{
	m_next=0;
  if(m_random)
    Shuffle();
	return !m_rotation.empty();
}

//------------------------------------------------------------------------
bool CLevelRotation::Advance()
{
	++m_next;
  if (m_next>=m_rotation.size())
		return false;
	return true;
}

//------------------------------------------------------------------------
const char *CLevelRotation::GetNextLevel() const
{
  if (m_next>=0 && m_next<m_rotation.size())
  {
    int next = m_random?m_shuffle[m_next]:m_next;
		return m_rotation[next].levelName.c_str();
  }
	return 0;
}

//------------------------------------------------------------------------
const char *CLevelRotation::GetNextGameRules() const
{
	if (m_next>=0 && m_next<m_rotation.size())
  {
    int next = m_random?m_shuffle[m_next]:m_next;
    return m_rotation[next].gameRulesName.c_str();
  }
	return 0;
}

//------------------------------------------------------------------------

int  CLevelRotation::GetNextSettingsNum() const
{
  if (m_next>=0 && m_next<m_rotation.size())
  {
    int next = m_random?m_shuffle[m_next]:m_next;    
    return m_rotation[next].settings.size();
  }
  return 0;
}

//------------------------------------------------------------------------

const char *CLevelRotation::GetNextSetting(int idx)
{
  if (m_next>=0 && m_next<m_rotation.size())
  {
    int next = m_random?m_shuffle[m_next]:m_next;    
    if(idx >=0 && idx < m_rotation[next].settings.size())
      return m_rotation[next].settings[idx].c_str();
    else
      return 0;
  }
  return 0;
}

//------------------------------------------------------------------------

int CLevelRotation::GetLength() const
{
	return (int)m_rotation.size();
}

int CLevelRotation::GetNext() const
{
	return m_next;
}

//------------------------------------------------------------------------
void CLevelRotation::SetRandom(bool rnd)
{
  m_random = rnd;
  if(m_random)
    Shuffle();
}

//------------------------------------------------------------------------

bool CLevelRotation::IsRandom()const
{
  return m_random;
}

//------------------------------------------------------------------------
void CLevelRotation::ChangeLevel(IConsoleCmdArgs* pArgs)
{
	SGameContextParams ctx;
	const char *nextGameRules=GetNextGameRules();
	if (nextGameRules && nextGameRules[0])
		ctx.gameRules = nextGameRules;
	else if (IEntity *pEntity=CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRulesEntity())
		ctx.gameRules = pEntity->GetClass()->GetName();
	else if (ILevelInfo *pLevelInfo=CCryAction::GetCryAction()->GetILevelSystem()->GetLevelInfo(GetNextLevel()))
		ctx.gameRules = pLevelInfo->GetDefaultGameType()->name;

	ctx.levelName = GetNextLevel();

  for(int i=0;i<GetNextSettingsNum();++i)
  {
    gEnv->pConsole->ExecuteString(GetNextSetting(i));
  }
  
  if (CCryAction::GetCryAction()->StartedGameContext())
  {
		CCryAction::GetCryAction()->ChangeGameContext(&ctx);
  }
  else
  {
    gEnv->pConsole->ExecuteString( string("sv_gamerules ") + ctx.gameRules );

    string command = string("map ") + ctx.levelName;
    if (pArgs)
      for (int i = 1; i < pArgs->GetArgCount(); ++i)
        command += string(" ") + pArgs->GetArg(i);
		command += " s";
    gEnv->pConsole->ExecuteString(command);
  }
/*  else
  {
    int flags = eGSF_ImmersiveMultiplayer;

    SGameStartParams params;
    params.flags = flags | eGSF_Server;

    if (!gEnv->pSystem->IsDedicated())
    {
      params.flags |= eGSF_Client;
      params.hostname = "localhost";
    }
    
    ICVar* max_players = gEnv->pConsole->GetCVar("sv_maxplayers");
    params.maxPlayers = max_players?max_players->GetIVal():32; 
    ICVar* loading = gEnv->pConsole->GetCVar("g_enableloadingscreen");
    if(loading)
      loading->Set(0);
    
    params.recording = false;//TODO : talk to Lin

    params.pContextParams = &ctx;

    params.port = gEnv->pConsole->GetCVar("sv_port")->GetIVal();

    CCryAction::GetCryAction()->StartGameContext(&params);
  }*/
	
	if (!Advance())
		First();
}

void CLevelRotation::Shuffle()
{
  m_shuffle.resize(m_rotation.size());
  if(m_rotation.empty())
    return;

  for(int i=0;i<m_shuffle.size();++i)
    m_shuffle[i] = i;
  for(int i=0;i<m_shuffle.size();++i)
  {
    int idx = rand()%m_shuffle.size();
    std::swap(m_shuffle[i],m_shuffle[idx]);
  }
}

//------------------------------------------------------------------------
bool CLevelInfo::SupportsGameType(const char *gameTypeName) const
{
	//read level meta data
  for(int i=0;i<m_gamerules.size();++i)
  {
    if(!stricmp(m_gamerules[i].c_str(),gameTypeName))
      return true;
  }
  return false;
}

//------------------------------------------------------------------------
const char* CLevelInfo::GetDisplayName() const
{
  return m_levelDisplayName.c_str();
}

//////////////////////////////////////////////////////////////////////////
bool CLevelInfo::ReadInfo()
{
	string levelPath = m_levelPath;
	string paks = levelPath + string("/*.pak");

	if (!gEnv->pCryPak->OpenPacks(paks.c_str(), (unsigned int)0))
	{
		return false;
	}
#ifdef USING_LICENSE_PROTECTION
	bool decryptIt = false;
	string decryptionKey;
	if (GetISystem()->IsEncryptedLevel(levelPath))
	{
		decryptIt = true;
		ICVar* pDecryptionKeyCVar = gEnv->pConsole->GetCVar("level_decryption_key");
		if (pDecryptionKeyCVar)
			decryptionKey = pDecryptionKeyCVar->GetString();

		bool checkResult = GetISystem()->IsValidLicenseBuffer(levelPath, decryptionKey.c_str(), decryptionKey.length());
		if (false == checkResult)
		{
			const int ErrorMessageSize = 1024;
			char errorMessage[ErrorMessageSize] = {0,};
			sprintf_s(errorMessage, ErrorMessageSize, "Map %s could not be loaded.\nPlease check whether the right decryption key has been used", levelPath.c_str());
			MessageBox(NULL, errorMessage, "Load error", MB_ICONINFORMATION);
		}
	}
#endif
	
	string xmlFile = levelPath + string("/LevelInfo.xml");
#ifdef USING_LICENSE_PROTECTION
	XmlNodeRef rootNode = GetISystem()->LoadEncryptedXmlFile(xmlFile.c_str(), decryptIt,decryptionKey);
#else
	XmlNodeRef rootNode = GetISystem()->LoadXmlFile(xmlFile.c_str());
#endif
	if (rootNode)
	{
		string name = m_levelName;

		m_heightmapSize = atoi(rootNode->getAttr("HeightmapSize"));

		string dataFile = levelPath + string("/LevelDataAction.xml");
#ifdef USING_LICENSE_PROTECTION
		XmlNodeRef dataNode = GetISystem()->LoadEncryptedXmlFile(dataFile.c_str(), decryptIt, decryptionKey);
#else
		XmlNodeRef dataNode = GetISystem()->LoadXmlFile(dataFile.c_str());
#endif
		if(!dataNode)
		{
			dataFile = levelPath + string("/LevelData.xml");
#ifdef USING_LICENSE_PROTECTION
			dataNode = GetISystem()->LoadEncryptedXmlFile(dataFile.c_str(), decryptIt, decryptionKey);
#else
			dataNode = GetISystem()->LoadXmlFile(dataFile.c_str());
#endif
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
						const char *gameTypeName = gameTypeNode->getAttr("Name");

						if (gameTypeName)
						{
							ILevelInfo::TGameTypeInfo info;

							info.cgfCount = 0;
							gameTypeNode->getAttr("CGFCount", info.cgfCount);
							info.name = gameTypeNode->getAttr("Name");
							info.xmlFile = gameTypeNode->getAttr("File");
							m_gameTypes.push_back(info);
						}
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
							const char *musicLibraryName = musicLibrary->getAttr("File");

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
          const char *key, *value;
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
const ILevelInfo::TGameTypeInfo *CLevelInfo::GetDefaultGameType() const
{
	if (!m_gameTypes.empty())
	{
		return &m_gameTypes[0];
	}

	return 0;
};

/// Used by console auto completion.
struct SLevelNameAutoComplete : public IConsoleArgumentAutoComplete
{
	std::vector<string> levels;
	virtual int GetCount() const { return levels.size(); };
	virtual const char* GetValue( int nIndex ) const { return levels[nIndex].c_str(); };
};
// definition and declaration must be separated for devirtualization
SLevelNameAutoComplete g_LevelNameAutoComplete;

//------------------------------------------------------------------------
CLevelSystem::CLevelSystem(ISystem *pSystem, const char *levelsFolder)
: m_pSystem(pSystem),
	m_pCurrentLevel(0),
	m_pLoadingLevelInfo(0)
{
	CRY_ASSERT(pSystem);

	//if (!gEnv->IsEditor())
		Rescan(levelsFolder);

	// register with system to get loading progress events
	m_pSystem->SetLoadingProgressListener(this);
	m_fLastLevelLoadTime = 0;
	m_fFilteredProgress = 0;
	m_fLastTime = 0;
	m_bLevelLoaded = false;
	m_bRecordingFileOpens = false;

	m_levelLoadStartTime.SetValue(0);

	m_nLoadedLevelsCount = 0;

#ifndef SP_DEMO
	gEnv->pConsole->RegisterAutoComplete( "map",&g_LevelNameAutoComplete );
#endif
}

//------------------------------------------------------------------------
CLevelSystem::~CLevelSystem()
{
	// register with system to get loading progress events
	m_pSystem->SetLoadingProgressListener(0);
}

//------------------------------------------------------------------------
void CLevelSystem::Rescan(const char *levelsFolder)
{
	if (levelsFolder)
	{
		if (const ICmdLineArg *pModArg = m_pSystem->GetICmdLine()->FindArg(eCLAT_Pre,"MOD"))
		{
			if (m_pSystem->IsMODValid(pModArg->GetValue()))
			{
				m_levelsFolder.Format("Mods/%s/%s/%s", pModArg->GetValue(), PathUtil::GetGameFolder().c_str(), levelsFolder);
				ScanFolder(0);
			}
		}

		m_levelsFolder = levelsFolder;
	}

	CRY_ASSERT(!m_levelsFolder.empty());

	ScanFolder(0);

	g_LevelNameAutoComplete.levels.clear();
	for (int i = 0; i < (int)m_levelInfos.size(); i++)
	{
		g_LevelNameAutoComplete.levels.push_back( PathUtil::GetFileName(m_levelInfos[i].GetName()) );
	}
}

void CLevelSystem::LoadRotation()
{
  if (ICVar *pLevelRotation=gEnv->pConsole->GetCVar("sv_levelrotation"))
  {
    ILevelRotationFile* file = 0;
    IPlayerProfileManager *pProfileMan = CCryAction::GetCryAction()->GetIPlayerProfileManager();
    if(pProfileMan)
    {
      const char *userName = pProfileMan->GetCurrentUser();
      IPlayerProfile *pProfile = pProfileMan->GetCurrentProfile(userName);
      if(pProfile)
      {
        file = pProfile->GetLevelRotationFile(pLevelRotation->GetString());
      }
      else if (pProfile = pProfileMan->GetDefaultProfile())
      {
        file = pProfile->GetLevelRotationFile(pLevelRotation->GetString());
      }
    }
    bool ok = false;
    if(file)
    {
      ok = m_levelRotation.Load(file);
      file->Complete();
    }

    if(!ok)
      CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Failed to load '%s' as level rotation!", pLevelRotation->GetString());
  }
}

//------------------------------------------------------------------------
void CLevelSystem::ScanFolder(const char *subfolder)
{
	string folder;
	if (subfolder && subfolder[0])
		folder = subfolder;

	string search(m_levelsFolder);
	if (!folder.empty())
		search += string("/") + folder;
	search += "/*.*";

	ICryPak *pPak = gEnv->pCryPak;

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
			levelInfo.m_levelName = UnifyName(levelInfo.m_levelName);

			levelInfo.ReadMetaData();

			m_levelInfos.push_back(levelInfo);
		
		} while (pPak->FindNext(handle, &fd) >= 0);

		pPak->FindClose(handle);
	}
}

//------------------------------------------------------------------------
int CLevelSystem::GetLevelCount()
{
	return (int)m_levelInfos.size();
}

//------------------------------------------------------------------------
ILevelInfo *CLevelSystem::GetLevelInfo(int level)
{
	return GetLevelInfoInternal(level);
}

//------------------------------------------------------------------------
CLevelInfo *CLevelSystem::GetLevelInfoInternal(int level)
{
	if ((level >= 0) && (level < GetLevelCount()))
	{
		return &m_levelInfos[level];
	}

	return 0;
}

//------------------------------------------------------------------------
ILevelInfo *CLevelSystem::GetLevelInfo(const char *levelName)
{
	return GetLevelInfoInternal(levelName);
}

//------------------------------------------------------------------------
CLevelInfo *CLevelSystem::GetLevelInfoInternal(const char *levelName)
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

	// Try stripping out the folder to find the raw filename
	string sLevelName(levelName);
	size_t lastSlash = sLevelName.find_last_of('\\');
	if(lastSlash != string::npos)
	{
		sLevelName = sLevelName.substr(lastSlash + 1, sLevelName.size() - lastSlash - 1);
		return GetLevelInfoInternal(sLevelName.c_str());
	}

	return 0;
}

//------------------------------------------------------------------------
void CLevelSystem::AddListener(ILevelSystemListener *pListener)
{
	std::vector<ILevelSystemListener *>::iterator it = std::find(m_listeners.begin(), m_listeners.end(), pListener);

	if (it == m_listeners.end())
	{
		m_listeners.push_back(pListener);
	}
}

//------------------------------------------------------------------------
void CLevelSystem::RemoveListener(ILevelSystemListener *pListener)
{
	std::vector<ILevelSystemListener *>::iterator it = std::find(m_listeners.begin(), m_listeners.end(), pListener);

	if (it != m_listeners.end())
	{
		m_listeners.erase(it);
	}
}

//------------------------------------------------------------------------
ILevel *CLevelSystem::LoadLevel(const char *levelName)
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Level load (%s)", levelName);

	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_START_BEGIN,0,0 );
  LOADING_TIME_PROFILE_SECTION(GetISystem());

	CRY_ASSERT(levelName);
	
	m_levelLoadStartTime = gEnv->pTimer->GetAsyncTime();

	CLevelInfo *pLevelInfo = GetLevelInfoInternal(levelName);

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
	ICVar *con_showonload = gEnv->pConsole->GetCVar("con_showonload");
	if (con_showonload && con_showonload->GetIVal() != 0)
	{
		gEnv->pConsole->ShowConsole(true);
		ICVar *g_enableloadingscreen = gEnv->pConsole->GetCVar("g_enableloadingscreen");
		if (g_enableloadingscreen)
			g_enableloadingscreen->Set(0);
	}

	CCryAction::GetCryAction()->ClearBreakHistory();
	m_pLoadingLevelInfo = pLevelInfo;
	OnLoadingStart(pLevelInfo);

	// ensure a physical global area is present
	IMaterialManager* pMatMan = gEnv->p3DEngine->GetMaterialManager();    
	IPhysicalWorld *pPhysicalWorld = gEnv->pPhysicalWorld;
	IPhysicalEntity *pGlobalArea = pPhysicalWorld->AddGlobalArea();

	ICryPak *pPak = gEnv->pCryPak;
	pPak->Notify( ICryPak::EVENT_BEGIN_LOADLEVEL );

	// Inform resource manager about loading of the new level.
	GetISystem()->GetIResourceManager()->PrepareLevel( pLevelInfo->GetPath(),pLevelInfo->GetName() );

	if (!pPak->OpenPacks(pLevelInfo->GetPaks(), (unsigned int)0))
	{
		OnLoadingError(pLevelInfo, "");

		return 0;
	}
/*
	ICVar *pFileCache = gEnv->pConsole->GetCVar("sys_FileCache");		CRY_ASSERT(pFileCache);

	if(pFileCache->GetIVal())
	{
		if(pPak->OpenPack("",pLevelInfo->GetPath()+string("/FileCache.dat")))
			gEnv->pLog->Log("FileCache.dat loaded");
		 else
			gEnv->pLog->Log("FileCache.dat not loaded");
	}
*/

	m_pSystem->SetThreadState(ESubsys_Physics, false);

	ICVar *pSpamDelay = gEnv->pConsole->GetCVar("log_SpamDelay");
	float spamDelay = 0.0f;
	if (pSpamDelay)
	{
		spamDelay = pSpamDelay->GetFVal();
		pSpamDelay->Set(0.0f);
	}

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

	if (!gEnv->p3DEngine->LoadLevel(pLevelInfo->GetPath(), pLevelInfo->GetDefaultGameType()->name))
	{
		OnLoadingError(pLevelInfo, "");

		return 0;
	}
	if (gEnv->pEntitySystem)
	{
		int nTerrainSize = gEnv->p3DEngine->GetTerrainSize();
		gEnv->pEntitySystem->LoadLayers(pLevelInfo->GetPath() + string("/LevelData.xml"));
		gEnv->pEntitySystem->ResizeProximityGrid( nTerrainSize,nTerrainSize );
	}

	// reset all the script timers
	gEnv->pScriptSystem->ResetTimers();

	// Reset TimeOfDayScheduler
	CCryAction::GetCryAction()->GetTimeOfDayScheduler()->Reset();

	// Reset dialog system
	if (gEnv->pDialogSystem)
		gEnv->pDialogSystem->Reset();

	// Reset sound system.
	gEnv->pSoundSystem->Silence(true, false);
	gEnv->pSoundSystem->GetInterfaceExtended()->RecomputeSoundOcclusion(true,true,true);
	gEnv->pSoundSystem->Update(eSoundUpdateMode_All); // update once so sounds actually stop

#ifdef SP_DEMO
	if (!gEnv->IsEditor())
	{
		string sLevel=string('i')+string('s')+string('l')+string('a')+string('n')+string('d');
		if (!strstr(pLevelInfo->GetName(),sLevel.c_str()))
			gEnv->pAISystem=NULL;
	}
#endif

	if (gEnv->pAISystem)
	{
		gEnv->pAISystem->FlushSystem();
		CCryAction::GetCryAction()->DestroyAllAIActorProxies();
		bool loadAI = true;
		if (gEnv->bMultiplayer)
			if (ICVar * pSvAI = gEnv->pConsole->GetCVar("sv_AISystem"))
				if (!pSvAI->GetIVal())
					loadAI = false;
		if (loadAI)
		{
			gEnv->pAISystem->SetLevelPath(pLevelInfo->GetPath());
			gEnv->pAISystem->LoadNavigationData(pLevelInfo->GetPath(), pLevelInfo->GetDefaultGameType()->name);
			gEnv->pAISystem->LoadCover(pLevelInfo->GetPath(), pLevelInfo->GetDefaultGameType()->name);
	}
	}
		
	const ILevelInfo::TStringVec& musicLibs = pLevelInfo->GetMusicLibs();
	for (ILevelInfo::TStringVec::const_iterator i = musicLibs.begin(); i!= musicLibs.end(); ++i)
	{
		gEnv->pMusicSystem->LoadFromXML(*i, true, false);
	}

	string missionXml = pLevelInfo->GetDefaultGameType()->xmlFile;
	string xmlFile = string(pLevelInfo->GetPath()) + "/" + missionXml;

	XmlNodeRef rootNode = m_pSystem->LoadXmlFile(xmlFile.c_str());

	if (rootNode)
	{
		const char *script = rootNode->getAttr("Script");
		
		if (script && script[0])
			gEnv->pScriptSystem->ExecuteFile(script, true, true);

		XmlNodeRef objectsNode = rootNode->findChild("Objects");

		if (objectsNode)
			gEnv->pEntitySystem->LoadEntities(objectsNode);
	}

	// Now that we've registered our AI objects, we can init
	if (gEnv->pAISystem)
		gEnv->pAISystem->Reset(IAISystem::RESET_ENTER_GAME);

	//////////////////////////////////////////////////////////////////////////
	// Movie system must be loaded after entities.
	//////////////////////////////////////////////////////////////////////////
	string movieXml = pLevelInfo->GetPath() + string("/moviedata.xml");
	IMovieSystem *movieSys = gEnv->pMovieSystem;
	if (movieSys != NULL)
	{
		movieSys->Load( movieXml, pLevelInfo->GetDefaultGameType()->name );
		movieSys->Reset(true,true);
	}

	delete m_pCurrentLevel;
	CLevel *pLevel = new CLevel();
	pLevel->m_levelInfo = *pLevelInfo;
	m_pCurrentLevel = pLevel;

	m_pSystem->SetThreadState(ESubsys_Physics, true);

	CCryAction::GetCryAction()->GetIMaterialEffects()->PreLoadAssets();

	gEnv->pConsole->SetScrollMax(600/2);

	pPak->GetRecorderdResourceList(ICryPak::RFOM_NextLevel)->Clear();
	pPak->Notify( ICryPak::EVENT_END_LOADLEVEL );
	
	gEnv->pFlowSystem->Reset();

	if (pSpamDelay)
		pSpamDelay->Set(spamDelay);

	
	m_bLevelLoaded = true;

	return m_pCurrentLevel;
}

//------------------------------------------------------------------------
ILevel* CLevelSystem::SetEditorLoadedLevel(const char *levelName)
{
	CLevelInfo *pLevelInfo = GetLevelInfoInternal(levelName);

	if (!pLevelInfo)
	{
		// alert the listener
		OnLevelNotFound(levelName);

		return 0;
	}

	m_lastLevelName = levelName;

	//////////////////////////////////////////////////////////////////////////
	// Read main level info.
	if (!pLevelInfo->ReadInfo())
	{
		OnLoadingError(pLevelInfo, "Failed to read level info (level.pak might be corrupted)!");
		return 0;
	}
	//////////////////////////////////////////////////////////////////////////

	delete m_pCurrentLevel;
	CLevel *pLevel = new CLevel();
	pLevel->m_levelInfo = *pLevelInfo;
	m_pCurrentLevel = pLevel;

	m_bLevelLoaded = true;

	return m_pCurrentLevel;
}

//------------------------------------------------------------------------
// Search positions of all entities with class "PrecacheCamera" and pass it to the 3dengine
void CLevelSystem::PrecacheLevelRenderData()
{
	if (gEnv->pSystem->IsDedicated())
		return;


//Diesel new 3203
	gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_START, NULL, NULL);




	ICVar *pPrecacheVar = gEnv->pConsole->GetCVar("e_PrecacheLevel");		CRY_ASSERT(pPrecacheVar);

	if(pPrecacheVar && pPrecacheVar->GetIVal()==0)
		return;

	if (IGame *pGame = gEnv->pGame)
		if(I3DEngine * p3DEngine = gEnv->p3DEngine)
	{
		std::vector<Vec3> arrCamOutdoorPositions;

		IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
		IEntityItPtr pEntityIter = pEntitySystem->GetEntityIterator();

		IEntityClass *pPrecacheCameraClass = pEntitySystem->GetClassRegistry()->FindClass( "PrecacheCamera" );
		IEntity *pEntity = NULL;
		while (pEntity = pEntityIter->Next())
		{
			if (pEntity->GetClass() == pPrecacheCameraClass)
			{
				arrCamOutdoorPositions.push_back( pEntity->GetWorldPos() );
			}
		}
		Vec3 *pPoints = 0;
		if (arrCamOutdoorPositions.size() > 0)
		{
			pPoints = &arrCamOutdoorPositions[0];
		}

		p3DEngine->PrecacheLevel(true, pPoints,arrCamOutdoorPositions.size() );
	}
}

//------------------------------------------------------------------------
void CLevelSystem::PrepareNextLevel(const char *levelName)
{
	m_levelLoadStartTime = gEnv->pTimer->GetAsyncTime();
	CLevelInfo *pLevelInfo = GetLevelInfoInternal(levelName);
	if (!pLevelInfo)
	{
		// alert the listener
		//OnLevelNotFound(levelName);

		return;
	}
	if (!gEnv->pCryPak->OpenPacks(pLevelInfo->GetPaks(), (unsigned int)0))
	{
		//OnLoadingError(pLevelInfo, "");
		return;
	}

	// Disable locking of resources to allow everything to be offloaded.

	//string filename = PathUtil::Make( pLevelInfo->GetPath(),"resourcelist.txt" );
	//gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_NextLevel)->Load( filename.c_str() );
}

//------------------------------------------------------------------------
void CLevelSystem::OnLevelNotFound(const char *levelName)
{
	for (std::vector<ILevelSystemListener *>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		(*it)->OnLevelNotFound(levelName);
	}
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingStart(ILevelInfo *pLevelInfo)
{
#ifdef SP_DEMO
	if (!gEnv->IsEditor())
	{
		string sLevel=string('i')+string('s')+string('l')+string('a')+string('n')+string('d');
		if (!strstr(pLevelInfo->GetName(),sLevel.c_str()))
			return;
	}
#endif

	m_fFilteredProgress = 0.f;
	m_fLastTime = gEnv->pTimer->GetAsyncCurTime();

	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_START,0,0 );

#ifdef WIN32
	/*
	m_bRecordingFileOpens = GetISystem()->IsDevMode() && gEnv->pCryPak->GetRecordFileOpenList() == ICryPak::RFOM_Disabled;
	if (m_bRecordingFileOpens)
	{
		gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_Level)->Clear();
		gEnv->pCryPak->RecordFileOpen(ICryPak::RFOM_Level);
	}
	*/
#endif

	for (std::vector<ILevelSystemListener *>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		(*it)->OnLoadingStart(pLevelInfo);
	}

	BEGIN_LOADING(pLevelInfo->GetName());
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingError(ILevelInfo *pLevelInfo, const char *error)
{
	if (!pLevelInfo)
		pLevelInfo = m_pLoadingLevelInfo;
	if (!pLevelInfo)
	{
		CRY_ASSERT(false);
		GameWarning("OnLoadingError without a currently loading level");
	}

	for (std::vector<ILevelSystemListener *>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		(*it)->OnLoadingError(pLevelInfo, error);
	}

	END_LOADING(pLevelInfo->GetName());
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingComplete(ILevel *pLevelInfo)
{
	// let gamerules precache anything needed
	if (IGameRules *pGameRules=CCryAction::GetCryAction()->GetIGameRulesSystem()->GetCurrentGameRules())
		pGameRules->PrecacheLevel();

	//////////////////////////////////////////////////////////////////////////
	// Notify 3D engine that loading finished.
	//////////////////////////////////////////////////////////////////////////
	gEnv->p3DEngine->PostLoadLevel();

	if (gEnv->pScriptSystem)
		gEnv->pScriptSystem->ForceGarbageCollection();

  PrecacheLevelRenderData();

	for (std::vector<ILevelSystemListener *>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		(*it)->OnLoadingComplete(pLevelInfo);
	}
	
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_END,0,0 );
	
	if (m_bRecordingFileOpens)
	{
		// Stop recoding file opens.
		gEnv->pCryPak->RecordFileOpen(ICryPak::RFOM_Disabled);
		// Save recorded list.
		SaveOpenedFilesList();
	}

	CTimeValue t = gEnv->pTimer->GetAsyncTime();
	m_fLastLevelLoadTime = (t-m_levelLoadStartTime).GetSeconds();
	
	if (!gEnv->IsEditor())
	{
		CryLogAlways( "-----------------------------------------------------" );
		CryLogAlways( "*LOADING: Level %s loading time: %.2f seconds",m_lastLevelName.c_str(),m_fLastLevelLoadTime );
		CryLogAlways( "-----------------------------------------------------" );
	}

	LogLoadingTime();

	m_nLoadedLevelsCount++;

	// Hide console after loading.
	gEnv->pConsole->ShowConsole(false);

	if (!gEnv->bServer)
		return;

	/*
	if( IStatsTracker* tr = CCryAction::GetCryAction()->GetIGameStatistics()->GetSessionTracker() )
	{
		string mapName = "no_map_assigned";
		if( pLevelInfo )
			if (ILevelInfo * pInfo = pLevelInfo->GetLevelInfo())
			{
				mapName = pInfo->GetName();
				PathUtil::RemoveExtension(mapName);
				mapName = PathUtil::GetFileName(mapName);
				mapName.MakeLower();
			}
		tr->StateValue(eSP_Map, mapName.c_str());
	}
	*/
	END_LOADING(pLevelInfo->GetLevelInfo()->GetName());
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingProgress(ILevelInfo *pLevel, int progressAmount)
{
	for (std::vector<ILevelSystemListener *>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
	{
		(*it)->OnLoadingProgress(pLevel, progressAmount);
	}
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingProgress(int steps) 
{
	float fProgress = (float)gEnv->p3DEngine->GetLoadedObjectCount();

  m_fFilteredProgress = min(m_fFilteredProgress, fProgress);

  float fFrameTime = gEnv->pTimer->GetAsyncCurTime() - m_fLastTime;

  float t = CLAMP(fFrameTime*.25f, 0.0001f, 1.0f);

  m_fFilteredProgress = fProgress*t + m_fFilteredProgress*(1.f-t);

  m_fLastTime = gEnv->pTimer->GetAsyncCurTime();

	OnLoadingProgress( m_pLoadingLevelInfo, (int)m_fFilteredProgress );
}

//------------------------------------------------------------------------
string& CLevelSystem::UnifyName(string& name)
{
	//name.MakeLower();
	name.replace('\\', '/');

	return name;
}

//////////////////////////////////////////////////////////////////////////
void CLevelSystem::LogLoadingTime()
{
	if (gEnv->IsEditor())
		return;

	if (!GetISystem()->IsDevMode())
		return;

#if defined(WIN32) && !defined(XENON)
	string filename = gEnv->pSystem->GetRootFolder();
	filename += "Game_LevelLoadTime.log";

	FILE *file = fxopen(filename,"at");
	if (!file)
		return;

	char vers[128];
	GetISystem()->GetFileVersion().ToString(vers);

	const char *sChain = "";
	if (m_nLoadedLevelsCount > 0)
		sChain = " (Chained)";

	string text;
	text.Format( "\n[%s] Level %s loaded in %d seconds%s",vers,m_lastLevelName.c_str(),(int)m_fLastLevelLoadTime,sChain );
	fwrite( text.c_str(),text.length(),1,file );
	fclose(file);

#endif // Only log on windows.
}

void CLevelSystem::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
	s->AddContainer(m_levelInfos);
	s->Add(m_levelsFolder);
	s->AddContainer(m_listeners);
	for (size_t i=0; i<m_levelInfos.size(); ++i)
	{
		m_levelInfos[i].GetMemoryStatistics(s);
	}
}

void CLevelInfo::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(m_levelName);
	s->Add(m_levelPath);
	s->Add(m_levelPaks);
	s->AddContainer(m_musicLibs);
	for (size_t i=0; i<m_musicLibs.size(); i++)
		s->Add(m_musicLibs[i]);
	s->AddContainer(m_gameTypes);
	for (size_t i=0; i<m_gameTypes.size(); i++)
	{
		s->Add(m_gameTypes[i].name);
		s->Add(m_gameTypes[i].xmlFile);
	}
}

//////////////////////////////////////////////////////////////////////////
void CLevelSystem::SaveOpenedFilesList()
{
	if (!m_pLoadingLevelInfo)
		return;

	// Write resource list to file.
	string filename = PathUtil::Make( m_pLoadingLevelInfo->GetPath(),"resourcelist.txt" );
	FILE *file = fxopen( filename.c_str(),"wt",true );
	if (file)
	{
		IResourceList *pResList = gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_Level);
		for (const char *fname = pResList->GetFirst(); fname; fname = pResList->GetNext())
		{
			fprintf( file,"%s\n",fname );
		}
		fclose(file);
	}
}

//////////////////////////////////////////////////////////////////////////
void CLevelSystem::UnLoadLevel()
{
	//AM: Flush render thread (Flush is not exposed - using EndFrame())
	//We are about to delete resources that could be in use
	if (gEnv->pRenderer)
	{
		gEnv->pRenderer->EndFrame();
	}

	CCryAction::GetCryAction()->GetIItemSystem()->Reset();

	if (gEnv->pEntitySystem)
	{
		gEnv->pEntitySystem->Reset(); // This needs to be called for editor and game, otherwise entities won't be released
	}

	if (!gEnv->IsEditor())
	{
		CCryAction::GetCryAction()->GetIItemSystem()->ClearGeometryCache();
		CCryAction::GetCryAction()->GetIItemSystem()->ClearSoundCache();

		if (gEnv->pMovieSystem)
		{
			gEnv->pMovieSystem->Reset(false,false);
		}

		// Mute and Reset Sound System and unload Data
		if (gEnv->pSoundSystem)
		{
			gEnv->pSoundSystem->Silence(true, true);
			gEnv->pSoundSystem->GetInterfaceExtended()->RecomputeSoundOcclusion(true,true,true);
			gEnv->pSoundSystem->Update(eSoundUpdateMode_All); // update once so sounds actually stop
		}

		// Delete 3d engine resources
		if (gEnv->p3DEngine)
		{
			gEnv->p3DEngine->UnloadLevel();
		}
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(ILevelSystem)
#include UNIQUE_VIRTUAL_WRAPPER(ILevelInfo)
#include UNIQUE_VIRTUAL_WRAPPER(ILevelRotation)
