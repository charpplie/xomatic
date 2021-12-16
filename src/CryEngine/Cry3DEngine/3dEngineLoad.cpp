////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   3dengineload.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Level loading
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "3dEngine.h"
#include "terrain.h"
#include "VoxTerrain.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_water.h"
#include "IParticles.h"
#include "DecalManager.h"
#include "Vegetation.h"
#include "WaterVolumes.h"
#include "Brush.h"
#include "MatMan.h"
#include "IndexedMesh.h"
#include "SkyLightManager.h"
#include "ObjectsTree.h"
#include "LightEntity.h"
#include "WaterWaveRenderNode.h"
#include "RoadRenderNode.h"
#include "ColorBleeding.h"
#include <ICryAnimation.h>

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
#include "IPhysicsGPU.h"   
#endif

#define LEVEL_DATA_FILE "LevelData.xml"
#define CUSTOM_MATERIALS_FILE "Materials.xml"
#define PARTICLES_FILE "LevelParticles.xml"
#define SHADER_LIST_FILE "ShadersList.txt"
#define LEVEL_CONFIG_FILE "Level.cfg"
#define LEVEL_EDITOR_CONFIG_FILE "Editor.cfg"

inline Vec3 StringToVector( const char *str )
{
	Vec3 vTemp(0,0,0);
	float x,y,z;
	if (sscanf( str,"%f,%f,%f",&x,&y,&z ) == 3)
	{
		vTemp(x,y,z);
	}
	else
	{
		vTemp(0,0,0);
	}
	return vTemp;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::ClearRenderResources()
{
//  PrintMessage("*** Clearing render resources ***");

//	GetSystem()->GetIAnimationSystem()->	;

//  ShutDown(false);

	if(GetRenderer())
	{
		GetRenderer()->FreeResources(FRR_DELETED_MESHES);
	}

  //if(GetRenderer())
  //  GetRenderer()->FreeResources(FRR_SHADERS | FRR_TEXTURES | FRR_RESTORE);

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	SAFE_DELETE(m_pSun);

	CleanLevelShaders();

	if (m_pRESky)
		m_pRESky->Release(true);
	if (m_pREHDRSky)
		m_pREHDRSky->Release(true);
	if (m_pREPostProcess)
		m_pREPostProcess->Release(true);
	m_pRESky = 0;
	m_pREHDRSky = 0;
	m_pREPostProcess = 0;

	//if (m_pPartManager)
	//{
	//	m_pPartManager->ClearRenderResources();
	//}

	SAFE_DELETE( m_pDecalManager );

	if(m_pColorBleeding)
		m_pColorBleeding->Cleanup();

  ReRegisterKilledVegetationInstances();

	// delete terrain
	if(m_pTerrain)
	{
		PrintMessage("Deleting terrain");
		SAFE_DELETE( m_pTerrain );
		PrintMessage(" ");
	}

  // delete voxel terrain
  if(m_pVoxTerrain)
  {
    PrintMessage("Deleting voxel terrain");
    SAFE_DELETE( m_pVoxTerrain );
    PrintMessage(" ");
  }

	// delete outdoor objects
  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
	  if(m_pObjectsTree[nSID])
	  {
		  PrintMessage("Deleting outdoor objects");
		  SAFE_DELETE(m_pObjectsTree[nSID]);
	  }
  }

	// delete indoors
	if(m_pVisAreaManager)
	{
		PrintMessage("Deleting indoors");
		SAFE_DELETE( m_pVisAreaManager );
		PrintMessage(" ");
	}

  if( m_pWaterWaveManager )
  {
    PrintMessage("Deleting water waves");
    SAFE_DELETE( m_pWaterWaveManager );
  }

	// free vegetation and brush CGF's
	if(!m_bEditor)
	{
		if (m_pObjManager)
			m_pObjManager->UnloadVegetationModels();
	}

	//////////////////////////////////////////////////////////////////////////
	// Purge destroyed physics entities.
	//////////////////////////////////////////////////////////////////////////
	if(gEnv->pPhysicalWorld)
		gEnv->pPhysicalWorld->PurgeDeletedEntities();

	//GetRenderer()->PreLoad();
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetLevelPath( const char * szFolderName )
{
	// make folder path
	assert(strlen(szFolderName) < 1024);
	strcpy( m_szLevelFolder,szFolderName );
	if (strlen(m_szLevelFolder) > 0)
	{
		if (m_szLevelFolder[strlen(m_szLevelFolder)-1] != '/')
			strcat( m_szLevelFolder,"/" );
	}
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadDefaultAssets()
{
	m_pTerrainMat = MakeSystemMaterialFromShader("Terrain");
	m_pVoxTerrainMat = MakeSystemMaterialFromShader("VoxTerrain");
	m_nBlackTexID = GetRenderer()->EF_LoadTexture("Textures/Defaults/black.dds",FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D)->GetTextureID();

	if (!m_pRESky)
	{
		m_pRESky = (CRESky*)GetRenderer()->EF_CreateRE(eDATA_Sky); //m_pRESky->m_fAlpha = 1.f;
	}
	if (!m_pREHDRSky)
	{
		m_pREHDRSky = (CREHDRSky*)GetRenderer()->EF_CreateRE(eDATA_HDRSky);
	}
	if (!m_pREPostProcess)
	{
		m_pREPostProcess = (CREPostProcess*)GetRenderer()->EF_CreateRE(eDATA_PostProcess);
	}

	if (m_pFarTreeSprites)
	{
		m_pFarTreeSprites = GetRenderer()->EF_LoadShader("FarTreeSprites", EF_SYSTEM);
	}
}


//////////////////////////////////////////////////////////////////////////
bool C3DEngine::InitLevelForEditor(const char * szFolderName, const char * szMissionName)
{




	LOADING_TIME_PROFILE_SECTION(GetSystem());

	m_bEditor = true;

	gEnv->pPhysicalWorld->DeactivateOnDemandGrid();

	if(!szFolderName || !szFolderName[0])
	{ Warning( "C3DEngine::LoadLevel: Level name is not specified"); return 0; }

	if(!szMissionName || !szMissionName[0])
	{ Warning( "C3DEngine::LoadLevel: Mission name is not specified"); }

	char szMissionNameBody[256] = "NoMission";
	if(!szMissionName)
		szMissionName = szMissionNameBody;

	SetLevelPath( szFolderName );

	// Load console vars specific to this level.
	GetISystem()->LoadConfiguration( GetLevelFilePath(LEVEL_CONFIG_FILE) );
  GetISystem()->LoadConfiguration( GetLevelFilePath(LEVEL_EDITOR_CONFIG_FILE) );

	if(!m_pObjManager)
		m_pObjManager = new CObjManager();

	if(!m_pVisAreaManager)
		m_pVisAreaManager = new CVisAreaManager( );

	assert (gEnv->pCharacterManager);

	// recreate particles and decals
	if(m_pPartManager)
		m_pPartManager->Reset(false);

	// recreate decals
	delete m_pDecalManager;
	m_pDecalManager = new CDecalManager();

	// restore game state
	EnableOceanRendering(true);
	m_pObjManager->m_bLockCGFResources = 0;

	LoadDefaultAssets();

	if (IsPreloadEnabled())
	{
		if(gEnv->pGame)
			m_pPartManager->LoadLibrary( "*", NULL, true );
	}

	//////////////////////////////////////////////////////////////////////////
	// Precache decals materials.
	{
		GetMatMan()->PrecacheDecalMaterials();
	}

  if( !m_pWaterWaveManager )
  {
    m_pWaterWaveManager = new CWaterWaveManager();
  }
  
	
	{
		string SettingsFileName = GetLevelFilePath("ScreenshotMap.Settings");

		FILE * metaFile = gEnv->pCryPak->FOpen(SettingsFileName,"r");
		if(metaFile)
		{
			char Data[1024*8];
			gEnv->pCryPak->FRead(Data,sizeof(Data),metaFile);
			sscanf(Data, "<Map CenterX=\"%f\" CenterY=\"%f\" SizeX=\"%f\" SizeY=\"%f\" Height=\"%f\"  Quality=\"%d\" />",
													&GetCVars()->e_ScreenShotMapCenterX,
													&GetCVars()->e_ScreenShotMapCenterY,
													&GetCVars()->e_ScreenShotMapSizeX,
													&GetCVars()->e_ScreenShotMapSizeY,
													&GetCVars()->e_ScreenShotMapCamHeight,
													&GetCVars()->e_ScreenShotQuality);
			gEnv->pCryPak->FClose(metaFile);
		}
	}

	LoadPhysicsData();

//	delete m_pObjectsTree[nSID];
//	m_pObjectsTree[nSID] = NULL;
	return (true);

}

bool C3DEngine::LoadTerrain(XmlNodeRef pDoc, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, int nSID, Vec3 vSegmentOrigin)
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

  PrintMessage("===== Loading %s =====", COMPILED_HEIGHT_MAP_FILE_NAME);

	// open file
	FILE * f = GetPak()->FOpen(GetLevelFilePath(COMPILED_HEIGHT_MAP_FILE_NAME), "rbx");
	if(!f)
		return 0; 

	// read header
	STerrainChunkHeader header;
	if(!GetPak()->FRead(&header, 1, f, false))
	{
		GetPak()->FClose(f);
		return 0;
	}

  SwapEndian(header, (header.nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian);

	if(header.nChunkSize)
	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, 0, "Terrain");

    if(!m_pTerrain)
		  m_pTerrain = (CTerrain*)CreateTerrain(header.TerrainInfo);

		m_pTerrain->LoadSurfaceTypesFromXML(pDoc, nSID);

		if(!m_pTerrain->Load(f,header.nChunkSize-sizeof(STerrainChunkHeader),&header, ppStatObjTable, ppMatTable, nSID, vSegmentOrigin))
    {
      delete m_pTerrain;
      m_pTerrain = NULL;
    }
	}

  assert(GetPak()->FEof(f));

  GetPak()->FClose(f);

  return m_pTerrain!=NULL;
}

bool C3DEngine::LoadVoxTerrain(XmlNodeRef pDoc, std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable, int nSID, Vec3 vSegmentOrigin)
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

  PrintMessage("===== Loading %s =====", COMPILED_VOX_MAP_FILE_NAME);

  // open file
  FILE * f = GetPak()->FOpen(GetLevelFilePath(COMPILED_VOX_MAP_FILE_NAME), "rbx");
  if(!f)
    return 0; 

  // read header
  SIsoTreeChunkHeader header;
  if(!GetPak()->FRead(&header, 1, f, false))
  {
    GetPak()->FClose(f);
    return 0;
  }

  SwapEndian(header, eLittleEndian);

  if(header.nChunkSize)
  {
    SVoxTerrainInfo info;
    m_pVoxTerrain = (CVoxTerrain*)CreateVoxTerrain(info);

    int nDataSize = header.nChunkSize-sizeof(SIsoTreeChunkHeader);
    if(!m_pVoxTerrain->Load_T(f,nDataSize,&header, true, eLittleEndian, NULL, nSID))
    {
      delete m_pVoxTerrain;
      m_pVoxTerrain = NULL;
    }
  }

  assert(GetPak()->FEof(f));

  if(m_pVoxTerrain)
    m_pVoxTerrain->m_pDataFile = f;
  else
    GetPak()->FClose(f);

  return m_pVoxTerrain!=NULL;
}

bool C3DEngine::LoadVisAreas(std::vector<struct IStatObj*> ** ppStatObjTable, std::vector<IMaterial*> ** ppMatTable)
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

  PrintMessage("===== Loading %s =====", COMPILED_VISAREA_MAP_FILE_NAME);

	// open file
	FILE * f = GetPak()->FOpen(GetLevelFilePath(COMPILED_VISAREA_MAP_FILE_NAME), "rbx");
	if(!f)
		return false; 

	// read header
	SVisAreaManChunkHeader header;
	if(!GetPak()->FRead(&header, 1, f, false))
	{
		GetPak()->FClose(f);
		return 0;
	}

  SwapEndian(header, (header.nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian);

	if(header.nChunkSize)
	{
		assert(!m_pVisAreaManager);
		m_pVisAreaManager = new CVisAreaManager( );
		if(!m_pVisAreaManager->Load(f, header.nChunkSize, &header, *ppStatObjTable, *ppMatTable))
		{
			delete m_pVisAreaManager;
			m_pVisAreaManager = NULL;
		}
	}

  assert(GetPak()->FEof(f));

  GetPak()->FClose(f);

	return m_pVisAreaManager!=NULL;
}


void C3DEngine::UnloadLevel()
{
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_UNLOAD, 0, 0);

	ClearRenderResources();

	if (gEnv->pCharacterManager)
	{
		// Delete all character instances and models.
//Diesel cut
		//gEnv->pCharacterManager->ClearResources();
//Diesel new
		CryComment("Deleting Characters");
		gEnv->pCharacterManager->ClearResources(false);
		CryComment("done");
//Diesel new

	}

	// Last add!
	if (m_pPartManager)
	{
		m_pPartManager->ClearRenderResources(true);
	}

	//SAFE_DELETE(m_pObjManager);
	// delete terrain
	SAFE_DELETE(m_pTerrain);

  // delete voxel terrain
  SAFE_DELETE( m_pVoxTerrain );

	// delete indoors
	SAFE_DELETE(m_pVisAreaManager);

	// free models
	m_pObjManager->UnloadVegetationModels();

	// re-create decal manager
	SAFE_DELETE( m_pDecalManager); 
	SAFE_DELETE( m_pWaterWaveManager );  

	m_pTerrainWaterMat=0;
	m_nWaterBottomTexId=0;

  if(m_pObjManager)
    m_pObjManager->CheckObjectLeaks();

	// Force to clean render resources left after deleting all objects.
	if(GetRenderer())
	{
		// This may also release some of the materials.
		GetRenderer()->FreeResources(FRR_DELETED_MESHES);
	}

	if (GetMatMan())
	{
		// Should be after deleting all meshes.
		// We force delete all materials.
		GetMatMan()->ShutDown();
	}

	// Must be sent last.
	// Cleanup all containers 
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_POST_UNLOAD, 0, 0);
}


//////////////////////////////////////////////////////////////////////////
bool C3DEngine::LoadLevel(const char * szFolderName, const char * szMissionName)
{
	LOADING_TIME_PROFILE_SECTION(GetSystem());

  m_vPrevMainFrameCamPos.Set(-1000000.f,-1000000.f,-1000000.f);

	m_fAverageFPS = 0.0f;
	m_fMinFPS = 0.0f;
	m_fMaxFPS = 0.0f;
	m_nFramesCount = 0;
	
	// Cleanup all containers 
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_POST_UNLOAD, 0, 0);
#if !defined(PS3) && !defined(XENON)
	m_bEditor = false;
#endif
	gEnv->pPhysicalWorld->DeactivateOnDemandGrid();

  ClearRenderResources();

	//GetMaterialManager()->ReleaseSurfaceTypeManager();
	//GetMaterialManager()->CreateSurfaceTypeManager();

	assert(!m_bEditor);
 
  if(!szFolderName || !szFolderName[0])
  { Warning( "C3DEngine::LoadLevel: Level name is not specified"); return 0; }

	if(!szMissionName || !szMissionName[0])
	{ Warning( "C3DEngine::LoadLevel: Mission name is not specified"); }

  char szMissionNameBody[256] = "NoMission";
  if(!szMissionName)
    szMissionName = szMissionNameBody;

	SetLevelPath( szFolderName );
	
	// Load console vars specific to this level.
	GetISystem()->LoadConfiguration( GetLevelFilePath(LEVEL_CONFIG_FILE) );

  { // check is LevelData.xml file exist
    char sMapFileName[_MAX_PATH];
    strcpy(sMapFileName,m_szLevelFolder);
    strcat(sMapFileName,LEVEL_DATA_FILE);
    if(!IsValidFile(sMapFileName))
    { PrintMessage("Error: Level not found: %s", sMapFileName); return 0; }
  }

  if(!m_pObjManager)
    m_pObjManager = new CObjManager();

	GetObjManager()->m_decalsToPrecreate.resize( 0 );

	assert (gEnv->pCharacterManager);

	// delete terrain
	if(m_pTerrain)
	{
		PrintMessage("Deleting Terrain");
		SAFE_DELETE(m_pTerrain);
		PrintMessage(" ");
	}

  // delete voxel terrain
  if(m_pVoxTerrain)
  {
    PrintMessage("Deleting voxel terrain");
    SAFE_DELETE( m_pVoxTerrain );
    PrintMessage(" ");
  }

	// delete indoors
	if(m_pVisAreaManager)
	{
		PrintMessage("Deleting Indoors");
		SAFE_DELETE(m_pVisAreaManager);
		PrintMessage(" ");
	}

	// free models
	m_pObjManager->UnloadObjects(false);

  // print level name into console
  char header[512];
  snprintf(header, sizeof(header),
    "---------------------- Loading level %s, mission %s ------------------------------------",
    szFolderName, szMissionName);
  header[100]=0;
  PrintMessage(header);

	LoadDefaultAssets();













	// Load LevelData.xml File.



	XmlNodeRef xmlLevelData = GetSystem()->LoadXmlFile( GetLevelFilePath(LEVEL_DATA_FILE) );


	if(xmlLevelData==0)
	{
		Error( "C3DEngine::LoadLevel: xml file not found (files missing?)"); // files missing ?
		return false;
	}

	// re-create decal manager
	delete m_pDecalManager; 
	m_pDecalManager = new CDecalManager();

  SAFE_DELETE( m_pWaterWaveManager );  
  m_pWaterWaveManager = new CWaterWaveManager();

  // preload level cgfs
  if( GetCVars()->e_StatObjPreload && !gEnv->bEditor)
  {
    if( GetCVars()->e_StatObjPreload==2 )
      GetSystem()->OutputLoadingTimeStats();

		GetMatMan()->PreloadLevelMaterials();

    m_pObjManager->PreloadLevelObjects();

    if( GetCVars()->e_StatObjPreload==2 )
    {
      GetSystem()->OutputLoadingTimeStats();
      exit(0);
    }
  }

  std::vector<struct IStatObj*> * pStatObjTable = NULL;
  std::vector<IMaterial*> * pMatTable = NULL;

  int nSID = 0;

  // load terrain
  XmlNodeRef nodeRef = xmlLevelData->findChild("SurfaceTypes");

	if(!LoadTerrain(nodeRef, &pStatObjTable, &pMatTable, nSID, Vec3(0,0,0)))
	{
    Error("Terrain file (%s) not found or file version error, please try to re-export the level", COMPILED_HEIGHT_MAP_FILE_NAME);
    return false;
	}

  // load vox terrain
  if(GetCVars()->e_VoxTer && !LoadVoxTerrain(nodeRef, &pStatObjTable, &pMatTable, nSID, Vec3(0,0,0)))
  {
    Error("VoxTerrain file (%s) not found or file version error, please try to re-export the level", COMPILED_VOX_MAP_FILE_NAME);
    return false;
  }

	// load indoors 
	if(!LoadVisAreas(&pStatObjTable, &pMatTable))
	{
		Error("VisAreas file (%s) not found or file version error, please try to re-export the level", COMPILED_VISAREA_MAP_FILE_NAME);
		return false;
	}

  SAFE_DELETE(pStatObjTable);
  SAFE_DELETE(pMatTable);

  COctreeNode::FreeLoadingCache();

	// re-create particles and decals
	if(m_pPartManager)
		m_pPartManager->Reset(false);

	LoadParticleEffects( xmlLevelData );

	//////////////////////////////////////////////////////////////////////////
	// Precache decals materials.
	{
		GetMatMan()->PrecacheDecalMaterials();
	}

  // load leveldata.xml
  m_pTerrainWaterMat=0;
	m_nWaterBottomTexId=0;
  LoadMissionDataFromXMLNode(szMissionName);

  // init water if not initialized already (if no mission was found)
  if(m_pTerrain && !m_pTerrain->GetOcean())
    m_pTerrain->InitTerrainWater(m_pTerrainWaterMat, m_nWaterBottomTexId);

	//// fully update sky light before the level gets played // TODO: where to put?
	//if( m_pSkyLightManager )
	//	m_pSkyLightManager->FullUpdate();

	//while(Get3DEngine()->m_lstVoxelObjectsForUpdate.Count())
//		GetTerrain()->Voxel_Recompile_Modified_Incrementaly();

	LoadPhysicsData();

	// restore game state
	EnableOceanRendering(true);
	m_pObjManager->m_bLockCGFResources = 0;

  return (true);
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadPhysicsData()
{
	if (gEnv->pScriptSystem)
	{
		// Load explosion shapes.
		gEnv->pScriptSystem->ExecuteFile( "scripts/physics.lua",true,true );
	}
}

void C3DEngine::LoadTerrainSurfacesFromXML(XmlNodeRef pDoc, bool bUpdateTerrain, int nSID)
{
  if(!m_pTerrain)
    return;

	m_pTerrain->LoadSurfaceTypesFromXML(pDoc, nSID);

	m_pTerrain->UpdateSurfaceTypes(nSID);

  m_pTerrain->InitHeightfieldPhysics(nSID);
}  

void C3DEngine::LoadMissionDataFromXMLNode(const char * szMissionName)
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

  if(!m_pTerrain)
  {
    Warning( "Calling C3DEngine::LoadMissionDataFromXMLNode while level is not loaded");
    return;
  }

  GetRenderer()->MakeMainContextActive();

  // set default values
	m_vFogColor(1,1,1);
  m_fMaxViewDistHighSpec = 8000;
  m_fMaxViewDistLowSpec = 1000;
  m_fTerrainDetailMaterialsViewDistRatio = 1;
  m_vDefFogColor    = m_vFogColor;

  // mission environment
	if (szMissionName && szMissionName[0])
	{
		char szFileName[256];
		sprintf(szFileName,"mission_%s.xml",szMissionName);
		XmlNodeRef xmlMission = GetSystem()->LoadXmlFile(Get3DEngine()->GetLevelFilePath(szFileName));
		if (xmlMission)
		{
			LoadEnvironmentSettingsFromXML(xmlMission->findChild("Environment"), GetDefSID());
			LoadTimeOfDaySettingsFromXML(xmlMission->findChild("TimeOfDay"));
		}
		else
			Error( "C3DEngine::LoadMissionDataFromXMLNode: Mission file not found: %s", szFileName);
	}
	else
		Error( "C3DEngine::LoadMissionDataFromXMLNode: Mission name is not defined");
}

char * C3DEngine::GetXMLAttribText(XmlNodeRef pInputNode,const char * szLevel1,const char * szLevel2,const char * szDefaultValue)
{
  static char szResText[128];  
  strncpy(szResText,szDefaultValue,128);

	XmlNodeRef nodeLevel = pInputNode->findChild(szLevel1);
  if(nodeLevel)
	if(nodeLevel->haveAttr(szLevel2))
		strncpy(szResText, nodeLevel->getAttr(szLevel2), sizeof(szResText));
   
  return szResText;
}

char*
C3DEngine::GetXMLAttribText( XmlNodeRef pInputNode, const char* szLevel1, const char* szLevel2, const char* szLevel3, const char* szDefaultValue )
{
	static char szResText[ 128 ];  
	strncpy( szResText, szDefaultValue, 128 );

	XmlNodeRef nodeLevel = pInputNode->findChild( szLevel1 );
	if( nodeLevel )
	{
		nodeLevel = nodeLevel->findChild( szLevel2 );
		if( nodeLevel )
		{
			strncpy( szResText, nodeLevel->getAttr( szLevel3 ), sizeof( szResText ) );
		}
	}

	return szResText;
}

void C3DEngine::UpdateMoonDirection()
{
	float moonLati(-gf_PI + gf_PI * m_moonRotationLatitude / 180.0f);
	float moonLong(0.5f * gf_PI - gf_PI * m_moonRotationLongitude / 180.0f);

	float sinLon(sinf(moonLong)); float cosLon(cosf(moonLong));
	float sinLat(sinf(moonLati)); float cosLat(cosf(moonLati));

	m_moonDirection = Vec3(sinLon * cosLat, sinLon * sinLat, cosLon);
}

void C3DEngine::LoadEnvironmentSettingsFromXML(XmlNodeRef pInputNode, int nSID)
{
  PrintMessage("Loading environment settings from XML ...");

	// set start and end time for dawn/dusk (to fade moon/sun light in and out)
	float dawnTime = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "DawnTime", "355"));
	float dawnDuration = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "DawnDuration", "10"));
	float duskTime = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "DuskTime", "365"));
	float duskDuration = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "DuskDuration", "10"));

	m_dawnStart = (dawnTime - dawnDuration * 0.5f) / 60.0f;
	m_dawnEnd = (dawnTime + dawnDuration * 0.5f) / 60.0f;
	m_duskStart = 12.0f + (duskTime - duskDuration * 0.5f) / 60.0f;
	m_duskEnd = 12.0f + (duskTime + duskDuration * 0.5f) / 60.0f;

	if (m_dawnEnd > m_duskStart)
	{
		m_duskEnd += m_dawnEnd - m_duskStart;
		m_duskStart = m_dawnEnd;
	}

	// get rotation of sun around z axis (needed to define an arbitrary path over zenit for day/night cycle position calculations)
	m_sunRotationZ = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "SunRotation", "240"));
	m_sunRotationLongitude = (float) atof(GetXMLAttribText(pInputNode, "Lighting", "Longitude", "90"));

  // get scattering direction per level
  m_vScatteringDir = StringToVector(GetXMLAttribText(pInputNode,"IceScattering","ScatteringDirection","1,1,1"));
  m_vScatteringDir = m_vScatteringDir.GetNormalized() * DISTANCE_TO_THE_SUN;

	// get moon info
	m_moonRotationLatitude = (float) atof(GetXMLAttribText(pInputNode, "Moon", "Latitude", "240"));
	m_moonRotationLongitude = (float) atof(GetXMLAttribText(pInputNode, "Moon", "Longitude", "45"));
	UpdateMoonDirection();

	m_nightMoonSize = (float) atof(GetXMLAttribText(pInputNode, "Moon", "Size", "0.5"));
	
	{
		char moonTexture[256] = "";
		strncpy( moonTexture, GetXMLAttribText( pInputNode, "Moon", "Texture", "" ), sizeof( moonTexture ) );	
		moonTexture[ sizeof( moonTexture ) - 1 ] = '\0';

		ITexture* pTex( 0 );
		if( moonTexture[0] != '\0' )
			pTex = GetRenderer()->EF_LoadTexture( moonTexture, FT_DONT_STREAM, eTT_2D );

		m_nNightMoonTexId = pTex ? pTex->GetTextureID() : 0;
	}

  // max view distance
  m_fMaxViewDistHighSpec = (float)atol(GetXMLAttribText(pInputNode,"Fog","ViewDistance","8000"));
  m_fMaxViewDistLowSpec  = (float)atol(GetXMLAttribText(pInputNode,"Fog","ViewDistanceLowSpec","1000"));
  m_fMaxViewDistScale = 1.f;

	m_volFogGlobalDensityMultiplierLDR = (float) max(atof(GetXMLAttribText(pInputNode, "Fog", "LDRGlobalDensMult", "1.0")), 0.0);

  float fTerrainDetailMaterialsViewDistRatio = (float)atof(GetXMLAttribText(pInputNode,"Terrain","DetailLayersViewDistRatio","1.0"));
  if(m_fTerrainDetailMaterialsViewDistRatio != fTerrainDetailMaterialsViewDistRatio)
    GetTerrain()->ResetTerrainVertBuffers(NULL, nSID);
  m_fTerrainDetailMaterialsViewDistRatio = fTerrainDetailMaterialsViewDistRatio;

  // SkyBox
  m_skyMatName = GetXMLAttribText(pInputNode, "SkyBox", "Material", "Materials/Sky/Sky");
	m_skyLowSpecMatName = GetXMLAttribText(pInputNode, "SkyBox", "MaterialLowSpec", "Materials/Sky/Sky");

	m_fSkyBoxAngle = (float)atof(GetXMLAttribText(pInputNode,"SkyBox","Angle","0.0"));
	m_fSkyBoxStretching = (float)atof(GetXMLAttribText(pInputNode,"SkyBox","Stretching","1.0"));

  // set terrain water, sun road and bottom shaders
  char szTerrainWaterMatName[256]="";
  strncpy(szTerrainWaterMatName,GetXMLAttribText(pInputNode,"Ocean","Material","Materials/ocean/ocean_medium_waves"),sizeof(szTerrainWaterMatName));
	m_pTerrainWaterMat = szTerrainWaterMatName[0] ? m_pMatMan->LoadMaterial(szTerrainWaterMatName,false) : NULL;

	if(m_pTerrain)
	  m_pTerrain->InitTerrainWater(m_pTerrainWaterMat,  0);

	Vec3 oceanFogColor( StringToVector( GetXMLAttribText( pInputNode, "Ocean", "FogColor", "29,102,141" ) ) );
	m_oceanFogColorMultiplierEnvironment = (float) atof( GetXMLAttribText( pInputNode, "Ocean", "FogColorMultiplier", "0.2" ) ) ;
	m_oceanFogColor = oceanFogColor * (m_oceanFogColorMultiplierEnvironment / 255.0f);	
	
	Vec3 oceanFogColorShallow( 0, 0, 0); //StringToVector( GetXMLAttribText( pInputNode, "Ocean", "FogColorShallow", "206,249,253" ) ) );
	float oceanFogColorShallowMultiplier( 0); //float) atof( GetXMLAttribText( pInputNode, "Ocean", "FogColorShallowMultiplier", "0.5" ) ) );	
	m_oceanFogColorShallow = oceanFogColorShallowMultiplier * ( oceanFogColorShallow / 255.0f );	

	m_oceanFogDensity = (float) atof( GetXMLAttribText( pInputNode, "Ocean", "FogDensity", "0.2" ) );	

  m_oceanCausticsDistanceAtten = 100.0f;//(float) atof( GetXMLAttribText( pInputNode, "Ocean", "CausticsDistanceAtten", "100.0" ) );		
  m_oceanCausticsMultiplier = 0.85f;
  m_oceanCausticsDarkeningMultiplier = 1.0f;

  m_oceanWindDirection = (float) atof( GetXMLAttribText( pInputNode, "OceanAnimation", "WindDirection", "1.0" ) );		
  m_oceanWindSpeed = (float) atof( GetXMLAttribText( pInputNode, "OceanAnimation", "WindSpeed", "4.0" ) );		
  m_oceanWavesSpeed = (float) atof( GetXMLAttribText( pInputNode, "OceanAnimation", "WavesSpeed", "1.0" ) );		
  m_oceanWavesSpeed = clamp_tpl<float>(m_oceanWavesSpeed, 0.0f, 1.0f);
  m_oceanWavesAmount = (float) atof( GetXMLAttribText( pInputNode, "OceanAnimation", "WavesAmount", "1.5" ) );		
  m_oceanWavesAmount = clamp_tpl<float>( m_oceanWavesAmount , 0.4f, 3.0f);
  m_oceanWavesSize = (float) atof( GetXMLAttribText( pInputNode, "OceanAnimation", "WavesSize", "0.75" ) );		
  m_oceanWavesSize = clamp_tpl<float>( m_oceanWavesSize , 0.0f, 3.0f);

  // re-scale speed based on size - the smaller the faster waves move 
  m_oceanWavesSpeed /= clamp_tpl<float>(m_oceanWavesSize, 0.45f, 1.0f);


  //m_oceanCausticsMultiplier = (float) atof( GetXMLAttribText( pInputNode, "Ocean", "CausticsMultiplier", "1.0" ) );		
  //m_oceanCausticsDarkeningMultiplier = (float) atof( GetXMLAttribText( pInputNode, "Ocean", "CausticsDarkeningMultiplier", "1.0" ) );		

  // get wind
  Vec3 vWindSpeed = StringToVector(GetXMLAttribText(pInputNode,"EnvState","WindVector","1,0,0"));
	SetWind(vWindSpeed);

	// update relevant time of day settings 
	ITimeOfDay* pTimeOfDay( GetTimeOfDay() );
	if( pTimeOfDay )
		pTimeOfDay->Update( true, true );

	{
		const char * pText = GetXMLAttribText(pInputNode,"EnvState","ShowTerrainSurface", "true");
		m_bShowTerrainSurface = !strcmp(pText, "true") || !strcmp(pText, "1");
	}

	{
		const char * pText = GetXMLAttribText(pInputNode,"EnvState","SunShadows", "true");
		m_bSunShadows = !strcmp(pText, "true") || !strcmp(pText, "1");
	}

	// load cloud shadow texture
	{
    float fCloudShadowSpeed = (float)atof(GetXMLAttribText( pInputNode, "EnvState", "CloudShadowSpeed", "0.0"));

		char cloudShadowTexture[256] = "";
		strncpy( cloudShadowTexture, GetXMLAttribText( pInputNode, "EnvState", "CloudShadowTexture", "" ), sizeof( cloudShadowTexture ) );	
		cloudShadowTexture[ sizeof( cloudShadowTexture ) - 1 ] = '\0';

		ITexture* pTex( 0 );
		if( cloudShadowTexture[0] != '\0' )
			pTex = GetRenderer()->EF_LoadTexture( cloudShadowTexture, FT_DONT_RELEASE, eTT_2D );
		
		m_nCloudShadowTexId = pTex ? pTex->GetTextureID() : 0;
		GetRenderer()->SetCloudShadowTextureId( m_nCloudShadowTexId, vWindSpeed*fCloudShadowSpeed );
	}

  PrintMessagePlus(" ok");
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadTimeOfDaySettingsFromXML( XmlNodeRef node )
{
	if (node)
	{
		GetTimeOfDay()->Serialize(node,true);
		ITimeOfDay::SAdvancedInfo info;
		GetTimeOfDay()->GetAdvancedInfo( info );
		GetTimeOfDay()->SetTime( info.fStartTime,true );
	}
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::LoadParticleEffects( XmlNodeRef &levelDataRoot )
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

	if (!m_pPartManager)
		return;

	if (!m_bEditor)
	{
		m_pPartManager->LoadLibrary( "Level", GetLevelFilePath(PARTICLES_FILE), true );

		if (GetCVars()->e_ParticlesPreload)
		{
			// Force loading all effects and assets, to ensure no runtime stalls.
			CTimeValue t0 = GetTimer()->GetAsyncTime();
			PrintMessage( "Preloading Particle Effects..." );
			m_pPartManager->LoadLibrary( "*", NULL, true );
			CTimeValue t1 = GetTimer()->GetAsyncTime();
			float dt = (t1-t0).GetSeconds();
			PrintMessage( "Particle Effects Loaded in %.2f seconds",dt );
		}
		else
		{
			// Load just specified libraries.
			m_pPartManager->LoadLibrary( "@PreloadLibs", NULL, true );
		}
	}
}

//! create static object containing empty IndexedMesh
IStatObj * C3DEngine::CreateStatObj()
{
	CStatObj * pStatObj = new CStatObj();
	pStatObj->m_pIndexedMesh = new CIndexedMesh();
	return pStatObj;
}

bool C3DEngine::RestoreTerrainFromDisk(int nSID)
{
	if(m_pTerrain && m_pObjManager && !m_bEditor && GetCVars()->e_TerrainDeformations)
	{
		m_pTerrain->ResetTerrainVertBuffers(NULL,nSID);

    if(FILE * f = GetPak()->FOpen(GetLevelFilePath(COMPILED_HEIGHT_MAP_FILE_NAME), "rbx"))
    {
      GetTerrain()->ReloadModifiedHMData(f,nSID);
      GetPak()->FClose(f);
    }
	}

	ResetParticlesAndDecals();

  // update roads
  for(int id=0; id<Get3DEngine()->m_pObjectsTree.Count(); id++)
  {
    if(m_pObjectsTree[id] && GetCVars()->e_TerrainDeformations)
    {
      PodArray<IRenderNode*> lstRoads;
      m_pObjectsTree[id]->GetObjectsByType(lstRoads, eERType_Road, NULL);
      for(int i=0; i<lstRoads.Count(); i++)
      {
        CRoadRenderNode * pRoad = (CRoadRenderNode *)lstRoads[i];
        pRoad->OnTerrainChanged();
      }
    }
  }

  ReRegisterKilledVegetationInstances();

	return true;
}

void C3DEngine::ReRegisterKilledVegetationInstances()
{
  for(int i=0; i<m_lstKilledVegetations.Count(); i++)
  {
    IRenderNode * pObj = m_lstKilledVegetations[i];
    pObj->Physicalize();
    Get3DEngine()->RegisterEntity(pObj);
  }

  m_lstKilledVegetations.Clear();
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngine::LoadUsedShadersList()
{
	LOADING_TIME_PROFILE_SECTION(GetSystem());

	CCryFile file;
	if (file.Open( GetLevelFilePath(SHADER_LIST_FILE),"rb" ))
	{
		int nSize = file.GetLength();
		char *pShaderData = new char[nSize+1];
		file.ReadRaw( pShaderData,nSize );
		pShaderData[nSize] = '\0'; // Make null terminted string.

		gEnv->pRenderer->EF_Query(EFQ_SetShaderCombinations, (INT_PTR)pShaderData );

		delete []pShaderData;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngine::PrecreateDecals()
{
	LOADING_TIME_PROFILE_SECTION(GetSystem());

	CObjManager::DecalsToPrecreate& decals( GetObjManager()->m_decalsToPrecreate );
	// pre-create ...
	if( GetCVars()->e_DecalsPreCreate )
	{
		CryLog( "Pre-creating %d decals...", (int)decals.size() );

		CObjManager::DecalsToPrecreate::iterator it( decals.begin() );
		CObjManager::DecalsToPrecreate::iterator itEnd( decals.end() );
		for( ; it != itEnd; ++it )
		{
			IDecalRenderNode* pDecalRenderNode( *it );
			pDecalRenderNode->Precache();
		}

		CryLog( " done.\n" );
	}
	else
		CryLog( "Skipped pre-creation of decals.\n" );
	
	// ... and discard list (even if pre-creation was skipped!)
	decals.resize( 0 );

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Called by game when everything needed for level is loaded.
//////////////////////////////////////////////////////////////////////////
void C3DEngine::PostLoadLevel()
{
	//////////////////////////////////////////////////////////////////////////
	// Submit water material to physics
	//////////////////////////////////////////////////////////////////////////
	{
		IMaterialManager* pMatMan = GetMaterialManager();
		IPhysicalWorld *pPhysicalWorld = gEnv->pPhysicalWorld;
		IPhysicalEntity *pGlobalArea = pPhysicalWorld->AddGlobalArea();

		pe_params_buoyancy pb;
		pb.waterPlane.n.Set(0,0,1);
		pb.waterPlane.origin.Set(0,0,GetWaterLevel());
		pGlobalArea->SetParams(&pb);

		ISurfaceType *pSrfType = pMatMan->GetSurfaceTypeByName("mat_water");
		if (pSrfType)
			pPhysicalWorld->SetWaterMat( pSrfType->GetId() );

		pe_params_waterman pwm;
		pwm.nExtraTiles = 3;
		pwm.nCells = 40;
		pwm.tileSize = 4;
		pwm.waveSpeed = 11.0f;
		pwm.dampingCenter = 0.6f;
		pwm.dampingRim = 2.3f;
		//pPhysicalWorld->SetWaterManagerParams(&pwm);
	}

  //////////////////////////////////////////////////////////////////////////
  // Load and activate all shaders used by the level.
  //////////////////////////////////////////////////////////////////////////
  if (!m_bEditor)
    LoadUsedShadersList();

	if(GetCVars()->e_PrecacheLevel)
	{
		// pre-create decals
		PrecreateDecals();
	}

  CompleteObjectsGeometry();

#ifndef EXCLUDE_GPU_PARTICLE_PHYSICS
	IGPUPhysicsManager *pGPUPhysics;

	pGPUPhysics = m_pSystem->GetIGPUPhysicsManager();
	if( pGPUPhysics != NULL )
	{
		pGPUPhysics->SetupSceneRelatedData();
	}
#endif

	GetRenderer()->PrecacheResources();
}


int C3DEngine::SaveStatObj(IStatObj *pStatObj, TSerialize ser)
{
	bool bVal;
	if (!(pStatObj->GetFlags() & STATIC_OBJECT_GENERATED))
	{
		ser.Value("altered",bVal=false);
		ser.Value("file",pStatObj->GetFilePath());
		ser.Value("geom",pStatObj->GetGeoName());
	}	else 
	{
		ser.Value("altered",bVal=true);
		ser.Value("CloneSource", pStatObj->GetCloneSourceObject() ? pStatObj->GetCloneSourceObject()->GetFilePath():"0");
		pStatObj->Serialize(ser);
	}

	return 1;
}

IStatObj *C3DEngine::LoadStatObj(TSerialize ser)
{
	bool bVal;
	IStatObj *pStatObj;
	ser.Value("altered",bVal);
	if (!bVal)
	{
		string fileName,geomName;
		ser.Value("file",fileName);
		ser.Value("geom",geomName);
		pStatObj = LoadStatObj(fileName,geomName);
	} else
	{
		string srcObjName;
		ser.Value("CloneSource", srcObjName);
		if (*(const unsigned short*)(const char*)srcObjName!='0')
			pStatObj = LoadStatObj(srcObjName)->Clone(false,false,true);
		else
			pStatObj = CreateStatObj();
		pStatObj->Serialize(ser);
	}
	return pStatObj;
}
