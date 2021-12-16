////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   gameengine.cpp
//  Version:     v1.00
//  Created:     13/5/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "gameengine.h"

#include "CryEditDoc.h"
#include "Objects\ObjectManager.h"
#include "Objects\EntityScript.h"
#include "Objects/AIWave.h"
#include "Objects/ShapeObject.h"
#include "Geometry\EdMesh.h"
#include "Mission.h"
#include "GameExporter.h"
#include ".\Terrain\SurfaceType.h"
#include "VegetationMap.h"
#include ".\Terrain\Heightmap.h"
#include ".\Terrain\TerrainGrid.h"
#include "ViewManager.h"
#include "StartupLogoDialog.h"

#include "AI\AIManager.h"
#include "AI\CoverSurfaceManager.h"
#include "Objects\AICoverSurface.h"
#include "AI\NavDataGeneration\Navigation.h"
#include "Material\MaterialManager.h"
#include "Particles\ParticleManager.h"
#include "HyperGraph\FlowGraphManager.h"
#include "UIEnumsDatabase.h"
#include "EquipPackLib.h"
#include "Util\Ruler.h"

#include <IAgent.h>
#include <I3DEngine.h>
#include <IAISystem.h>
#include <IEntitySystem.h>
#include <IMovieSystem.h>
#include <IInput.h>
#include <IScriptSystem.h>
#include <ICryPak.h>
#include <IPhysics.h>
#include <IDataProbe.h>
#include <IEditorGame.h>
#include <ITimer.h>
#include <IFlowSystem.h>
#include <ICryAnimation.h>
#include <IAnimationGraph.h>
#include <IGame.h> // Music Logic
#include <IGameFramework.h> // Music Logic
#include <IHardwareMouse.h>
#include <IDialogSystem.h>

//////////////////////////////////////////////////////////////////////////
#include "platform_impl.h"
//////////////////////////////////////////////////////////////////////////

#include "Console/ConsoleSync.h"

//////////////////////////////////////////////////////////////////////////
// Implementation of System Callback structure.
//////////////////////////////////////////////////////////////////////////
struct SSystemUserCallback : public ISystemUserCallback
{
	SSystemUserCallback( IInitializeUIInfo *logo ) { m_pLogo = logo; };

	// interface ISystemUserCallback ---------------------------------------------------

	virtual void OnSystemConnect( ISystem *pSystem )
	{
		ModuleInitISystem(pSystem,"Editor");
	}

	virtual bool OnError( const char *szErrorString )
	{
		if (szErrorString)
			Log( szErrorString );
		if (GetIEditor()->IsInTestMode())
		{
			exit(1);
		}
		char str[4096];
		if (szErrorString)
			sprintf( str,"%s\r\nSave Level Before Exiting the Editor?",szErrorString );
		else
			sprintf( str,"Unknown Error\r\nSave Level Before Exiting the Editor?" );
		int res = MessageBox( NULL,str,"Engine Error",MB_YESNOCANCEL|MB_ICONERROR|MB_TOPMOST|MB_APPLMODAL|MB_DEFAULT_DESKTOP_ONLY);
		if (res == IDYES || res == IDNO)
		{
			if (res == IDYES)
			{
				if (GetIEditor()->SaveDocument())
					MessageBox( NULL,"Level has been sucessfully saved!\r\nPress Ok to terminate Editor.","Save",MB_OK );
			}
		}
		return true;
	}
	virtual bool OnSaveDocument()
	{
		return GetIEditor()->SaveDocumentBackup();
	}
	virtual void OnProcessSwitch()
	{
		if (GetIEditor()->IsInGameMode())
			GetIEditor()->SetInGameMode(false);
	}
	virtual void OnInitProgress( const char *sProgressMsg )
	{
		if (m_pLogo)
			m_pLogo->SetInfoText( sProgressMsg );
	}
	
	// to collect the memory information in the user program/application
	virtual void GetMemoryUsage( ICrySizer* pSizer )
	{
		GetIEditor()->GetMemoryUsage(pSizer);
	}

private: // ---------------------------------------------------------------------------

	IInitializeUIInfo *m_pLogo;
};

//////////////////////////////////////////////////////////////////////////
// Implements EntitySystemSink for InGame mode.
//////////////////////////////////////////////////////////////////////////
struct SInGameEntitySystemListener  : public IEntitySystemSink
{
	SInGameEntitySystemListener() {}
	~SInGameEntitySystemListener()
	{
		// Remove all remaining entities from entity system.
		IEntitySystem *pEntitySystem = GetIEditor()->GetSystem()->GetIEntitySystem();
		for (std::set<int>::iterator it = m_entities.begin(); it != m_entities.end(); ++it)
		{
			EntityId entityId = *it;
			IEntity *pEntity = pEntitySystem->GetEntity(entityId);
			if (pEntity)
			{
				IEntity *pParent = pEntity->GetParent();
				if (pParent)
				{
					// Childs of unremovable entity are also not deleted.. (Needed for vehicles weapons for example)
					if (pParent->GetFlags() & ENTITY_FLAG_UNREMOVABLE)
						continue;
				}
			}
			pEntitySystem->RemoveEntity(*it, true);
		}
	}

	virtual bool OnBeforeSpawn( SEntitySpawnParams &params )
	{
		return true;
	}

	virtual void OnSpawn( IEntity *e, SEntitySpawnParams &params  )
	{
		//if (params.ed.ClassId!=0 && ed.ClassId!=PLAYER_CLASS_ID) // Ignore MainPlayer
		if (!(e->GetFlags() & ENTITY_FLAG_UNREMOVABLE))
		{
			m_entities.insert(e->GetId());
		}
	}
	virtual bool OnRemove( IEntity *e )
	{
		if (!(e->GetFlags()& ENTITY_FLAG_UNREMOVABLE))
			m_entities.erase(e->GetId());
		return true;
	}
	virtual void OnEvent( IEntity *pEntity, SEntityEvent &event )
	{
	}
private:
	// Ids of all spawned entities.
	std::set<int> m_entities;
};

namespace
{
	SInGameEntitySystemListener* s_InGameEntityListener = 0;
};

//////////////////////////////////////////////////////////////////////////
// Timur.
// This is FarCry.exe authentication function, this code is not for public release!!
//////////////////////////////////////////////////////////////////////////
static void GameSystemAuthCheckFunction( void *data )
{
	// src and trg can be the same pointer (in place encryption)
	// len must be in bytes and must be multiple of 8 byts (64bits).
	// key is 128bit:  int key[4] = {n1,n2,n3,n4};
	// void encipher(unsigned int *const v,unsigned int *const w,const unsigned int *const k )
#define TEA_ENCODE( src,trg,len,key ) {\
	register unsigned int *v = (src), *w = (trg), *k = (key), nlen = (len) >> 3; \
	register unsigned int delta=0x9E3779B9,a=k[0],b=k[1],c=k[2],d=k[3]; \
	while (nlen--) {\
	register unsigned int y=v[0],z=v[1],n=32,sum=0; \
	while(n-->0) { sum += delta; y += (z << 4)+a ^ z+sum ^ (z >> 5)+b; z += (y << 4)+c ^ y+sum ^ (y >> 5)+d; } \
	w[0]=y; w[1]=z; v+=2,w+=2; }}

	// src and trg can be the same pointer (in place decryption)
	// len must be in bytes and must be multiple of 8 byts (64bits).
	// key is 128bit: int key[4] = {n1,n2,n3,n4};
	// void decipher(unsigned int *const v,unsigned int *const w,const unsigned int *const k)
#define TEA_DECODE( src,trg,len,key ) {\
	register unsigned int *v = (src), *w = (trg), *k = (key), nlen = (len) >> 3; \
	register unsigned int delta=0x9E3779B9,a=k[0],b=k[1],c=k[2],d=k[3]; \
	while (nlen--) { \
	register unsigned int y=v[0],z=v[1],sum=0xC6EF3720,n=32; \
	while(n-->0) { z -= (y << 4)+c ^ y+sum ^ (y >> 5)+d; y -= (z << 4)+a ^ z+sum ^ (z >> 5)+b; sum -= delta; } \
	w[0]=y; w[1]=z; v+=2,w+=2; }}

	// Data assumed to be 32 bytes.
	int key1[4] = {1178362782,223786232,371615531,90884141};
	TEA_DECODE( (unsigned int*)data,(unsigned int*)data,32,(unsigned int*)key1 );
	int key2[4] = {89158165, 1389745433,971685123,785741042};
	TEA_ENCODE( (unsigned int*)data,(unsigned int*)data,32,(unsigned int*)key2 );
}

//////////////////////////////////////////////////////////////////////////
// This class implements calls from the game to the editor.
//////////////////////////////////////////////////////////////////////////
class CGameToEditorInterface : public IGameToEditorInterface
{
	virtual void SetUIEnums( const char *sEnumName,const char **sStringsArray,int nStringCount )
	{
		GetIEditor()->GetUIEnumsDatabase()->SetEnumStrings( sEnumName,sStringsArray,nStringCount );
	}
};

//////////////////////////////////////////////////////////////////////////
CGameEngine::CGameEngine()
: m_gameDll(0),
	m_pEditorGame(0)
{
	m_ISystem = NULL;

	m_pNavigation = 0;

	m_bLevelLoaded = false;
	m_inGameMode = false;
	m_simulationMode = false;
	m_simulationModeAI = false;
	m_syncPlayerPosition = true;
	m_hSystemHandle = 0;
	m_bUpdateFlowSystem = false;
	m_bJustCreated = false;

	m_pGameToEditorInterface = new CGameToEditorInterface;

	m_levelName = "Untitled";

	m_playerViewTM.SetIdentity();

	GetIEditor()->RegisterNotifyListener(this);
}


//////////////////////////////////////////////////////////////////////////
CGameEngine::~CGameEngine()
{
	GetIEditor()->UnregisterNotifyListener(this);

	if (m_pEditorGame)
	{
		m_pEditorGame->Shutdown();
	}

	// Release all EdMEshes.
	CEdMesh::ReleaseAll();

	// Disable callback to editor.
	m_ISystem->GetIMovieSystem()->SetCallback( NULL );
	
	if (m_ISystem)
		m_ISystem->Release();
	m_ISystem = NULL;

	if (m_pNavigation)
	{
		delete m_pNavigation;
	}

	delete m_pSystemUserCallback;

	if (m_gameDll)
	{
		FreeLibrary(m_gameDll);
	}

	if (m_hSystemHandle)
		FreeLibrary(m_hSystemHandle);

	//////////////////////////////////////////////////////////////////////////
	// Delete entity registry.
	//////////////////////////////////////////////////////////////////////////
	CEntityScriptRegistry::Instance()->Release();
	delete m_pGameToEditorInterface;
}

//////////////////////////////////////////////////////////////////////////
static int ed_killmemory_size;

void KillMemory( IConsoleCmdArgs* /* pArgs */ )
{
	while(true) {
		const int limit = 10000000;
		int size;
		if (ed_killmemory_size > 0)
			size = ed_killmemory_size;
		else {
			size = rand()*rand();
			size = size > limit ? limit : size;
		}

		uint8 * alloc = new uint8[size];
	}
}


//------------------------------------------------------------------------
static void CmdGotoEditor(IConsoleCmdArgs *pArgs)
{
	// feature is mostly useful for QA purposes, this works with the game "goto" command
	// this console command actually is used by the game command, the editor command shouldn't be used by the user

	int iArgCount = pArgs->GetArgCount();
	
	CViewManager *pViewManager = GetIEditor()->GetViewManager();
	CViewport *pRenderViewport = pViewManager->GetGameViewport();		if(!pRenderViewport)return;

	float x,y,z,wx,wy,wz;

	if(iArgCount==7
		&& sscanf(pArgs->GetArg(1),"%f",&x)==1
		&& sscanf(pArgs->GetArg(2),"%f",&y)==1
		&& sscanf(pArgs->GetArg(3),"%f",&z)==1
		&& sscanf(pArgs->GetArg(4),"%f",&wx)==1
		&& sscanf(pArgs->GetArg(5),"%f",&wy)==1
		&& sscanf(pArgs->GetArg(6),"%f",&wz)==1)
	{
		Matrix34 tm = pRenderViewport->GetViewTM();

		tm.SetTranslation(Vec3(x,y,z));
		tm.SetRotation33( Matrix33::CreateRotationXYZ( DEG2RAD(Ang3(wx,wy,wz)) ) );

		pRenderViewport->SetViewTM(tm);
	}
}



//////////////////////////////////////////////////////////////////////////
bool CGameEngine::Init( bool bPreviewMode,bool bTestMode, const char *sInCmdLine,IInitializeUIInfo *logo )
{
	m_pSystemUserCallback = new SSystemUserCallback(logo);



	m_hSystemHandle = LoadLibrary( "CrySystem.dll" );
	if (!m_hSystemHandle)
	{
		Error( "CrySystem.DLL Loading Failed" );
		return false;
	}

	PFNCREATESYSTEMINTERFACE pfnCreateSystemInterface = 
		(PFNCREATESYSTEMINTERFACE)::GetProcAddress( m_hSystemHandle,"CreateSystemInterface" );

	SSystemInitParams sip;
	sip.bEditor = true;
	sip.bDedicatedServer = false;
	sip.bPreview = bPreviewMode;
	sip.bTestMode = bTestMode;
	sip.hInstance = AfxGetInstanceHandle();
	if (AfxGetMainWnd())
		sip.hWnd = AfxGetMainWnd()->GetSafeHwnd();

	sip.pLogCallback = &m_logFile;
	sip.sLogFileName = "Editor.log";
	sip.pUserCallback = m_pSystemUserCallback;
	sip.pValidator = GetIEditor()->GetErrorReport(); // Assign validator from Editor.
	if (sInCmdLine)
		strcpy( sip.szSystemCmdLine,sInCmdLine );
	sip.pCheckFunc = &GameSystemAuthCheckFunction;

	m_ISystem = pfnCreateSystemInterface( sip );

	if (!m_ISystem)
	{
		Error( "CreateSystemInterface Failed" );

		return false;
	}

	m_pNavigation = new CNavigation (gEnv->pSystem);
	m_pNavigation->Init ();

	if(gEnv && gEnv->p3DEngine && gEnv->p3DEngine->GetTimeOfDay())
		gEnv->p3DEngine->GetTimeOfDay()->BeginEditMode();

	CLogFile::AboutSystem();


	REGISTER_CVAR(ed_killmemory_size, -1, VF_DUMPTODISK, "Sets the testing allocation size. -1 for random" );
	REGISTER_COMMAND( "ed_killmemory", KillMemory, VF_NULL,"" );

	REGISTER_COMMAND("ed_goto", CmdGotoEditor, VF_CHEAT, 
		"Internal command, used by the 'GOTO' console command\n");

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::InitGame( const char *sGameDLL )
{
	CryComment("Loading Game DLL: %s",sGameDLL);
	m_gameDll = LoadLibrary(sGameDLL);

	if (!m_gameDll)
	{
		CryLogAlways("Failed to load Game DLL \"%s\"", sGameDLL);
		//Error("InitGame(%s) failed!", sGameDLL);
		CEntityScriptRegistry::Instance()->LoadScripts();

		return false;
	}

	IEditorGame::TEntryFunction CreateEditorGame = (IEditorGame::TEntryFunction)GetProcAddress(m_gameDll, "CreateEditorGame");

	if (!CreateEditorGame)
	{
		Error("CryGetProcAddress(%d, %s) failed!", m_gameDll, "CreateEditorGame");

		return false;
	}

	m_pEditorGame = CreateEditorGame();

	if (!m_pEditorGame)
	{
		Error("CreateEditorGame() failed!");

		return false;
	}

	if (!m_pEditorGame->Init(m_ISystem,m_pGameToEditorInterface))
	{
		Error("IEditorGame::Init(%d) failed!", m_ISystem);

		return false;
	}

	// Enable displaying of labels.
	//@workaround!: @FIXME
	ICVar* pCvar = m_ISystem->GetIConsole()->GetCVar("r_DisplayInfo");
	if (pCvar) pCvar->Set("1");

	//////////////////////////////////////////////////////////////////////////
	// Execute Editor.lua ovrride file.
	IScriptSystem *pScriptSystem = m_ISystem->GetIScriptSystem();
	pScriptSystem->ExecuteFile("Editor.Lua",false);
	//////////////////////////////////////////////////////////////////////////

	CStartupLogoDialog::SetText( "Loading Entity Scripts..." );
	CEntityScriptRegistry::Instance()->LoadScripts();

	CStartupLogoDialog::SetText( "Initializing Flow Graphs..." );
  if(GetIEditor()->GetFlowGraphManager())
	  GetIEditor()->GetFlowGraphManager()->Init();

	// Now initialize our AI.
  if(GetIEditor()->GetAI())
	  GetIEditor()->GetAI()->Init( m_ISystem );

	// Load Equipment packs from disk and export to Game
	GetIEditor()->GetEquipPackLib()->Reset();
	GetIEditor()->GetEquipPackLib()->LoadLibs(true);

	// in editor we do it later, bExecuteCommandLine was set to false
	m_ISystem->ExecuteCommandLine();

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::SetLevelPath( const CString &path )
{
	CString relPath = path;
	if (relPath.IsEmpty())
		relPath = path;

	m_levelPath = Path::RemoveBackslash(relPath);
	m_levelName = Path::RemoveBackslash(relPath);

	if (gEnv->p3DEngine)
		gEnv->p3DEngine->SetLevelPath( m_levelPath );
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::SetMissionName( const CString &mission )
{
	m_missionName = mission;
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::InitTerrain()
{
	// Initialize terrain grid.
	CHeightmap *pHeightmap = GetIEditor()->GetHeightmap();
	if (pHeightmap)
	{
		pHeightmap->InitSectorGrid();

		// construct terrain in 3dengine if was not loaded during SerializeTerrain call
		if (!gEnv->p3DEngine->GetITerrain())
		{ 
			SSectorInfo si;
			pHeightmap->GetSectorsInfo(si);

			STerrainInfo TerrainInfo;
			TerrainInfo.nHeightMapSize_InUnits=si.sectorSize*si.numSectors/si.unitSize;
			TerrainInfo.nUnitSize_InMeters=si.unitSize;
			TerrainInfo.nSectorSize_InMeters=si.sectorSize;
			TerrainInfo.nSectorsTableSize_InSectors=si.numSectors;
			TerrainInfo.fHeightmapZRatio = 1.f/(256.f*256.f)*pHeightmap->GetMaxHeight();
			TerrainInfo.fOceanWaterLevel = pHeightmap->GetWaterLevel();
			ITerrain * pTerrain = gEnv->p3DEngine->CreateTerrain(TerrainInfo);

      SVoxTerrainInfo VoxTerrainInfo;
      VoxTerrainInfo.aabbTerrain = AABB(Vec3(0,0,0),Vec3(si.sectorSize*si.numSectors,si.sectorSize*si.numSectors,si.sectorSize*si.numSectors));
      VoxTerrainInfo.fOceanWaterLevel = pHeightmap->GetWaterLevel();
      IVoxTerrain * pVoxTerrain = gEnv->p3DEngine->CreateVoxTerrain(VoxTerrainInfo);

			// pass heightmap data to the 3dengine
			pHeightmap->UpdateEngineTerrain(false);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::LoadLevel( const CString &levelPath,const CString &mission,bool bDeleteAIGraph,bool bReleaseResources,bool bInitTerrain )
{
  LOADING_TIME_PROFILE_SECTION(GetIEditor()->GetSystem());

	// In case 3d engine was not been initialized before.
	gEnv->p3DEngine = m_ISystem->GetI3DEngine();
	m_bLevelLoaded = false;

	////////////////////////////////////////////////////////////////////////
	// Load a map inside the engine
	////////////////////////////////////////////////////////////////////////
	SetLevelPath( levelPath );
	m_missionName = mission;

	CLogFile::FormatLine("Loading map '%s' into engine...", (const char*)m_levelPath );

	// Switch the current directory back to the Master CD folder first.
	// The engine might have trouble to find some files when the current
	// directory is wrong
	SetCurrentDirectoryW( GetIEditor()->GetMasterCDFolder() );

	CString pakFile = m_levelPath + "/*.pak";
	// Open Pak file for this level.
	if (!m_ISystem->GetIPak()->OpenPacks( pakFile ))
	{
		// Pak1 not found.
		//CryWarning( VALIDATOR_MODULE_GAME,VALIDATOR_WARNING,"Level Pack File %s Not Found",sPak1.c_str() );
	}

	// Initialize physics grid.
	if (bReleaseResources)
	{
		SSectorInfo si;
		GetIEditor()->GetHeightmap()->GetSectorsInfo( si );
		float terrainSize = si.sectorSize * si.numSectors;
		if (m_ISystem->GetIPhysicalWorld())
		{
      float fCellSize = terrainSize>2048 ? terrainSize*(1.0f/1024) : 2.0f;      
      if(ICVar* pCvar = m_ISystem->GetIConsole()->GetCVar("e_PhysMinCellSize")) 
        fCellSize = max(fCellSize,(float)pCvar->GetIVal());
			int log2PODGridSize = 0;
			if (fCellSize==2.0f)
				log2PODGridSize = 2;
			else if (fCellSize==4.0f)
				log2PODGridSize = 1;
      m_ISystem->GetIPhysicalWorld()->SetupEntityGrid( 2,Vec3(0,0,0),terrainSize/fCellSize,terrainSize/fCellSize,fCellSize,fCellSize, log2PODGridSize );
		}
		//////////////////////////////////////////////////////////////////////////
		// Resize proximity grid in entity system.
		if (bReleaseResources)
		{
			if (gEnv->pEntitySystem)
				gEnv->pEntitySystem->ResizeProximityGrid( terrainSize,terrainSize );
		}
		//////////////////////////////////////////////////////////////////////////
	}

	//////////////////////////////////////////////////////////////////////////
	// Load level in 3d engine.
	//////////////////////////////////////////////////////////////////////////
	if (!gEnv->p3DEngine->InitLevelForEditor( Path::ToUnixPath(m_levelPath), m_missionName ))
	{
		CLogFile::WriteLine("ERROR: Can't load level !");
		AfxMessageBox( "ERROR: Can't load level !" );
		return false;
	}

	if (bInitTerrain)
		InitTerrain();

	// Reset sound occlusion info.
	gEnv->pSoundSystem->GetInterfaceExtended()->RecomputeSoundOcclusion(true,true,true);

	GetIEditor()->GetObjectManager()->SendEvent( EVENT_REFRESH );

		//GetIEditor()->GetSystem()->GetI3DEngine()->RemoveAllStaticObjects();
/*
	CVegetationMap *vegMap = GetIEditor()->GetVegetationMap();
	if (vegMap)
		vegMap->PlaceObjectsOnTerrain();
*/
	//GetIEditor()->GetStatObjMap()->PlaceObjectsOnTerrain();
	if (gEnv->pAISystem && bDeleteAIGraph)
	{
		gEnv->pAISystem->FlushSystemNavigation();
// this does nothing
//		gEnv->pAISystem->LoadTriangulation( 0,0 );
		//if (!LoadAI( m_levelName,m_missionName ))
		//return false;
	}

	m_bLevelLoaded = true;

	if (!bReleaseResources)
		ReloadEnvironment();

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::ReloadLevel()
{
	if (!IsLevelLoaded())
		return false;
	if (!LoadLevel( GetLevelPath(),GetMissionName(),false,false,false ))
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::LoadAI( const CString &levelName,const CString &missionName )
{
	if (!gEnv->pAISystem)
		return false;

	if (!IsLevelLoaded())
		return false;

	float fStartTime = m_ISystem->GetITimer()->GetAsyncCurTime();
	CLogFile::FormatLine( "Loading AI Triangulation %s, %s",(const char*)levelName,(const char*)missionName );

	gEnv->pAISystem->FlushSystemNavigation();
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_CLEAR_AIGRAPH );
//	gEnv->pAISystem->LoadTriangulation( levelName,missionName );
	gEnv->pAISystem->LoadNavigationData( levelName,missionName );
	gEnv->pAISystem->LoadCover( levelName, missionName );

	CLogFile::FormatLine( "Finished Loading AI Triangulation in %6.3f secs", m_ISystem->GetITimer()->GetAsyncCurTime() - fStartTime);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::LoadMission( const CString &mission )
{
	if (!IsLevelLoaded())
		return false;
	if (mission != m_missionName)
	{
		m_missionName = mission;
		gEnv->p3DEngine->LoadMissionDataFromXMLNode( m_missionName );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::ReloadEnvironment()
{
	if (!gEnv->p3DEngine)
		return false;
	if (!IsLevelLoaded() && !m_bJustCreated)
		return false;

	if (!GetIEditor()->GetDocument())
		return false;

	//char szLevelPath[_MAX_PATH];
	//strcpy( szLevelPath,GetLevelPath() );
	//PathAddBackslash( szLevelPath );
	//CGameExporter gameExporter( GetIEditor()->GetSystem() );
	//gameExporter.ExportLevelData( szLevelPath,false );

	XmlNodeRef env = CreateXmlNode("Environment");
	CXmlTemplate::SetValues( GetIEditor()->GetDocument()->GetEnvironmentTemplate(),env );

	// Notify mission that environment may be changed.
	GetIEditor()->GetDocument()->GetCurrentMission()->OnEnvironmentChange();

	//! Add lighting node to environment settings.
	GetIEditor()->GetDocument()->GetCurrentMission()->GetLighting()->Serialize( env,false );

	CString xmlStr = env->getXML();

	// Reload level data in engine.
	gEnv->p3DEngine->LoadEnvironmentSettingsFromXML( env );
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::SetGameMode( bool inGame )
{
	if (m_inGameMode == inGame)
		return;

	if (!m_pEditorGame)
		return;

	if (!GetIEditor()->GetDocument())
		return;

	//////////////////////////////////////////////////////////////////////////
	LockResources();

	if ( inGame )
	{
    if( gEnv->p3DEngine )
    {
      gEnv->p3DEngine->ResetPostEffects();
    }

		GetIEditor()->Notify( eNotify_OnBeginGameMode );
	}
	else
  {        
		GetIEditor()->Notify( eNotify_OnEndGameMode );
  }

	m_ISystem->SetThreadState(ESubsys_Physics, false);

	//////////////////////////////////////////////////////////////////////////
	if (gEnv->p3DEngine)
	{
		// Reset some 3d engine effects.
    if ( !inGame )
    {
      gEnv->p3DEngine->ResetPostEffects();
    }

		gEnv->p3DEngine->ResetParticlesAndDecals();
	}

	CViewport *pGameViewport = GetIEditor()->GetViewManager()->GetGameViewport();

	if (GetIEditor()->GetFlowGraphManager())
		GetIEditor()->GetFlowGraphManager()->OnEnteringGameMode(inGame);

	m_inGameMode = inGame;
	m_pEditorGame->SetGameMode( inGame );
	if (inGame)
	{
		CRuler *pRuler = GetIEditor()->GetRuler();
		if (pRuler)
		{
			pRuler->SetActive(false);
		}

		GetIEditor()->GetAI()->SaveAndReloadActionGraphs();

		gEnv->p3DEngine->GetTimeOfDay()->EndEditMode();
		HideLocalPlayer(false);
		// Go to game mode.
	
		m_pEditorGame->SetPlayerPosAng( m_playerViewTM.GetTranslation(),m_playerViewTM.TransformVector(FORWARD_DIRECTION) );

		IEntity *myPlayer = m_pEditorGame->GetPlayer();
		if (myPlayer)
		{
			pe_player_dimensions dim;
			dim.heightEye = 0;
			if (myPlayer->GetPhysics())
				myPlayer->GetPhysics()->GetParams( &dim );

			if (pGameViewport)
			{
				//myPlayer->SetPos( pGameViewport->GetViewTM().GetTranslation() - Vec3(0,0,dim.heightEye) );
				//myPlayer->SetAngles( Ang3::GetAnglesXYZ( Matrix33(pGameViewport->GetViewTM()) ) );
			}
		}
		
		//GetIXGame( GetIEditor()->GetGame() )->SetViewAngles( GetIEditor()->GetViewerAngles() );
		
		// Disable accelerators.
		GetIEditor()->EnableAcceleratos( false );

		// Reset physics state before switching to game.
		m_ISystem->GetIPhysicalWorld()->ResetDynamicEntities();

		// Reset Equipment
    //GetIXGame( GetIEditor()->GetGame() )->RestoreWeaponPacks();

		// Reset mission script.
		GetIEditor()->GetDocument()->GetCurrentMission()->ResetScript();

		// [Anton] - order changed, see comments for CGameEngine::SetSimulationMode
		//if (m_IGame)
			//m_IGame->ResetState();

		//! Send event to switch into game.
		GetIEditor()->GetObjectManager()->SendEvent( EVENT_INGAME );

		// reset all agents in aisystem
		// (MATT) Needs to occur after ObjectManager because AI now needs control over AI enabling on start {2008/09/22}
		if(m_ISystem->GetAISystem())
		{
			m_ISystem->GetAISystem()->Reset(IAISystem::RESET_ENTER_GAME);
		}

		// Reset movie system.
		GetIEditor()->GetAnimation()->ResetAnimations(true,false);

		// Reset Music Logic
		if (IAnimationGraphState * pState = gEnv->pGame->GetIGameFramework()->GetMusicGraphState())
		{
			// reset AIAlertness
			pState->SetInput("AIAlertness", 0.0f);

			// reset Levelname (could be done once on level load instead)
			if (gEnv->pGame->GetIGameFramework()->GetLevelName())
				pState->SetInput("LevelName", gEnv->pGame->GetIGameFramework()->GetLevelName());

			// reset AI_Intensity
			pState->SetInput("AI_Intensity", 0.0f);

			// reset Player_Intensity
			pState->SetInput("Player_Intensity", 0.0f);
		}

		// When the player starts the game inside an area trigger, it will get 
		// triggered.
		gEnv->pEntitySystem->ResetAreas();

		// Switch to first person mode.
		//GetIEditor()->GetGame()->SetViewMode(false);
		// drawing of character.
		
		SEntityEvent event;
		event.event = ENTITY_EVENT_RESET;
    event.nParam[0] = 1;
		gEnv->pEntitySystem->SendEventToAll( event );

		event.event = ENTITY_EVENT_LEVEL_LOADED;
		gEnv->pEntitySystem->SendEventToAll( event );

		event.event = ENTITY_EVENT_START_GAME;
		gEnv->pEntitySystem->SendEventToAll( event );

		// Register in game entitysystem listener.
		s_InGameEntityListener = new SInGameEntitySystemListener;
		gEnv->pEntitySystem->AddSink(s_InGameEntityListener);
	}
	else
	{
		gEnv->p3DEngine->GetTimeOfDay()->BeginEditMode();

		// reset all agents in aisystem
		if(m_ISystem->GetAISystem())
			m_ISystem->GetAISystem()->Reset(IAISystem::RESET_EXIT_GAME);

		IEntity *myPlayer = m_pEditorGame->GetPlayer();
		if (myPlayer)
		{
			//use the system camera matrix as view matrix when getting back to editor mode.
			m_playerViewTM = m_ISystem->GetViewCamera().GetMatrix();
			//m_playerViewTM = myPlayer->GetWorldTM();
		}

		// Out of game in Editor mode.
		//GetIEditor()->SetViewerPos( m_ISystem->GetViewCamera().GetPos() );
		//GetIEditor()->SetViewerAngles( m_ISystem->GetViewCamera().GetAngles() );
		if (pGameViewport)
		{
			pGameViewport->SetViewTM( m_playerViewTM );
		}

		// Unregister ingame entitysystem listener, and kill all remaining entities.
		gEnv->pEntitySystem->RemoveSink(s_InGameEntityListener);
		delete s_InGameEntityListener;
		s_InGameEntityListener = 0;

		// Enable accelerators.
		GetIEditor()->EnableAcceleratos( true );

		// Reset mission script.
		GetIEditor()->GetDocument()->GetCurrentMission()->ResetScript();

		if (GetIEditor()->Get3DEngine()->IsTerrainHightMapModifiedByGame())
			GetIEditor()->GetHeightmap()->UpdateEngineTerrain();

		// reset movie-system
		GetIEditor()->GetAnimation()->ResetAnimations(false,true);

		m_ISystem->GetIPhysicalWorld()->ResetDynamicEntities();
		
		//if (m_IGame)
			//m_IGame->ResetState();

		SEntityEvent event;
		event.event = ENTITY_EVENT_RESET;
    event.nParam[0] = 0;
		gEnv->pEntitySystem->SendEventToAll( event );

		// [Anton] - order changed, see comments for CGameEngine::SetSimulationMode
		//! Send event to switch out of game.
		GetIEditor()->GetObjectManager()->SendEvent( EVENT_OUTOFGAME );
		
		// Switch to third person mode.
	   //GetIEditor()->GetGame()->SetViewMode(true);

		// Hide all drawing of character.

		HideLocalPlayer(true);
	}
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_PHYSICS_APPLYSTATE );

	// Enables engine to know about that.
	gEnv->bEditorGameMode = inGame;


	if (AfxGetMainWnd())
	{
		HWND hWnd = AfxGetMainWnd()->GetSafeHwnd();

		// marcok: setting exclusive mode based on whether we are inGame 
		// fixes mouse sometimes getting stuck in the editor. However, doing
		// this for the keyboard disables editor keys when you go out of
		// gamemode
//		m_ISystem->GetIInput()->SetExclusiveMode( eDI_Mouse, inGame, hWnd );
		m_ISystem->GetIInput()->SetExclusiveMode( eDI_Keyboard, false, hWnd );
		m_ISystem->GetIInput()->ClearKeyState();

		// Julien: we now use hardware mouse, it doesn't change anything
		// for keyboard but mouse should be fixed now.

		if(gEnv->pHardwareMouse)
		{
			gEnv->pHardwareMouse->SetGameMode(inGame);
		}

		AfxGetMainWnd()->SetFocus();
	}

	UnlockResources();
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::SetSimulationMode( bool enabled,bool bOnlyPhysics )
{
	if (m_simulationMode == enabled)
		return;

	if (!bOnlyPhysics)
		LockResources();

	if (enabled)
	{
		CRuler *pRuler = GetIEditor()->GetRuler();
		if (pRuler)
		{
			pRuler->SetActive(false);
		}

		GetIEditor()->Notify( eNotify_OnBeginSimulationMode );

		if (!bOnlyPhysics)
			GetIEditor()->GetAI()->SaveAndReloadActionGraphs();
	}
	else
		GetIEditor()->Notify( eNotify_OnEndSimulationMode );

	m_simulationMode = enabled;
	m_ISystem->SetThreadState(ESubsys_Physics,false);

	if (!bOnlyPhysics)
		m_simulationModeAI = enabled;
	
	if (m_simulationMode)
	{
		if (!bOnlyPhysics)
		{
			if( m_ISystem->GetI3DEngine() )
				m_ISystem->GetI3DEngine()->ResetPostEffects();

			// make sure FlowGraph is initialized
			if (m_ISystem->GetIFlowSystem())
				m_ISystem->GetIFlowSystem()->Reset();

			GetIEditor()->SetConsoleVar( "ai_ignoreplayer",1 );
			//GetIEditor()->SetConsoleVar( "ai_soundperception",0 );
		}

		// [Anton] the order of the next 3 calls changed, since, EVENT_INGAME loads physics state (if any),
		// and Reset should be called before it
		m_ISystem->GetIPhysicalWorld()->ResetDynamicEntities();
		//if (m_IGame)
			//m_IGame->ResetState();
		GetIEditor()->GetObjectManager()->SendEvent( EVENT_INGAME );

		if (!bOnlyPhysics && m_ISystem->GetAISystem())
		{
			// (MATT) Had to move this here because AI system now needs control over entities enabled on start {2008/09/22}
			// When turning physics on, emulate out of game event.
			m_ISystem->GetAISystem()->Reset(IAISystem::RESET_ENTER_GAME);
		}

		// Register in game entitysystem listener.
		s_InGameEntityListener = new SInGameEntitySystemListener;
		gEnv->pEntitySystem->AddSink(s_InGameEntityListener);
	}
	else
	{
		if (!bOnlyPhysics)
		{
			GetIEditor()->SetConsoleVar( "ai_ignoreplayer",0 );
			//GetIEditor()->SetConsoleVar( "ai_soundperception",1 );

			if( m_ISystem->GetI3DEngine() )
				m_ISystem->GetI3DEngine()->ResetPostEffects();

			// When turning physics off, emulate out of game event.
			if (m_ISystem->GetAISystem())
				m_ISystem->GetAISystem()->Reset(IAISystem::RESET_EXIT_GAME);
		}

		m_ISystem->GetIPhysicalWorld()->ResetDynamicEntities();
		//if (m_IGame)
			//m_IGame->ResetState();

		if (!bOnlyPhysics)
		{
			if (GetIEditor()->Get3DEngine()->IsTerrainHightMapModifiedByGame())
				GetIEditor()->GetHeightmap()->UpdateEngineTerrain();
		}

		GetIEditor()->GetObjectManager()->SendEvent( EVENT_OUTOFGAME );	

		// Unregister ingame entitysystem listener, and kill all remaining entities.
		gEnv->pEntitySystem->RemoveSink(s_InGameEntityListener);
		delete s_InGameEntityListener;
		s_InGameEntityListener = 0;
	}

	if (!bOnlyPhysics)
	{
		SEntityEvent event;
		event.event = ENTITY_EVENT_RESET;
		event.nParam[0] = (UINT_PTR)enabled;
		gEnv->pEntitySystem->SendEventToAll( event );
	}
	
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_PHYSICS_APPLYSTATE );

	if (m_simulationMode && !bOnlyPhysics)
	{
		SEntityEvent event;
		event.event = ENTITY_EVENT_LEVEL_LOADED;
		gEnv->pEntitySystem->SendEventToAll( event );

		event.event = ENTITY_EVENT_START_GAME;
		gEnv->pEntitySystem->SendEventToAll( event );
	}

	if(m_ISystem->GetIGame())
	m_ISystem->GetIGame()->GetIGameFramework()->OnEditorSetGameMode(enabled+2);

	if (!bOnlyPhysics)
		UnlockResources();
}

CNavigation * CGameEngine::GetNavigation ()
{
	return m_pNavigation;
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::LoadAINavigationData()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Loading AI Navigation" );

	CWaitCursor waitcrs;

	// Inform AiPoints that their GraphNodes are invalid (so release references to them)
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_CLEAR_AIGRAPH );

	CLogFile::FormatLine( "Loading AI navigation data for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	gEnv->pAISystem->FlushSystemNavigation();
	gEnv->pAISystem->LoadNavigationData( m_levelPath,m_missionName );

	// Inform AiPoints that they need to recreate GraphNodes for navigation NEEDED??
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_ATTACH_AIPOINTS );
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::GenerateAiAllNavigation()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Generating All AI Navigation" );
	CWaitCursor waitcrs;

	wait.Step(1);

	// Inform AiPoints to release their GraphNodes as FlushSystemNavigation() clears all.
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_CLEAR_AIGRAPH );

	// TOxDO Mrz 20, 2008: <pvl> flush editor nav too?
	// UPDATE Aug 5, 2008: <pvl> yes, I'd say
	gEnv->pAISystem->FlushSystemNavigation();
	m_pNavigation->FlushSystemNavigation();

	wait.Step(5);
	
	CLogFile::FormatLine( "Generating Triangulation for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->GenerateTriangulation( m_levelPath,m_missionName );

	wait.Step(20);

	// Inform AiPoints that they need to recreate GraphNodes ready for graph export
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_ATTACH_AIPOINTS );

	CLogFile::FormatLine( "Generating Flight navigation for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->GenerateFlightNavigation( m_levelPath,m_missionName );

	CLogFile::FormatLine( "Generating 3D navigation volumes for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->Generate3DVolumes( m_levelPath,m_missionName );

	wait.Step(30);

	CLogFile::FormatLine( "Generating Waypoints for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->ReconnectAllWaypointNodes();

	wait.Step(40);

//	CLogFile::FormatLine( "Generating layered nav mesh for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
//	m_pNavigation->GenerateLayeredNavMesh();

	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	wait.Step(60);

	// FIXxME Mrz 18, 2008: <pvl> should we just call ExportAIGraph() from here?
	// UPDATE Mrz 20, 2008: <pvl> there's no need actually - our caller
	// CCryEditApp::OnAiGenerateAllNavigation() calls that immediately after
	// this function returns
	// NOTE Mrz 20, 2008: <pvl> interestingly enough, road navigation gets
	// processed as part of ReconnectAllWaypointNodes() but it's not written
	// out there - let's do it here until I can find a better place
	// TODO Mrz 20, 2008: <pvl> it might be best that we invoke roads processing
	// from here as well and add the saving to it so that road nav region saves
	// itself after it's finished its preprocessing just as other regions do?
	char fileNameRoads[1024];
	sprintf(fileNameRoads, "%s\\roadnav%s.bai",m_levelPath,m_missionName);
	m_pNavigation->ExportData(0, 0, fileNameRoads, 0, 0, 0, 0);

	wait.Step(65);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);

	wait.Step(100);
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::GenerateAiTriangulation()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Generating AI Triangulation" );
	CWaitCursor waitcrs;

	// Inform AiPoints to release their GraphNodes as FlushSystemNavigation() clears all.
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_CLEAR_AIGRAPH );

	m_pNavigation->FlushSystemNavigation();

	CLogFile::FormatLine( "Generating Triangulation for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->GenerateTriangulation( m_levelPath,m_missionName );

	// Inform AiPoints that they need to recreate GraphNodes ready for graph export
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_ATTACH_AIPOINTS );

	CLogFile::FormatLine( "Generating Waypoints for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->ReconnectAllWaypointNodes();

	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::GenerateAiWaypoint()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Generating AI Waypoints" );
	CWaitCursor waitcrs;

	// Inform AiPoints that they need to create GraphNodes (if they haven't already) ready for graph export
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_ATTACH_AIPOINTS );

	CLogFile::FormatLine( "Generating Waypoints for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->ReconnectAllWaypointNodes();

	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::GenerateAiFlightNavigation()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Generating AI Flight navigation" );
	CWaitCursor waitcrs;

	m_pNavigation->FlushSystemNavigation();

	CLogFile::FormatLine( "Generating Flight navigation for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->GenerateFlightNavigation( m_levelPath,m_missionName );

	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::GenerateAiNavVolumes()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Generating AI 3D navigation volumes" );
	CWaitCursor waitcrs;

	m_pNavigation->FlushSystemNavigation();

	CLogFile::FormatLine( "Generating 3D navigation volumes for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->Generate3DVolumes( m_levelPath,m_missionName );

	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);
}

void CGameEngine::GenerateAiLayeredNavMesh()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Generating AI Layered Nav Mesh" );
	CWaitCursor waitcrs;

	wait.Step(1);

	// AiPoints are incompatible with LNM, so remove from the graph before export
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_CLEAR_AIGRAPH );

	CLogFile::FormatLine( "Generating layered nav mesh for Level:%s Mission:%s",(const char*)m_levelPath,(const char*)m_missionName );
	m_pNavigation->GenerateLayeredNavMesh (m_levelPath, m_missionName);

	wait.Step(80);

	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);

	// Inform AiPoints that can reattach GraphNodes now LNM export is done
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_ATTACH_AIPOINTS );
}



//====================================================================
// ValidateAINavigation
//====================================================================
void CGameEngine::ValidateAINavigation()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Validating AI navigation" );
	CWaitCursor waitcrs;

	if (m_pNavigation->GetGraph())
	{
		if (m_pNavigation->GetGraph()->Validate("", true))
			CLogFile::FormatLine( "AI navigation validation OK");
		else
			CLogFile::FormatLine( "AI navigation validation failed");
	}
	else
	{
		CLogFile::FormatLine( "No AI graph - cannot validate");
	}

	m_pNavigation->ValidateNavigation();
}



//====================================================================
// ClearAllAINavigation: Clears all AI navigation data from the level.
//====================================================================
void CGameEngine::ClearAllAINavigation()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Clearing all AI navigation data" );
	CWaitCursor waitcrs;

	wait.Step(1);

	// Inform AiPoints to release their GraphNodes.
	GetIEditor()->GetObjectManager()->SendEvent(EVENT_CLEAR_AIGRAPH);

	// Flush old navigation data
	m_pNavigation->FlushLayeredNavMesh();
	gEnv->pAISystem->FlushSystemNavigation();
	m_pNavigation->FlushSystemNavigation();

	wait.Step(20);

	// Ensure file data is all cleared (prevents confusion if the old data were to be reloaded implicitly)
	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing empty data to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	wait.Step(60);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);

	wait.Step(100);
}

//====================================================================
// FreeLNMNavigation: Frees LNM debug data from the editor.
//====================================================================
void CGameEngine::FreeLNMDebugData()
{
	if (!gEnv->pAISystem)
		return;

	CWaitProgress wait( "Freeing LNM debug data" );
	CWaitCursor waitcrs;

	wait.Step(10);

	// Flush old LNM generation data - helps reduce memory usage
	m_pNavigation->FlushLayeredNavMesh();

	wait.Step(100);
}


//====================================================================
// Generate3DDebugVoxels
//====================================================================
void CGameEngine::Generate3DDebugVoxels()
{
	CWaitProgress wait( "Generating 3D debug voxels" );
	CWaitCursor waitcrs;

  m_pNavigation->Generate3DDebugVoxels();

	CLogFile::FormatLine( "Use ai_debugdrawvolumevoxels to view more/fewer voxels");

	char fileName[1024];
	sprintf(fileName, "%s/net%s.bai",m_levelPath,m_missionName);
	AILogProgress(" Now writing to %s.",fileName);
	m_pNavigation->GetGraph()->WriteToFile (fileName);

	gEnv->pAISystem->LoadNavigationData (m_levelPath, m_missionName);
}

void CGameEngine::GenerateAICoverSurfaces()
{
	if (!gEnv->pAISystem)
		return;

	CCoverSurfaceManager* coverSurfaceManager = GetIEditor()->GetAI()->GetCoverSurfaceManager();
	const CCoverSurfaceManager::SurfaceObjects& surfaceObjects = coverSurfaceManager->GetSurfaceObjects();
	uint32 surfaceObjectCount = surfaceObjects.size();
	if (!surfaceObjectCount)
		return;

	CWaitProgress wait( "Generating AI Cover Surfaces" );
	CWaitCursor waitcrs;

	wait.Step(1);

	char fileName[1024];
	sprintf(fileName, "%s\\cover%s.bai", m_levelPath, m_missionName);

	float stepInc = 98 / (float)surfaceObjectCount;
	float step = 1.0f;

	GetIEditor()->GetAI()->GetCoverSurfaceManager()->ClearGameSurfaces();

	CCoverSurfaceManager::SurfaceObjects::const_iterator it = surfaceObjects.begin();
	CCoverSurfaceManager::SurfaceObjects::const_iterator end = surfaceObjects.end();

	for ( ; it != end; ++it)
	{
		CAICoverSurface* coverSurfaceObject = *it;
		coverSurfaceObject->Generate();
		
		step += stepInc;
		wait.Step((uint32)step);
	}
	
	AILogProgress( "Now writing to %s.", fileName);

	GetIEditor()->GetAI()->GetCoverSurfaceManager()->WriteToFile(fileName);
	
	wait.Step(100);
}

void CGameEngine::ExportAiData(const char * navFileName, const char * areasFileName,
			const char* roadsFileName, const char* vertsFileName, const char* volumeFileName,
			const char* flightFileName, const char* lnmFileName)
{
	m_pNavigation->ExportData(navFileName, areasFileName, roadsFileName, vertsFileName, volumeFileName, flightFileName, lnmFileName);
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::ForceRegisterEntitiesInSectors()
{
  return;

	// Reset state of all entities.
	IEntityItPtr entityIt = gEnv->pEntitySystem->GetEntityIterator();
	entityIt->MoveFirst();
	for (IEntity *entity = entityIt->Next(); entity != 0; entity = entityIt->Next())
	{
		//if (entity->Re
		//entity->SetRegisterInSectors(false);
		//entity->SetRegisterInSectors(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::ResetResources()
{
	if (gEnv->pEntitySystem)
	{
		/// Delete all entities.
		gEnv->pEntitySystem->Reset();
		// In editor we are at the same time client and server.
		gEnv->bClient = true;
		gEnv->bServer = true;
		gEnv->bMultiplayer = false;
		gEnv->bHostMigrating = false;
	}
	if (gEnv->p3DEngine)
		gEnv->p3DEngine->ClearRenderResources();
	HideLocalPlayer(true);
}
//////////////////////////////////////////////////////////////////////////
void CGameEngine::SetPlayerEquipPack( const char *sEqipPackName )
{
	if (m_pEditorGame)
	{
//			GetIXGame( m_IGame )->SetPlayerEquipPackName( sEqipPackName );
	}
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::ReloadResourceFile( const CString &filename )
{
	GetIEditor()->GetRenderer()->EF_ReloadFile( filename );
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::SetPlayerViewMatrix( const Matrix34 &tm,bool bEyePos )
{
	m_playerViewTM = tm;

	Vec3	oPosition(m_playerViewTM.GetTranslation());
	Vec3	oDirection(m_playerViewTM.TransformVector(FORWARD_DIRECTION));

	if (gSettings.oHotUpdateSystemSettings.boSyncCamera)
	{
		CConsoleSynchronization::GetCConsoleSynchronization().OnCameraModified(oPosition,oDirection);
	}

	if (!m_pEditorGame)
		return;

	//if (m_inGameMode || m_simulationMode)
	if (m_syncPlayerPosition)
	{
		m_pEditorGame->SetPlayerPosAng(oPosition,oDirection);
	}
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::HideLocalPlayer( bool bHide )
{
	if (m_pEditorGame)
		m_pEditorGame->HidePlayer(bHide);
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::SyncPlayerPosition( bool bEnable )
{
	m_syncPlayerPosition = bEnable;
	if (m_syncPlayerPosition)
	{
		SetPlayerViewMatrix( m_playerViewTM );
	}
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::SetCurrentMOD( const char *sMod )
{
	m_MOD = sMod;
}

//////////////////////////////////////////////////////////////////////////
const char* CGameEngine::GetCurrentMOD() const
{
	return m_MOD;
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::Update()
{
	if (m_pEditorGame && m_inGameMode)
	{
		m_pEditorGame->Update(true, 0);

		CViewport *pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
		if (pRenderViewport)
		{
			// Make sure we at least try to update game viewport (Needed for AVI recording).
			pRenderViewport->Update();
		}
	}
	else
	{
		// [marco] check current sound and vis areas for music etc.	
		// but if in game mode, 'cos is already done in the above call to game->update()
		//if (m_IGame)	GetIXGame( m_IGame )->CheckSoundVisAreas();
		unsigned int updateFlags = ESYSUPDATE_EDITOR;

		CRuler *pRuler = GetIEditor()->GetRuler();
		const bool bRulerNeedsUpdate = (pRuler && pRuler->HasQueuedPaths());
		
		if (!m_simulationMode)
			updateFlags |= ESYSUPDATE_IGNORE_PHYSICS;
		if (!m_simulationModeAI && !bRulerNeedsUpdate)
			updateFlags |= ESYSUPDATE_IGNORE_AI;

		GetIEditor()->GetSystem()->Update( updateFlags );

		// Update flow system in simulation mode.
		if (GetSimulationMode() || m_bUpdateFlowSystem)
		{
			IFlowSystem *pFlowSystem = GetIFlowSystem();
			if (pFlowSystem)
				pFlowSystem->Update();

      IDialogSystem* pDialogSystem = gEnv->pGame ? gEnv->pGame->GetIGameFramework()->GetIDialogSystem() : NULL;
			if (pDialogSystem)
				pDialogSystem->Update( gEnv->pTimer->GetFrameTime() );

      if(m_pEditorGame)
			  m_pEditorGame->Update(true, updateFlags | ESYSUPDATE_EDITOR_AI_PHYSICS);
		}
	}

	// [marco] after system update, retrigger areas if necessary			
	if (m_pEditorGame)
	{
			//if (m_IGame)	GetIXGame( m_IGame )->RetriggerAreas();
			//if (m_IGame)	GetIXGame( m_IGame )->HideLocalPlayer( true, true );
	}

	m_pNavigation->Update ();
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
	case eNotify_OnBeginNewScene:
	case eNotify_OnBeginSceneOpen:
		ResetResources();
		if (m_pEditorGame)
			m_pEditorGame->OnBeforeLevelLoad();
		break;

	case eNotify_OnBeginTerrainRebuild:
		if (m_pEditorGame)
			m_pEditorGame->OnBeforeLevelLoad();
		break;
	
	case eNotify_OnEndNewScene:
	case eNotify_OnEndSceneOpen:			// This method must be called so we will have a player to start game mode later.
	case eNotify_OnEndTerrainRebuild:
		if (m_pEditorGame)
		{
			m_pEditorGame->OnAfterLevelLoad(m_levelName, m_levelPath);
			HideLocalPlayer(true);
		}
		else
		{
			gEnv->p3DEngine->PostLoadLevel();
		}
		break;

	case eNotify_OnQuit:
	case eNotify_OnCloseScene:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
IEntity* CGameEngine::GetPlayerEntity()
{
	if (m_pEditorGame)
		return m_pEditorGame->GetPlayer();
	return 0;
}

//////////////////////////////////////////////////////////////////////////
IFlowSystem* CGameEngine::GetIFlowSystem() const
{
	if (m_pEditorGame)
		return m_pEditorGame->GetIFlowSystem();
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
IGameTokenSystem* CGameEngine::GetIGameTokenSystem() const
{
	if (m_pEditorGame)
		return m_pEditorGame->GetIGameTokenSystem();
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
IEquipmentSystemInterface* CGameEngine::GetIEquipmentSystemInterface() const
{
	if (m_pEditorGame)
		return m_pEditorGame->GetIEquipmentSystemInterface();
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::LockResources()
{
	gEnv->p3DEngine->LockCGFResources();
	GetISystem()->GetIAnimationSystem()->LockResources();
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::UnlockResources()
{
	GetISystem()->GetIAnimationSystem()->UnlockResources();
	gEnv->p3DEngine->UnlockCGFResources();
}

//////////////////////////////////////////////////////////////////////////
bool CGameEngine::SupportsMultiplayerGameRules()
{
	return m_pEditorGame->SupportsMultiplayerGameRules();
}

//////////////////////////////////////////////////////////////////////////
void CGameEngine::ToggleMultiplayerGameRules()
{
	m_pEditorGame->ToggleMultiplayerGameRules();
}
