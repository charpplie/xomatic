////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   3dengine.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Implementation of I3DEngine interface methods
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <ICryAnimation.h>
#include <IFaceGen.h>
#include <IGameFramework.h>

#include "3dEngine.h"
#include "terrain.h"
#include "VisAreas.h"
#include "ObjMan.h"
#include "terrain_water.h"

#include "DecalManager.h"
#include "Vegetation.h"
#include "IndexedMesh.h"
#include "WaterVolumes.h"

#include "MatMan.h"

#include "Brush.h"
#include "CullBuffer.h"
#include "CGF/CGFLoader.h"
#include "CGF/ReadOnlyChunkFile.h"

#include "CloudRenderNode.h"
#include "CloudsManager.h"
#include "SkyLightManager.h"
#include "FogVolumeRenderNode.h"
#include "RoadRenderNode.h"
#include "DecalRenderNode.h"
#include "TimeOfDay.h"
#include "VoxMan.h"
#include "LightEntity.h"
#include "FogVolumeRenderNode.h"
#include "ObjectsTree.h"
#include "WaterVolumeRenderNode.h"
#include "WaterWaveRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "VolumeObjectRenderNode.h"
#include "AutoCubeMapRenderNode.h"
#include "WaterWaveRenderNode.h"
#include "RopeRenderNode.h"
#include "RenderMeshMerger.h"
#include "PhysCallbacks.h"
#include "VoxTerrain.h"
#include "IrradianceVolumeRenderNode.h"
#include "ColorBleeding.h"

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
#include "PrismRenderNode.h"
#endif // EXCLUDE_DOCUMENTATION_PURPOSE


//#ifdef WIN32
//#include <windows.h>
//#endif

uint Cry3DEngineBase::m_nMainThreadId = 0;
ISystem * Cry3DEngineBase::m_pSystem=0;
IRenderer * Cry3DEngineBase::m_pRenderer=0;
ITimer * Cry3DEngineBase::m_pTimer=0;
ILog * Cry3DEngineBase::m_pLog=0;
IPhysicalWorld * Cry3DEngineBase::m_pPhysicalWorld=0;
CTerrain * Cry3DEngineBase::m_pTerrain=0;
class CVoxTerrain * Cry3DEngineBase::m_pVoxTerrain=0;
CObjManager * Cry3DEngineBase::m_pObjManager=0;
IConsole * Cry3DEngineBase::m_pConsole=0;
C3DEngine * Cry3DEngineBase::m_p3DEngine=0;
CVars * Cry3DEngineBase::m_pCVars=0;
ICryPak * Cry3DEngineBase::m_pCryPak=0;
IParticleManager * Cry3DEngineBase::m_pPartManager=0;
CDecalManager  * Cry3DEngineBase::m_pDecalManager=0;
CSkyLightManager* Cry3DEngineBase::m_pSkyLightManager=0;
CCloudsManager * Cry3DEngineBase::m_pCloudsManager=0;
CVisAreaManager* Cry3DEngineBase::m_pVisAreaManager=0;
CWaterWaveManager  * Cry3DEngineBase::m_pWaterWaveManager=0;
CRenderMeshMerger  * Cry3DEngineBase::m_pRenderMeshMerger = 0;
CMatMan        * Cry3DEngineBase::m_pMatMan=0;
int Cry3DEngineBase::m_nRenderStackLevel=-1;
float Cry3DEngineBase::m_fZoomFactor=1.f;
int Cry3DEngineBase::m_dwRecursionDrawFlags[MAX_RECURSION_LEVELS];
int Cry3DEngineBase::m_nRenderFrameID = 0;
uint32 Cry3DEngineBase::m_nRenderMainFrameID = 0;
int Cry3DEngineBase::m_nRenderThreadListID = 0;
bool Cry3DEngineBase::m_bProfilerEnabled = false;
bool Cry3DEngineBase::m_bRenderIntoShadowmap = false;
int Cry3DEngineBase::m_CpuFlags=0;
#if (!defined(PS3) && !defined(XENON))
	bool Cry3DEngineBase::m_bEditor = false;
#endif
ESystemConfigSpec Cry3DEngineBase::m_LightConfigSpec = CONFIG_VERYHIGH_SPEC;
IMaterialManager* Cry3DEngineBase::m_pMaterialManager = NULL;
CCamera Cry3DEngineBase::m_Camera;
int Cry3DEngineBase::m_arrInstancesCounter[eERType_TypesNum];
IGetLayerIdAtCallback * C3DEngine::m_pGetLayerIdAtCallback = 0;


#define LAST_POTENTIALLY_VISIBLE_TIME 2

// The only direct particle function needed by 3DEngine, implemented in the same DLL.
extern IParticleManager* CreateParticleManager(bool bEnable);

//////////////////////////////////////////////////////////////////////
C3DEngine::C3DEngine(ISystem	* pSystem)
{
//#if defined(_DEBUG) && defined(WIN32)
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif

  CVegetation::InitVegDecomprTable();

	memset(m_dwRecursionDrawFlags,0,sizeof(m_dwRecursionDrawFlags));

  Cry3DEngineBase::m_nMainThreadId = CryGetCurrentThreadId();
  Cry3DEngineBase::m_pSystem=pSystem;
  Cry3DEngineBase::m_pRenderer=gEnv->pRenderer;
  Cry3DEngineBase::m_pTimer=gEnv->pTimer;
  Cry3DEngineBase::m_pLog=gEnv->pLog;
  Cry3DEngineBase::m_pPhysicalWorld=gEnv->pPhysicalWorld;
  Cry3DEngineBase::m_pConsole=gEnv->pConsole;
  Cry3DEngineBase::m_p3DEngine=this;
  Cry3DEngineBase::m_pCryPak=gEnv->pCryPak;
  Cry3DEngineBase::m_pCVars=0;
	Cry3DEngineBase::m_pRenderMeshMerger = new CRenderMeshMerger;
  Cry3DEngineBase::m_CpuFlags=pSystem->GetCPUFlags();
	memset(Cry3DEngineBase::m_arrInstancesCounter,0,sizeof(Cry3DEngineBase::m_arrInstancesCounter));

#if (!defined(PS3) && !defined(XENON))
  m_bEditor = pSystem->IsEditor();
#endif

	m_pCVars            = new CVars();
	Cry3DEngineBase::m_pCVars = m_pCVars;

#ifdef _DEBUG
#ifndef _XBOX
//  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // _crtBreakAlloc
#endif
#endif

  m_pMatMan = new CMatMan();

	m_pTimeOfDay = new CTimeOfDay;
 
	m_szLevelFolder[0]=0;

	m_pSun=0;
	m_nFlags=0;
  m_pSkyMat = 0;
	m_pSkyLowSpecMat = 0;
  m_pTerrainWaterMat = 0;
  m_pTerrainMat = 0;
  m_pVoxTerrainMat = 0;
	m_nWaterBottomTexId=0;
  m_vSunDir = Vec3(5.f, 5.f, DISTANCE_TO_THE_SUN);
  m_vSunDirRealtime = Vec3(5.f, 5.f, DISTANCE_TO_THE_SUN).GetNormalized();

  m_pTerrain=0;	

	m_nBlackTexID = 0;

  // create components
  m_pObjManager = new CObjManager();

  m_pDecalManager     = 0;//new CDecalManager   (m_pSystem, this);
	m_pCloudsManager    = new CCloudsManager;
	m_pPartManager			= 0;
  m_pVisAreaManager   = 0;
	m_pSkyLightManager  = new CSkyLightManager();
  m_pWaterWaveManager = 0;

  // create REs
  m_pRESky              = 0;
	m_pREHDRSky           = 0;
  m_pREPostProcess = 0;
  
  m_pFarTreeSprites   = 0;

  m_pPhysMaterialEnumerator=0;

  m_fMaxViewDistHighSpec = 8000;
  m_fMaxViewDistLowSpec  = 1000;
  m_fTerrainDetailMaterialsViewDistRatio = 1.f;

  m_fSkyBoxAngle=0;
	m_fSkyBoxStretching=0;

	m_pGlobalWind	= 0;
	m_vWindSpeed(1,0,0);

	m_bOcean = true;
	m_bCameraUnderWater = false;
  m_nOceanRenderFlags[0] = m_nOceanRenderFlags[1] = 0;

	m_bSunShadows = m_bShowTerrainSurface = true;

	m_nRenderLightsNum=m_nRealLightsNum=m_nDeferredLightsNum=0;

  m_bHRDEnabled=0;
  m_pLightQuality = GetConsole()->GetCVar("r_Quality_BumpMapping");
  m_pResetSfx=GetConsole()->GetCVar("r_ResetScreenFx");

  m_pHRDRendering=GetConsole()->GetCVar("r_HDRRendering"); 
	m_pSphericalSkinning=GetConsole()->GetCVar("ca_SphericalSkinning");
  m_pDeferredShading = GetConsole()->GetCVar("r_DeferredShading"); 
  m_pDeferredShadingDebug = GetConsole()->GetCVar("r_DeferredShadingDebug"); 
  m_pDeferredShadingILSamples= GetConsole()->GetCVar("r_DeferredShadingILSamples"); 

	m_pConsolepFont = NULL;

	m_pCoverageBuffer = new CCullBuffer();

	union {
		CStatObjFoliage* CStatObjFoliage::*next;
		INT_PTR inext;
	} tmp;
	tmp.inext = 0; tmp.next = &CStatObjFoliage::m_next;
	m_pFirstFoliage = m_pLastFoliage = (CStatObjFoliage*)((INT_PTR)&m_pFirstFoliage-tmp.inext);

	m_fHDRDynamicMultiplier=2.0f;

	m_vSkyHightlightPos.Set(0,0,0);
	m_vSkyHightlightCol.Set(0,0,0);
	m_fSkyHighlightSize = 0;

  m_bRenderIntoShadowmap = false;

	m_volFogGlobalDensity = 0.02f;
	m_volFogGlobalDensityMultiplierLDR = 1.0f;
	m_volFogAtmosphereHeight = 4000.0f;
	m_volFogArtistTweakDensityOffset = 0.0f;

	m_volFogGlobalDensityMod = 0;
	m_volFogAtmosphereHeightMod = 0;

//	m_pObjectsTree[nSID] = NULL;
//  m_pSceneTree = NULL;

	m_idMatLeaves = -1;

/* // structures should not grow too much - commented out as in 64bit they do
	assert(sizeof(CVegetation)-sizeof(IRenderNode)<=52);
	assert(sizeof(CBrush)-sizeof(IRenderNode)<=120);
	assert(sizeof(IRenderNode)<=96);
*/
	m_oceanFogColor = 0.2f * Vec3( 29.0f, 102.0f, 141.0f ) / 255.0f;
	m_oceanFogColorShallow = Vec3(0,0,0); //0.5f * Vec3( 206.0f, 249.0f, 253.0f ) / 255.0f;
	m_oceanFogDensity = 0; //0.2f;

  m_oceanCausticsDistanceAtten = 100.0f; 
  m_oceanCausticsMultiplier = 1.0f;  
  m_oceanCausticsDarkeningMultiplier = 1.0f;

  m_oceanWindDirection = 1;
  m_oceanWindSpeed = 4.0f;
  m_oceanWavesSpeed = 5.0f;
  m_oceanWavesAmount = 1.5f;
  m_oceanWavesSize = 0.75f;

	m_fRefreshSceneDataCVarsSumm = -1;

	if (!m_LTPRootFree.pNext)
	{
		m_LTPRootFree.pNext = &m_LTPRootFree;
		m_LTPRootFree.pPrev = &m_LTPRootFree;
	}

	if (!m_LTPRootUsed.pNext)
	{
		m_LTPRootUsed.pNext = &m_LTPRootUsed;
		m_LTPRootUsed.pPrev = &m_LTPRootUsed;
	}
/*
	CTerrainNode::SetProcObjChunkPool(new SProcObjChunkPool);
	CTerrainNode::SetProcObjPoolMan(new CProcVegetPoolMan);
  */
  m_bResetRNTmpDataPool = false;

  m_fSunDirUpdateTime = 0;
	m_vSunDirNormalized.zero();

	m_volFogRamp = Vec3(0, 100.0f, 0);
	m_nightSkyHorizonCol = Vec3(0,0,0);
	m_nightSkyZenithCol = Vec3(0,0,0);
	m_nightSkyZenithColShift = 0;
	m_nightSkyStarIntensity = 0;
	m_moonDirection = Vec3(0,0,0);
	m_nightMoonCol = Vec3(0,0,0);
	m_nightMoonSize = 0;
	m_nightMoonInnerCoronaCol = Vec3(0,0,0);
	m_nightMoonInnerCoronaScale = 1.0f;
	m_nightMoonOuterCoronaCol = Vec3(0,0,0);
	m_nightMoonOuterCoronaScale = 1.0f;
	m_sunRotationZ = 0;
	m_sunRotationLongitude = 90.0f;
	m_moonRotationLatitude = 0;
	m_moonRotationLongitude = 0;
	m_oceanFogColorMultiplier = 0;
	m_oceanFogColorMultiplierEnvironment= 0;
	m_skyboxMultiplier = 1.0f;
	m_dayNightIndicator = 1.0f;
	m_fogColor2 = Vec3(0,0,0);

	m_dawnStart = 350.0f / 60.0f;
	m_dawnEnd = 360.0f / 60.0f;
	m_duskStart = 12.0f + 360.0f / 60.0f;
	m_duskEnd = 12.0f + 370.0f / 60.0f;

  m_vPrevMainFrameCamPos.Set(-1000000.f,-1000000.f,-1000000.f);
  m_bContentPrecacheRequested = false;

//  for(int i=0 ;i<MAX_SURFACE_TYPES_COUNT; i++)
  //  m_arrSurfTypeMapping[i] = -1;

	m_pColorBleeding = NULL;

	ClearDebugFPSInfo();

  m_fMaxViewDistScale=1.f;

	m_ptexIconLowMemoryUsage=NULL;
	m_ptexIconAverageMemoryUsage=NULL;
	m_ptexIconHighMemoryUsage=NULL;
	m_ptexIconEditorConnectedToConsole=NULL;
	m_pScreenshotCallback = 0;
}

//////////////////////////////////////////////////////////////////////
C3DEngine::~C3DEngine()
{
	delete CTerrainNode::GetProcObjPoolMan();
	CTerrainNode::SetProcObjChunkPool(NULL);

	delete CTerrainNode::GetProcObjChunkPool();
	CTerrainNode::SetProcObjPoolMan(NULL);

	assert(IsHeapValid());

	ShutDown();

	delete m_pTimeOfDay;
  delete m_pDecalManager; 
  delete m_pVisAreaManager;
  delete m_pCVars;
	delete m_pMatMan; m_pMatMan=0;
	delete m_pCoverageBuffer; m_pCoverageBuffer=0;
	delete m_pSkyLightManager;
  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  	SAFE_DELETE(m_pObjectsTree[nSID]);
//  delete m_pSceneTree;
  delete m_pRenderMeshMerger;
  delete m_pCloudsManager;

	SAFE_RELEASE(m_ptexIconAverageMemoryUsage);
	SAFE_RELEASE(m_ptexIconLowMemoryUsage);
	SAFE_RELEASE(m_ptexIconHighMemoryUsage);

	SAFE_RELEASE(m_ptexIconEditorConnectedToConsole);
}

//////////////////////////////////////////////////////////////////////

void C3DEngine::RemoveEntInFoliage(int i, IPhysicalEntity *pent)
{
	IPhysicalEntity *pent1 = m_pPhysicalWorld->GetPhysicalEntityById(m_arrEntsInFoliage[i].id);
	pe_params_foreign_data pfd;
	if (!pent)
		pent = pent1;
	else if (pent!=pent1)
	{	// probably someone overwrote foreign flags 
		for(i=m_arrEntsInFoliage.size()-1; i>=0 && pent!=m_pPhysicalWorld->GetPhysicalEntityById(m_arrEntsInFoliage[i].id); i--);
		if (i<0)
			return;
	}
	if (pent && pent->GetParams(&pfd) && (pfd.iForeignFlags>>8&255)==i+1)
	{
		MARK_UNUSED pfd.pForeignData,pfd.iForeignData,pfd.iForeignFlags;
		pfd.iForeignFlagsAND = 255;
		pent->SetParams(&pfd,1);
	}
	int j = m_arrEntsInFoliage.size()-1;
	if (i<j)
	{
		m_arrEntsInFoliage[i] = m_arrEntsInFoliage[j];
		if ((pent=m_pPhysicalWorld->GetPhysicalEntityById(m_arrEntsInFoliage[i].id)) && pent->GetParams(&pfd) && 
			(pfd.iForeignFlags>>8&255)==j+1)
		{
			MARK_UNUSED pfd.pForeignData,pfd.iForeignData;
      pfd.iForeignFlags = pfd.iForeignFlags & 255 | (i+1)<<8;
			pent->SetParams(&pfd,1);
		}
	}
	m_arrEntsInFoliage.DeleteLast();
}

bool C3DEngine::Init()
{	  	
  ShutDown();

	m_pPartManager = CreateParticleManager(!gEnv->pSystem->IsDedicated());
	m_pSystem->SetIParticleManager(m_pPartManager);

	ICryFont *pCryFont = gEnv->pCryFont;
	if (pCryFont)
	{
		m_pConsolepFont = pCryFont->GetFont("console");
	}
	if (GetPhysicalWorld())
	{
		CPhysCallbacks::Init();
	}

	if (!gEnv->pSystem->IsDedicated())
		m_pColorBleeding = new CColorBleeding();
	else
		m_pColorBleeding = 0;

	return  (true);
}

bool C3DEngine::IsCameraAnd3DEngineInvalid(const CCamera cam, const char * szCaller)
{
	if (!m_pObjManager || !m_pDecalManager)
		return (true); 

	if( !_finite(cam.GetMatrix().m03) || !_finite(cam.GetMatrix().m13) || !_finite(cam.GetMatrix().m23) || GetMaxViewDistance()<=0 ||
      cam.GetMatrix().m23 < -100000.f || cam.GetMatrix().m23 > 100000.f || cam.GetFov() < 0.0001f || cam.GetFov() > gf_PI)
  {
    Error("Bad camera passed to 3DEngine from %s: Pos=(%.1f, %.1f, %.1f), Fov=%.1f, MaxViewDist=%.1f", 
      szCaller, 
      cam.GetMatrix().m03, cam.GetMatrix().m13, cam.GetMatrix().m23, 
      cam.GetFov(), _finite(cam.GetMatrix().m03) ? GetMaxViewDistance() : 0);
    return true;
  }

  return false;
}

void C3DEngine::OnFrameStart()
{
	FUNCTION_PROFILER_3DENGINE;

	if (m_pPartManager)
		m_pPartManager->OnFrameStart();
}

float g_oceanLevel,g_oceanStep;
float GetOceanLevelCallback(int ix,int iy)
{
	return gEnv->p3DEngine->GetOceanWaterLevel(Vec3(ix*g_oceanStep,iy*g_oceanStep,g_oceanLevel));	
	/*pe_status_waterman psw;
	gEnv->pPhysicalWorld->GetWatermanStatus(&psw);
	Vec2 pos = Vec2((float)ix,(float)iy)*((C3DEngine*)gEnv->p3DEngine)->GetCVars()->e_PhysOceanCell-Vec2(psw.origin);
	Vec2i itile(FtoI(pos.x*0.25f-0.5f), FtoI(pos.y*0.25f-0.5f));
	if ((itile.x|itile.y)<0 || max(itile.x,itile.y)>6)
		return g_oceanLevel;
	SWaterTileBase *pTile = psw.pTiles[itile.x+itile.y*7];
	if (!pTile || !pTile->bActive)
		return g_oceanLevel;
	Vec2i ipos(FtoI((pos.x-itile.x*4)*10.0f-0.5f), FtoI((pos.y-itile.y*4)*10.0f-0.5f));
	return psw.origin.z+pTile->ph[itile.x+itile.y*40];*/
}
unsigned char GetOceanSurfTypeCallback(int ix,int iy)
{
	return 0;	
}

void C3DEngine::Update()
{
	m_bProfilerEnabled = gEnv->pFrameProfileSystem->IsProfiling();

  FUNCTION_PROFILER_3DENGINE;

	m_LightConfigSpec = (ESystemConfigSpec)GetCurrentLightSpec();

///	if(m_pVisAreaManager)
	//	m_pVisAreaManager->Preceche(m_pObjManager);

	if (GetObjManager())
		GetObjManager()->ClearStatObjGarbage();

	if(m_pColorBleeding)
		m_pColorBleeding->UpdatePosition(GetCamera());

	if(m_pTerrain)
  {
    m_pTerrain->Voxel_Recompile_Modified_Incrementaly_Objects();
    m_pTerrain->Recompile_Modified_Incrementaly_RoadRenderNodes();
  }

	if(m_pDecalManager)
		m_pDecalManager->Update(GetTimer()->GetFrameTime());

	if(GetCVars()->e_PrecacheLevel == 3)
		PrecacheLevel(true,0,0);

	if(GetCVars()->e_VoxelGenerate)
		GetTerrain()->BuildVoxelSpace();

	DebugDraw_Draw();

  ProcessCVarsChange();

	float dt = GetTimer()->GetFrameTime();
	CStatObjFoliage *pFoliage,*pFoliageNext;
	for(pFoliage=m_pFirstFoliage; &pFoliage->m_next!=&m_pFirstFoliage; pFoliage=pFoliageNext) {
		pFoliageNext = pFoliage->m_next;
		pFoliage->Update(dt);
	}
	for(int i=m_arrEntsInFoliage.size()-1;i>=0;i--) if ((m_arrEntsInFoliage[i].timeIdle+=dt)>0.3f)
		RemoveEntInFoliage(i);

	pe_params_area pa;
	IPhysicalEntity *pArea;
	if ((pArea = gEnv->pPhysicalWorld->AddGlobalArea())->GetParams(&pa))
  {
		if (GetCVars()->e_PhysOceanCell>0 && (!pa.pGeom || g_oceanStep!=GetCVars()->e_PhysOceanCell))
		{
			pa = pe_params_area();
			primitives::heightfield hf;
			hf.origin.zero().z = g_oceanLevel = GetWaterLevel();
			g_oceanStep = GetCVars()->e_PhysOceanCell;
			hf.size.set((int)(8192/g_oceanStep),(int)(8192/g_oceanStep));
			hf.step.set(g_oceanStep,g_oceanStep);
			hf.fpGetHeightCallback = GetOceanLevelCallback;
			hf.fpGetSurfTypeCallback = GetOceanSurfTypeCallback;
			hf.Basis.SetIdentity();
			hf.bOriented = 0;
			hf.typemask = 255;
			hf.typehole = 255;
			hf.heightscale = 1.0f;
			pa.pGeom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::heightfield::type, &hf);
			pArea->SetParams(&pa);

			Vec4 par0,par1;
			GetOceanAnimationParams(par0,par1);
			pe_params_buoyancy pb;
			pb.waterFlow = Vec3(par1.z,par1.y,0)*(par0.z*0.25f); // tone down the speed
			//pArea->SetParams(&pb);
		} 
		else if (GetCVars()->e_PhysOceanCell<=0 && pa.pGeom)
		{
			pa = pe_params_area();
			pa.pGeom = 0;
			pArea->SetParams(&pa);
			pe_params_buoyancy pb;
			pb.waterFlow.zero();
			pArea->SetParams(&pb);
		}
  }

  CRenderMeshUtils::ClearHitCache();
}

void C3DEngine::ProcessCVarsChange()
{
  float fNewCVarsSumm = 
      GetCVars()->e_VegetationAlignToTerrainAmount +
      GetCVars()->e_VoxTerHideIntegrated +
			GetCVars()->e_ShadowsCastViewDistRatio +
      GetCVars()->e_Dissolve +
      GetCVars()->e_VegetationUseTerrainColor +
      GetCVars()->e_TerrainDetailMaterials + 
      GetCVars()->e_ViewDistRatio + 
      GetCVars()->e_ViewDistMin + 
      GetCVars()->e_ViewDistRatioDetail + 
      GetCVars()->e_ViewDistRatioVegetation + 
      GetCVars()->e_DefaultMaterial +
      GetCVars()->e_VegetationSpritesDistanceRatio +
      GetCVars()->e_VegetationSpritesDistanceCustomRatioMin +
      GetCVars()->e_VegetationSpritesMinDistance +
      GetGeomDetailScreenRes() +
      GetCVars()->e_Portals +
      GetTerrain()->IsAmbientOcclusionEnabled() +
      GetCVars()->e_DebugDraw +
      GetCVars()->e_ViewDistCompMaxSize;

  if( m_fRefreshSceneDataCVarsSumm != -1 && m_fRefreshSceneDataCVarsSumm != fNewCVarsSumm )
  {
    UpdateStatInstGroups();

    // re-register every instance in level
		const float terrainSize = (float)GetTerrainSize();
    GetObjManager()->ReregisterEntitiesInArea(
      Vec3(-terrainSize,-terrainSize,-terrainSize), 
      Vec3(terrainSize*2.f,terrainSize*2.f,terrainSize*2.f));

    // force recreation of terrain meshes
    GetTerrain()->ResetTerrainVertBuffers(NULL,-1);

    // refresh vegetation properties
    UpdateStatInstGroups();

    // force refresh of temporary data associated with visible objects
    MarkRNTmpDataPoolForReset();
  }

  m_fRefreshSceneDataCVarsSumm = fNewCVarsSumm;

  {
    float fNewCVarsSumm2 = 
      float(int(GetSkyColor().GetLength()*10)/10) + 
      GetCVars()->e_LodRatio;

    static float fCVarsSumm2 = fNewCVarsSumm2;

    if(fCVarsSumm2 != fNewCVarsSumm2)
    {
      MarkRNTmpDataPoolForReset();

      fCVarsSumm2 = fNewCVarsSumm2;
    }
  }
}

//////////////////////////////////////////////////////////////////////
void C3DEngine::UpdateScene()
{	
	if(GetCVars()->e_Sun)
		UpdateSunLightSource();

	FindPotentialLightSources();

	// Set traceable fog volume areas
	CFogVolumeRenderNode::SetTraceableArea( AABB( GetCamera().GetPosition() - Vec3( 1024.0f, 1024.0f, 1024.0f ), GetCamera().GetPosition() + Vec3( 1024.0f, 1024.0f, 1024.0f ) ) );

	/*
	if(GetCVars()->e_stream_preload_textures && Cry3DEngineBase::m_nRenderStackLevel==0)
	{
		m_fPreloadStartTime = GetCurTimeSec();
		bool bPreloadOutdoor = m_pVisAreaManager->PreloadResources();

		//    for(int p=0; p<16; p++)
		if(bPreloadOutdoor && m_pTerrain->PreloadResources() && m_pSkyBoxMat)
		{
			FRAME_PROFILER( "Renderer::EF_PrecacheResource", GetSystem(), PROFILE_RENDERER );
			GetRenderer()->EF_PrecacheResource(m_pSkyBoxMat->GetShaderItem().m_pShader, 0, 1.f, 0);
		}
	}
	*/
	/*
		// precache tarrain data if camera was teleported more than 32 meters
		if(m_nRenderStackLevel==0)
		{
			if(GetDistance(m_pTerrain->m_vPrevCameraPos, GetCamera().GetPosition()) > 32)
				m_pTerrain->PreCacheArea(GetCamera().GetPosition(), GetCamera().GetZMax()*1.5f);
				m_pTerrain->m_vPrevCameraPos = GetCamera().GetPosition();
		}
	*/
}

//////////////////////////////////////////////////////////////////////
void C3DEngine::ShutDown()
{
	PrintMessage("Removing physicalized foliages ...");
	CStatObjFoliage *pFoliage,*pFoliageNext;
	for(pFoliage=m_pFirstFoliage; &pFoliage->m_next!=&m_pFirstFoliage; pFoliage=pFoliageNext) {
		pFoliageNext = pFoliage->m_next;
		delete pFoliage;
	}
	m_arrEntsInFoliage.Reset();

	if(GetRenderer() != GetSystem()->GetIRenderer())
		CryFatalError("Renderer was deallocated before I3DEngine::ShutDown() call");

	ClearRenderResources();

  // reinit console variables
//  delete m_pCVars;
//  m_pCVars = new CVars(m_pSystem);

  PrintMessage("Removing lights ...");
	SAFE_DELETE(m_pColorBleeding);
	for(int i=0; i<m_lstDynLights.Count(); i++)
  {
    CDLight * pLight = m_lstDynLights[i];
    FreeLightSourceComponents(pLight);
  }
  DeleteAllStaticLightSources();

//  PrintMessage("Deleting visareas ...");
  delete m_pVisAreaManager;
  m_pVisAreaManager = 0;

//  PrintMessage("Deleting terrain ...");
  delete m_pTerrain;
  m_pTerrain=0;

//  PrintMessage("Deleting 3d terrain ...");
  delete m_pVoxTerrain;
  m_pVoxTerrain=0;

  //PrintMessage("Deleting merged scene tree ...");
 // SAFE_DELETE(m_pSceneTree);

//	PrintMessage("Deleting outdoor ...");
  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
    SAFE_DELETE(m_pObjectsTree[nSID]);

  if (m_pGlobalWind != 0)
	{
		GetPhysicalWorld()->DestroyPhysicalEntity( m_pGlobalWind );
		m_pGlobalWind = 0;
	}

	UnlockCGFResources();

	PrintMessage("PartManager shutdown ...");
  delete m_pPartManager;
	m_pPartManager=0;
	m_pSystem->SetIParticleManager(m_pPartManager);

  PrintMessage("ObjManager shutdown ...");
  delete m_pObjManager;
  m_pObjManager=0;

	if (GetPhysicalWorld())
	{
		CPhysCallbacks::Done();
	}

	assert(IsHeapValid());

	FreeRNTmpDataPool();
}

#ifdef WIN64
#pragma warning( push )									//AMD Port
#pragma warning( disable : 4311 )
#endif

//////////////////////////////////////////////////////////////////////
bool C3DEngine::SetupCamera(const CCamera & _newCam, const char * szCallerName) 
{
  if(IsCameraAnd3DEngineInvalid(_newCam, szCallerName))
    return false;

  CCamera newCam = _newCam;

	//newCam.SetFrustum(newCam.GetViewSurfaceX(),newCam.GetViewSurfaceZ(), DEG2RAD(45.0f), newCam.GetNearPlane(), newCam.GetFarPlane() );

  if(GetCVars()->e_CameraRotationSpeed)
  {
    Matrix34 mat = _newCam.GetMatrix();
    Matrix33 matRot; matRot.SetRotationZ(-GetCurTimeSec()*GetCVars()->e_CameraRotationSpeed);
    newCam.SetMatrix(mat*matRot);
  }

  if(!GetCVars()->e_CameraFreeze)
	  SetCamera(newCam);
  else
    DebugDraw_PushFrustrum( "MainCamera", GetCamera(), Col_Green, -999.f );

	m_bCameraUnderWater = IsUnderWater(newCam.GetPosition());

	GetRenderer()->SetCamera(newCam);

  return true;
}

#ifdef WIN64
#pragma warning( pop )									//AMD Port
#endif

IStatObj* C3DEngine::LoadStatObj( const char *szFileName,const char *szGeomName,IStatObj::SSubObject **ppSubObject, bool bUseStreaming,unsigned long nLoadingFlags )
{
  if(!szFileName || !szFileName[0])
  {
    m_pSystem->Warning( VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR,0,0,"I3DEngine::LoadStatObj: filename is not specified" );
    return 0;
  }

  if(!m_pObjManager)
    m_pObjManager = new CObjManager();

  return m_pObjManager->LoadStatObj(szFileName, szGeomName, ppSubObject, bUseStreaming,nLoadingFlags);
}

IStatObj * C3DEngine::FindStatObjectByFilename(const char * filename)
{
	if ( filename ==  NULL )
		return NULL;

	if ( filename[0] == 0 )
		return NULL;

	return m_pObjManager->FindStaticObjectByFilename(filename);
}

Vec3 C3DEngine::GetEntityRegisterPoint( IRenderNode* pEnt )
{
  AABB aabb = pEnt->GetBBox();

  Vec3 vPoint;
   
  if(pEnt->m_dwRndFlags & ERF_REGISTER_BY_POSITION)
  {
    vPoint = pEnt->GetPos();  
    vPoint.z += 0.25f;

		if(pEnt->GetRenderNodeType() != eERType_Light)
		{
			// check for valid position
			if(aabb.GetDistanceSqr(vPoint)>sqr(128.f))
				Warning("I3DEngine::RegisterEntity: invalid entity position: Name: %s, Class: %s, Pos=(%.1f,%.1f,%.1f), BoxMin=(%.1f,%.1f,%.1f), BoxMax=(%.1f,%.1f,%.1f)", 
				pEnt->GetName(), pEnt->GetEntityClassName(), 
				pEnt->GetPos().x, pEnt->GetPos().y, pEnt->GetPos().z,
				pEnt->GetBBox().min.x, pEnt->GetBBox().min.y, pEnt->GetBBox().min.z,
				pEnt->GetBBox().max.x, pEnt->GetBBox().max.y, pEnt->GetBBox().max.z
				);

			// clamp by bbox
			vPoint.CheckMin(aabb.max);
			vPoint.CheckMax(aabb.min+Vec3(0,0,.5f));
		}
  }
  else
    vPoint = aabb.GetCenter();  

  return vPoint;
}

void C3DEngine::RegisterEntity( IRenderNode* pEnt, int nSID )
{
  FUNCTION_PROFILER_3DENGINE;

  if(m_nRenderStackLevel>=0)
  {
    Warning("I3DEngine::RegisterEntity: RegisterEntity is called for %s during 3DEngine::Render() call - ignored", pEnt->GetName());
    assert(0);
    return;
  }

#ifdef _DEBUG // crash test basically
  const char * szClass = pEnt->GetEntityClassName();
  const char * szName = pEnt->GetName();
  if(!szName[0] && !szClass[0])
    Warning("I3DEngine::RegisterEntity: Entity undefined"); // do not register undefined objects
//  if(strstr(szName,"Dude"))
  //  int y=0;
#endif

  AABB aabb(pEnt->GetBBox());
  float fObjRadiusSqr = aabb.GetRadiusSqr();
	EERType eERType = pEnt->GetRenderNodeType();

#ifdef SUPP_HMAP_OCCL  
  if(pEnt->m_pRNTmpData)
    pEnt->m_pRNTmpData->userData.m_OcclState.vLastVisPoint.Set(0,0,0);
#endif
  if(m_pVoxTerrain && (eERType == eERType_Decal || eERType == eERType_Road))
    m_pVoxTerrain->RequestTextureUpdate(aabb);

  if(!(pEnt->m_dwRndFlags&ERF_RENDER_ALWAYS) && !(pEnt->m_dwRndFlags&ERF_CASTSHADOWMAPS))
    if(GetCVars()->e_ObjFastRegister && pEnt->m_pOcNode && ((COctreeNode*)pEnt->m_pOcNode)->IsRightNode(aabb, fObjRadiusSqr, pEnt->m_fWSMaxViewDist))
  { // same octree node
    Vec3 vEntCenter = GetEntityRegisterPoint( pEnt );

		IVisArea * pVisArea = pEnt->GetEntityVisArea();
    if(pVisArea && pVisArea->IsPointInsideVisArea(vEntCenter))
      return; // same visarea

    IVisArea * pVisAreaFromPos = (!m_pVisAreaManager || pEnt->GetRndFlags()&ERF_OUTDOORONLY) ? NULL : GetVisAreaManager()->GetVisAreaFromPos(vEntCenter);
    if(pVisAreaFromPos == pVisArea)
      return; // same visarea or same outdoor
  }

  if(pEnt->m_pOcNode)
  {
	  UnRegisterEntity( pEnt );
  }
  else if(GetCVars()->e_StreamCgf && eERType == eERType_Unknown)
  { //  Temporary solution: Force streaming priority update for objects that was not registered before 
    //  and was not visible before since usual prediction system was not able to detect them
    int nFrameId = GetFrameID();
    if(pEnt->GetDrawFrame(0) < nFrameId-16)
    {
      const Vec3& vCamPos = GetCamera().GetPosition();
      float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,aabb))*m_fZoomFactor;
      GetObjManager()->UpdateRenderNodeStreamingPrioriry(pEnt, fEntDistance);
      if(GetCVars()->e_StreamCgfDebug==2)
        PrintMessage("C3DEngine::RegisterEntity__GetObjManager()->UpdateRenderNodeStreamingPrioriry %s", pEnt->GetName());
    }
  }

  if(eERType == eERType_Vegetation)
  {
    CVegetation * pInst = (CVegetation*)pEnt;
    pInst->UpdateRndFlags();
  }

  pEnt->m_fWSMaxViewDist = pEnt->GetMaxViewDist();
  
	bool bLightIgnoresVisAreas = false; 

  if(eERType != eERType_Light)
  {
    if(fObjRadiusSqr > sqr(MAX_VALID_OBJECT_VOLUME) || !_finite(fObjRadiusSqr))
    {
      Warning("I3DEngine::RegisterEntity: Object has invalid bbox: name: %s, class name: %s, GetRadius() = %.2f",
        pEnt->GetName(), pEnt->GetEntityClassName(), fObjRadiusSqr);
      return; // skip invalid objects - usually only objects with invalid very big scale will reach this point
    }

    if(pEnt->m_dwRndFlags & ERF_RENDER_ALWAYS)
      if(m_lstAlwaysVisible.Find(pEnt)<0)
        m_lstAlwaysVisible.Add(pEnt);
  }
  else
  {
		CLightEntity * pLight = (CLightEntity *)pEnt;
		if( (pLight->m_light.m_Flags & (DLF_IGNORES_VISAREAS|DLF_DEFERRED_LIGHT|DLF_THIS_AREA_ONLY)) == (DLF_IGNORES_VISAREAS|DLF_DEFERRED_LIGHT))
		{
			bLightIgnoresVisAreas = true;
		}
  }

	//////////////////////////////////////////////////////////////////////////
	// Check for occlusion proxy.
	{
		if(CStatObj * pStatObj = (CStatObj *)pEnt->GetEntityStatObj(0))
		{
			if(pStatObj->m_bHaveOcclusionProxy)
			{
				pEnt->m_dwRndFlags |=	ERF_GOOD_OCCLUDER;
				pEnt->m_nInternalFlags |=	IRenderNode::HAS_OCCLUSION_PROXY;
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////

	if(pEnt->GetRndFlags()&ERF_OUTDOORONLY || bLightIgnoresVisAreas || !(m_pVisAreaManager && m_pVisAreaManager->SetEntityArea(pEnt, aabb, fObjRadiusSqr)))
	{
    if(nSID == -1)
    { 
      nSID = 0;

      // if segment is not set - find best one automatically
      Vec3 vCenter = aabb.GetCenter();
			const int iTreeCount = Get3DEngine()->m_pObjectsTree.Count();
      for(int n=0; n < iTreeCount; n++)
      {
        if(Get3DEngine()->m_pObjectsTree[n])
        {
          if(Overlap::Point_AABB2D(vCenter, m_pObjectsTree[n]->GetBBox()))
          {
            nSID = n;
            break;
          }
        }
      }
    }

		if(nSID>=0 && nSID<m_pObjectsTree.Count())
    {
      if(!m_pObjectsTree[nSID])
      {
        const float terrainSize = (float)GetTerrainSize();
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
        m_pObjectsTree[nSID] = new COctreeNode(nSID, AABB(Vec3(0,0,0),Vec3(terrainSize, terrainSize, terrainSize)), NULL);
      }

      m_pObjectsTree[nSID]->InsertObject(pEnt, aabb, fObjRadiusSqr, aabb.GetCenter());
    }
	}

  if(pEnt->GetIntegrationType()>=eIT_VoxelMesh && GetTerrain())
	{
		const AABB box(pEnt->GetBBox());
    GetTerrain()->ResetTerrainVertBuffers(&box,nSID);
	}
}

bool C3DEngine::UnRegisterEntity( IRenderNode* pEnt )
{
	FUNCTION_PROFILER_3DENGINE;

//  if(!(m_pObjectsTree[nSID] || m_pVisAreaManager))
  //  return false;

//	assert(m_nRenderStackLevel<0);
//	if(m_nRenderStackLevel>=0)
	//	Warning("C3DEngine::UnRegisterEntity: UnRegisterEntity is called for %s during 3DEngine::Render() call", pEnt->GetName());

#ifdef _DEBUG // crash test basically
	const char * szClass = pEnt->GetEntityClassName();
	const char * szName = pEnt->GetName();
	if(!szName[0] && !szClass[0])
		Warning("C3DEngine::RegisterEntity: Entity undefined");
#endif

  bool bFound = false;
	pEnt->PhysicalizeFoliage(false);

	if(pEnt->m_pOcNode)
		bFound = ((COctreeNode*)pEnt->m_pOcNode)->DeleteObject(pEnt);

  if(pEnt->m_dwRndFlags & ERF_RENDER_ALWAYS)
    m_lstAlwaysVisible.Delete(pEnt);

  if(m_pVoxTerrain && (pEnt->GetRenderNodeType() == eERType_Decal || pEnt->GetRenderNodeType() == eERType_Road))
    m_pVoxTerrain->RequestTextureUpdate(pEnt->GetBBox());

  if(pEnt->GetIntegrationType()>=eIT_VoxelMesh && GetTerrain())
	{
		const AABB box(pEnt->GetBBox());
    GetTerrain()->ResetTerrainVertBuffers(&box,pEnt->m_nSID);
	}

  return bFound;
}

bool C3DEngine::CreateDecalOnStatObj( const CryEngineDecalInfo& decal, CDecal* pCallerManagedDecal )
{
  FUNCTION_PROFILER( gEnv->pSystem, PROFILE_3DENGINE );

  if( !GetCVars()->e_Decals && !pCallerManagedDecal )
		return false;

	CryEngineDecalInfo decal_adjusted = decal;
	decal_adjusted.bAdjustPos = true;
	return m_pDecalManager->Spawn( decal_adjusted, GetMaxViewDistance(), GetTerrain(), pCallerManagedDecal );
}

void C3DEngine::SelectEntity(IRenderNode *pEntity)
{
  static IRenderNode *pSelectedNode; 
  static float fLastTime;
  if (pEntity && GetCVars()->e_Decals == 2)
  {
    float fCurTime = gEnv->pTimer->GetAsyncCurTime();
    if (fCurTime-fLastTime < 1.0f)
      return;
    fLastTime = fCurTime;
    if (pSelectedNode)
      pSelectedNode->SetRndFlags(ERF_SELECTED, false);
    pEntity->SetRndFlags(ERF_SELECTED, true);
    pSelectedNode = pEntity;
  }
}

void C3DEngine::CreateDecal(const struct CryEngineDecalInfo & decal)
{
  if(!GetCVars()->e_DecalsAllowGameDecals)
    return;

	if(GetCVars()->e_Decals == 2)
  {
    IRenderNode * pRN = decal.ownerInfo.pRenderNode;
    PrintMessage("Debug: C3DEngine::CreateDecal: Pos=(%.1f,%.1f,%.1f) Size=%.2f DecalMaterial=%s HitObjectName=%s(%s)", 
      decal.vPos.x, decal.vPos.y, decal.vPos.z, decal.fSize, decal.szMaterialName,
      pRN ? pRN->GetName() : "NULL", pRN ? pRN->GetEntityClassName() : "NULL");
  }

  if(m_pVoxTerrain)
  {
    CDecalRenderNode * pDecal = (CDecalRenderNode *)CreateRenderNode(eERType_Decal);

    IMaterial * pMat = GetMaterialManager()->LoadMaterial(decal.szMaterialName);
    pDecal->SetMaterial(pMat);

    Matrix33 matRot; matRot.SetIdentity();

    Matrix33 matRot0 = Matrix33::CreateRotationVDir( (decal).vNormal, (decal).fAngle );

    matRot = Matrix33::CreateRotationX(-gf_PI/2);
    Matrix33 matRotZ = Matrix33::CreateRotationZ(gf_PI/2);

    matRot = matRot0*matRot;
    matRot = matRot * matRotZ;

    Matrix34 wtm; wtm.SetIdentity();
    wtm.SetScale(Vec3(decal.fSize,decal.fSize,decal.fSize), decal.vPos);
    wtm = wtm * matRot;

    SDecalProperties decalProperties;
    decalProperties.m_projectionType = SDecalProperties::ePlanar;

    decalProperties.m_pos = wtm.TransformPoint( Vec3( 0, 0, 0 ) );
    decalProperties.m_normal = wtm.TransformVector( Vec3( 0, 0, 1 ) );
    decalProperties.m_pMaterialName = pMat->GetName();
    decalProperties.m_radius = decalProperties.m_normal.GetLength();

    Matrix33 rotation( wtm ); 
    rotation.SetRow( 0, rotation.GetRow( 0 ).GetNormalized() );
    rotation.SetRow( 1, rotation.GetRow( 1 ).GetNormalized() );
    rotation.SetRow( 2, rotation.GetRow( 2 ).GetNormalized() );

    decalProperties.m_explicitRightUpFront = rotation;
    decalProperties.m_sortPrio = 32;

    pDecal->SetDecalProperties( decalProperties );

    pDecal->SetMatrix( wtm );

    pDecal->SetDecalProperties( decalProperties );

    RegisterEntity(pDecal);

    return;
  }

	/*if(decal.ownerInfo.pRenderNode && !decal.ownerInfo.pRenderNode->m_pOcNode)
  {
    if(GetCVars()->e_Decals == 2)
    {
      IRenderNode * pRN = decal.ownerInfo.pRenderNode;
      PrintMessage("Debug: C3DEngine::CreateDecal: Decal creation was skipped because owner render node is not registered for rendering\nPos=(%.1f,%.1f,%.1f) Size=%.2f DecalMaterial=%s HitObjectName=%s(%s)", 
        decal.vPos.x, decal.vPos.y, decal.vPos.z, decal.fSize, decal.szMaterialName,
        pRN ? pRN->GetName() : "NULL", pRN ? pRN->GetEntityClassName() : "NULL");
    }

    return;
  }*/

  // decal on CHR, ownerInfo.nPartID is bone id
  if(decal.ownerInfo.pRenderNode)
    if(ICharacterInstance * pChar = decal.ownerInfo.pRenderNode->GetEntityCharacter(0))
  {
    if(pChar->GetOjectType() == CHR)
    {
      CryEngineDecalInfo DecalLCS = decal;
      pChar->CreateDecal(DecalLCS);
    }
  }

  static uint32 nGroupId = 0; nGroupId++;

  if(decal.ownerInfo.pRenderNode && decal.fSize>.5f)
  {
    PodArray<SRNInfo> lstEntities;
    Vec3 vRadius(decal.fSize,decal.fSize,decal.fSize);
    const AABB cExplosionBox(decal.vPos - vRadius, decal.vPos + vRadius);

    if(CVisArea * pArea = (CVisArea *)decal.ownerInfo.pRenderNode->GetEntityVisArea())
    {
      if(pArea->m_pObjectsTree)
        pArea->m_pObjectsTree->MoveObjectsIntoList(&lstEntities, &cExplosionBox, false, true, true, true);
    }
    else
      Get3DEngine()->MoveObjectsIntoListGlobal(&lstEntities, &cExplosionBox, false, true, true, true);

    for(int i=0; i<lstEntities.Count(); i++)
    {
      // decals on statobj's of render node
      CryEngineDecalInfo decalOnRenderNode = decal;
      decalOnRenderNode.ownerInfo.pRenderNode = lstEntities[i].pNode;
      decalOnRenderNode.nGroupId = nGroupId;

//      if(decalOnRenderNode.ownerInfo.pRenderNode->GetRenderNodeType() != decal.ownerInfo.pRenderNode->GetRenderNodeType())
  //      continue;

      if(decalOnRenderNode.ownerInfo.pRenderNode->GetRndFlags()&ERF_HIDDEN)
        continue;

      m_pDecalManager->SpawnHierarhical( decalOnRenderNode, GetMaxViewDistance(), GetTerrain(), NULL );
    }
  }
  else
  {
    CryEngineDecalInfo decalStatic = decal;
    decalStatic.nGroupId = nGroupId;
    m_pDecalManager->SpawnHierarhical( decalStatic, GetMaxViewDistance(), GetTerrain(), NULL );
  }
}

/*
float C3DEngine::GetDayTime(float fPrevDayTime) 
{ 
  if(fPrevDayTime)
  {
    float fDayTimeDiff = (GetCVars()->e_day_time - fPrevDayTime);
    while(fDayTimeDiff>12)
      fDayTimeDiff-=12;
    while(fDayTimeDiff<-12)
      fDayTimeDiff+=12;  
    fDayTimeDiff = (float)fabs(fDayTimeDiff);
    return fDayTimeDiff;
  }

  return GetCVars()->e_day_time; 
} */

namespace
{
	inline const float GetSunSkyRel(const Vec3& crSkyColor, const Vec3& crSunColor)
	{
		const float cSunMax = max(0.1f, max(crSunColor.x, max(crSunColor.y, crSunColor.z)));
		const float cSkyMax = max(0.1f, max(crSkyColor.x, max(crSkyColor.y, crSkyColor.z)));
		return cSunMax / (cSkyMax + cSunMax);
	}
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetSkyColor(Vec3 vColor)
{
	if (m_pObjManager)
	{
		m_pObjManager->m_vSkyColor = vColor;
		m_pObjManager->m_fSunSkyRel = GetSunSkyRel(m_pObjManager->m_vSkyColor, m_pObjManager->m_vSunColor);
	}
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetSunColor(Vec3 vColor)
{
	if (m_pObjManager)
	{
		m_pObjManager->m_vSunColor = vColor;
		m_pObjManager->m_fSunSkyRel = GetSunSkyRel(m_pObjManager->m_vSkyColor, m_pObjManager->m_vSunColor);
	}
}

void C3DEngine::SetSkyBrightness(float fMul)
{
	if (m_pObjManager)
		m_pObjManager->m_fSkyBrightMul = fMul;
}

void C3DEngine::SetSSAOAmount(float fMul)
{
  if (m_pObjManager)
    m_pObjManager->m_fSSAOAmount = fMul;
}

void C3DEngine::SetGIAmount(float fMul)
{
	if (m_pObjManager)
		m_pObjManager->m_fGIAmount = fMul;
}

float C3DEngine::GetTerrainElevation(float x, float y, bool bIncludeOutdoorVoxles)
{
  if(GetCVars()->e_VoxTerHeightmapEditing && m_pVoxTerrain && GetCVars()->e_VoxTer && C3DEngine::m_pGetLayerIdAtCallback)
  {
    float fTerrainZ = C3DEngine::m_pGetLayerIdAtCallback->GetElevationAtPosition(y/GetTerrain()->GetHeightMapUnitSize(),x/GetTerrain()->GetHeightMapUnitSize());
    return fTerrainZ;
  }

  float fZ = 0;

  if(m_pTerrain)
    fZ = m_pTerrain->GetZApr(x, y, bIncludeOutdoorVoxles, GetDefSID());

  Vec3 vNorm;
  if(m_pVoxTerrain && GetCVars()->e_VoxTer && m_bEditor)
  {
    if(bIncludeOutdoorVoxles)
      fZ = max(0.f, m_pVoxTerrain->GetElevation3D(Vec3(x,y,0), vNorm));
    else
      fZ = max(fZ, m_pVoxTerrain->GetElevation3D(Vec3(x,y,0), vNorm));
  }

  return fZ;
}

float C3DEngine::GetTerrainElevation3D(Vec3 vPos)
{
  if(GetCVars()->e_VoxTerHeightmapEditing && m_pVoxTerrain && GetCVars()->e_VoxTer && C3DEngine::m_pGetLayerIdAtCallback)
  {
    float fTerrainZ = C3DEngine::m_pGetLayerIdAtCallback->GetElevationAtPosition(vPos.y/GetTerrain()->GetHeightMapUnitSize(),vPos.x/GetTerrain()->GetHeightMapUnitSize());
    return fTerrainZ;
  }

  float fZ = 0;

  if(m_pTerrain)
    fZ = m_pTerrain->GetZApr(vPos.x, vPos.y, false, GetDefSID());

  Vec3 vNorm;
  if(m_pVoxTerrain && GetCVars()->e_VoxTer)
    fZ = max(fZ, m_pVoxTerrain->GetElevation3D(vPos, vNorm));

  return fZ;
}

float C3DEngine::GetTerrainZ(int x, int y)
{
  Vec3 vNorm;
  if(m_pVoxTerrain)
    return m_pVoxTerrain->GetElevation3D(Vec3((float)x,(float)y,0.f), vNorm);

  if(x<0 || y<0 || x>=CTerrain::GetTerrainSize() || y>=CTerrain::GetTerrainSize())
		return TERRAIN_BOTTOM_LEVEL;
  return m_pTerrain ? m_pTerrain->GetZ(x, y, GetDefSID()) : 0;
}

bool C3DEngine::GetTerrainHole(int x, int y)
{
  return m_pTerrain ? m_pTerrain->GetHole(x, y, GetDefSID()) : false;
}

int C3DEngine::GetHeightMapUnitSize()
{
  return CTerrain::GetHeightMapUnitSize();
}

int C3DEngine::GetTerrainSize()
{
  return CTerrain::GetTerrainSize();
}

void C3DEngine::RemoveAllStaticObjects(int nSID)
{
  if(m_pTerrain)
	  m_pTerrain->RemoveAllStaticObjects(nSID);
}

void C3DEngine::SetTerrainSurfaceType(int x, int y, int nType)
{
	assert(0);
//  if(m_pTerrain)
  //  m_pTerrain->SetSurfaceType(x,y,nType);
}

void C3DEngine::SetTerrainSectorTexture( const int nTexSectorX, const int nTexSectorY, unsigned int textureId )
{
	if(m_pTerrain)
		m_pTerrain->SetTerrainSectorTexture(nTexSectorX, nTexSectorY, textureId, GetDefSID());
}

void C3DEngine::OnExplosion(Vec3 vPos, float fRadius, bool bDeformTerrain)
{
  if(GetCVars()->e_Decals == 2)
    PrintMessage("Debug: C3DEngine::OnExplosion: Pos=(%.1f,%.1f,%.1f) fRadius=%.2f", vPos.x, vPos.y, vPos.z, fRadius);

  if(vPos.x<0 || vPos.x>=CTerrain::GetTerrainSize() || vPos.y<0 || vPos.y>=CTerrain::GetTerrainSize() || fRadius<=0)
    return; // out of terrain

  if(m_pVoxTerrain && bDeformTerrain && GetCVars()->e_TerrainDeformations)
  {
    m_pVoxTerrain->DoVoxelShape(vPos, fRadius/2, -1, Vec3(255,255,255), eveoSubstract, evbsSphere, NULL, 0.5f);
    return;
  }

	// do not create decals near the terrain holes
  if(!m_pVoxTerrain)
  {
    for(int x=int(vPos.x-fRadius); x<=int(vPos.x+fRadius)+1; x+=CTerrain::GetHeightMapUnitSize())
      for(int y=int(vPos.y-fRadius); y<=int(vPos.y+fRadius)+1; y+=CTerrain::GetHeightMapUnitSize())
        if(m_pTerrain->GetHole(x,y, GetDefSID()))
          return;
  }

	// try to remove objects around
	bool bGroundDeformationAllowed = m_pTerrain->RemoveObjectsInArea(vPos,fRadius) && bDeformTerrain && GetCVars()->e_TerrainDeformations;

	// reduce ground decals size depending on distance to the ground
	float fExploHeight = vPos.z - GetTerrain()->GetZApr(vPos.x,vPos.y, false, GetDefSID());

	if(bGroundDeformationAllowed && (fExploHeight > -0.1f) && fExploHeight<fRadius && fRadius>0.125f)
  {
    m_pTerrain->MakeCrater(vPos,fRadius);

    // update roads
    {
      PodArray<IRenderNode*> lstRoads;
			AABB bbox(vPos-Vec3(fRadius,fRadius,fRadius), vPos+Vec3(fRadius,fRadius,fRadius));
      Get3DEngine()->GetObjectsByTypeGlobal(lstRoads, eERType_Road, &bbox);
      for(int i=0; i<lstRoads.Count(); i++)
      {
        CRoadRenderNode * pRoad = (CRoadRenderNode *)lstRoads[i];
        pRoad->OnTerrainChanged();
      }
    }
  }

	// delete decals what can not be correctly updated
	Vec3 vRadius(fRadius,fRadius,fRadius);
  AABB areaBox(vPos-vRadius, vPos+vRadius);
	Get3DEngine()->DeleteDecalsInRange( &areaBox, NULL );
}

float C3DEngine::GetMaxViewDistance( bool bScaled )
{
  // spec lerp factor
  float lerpSpec = clamp(GetCVars()->e_MaxViewDistSpecLerp, 0.0f, 1.0f);
	
  // camera height lerp factor
  float lerpHeight = clamp(max( 0.f, GetSystem()->GetViewCamera().GetPosition().z - GetWaterLevel()) / max( 1.f, GetCVars()->e_MaxViewDistFullDistCamHeight ), 0.0f, 1.0f);
  
  // lerp between specs
  float fMaxViewDist = m_fMaxViewDistLowSpec * (1.f - lerpSpec) + m_fMaxViewDistHighSpec * lerpSpec;

  // lerp between prev result and high spec
  fMaxViewDist = fMaxViewDist * (1.f - lerpHeight) + m_fMaxViewDistHighSpec * lerpHeight;

  if(bScaled)
    fMaxViewDist *= m_fMaxViewDistScale;

  // for debugging
	const float fMaxViewDistCVar = GetCVars()->e_MaxViewDistance;
	fMaxViewDist = (float)__fsel(fMaxViewDistCVar, fMaxViewDistCVar, fMaxViewDist);
  
	fMaxViewDist = (float)__fsel(fabsf(fMaxViewDist) - 0.100001f, fMaxViewDist, 0.100001f);

  return fMaxViewDist;
}

void C3DEngine::SetFogColor(const Vec3& vFogColor)
{
  m_vFogColor    = vFogColor;
	GetRenderer()->SetClearColor(m_vFogColor);
}

Vec3 C3DEngine::GetFogColor( )
{
  return m_vFogColor;
}

void C3DEngine::GetVolumetricFogSettings( float& globalDensity, float& atmosphereHeight, float& artistTweakDensityOffset, float& globalDensityMultiplierLDR )
{
	globalDensity = m_volFogGlobalDensity;
	globalDensityMultiplierLDR = m_volFogGlobalDensityMultiplierLDR;
	atmosphereHeight = m_volFogAtmosphereHeight;
	artistTweakDensityOffset = m_volFogArtistTweakDensityOffset;
}

void C3DEngine::SetVolumetricFogSettings( float globalDensity, float atmosphereHeight, float artistTweakDensityOffset )
{
	m_volFogGlobalDensity = ( globalDensity < 0.0f ) ? 0.0f : globalDensity;
	m_volFogAtmosphereHeight = ( atmosphereHeight < 1.0f ) ? 1.0f : atmosphereHeight;
	m_volFogArtistTweakDensityOffset = ( artistTweakDensityOffset < 0.0f ) ? 0.0f : artistTweakDensityOffset;
}

void C3DEngine::GetVolumetricFogModifiers(float& globalDensityModifier, float& atmosphereHeightModifier)
{
	globalDensityModifier = m_volFogGlobalDensityMod;
	atmosphereHeightModifier = m_volFogAtmosphereHeightMod;
}	

void C3DEngine::SetVolumetricFogModifiers(float globalDensityModifier, float atmosphereHeightModifier, bool reset)
{
	if (reset)
	{
		m_volFogGlobalDensityMod = globalDensityModifier;
		m_volFogAtmosphereHeightMod = atmosphereHeightModifier;
	}
	else
	{
		m_volFogGlobalDensityMod += globalDensityModifier;
		m_volFogAtmosphereHeightMod += atmosphereHeightModifier;
	}
}

void C3DEngine::GetSkyLightParameters( Vec3& sunIntensity, float& Km, float& Kr, float& g, Vec3& rgbWaveLengths )
{
	CSkyLightManager::SSkyDomeCondition skyCond;
	m_pSkyLightManager->GetCurSkyDomeCondition( skyCond );

	g = skyCond.m_g;
	Km = skyCond.m_Km;
	Kr = skyCond.m_Kr;
	sunIntensity = skyCond.m_sunIntensity;
	rgbWaveLengths = skyCond.m_rgbWaveLengths;
}

void C3DEngine::SetSkyLightParameters( const Vec3& sunIntensity, float Km, float Kr, float g, const Vec3& rgbWaveLengths, bool forceImmediateUpdate )
{
	CSkyLightManager::SSkyDomeCondition skyCond;

	skyCond.m_g = g;
	skyCond.m_Km = Km;
	skyCond.m_Kr = Kr;
	skyCond.m_sunIntensity = sunIntensity;
	skyCond.m_rgbWaveLengths = rgbWaveLengths;
	skyCond.m_sunDirection = m_vSunDir;
	
	m_pSkyLightManager->SetSkyDomeCondition( skyCond );
	if (forceImmediateUpdate && IsHDRSkyMaterial(GetSkyMaterial()))
		m_pSkyLightManager->FullUpdate();
}

float C3DEngine::GetHDRDynamicMultiplier() const
{
	return m_fHDRDynamicMultiplier;
}

void C3DEngine::SetHDRDynamicMultiplier( const float value )
{
	m_fHDRDynamicMultiplier = clamp_tpl(value,0.01f,100.0f);
}

void C3DEngine::TraceFogVolumes( const Vec3& worldPos, ColorF& fogVolumeContrib )
{
	CFogVolumeRenderNode::TraceFogVolumes( worldPos, fogVolumeContrib );
}

void C3DEngine::GetCloudShadingMultiplier( float& sunLightMultiplier, float& skyLightMultiplier )
{
	sunLightMultiplier = m_fCloudShadingSunLightMultiplier;
	skyLightMultiplier = m_fCloudShadingSkyLightMultiplier;
}

void C3DEngine::SetCloudShadingMultiplier( float sunLightMultiplier, float skyLightMultiplier )
{
	m_fCloudShadingSunLightMultiplier = ( sunLightMultiplier >= 0 ) ? sunLightMultiplier : 0;
	m_fCloudShadingSkyLightMultiplier = ( skyLightMultiplier >= 0 ) ? skyLightMultiplier : 0;
}

void C3DEngine::SetPhysMaterialEnumerator(IPhysMaterialEnumerator * pPhysMaterialEnumerator)
{
  m_pPhysMaterialEnumerator = pPhysMaterialEnumerator;
}

IPhysMaterialEnumerator * C3DEngine::GetPhysMaterialEnumerator()
{
  return m_pPhysMaterialEnumerator;
}

void C3DEngine::ApplyForceToEnvironment(Vec3 vPos, float fRadius, float fAmountOfForce)
{
  if(m_pTerrain)
    m_pTerrain->ApplyForceToEnvironment(vPos, fRadius, fAmountOfForce);
}

float C3DEngine::GetDistanceToSectorWithWater()
{
	CTerrainNode * pSectorInfo = NULL;
	if(m_pTerrain)
		pSectorInfo = m_pTerrain->GetSecInfo(GetCamera().GetPosition());

  return (pSectorInfo && (m_pTerrain && m_pTerrain->GetDistanceToSectorWithWater() > 0.1f))
    ? m_pTerrain->GetDistanceToSectorWithWater() : max(GetCamera().GetPosition().z - GetWaterLevel(),0.1f);
}


Vec3 C3DEngine::GetSkyColor() const
{
  return m_pObjManager ? m_pObjManager->m_vSkyColor : Vec3(0,0,0);
}

Vec3 C3DEngine::GetSunColor() const
{
  return m_pObjManager ? m_pObjManager->m_vSunColor : Vec3(0,0,0);
}

float C3DEngine::GetSkyBrightness() const
{
	return m_pObjManager ? m_pObjManager->m_fSkyBrightMul : 1.0f;
}

float C3DEngine::GetSSAOAmount() const
{
  return m_pObjManager ? m_pObjManager->m_fSSAOAmount: 1.0f;
}

float C3DEngine::GetGIAmount() const
{
	return m_pObjManager ? m_pObjManager->m_fGIAmount: 1.0f;
}

float C3DEngine::GetSunRel() const
{
	return m_pObjManager->m_fSunSkyRel;
}

float C3DEngine::GetTerrainTextureMultiplier(int nSID) const
{
  if(!m_pTerrain)
    return 1;

	return m_pTerrain->GetTerrainTextureMultiplier(nSID);
}

void C3DEngine::SetSunDir( const Vec3& newSunDir )
{ 
  Vec3 vSunDirNormalized = newSunDir.normalized();
  m_vSunDirRealtime = vSunDirNormalized;
  if( vSunDirNormalized.Dot(m_vSunDirNormalized) < GetCVars()->e_SunAngleSnapDot || 
    GetCurTimeSec() - m_fSunDirUpdateTime > GetCVars()->e_SunAngleSnapSec )
  {
    m_vSunDirNormalized = vSunDirNormalized;
    m_vSunDir = m_vSunDirNormalized * DISTANCE_TO_THE_SUN; 

    m_fSunDirUpdateTime = GetCurTimeSec();
  }
}

Vec3 C3DEngine::GetSunDir()  const
{ 
  return m_vSunDir; 
}

Vec3 C3DEngine::GetSunDirNormalized() const
{ 
	return m_vSunDirNormalized; 
}

Vec3 C3DEngine::GetRealtimeSunDirNormalized() const
{ 
  return m_vSunDirRealtime; 
}

Vec3 C3DEngine::GetAmbientColorFromPosition(const Vec3 & vPos, float fRadius) 
{
  Vec3 ret(0,0,0); 

  if(m_pObjManager)
	{
		if(CVisArea * pVisArea = (CVisArea *)m_pVisAreaManager->GetVisAreaFromPos(vPos))
      ret = pVisArea->GetFinalAmbientColor();
		else
			ret = Get3DEngine()->GetSkyColor();

		assert(ret.x>=0 && ret.y>=0 && ret.z>=0);
	}

	return ret;
}

void C3DEngine::FreeRenderNodeState(IRenderNode * pEnt)
{
//  PrintMessage("vlad: C3DEngine::FreeRenderNodeState: %d, %s", (int)(IRenderNode*)pEnt, pEnt->GetName());

	/*
	LightMapInfo * pLightMapInfo = (LightMapInfo *)pEnt->GetLightMapInfo();
	if(pLightMapInfo && pLightMapInfo->pLMTCBuffer)
		GetRenderer()->ReleaseBuffer(pLightMapInfo->pLMTCBuffer);
	*/
	// TODO: Delete data here, or at a more appropiate place 
	// (currently done in CBrush::~CBrush(), hope that is 'a more appropiate place')

  m_lstAlwaysVisible.Delete(pEnt);

  if(m_pDecalManager && (pEnt->m_nInternalFlags & IRenderNode::DECAL_OWNER))
    m_pDecalManager->OnEntityDeleted(pEnt);

  if(pEnt->GetRenderNodeType() == eERType_Light)
    GetRenderer()->OnEntityDeleted(pEnt);

  // Procedural vegetation in pure game seems the only case where we can rely on ERF_CASTSHADOWMAPS flag
  // and it is the only time critical one because of high amount of such objects
  // So only those objects are checked for ERF_CASTSHADOWMAPS flag before searching in casters list
  // TODO: use IRenderNode::WAS_CASTER flag when available
  if((pEnt->GetRndFlags() & ERF_CASTSHADOWMAPS) || !(pEnt->GetRndFlags() & ERF_PROCEDURAL))
  { // make sure pointer to object will not be used somewhere in the renderer
		Get3DEngine()->OnCasterDeleted(pEnt);
  }

  UnRegisterEntity(pEnt);

  if(pEnt->m_pRNTmpData)
  {
    Get3DEngine()->FreeRNTmpData(&pEnt->m_pRNTmpData);
    assert(!pEnt->m_pRNTmpData);
  }

//#endif
	/*
  if(pEnt->m_pShadowMapInfo)
	{
		pEnt->m_pShadowMapInfo->Release(pEnt->GetRenderNodeType(), GetRenderer());
		pEnt->m_pShadowMapInfo = NULL;
	}
*/
  // check that was really unregistered
/*  if(pEnt->m_pSector)
    for(int t=0; t<2; t++)
	{
	  for(int e=0; e<pEnt->m_pSector->m_lstEntities[t].Count(); e++)
		{
			if(pEnt->m_pSector->m_lstEntities[t][e]->GetEntityRS() == pEntRendState)
			{
				assert(0); // UnRegisterInAllSectors do this already
				pEnt->m_pSector->m_lstEntities[t].Delete(e);
			}
		}

		pEnt->m_pSector->m_lstEntities[t].Delete(pEnt);
	}										 

  pEnt->m_pSector = 0;
  */

	// delete occlusion client
/*	for(int i=0; i<2; i++)
	if(m_pOcclState && m_pOcclState->arrREOcclusionQuery[i])
	{
		m_pOcclState->arrREOcclusionQuery[i]->Release();
		m_pOcclState->arrREOcclusionQuery[i] = 0;
	}*/

//  delete m_pOcclState;
  //m_pOcclState = 0;

  //todo: is it needed?
//	pEnt->GetEntityVisArea() = 0;
}

const char * C3DEngine::GetLevelFilePath(const char * szFileName)
{
  strcpy(m_sGetLevelFilePathTmpBuff, m_szLevelFolder);
	if (*szFileName && (*szFileName == '/' || *szFileName == '\\'))
		strcat(m_sGetLevelFilePathTmpBuff, szFileName+1);
	else
		strcat(m_sGetLevelFilePathTmpBuff, szFileName);
	return m_sGetLevelFilePathTmpBuff;
}
/*
uint16 * C3DEngine::GetUnderWaterSmoothHMap(int & nDimensions)
{
  return m_pTerrain ? m_pTerrain->GetUnderWaterSmoothHMap(nDimensions) : 0;
}

void C3DEngine::MakeUnderWaterSmoothHMap(int nWaterUnitSize)
{
  if(m_pTerrain)
    m_pTerrain->MakeUnderWaterSmoothHMap(nWaterUnitSize);
}
*/
void C3DEngine::SetTerrainBurnedOut(int x, int y, bool bBurnedOut)
{
	assert(!"not supported");
/*  if(m_pTerrain)
  {
    m_pTerrain->SetBurnedOut(x, y, bBurnedOut);

    // update vert buffers
    CTerrainNode * pInfo = m_pTerrain->GetSecInfo(x,y);
    if(pInfo)
      pInfo->ReleaseHeightMapGeometry();
  }*/
}

bool C3DEngine::IsTerrainBurnedOut(int x, int y)
{
	assert(!"not supported");
  return 0;//m_pTerrain ? m_pTerrain->IsBurnedOut(x, y) : 0;
}

int C3DEngine::GetTerrainSectorSize()
{
  return m_pTerrain ? m_pTerrain->GetSectorSize() : 0;
}

void C3DEngine::ActivatePortal(const Vec3 &vPos, bool bActivate, const char * szEntityName)
{
	if (m_pVisAreaManager)
		m_pVisAreaManager->ActivatePortal(vPos, bActivate, szEntityName);
}

bool C3DEngine::SetStatInstGroup(int nGroupId, const IStatInstGroup & siGroup, int nSID)
{
  m_fRefreshSceneDataCVarsSumm = -100;

	m_pObjManager->m_lstStaticTypes.resize(max(nGroupId+1, (int)m_pObjManager->m_lstStaticTypes.size()));

	if(nGroupId<0 || nGroupId>=(int)m_pObjManager->m_lstStaticTypes.size())
		return false;

	StatInstGroup & rGroup = m_pObjManager->m_lstStaticTypes[nGroupId];

	rGroup.pStatObj = siGroup.pStatObj;

	if(rGroup.pStatObj)
		strncpy(rGroup.szFileName, siGroup.pStatObj->GetFilePath(), sizeof(rGroup.szFileName));
  else
    rGroup.szFileName[0] = 0;

  rGroup.bHideability	= siGroup.bHideability;
  rGroup.bHideabilitySecondary	= siGroup.bHideabilitySecondary;
  rGroup.nPlayerHideable = siGroup.nPlayerHideable;
  rGroup.bPickable	  = siGroup.bPickable;
	rGroup.fBending			= siGroup.fBending;
	rGroup.bCastShadow   = siGroup.bCastShadow;
	rGroup.bRecvShadow   = siGroup.bRecvShadow;
	rGroup.bPrecShadow   = siGroup.bPrecShadow;

	rGroup.bUseAlphaBlending						= siGroup.bUseAlphaBlending;
	rGroup.fSpriteDistRatio						= siGroup.fSpriteDistRatio;
  rGroup.fLodDistRatio						= siGroup.fLodDistRatio;
	rGroup.fShadowDistRatio						= siGroup.fShadowDistRatio;
	rGroup.fMaxViewDistRatio						= siGroup.fMaxViewDistRatio;

	rGroup.fBrightness									= siGroup.fBrightness;
  
	rGroup.pMaterial										= siGroup.pMaterial;	
//	if(siGroup.pMaterial) // make sure mat id is set
	//	if(int nMaterialId = GetObjManager()->GetObjectMaterialId(rGroup.pMaterial))
		//	if(nMaterialId<0)
			//	rGroup.pMaterial = NULL;

	rGroup.bUseSprites						      = siGroup.bUseSprites;

	rGroup.fDensity										= siGroup.fDensity;
	rGroup.fElevationMax								= siGroup.fElevationMax;
	rGroup.fElevationMin								= siGroup.fElevationMin;
	rGroup.fSize												= siGroup.fSize;
	rGroup.fSizeVar										= siGroup.fSizeVar;
	rGroup.fSlopeMax										= siGroup.fSlopeMax;
	rGroup.fSlopeMin										= siGroup.fSlopeMin;

	rGroup.bRandomRotation             = siGroup.bRandomRotation;
  rGroup.nMaterialLayers = siGroup.nMaterialLayers;

	rGroup.bUseTerrainColor             = siGroup.bUseTerrainColor;
	rGroup.bAlignToTerrain             = siGroup.bAlignToTerrain;
  rGroup.bAffectedByVoxels             = siGroup.bAffectedByVoxels;
	rGroup.minConfigSpec = siGroup.minConfigSpec;

	rGroup.Update(GetCVars(), Get3DEngine()->GetGeomDetailScreenRes());

	if (GetTerrain())
		GetTerrain()->MarkAllSectorsAsUncompiled(nSID);

  MarkRNTmpDataPoolForReset();

	return true;
}

bool C3DEngine::GetStatInstGroup(int nGroupId, IStatInstGroup & siGroup, int nSID)
{
	if(nGroupId<0 || nGroupId>=(int)m_pObjManager->m_lstStaticTypes.size())
		return false;

	StatInstGroup & rGroup = m_pObjManager->m_lstStaticTypes[nGroupId];

	siGroup.pStatObj			= rGroup.pStatObj;
	if(siGroup.pStatObj)
		strncpy(siGroup.szFileName, rGroup.pStatObj->GetFilePath(), sizeof(siGroup.szFileName));

	siGroup.bHideability	= rGroup.bHideability;
	siGroup.bHideabilitySecondary	= rGroup.bHideabilitySecondary;
	siGroup.nPlayerHideable = rGroup.nPlayerHideable;
  siGroup.bPickable	    = rGroup.bPickable;
	siGroup.fBending			= rGroup.fBending;
	siGroup.bCastShadow   = rGroup.bCastShadow;
	siGroup.bRecvShadow   = rGroup.bRecvShadow;
	siGroup.bPrecShadow   = rGroup.bPrecShadow;

	siGroup.bUseAlphaBlending						= rGroup.bUseAlphaBlending;
	siGroup.fSpriteDistRatio						= rGroup.fSpriteDistRatio;
  siGroup.fLodDistRatio               = rGroup.fLodDistRatio;
	siGroup.fShadowDistRatio						= rGroup.fShadowDistRatio;
	siGroup.fMaxViewDistRatio						= rGroup.fMaxViewDistRatio;

	siGroup.fBrightness									= rGroup.fBrightness;
	siGroup.pMaterial										= rGroup.pMaterial;
  siGroup.bUseSprites                 = rGroup.bUseSprites;

	siGroup.fDensity										= rGroup.fDensity;
	siGroup.fElevationMax								= rGroup.fElevationMax;
	siGroup.fElevationMin								= rGroup.fElevationMin;
	siGroup.fSize												= rGroup.fSize;
	siGroup.fSizeVar										= rGroup.fSizeVar;
	siGroup.fSlopeMax										= rGroup.fSlopeMax;
	siGroup.fSlopeMin										= rGroup.fSlopeMin;

	return true;
}

void C3DEngine::UpdateStatInstGroups()
{
  if(!m_pObjManager)
    return;

  for(uint32 nGroupId=0; nGroupId<m_pObjManager->m_lstStaticTypes.size(); nGroupId++)
  {
    StatInstGroup & rGroup = m_pObjManager->m_lstStaticTypes[nGroupId];
    rGroup.Update(GetCVars(), Get3DEngine()->GetGeomDetailScreenRes());
  }
}

void C3DEngine::GetMemoryUsage(class ICrySizer * pSizer)
{
	if (!pSizer->Add(*this))
		return; // we've already added this object

#ifndef _LIB // Only when compiling as dynamic library
	{
		//SIZER_COMPONENT_NAME(pSizer,"Strings");
		//pSizer->AddObject( (this+1),string::_usedMemory(0) );
	}
	{
		SIZER_COMPONENT_NAME(pSizer,"STL Allocator Waste");
		CryModuleMemoryInfo meminfo;
		ZeroStruct(meminfo);
		CryGetMemoryInfoForModule( &meminfo );
		pSizer->AddObject( (this+2),(size_t)meminfo.STL_wasted );
	}
#endif

	{
	  SIZER_COMPONENT_NAME(pSizer, "Particles");
		if(m_pPartManager)
			m_pPartManager->GetMemoryUsage(pSizer);
	}

	pSizer->AddContainer(m_lstDynLights);
  pSizer->AddContainer(m_lstDynLightsNoLight);
  pSizer->AddContainer(m_lstStaticLights);
  pSizer->AddContainer(m_lstAffectingLightsCombinations);
  pSizer->AddContainer(m_arrLightProjFrustums);
  pSizer->AddContainer(m_arrEntsInFoliage);

  pSizer->AddContainer(m_lstVoxelObjectsForUpdate);
  pSizer->AddContainer(m_lstRoadRenderNodesForUpdate);

  pSizer->AddContainer(m_lstKilledVegetations);
  pSizer->AddContainer(arrFPSforSaveLevelStats);
  pSizer->AddContainer(m_lstAlwaysVisible);
	pSizer->AddObject(m_pCVars, sizeof(CVars));

	if(m_pDecalManager)
	{
	  SIZER_COMPONENT_NAME(pSizer, "DecalManager");
		m_pDecalManager->GetMemoryUsage(pSizer);
		pSizer->AddObject(m_pDecalManager, sizeof(*m_pDecalManager));
	}

  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
    if(m_pObjectsTree[nSID])
    {
      SIZER_COMPONENT_NAME(pSizer, "OutdoorObjectsTree");
      m_pObjectsTree[nSID]->GetMemoryUsage(pSizer);
    }
  }

	if(m_pObjManager)
	{
	  SIZER_COMPONENT_NAME(pSizer, "ObjManager");
		m_pObjManager->GetMemoryUsage(pSizer);
	}
	
	if (m_pTerrain)
	{
	  SIZER_COMPONENT_NAME(pSizer, "Terrain");
		m_pTerrain->GetMemoryUsage(pSizer);
	}

	if (m_pVisAreaManager)
	{
	  SIZER_COMPONENT_NAME(pSizer, "VisAreas");
		m_pVisAreaManager->GetMemoryUsage(pSizer);
	}

	if(m_pCoverageBuffer)
	{
		SIZER_COMPONENT_NAME(pSizer, "ObjManager");
		pSizer->AddObject(m_pCoverageBuffer, sizeof(*m_pCoverageBuffer));
	}

	if(m_pCoverageBuffer)
		m_pCoverageBuffer->GetMemoryUsage(pSizer);

  {
    SIZER_COMPONENT_NAME(pSizer, "RNTmpDataPool");

    CRNTmpData * pNext = NULL;

    for(CRNTmpData * pElem = m_LTPRootFree.pNext; pElem!=&m_LTPRootFree; pElem = pNext)
    {
      pSizer->AddObject(pElem, sizeof(*pElem));
      pNext = pElem->pNext;
    }

    for(CRNTmpData * pElem = m_LTPRootUsed.pNext; pElem!=&m_LTPRootUsed; pElem = pNext)
    {
      pSizer->AddObject(pElem, sizeof(*pElem));
      pNext = pElem->pNext;
    }
    

//    pSizer->AddObject(&m_RNTmpDataPools, m_RNTmpDataPools.GetMemUsage());
  }

  if(CProcVegetPoolMan * pMan = CTerrainNode::GetProcObjPoolMan())
  {
    SIZER_COMPONENT_NAME(pSizer, "ProcVegetPool");

		if (pMan)
			pMan->GetMemoryUsage(pSizer);

    SProcObjChunkPool * pPool = CTerrainNode::GetProcObjChunkPool();
		if (pPool)
			pPool->GetMemoryUsage(pSizer);
  }

  if(m_arrBaseTextureData.Count())
  {
    SIZER_COMPONENT_NAME(pSizer, "LayersBaseTextureData");

    for(int i=0; i<m_arrBaseTextureData.Count(); i++)
      pSizer->AddObject(&m_arrBaseTextureData[i], m_arrBaseTextureData[i].GetMemoryUsage());
  }

  if(m_pVoxTerrain)
    m_pVoxTerrain->GetMemoryUsage(pSizer);
}

void C3DEngine::GetResourceMemoryUsage(ICrySizer * pSizer,const AABB& cstAABB)
{
	//////////////////////////////////////////////////////////////////////////
	std::vector<IRenderNode*>	cFoundRenderNodes;
	unsigned int							nFoundObjects(0);

	nFoundObjects=GetObjectsInBox(cstAABB);
	cFoundRenderNodes.resize(nFoundObjects,NULL);
	GetObjectsInBox(cstAABB,&cFoundRenderNodes.front());

	size_t nCurrentRenderNode(0);
	size_t nTotalNumberOfRenderNodes(0);

	nTotalNumberOfRenderNodes=cFoundRenderNodes.size();
	for (nCurrentRenderNode=0;nCurrentRenderNode<nTotalNumberOfRenderNodes;++nCurrentRenderNode)
	{
		IRenderNode*& piRenderNode=cFoundRenderNodes[nCurrentRenderNode];

		IMaterial*	piMaterial(piRenderNode->GetMaterialOverride());
		if (!piMaterial)
		{
			piMaterial=piRenderNode->GetMaterial();			
		}

		if (piMaterial)
		{
			piMaterial->GetResourceMemoryUsage(pSizer);
		}

		{
			IRenderMesh*	piMesh(NULL);
			size_t				nCount(0);

			piMesh=piRenderNode->GetRenderMesh(nCount);
			for (;piMesh;++nCount,piMesh=piRenderNode->GetRenderMesh(nCount))
			{
				// Timur, RenderMesh may not be loaded due to streaming!
				piMesh->GetMemoryUsage(pSizer,IRenderMesh::MEM_USAGE_COMBINED);
			}				
		}
	}
	//////////////////////////////////////////////////////////////////////////

 if (m_pTerrain)
 {
	 CTerrainNode	*poTerrainNode=m_pTerrain->FindMinNodeContainingBox(cstAABB, GetDefSID());
	 if (poTerrainNode)
	 {
		 poTerrainNode->GetResourceMemoryUsage(pSizer,cstAABB);
	 }
 }


	//{
	//	SIZER_COMPONENT_NAME(pSizer, "Particles");
	//	if(m_pPartManager)
	//		m_pPartManager->GetMemoryUsage(pSizer);
	//}

	//pSizer->AddContainer(m_lstDynLights);
	//pSizer->AddContainer(m_lstDynLightsNoLight);
	//pSizer->AddContainer(m_lstStaticLights);
	//pSizer->AddContainer(m_lstAffectingLightsCombinations);
	//pSizer->AddContainer(m_arrLightProjFrustums);
	//pSizer->AddContainer(m_arrEntsInFoliage);

	//pSizer->AddContainer(m_lstVoxelObjectsForUpdate);
	//pSizer->AddContainer(m_lstRoadRenderNodesForUpdate);

	//pSizer->AddContainer(m_lstKilledVegetations);
	//pSizer->AddContainer(arrFPSforSaveLevelStats);
	//pSizer->AddContainer(m_lstAlwaysVisible);
	//pSizer->AddObject(m_pCVars, sizeof(CVars));

	//if(m_pDecalManager)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "DecalManager");
	//	m_pDecalManager->GetMemoryUsage(pSizer);
	//	pSizer->AddObject(m_pDecalManager, sizeof(*m_pDecalManager));
	//}

	//if(m_pObjectsTree[nSID])
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "OutdoorObjectsTree");
	//	m_pObjectsTree[nSID]->GetMemoryUsage(pSizer);
	//}

	//if(m_pObjManager)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "ObjManager");
	//	m_pObjManager->GetMemoryUsage(pSizer);
	//}

	//if (m_pTerrain)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "Terrain");
	//	m_pTerrain->GetMemoryUsage(pSizer);
	//}

	//if (m_pVisAreaManager)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "VisAreas");
	//	m_pVisAreaManager->GetMemoryUsage(pSizer);
	//}

	//if(m_pCoverageBuffer)
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "ObjManager");
	//	pSizer->AddObject(m_pCoverageBuffer, sizeof(*m_pCoverageBuffer));
	//}

	//if(m_pCoverageBuffer)
	//	m_pCoverageBuffer->GetMemoryUsage(pSizer);

	//{
	//	SIZER_COMPONENT_NAME(pSizer, "RNTmpDataPool");

	//	CRNTmpData * pNext = NULL;

	//	for(CRNTmpData * pElem = m_LTPRootFree.pNext; pElem!=&m_LTPRootFree; pElem = pNext)
	//	{
	//		pSizer->AddObject(pElem, sizeof(*pElem));
	//		pNext = pElem->pNext;
	//	}

	//	for(CRNTmpData * pElem = m_LTPRootUsed.pNext; pElem!=&m_LTPRootUsed; pElem = pNext)
	//	{
	//		pSizer->AddObject(pElem, sizeof(*pElem));
	//		pNext = pElem->pNext;
	//	}


	//	//    pSizer->AddObject(&m_RNTmpDataPools, m_RNTmpDataPools.GetMemUsage());
	//}

	//if(CProcVegetPoolMan * pMan = CTerrainNode::GetProcObjPoolMan())
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "ProcVegetPool");

	//	if (pMan)
	//		pMan->GetMemoryUsage(pSizer);

	//	SProcObjChunkPool * pPool = CTerrainNode::GetProcObjChunkPool();
	//	if (pPool)
	//		pPool->GetMemoryUsage(pSizer);
	//}

	//if(m_arrBaseTextureData.Count())
	//{
	//	SIZER_COMPONENT_NAME(pSizer, "LayersBaseTextureData");

	//	for(int i=0; i<m_arrBaseTextureData.Count(); i++)
	//		pSizer->AddObject(&m_arrBaseTextureData[i], m_arrBaseTextureData[i].GetMemoryUsage());
	//}

	//if(m_pVoxTerrain)
	//	m_pVoxTerrain->GetMemoryUsage(pSizer);
}

bool C3DEngine::IsUnderWater( const Vec3& vPos ) const
{
	bool bUnderWater = false;
	for (IPhysicalEntity* pArea = 0; pArea = GetPhysicalWorld()->GetNextArea(pArea); )
	{
		if (bUnderWater)
			// Must finish iteration to unlock areas.
			continue;

		pe_params_buoyancy buoy;
		if (pArea->GetParams(&buoy) && buoy.iMedium == 0)
		{
			if (!is_unused(buoy.waterPlane.n))
			{
				if (buoy.waterPlane.n * vPos < buoy.waterPlane.n * buoy.waterPlane.origin)
				{
					pe_status_contains_point st;
					st.pt = vPos;
					if (pArea->GetStatus(&st))
						bUnderWater = true;
				}
			}
		}
	}
	return bUnderWater;
}

void C3DEngine::SetOceanRenderFlags( uint8 nFlags )
{
  m_nOceanRenderFlags[m_nRenderThreadListID] = nFlags;
}

uint8 C3DEngine::GetOceanRenderFlags() const
{
  return m_nOceanRenderFlags[m_nRenderThreadListID];
}

uint32 C3DEngine::GetOceanVisiblePixelsCount() const
{
  return COcean::GetVisiblePixelsCount();
}

float C3DEngine::GetBottomLevel(const Vec3 & referencePos, float maxRelevantDepth, int objflags)
{
	FUNCTION_PROFILER_3DENGINE;

	const float terrainWorldZ = GetTerrainElevation(referencePos.x, referencePos.y);

	const float padding = 0.2f;
	float rayLength;

	// NOTE: Terrain is above referencePos, so referencePos is probably inside a voxel or something.
	if (terrainWorldZ <= referencePos.z)
		rayLength = min(maxRelevantDepth, (referencePos.z - terrainWorldZ));
	else
		rayLength = maxRelevantDepth;

	rayLength += padding * 2.0f;

	ray_hit hit;
	int rayFlags = geom_colltype_player<<rwi_colltype_bit | rwi_stop_at_pierceable;
	if (m_pPhysicalWorld->RayWorldIntersection(referencePos + Vec3(0,0,padding), Vec3(0,0,-rayLength), 
		ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid, rayFlags, &hit, 1))
	{
		return hit.pt.z;
	}

	// Terrain was above or too far below referencePos, and no solid object was close enough.
	return BOTTOM_LEVEL_UNKNOWN;
}

float C3DEngine::GetBottomLevel(const Vec3 & referencePos, float maxRelevantDepth /* = 10.0f */)
{
	return GetBottomLevel (referencePos, maxRelevantDepth, ent_terrain|ent_static|ent_sleeping_rigid|ent_rigid);
}

float C3DEngine::GetBottomLevel(const Vec3 & referencePos, int objflags)
{
	return GetBottomLevel (referencePos, 10.0f, objflags);
}

float C3DEngine::GetWaterLevel(const Vec3 * pvPos, Vec3 * pvFlowDir)
{
	FUNCTION_PROFILER_3DENGINE;

	bool bInVisarea = m_pVisAreaManager && m_pVisAreaManager->GetVisAreaFromPos(*pvPos)!=0;

	Vec3 gravity;
	pe_params_buoyancy pb[4];
	int i,nBuoys=m_pPhysicalWorld->CheckAreas(*pvPos,gravity,pb,4);
	
	float max_level = (!bInVisarea)? GetOceanWaterLevel( *pvPos ) : WATER_LEVEL_UNKNOWN;

	for(i=0;i<nBuoys;i++)
	{ 
		if (pb[i].iMedium==0 && (!bInVisarea || fabs_tpl(pb[i].waterPlane.origin.x)+fabs_tpl(pb[i].waterPlane.origin.y)>0))
		{
			float fVolumeLevel = pvPos->z-pb[i].waterPlane.n.z*(pb[i].waterPlane.n*(*pvPos-pb[i].waterPlane.origin));

			// it's not ocean
			if( pb[i].waterPlane.origin.y + pb[i].waterPlane.origin.x != 0.0f)
				max_level = max(max_level, fVolumeLevel);
		}
	}

	return max(WATER_LEVEL_UNKNOWN, max_level);
}

float C3DEngine::GetWaterLevel()
{
	//function is too trivial to add profiling to(overhead more than function itself)
//	FUNCTION_PROFILER_3DENGINE;
	return m_pTerrain? m_pTerrain->CTerrain::GetWaterLevel() : WATER_LEVEL_UNKNOWN;
}

float C3DEngine::GetOceanWaterLevel( const Vec3 &pCurrPos ) const
{
  FUNCTION_PROFILER_3DENGINE;



  static int nFrameID = -1;
  int nCurrFrameID = GetFrameID();

  static float fWaterLevel = 0;
  if( nFrameID != nCurrFrameID && m_pTerrain)
  {
    fWaterLevel = m_pTerrain->GetWaterLevel();
    nFrameID = nCurrFrameID;
  }

  return fWaterLevel + COcean::GetWave( pCurrPos );
}

Vec4 C3DEngine::GetCausticsParams() const
{
  return Vec4(1, m_oceanCausticsDistanceAtten, m_oceanCausticsMultiplier, m_oceanCausticsDarkeningMultiplier);
}

void C3DEngine::GetOceanAnimationParams(Vec4 &pParams0, Vec4 &pParams1 ) const
{
  static int nFrameID = -1;
  int nCurrFrameID = GetFrameID();

  static Vec4 s_pParams0 = Vec4(0,0,0,0);
  static Vec4 s_pParams1 = Vec4(0,0,0,0);

  if( nFrameID != nCurrFrameID && m_pTerrain)
  {
		cry_sincosf(m_oceanWindDirection, &s_pParams1.y, &s_pParams1.z);

    s_pParams0 = Vec4(m_oceanWindDirection,  m_oceanWindSpeed, m_oceanWavesSpeed, m_oceanWavesAmount);
    s_pParams1.x = m_oceanWavesSize;
    s_pParams1.w = m_pTerrain->GetWaterLevel();

    nFrameID = nCurrFrameID;
  }
  
  pParams0 = s_pParams0;
  pParams1 = s_pParams1;
};

IVisArea * C3DEngine::CreateVisArea()
{
	return m_pObjManager ? m_pVisAreaManager->CreateVisArea() : 0;
}

void C3DEngine::DeleteVisArea(IVisArea * pVisArea)
{
  if(!m_pVisAreaManager->IsValidVisAreaPointer((CVisArea*)pVisArea))
  {
    Warning( "I3DEngine::DeleteVisArea: Invalid VisArea pointer");
    return;
  }
	if(m_pObjManager)
	{
		CVisArea * pArea = (CVisArea*)pVisArea;

//		if(pArea->m_pObjectsTree)
	//		pArea->m_pObjectsTree->FreeAreaBrushes(true);

		PodArray<SRNInfo> lstEntitiesInArea;
		if(pArea->m_pObjectsTree)
			pArea->m_pObjectsTree->MoveObjectsIntoList(&lstEntitiesInArea,NULL);

		// unregister from indoor
		for(int i=0; i<lstEntitiesInArea.Count(); i++)
			Get3DEngine()->UnRegisterEntity(lstEntitiesInArea[i].pNode);

		if(pArea->m_pObjectsTree)
			assert(pArea->m_pObjectsTree->GetObjectsCount(eMain)==0);

		m_pVisAreaManager->DeleteVisArea((CVisArea*)pVisArea);

		for(int i=0; i<lstEntitiesInArea.Count(); i++)
			Get3DEngine()->RegisterEntity(lstEntitiesInArea[i].pNode);
	}
}

void C3DEngine::UpdateVisArea(IVisArea * pVisArea, const Vec3 * pPoints, int nCount, const char * szName, 
  const SVisAreaInfo & info, bool bReregisterObjects)
{
	if(!m_pObjManager)
    return;

  CVisArea * pArea = (CVisArea*)pVisArea;

//	PrintMessage("C3DEngine::UpdateVisArea: %s", szName);

  Vec3 vTotalBoxMin = pArea->m_boxArea.min;
  Vec3 vTotalBoxMax = pArea->m_boxArea.max;

  m_pVisAreaManager->UpdateVisArea((CVisArea*)pVisArea, pPoints, nCount, szName, info);

  if(((CVisArea*)pVisArea)->m_pObjectsTree && ((CVisArea*)pVisArea)->m_pObjectsTree->GetObjectsCount(eMain))
  {
    // merge old and new bboxes
    vTotalBoxMin.CheckMin(pArea->m_boxArea.min);
    vTotalBoxMax.CheckMax(pArea->m_boxArea.max);
  }
  else
  {
    vTotalBoxMin = pArea->m_boxArea.min;
    vTotalBoxMax = pArea->m_boxArea.max;
  }

	if(bReregisterObjects)
		m_pObjManager->ReregisterEntitiesInArea(vTotalBoxMin - Vec3(8,8,8), vTotalBoxMax + Vec3(8,8,8));
}

void C3DEngine::ResetParticlesAndDecals()
{
	if(m_pPartManager)
		m_pPartManager->Reset(true);

	if(m_pDecalManager)
		m_pDecalManager->Reset();

	CStatObjFoliage *pFoliage,*pFoliageNext;
	for(pFoliage=m_pFirstFoliage; &pFoliage->m_next!=&m_pFirstFoliage; pFoliage=pFoliageNext) {
		pFoliageNext = pFoliage->m_next;
		delete pFoliage;
	}

  ReRegisterKilledVegetationInstances();
}

IRenderNode * C3DEngine::CreateRenderNode( EERType type )
{
	switch (type)
	{
		case eERType_Brush:
			{
				CBrush * pBrush = new CBrush();
				return pBrush;
			}
		case eERType_Vegetation:
			{
				CVegetation * pVeget = new CVegetation();
				return pVeget;
			}
		case eERType_Cloud:
			{
				CCloudRenderNode* pCloud = new CCloudRenderNode();
				return pCloud;
			}
		case eERType_VoxelObject:
			{
				CVoxelObject* pVoxArea = new CVoxelObject(Vec3(0,0,0), DEF_VOX_UNIT_SIZE, 
					DEF_VOX_VOLUME_SIZE, DEF_VOX_VOLUME_SIZE, DEF_VOX_VOLUME_SIZE);
				return pVoxArea;
			}
		case eERType_FogVolume:
			{
				CFogVolumeRenderNode* pFogVolume = new CFogVolumeRenderNode();
				return pFogVolume;
			}
    case eERType_Road:
      {
        CRoadRenderNode* pRoad = new CRoadRenderNode();
        return pRoad;
      }
		case eERType_Decal:
			{
				CDecalRenderNode* pDecal = new CDecalRenderNode();
				return pDecal;
			}
		case eERType_WaterVolume:
			{
				CWaterVolumeRenderNode* pWaterVolume = new CWaterVolumeRenderNode();
				return pWaterVolume;
			}
    case eERType_WaterWave:
      {
        CWaterWaveRenderNode *pRenderNode = new CWaterWaveRenderNode();
        return pRenderNode;
      }
		case eERType_DistanceCloud:
			{
				CDistanceCloudRenderNode *pRenderNode = new CDistanceCloudRenderNode();
				return pRenderNode;
			}
		case eERType_AutoCubeMap:
			{
				CAutoCubeMapRenderNode *pRenderNode = new CAutoCubeMapRenderNode();
				return pRenderNode;
			}
		case eERType_Rope:
			{
				IRopeRenderNode *pRenderNode = new CRopeRenderNode();
				return pRenderNode;
			}
		case eERType_VolumeObject:
			{
				IVolumeObjectRenderNode* pRenderNode = new CVolumeObjectRenderNode();
				return pRenderNode;
			}
		case eERType_IrradianceVolume:
			{
				IIrradianceVolumeRenderNode* pRenderNode = new CIrradianceVolumeRenderNode();
				return pRenderNode;
			}

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
		case eERType_PrismObject:
			{
				IPrismRenderNode* pRenderNode = new CPrismRenderNode();
				return pRenderNode;
			}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

	}
	assert( !"C3DEngine::CreateRenderNode: Specified node type is not supported." );
	return 0;
}

void C3DEngine::DeleteRenderNode(IRenderNode * pRenderNode)
{
	UnRegisterEntity(pRenderNode);
//	m_pObjManager->m_lstBrushContainer.Delete((CBrush*)pRenderNode);
	//m_pObjManager->m_lstVegetContainer.Delete((CVegetation*)pRenderNode);
	delete pRenderNode;
}

void C3DEngine::SetWind( const Vec3 & vWind )
{
	m_vWindSpeed = vWind;
	if (!m_vWindSpeed.IsZero())
	{
		// Maintain a large physics area for global wind.
		if (!m_pGlobalWind)
		{
			primitives::sphere geomSphere;
			geomSphere.center = Vec3(0);
			geomSphere.r = 1e7f;
			IGeometry *pGeom = m_pPhysicalWorld->GetGeomManager()->CreatePrimitive( primitives::sphere::type, &geomSphere );
			m_pGlobalWind = GetPhysicalWorld()->AddArea( pGeom, Vec3(ZERO), Quat(IDENTITY), 1.0f );

			pe_params_foreign_data fd;
			fd.iForeignFlags = PFF_OUTDOOR_AREA;
			m_pGlobalWind->SetParams(&fd);
		}

		// Set medium half-plane above water level.
		pe_params_buoyancy pb;
		pb.iMedium = 1;
		pb.waterPlane.n.Set(0,0,-1);
		pb.waterPlane.origin.Set(0,0,GetWaterLevel());
		pb.waterFlow = m_vWindSpeed;
		pb.waterResistance = 0;
		pb.waterDensity = 0;
		m_pGlobalWind->SetParams(&pb);
	}
	else if (m_pGlobalWind)
	{
		GetPhysicalWorld()->DestroyPhysicalEntity( m_pGlobalWind );
		m_pGlobalWind = 0;
	}
}

Vec3 C3DEngine::GetWind( const AABB& box, bool bIndoors ) const
{
  FUNCTION_PROFILER_3DENGINE;

	if (!m_pCVars->e_Wind)
		return Vec3(ZERO);

	// Start with global wind.
	Vec3 vWind;
	if (bIndoors)
		vWind.zero();
	else
		vWind = m_vWindSpeed;
	if (m_pCVars->e_WindAreas)
	{
		// Iterate all areas, looking for wind areas. Skip first (global) area, and global wind.
		pe_params_buoyancy pb;

		IPhysicalEntity* pArea = GetPhysicalWorld()->GetNextArea(0);
		if (pArea)
		{
			assert(!(pArea->GetParams(&pb) && pb.iMedium == 1));
			while (pArea = GetPhysicalWorld()->GetNextArea(pArea))
			{
				if (pArea != m_pGlobalWind && pArea->GetParams(&pb) && pb.iMedium == 1)
				{
					if (bIndoors)
					{
						// Skip outdoor areas.
						pe_params_foreign_data fd;
						if (pArea->GetParams(&fd) && (fd.iForeignFlags & PFF_OUTDOOR_AREA))
							continue;
					}
					pe_status_area area;
					area.ctr = box.GetCenter();				
					area.size = box.GetSize() * 0.5f;          
					if (pArea->GetStatus(&area))
						vWind += area.pb.waterFlow;
				}
			}
		}
	}
	return vWind;
}

IVisArea * C3DEngine::GetVisAreaFromPos(const Vec3 &vPos )
{
	if(m_pObjManager && m_pVisAreaManager)
		return m_pVisAreaManager->GetVisAreaFromPos(vPos);

	return 0;
}

bool C3DEngine::IntersectsVisAreas(const AABB& box, void** pNodeCache)
{
	if(m_pObjManager && m_pVisAreaManager)
		return m_pVisAreaManager->IntersectsVisAreas(box, pNodeCache);
	return false;
}

bool C3DEngine::ClipToVisAreas(IVisArea* pInside, Sphere& sphere, Vec3 const& vNormal, void* pNodeCache)
{
	if (pInside)
		return pInside->ClipToVisArea(true, sphere, vNormal);
	else if (m_pVisAreaManager)
		return m_pVisAreaManager->ClipOutsideVisAreas(sphere, vNormal, pNodeCache);
	return false;
}

bool C3DEngine::IsVisAreasConnected(IVisArea * pArea1, IVisArea * pArea2, int nMaxRecursion, bool bSkipDisabledPortals)
{
	if (pArea1==pArea2)
		return (true);	// includes the case when both pArea1 & pArea2 are NULL (totally outside)
										// not considered by the other checks
	if (!pArea1 || !pArea2)
		return (false); // avoid a crash - better to put this check only
										// here in one place than in all the places where this function is called

	nMaxRecursion *= 2; // include portals since portals are the areas

	if(m_pObjManager && m_pVisAreaManager)
		return ((CVisArea*)pArea1)->FindVisArea((CVisArea*)pArea2, nMaxRecursion, bSkipDisabledPortals);

	return false;
}

bool C3DEngine::IsOutdoorVisible()
{
	if(m_pObjManager && m_pVisAreaManager)
		return m_pVisAreaManager->IsOutdoorAreasVisible();

	return false;
}

void C3DEngine::EnableOceanRendering(bool bOcean)
{
	m_bOcean = bOcean;
}

//////////////////////////////////////////////////////////////////////////
IMaterialManager* C3DEngine::GetMaterialManager()
{
	return m_pMatMan;
}
//////////////////////////////////////////////////////////////////////////

bool C3DEngine::IsTerrainHightMapModifiedByGame()
{
  return m_pTerrain ? (m_pTerrain->m_bHeightMapModified!=0) : 0;
}

void C3DEngine::CheckPhysicalized(const Vec3 & vBoxMin, const Vec3 & vBoxMax)
{
/*  if(!GetCVars()->e_stream_areas)
    return;

  CVisArea * pVisArea = (CVisArea *)GetVisAreaFromPos((vBoxMin+vBoxMax)*0.5f);
  if(pVisArea)
  { // indoor
    pVisArea->CheckPhysicalized();
    IVisArea * arrConnections[16];
    int nConections = pVisArea->GetRealConnections(arrConnections,16);
    for(int i=0; i<nConections; i++)
      ((CVisArea*)arrConnections[i])->CheckPhysicalized();
  }
  else
  { // outdoor
    CTerrainNode * arrSecInfo[] = 
    {
      m_pTerrain->GetSecInfo(int(vBoxMin.x), int(vBoxMin.y)),
      m_pTerrain->GetSecInfo(int(vBoxMax.x), int(vBoxMin.y)),
      m_pTerrain->GetSecInfo(int(vBoxMin.x), int(vBoxMax.y)),
      m_pTerrain->GetSecInfo(int(vBoxMax.x), int(vBoxMax.y))
    };

    for(int i=0; i<4; i++)
      if(arrSecInfo[i])
        arrSecInfo[i]->CheckPhysicalized();
  }*/
}

void C3DEngine::CheckMemoryHeap()
{
	assert (IsHeapValid());
}

bool C3DEngine::SetMaterialFloat( char * szMatName, int nSubMatId, int nTexSlot, char * szParamName, float fValue )
{
	IMaterial * pMat = GetMaterialManager()->FindMaterial( szMatName );
	if(!pMat)
	{ Warning("I3DEngine::SetMaterialFloat: Material not found: %s", szMatName); return false; }

	if(nSubMatId>0)
	{
		if(nSubMatId<=pMat->GetSubMtlCount())
			pMat = pMat->GetSubMtl(nSubMatId-1);
		else
		{ 
			Warning("I3DEngine::SetMaterialFloat: SubMaterial not found: %s, SubMatId: %d", szMatName, nSubMatId); 
			return false; 
		}
	}

	SEfResTexture * pTex = pMat->GetShaderItem().m_pShaderResources->GetTexture(nTexSlot);
	if(!pTex)
	{ Warning("I3DEngine::SetMaterialFloat: Texture slot not found: %s, TexSlot: %d", szMatName, nTexSlot); return false; }

	char szM_Name[256] = "";

	if(strncmp(szParamName,"m_", 2))
		strcpy_s(szM_Name,"m_");

	strcat_s(szM_Name,szParamName);

	return pTex->m_TexModificator.SetMember(szM_Name,fValue);
}

void C3DEngine::CloseTerrainTextureFile(int nSID)
{
	if(m_pTerrain)
		m_pTerrain->CloseTerrainTextureFile(nSID);
}

//////////////////////////////////////////////////////////////////////////
int C3DEngine::GetLoadedObjectCount()
{
	int nObjectsLoaded = m_pObjManager ? m_pObjManager->GetLoadedObjectCount() : 0;
	
	if(gEnv->pCharacterManager)
	{
		ICharacterManager::Statistics AnimStats;
		gEnv->pCharacterManager->GetStatistics(AnimStats);
		nObjectsLoaded += AnimStats.numAnimObjectModels + AnimStats.numCharModels;
	}

  nObjectsLoaded += GetInstCount(eERType_VoxelObject);

	return nObjectsLoaded;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::GetLoadedStatObjArray( IStatObj** pObjectsArray,int &nCount )
{
	if (m_pObjManager)
		m_pObjManager->GetLoadedStatObjArray( pObjectsArray,nCount );
	else
		nCount = 0;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::GetObjectsStreamingStatus(SObjectsStreamingStatus &outStatus)
{
	if (m_pObjManager)
	{
		m_pObjManager->GetObjectsStreamingStatus(outStatus);
	}
	else
	{
		memset(&outStatus, 0, sizeof(outStatus));
	}
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::DeleteEntityDecals(IRenderNode * pEntity)
{
	if(m_pDecalManager && pEntity && (pEntity->m_nInternalFlags & IRenderNode::DECAL_OWNER))
		m_pDecalManager->OnEntityDeleted(pEntity);
}

void C3DEngine::CompleteObjectsGeometry()
{
  while(m_pTerrain && m_pTerrain->Voxel_Recompile_Modified_Incrementaly_Objects());
  while(m_pTerrain && m_pTerrain->Recompile_Modified_Incrementaly_RoadRenderNodes());
}

void C3DEngine::DeleteDecalsInRange( AABB * pAreaBox, IRenderNode * pEntity )
{
	if(m_pDecalManager)
		m_pDecalManager->DeleteDecalsInRange( pAreaBox, pEntity );
}

void C3DEngine::LockCGFResources()
{
	if(m_pObjManager)
		m_pObjManager->m_bLockCGFResources = 1;
}

void C3DEngine::UnlockCGFResources()
{
	if(m_pObjManager)
	{
		m_pObjManager->m_bLockCGFResources = 0;
		m_pObjManager->FreeNotUsedCGFs();
	}
}

void CLightEntity::ShadowMapInfo::Release(struct IRenderer * pRenderer)
{
/*	if(pShadowMapCasters)
	{
//		pShadowMapCasters->Clear();

//		delete pShadowMapCasters;
		pShadowMapCasters = 0;
	}
*/
/*	if(	pShadowMapFrustumContainer )//&& eEntType != eERType_Vegetation ) // vegetations share same frustum allocated in CStatObj
	{
		delete pShadowMapFrustumContainer->m_LightFrustums.Get(0)->pEntityList;
		pShadowMapFrustumContainer->m_LightFrustums.Get(0)->pEntityList=0;

		delete pShadowMapFrustumContainer;
		pShadowMapFrustumContainer=0;
	}

	if(	pShadowMapFrustumContainerPassiveCasters )//&& eEntType != eERType_Vegetation ) // vegetations share same frustum allocated in CStatObj
	{
		delete pShadowMapFrustumContainerPassiveCasters->m_LightFrustums.Get(0)->pEntityList;
		pShadowMapFrustumContainerPassiveCasters->m_LightFrustums.Get(0)->pCastersList=0;

		delete pShadowMapFrustumContainerPassiveCasters;
		pShadowMapFrustumContainerPassiveCasters=0;
	}*/
/*
	if(pShadowMapRenderMeshsList)
	{
		for(int i=0; i<pShadowMapRenderMeshsList->Count(); i++)
		{
			if(pShadowMapRenderMeshsList->GetAt(i))
			{
				pRenderer->DeleteRenderMesh(pShadowMapRenderMeshsList->GetAt(i));
				pShadowMapRenderMeshsList->GetAt(i)=0;
			}
		}
		delete pShadowMapRenderMeshsList;
		pShadowMapRenderMeshsList=0;
	}*/

	delete this;
}

Vec3 C3DEngine::GetTerrainSurfaceNormal(Vec3 vPos) 
{ 
  if(m_pVoxTerrain && !GetCVars()->e_VoxTerHeightmapEditing)
    return m_pVoxTerrain->GetTerrainSurfaceNormal3D(vPos, .1f);

	return m_pTerrain ? m_pTerrain->GetTerrainSurfaceNormal(vPos, 0.5f*GetHeightMapUnitSize(), true) : Vec3(0.f,0.f,1.f); 
}

IMemoryBlock * C3DEngine::Voxel_GetObjects(Vec3 vPos, float fRadius, int nSurfaceTypeId, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget)
{
	PodArray<CVoxelObject*> lstVoxAreas;

	Vec3 vBaseColor(0,0,0);

	if(m_pTerrain)
		m_pTerrain->DoVoxelShape(vPos, fRadius, nSurfaceTypeId, vBaseColor, eOperation, eShape, eTarget, &lstVoxAreas);

	if(m_pVisAreaManager)
		m_pVisAreaManager->DoVoxelShape(vPos, fRadius, nSurfaceTypeId, vBaseColor, eOperation, eShape, eTarget, &lstVoxAreas);

	CMemoryBlock * pMemBlock = new CMemoryBlock();
	pMemBlock->SetData(lstVoxAreas.GetElements(),lstVoxAreas.GetDataSize());

	return pMemBlock;
}

void C3DEngine::Voxel_Paint(Vec3 vPos, float fRadius, int nSurfaceTypeId, Vec3 vBaseColor, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget, PodArray<IRenderNode*> * pBrushes, float fVoxelSize)
{
  if(eOperation==eveoPaintHeightPos || eOperation==eveoPaintHeightNeg || eOperation==eveoCreate || eOperation==eveoSubstract || eOperation==eveoMaterial || eOperation==eveoBaseColor)
  {
    if(nSurfaceTypeId<0)
    {
      Warning("C3DEngine::Voxel_Paint: Surface type is not specified");
      return;
    }
  }

  if(GetCVars()->e_VoxTer)
  {
    if(!m_pVoxTerrain)
    {
      SVoxTerrainInfo info;
			const float terrainSize = (float)GetTerrainSize();
      info.aabbTerrain = AABB(Vec3(0,0,0), Vec3(terrainSize, terrainSize, terrainSize));
      m_pVoxTerrain = new CVoxTerrain(info);
    }

    return m_pVoxTerrain->DoVoxelShape(vPos, fRadius, nSurfaceTypeId, vBaseColor*255.f, eOperation, eShape, pBrushes, fVoxelSize);
  }

	if(m_pTerrain)
		m_pTerrain->DoVoxelShape(vPos, fRadius, nSurfaceTypeId, vBaseColor, eOperation, eShape, eTarget, NULL);

	if(m_pVisAreaManager)
		m_pVisAreaManager->DoVoxelShape(vPos, fRadius, nSurfaceTypeId, vBaseColor, eOperation, eShape, eTarget, NULL);

  CrySleep(10); // limit update frequency
}

//////////////////////////////////////////////////////////////////////////
IIndexedMesh* C3DEngine::CreateIndexedMesh()
{
	return new CIndexedMesh();
}

void C3DEngine::Voxel_SetFlags(bool bPhysics, bool bSimplify, bool bShadows, bool bMaterials)
{
	if(m_pTerrain)
		m_pTerrain->Voxel_SetFlags(bPhysics, bSimplify, bShadows, bMaterials);
}


void C3DEngine::SerializeState( TSerialize ser )
{
	if (ser.IsReading())
	{
		CStatObjFoliage *pFoliage,*pFoliageNext;
		for(pFoliage=m_pFirstFoliage; &pFoliage->m_next!=&m_pFirstFoliage; pFoliage=pFoliageNext) {
			pFoliageNext = pFoliage->m_next;
			delete pFoliage;
		}
	}

	ser.Value("m_moonRotationLatitude", m_moonRotationLatitude);
	ser.Value("m_moonRotationLongitude", m_moonRotationLongitude);
	if (ser.IsReading())
		UpdateMoonDirection();

	if(m_pTerrain)
		m_pTerrain->SerializeTerrainState(ser);

	if(m_pDecalManager)
		m_pDecalManager->Serialize(ser);

	m_pPartManager->Serialize(ser);
	m_pTimeOfDay->Serialize(ser);
}

void C3DEngine::PostSerialize( bool bReading )
{
	m_pPartManager->PostSerialize(bReading);
}

void C3DEngine::InitMaterialDefautMappingAxis(IMaterial * pMat)
{
	uint8 * arrProj[3] = {(uint8*)"X",(uint8*)"Y",(uint8*)"Z"};
	pMat->m_ucDefautMappingAxis = 'Z';
	pMat->m_fDefautMappingScale = 1.f;
	for(int c=0; c<3 && c<pMat->GetSubMtlCount(); c++)
	{
		IMaterial * pSubMat = (IMaterial*)pMat->GetSubMtl(c);
		pSubMat->m_ucDefautMappingAxis = arrProj[c][0];
		pSubMat->m_fDefautMappingScale = pMat->m_fDefautMappingScale;
	}
}

//////////////////////////////////////////////////////////////////////////
CContentCGF* C3DEngine::LoadChunkFileContent( const char *filename,bool bNoWarningMode )
{
	class Listener : public ILoaderCGFListener
	{
	public:
		virtual void Warning( const char *format ) {Cry3DEngineBase::Warning("%s", format);}
		virtual void Error( const char *format ) {Cry3DEngineBase::Error("%s", format);}
    virtual bool IsValidationEnabled( ) { return Cry3DEngineBase::GetCVars()->e_StatObjValidate != 0; }
	};
	Listener listener;
  CLoaderCGF cgf;
#if !defined(XENON) && !defined(PS3)
	 CReadOnlyChunkFile *pChunkFile = new CReadOnlyChunkFile(true,bNoWarningMode);


#endif
	CContentCGF *pCGF = cgf.LoadCGF( filename,*pChunkFile,&listener );
	if (pCGF)
	{
		pCGF->SetChunkFile(pChunkFile);
	}
	return pCGF;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::ReleaseChunkFileContent( CContentCGF *pCGF )
{
	delete pCGF;
}

//////////////////////////////////////////////////////////////////////////
IChunkFile * C3DEngine::CreateChunkFile( bool bReadOnly )
{
	if (bReadOnly)
    return new CReadOnlyChunkFile(false);
	else
		return new CChunkFile;
}

int C3DEngine::GetTerrainTextureNodeSizeMeters()
{
	if(m_pTerrain)
		return m_pTerrain->GetTerrainTextureNodeSizeMeters();
	return 0;
}

int C3DEngine::GetTerrainTextureNodeSizePixels(int nLayer)
{
	if(m_pTerrain)
		return m_pTerrain->GetTerrainTextureNodeSizePixels(nLayer,0);
	return 0;
}

void C3DEngine::SetHeightMapMaxHeight(float fMaxHeight)
{
	if(m_pTerrain)
		m_pTerrain->SetHeightMapMaxHeight(fMaxHeight);
}

ITerrain * C3DEngine::CreateTerrain(const STerrainInfo & TerrainInfo)
{
	delete m_pTerrain;
	m_pTerrain = new CTerrain(TerrainInfo);
	return (ITerrain *)m_pTerrain;
}

void C3DEngine::DeleteTerrain()
{
	delete m_pTerrain;
	m_pTerrain = NULL;
}

IVoxTerrain * C3DEngine::CreateVoxTerrain(const SVoxTerrainInfo & Info)
{
  SAFE_DELETE(m_pVoxTerrain);
  if(GetCVars()->e_VoxTer)
    m_pVoxTerrain = new CVoxTerrain(Info);
  return (IVoxTerrain *)m_pVoxTerrain;
}

void C3DEngine::DeleteVoxTerrain()
{
  SAFE_DELETE(m_pVoxTerrain);
}

/*
void C3DEngine::RenderImposterContent(class CREImposter * pImposter, const CCamera & cam)
{
	if(m_pTerrain && pImposter)
	{
		// set camera and disable frustum culling
		CCamera oldCam = Get3DEngine()->GetCamera();
		Get3DEngine()->SetCamera(cam);
		GetCVars()->e_camera_frustum_test = false;

		// start rendering
		GetRenderer()->EF_StartEf();  

		// prepare lights
		UpdateLightSources();
		PrepareLightSourcesForRendering();    

		// render required terrain sector
		m_pTerrain->RenderImposterContent(pImposter, cam);

		// setup fog
		SetupDistanceFog();

		// finish rendering
		GetRenderer()->EF_EndEf3D(SHDF_SORT | SHDF_ZPASS);

		// restore
		Get3DEngine()->SetCamera(oldCam);
		GetCVars()->e_camera_frustum_test = true;
	}
}*/

void C3DEngine::GetVoxelRenderNodes(struct IRenderNode**pRenderNodes, int & nCount)
{
}

void C3DEngine::PrecacheLevel(bool bPrecacheAllVisAreas, Vec3 * pPrecachePoints, int nPrecachePointsNum)
{
	if(GetVisAreaManager())
		GetVisAreaManager()->PrecacheLevel(bPrecacheAllVisAreas, pPrecachePoints, nPrecachePointsNum);
}

//////////////////////////////////////////////////////////////////////////
ITimeOfDay* C3DEngine::GetTimeOfDay()
{
	return m_pTimeOfDay;
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::SetGlobalParameter( E3DEngineParameter param,const Vec3 &v )
{
	float fValue = v.x;
	switch (param)
	{
	case E3DPARAM_SKY_COLOR:
		SetSkyColor( v );
		break;
	case E3DPARAM_SUN_COLOR:
		SetSunColor( v );
		break;

	case E3DPARAM_AMBIENT_GROUND_COLOR:
		m_vAmbGroundCol = v;
		break;
	case E3DPARAM_AMBIENT_MIN_HEIGHT:
		m_fAmbMaxHeight = v.x;
		break;
	case E3DPARAM_AMBIENT_MAX_HEIGHT:
		m_fAmbMinHeight = v.x;
		break;

	case E3DPARAM_SKY_HIGHLIGHT_POS:
		m_vSkyHightlightPos = v;
		break;
	case E3DPARAM_SKY_HIGHLIGHT_COLOR:
		m_vSkyHightlightCol = v;
		break;
	case E3DPARAM_SKY_HIGHLIGHT_SIZE:
		m_fSkyHighlightSize = fValue > 0.0f ? fValue : 0.0f;
		break;
	case E3DPARAM_VOLFOG_RAMP:
		m_volFogRamp = v;
		break;
	case E3DPARAM_NIGHSKY_HORIZON_COLOR:
		m_nightSkyHorizonCol = v;
		break;
	case E3DPARAM_NIGHSKY_ZENITH_COLOR:
		m_nightSkyZenithCol = v;
		break;
	case E3DPARAM_NIGHSKY_ZENITH_SHIFT:
		m_nightSkyZenithColShift = v.x;
		break;
	case E3DPARAM_NIGHSKY_STAR_INTENSITY:
		m_nightSkyStarIntensity = v.x;
		break;
	case E3DPARAM_NIGHSKY_MOON_COLOR:
		m_nightMoonCol = v;
		break;
	case E3DPARAM_NIGHSKY_MOON_SIZE:
		m_nightMoonSize = v.x;
		break;
	case E3DPARAM_NIGHSKY_MOON_INNERCORONA_COLOR:
		m_nightMoonInnerCoronaCol = v;
		break;
	case E3DPARAM_NIGHSKY_MOON_INNERCORONA_SCALE:
		m_nightMoonInnerCoronaScale = v.x;
		break;
	case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_COLOR:
		m_nightMoonOuterCoronaCol = v;
		break;
	case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_SCALE:
		m_nightMoonOuterCoronaScale = v.x;
		break;
	case E3DPARAM_OCEANFOG_COLOR_MULTIPLIER:
		m_oceanFogColorMultiplier = v.x;
		break;
	case E3DPARAM_OCEANFOG_COLOR:
		m_oceanFogColor = v * m_oceanFogColorMultiplierEnvironment;
		break;
	case E3DPARAM_OCEANFOG_DENSITY:
		m_oceanFogDensity = v.x;
		break;
	case E3DPARAM_SKY_MOONROTATION:
		m_moonRotationLatitude = v.x;
		m_moonRotationLongitude = v.y;
		UpdateMoonDirection();
		break;
	case E3DPARAM_SKYBOX_MULTIPLIER:
		m_skyboxMultiplier = v.x;
		break;
	case E3DPARAM_DAY_NIGHT_INDICATOR:
		m_dayNightIndicator = v.x;
		gEnv->pSoundSystem->SetGlobalParameter("daylight", m_dayNightIndicator);
		break;
	case E3DPARAM_FOG_COLOR2:
		m_fogColor2 = v;
		break;
  case E3DPARAM_COLORGRADING_COLOR_SATURATION:
    m_fSaturation = fValue;
    SetPostEffectParam("ColorGrading_Saturation", m_fSaturation);
    break;
  case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR:
    m_pPhotoFilterColor = Vec4(v.x, v.y, v.z, 1);
    SetPostEffectParamVec4("clr_ColorGrading_PhotoFilterColor", m_pPhotoFilterColor);
    break;
  case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY:
    m_fPhotoFilterColorDensity = fValue;
    SetPostEffectParam("ColorGrading_PhotoFilterColorDensity", m_fPhotoFilterColorDensity);
    break;
  case E3DPARAM_COLORGRADING_FILTERS_GRAIN:
    m_fGrainAmount = fValue;
    SetPostEffectParam("ColorGrading_GrainAmount", m_fGrainAmount);
    break;
	case E3DPARAM_SKY_SKYBOX_ANGLE: // sky box rotation
		m_fSkyBoxAngle = fValue;
		break;
	case E3DPARAM_SKY_SKYBOX_STRETCHING: // sky box stretching
		m_fSkyBoxStretching = fValue;
		break;
	case E3DPARAM_SKY_SUNROTATION:
		m_sunRotationZ = v.x;
		m_sunRotationLongitude = v.y;
		break;
	case E3DPARAM_NIGHSKY_MOON_DIRECTION: // moon direction is fixed per level or updated via FG node (E3DPARAM_SKY_MOONROTATION)

	default:
		assert(0);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void C3DEngine::GetGlobalParameter( E3DEngineParameter param,Vec3 &v )
{
	switch (param)
	{
	case E3DPARAM_SKY_COLOR:
		v = GetSkyColor();
		break;
	case E3DPARAM_SUN_COLOR:
		v = GetSunColor();
		break;

	case E3DPARAM_AMBIENT_GROUND_COLOR:
		v = m_vAmbGroundCol;
		break;
	case E3DPARAM_AMBIENT_MIN_HEIGHT:
		v.x = m_fAmbMaxHeight;
		break;
	case E3DPARAM_AMBIENT_MAX_HEIGHT:
		v.x = m_fAmbMinHeight;
		break;

	case E3DPARAM_SKY_HIGHLIGHT_POS:
		v = m_vSkyHightlightPos;
		break;
	case E3DPARAM_SKY_HIGHLIGHT_COLOR:
		v = m_vSkyHightlightCol;
		break;
	case E3DPARAM_SKY_HIGHLIGHT_SIZE:
		v.x = m_fSkyHighlightSize;
		break;
	case E3DPARAM_VOLFOG_RAMP:
		v = m_volFogRamp;
		break;
	case E3DPARAM_NIGHSKY_HORIZON_COLOR:
		v = m_nightSkyHorizonCol;
		break;
	case E3DPARAM_NIGHSKY_ZENITH_COLOR:
		v = m_nightSkyZenithCol;
		break;
	case E3DPARAM_NIGHSKY_ZENITH_SHIFT:
		v = Vec3( m_nightSkyZenithColShift, 0, 0 );
		break;
	case E3DPARAM_NIGHSKY_STAR_INTENSITY:
		v = Vec3( m_nightSkyStarIntensity, 0, 0 );
		break;
	case E3DPARAM_NIGHSKY_MOON_DIRECTION:
		v = m_moonDirection;
		break;
	case E3DPARAM_NIGHSKY_MOON_COLOR:
		v = m_nightMoonCol;
		break;
	case E3DPARAM_NIGHSKY_MOON_SIZE:
		v = Vec3( m_nightMoonSize, 0, 0 );
		break;
	case E3DPARAM_NIGHSKY_MOON_INNERCORONA_COLOR:
		v = m_nightMoonInnerCoronaCol;
		break;
	case E3DPARAM_NIGHSKY_MOON_INNERCORONA_SCALE:
		v = Vec3( m_nightMoonInnerCoronaScale, 0, 0 );
		break;
	case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_COLOR:
		v = m_nightMoonOuterCoronaCol;
		break;
	case E3DPARAM_NIGHSKY_MOON_OUTERCORONA_SCALE:
		v = Vec3( m_nightMoonOuterCoronaScale, 0, 0 );
		break;
	case E3DPARAM_SKY_SUNROTATION:
		v = Vec3(m_sunRotationZ, m_sunRotationLongitude, 0);
		break;
	case E3DPARAM_SKY_MOONROTATION:
		v = Vec3(m_moonRotationLatitude, m_moonRotationLongitude, 0);
		break;
	case E3DPARAM_OCEANFOG_COLOR_MULTIPLIER:
		v = Vec3(m_oceanFogColorMultiplier, 0, 0);
		break;
	case E3DPARAM_OCEANFOG_COLOR:
		v = m_oceanFogColor;
		break;
	case E3DPARAM_OCEANFOG_DENSITY:
		v = Vec3(m_oceanFogDensity, 0, 0);
		break;
	case E3DPARAM_SKYBOX_MULTIPLIER:
		v = Vec3(m_skyboxMultiplier, 0, 0);
		break;
	case E3DPARAM_DAY_NIGHT_INDICATOR:
		v = Vec3(m_dayNightIndicator, 0, 0);
		break;
	case E3DPARAM_FOG_COLOR2:
		v = m_fogColor2;
		break;
  case E3DPARAM_COLORGRADING_COLOR_SATURATION:
    v = Vec3(m_fSaturation, 0, 0);
      break;
  case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR:
    v = Vec3(m_pPhotoFilterColor.x, m_pPhotoFilterColor.y, m_pPhotoFilterColor.z);
    break;
  case E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY:
    v = Vec3(m_fPhotoFilterColorDensity, 0, 0);
      break;
  case E3DPARAM_COLORGRADING_FILTERS_GRAIN:
    v = Vec3(m_fGrainAmount, 0, 0);
      break;
	default:
		assert(0);
	}
}

//////////////////////////////////////////////////////////////////////////
bool C3DEngine::CheckIntersectClouds(const Vec3 & p1, const Vec3 & p2)
{
	return m_pCloudsManager->CheckIntersectClouds(p1, p2);
}

void C3DEngine::OnRenderMeshDeleted(IRenderMesh * pRenderMesh)
{
	if(m_pDecalManager)
		m_pDecalManager->OnRenderMeshDeleted(pRenderMesh);
}

bool C3DEngine::RayObjectsIntersection2D( Vec3 vStart, Vec3 vEnd, Vec3 & vHitPoint, EERType eERType )
{
	float fClosestHitDistance = 1000000;

  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
    if(m_pObjectsTree[nSID])
    {
      if(Overlap::Point_AABB2D(vStart, m_pObjectsTree[nSID]->GetBBox()))
        if(m_pObjectsTree[nSID])
          m_pObjectsTree[nSID]->RayObjectsIntersection2D( vStart, vEnd, vHitPoint, fClosestHitDistance, eERType );
    }
  }

	return (fClosestHitDistance < 1000000);
}

#include "CryThread.h"
namespace
{
  CryCriticalSection g_cCheckCreateRNTmpData;
}

void C3DEngine::CreateRNTmpData(CRNTmpData ** ppInfo, IRenderNode * pRNode)
{
  assert(!(*ppInfo));

  AUTO_LOCK(g_cCheckCreateRNTmpData);

  FUNCTION_PROFILER_3DENGINE;

  // make sure element is allocated
  if(m_LTPRootFree.pNext == &m_LTPRootFree)
  {
    CRNTmpData * pNew = new CRNTmpData;//m_RNTmpDataPools.GetNewElement();
    pNew->Link(&m_LTPRootFree);
  }

  // move element from m_LTPRootFree to m_LTPRootUsed
  CRNTmpData * pElem = m_LTPRootFree.pNext;
  pElem->Unlink();
  pElem->Link(&m_LTPRootUsed);

  *ppInfo = pElem;
  (*ppInfo)->pOwnerRef = ppInfo;

  assert(!(*((*ppInfo)->pOwnerRef))->userData.m_pFoliage);

  memset(&pElem->userData, 0, sizeof(pElem->userData));

  pRNode->OnRenderNodeBecomeVisible();

  (*ppInfo)->nCreatedFrameId = GetMainFrameID();
}

static SBending sBendRemoved;

void C3DEngine::FreeRNTmpData(CRNTmpData ** ppInfo)
{
	assert((*ppInfo)->pNext != &m_LTPRootFree);
	assert((*ppInfo)->pPrev != &m_LTPRootFree);
	(*ppInfo)->Unlink();

  //  (*ppInfo)->ReleaseUserData(GetRenderer());

  {
#ifdef SUPP_HWOBJ_OCCL
    if((*ppInfo)->userData.m_OcclState.pREOcclusionQuery)
    {
      (*ppInfo)->userData.m_OcclState.pREOcclusionQuery->Release(false);
      (*ppInfo)->userData.m_OcclState.pREOcclusionQuery = NULL;
    }
#endif
    if((*ppInfo)->userData.m_pFoliage)
    {
      (*ppInfo)->userData.m_pFoliage->Release();
      (*ppInfo)->userData.m_pFoliage = NULL;
    }

    if((*ppInfo)->userData.pRenderObject)
    {
      if ((*ppInfo)->userData.pRenderObject->m_pBending)
        (*ppInfo)->userData.pRenderObject->m_pBending = &sBendRemoved;
      GetRenderer()->EF_ObjRemovePermanent((*ppInfo)->userData.pRenderObject);
    //  (*ppInfo)->userData.pRenderObject = NULL; //Diesel cut
    }
  }

	(*ppInfo)->Link(&m_LTPRootFree);	
	*((*ppInfo)->pOwnerRef) = NULL;
}

void C3DEngine::UpdateRNTmpDataPool(bool bFreeAll)
{
	FUNCTION_PROFILER_3DENGINE;

	// move old elements into m_LTPRootFree
	CRNTmpData * pNext = NULL;
	for(CRNTmpData * pElem = m_LTPRootUsed.pNext; pElem!=&m_LTPRootUsed; pElem = pNext)
	{
    PrefetchLine(pElem->pNext, 0);

		pNext = pElem->pNext;
		if(bFreeAll || (pElem->nLastUsedFrameId < (GetMainFrameID()-4)))
    {
			FreeRNTmpData(&pElem);
    }
	}
  
/*
  for(int p=0; p<m_RNTmpDataPools.m_Pools.Count(); p++)
  {
    CRNTmpData * pArray = m_RNTmpDataPools.m_Pools[p];

    for(int e=0; e<m_RNTmpDataPools.GetMaxElemsInChunk(); e++)
    {
      CRNTmpData * pElem = &pArray[e];

      if(pElem->pOwnerRef && *(pElem->pOwnerRef))
      {
        if(bFreeAll || (pElem->nLastUsedFrameId < (GetMainFrameID()-4)))
        {
          FreeRNTmpData(&pElem);
        }
      }
    }
  }*/
}

void C3DEngine::FreeRNTmpDataPool()
{
	// move all into m_LTPRootFree
	UpdateRNTmpDataPool(true);

	// delete all elements of m_LTPRootFree
	CRNTmpData * pNext = NULL;
	for(CRNTmpData * pElem = m_LTPRootFree.pNext; pElem!=&m_LTPRootFree; pElem = pNext)
	{
		pNext = pElem->pNext;
		pElem->Unlink();
		delete pElem;
	}
}

bool C3DEngine::IsAmbientOcclusionEnabled() 
{ 
  return GetTerrain() ? GetTerrain()->IsAmbientOcclusionEnabled() : false; 
}


uint32 C3DEngine::CopyObjectsByType( EERType objType, const AABB *pBox, IRenderNode **pObjects )
{
	PodArray<IRenderNode*> lstObjects;

	GetObjectsByTypeGlobal(lstObjects,objType,pBox);

	if (GetVisAreaManager())
		GetVisAreaManager()->GetObjectsByType(lstObjects,objType,pBox);

	if(pObjects && !lstObjects.IsEmpty())
		memcpy(pObjects,&lstObjects[0],lstObjects.GetDataSize());

	return lstObjects.Count();
}

uint32 C3DEngine::CopyObjects(const AABB *pBox, IRenderNode **pObjects )
{
	PodArray<IRenderNode*> lstObjects;

  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  	if (m_pObjectsTree[nSID])
	  	m_pObjectsTree[nSID]->GetObjects(lstObjects,pBox);

	if (GetVisAreaManager())
		GetVisAreaManager()->GetObjects(lstObjects,pBox);

	if(pObjects && !lstObjects.IsEmpty())
		memcpy(pObjects,&lstObjects[0],lstObjects.GetDataSize());

	return lstObjects.Count();
}

uint32 C3DEngine::GetObjectsByType( EERType objType, IRenderNode **pObjects )
{
	return CopyObjectsByType(objType, NULL, pObjects);
}

uint32 C3DEngine::GetObjectsByTypeInBox( EERType objType, const AABB &bbox, IRenderNode **pObjects )
{
	return CopyObjectsByType(objType, &bbox, pObjects);
}

uint32 C3DEngine::GetObjectsInBox(const AABB &bbox, IRenderNode **pObjects)
{
	return CopyObjects(&bbox, pObjects);
}

int SImageInfo::GetMemoryUsage()
{
  int nSize=0;
  if(detailInfo.pImgMips[0])
    nSize += (int)((float)(detailInfo.nDim*detailInfo.nDim*sizeof(ColorB))*1.3f);
  if(baseInfo.pImgMips[0])
    nSize += (int)((float)(baseInfo.nDim*baseInfo.nDim*sizeof(ColorB))*1.3f);
  return nSize;
}

/*void SImageInfo::InitDetailData(int nLayerId)
{
  {
    for(int nMip=0; nMip<SImageSubInfo::nMipsNum; nMip++)
      SAFE_DELETE_ARRAY(detailInfo.pImgMips[nMip]);
  }

  if(pSurfType && pSurfType->pLayerMat)
  {
    SShaderItem & sItem = pSurfType->pLayerMat->GetShaderItem(0);
    if(SEfResTexture * pTex = sItem.m_pShaderResources->GetTexture(EFTT_DIFFUSE))
    {
      if(ITexture * pITex = pTex->m_Sampler.m_pITex)
      {
        PrintMessage("SImageInfo::InitMaterialData: nLayerId=%d, Dim=%dx%d, pMat=%s ...", 
          nLayerId, pITex->GetWidth(), pITex->GetWidth(), pSurfType->pLayerMat ? pSurfType->pLayerMat->GetName() : "NULL");

        int nImgSize = pITex->GetWidth()*pITex->GetWidth()*4;
        byte * pImage = new byte[nImgSize];   

        memcpy(pImage, pITex->GetData32(0,0,0,eTF_A8R8G8B8), nImgSize);

        detailInfo.nDim = pITex->GetWidth();

        detailInfo.fTiling = 1;
        if(pTex->m_bUTile)
          detailInfo.fTiling *= pTex->m_TexModificator.m_Tiling[0];
        detailInfo.fTiling *= detailInfo.fTilingIn;

        byte ** pMips = Get3DEngine()->AllocateMips(pImage, detailInfo.nDim, detailInfo.pImgMips);

        PrintMessagePlus(" ok");
      }
    }
  }
}*/

byte ** C3DEngine::AllocateMips(byte*pImage, int nDim, byte ** pImageMips)
{
  memset(pImageMips,0,SImageSubInfo::nMipsNum*sizeof(pImageMips[0]));

  pImageMips[0] = new byte[nDim*nDim*sizeof(ColorB)];
  memcpy(pImageMips[0], pImage, nDim*nDim*sizeof(ColorB));

  ColorB * pMipMain = (ColorB *)pImageMips[0];

  for(int nMip=1; (nDim>>nMip) && nMip<SImageSubInfo::nMipsNum; nMip++)
  {
    int nDimMip = nDim>>nMip;

    int nSubSize = 1<<nMip;

    pImageMips[nMip] = new byte[nDimMip*nDimMip*sizeof(ColorB)];

    ColorB * pMipThis = (ColorB *)pImageMips[nMip];

    for(int x=0; x<nDimMip; x++)
    {
      for(int y=0; y<nDimMip; y++)
      {
        ColorF colSumm(0,0,0,0);
        float fCount = 0;
        for(int _x = x*nSubSize - nSubSize/2; _x < x*nSubSize + nSubSize + nSubSize/2; _x ++)
        {
          for(int _y = y*nSubSize - nSubSize/2; _y < y*nSubSize + nSubSize + nSubSize/2; _y ++)
          {
            int nMask = nDim-1;
            int id = (_x&nMask)*nDim + (_y&nMask);
            colSumm.r += 1.f/255.f*pMipMain[id].r;
            colSumm.g += 1.f/255.f*pMipMain[id].g;
            colSumm.b += 1.f/255.f*pMipMain[id].b;
            colSumm.a += 1.f/255.f*pMipMain[id].a;
            fCount ++;
          }
        }

        colSumm /= fCount;

        colSumm.Clamp(0,1);

        pMipThis[x*nDimMip + y] = colSumm;
      }
    }
  }

  return pImageMips;
}

void C3DEngine::SetTerrainLayerBaseTextureData(int nLayerId, byte*, int, const char * pImgFileName, IMaterial * pMat, float fBr, float fTiling, int nDetailSurfTypeId, float fLayerTilingDetail, float fSpecularAmount, float fSortOrder, ColorF layerFilterColor, float fUseRemeshing, bool bShowSelection)
{
  if(!GetCVars()->e_VoxTer)
    return;

  if(nLayerId<0)
    return;

  PrintMessage("SetTerrainLayerBaseTextureData: nLayerId=%d, nDetailSurfTypeId=%d, fBr=%.1f, pMat=%s ...", nLayerId, nDetailSurfTypeId, fBr, pMat ? pMat->GetName() : "NULL");

  int nMaxCount = max(nLayerId+1, m_arrBaseTextureData.Count());
  m_arrBaseTextureData.PreAllocate(nMaxCount, nMaxCount);

  SImageInfo & rImgInfo = m_arrBaseTextureData[nLayerId];

  ZeroStruct(rImgInfo.arrTextureId);

  if(pMat && strcmp(pMat->GetName(),"Default"))
  {
    strncpy(rImgInfo.szDetMatName, pMat->GetName(),sizeof(rImgInfo.szDetMatName));
    rImgInfo.nPhysSurfaceType = pMat->GetSurfaceTypeId();
  }
  else
  {
    memset(rImgInfo.szDetMatName,0,sizeof(rImgInfo.szDetMatName));
    rImgInfo.nPhysSurfaceType = 0;
  }

  if(pImgFileName)
    strncpy(rImgInfo.szBaseTexName, pImgFileName,sizeof(rImgInfo.szBaseTexName));
  else
    memset(rImgInfo.szBaseTexName,0,sizeof(rImgInfo.szBaseTexName));

  rImgInfo.nDetailSurfTypeId = nDetailSurfTypeId;

  if(m_pVoxTerrain)
  {
    m_pVoxTerrain->RequestTextureUpdate(AABB(Vec3(0,0,0),Vec3((float)GetTerrainSize(), (float)GetTerrainSize(), (float)GetTerrainSize())));

    if(rImgInfo.fBr == fBr && bShowSelection)
    {
      m_pVoxTerrain->m_nSelLayer = nLayerId;
      m_pVoxTerrain->m_fSelLayerTime = GetCurTimeSec();
    }
  }

  rImgInfo.fBr = fBr;
  rImgInfo.layerFilterColor = layerFilterColor;
  rImgInfo.nLayerId = nLayerId;
  rImgInfo.fUseRemeshing = fUseRemeshing;

  rImgInfo.baseInfo.nSortOrder = (int)fSortOrder;
  rImgInfo.baseInfo.fSpecularAmount = fSpecularAmount; 
  rImgInfo.baseInfo.fTiling = rImgInfo.baseInfo.fTilingIn = fTiling;

  rImgInfo.detailInfo.nSortOrder = 0;
  rImgInfo.detailInfo.fSpecularAmount = 0; 
  rImgInfo.detailInfo.fTiling = rImgInfo.detailInfo.fTilingIn = fLayerTilingDetail;  

  for(int nMip=0; nMip<SImageSubInfo::nMipsNum; nMip++)
    SAFE_DELETE_ARRAY(rImgInfo.baseInfo.pImgMips[nMip]);
  rImgInfo.baseInfo.fAmount = 0;

  // base
  /*if(GetCVars()->e_VoxTerTexBuildOnCPU)
  {
    if(ITexture * pITex = GetRenderer()->EF_LoadTexture(rImgInfo.szBaseTexName, FT_DONT_STREAM,eTT_2D))
    {
      PrintMessage("SImageInfo::LoadBaseTexture: nLayerId=%d, Dim=%dx%d, pMat=%s ...", 
        nLayerId, pITex->GetWidth(), pITex->GetWidth(), rImgInfo.szBaseTexName);
      int nImgSize = pITex->GetWidth()*pITex->GetWidth()*4;
      byte * pImage = new byte[nImgSize];   
      memcpy(pImage, pITex->GetData32(0,0,0,eTF_A8R8G8B8), nImgSize);
      rImgInfo.baseInfo.nDim = pITex->GetWidth();
      rImgInfo.baseInfo.fTiling = 1;
      rImgInfo.baseInfo.fTiling *= rImgInfo.baseInfo.fTilingIn;
      rImgInfo.baseInfo.fAmount = 1;
      byte ** pMips = Get3DEngine()->AllocateMips(pImage, rImgInfo.baseInfo.nDim, rImgInfo.baseInfo.pImgMips);
      delete [] pImage;
      PrintMessagePlus(" ok");
    }
  }*/

  for(int nMip=0; nMip<SImageSubInfo::nMipsNum; nMip++)
    SAFE_DELETE_ARRAY(rImgInfo.detailInfo.pImgMips[nMip]);
  rImgInfo.detailInfo.fAmount = 0;
  rImgInfo.detailInfo.nDim = 0;

  // detail
/*  if(GetCVars()->e_VoxTerTexBuildOnCPU && rImgInfo.szDetMatName[0])
  {
    rImgInfo.detailInfo.pMat = pMat;
    rImgInfo.detailInfo.fTiling = rImgInfo.detailInfo.fTilingIn;
    rImgInfo.detailInfo.fAmount = 1;
  }*/

  PrintMessagePlus(" ok");
}

SImageInfo * C3DEngine::GetBaseTextureData(int nLayerId)
{
  if( nLayerId<0 || nLayerId>=m_arrBaseTextureData.Count() )//|| !m_arrBaseTextureData[nLayerId].baseInfo.nDim)
  {
    return NULL;
  }

  return &m_arrBaseTextureData[nLayerId];
}

SImageInfo * C3DEngine::GetBaseTextureDataFromSurfType(int nLayerId)
{
  assert(nLayerId>=0 && nLayerId<MAX_SURFACE_TYPES_COUNT);

  if(nLayerId<0 || nLayerId>=m_arrBaseTextureData.Count() || !m_arrBaseTextureData[nLayerId].baseInfo.nDim)
    return NULL;

  return &m_arrBaseTextureData[nLayerId];
}

void C3DEngine::RegisterForStreaming(IStreamable*pObj)
{
  if(GetObjManager())
    GetObjManager()->RegisterForStreaming(pObj);
}

void C3DEngine::UnregisterForStreaming(IStreamable*pObj)
{
  if(GetObjManager())
    GetObjManager()->UnregisterForStreaming(pObj);
}

void C3DEngine::RenderRenderNode(IShadowCaster * pRNode, SRenderObjectModifier * pROModifier)
{
  switch(pRNode->GetRenderNodeType())
  {
  case eERType_Vegetation:
    {
      CVegetation * pObj = (CVegetation*)pRNode;
      Get3DEngine()->CheckCreateRNTmpData(&pObj->m_pRNTmpData, pObj);
      int nLod = pObj->m_pRNTmpData->userData.nLod;
      if(m_bRenderIntoShadowmap && (pObj->GetDrawFrame(0)<(GetFrameID()-10)))
        pObj->m_pRNTmpData->userData.nLod += GetCVars()->e_ShadowsLodBiasInvis;
      pObj->m_pRNTmpData->userData.nLod += GetCVars()->e_ShadowsLodBiasFixed;
      pObj->Render(pROModifier, -1);
      pObj->m_pRNTmpData->userData.nLod = nLod;
    }
    break;

  case eERType_Brush:
    {
      CBrush * pObj = (CBrush*)pRNode;
      Get3DEngine()->CheckCreateRNTmpData(&pObj->m_pRNTmpData, pObj);
      int nLod = pObj->m_pRNTmpData->userData.nLod;
      if(m_bRenderIntoShadowmap && (pObj->GetDrawFrame(0)<(GetFrameID()-10)))
        pObj->m_pRNTmpData->userData.nLod += GetCVars()->e_ShadowsLodBiasInvis;
      pObj->m_pRNTmpData->userData.nLod += GetCVars()->e_ShadowsLodBiasFixed;
      pObj->Render(pROModifier, -1);
      pObj->m_pRNTmpData->userData.nLod = nLod;
    }
    break;

  default:
    {
      SRendParams rParams;
      
      if(pRNode->IsRenderNode())
      {
			  rParams.pRenderNode = (IRenderNode*)pRNode;
        Get3DEngine()->CheckCreateRNTmpData(&rParams.pRenderNode->m_pRNTmpData, rParams.pRenderNode);
        rParams.nLod = rParams.pRenderNode->m_pRNTmpData ? rParams.pRenderNode->m_pRNTmpData->userData.nLod : 0;
        if(m_bRenderIntoShadowmap && (rParams.pRenderNode->GetDrawFrame(0)<(GetFrameID()-10)))
          rParams.nLod += GetCVars()->e_ShadowsLodBiasInvis;
        rParams.nLod += GetCVars()->e_ShadowsLodBiasFixed;
      }

      if(pROModifier && pROModifier->nStatesInUse)
      {
        rParams.fDistance = pROModifier->fDistance;
      }

      if(pROModifier && pROModifier->nMatricesInUse)
      {
        rParams.pMatrix = &pROModifier->mat; 
        rParams.pPrevMatrix = &pROModifier->prev_mat; 
      }

      pRNode->Render(rParams);
    }
    break;
  }
}

int C3DEngine::GetGeomDetailScreenRes()
{
  return GetCVars()->e_ForceDetailLevelForScreenRes ? GetCVars()->e_ForceDetailLevelForScreenRes : GetRenderer()->GetWidth();
}

SImageSubInfo * C3DEngine::GetImageInfo(const char * pName)
{
  if(m_imageInfos.find(string(pName)) != m_imageInfos.end())
    return m_imageInfos[string(pName)];

  return NULL;
}

SImageSubInfo * C3DEngine::RegisterImageInfo(byte ** pMips, int nDim, const char * pName)
{
  if(m_imageInfos.find(string(pName)) != m_imageInfos.end())
    return m_imageInfos[string(pName)];

  assert(pMips && pMips[0]);

  SImageSubInfo * pImgSubInfo = new SImageSubInfo;

  pImgSubInfo->nDim = nDim;

  int nMipDim = pImgSubInfo->nDim;

  for(int m=0; m<SImageSubInfo::nMipsNum && nMipDim; m++)
  {
    pImgSubInfo->pImgMips[m] = new byte[nMipDim*nMipDim*4];

    memcpy(pImgSubInfo->pImgMips[m], pMips[m], nMipDim*nMipDim*4);

    nMipDim /= 2;
  }

  const string strFileName = pName;

  pImgSubInfo->nReady = 1;

  m_imageInfos[strFileName] = pImgSubInfo;

  return pImgSubInfo;
}

/*void C3DEngine::CheckUpdateImageInfos()
{
  //  m_imageInfos.clear();

  // collect textures from decal render nodes
  if(Get3DEngine()->m_pObjectsTree[nSID])
  {
    static PodArray<IRenderNode*> lstObjects; lstObjects.Clear();

    Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstObjects, eERType_Decal, NULL);
    Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstObjects, eERType_Brush, NULL);

    for(int d=0; d<lstObjects.Count(); d++)
    {
      IRenderNode * pNode = lstObjects[d];

      if(IMaterial * pMaterial = pNode->GetMaterial())
      {
        if(pMaterial->GetSubMtlCount())
          pMaterial = pMaterial->GetSubMtl(0);

        SShaderItem & sItem = pMaterial->GetShaderItem(0);

        if(SEfResTexture * pTex = sItem.m_pShaderResources->GetTexture(EFTT_DIFFUSE))
        {
          if(ITexture * pITex = pTex->m_Sampler.m_pITex)
          {
            if(byte ** pImgMips = pITex->GetSystemCopy())
            {
              if(pImgMips[0] && pITex->GetWidth() == pITex->GetHeight())
              {
                RegisterImageInfo(pImgMips, pITex->GetWidth(), pITex->GetName());
              }
            }
          }
        }
      }
    }
  }
}*/


const char * gVoxelEditOperationNames[eveoLast] =
{
  "None",
  "Soft Create",
  "Soft Substract",
  "Hard Create",
  "Hard Substract",
  "Material",
  "Soft Base Color",
  "Blur Positive",
  "Blur Negative",
  "Copy Terrain Pos",
  "Copy Terrain Neg",
  "Pick Height",
  "Integrate Pos",
  "Integrate Neg",
  "Force LOD",
  "Limit LOD",
};

const char * C3DEngine::GetVoxelEditOperationName(EVoxelEditOperation eOperation)
{
  return gVoxelEditOperationNames[eOperation];
}

void C3DEngine::SyncProcessStreamingUpdate() 
{ 
	if( m_pObjManager )
		m_pObjManager->ProcessObjectsStreaming_Finish();
}

void C3DEngine::SetScreenshotCallback(IScreenshotCallback* pCallback)
{
	m_pScreenshotCallback = pCallback;
}

#include UNIQUE_VIRTUAL_WRAPPER(IFoliage)
#include UNIQUE_VIRTUAL_WRAPPER(I3DEngine)
DEVIRTUALIZATION_VTABLE_FIX_IMPL(I3DEngine);
 
