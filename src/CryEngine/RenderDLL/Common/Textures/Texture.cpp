/*=============================================================================
  Texture.cpp : Common texture manager implementation.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include <ImageExtensionHelper.h>
#include "Image/CImage.h"
#include "IDirectBee.h"			// connection to D3D9 wrapper
#include <IFlashPlayer.h>
#include <IVideoPlayer.h>
#include "I3DEngine.h"
#include "StringUtils.h"								// stristr()

string CTexture::s_GlobalTextureFilter;
int CTexture::s_nGlobalDefState;
STexState CTexture::s_sDefState;
STexState CTexture::s_sGlobalDefState;
STexStageInfo CTexture::s_TexStages[MAX_TMU];
int CTexture::s_TexStateIDs[MAX_TMU];
int CTexture::s_CurStage;
int CTexture::s_nStreamingEnabled;
int CTexture::s_nStreamingMode;
int CTexture::s_nMaxTextureSize;
bool CTexture::s_bPrecachePhase;
volatile bool CTexture::s_bStreamingShow = false;
int CTexture::s_nStreamingThroughput = 0;
float CTexture::s_nStreamingTotalTime = 0;

std::vector<STexState> CTexture::s_TexStates;

TArray<STexGrid> CTexture::m_TGrids;

// ==============================================================================
CTexture *CTexture::s_ptexNoTexture;
CTexture *CTexture::s_ptexWhite;
CTexture *CTexture::s_ptexGray;
CTexture *CTexture::s_ptexBlack;
CTexture *CTexture::s_ptexFlatBump;
CTexture *CTexture::s_ptexMipMapDebug;
CTexture *CTexture::s_ptexColorBlue;
CTexture *CTexture::s_ptexColorCyan;
CTexture *CTexture::s_ptexColorGreen;
CTexture *CTexture::s_ptexColorPurple;
CTexture *CTexture::s_ptexColorRed;
CTexture *CTexture::s_ptexColorWhite;
CTexture *CTexture::s_ptexColorYellow;
CTexture *CTexture::s_ptexPaletteDebug;
CTexture *CTexture::s_ptexPaletteTexelsPerMeter;
CTexture *CTexture::s_ptexIconShaderCompiling;
CTexture *CTexture::s_ptexIconStreaming;
CTexture *CTexture::s_ptexIconStreamingTerrainTexture;
CTexture *CTexture::s_ptexIconNullSoundSystem;
CTexture *CTexture::s_ptexIconNullMusicSystem;
CTexture *CTexture::s_ptexMipColors_Diffuse;
CTexture *CTexture::s_ptexMipColors_Bump;
CTexture *CTexture::s_ptexScreenNoiseMap;
CTexture *CTexture::s_ptexNoiseVolumeMap;
CTexture *CTexture::s_ptexVectorNoiseVolMap;
CTexture *CTexture::s_ptexVignettingMap;
CTexture *CTexture::s_ptexFromRE[8];
CTexture *CTexture::s_ptexShadowID[8];
CTexture *CTexture::s_ptexFromRE_FromContainer[2];
CTexture *CTexture::s_ptexFromObj;
CTexture *CTexture::s_ptexRT_2D;
CTexture *CTexture::s_ptexRT_CM;
CTexture *CTexture::s_ptexFromLight;
CTexture *CTexture::s_ptexFlare;
CTexture *CTexture::s_ptexScreenMap;
CTexture *CTexture::s_ptexScreenShadowMap[MAX_REND_LIGHT_GROUPS];
CTexture *CTexture::s_ptexCurrentScreenShadowMap[MAX_REND_LIGHT_GROUPS];
CTexture *CTexture::s_ptexAOTarget;
CTexture *CTexture::s_ptexSSGITarget;
CTexture *CTexture::s_ptexSceneNormalsMap;

CTexture *CTexture::s_ptexIrrVolumeRT[] = { NULL, NULL, NULL };
CTexture *CTexture::s_ptexIrrVolumeRTDebug = NULL;
CTexture *CTexture::s_ptexIrrVolumeColorMap = NULL;
CTexture *CTexture::s_ptexIrrVolumeNormalMap = NULL;
CTexture *CTexture::s_ptexIrrVolumeDepthMap = NULL;

// Post-process related textures
CTexture *CTexture::s_ptexBackBuffer;
CTexture *CTexture::s_ptexPrevBackBuffer;
CTexture *CTexture::s_ptexPrevZTarget;
CTexture *CTexture::s_ptexBackBufferTemp;
CTexture *CTexture::s_ptexBackBufferScaled[3];
CTexture *CTexture::s_ptexEffectsAccum;
CTexture *CTexture::s_ptexGlow;

CTexture *CTexture::s_ptexCloak;

CTexture *CTexture::s_ptexWaterOcean;
CTexture *CTexture::s_ptexWaterVolumeTemp;
CTexture *CTexture::s_ptexWaterVolumeDDN;
CTexture *CTexture::s_ptexWaterPuddles[2];
CTexture *CTexture::s_ptexWaterPuddlesDDN;

CTexture *CTexture::s_ptexScatterLayer;
CTexture *CTexture::s_ptexTranslucenceLayer;

CTexture *CTexture::s_ptexRT_ShadowPool;
CTexture *CTexture::s_ptexRT_NULL;
CTexture *CTexture::s_ptexCloudsLM;

CTexture *CTexture::s_ptexLightInfo[4];
CTexture *CTexture::s_ptexSceneTarget;
CTexture *CTexture::s_ptexCurrSceneTarget;
CTexture *CTexture::s_ptexCurrentSceneDiffuseAccMap;
CTexture *CTexture::s_ptexSceneDiffuseAccMap;
CTexture *CTexture::s_ptexSceneSpecularAccMap;
CTexture *CTexture::s_ptexSceneTexturesMap;
CTexture *CTexture::s_ptexZTarget;
CTexture *CTexture::s_ptexZOcclusion[2];
CTexture *CTexture::s_ptexZTargetMS;
CTexture *CTexture::s_ptexZTargetScaled;
CTexture *CTexture::s_ptexHDRTarget;
CTexture *CTexture::s_ptexHDRTargetEncoded;
CTexture *CTexture::s_ptexHDRTargetScaled[3];
CTexture *CTexture::s_ptexHDRBrightPass[2];
CTexture *CTexture::s_ptexHDRAdaptedLuminanceCur[8];
int CTexture::s_nCurLumTextureIndex;
CTexture *CTexture::s_ptexCurLumTexture;
CTexture *CTexture::s_ptexCurLumTextureSys;
CTexture *CTexture::s_ptexHDRToneMaps[NUM_HDR_TONEMAP_TEXTURES];
CTexture *CTexture::s_ptexSkyDomeMie;
CTexture *CTexture::s_ptexSkyDomeRayleigh;
CTexture *CTexture::s_ptexSkyDomeMoon;
CTexture *CTexture::s_ptexText_FromSF[NUM_SCALEFORM_TEXTURES];
CTexture *CTexture::s_ptexVolObj_Density;
CTexture *CTexture::s_ptexVolObj_Shadow;
CTexture *CTexture::s_ptexColorChart;



CTexture *CTexture::s_ptexStereoL;
CTexture *CTexture::s_ptexStereoR;

SEnvTexture *CTexture::s_pCurEnvTexture;

SEnvTexture CTexture::s_EnvCMaps[MAX_ENVCUBEMAPS];
SEnvTexture CTexture::s_EnvTexts[MAX_ENVTEXTURES];

TArray<SEnvTexture> CTexture::s_CustomRT_CM;
TArray<SEnvTexture> CTexture::s_CustomRT_2D;

TArray<STexPool *> CTexture::s_TexturesPools;
STexPoolItem CTexture::s_FreeTexPoolItems;

CTexture CTexture::s_ShaderTemplates[EFTT_MAX];

CTexture *CTexture::s_pBackBuffer;
CTexture CTexture::s_FrontBufferTextures[2];

int CTexture::s_nCurTexResolutionReduction;
int CTexture::s_nCurTexBumpResolutionReduction;

ETEX_Format CTexture::s_eTFZ = eTF_R32F; 

//============================================================

CTexture::~CTexture()
{
	IReadStreamPtr task;
	while(!m_TextureStreamTasks.empty())
	{
		if(m_TextureStreamTasks.try_pop(task) && task)
			task->Wait();
	}
  if (gRenDev && gRenDev->m_pRT)
    gRenDev->m_pRT->RC_ReleaseDeviceTexture(this);

#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
  if(m_pImageSystemCopy)
    for(int nMip=0; nMip<m_nImageSystemCopyMipsNum; nMip++)
      SAFE_DELETE_ARRAY(m_pImageSystemCopy[nMip]);
  SAFE_DELETE_ARRAY(m_pImageSystemCopy);
#endif
}

void CTexture::RT_ReleaseDevice()
{
  ReleaseDeviceTexture(false);

  uint32 i;
  if (m_bStreamRequested)
  {
    for (i=0; i<s_StreamingRequested.Num(); i++)
    {
      if (s_StreamingRequested[i] == this)
      {
        s_StreamingRequested[i] = NULL;
        break;
      }
    }
  }
}

void CTexture::AddDirtRect(RECT& rc)
{
  uint32 i;
  for (i=0; i<m_DirtyRects.size(); i++)
  {
    RECT& rcT = m_DirtyRects[i];
    if (rc.left == rcT.left && rc.right == rcT.right && rc.top == rcT.top && rc.bottom == rcT.bottom)
      break;
  }
  if (i != m_DirtyRects.size())
    return;
  m_DirtyRects.push_back(rc);
}

const CCryName& CTexture::mfGetClassName()
{
  return s_sClassName;
}

CCryName CTexture::GenName(const char *name, uint32 nFlags, float fAmount1, float fAmount2)
{
  string strName = name;
	strName.MakeLower();

  if (!(nFlags & FT_DONT_GENNAME))
	{




	  string Name;
 
		strName += Name.Format("(%x)", nFlags & FT_AFFECT_INSTANCE);
	}

  return CCryName(strName.c_str());
}

CTexture *CTexture::GetByID(int nID)
{
  CTexture *pTex = NULL;

  const CCryName& className = mfGetClassName();
  CBaseResource *pBR = CBaseResource::GetResource(className, nID, false);
  if (!pBR)
    return s_ptexNoTexture;
  pTex = (CTexture *)pBR;
  return pTex;
}

CTexture *CTexture::GetByName(const char *szName)
{
  CTexture *pTex = NULL;

  CCryName Name = GenName(szName, FT_DONT_GENNAME);

  CBaseResource *pBR = CBaseResource::GetResource(mfGetClassName(), Name, false);
  if (!pBR)
    return NULL;
  pTex = (CTexture *)pBR;
  return pTex;
}

CTexture *CTexture::NewTexture(const char *name, uint32 nFlags, ETEX_Format eTFDst, bool& bFound, float fAmount1, float fAmount2, int8 nPriority)
{
  CTexture *pTex = NULL;

  CCryName Name = GenName(name, nFlags, fAmount1, fAmount2);

  CBaseResource *pBR = CBaseResource::GetResource(mfGetClassName(), Name, false);
  if (!pBR)
  {
    pTex = new CTexture;
    pTex->Register(mfGetClassName(), Name);
    bFound = false;
    pTex->m_nFlags = nFlags;
    pTex->m_eTFDst = eTFDst;
    pTex->m_nPriority = __max(nPriority, pTex->m_nPriority);
  }
  else
  {
    pTex = (CTexture *)pBR;
    pTex->AddRef();
    bFound = true;
    pTex->m_nPriority = __max(nPriority, pTex->m_nPriority);
  }

  return pTex;
}

void CTexture::PostCreate()
{
  m_nSize = CTexture::TextureDataSize(m_nWidth, m_nHeight, m_nDepth, m_nMips, m_eTFDst);

  if (m_eTT == eTT_Cube)
    m_nSize *= 6;

#if defined (DIRECT3D10)
  m_nSize *= m_nArraySize;
#endif
}

CTexture *CTexture::CreateTextureObject(const char *name, uint32 nWidth, uint32 nHeight, int nDepth, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID, int8 nPriority)
{
  bool bFound = false;

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Texture, 0, "%s", name);

  CTexture *pTex = NewTexture(name, nFlags, eTF, bFound, -1, -1, nPriority);
  if (bFound)
  {
    if (!pTex->m_nWidth)
    {
      pTex->m_nWidth = nWidth;
      pTex->m_nSrcWidth = nWidth;
    }
    if (!pTex->m_nHeight)
    {
      pTex->m_nHeight = nHeight;
      pTex->m_nSrcHeight = nHeight;
    }
		pTex->m_nFlags |= nFlags & (FT_DONT_RELEASE | FT_USAGE_RENDERTARGET);

    return pTex;
  }
  pTex->m_nDepth = nDepth;
  pTex->m_nWidth = nWidth;
  pTex->m_nHeight = nHeight;
  pTex->m_nSrcWidth = nWidth;
  pTex->m_nSrcHeight = nHeight;
  pTex->m_eTT = eTT;
  pTex->m_eTFDst = eTF;
  pTex->m_nCustomID = nCustomID;
  pTex->m_SrcName = name;

  return pTex;
}

void CTexture::GetMemoryUsage( ICrySizer *pSizer ) const
{ 








	pSizer->Add( *this );
	pSizer->AddObject( m_SrcName );
	
	if (m_pFileTexMips)
	{
		pSizer->AddObject((char*)m_pFileTexMips-4, (sizeof(*m_pFileTexMips)*m_nMips) + 4);
		for (int i=0; i<m_nMips; i++)
		{
			m_pFileTexMips[i].GetMemoryUsage(pSizer);
		}
	}
	
#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT
	if(m_pImageSystemCopy && m_pImageSystemCopy[0])
	{
		SIZER_COMPONENT_NAME(pSizer, "System copy");
		pSizer->AddObject((void *)m_pImageSystemCopy,(m_nWidth*m_nHeight*4)*4/3);
	}
#endif




}


CTexture *CTexture::CreateTextureArray(const char *name, uint32 nWidth, uint32 nHeight, uint32 nArraySize, uint32 nFlags, ETEX_Format eTF, int nCustomID, int8 nPriority)
{
#if defined (DIRECT3D10)
  CTexture *pTex = CreateTextureObject(name, nWidth, nHeight, 1, eTT_2D, nFlags , eTF, nCustomID, nPriority);
  pTex->m_nWidth = nWidth;
  pTex->m_nHeight = nHeight;
  pTex->m_nArraySize = nArraySize;

#ifdef WIN32
  if(CRenderer::m_pDirectBee)
    CRenderer::m_pDirectBee->PushName(name);
#endif

  //FIX: replace CreateRenderTarget call by Create2DTexture
  bool bRes = pTex->CreateRenderTarget(eTF);
  if (!bRes)
    pTex->m_nFlags |= FT_FAILED;
  pTex->PostCreate();

  // 	through NVAPI we tell driver not to sync
  if(bRes)
    if(nFlags&FT_DONTSYNCMULTIGPU)
    {
      gRenDev->Hint_DontSync(*pTex);	
    }

#ifdef WIN32
    if(CRenderer::m_pDirectBee)
      CRenderer::m_pDirectBee->PushName();
#endif

    return pTex;
#else
    return NULL;
#endif
}

CTexture* CTexture::CreateRenderTarget(const char *name, uint32 nWidth, uint32 nHeight, ETEX_Type eTT, uint32 nFlags, ETEX_Format eTF, int nCustomID, int8 nPriority, bool bHiQuality)
{
  CTexture *pTex = CreateTextureObject(name, nWidth, nHeight, 1, eTT, nFlags | FT_USAGE_RENDERTARGET, eTF, nCustomID, nPriority);
	pTex->m_nWidth = nWidth;
	pTex->m_nHeight = nHeight;
	if( bHiQuality )
		pTex->SetHighQualityRT();

#ifdef WIN32
	if(CRenderer::m_pDirectBee)
		CRenderer::m_pDirectBee->PushName(name);
#endif

	bool bRes = pTex->CreateRenderTarget(eTF);
  if (!bRes)
    pTex->m_nFlags |= FT_FAILED;
  pTex->PostCreate();

	// 	through NVAPI we tell driver not to sync
	if(bRes)
	if(nFlags&FT_DONTSYNCMULTIGPU)
	{
		gRenDev->Hint_DontSync(*pTex);	
	}

#ifdef WIN32
	if(CRenderer::m_pDirectBee)
		CRenderer::m_pDirectBee->PushName();
#endif

  return pTex;
}

bool CTexture::Create2DTexture(int nWidth, int nHeight, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst)
{
  if (nMips <= 0)
    nMips = CTexture::CalcNumMips(nWidth, nHeight);
  m_eTFSrc = eTFSrc;
  m_nMips = nMips;

  STexData Data[6];
  Data[0].m_eTF = eTFSrc;
  Data[0].m_nDepth = 1;
  Data[0].m_nWidth = nWidth;
  Data[0].m_nHeight = nHeight;
  Data[0].m_nMips = 1;
  Data[0].m_nSize = CTexture::TextureDataSize(nWidth, nHeight, 1, 1, eTFSrc);
  Data[0].m_pData = pData;







  bool bRes = CreateTexture(Data);
  if (!bRes)
    m_nFlags |= FT_FAILED;

  PostCreate();

  return bRes;
}

CTexture *CTexture::Create2DTexture(const char *szName, int nWidth, int nHeight, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst, int8 nPriority, bool bAsyncDevTexCreation)
{
  FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

#ifdef WIN32
	if(CRenderer::m_pDirectBee)
		CRenderer::m_pDirectBee->PushName(szName);
#endif

  CTexture *pTex = CreateTextureObject(szName, nWidth, nHeight, 1, eTT_2D, nFlags, eTFDst, -1, nPriority);
  pTex->m_bAsyncDevTexCreation = bAsyncDevTexCreation;
  bool bFound = false;

  pTex->Create2DTexture(nWidth, nHeight, nMips, nFlags, pData, eTFSrc, eTFDst);

#ifdef WIN32
	if(CRenderer::m_pDirectBee)
		CRenderer::m_pDirectBee->PushName();
#endif

	return pTex;
}

bool CTexture::Create3DTexture(int nWidth, int nHeight, int nDepth, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst)
{
  //if (nMips <= 0)
  //  nMips = CTexture::CalcNumMips(nWidth, nHeight);
  m_eTFSrc = eTFSrc;
  m_nMips = nMips;

  STexData Data[6];
  Data[0].m_eTF = eTFSrc;
  Data[0].m_nWidth = nWidth;
  Data[0].m_nHeight = nHeight;
	Data[0].m_nDepth = nDepth;
  Data[0].m_nMips = 1;
  Data[0].m_nSize = CTexture::TextureDataSize(nWidth, nHeight, nDepth, 1, eTFSrc);
  Data[0].m_pData = pData;

  bool bRes = CreateTexture(Data);
  if (!bRes)
    m_nFlags |= FT_FAILED;

  PostCreate();

  return bRes;
}

CTexture *CTexture::Create3DTexture(const char *szName, int nWidth, int nHeight, int nDepth, int nMips, int nFlags, byte *pData, ETEX_Format eTFSrc, ETEX_Format eTFDst, int8 nPriority)
{
#ifdef WIN32
	if(CRenderer::m_pDirectBee)
		CRenderer::m_pDirectBee->PushName(szName);
#endif

  CTexture *pTex = CreateTextureObject(szName, nWidth, nHeight, nDepth, eTT_3D, nFlags, eTFDst, -1, nPriority);
  bool bFound = false;

  pTex->Create3DTexture(nWidth, nHeight, nDepth, nMips, nFlags, pData, eTFSrc, eTFDst);

#ifdef WIN32
	if(CRenderer::m_pDirectBee)
		CRenderer::m_pDirectBee->PushName();
#endif

	return pTex;
}

bool CTexture::Reload()
{
  byte *pData[6];
  int i;
  bool bOK = false;

  if (IsStreamed())
  {
    ReleaseDeviceTexture(false);
    return ToggleStreaming(true);
  }

  for (i=0; i<6; i++)
  {
    pData[i] = 0;
  }
  if (m_nFlags & FT_FROMIMAGE)
  {
    assert(!(m_nFlags & FT_USAGE_RENDERTARGET));
    bOK = LoadFromImage(m_SrcName.c_str(), true, eTF_Unknown);		// true=reloading
    if (!bOK)
      SetNoTexture();
  }
  else
  if (m_nFlags & (FT_USAGE_RENDERTARGET|FT_USAGE_DYNAMIC))
  {
    bOK = CreateDeviceTexture(pData);
    assert(bOK);
  }
  PostCreate();

  return bOK;
}

CTexture *CTexture::ForName(const char *name, uint32 nFlags, ETEX_Format eTFDst, float fAmount1, float fAmount2, int8 nPriority)
{
  bool bFound = false;

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Texture, 0, "%s", name);








  CTexture *pTex = NewTexture(name, nFlags, eTFDst, bFound, fAmount1, fAmount2, nPriority);
  if (bFound || name[0] == '$')
	{
		if(!bFound)
			pTex->m_SrcName = name;
		else
			// switch off streaming for the same texture with the same flags except DONT_STREAM
			if((nFlags & FT_DONT_STREAM) != 0 && (pTex->GetFlags()& FT_DONT_STREAM) == 0)
			{
				pTex->ReleaseDeviceTexture(false);
				pTex->m_nFlags |= FT_DONT_STREAM;
				pTex->Reload();
			}

    return pTex;
	}
  pTex->m_SrcName = name;

  if (CTexture::s_bPrecachePhase && !(nFlags & FT_DONT_STREAM))
  {
    pTex->m_eTFDst = eTFDst;
    if (nFlags & FT_TEX_NORMAL_MAP)
      nFlags |= FT_HAS_ATTACHED_ALPHA;
    pTex->m_nFlags = nFlags;
    pTex->m_bStreamed = true;
    pTex->m_bWasUnloaded = true;
  }
  else
    pTex->Load(eTFDst, fAmount1, fAmount2);

  return pTex;
}

struct CompareTextures
{
	bool operator()(const CTexture *a, const CTexture *b)
	{
		return (stricmp(a->GetSourceName(), b->GetSourceName()) < 0);
	}
};

void CTexture::Precache()
{ 
  LOADING_TIME_PROFILE_SECTION(iSystem);

  if (!s_bPrecachePhase)
    return;
  if (!gRenDev)
    return;

  gRenDev->m_pRT->RC_PreloadTextures();
}
    
void CTexture::RT_Precache()
{
  if (gRenDev->CheckDeviceLost())
    return;

  CTimeValue t0 = gEnv->pTimer->GetAsyncTime();
  iLog->Log("-- Precaching textures...");
  iLog->UpdateLoadingScreen(0);

  DynArray<CTexture *> Textures;

  SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
  if (pRL)
  {
    ResourcesMapItor itor;
    for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
    {
      CTexture *tp = (CTexture *)itor->second;
      if (!tp)
        continue;
      if (tp->IsStreamed() && (tp->IsUnloaded() || tp->IsPartiallyLoaded()))
			{
				tp->Relink(&CTexture::s_Root);
				Textures.push_back(tp);
			}
    }
  }

  std::sort(Textures.begin(), Textures.end(), CompareTextures());

  for (int i=0; i<Textures.size(); i++)
  {
    CTexture *tp = Textures[i];

		if (!s_nStreamingEnabled || !tp->m_bStreamPrepared)
			tp->Load(tp->m_eTFDst, -1, -1);
		if (s_nStreamingEnabled && tp->m_bStreamPrepared)
			tp->StreamRestore();

    if((i&15)==0)
      iLog->UpdateLoadingScreen(0);
  }

  CTimeValue t1 = gEnv->pTimer->GetAsyncTime();
  float dt = (t1-t0).GetSeconds();
  CryLog( "Precaching textures done in %.2f seconds",dt );

  s_bPrecachePhase = false;
}

bool CTexture::Load(ETEX_Format eTFDst, float fAmount1, float fAmount2)
{
  m_bWasUnloaded = false;
  m_bStreamed = false;

  bool bFound = LoadFromImage(m_SrcName.c_str(), false, eTFDst, fAmount1, fAmount2);		// false=not reloading
  if (!bFound)
    SetNoTexture();

  m_nFlags |= FT_FROMIMAGE;
  PostCreate();

  return bFound;
}

bool CTexture::ToggleStreaming(int bOn)
{
  if (!(m_nFlags & (FT_FROMIMAGE | FT_DONT_RELEASE)))
    return false;
  if (m_nFlags & FT_DONT_STREAM)
    return false;
	AbortStreamingTasks(this);
	if (bOn)
  {
    if (IsStreamed())
      return true;
    ReleaseDeviceTexture(false);
    m_bStreamed = true;
    if (StreamPrepare(true, m_eTFDst))
      return true;
    SAFE_DELETE_ARRAY(m_pFileTexMips);
    m_bStreamed = false;
    if (m_bNoTexture)
      return true;
  }
  ReleaseDeviceTexture(false);
  return Reload();
}

bool CTexture::LoadFromImage(const char *name, const bool bReload, ETEX_Format eTFDst, float fAmount1, float fAmount2)
{
	LOADING_TIME_PROFILE_SECTION(iSystem);

  string nm[6];
  CImageFile* im[6];
  uint32 i;
  uint32 nT = 1;

  if (CRenderer::CV_r_texnoload)
  {
    if (SetNoTexture())
      return true;
  }
	m_eTFDst = eTFDst;

  nm[0] = name;
  nm[0].MakeLower();

  STexData Data[6];
  string sNew = nm[0].SpanExcluding("+");
  if (sNew.size() != nm[0].size())
  {
    string second = string(&nm[0].c_str()[sNew.size()+1]);
    nm[0] = sNew;
    nm[1] = second;
    nT = 2;
  }
  for (i=0; i<6; i++)
  {
    im[i] = NULL;
  }

  if (nT == 1 && (s_nStreamingEnabled & 1) && !(m_nFlags & FT_DONT_STREAM) && !(m_eTT == eTT_3D || m_eTT == eTT_AutoCube))
  {
    m_bStreamed = true;
    if (StreamPrepare(bReload, eTFDst))
		{
			assert(m_pDevTexture);
      return true;
		}
		m_nFlags |= FT_DONT_STREAM;
    m_bStreamed = false;
    if (m_bNoTexture)
		{
			Unlink();
      return true;
		}
  }

  for (i=0; i<nT; i++)
  {
    im[i] = CImageFile::mfLoad_file(nm[i].c_str(),bReload, (m_nFlags & FT_ALPHA) ? FIM_ALPHA : 0);
    if (!im[i] || im[i]->mfGetFormat() == eTF_Unknown)
    {
      SAFE_DELETE(im[i]);
      continue;
    }
    if (m_nFlags & FT_ALPHA)
    {
      if (!im[i]->mfIs_image(0))
      {
        assert(!m_pDevTexture);
        m_pDevTexture = s_ptexWhite->m_pDevTexture;
#if defined (DIRECT3D10)
        m_pDeviceShaderResource = s_ptexWhite->m_pDeviceShaderResource;
#endif
        m_nDefState = s_ptexWhite->m_nDefState;
        SAFE_DELETE(im[i]);
        m_nFlags = s_ptexWhite->GetFlags();
        m_eTFDst = s_ptexWhite->GetDstFormat();
        m_nMips = s_ptexWhite->GetNumMips();
        m_nWidth = s_ptexWhite->GetWidth();
        m_nHeight = s_ptexWhite->GetHeight();
        m_nDepth = 1;
        m_nDefState = s_ptexWhite->m_nDefState;
        m_bNoTexture = true;
				Unlink();
				continue;
      }
    }
		if(im[i]->mfGet_Flags() & FIM_SPLITTED)							// propagate splitted file flag
			m_nFlags |= FT_SPLITTED;
		if(im[i]->mfGet_Flags() & FIM_X360_NOT_PRETILED)
			m_nFlags |= FT_TEX_WAS_NOT_PRE_TILED;
		if (im[i]->mfGet_Flags() & FIM_FILESINGLE)		// propagate flag from image to texture
			m_nFlags|=FT_FILESINGLE;
    if (im[i]->mfGet_Flags() & FIM_NORMALMAP)
    {
			if (!(m_nFlags & FT_TEX_NORMAL_MAP) && name && !CryStringUtils::stristr(name, "_ddn"))
			{
				// becomes reported as editor error
				gEnv->pSystem->Warning( VALIDATOR_MODULE_RENDERER,VALIDATOR_WARNING,VALIDATOR_FLAG_FILE|VALIDATOR_FLAG_TEXTURE,
					name,"Not a normal map texture tried to be used as a normal map: %s",name );
			}
    }



		ETEX_Format	eFormat	=	eTF_3DC;

		if(!(m_nFlags & FT_ALPHA) && !(im[i]->mfGetSrcFormat()==eFormat






			) && CryStringUtils::stristr(name,"_ddn")!=0 && CryStringUtils::stristr(name,"_ddndif")==0)		// improvable code
		{
			// becomes reported as editor error
			gEnv->pSystem->Warning( VALIDATOR_MODULE_RENDERER,VALIDATOR_WARNING,VALIDATOR_FLAG_FILE|VALIDATOR_FLAG_TEXTURE,
				name,"Non normal map used (wrong on DX11 and PS3): %s",name );
		}

    if (im[i]->mfGet_Flags() & FIM_NOTSUPPORTS_MIPS)
    {
      if (!(m_nFlags & FT_NOMIPS))
        m_nFlags |= FT_FORCE_MIPS;
    }
    if (im[i]->mfGet_Flags() & FIM_ALPHA)
      m_nFlags |= FT_HAS_ATTACHED_ALPHA;			// if the image has alpha attached we store this in the CTexture
    Data[i].m_nFlags = im[i]->mfGet_Flags();
    Data[i].m_pData = im[i]->mfGet_image(0);
    Data[i].m_nWidth = im[i]->mfGet_width();
    Data[i].m_nHeight = im[i]->mfGet_height();
    Data[i].m_nDepth = im[i]->mfGet_depth();
    Data[i].m_eTF = im[i]->mfGetFormat();
    Data[i].m_nMips = im[i]->mfGet_numMips();
    Data[i].m_fAmount = (i==0) ? fAmount1 : fAmount2;
    Data[i].m_AvgColor = im[i]->mfGet_AvgColor();
    if ((m_nFlags & FT_NOMIPS) || Data[i].m_nMips <= 0)
      Data[i].m_nMips = 1;
    Data[i].m_nSize = CTexture::TextureDataSize(Data[i].m_nWidth, Data[i].m_nHeight, Data[i].m_nDepth, Data[i].m_nMips, Data[i].m_eTF);
    Data[i].m_FileName = im[i]->mfGet_filename();
    if (!i)
    {
      if (im[i]->mfIs_image(1))
      {
        //return false;

        for (i=1; i<6; i++)
        {
          Data[i].m_nFlags = im[0]->mfGet_Flags();
          Data[i].m_pData = im[0]->mfGet_image(i);
          Data[i].m_nWidth = im[0]->mfGet_width();
          Data[i].m_nHeight = im[0]->mfGet_height();
          Data[i].m_nDepth = im[0]->mfGet_depth();
          Data[i].m_eTF = im[0]->mfGetFormat();
          Data[i].m_nMips = im[0]->mfGet_numMips();
          Data[i].m_AvgColor = im[0]->mfGet_AvgColor();
          if ((m_nFlags & FT_NOMIPS) || Data[i].m_nMips <= 0)
            Data[i].m_nMips = 1;
          Data[i].m_nSize = CTexture::TextureDataSize(Data[i].m_nWidth, Data[i].m_nHeight, Data[i].m_nDepth, Data[i].m_nMips, Data[i].m_eTF);
          Data[i].m_FileName = im[0]->mfGet_filename();
        }
        break;
      }
    }
  }

  bool bRes = true;

  if (Data[0].m_pData && Data[1].m_pData && !Data[5].m_pData)
  {
    Exchange(Data[0], Data[1]);
  }

  if (im[0])
  {
#ifdef WIN32
		if(CRenderer::m_pDirectBee)
			CRenderer::m_pDirectBee->PushName(PathUtil::GetFile(name));
#endif
		bRes = CreateTexture(Data);
#ifdef WIN32
		if(CRenderer::m_pDirectBee)
			CRenderer::m_pDirectBee->PushName();
#endif
  }
  else
    bRes = false;

  for (i=0; i<nT; i++)
  {
    SAFE_DELETE (im[i]);
    if (Data[i].m_pData && Data[i].m_bReallocated)
    {
      SAFE_DELETE_ARRAY(Data[i].m_pData);
    }
  }
	m_fCurrentMipBias = 0.f;

  return bRes;
}

bool CTexture::CreateTexture(STexData Data[6])
{
  m_nSrcWidth = Data[0].m_nWidth;
  m_nSrcHeight = Data[0].m_nHeight;
  m_nSrcDepth = Data[0].m_nDepth;
  m_eTFSrc = Data[0].m_eTF;
	m_nMips = Data[0].m_nMips;
  m_AvgColor = Data[0].m_AvgColor;
	m_bUseDecalBorderCol = (Data[0].m_nFlags & FIM_DECAL) != 0;
	m_bIsSRGB = (Data[0].m_nFlags & FIM_SRGB_READ) != 0;

	// On xenon bilinear filtering for FP formats requires expand
	if( gRenDev->m_bDeviceSupportsFP16Filter && (m_nFlags & FT_FILTER_LINEAR) )
		m_bHighQualityRT = true;

	assert(m_nSrcWidth && m_nSrcHeight && m_nMips);
  
  if (Data[2].m_pData || (m_nFlags & FT_REPLICATE_TO_ALL_SIDES))
    m_eTT = eTT_Cube;
  else
  if (m_nSrcDepth > 1)
  {
    m_eTT = eTT_3D;
    m_nFlags |= FT_DONT_RESIZE;
  }
  else
    m_eTT = eTT_2D;

  if (m_eTFDst == eTF_Unknown)
    m_eTFDst = m_eTFSrc;

  ImagePreprocessing(Data);
	assert(m_nWidth && m_nHeight && m_nMips);

  if (s_nMaxTextureSize > 0)
  {
    if (m_nWidth > s_nMaxTextureSize || m_nHeight > s_nMaxTextureSize)
      return false;
  }

  byte *pData[6];
  for (uint32 i=0; i<6; i++)
  {
    pData[i] = Data[i].m_pData;
  }

  bool bRes = CreateDeviceTexture(pData);

  return bRes;
}


int CTexture::GenerateMipsForNormalmap(TArray<Vec4>* Normals, int nWidth, int nHeight)
{
  int nMips = CTexture::CalcNumMips(nWidth, nHeight);
  int i, j, l;

  // Generate mips
  int reswp = nWidth;
  int reshp = nHeight;
  for (l=1; l<nMips; l++)
  {
    int resw = nWidth  >> l;
    int resh = nHeight >> l;
    if (!resw)
      resw = 1;
    if (!resh)
      resh = 1;
    Normals[l].Grow(resw*resh);
    Vec4 *curr = &Normals[l][0];
    Vec4 *prev = &Normals[l-1][0];
    int wmul = (reswp == 1) ? 1 : 2;
    int hmul = (reshp == 1) ? 1 : 2;
    for (j=0; j<resh; j++)
    {
      for (i=0; i<resw; i++)
      {
        Vec4 avg;
        if (wmul == 1)
        {
          avg = prev[wmul*i+hmul*j*reswp] +
            prev[wmul*i+(hmul*j+1)*reswp];
          avg.w /= 2.0f;
        }
        else
          if (hmul == 1)
          {
            avg = prev[wmul*i+hmul*j*reswp] +
              prev[wmul*i+1+hmul*j*reswp];
            avg.w /= 2.0f;
          }
          else
          {
            avg = prev[wmul*i+hmul*j*reswp] +
              prev[wmul*i+1+hmul*j*reswp] +
              prev[wmul*i+1+(hmul*j+1)*reswp] +
              prev[wmul*i+(hmul*j+1)*reswp];
            avg.w /= 4.0f;
          }
          float fInvDist = 1.0f / sqrtf(avg.x*avg.x + avg.y*avg.y + avg.z*avg.z);
          avg.x *= fInvDist;
          avg.y *= fInvDist;
          avg.z *= fInvDist;
          *curr = avg;
          curr++;
      }
    }
    reswp = resw;
    reshp = resh;
  }
  return nMips;
}

void CTexture::ConvertFloatNormalMap_To_ARGB8(TArray<Vec4>* Normals, int nWidth, int nHeight, int nMips, byte *pDst)
{
  int i, l;

  int n = 0;
  if (nMips <= 0)
    nMips = 1;
  if (m_eTFDst==eTF_V8U8)
  {
    // Signed normal map
    for (l=0; l<nMips; l++)
    {
      int resw = nWidth  >> l;
      int resh = nHeight >> l;
      if (!resw)
        resw = 1;
      if (!resh)
        resh = 1;
      for (i=0; i<resw*resh; i++)
      {
        Vec4 vN = Normals[l][i];
        pDst[n*2+0] = (byte)(vN.x * 127.5f);
        pDst[n*2+1] = (byte)(vN.y * 127.5f);
        n++;
      }
    }
  }
  else
  {
    // Unsigned normal map
    for (l=0; l<nMips; l++)
    {
      int resw = nWidth  >> l;
      int resh = nHeight >> l;
      if (!resw)
        resw = 1;
      if (!resh)
        resh = 1;
      for (i=0; i<resw*resh; i++)
      {
        Vec4 vN = Normals[l][i];
#ifndef XENON
#if defined (DIRECT3D10)
        pDst[n*4+0] = (byte)(vN.x * 127.0f + 128.0f);
        pDst[n*4+1] = (byte)(vN.y * 127.0f + 128.0f);
        pDst[n*4+2] = (byte)(vN.z * 127.0f + 128.0f);
#else
        pDst[n*4+2] = (byte)(vN.x * 127.0f + 128.0f);
        pDst[n*4+1] = (byte)(vN.y * 127.0f + 128.0f);
        pDst[n*4+0] = (byte)(vN.z * 127.0f + 128.0f);
#endif
        pDst[n*4+3] = (byte)vN.w;





#endif
        n++;
      }
    }
  }
}



// 1. Resize the texture data if resolution is not supported by the hardware or needed by the spec requirement
// 2. Convert the texture if Dst format is not supported by the hardware or needed by the spec requirement
void CTexture::ImagePreprocessing(STexData Data[6])
{


	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

	bool bDoProcessing = true;



	if(m_nFlags &FT_TEX_FONT)
		bDoProcessing = false;				// we generate the font in native format

	if(bDoProcessing)
	for (int i=0; i<6; i++)
  {
    STexData *pData = &Data[i];
    if (i >= 1 && !Data[5].m_pData)
      continue;
    if (!pData->m_pData)
      continue;
    ETEX_Format eTF = pData->m_eTF;
    int nMips = pData->m_nMips;
    int nSrcWidth = pData->m_nWidth;
    int nSrcHeight = pData->m_nHeight;
    m_nSrcWidth = nSrcWidth;
    m_nSrcHeight = nSrcHeight;
    int nNewWidth = nSrcWidth;
    int nNewHeight = nSrcHeight;
    bool bPowerOfTwo = true;
    int nNWidth = 1<<LogBaseTwo(nSrcWidth);
    int nNHeight = 1<<LogBaseTwo(nSrcHeight);
    bPowerOfTwo = ((nNWidth == nSrcWidth) && (nNHeight == nSrcHeight));
    if (!(s_nStreamingEnabled & 1) && !(m_nFlags & FT_DONT_RESIZE))
    {
      int minSize = max(CRenderer::CV_r_texminsize, 16);
      if (m_nFlags & FT_TEX_NORMAL_MAP)
      {
        if (CRenderer::CV_r_texbumpresolution > 0)
        {
          if (nNewWidth >= minSize || nNewHeight >= minSize)
          {
            int nRes = min(CRenderer::CV_r_texbumpresolution, 4);
            nNWidth = max(nNewWidth>>nRes, 1);
            nNHeight = max(nNewHeight>>nRes, 1);
						if (!DDSFormats::IsNormalMap(m_eTFDst) || nMips>1)
						{
							if (!DDSFormats::IsNormalMap(m_eTFDst) || (nNWidth>=4 && nNHeight>=4))
							{
								nNewWidth  = nNWidth;
								nNewHeight = nNHeight;
							}
						}
          }
        }
      }
      else
      {
        if (CRenderer::CV_r_texresolution > 0)
        {
          if (nNewWidth >= minSize || nNewHeight >= minSize)
          {
            int nRes = min(CRenderer::CV_r_texresolution, 4);
            nNewWidth = max(nNewWidth>>nRes, 1);
            nNewHeight = max(nNewHeight>>nRes, 1);
          }
        }
      }
      // User limitation check
      if (CRenderer::CV_r_texmaxsize > 1)
      {
        CRenderer::CV_r_texmaxsize = 1<<LogBaseTwo(CRenderer::CV_r_texmaxsize);

        nNewWidth = min(nNewWidth, CRenderer::CV_r_texmaxsize);
        nNewHeight = min(nNewHeight, CRenderer::CV_r_texmaxsize);
      }
    }

    // Hardware limitation check
    int nMaxSize;
    if ((nMaxSize=GetMaxTextureSize()) > 0)
    {
      nNewWidth = min(nNewWidth, nMaxSize);
      nNewHeight = min(nNewHeight, nMaxSize);
    }
    nNewWidth = max(nNewWidth, 1);
    nNewHeight = max(nNewHeight, 1);

    if (!bPowerOfTwo)
    {
      if (IsDXTCompressed(eTF))
        TextureWarning(pData->m_FileName.c_str(),"Error: CTexMan::ImagePreprocessing: ImagePreprocessing of non-power-of-two compressed texture name='%s' file='%s' (%dx%d)", m_SrcName.c_str(), pData->m_FileName.c_str(), nSrcWidth, nSrcHeight);
      else
 //Diesel cut TextureWarning     
 TextureWarning(pData->m_FileName.c_str(),"Warning: CTexMan::ImagePreprocessing: ImagePreprocessing of non-power-of-two texture name='%s' file='%s' (%dx%d)", m_SrcName.c_str(), pData->m_FileName.c_str(), nSrcWidth, nSrcHeight);
			m_nWidth = Data[0].m_nWidth;
			m_nHeight = Data[0].m_nHeight;
			m_nDepth = Data[0].m_nDepth;
			m_nMips = Data[0].m_nMips;
			return;
    }
    if (nNewWidth != nSrcWidth || nNewHeight != nSrcHeight)
    {
      if (IsDXTCompressed(eTF))
      {
        bool bResample = false;
        if (bPowerOfTwo)
        {
          if (nMips > 1)
          {
            int nLodDW = 0;
            int nLodDH = 0;
            int nOffs = 0;
            int blockSize = (eTF == eTF_DXT1 || eTF == eTF_CTX1) ? 8 : 16;
            int wdt = nSrcWidth;
            int hgt = nSrcHeight;
            int n = 0;
            while (wdt || hgt)
            {
              if (wdt <= 4 || hgt <= 4)
                break;
              if (!wdt)
                wdt = 1;
              if (!hgt)
                hgt = 1;
              if (wdt == nNewWidth)
                nLodDW = n;
              if (hgt == nNewHeight)
                nLodDH = n;
              if (nLodDH && nLodDW)
                break;
              nOffs += ((wdt+3)/4)*((hgt+3)/4)*blockSize;
              wdt >>= 1;
              hgt >>= 1;
              n++;
            }
            if (nLodDH != nLodDW)
            {
              TextureWarning(pData->m_FileName.c_str(),"Error: CTexMan::ImagePreprocessing: Scaling of '%s' compressed texture is dangerous (non-proportional scaling)", pData->m_FileName.c_str());
            }
            else
            if (n)
            {
              byte *dst = pData->m_pData;
              int nSize = pData->m_nSize;
              memmove(dst, &dst[nOffs], nSize-nOffs);
              pData->m_nSize = nSize-nOffs;
              pData->m_nMips = nMips-n;
              pData->m_nWidth = nNewWidth;
              pData->m_nHeight = nNewHeight;
              bResample = true;
            }
            else
            {
              bResample = true;
              nNewWidth = nSrcWidth;
              nNewHeight = nSrcHeight;
            }
          }
          else
          {
   //Diesel cut  TextureError       
TextureError(pData->m_FileName.c_str(),"Error: CTexMan::ImagePreprocessing: Scaling of '%s' compressed texture is dangerous (only single mip)", pData->m_FileName.c_str());
            bResample = true;
            nNewWidth = nSrcWidth;
            nNewHeight = nSrcHeight;
          }
        }
        if (!bResample)
        {
          if (s_nMaxTextureSize<=0 || (nSrcWidth<s_nMaxTextureSize && nSrcHeight<s_nMaxTextureSize))
          {
            int nOutSize = 0;
            byte *data_ = Convert(pData->m_pData, nSrcWidth, nSrcHeight, nMips, eTF, eTF_A8R8G8B8, nMips, nOutSize, true);
            byte *dst = new byte[nNewWidth*nNewHeight*4];
            byte *src = data_;
            Resample(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
            SAFE_DELETE_ARRAY(data_);
            data_ = Convert(dst, nNewWidth, nNewHeight, nMips, eTF_A8R8G8B8, eTF, nMips, nOutSize, true);
            SAFE_DELETE_ARRAY(dst);
            int DXTSize = TextureDataSize(nNewWidth, nNewHeight, 1, nMips, eTF);
            assert(nOutSize == DXTSize);
            pData->AssignData(data_);
            pData->m_nSize = DXTSize;
            pData->m_nMips = nMips;
            pData->m_nWidth = nNewWidth;
            pData->m_nHeight = nNewHeight;
          }
        }
      }
      else
      {
				int nComps = ComponentsForFormat(eTF);
        if (bPowerOfTwo)
        {
          bool bResampled = false;
          if (nMips > 1)
          {
            int nLodDW = 0;
            int nLodDH = 0;
            int nOffs = 0;
            int wdt = nSrcWidth;
            int hgt = nSrcHeight;
            int n = 0;
            while (wdt || hgt)
            {
              if (!wdt)
                wdt = 1;
              if (!hgt)
                hgt = 1;

              if (wdt == nNewWidth)
                nLodDW = n;
              if (hgt == nNewHeight)
                nLodDH = n;

              if (nLodDH && nLodDW)
                break;
              nOffs += wdt*hgt*nComps;
              wdt >>= 1;
              hgt >>= 1;
              n++;
            }
            if (nLodDH == nLodDW && n)
            {
              byte *dst = pData->m_pData;
              int nSize = pData->m_nSize;
              memmove(dst, &dst[nOffs], nSize-nOffs);
              pData->m_nSize = nSize-nOffs;
              pData->m_nMips = nMips-n;
              pData->m_nWidth = nNewWidth;
              pData->m_nHeight = nNewHeight;
              bResampled = true;
            }
          }
          if (!bResampled)
          {
            if (nComps == 1)
            {
              byte *dst = new byte[nNewWidth*nNewHeight];
              byte *src = pData->m_pData;
              Resample8(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
              pData->AssignData(dst);
              pData->m_nSize = nNewWidth*nNewHeight;
              pData->m_nMips = 1;
              pData->m_nWidth = nNewWidth;
              pData->m_nHeight = nNewHeight;
            }
            else
            {
              byte *dst;
              byte *src = pData->m_pData;
							if( IsFourBit(eTF) )
							{
								dst = new byte[nNewWidth*nNewHeight*2];
								Resample4Bit(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
								pData->AssignData(dst);
								pData->m_nSize = nNewWidth*nNewHeight*2;
							}
							else
							{
								dst = new byte[nNewWidth*nNewHeight*4];
	              Resample(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
								pData->AssignData(dst);
								pData->m_nSize = nNewWidth*nNewHeight*4;
							}
              pData->m_nMips = 1;
              pData->m_nWidth = nNewWidth;
              pData->m_nHeight = nNewHeight;
            }
          }
        }
        else
        {
          if (nMips > 1)
          {
						byte *src = pData->m_pData;
						if( IsFourBit(eTF) )
						{
							byte *dst = new byte[nNewWidth*nNewHeight*2];
							Resample4Bit(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
							pData->AssignData(dst);
							pData->m_nSize = nNewWidth*nNewHeight*2;
						}
						else
						{
							byte *dst = new byte[nNewWidth*nNewHeight*4];
							byte *src1 = NULL;
							if (nComps == 3)
							{
								src1 = new byte[nSrcWidth*nSrcHeight*4];
								for (int iTexel=0; iTexel<nSrcWidth*nSrcHeight; iTexel++)
								{
									src1[iTexel*4+0] = src[iTexel*3+0];
									src1[iTexel*4+1] = src[iTexel*3+1];
									src1[iTexel*4+2] = src[iTexel*3+2];
									src1[iTexel*4+3] = 255;
								}
								src = src1;
							}
							Resample(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
							SAFE_DELETE_ARRAY(src1);
							pData->AssignData(dst);
							pData->m_nSize = nNewWidth*nNewHeight*4;
						}
            pData->m_nMips = 1;
            pData->m_nWidth = nNewWidth;
            pData->m_nHeight = nNewHeight;
          }
          else
          {
						byte *dst;
            byte *src = pData->m_pData;
						if( IsFourBit(eTF) )
						{
							dst = new byte[nNewWidth*nNewHeight*2];
							Resample4Bit(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
							pData->AssignData(dst);
							pData->m_nSize = nNewWidth*nNewHeight*2;
						}
						else
						{
							dst = new byte[nNewWidth*nNewHeight*4];
	            Resample(dst, nNewWidth, nNewHeight, src, nSrcWidth, nSrcHeight);
							pData->AssignData(dst);
							pData->m_nSize = nNewWidth*nNewHeight*4;
						}
            pData->m_nMips = 1;
            pData->m_nWidth = nNewWidth;
            pData->m_nHeight = nNewHeight;
          }
        }
      }
    }
    ETEX_Format eTFDst = ClosestFormatSupported(m_eTFDst);
    if (eTFDst != m_eTFDst || eTF != eTFDst)
    {
      if (eTFDst == eTF_Unknown && !DDSFormats::IsNormalMap(eTFDst))
      {










        assert(0);

      }
      
//			if (DDSFormats::IsNormalMap(m_eTFDst) && eTFDst == eTF_Unknown)
//       eTFDst = eTF_A8R8G8B8;
			if (DDSFormats::IsNormalMap(m_eTFDst) && eTFDst == eTF_Unknown)
				eTFDst = eTF_DXT5;																// compressed normal map xy stored in ba, z reconstructed in shader 

      int nOutSize = 0;
      byte *data_ = Convert(pData->m_pData, nNewWidth, nNewHeight, pData->m_nMips, eTF, eTFDst, pData->m_nMips, nOutSize, true);
      if (data_)
      {
        pData->m_nSize = nOutSize;
        pData->m_eTF = eTFDst;
        m_eTFSrc = eTFDst;
        m_eTFDst = eTFDst;
        pData->AssignData(data_);
      }
    }
  }

//  assert(Data[0].m_pData);
  if (Data[0].m_pData)
  {
    m_nWidth = Data[0].m_nWidth;
    m_nHeight = Data[0].m_nHeight;
    m_nDepth = Data[0].m_nDepth;
    m_nMips = Data[0].m_nMips;
  }





}














int CTexture::ComponentsForFormat(ETEX_Format eTF)
{
  return BitsPerPixel(eTF) / 8;
}

int CTexture::BitsPerPixel(ETEX_Format eTF)
{
  switch (eTF)
  {
    case eTF_A8R8G8B8:
		case eTF_X8R8G8B8:
    case eTF_X8L8V8U8:
    case eTF_A2R10G10B10:





      return 32;
    case eTF_A4R4G4B4:
      return 16;
    case eTF_R8G8B8:
      return 24;
    case eTF_L8V8U8:
      return 24;
    case eTF_V8U8:
      return 16;
    case eTF_R5G6B5:
    case eTF_R5G5B5:
      return 16;
    case eTF_A16B16G16R16:
    case eTF_A16B16G16R16F:
      return 64;
    case eTF_A32B32G32R32F:
      return 128;
    case eTF_G16R16:
    case eTF_G16R16F:
    case eTF_V16U16:
      return 32;
    case eTF_A8:



    case eTF_L8:
      return 8;
    case eTF_A8L8:
      return 16;
    case eTF_R32F:
      return 32;
    case eTF_R16F:
      return 16;
		case eTF_DXT1:



      return 4;
    case eTF_DXT3:
    case eTF_DXT5:
    case eTF_3DC:
      return 8;
    case 	eTF_DEPTH24:
      return 32;
		case 	eTF_DEPTH16:
			return 32;
    case 	eTF_DF16:
      return 16;
    case 	eTF_DF24:
      return 24;
    case 	eTF_D16:
      return 16;
    case 	eTF_D24S8:
      return 32;
    case 	eTF_D32F:
      return 32;

    case 	eTF_NULL:
      return 8;

    default:
      assert(0);
  }
  return 0;
}

int CTexture::CalcNumMips(int nWidth, int nHeight)
{
  int nMips = 0;
  while (nWidth || nHeight)
  {
    if (!nWidth)   nWidth = 1;
    if (!nHeight)  nHeight = 1;
    nWidth >>= 1;
    nHeight >>= 1;
    nMips++;
  }
  return nMips;
}

int CTexture::TextureDataSize(int nWidth, int nHeight, int nDepth, int nMips, ETEX_Format eTF)
{
  if (eTF == eTF_Unknown)
    return 0;

  if (nMips <= 0)
    nMips = 0;
  int nSize = 0;
  int nM = 0;
  while (nWidth || nHeight || nDepth)
  {
    if (!nWidth)
      nWidth = 1;
    if (!nHeight)
      nHeight = 1;
    if (!nDepth)
      nDepth = 1;
    nM++;

    int nSingleMipSize;
    if (IsDXTCompressed(eTF))
    {
      int blockSize = (eTF == eTF_DXT1 || eTF == eTF_CTX1) ? 8 : 16;
      nSingleMipSize = ((nWidth+3)/4)*((nHeight+3)/4)*nDepth*blockSize;
    }
    else
      nSingleMipSize = nWidth * nHeight * nDepth * BitsPerPixel(eTF)/8;
    nSize += nSingleMipSize;

    nWidth >>= 1;
    nHeight >>= 1;
    nDepth >>= 1;
    if (nMips == nM)
      break;
  }
  //assert (nM == nMips);

  return nSize;
}

void CTexture::ExpandMipFromFile( byte* dest, const int destSize, const byte* src, const int srcSize, const ETEX_Format fmt )
{
	// upload mip from file with conversions depending on format and platform specifics
	switch (fmt)
	{
	case eTF_DXT1:
	case eTF_DXT3:
	case eTF_DXT5:
	case eTF_A8:
	case eTF_R32F:
	case eTF_G16R16F:
	case eTF_V16U16:
	case eTF_A4R4G4B4:
	case eTF_A16B16G16R16F:
	case eTF_3DC:
	case eTF_CTX1:
		assert(destSize == srcSize);
		if(dest != src) cryMemcpy(dest, src, srcSize);
		break;
	case eTF_L8:
#if defined(DIRECT3D10) && !defined(PS3)
		for (int i=srcSize-1; i>=0; --i)
		{
			const byte bSrc = src[i];
			dest[i*4+0] = bSrc;
			dest[i*4+1] = bSrc;
			dest[i*4+2] = bSrc;
			dest[i*4+3] = 255;
		}
#else
		assert(destSize == srcSize);
		if(dest != src) cryMemcpy(dest, src, srcSize);
#endif
		break;
	case eTF_A8R8G8B8:
		assert(destSize == srcSize);
		if(dest != src) cryMemcpy(dest, src, srcSize);
		for (int i=srcSize/4-1; i>=0; --i)
		{



#if defined(DIRECT3D10)
			std::swap(dest[i*4+2], dest[i*4+0]);
#endif
		}
		break;
	case eTF_X8R8G8B8:
		assert(destSize == srcSize);
		if(dest != src) cryMemcpy(dest, src, srcSize);
		for (int i=srcSize/4-1; i>=0; --i)
		{



#if defined(DIRECT3D10)
			std::swap(dest[i*4+2], dest[i*4+0]);
#endif
		}
		break;
	case eTF_R8G8B8:
		for (int i=srcSize/3-1; i>=0; --i)
		{





#if defined (DIRECT3D10)
			dest[i*4+0] = src[i*3+2];
			dest[i*4+1] = src[i*3+1];
			dest[i*4+2] = src[i*3+0];
			dest[i*4+3] = 255;
#else
			dest[i*4+0] = src[i*3+0];
			dest[i*4+1] = src[i*3+1];
			dest[i*4+2] = src[i*3+2];
			dest[i*4+3] = 255;
#endif
		}
		break;
	case eTF_L8V8U8:
		for (int i=srcSize/3-1; i>=0; --i)
		{
			dest[i*4+0] = src[i*3+0];
			dest[i*4+1] = src[i*3+1];
			dest[i*4+2] = src[i*3+2];
			dest[i*4+3] = 255;
		}
		break;
	default:
		assert(0);
	}
}
const char *CTexture::NameForTextureType(ETEX_Type eTT)
{
  char *sETT;
  switch (eTT)
  {
    case eTT_1D:
      sETT = "1D";
      break;
    case eTT_2D:
      sETT = "2D";
      break;
    case eTT_3D:
      sETT = "3D";
      break;
    case eTT_Cube:
      sETT = "Cube";
      break;
    case eTT_AutoCube:
      sETT = "AutoCube";
      break;
    case eTT_Auto2D:
      sETT = "Auto2D";
      break;
    default:
      assert(0);
      sETT = "Unknown";		// for better behaviour in non debug
      break;
  }
  return sETT;
}

const char *CTexture::NameForTextureFormat(ETEX_Format ETF)
{
  char *sETF;
  switch (ETF)
  {
		case eTF_Unknown:
			sETF = "Unknown";
			break;
    case eTF_R8G8B8:
      sETF = "R8G8B8";
      break;
    case eTF_A8R8G8B8:
      sETF = "A8R8G8B8";
      break;
    case eTF_X8R8G8B8:
      sETF = "X8R8G8B8";
      break;
    case eTF_A8:
      sETF = "A8";
      break;

















    case eTF_A8L8:
      sETF = "A8L8";
      break;
		case eTF_L8:
			sETF = "L8";
			break;
		case eTF_A4R4G4B4:
			sETF = "A4R4G4B4";
			break;
    case eTF_DXT1:
      sETF = "DXT1";
      break;
    case eTF_DXT3:
      sETF = "DXT3";
      break;
    case eTF_DXT5:
      sETF = "DXT5";
      break;
    case eTF_3DC:
      sETF = "3DC";
      break;
    case eTF_V16U16:
      sETF = "V16U16";
      break;
    case eTF_X8L8V8U8:
      sETF = "X8L8V8U8";
      break;
    case eTF_V8U8:
      sETF = "V8U8";
      break;
    case eTF_A16B16G16R16F:
      sETF = "A16B16G16R16F";
      break;
    case eTF_A16B16G16R16:
      sETF = "A16B16G16R16";
      break;
    case eTF_A32B32G32R32F:
      sETF = "A32B32G32R32F";
      break;
		case eTF_R16F:
			sETF = "R16F";
			break;
    case eTF_R32F:
      sETF = "R32F";
      break;
    case eTF_G16R16:
      sETF = "G16R16";
      break;
    case eTF_G16R16F:
      sETF = "G16R16F";
      break;
    case eTF_DF16:
      sETF = "DF16";
      break;
    case eTF_DEPTH24:
      sETF = "Depth24";
      break;
		case eTF_DF24:
			sETF = "DF24";
			break;
    case eTF_D16:
      sETF = "D16";
      break;
    case eTF_D24S8:
			sETF = "D24S8";
			break;
    case eTF_D32F:
			sETF = "D32F";
			break;
    case eTF_A2R10G10B10:
      sETF = "A2R10G10B10";
      break;
		case eTF_R5G6B5:
			sETF = "R5G6B5";
			break;
		case eTF_R5G5B5:
			sETF = "R5G5B5";
			break;
    case eTF_NULL:
      sETF = "NULL";
      break;
    default:
      assert(0);
      sETF = "Unknown";		// for better behaviour in non debug
      break;
  }
  return sETF;
}

ETEX_Format CTexture::TextureFormatForName(const char *sETF)
{
  if (!stricmp(sETF, "R8G8B8"))
    return eTF_R8G8B8;
  if (!stricmp(sETF, "A8R8G8B8"))
    return eTF_A8R8G8B8;
  if (!stricmp(sETF, "A8"))
    return eTF_A8;






  if (!stricmp(sETF, "A8L8"))
    return eTF_A8L8;
  if (!stricmp(sETF, "DXT1"))
    return eTF_DXT1;
  if (!stricmp(sETF, "DXT3"))
    return eTF_DXT3;
  if (!stricmp(sETF, "DXT5"))
    return eTF_DXT5;
  if (!stricmp(sETF, "3DC") || !stricmp(sETF, "ATI2"))
    return eTF_3DC;
  if (!stricmp(sETF, "V16U16"))
    return eTF_V16U16;
  if (!stricmp(sETF, "X8L8V8U8"))
    return eTF_X8L8V8U8;
  if (!stricmp(sETF, "V8U8"))
    return eTF_V8U8;
  if (!stricmp(sETF, "DF16"))
    return eTF_DF16;
  if (!stricmp(sETF, "DF24"))
    return eTF_DF24;
  if (!stricmp(sETF, "D16"))
    return eTF_D16;
  if (!stricmp(sETF, "D24S8"))
    return eTF_D24S8; 
  if (!stricmp(sETF, "D32F"))
    return eTF_D32F;

  assert (0);
  return eTF_Unknown;
}

ETEX_Type CTexture::TextureTypeForName(const char *sETT)
{
  if (!stricmp(sETT, "1D"))
    return eTT_1D;
  if (!stricmp(sETT, "2D"))
    return eTT_2D;
  if (!stricmp(sETT, "3D"))
    return eTT_3D;
  if (!stricmp(sETT, "Cube"))
    return eTT_Cube;
  if (!stricmp(sETT, "AutoCube"))
    return eTT_AutoCube;
  if (!stricmp(sETT, "User"))
    return eTT_User;
  assert(0);
  return eTT_2D;
}

void CTexture::Resample4Bit(byte *pDst, int nNewWidth, int nNewHeight, const byte *pSrc, int nSrcWidth, int nSrcHeight)
{
	int   i, j;
	const uint16 *inrow;
	uint32  ifrac, fracstep;
//	uint16 iData;

	uint16 *uout = (uint16 *)pDst;
	const uint16 *uin = (const uint16 *)pSrc;
	fracstep = nSrcWidth*0x10000/nNewWidth;
	if (!(nNewWidth & 3))
	{
		for (i=0 ; i<nNewHeight ; i++, uout+=nNewWidth)
		{
			inrow = uin + nSrcWidth*(i*nSrcHeight/nNewHeight);
			ifrac = fracstep >> 1;
			for (j=0; j<nNewWidth; j+=4)
			{
				uout[j] = inrow[ifrac>>16];
				ifrac += fracstep;
				uout[j+1] = inrow[ifrac>>16];
				ifrac += fracstep;
				uout[j+2] = inrow[ifrac>>16];
				ifrac += fracstep;
				uout[j+3] = inrow[ifrac>>16];
				ifrac += fracstep;
			}
		}
	}
	else
	{
		for (i=0; i<nNewHeight; i++, uout+=nNewWidth)
		{
			inrow = uin + nSrcWidth*(i*nSrcHeight/nNewHeight);
			ifrac = fracstep >> 1;
			for (j=0; j<nNewWidth; j++)
			{
				uout[j] = inrow[ifrac>>16];
				ifrac += fracstep;
			}
		}
	}
}


void CTexture::Resample(byte *pDst, int nNewWidth, int nNewHeight, const byte *pSrc, int nSrcWidth, int nSrcHeight)
{
  int   i, j;
  const uint32  *inrow;
  uint32  ifrac, fracstep;

  uint32 *uout = (uint32 *)pDst;
  const uint32 *uin = (const uint32 *)pSrc;
  fracstep = nSrcWidth*0x10000/nNewWidth;
  if (!(nNewWidth & 3))
  {
    for (i=0 ; i<nNewHeight ; i++, uout+=nNewWidth)
    {
      inrow = uin + nSrcWidth*(i*nSrcHeight/nNewHeight);
      ifrac = fracstep >> 1;
      for (j=0; j<nNewWidth; j+=4)
      {
        uout[j] = inrow[ifrac>>16];
        ifrac += fracstep;
        uout[j+1] = inrow[ifrac>>16];
        ifrac += fracstep;
        uout[j+2] = inrow[ifrac>>16];
        ifrac += fracstep;
        uout[j+3] = inrow[ifrac>>16];
        ifrac += fracstep;
      }
    }
  }
  else
  {
    for (i=0; i<nNewHeight; i++, uout+=nNewWidth)
    {
      inrow = uin + nSrcWidth*(i*nSrcHeight/nNewHeight);
      ifrac = fracstep >> 1;
      for (j=0; j<nNewWidth; j++)
      {
        uout[j] = inrow[ifrac>>16];
        ifrac += fracstep;
      }
    }
  }
}

void CTexture::Resample8(byte *pDst, int nNewWidth, int nNewHeight, const byte *pSrc, int nSrcWidth, int nSrcHeight)
{
  int   i, j;
  const byte  *inrow;
  uint32  ifrac, fracstep;

  byte *uout = pDst;
  const byte *uin = pSrc;
  fracstep = nSrcWidth*0x10000/nNewWidth;
  if (!(nNewWidth & 3))
  {
    for (i=0 ; i<nNewHeight ; i++, uout+=nNewWidth)
    {
      inrow = uin + nSrcWidth*(i*nSrcHeight/nNewHeight);
      ifrac = fracstep >> 1;
      for (j=0; j<nNewWidth; j+=4)
      {
        uout[j] = inrow[ifrac>>16];
        ifrac += fracstep;
        uout[j+1] = inrow[ifrac>>16];
        ifrac += fracstep;
        uout[j+2] = inrow[ifrac>>16];
        ifrac += fracstep;
        uout[j+3] = inrow[ifrac>>16];
        ifrac += fracstep;
      }
    }
  }
  else
  {
    for (i=0; i<nNewHeight; i++, uout+=nNewWidth)
    {
      inrow = uin + nSrcWidth*(i*nSrcHeight/nNewHeight);
      ifrac = fracstep >> 1;
      for (j=0; j<nNewWidth; j++)
      {
        uout[j] = inrow[ifrac>>16];
        ifrac += fracstep;
      }
    }
  }
}

bool CTexture::Invalidate(int nNewWidth, int nNewHeight, ETEX_Format eTF)
{
  bool bRelease = false;
  if (nNewWidth > 0 && nNewWidth != m_nWidth)
  {
    m_nWidth = nNewWidth;
    bRelease = true;
  }
  if (nNewHeight > 0 && nNewHeight != m_nHeight)
  {
    m_nHeight = nNewHeight;
    bRelease = true;
  }
  if (eTF != eTF_Unknown && eTF != m_eTFDst)
  {
    m_eTFDst = eTF;
    bRelease = true;
  }
  if (!m_pDevTexture)
    return false;

  if (bRelease)
    ReleaseDeviceTexture(true);

  return bRelease;
}

byte *CTexture::GetData32(int nSide, int nLevel, byte * pDst, ETEX_Format eDstFormat)
{
  int nPitch;
  byte *pData = LockData(nPitch, nSide, nLevel);
  if (!pData)
    return NULL;
  if (m_eTFDst != eTF_A8R8G8B8 && m_eTFDst != eTF_X8R8G8B8)
  {
    int nOutSize = 0;

    if(m_eTFSrc == eDstFormat && pDst)
      memcpy(pDst, pData, GetDeviceDataSize());
    else
      pDst = Convert(pData, m_nWidth, m_nHeight, 1, m_eTFSrc, eDstFormat, 1, nOutSize, true);
  }
  else
  {
    if(!pDst)
      pDst = new byte [m_nWidth*m_nHeight*4];
    memcpy(pDst, pData, m_nWidth*m_nHeight*4);
  }
  UnlockData(nSide, nLevel);
  return pDst;
}

const int CTexture::GetSize(bool bIncludePool) const
{
  int nSize = sizeof(CTexture);
  nSize += m_SrcName.capacity();
  nSize += m_DirtyRects.capacity()*sizeof(RECT);
  if (m_pFileTexMips)
  {
    for (int i=0; i<m_nMips; i++)
    {
      nSize += m_pFileTexMips[i].GetSize();
    }
  }
  if (bIncludePool && m_pPoolItem)
    nSize += m_pPoolItem->GetSize();

  return nSize;
}

void CTexture::Init()
{
  s_nCurTexResolutionReduction = CRenderer::CV_r_texresolution;
  s_nCurTexBumpResolutionReduction = CRenderer::CV_r_texbumpresolution;






  SDynTexture::Init();
  InitStreaming();
  if (!s_Root.m_Next)
  {
    s_Root.m_Next = &s_Root;
    s_Root.m_Prev = &s_Root;
  }
  for (int i=0; i<MAX_TMU; i++)
  {
    s_TexStateIDs[i] = -1;
  }
  s_nTexSizeHistory = 0;
  s_nProcessedTextureID1 = 0;
  s_nPhaseProcessingTextures = 0;
  s_fAdaptiveStreamDistRatio = 1.0f;

  CTexture::LoadDefaultTextures();

  SDynTexture2::Init(eTP_Clouds);
  SDynTexture2::Init(eTP_Sprites);
  SDynTexture2::Init(eTP_VoxTerrain);
  SDynTexture2::Init(eTP_DynTexSources);
}

int __cdecl TexCallback( const VOID* arg1, const VOID* arg2 )
{
  CTexture **pi1 = (CTexture **)arg1;
  CTexture **pi2 = (CTexture **)arg2;
  CTexture *ti1 = *pi1;
  CTexture *ti2 = *pi2;

  if (ti1->GetDeviceDataSize() > ti2->GetDeviceDataSize())
    return -1;
  if (ti1->GetDeviceDataSize() < ti2->GetDeviceDataSize())
    return 1;
  return stricmp(ti1->GetSourceName(), ti2->GetSourceName());
}

int __cdecl TexCallbackMips( const VOID* arg1, const VOID* arg2 )
{
  CTexture **pi1 = (CTexture **)arg1;
  CTexture **pi2 = (CTexture **)arg2;
  CTexture *ti1 = *pi1;
  CTexture *ti2 = *pi2;

  int i, nSize1, nSize2;

  nSize1 = 0;
  for (i=0; i<ti1->GetNumMips(); i++)
  {
    nSize1 += ti1->m_pFileTexMips[i].GetSize();
  }
  nSize2 = 0;
  for (i=0; i<ti2->GetNumMips(); i++)
  {
    nSize2 += ti2->m_pFileTexMips[i].GetSize();
  }

  if (nSize1 > nSize2)
    return -1;
  if (nSize1 < nSize2)
    return 1;
  return stricmp(ti1->GetSourceName(), ti2->GetSourceName());
}

void CTexture::Update()
{
  CRenderer *rd = gRenDev;
  uint32 i;
  char buf[256]="";

  AsyncRequestsProcessing();

  SDynTexture::Tick();

  SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
  ResourcesMapItor itor;

  if (s_nStreamingMode != CRenderer::CV_r_texturesstreaming)
  {
    InitStreaming();
    assert(s_nStreamingMode == CRenderer::CV_r_texturesstreaming);
  }

  if (pRL)
  {
		bool bUpdateNonDDNTextures = CRenderer::CV_r_texresolution!=s_nCurTexResolutionReduction;
		bool bUpdateDDNTextures = CRenderer::CV_r_texbumpresolution!=s_nCurTexBumpResolutionReduction;

    if (bUpdateNonDDNTextures || bUpdateDDNTextures)
    {
      s_nCurTexResolutionReduction = CRenderer::CV_r_texresolution;
      s_nCurTexBumpResolutionReduction = CRenderer::CV_r_texbumpresolution;

			CHWShader::mfFlushPendedShadersWait(-1);
			CTexture::FlushAllStreamingTasks();

      for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
      {
        CTexture *tp = (CTexture *)itor->second;
        if (!tp || (tp->GetFlags() & (FT_DONT_RELEASE | FT_DONT_RESIZE)))
          continue;
        if (!(tp->GetFlags() & FT_FROMIMAGE))
          continue;
        if (tp->GetTextureType() == eTT_Cube)
        {
          //if (tp->m_CubeSide > 0)
//            continue;
          if(bUpdateNonDDNTextures)
            tp->Reload();
        }
        else
        if (tp->GetFlags() & FT_TEX_NORMAL_MAP)
        {
					if(bUpdateDDNTextures)
            tp->Reload();
        }
        else
        {
          if (bUpdateNonDDNTextures)
            tp->Reload();
        }
      }
    }

    if (CRenderer::CV_r_texlog == 2 || CRenderer::CV_r_texlog == 3 || CRenderer::CV_r_texlog == 4)
    {
      FILE *fp = NULL;
      TArray<CTexture *> Texs;
      int Size = 0;
      int PartSize = 0;
      if (CRenderer::CV_r_texlog == 2 || CRenderer::CV_r_texlog == 3)
      {
        for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
        {
          CTexture *tp = (CTexture *)itor->second;
          if (CRenderer::CV_r_texlog == 3 && tp->IsNoTexture())
          {
            Texs.AddElem(tp);
          }
          else
          if (CRenderer::CV_r_texlog == 2 && !tp->IsNoTexture() && tp->m_pFileTexMips) // (tp->GetFlags() & FT_FROMIMAGE))
          {
            Texs.AddElem(tp);
          }
        }
        if (CRenderer::CV_r_texlog == 3)
				{
					CryLogAlways( "Logging to MissingTextures.txt..." );
          fp = fxopen("MissingTextures.txt", "w");
				}
        else
				{
					CryLogAlways( "Logging to UsedTextures.txt..." );
          fp = fxopen("UsedTextures.txt", "w");
				}
        fprintf(fp, "*** All textures: ***\n");

				if(Texs.Num())
	        qsort(&Texs[0], Texs.Num(), sizeof(CTexture *), TexCallbackMips);

        for (i=0; i<Texs.Num(); i++)
        {
					int w = Texs[i]->GetWidth();
					int h = Texs[i]->GetHeight();

          int nTSize = 0;
          for (int j=0; j<Texs[i]->GetNumMips(); j++)
          {
            nTSize += Texs[i]->m_pFileTexMips[j].GetSize();
          }

          fprintf(fp, "%d\t\t%d x %d\t\tType: %s\t\tMips: %d\t\tFormat: %s\t\t(%s)\n", nTSize,w,h,Texs[i]->NameForTextureType(Texs[i]->GetTextureType()), Texs[i]->GetNumMips(), Texs[i]->NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
          //Size += Texs[i]->GetDataSize();
          Size += nTSize;

          PartSize += Texs[i]->GetDeviceDataSize();
        }
        fprintf(fp, "*** Total Size: %d\n\n", Size /*, PartSize, PartSize */);

        Texs.Free();
      }
      for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
      {
        CTexture *tp = (CTexture *)itor->second;
        if (tp->m_nAccessFrameID == rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID)
        {
          Texs.AddElem(tp);
        }
      }

			if (fp)
			{
				fclose(fp);
				fp = 0;
			}

			fp = fxopen("UsedTextures_Frame.txt", "w");

      if (fp)
        fprintf(fp, "\n\n*** Textures used in current frame: ***\n");
      else
        rd->TextToScreenColor(4,13, 1,1,0,1, "*** Textures used in current frame: ***");
      int nY = 17;
      
			if(Texs.Num())
				qsort(&Texs[0], Texs.Num(), sizeof(CTexture *), TexCallback);
      
			Size = 0;
      for (i=0; i<Texs.Num(); i++)
      {
        if (fp)
          fprintf(fp, "%.3fKb\t\tType: %s\t\tFormat: %s\t\t(%s)\n", Texs[i]->GetDeviceDataSize()/1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
        else
        {
          sprintf(buf, "%.3fKb  Type: %s  Format: %s  (%s)", Texs[i]->GetDeviceDataSize()/1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
          rd->TextToScreenColor(4, nY, 0,1,0,1, buf);
          nY += 3;
        }
        PartSize += Texs[i]->GetDeviceDataSize();
        Size += Texs[i]->GetDataSize();
      }
      if (fp)
      {
        fprintf(fp, "*** Total Size: %.3fMb, Device Size: %.3fMb\n\n", Size/(1024.0f*1024.0f), PartSize/(1024.0f*1024.0f));
      }
      else
      {
        sprintf(buf, "*** Total Size: %.3fMb, Device Size: %.3fMb", Size/(1024.0f*1024.0f), PartSize/(1024.0f*1024.0f));
        rd->TextToScreenColor(4,nY+1, 0,1,1,1, buf);
      }

      Texs.Free();
      for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
      {
        CTexture *tp = (CTexture *)itor->second;
        if (tp && !tp->IsNoTexture())
        {
          Texs.AddElem(tp);
        }
      }

			if (fp)
			{
				fclose(fp);
				fp = 0;
			}
			fp = fxopen("UsedTextures_All.txt", "w");

      if (fp)
        fprintf(fp, "\n\n*** All Existing Textures: ***\n");
      else
        rd->TextToScreenColor(4,13, 1,1,0,1, "*** Textures loaded: ***");

			if(Texs.Num())
	      qsort(&Texs[0], Texs.Num(), sizeof(CTexture *), TexCallback);

			Size = 0;
      for (i=0; i<Texs.Num(); i++)
      {
        if (fp)
				{
					int w = Texs[i]->GetWidth();
					int h = Texs[i]->GetHeight();
          fprintf(fp, "%d\t\t%d x %d\t\t%d mips (%.3fKb)\t\tType: %s \t\tFormat: %s\t\t(%s)\n", Texs[i]->GetDataSize(),w,h,Texs[i]->GetNumMips(), Texs[i]->GetDeviceDataSize()/1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
				}
        else
        {
          sprintf(buf, "%.3fKb  Type: %s  Format: %s  (%s)", Texs[i]->GetDataSize()/1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
          rd->TextToScreenColor(4,nY, 0,1,0,1, buf);
          nY += 3;
        }
        Size += Texs[i]->GetDeviceDataSize();
      }
      if (fp)
      {
        fprintf(fp, "*** Total Size: %.3fMb\n\n", Size/(1024.0f*1024.0f));
      }
      else
      {
        sprintf(buf, "*** Total Size: %.3fMb", Size/(1024.0f*1024.0f));
        rd->TextToScreenColor(4,nY+1, 0,1,1,1, buf);
      }


      Texs.Free();
      for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
      {
        CTexture *tp = (CTexture *)itor->second;
        if (tp && !tp->IsNoTexture() && !tp->IsStreamed())
        {
          Texs.AddElem(tp);
        }
      }
			/*
      if (fp)
        fprintf(fp, "\n\n*** Textures non-streamed: ***\n");
      else
        rd->TextToScreenColor(4,13, 1,1,0,1, "*** Textures non-streamed: ***");

			if(Texs.Num())
	      qsort(&Texs[0], Texs.Num(), sizeof(CTexture *), TexCallback );

      Size = 0;
      for (i=0; i<Texs.Num(); i++)
      {
        if (fp)
          fprintf(fp, "%.3fKb\t\tType: %s\t\tFormat: %s\t\t(%s)\n", Texs[i]->GetDeviceDataSize()/1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureType(Texs[i]->GetTextureType()), Texs[i]->GetName());
        else
        {
          sprintf(buf, "%.3fKb  Type: %s  Format: %s  (%s)", Texs[i]->GetDeviceDataSize()/1024.0f, CTexture::NameForTextureType(Texs[i]->GetTextureType()), CTexture::NameForTextureFormat(Texs[i]->GetDstFormat()), Texs[i]->GetName());
          rd->TextToScreenColor(4,nY, 0,1,0,1, buf);
          nY += 3;
        }
        Size += Texs[i]->GetDeviceDataSize();
      }
      if (fp)
      {
        fprintf(fp, "*** Total Size: %.3fMb\n\n", Size/(1024.0f*1024.0f));
        fclose (fp);
      }
      else
      {
        sprintf(buf, "*** Total Size: %.3fMb", Size/(1024.0f*1024.0f));
        rd->TextToScreenColor(4,nY+1, 0,1,1,1, buf);
      }
			*/

			if (fp)
			{
				fclose(fp);
				fp = 0;
			}

      if (CRenderer::CV_r_texlog != 4)
        CRenderer::CV_r_texlog = 0;
    }
    else
    if (CRenderer::CV_r_texlog == 1)
    {
      //char *str = GetTexturesStatusText();

      TArray<CTexture *> Texs;
      //TArray<CTexture *> TexsNM;
      for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
      {
        CTexture *tp = (CTexture *)itor->second;
        if (tp && !tp->IsNoTexture())
        {
          Texs.AddElem(tp);
          //if (tp->GetFlags() & FT_TEX_NORMAL_MAP)
          //  TexsNM.AddElem(tp);
        }
      }

			if(Texs.Num())
	      qsort(&Texs[0], Texs.Num(), sizeof(CTexture *), TexCallback);

			int AllSize = 0;
      int Size = 0;
      int PartSize = 0;
      int NonStrSize = 0;
			int nNoStr = 0;
      int SizeNM = 0;
      int SizeDynCom = 0;
      int SizeDynAtl = 0;
      int PartSizeNM = 0;
      int nNumTex = 0;
			int nNumTexNM = 0;
      int nNumTexDynAtl = 0;
      int nNumTexDynCom = 0;
      for (i=0; i<Texs.Num(); i++)
      {
        if (Texs[i]->GetDevTexture() && !(Texs[i]->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET)))
        {
          AllSize += Texs[i]->GetDataSize();
          if (!Texs[i]->IsStreamed())
          {
            NonStrSize += Texs[i]->GetDataSize();
            nNoStr++;
          }
        }

        if (Texs[i]->GetFlags() & (FT_USAGE_RENDERTARGET | FT_USAGE_DYNAMIC))
        {
          if (Texs[i]->GetFlags() & FT_USAGE_ATLAS)
          {
            ++nNumTexDynAtl;
            SizeDynAtl += Texs[i]->GetDataSize();
          }
          else
          {
            ++nNumTexDynCom;
            SizeDynCom += Texs[i]->GetDataSize();
          }
        }
        else
				if(0 == (Texs[i]->GetFlags() & FT_TEX_NORMAL_MAP))
				{
          if (!Texs[i]->IsUnloaded())
					{
						++nNumTex;
						Size += Texs[i]->GetDataSize();
  					PartSize += Texs[i]->GetDeviceDataSize();
					}
				}
				else
				{
					if (!Texs[i]->IsUnloaded())
					{
						++nNumTexNM;
						SizeNM += Texs[i]->GetDataSize();
						PartSizeNM += Texs[i]->GetDeviceDataSize();
					}
        }
      }

			sprintf(buf, "All texture objects: %d (Size: %.3fMb), NonStreamed: %d (Size: %.3fMb)", Texs.Num(), AllSize/(1024.0f*1024.0f), nNoStr, NonStrSize/(1024.0f*1024.0f));
      rd->TextToScreenColor(4,13, 1,1,0,1, buf);
      sprintf(buf, "All Loaded Texture Maps: %d (All MIPS: %.3fMb, Loaded MIPS: %.3fMb)", nNumTex, Size/(1024.0f*1024.0f), PartSize/(1024.0f*1024.0f));
      rd->TextToScreenColor(4,16, 1,1,0,1, buf);
      sprintf(buf, "All Loaded Normal Maps: %d (All MIPS: %.3fMb, Loaded MIPS: %.3fMb)", nNumTexNM, SizeNM/(1024.0f*1024.0f), PartSizeNM/(1024.0f*1024.0f));
      rd->TextToScreenColor(4,19, 1,1,0,1, buf);
      sprintf(buf, "All Dynamic textures: %d (%.3fMb), %d Atlases (%.3fMb), %d Separared (%.3fMb)", nNumTexDynAtl+nNumTexDynCom, (SizeDynAtl+SizeDynCom)/(1024.0f*1024.0f), nNumTexDynAtl, SizeDynAtl/(1024.0f*1024.0f), nNumTexDynCom, SizeDynCom/(1024.0f*1024.0f));
      rd->TextToScreenColor(4,22, 1,1,0,1, buf);

      Texs.Free();
      for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
      {
        CTexture *tp = (CTexture *)itor->second;
        if (tp && !tp->IsNoTexture() && tp->m_nAccessFrameID == rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_nFrameUpdateID)
        {
          Texs.AddElem(tp);
        }
      }

			if(Texs.Num())
	      qsort(&Texs[0], Texs.Num(), sizeof(CTexture *), TexCallback);

			Size = 0;
      SizeDynAtl = 0;
      SizeDynCom = 0;
      PartSize = 0;
      NonStrSize = 0;
      for (i=0; i<Texs.Num(); i++)
      {
        Size += Texs[i]->GetDataSize();
        if (Texs[i]->GetFlags() & (FT_USAGE_DYNAMIC | FT_USAGE_RENDERTARGET))
        {
          if (Texs[i]->GetFlags() & FT_USAGE_ATLAS)
            SizeDynAtl += Texs[i]->GetDataSize();
          else
            SizeDynCom += Texs[i]->GetDataSize();
        }
        else
          PartSize += Texs[i]->GetDeviceDataSize();
        if (!Texs[i]->IsStreamed())
          NonStrSize += Texs[i]->GetDataSize();
      }
      sprintf(buf, "Current tex. objects: %d (Size: %.3fMb, Dyn. Atlases: %.3f, Dyn. Separated: %.3f, Loaded: %.3f, NonStreamed: %.3f)", Texs.Num(), Size/(1024.0f*1024.0f), SizeDynAtl/(1024.0f*1024.0f), SizeDynCom/(1024.0f*1024.0f), PartSize/(1024.0f*1024.0f), NonStrSize/(1024.0f*1024.0f));
      rd->TextToScreenColor(4,27, 1,0,0,1, buf);
    }
  }
}

bool CTexture::SetProjector(int nUnit, int nState, int nSamplerSlot)
{
  CRenderer *rd = gRenDev;

  bool bRes = false;
  SLightPass *lp = &rd->m_RP.m_LPasses[rd->m_RP.m_nCurLightPass];
  assert (lp->nLights==1 && (lp->pLights[0]->m_Flags & DLF_PROJECT));
  CDLight *dl = lp->pLights[0];
  if (dl && dl->m_pLightImage!=0)
  {
    bRes = true;
    CTexture *tp = (CTexture *)dl->m_pLightImage;
    if (dl->m_fAnimSpeed)
    {
      int n = 0;
      CTexture *t = tp;
      while (t)
      {
        t = t->m_NextTxt;
        n++;
      }
      if (n > 1)
      {
        int m = (int)(rd->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_RealTime / dl->m_fAnimSpeed) % n;
        for (int i=0; i<m; i++)
        {
          tp = tp->m_NextTxt;
        }
      }
    }
    tp->Apply(nUnit, nState, -1, nSamplerSlot);
  }
  return bRes;
}

//=========================================================================

SEnvTexture *CTexture::FindSuitableEnvCMap(Vec3& Pos, bool bMustExist, int RendFlags, float fDistToCam)
{
  float time0 = iTimer->GetAsyncCurTime();

  SEnvTexture *cm = NULL;
  int i;
  
  float dist = 999999;
  int firstForUse = -1;
  int firstFree = -1;
  for (i=0; i<MAX_ENVCUBEMAPS; i++)
  {
    SEnvTexture *cur = &CTexture::s_EnvCMaps[i];
    Vec3 delta = cur->m_CamPos - Pos;
    float s = delta.GetLengthSquared();
    if (s < dist)
    {
      dist = s;
      firstForUse = i;
      if (!dist)
        break;
    }
    if (!cur->m_pTex->m_pTexture && firstFree < 0)
      firstFree = i;
  }

	float curTime = iTimer->GetCurrTime();
  int nUpdate = -2;
  float fTimeInterval = fDistToCam * CRenderer::CV_r_envcmupdateinterval + CRenderer::CV_r_envcmupdateinterval*0.5f;
  float fDelta = curTime - CTexture::s_EnvCMaps[firstForUse].m_TimeLastUpdated;
  if (bMustExist)
    nUpdate = -2;
  else
  if (dist > MAX_ENVCUBEMAPSCANDIST_THRESHOLD)
  {
    if (firstFree >= 0)
      nUpdate = firstFree;
    else
      nUpdate = -1;
  }
  else
  if (fDelta > fTimeInterval)
    nUpdate = firstForUse;
  if (nUpdate == -2)
  {
    // No need to update (Up to date)
    return &CTexture::s_EnvCMaps[firstForUse];
  }
  if (nUpdate >= 0)
  {
    if (!CTexture::s_EnvCMaps[nUpdate].m_pTex->m_pTexture || gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvCMapUpdateTime < 0.1f)
    {
      int n = nUpdate;
      CTexture::s_EnvCMaps[n].m_TimeLastUpdated = curTime;
      CTexture::s_EnvCMaps[n].m_CamPos = Pos;
      CTexture::ScanEnvironmentCube(&CTexture::s_EnvCMaps[n], RendFlags, -1, false);
    }
    gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvCMapUpdateTime += iTimer->GetAsyncCurTime()-time0;
    return &CTexture::s_EnvCMaps[nUpdate];
  }

  dist = 0;
  firstForUse = -1;
  for (i=0; i<MAX_ENVCUBEMAPS; i++)
  {
    SEnvTexture *cur = &CTexture::s_EnvCMaps[i];
    if (dist < curTime-cur->m_TimeLastUpdated && !cur->m_bInprogress)
    {
      dist = curTime - cur->m_TimeLastUpdated;
      firstForUse = i;
    }
  }
  if (firstForUse < 0)
  {
    gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvCMapUpdateTime += iTimer->GetAsyncCurTime()-time0;
    return NULL;
  }
  int n = firstForUse;
  CTexture::s_EnvCMaps[n].m_TimeLastUpdated = curTime;
  CTexture::s_EnvCMaps[n].m_CamPos = Pos;
  CTexture::ScanEnvironmentCube(&CTexture::s_EnvCMaps[n], RendFlags, -1, false);

  gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvCMapUpdateTime += iTimer->GetAsyncCurTime()-time0;
  return &CTexture::s_EnvCMaps[n];
}

Ang3 sDeltAngles(Ang3& Ang0, Ang3& Ang1)
{
  Ang3 out;
  for (int i=0; i<3; i++)
  {
    float a0 = Ang0[i];
    a0 = (float)((360.0/65536) * ((int)(a0*(65536/360.0)) & 65535)); // angmod
    float a1 = Ang1[i];
    a1 = (float)((360.0/65536) * ((int)(a0*(65536/360.0)) & 65535));
    out[i] = a0 - a1;
  }
  return out;
}

SEnvTexture *CTexture::FindSuitableEnvTex(Vec3& Pos, Ang3& Angs, bool bMustExist, int RendFlags, bool bUseExistingREs, CShader *pSH, SRenderShaderResources *pRes, CRenderObject *pObj, bool bReflect, CRendElementBase *pRE, bool *bMustUpdate)
{
  SEnvTexture *cm = NULL;
  float time0 = iTimer->GetAsyncCurTime();

  int i;
  float distO = 999999;
  float adist = 999999;
  int firstForUse = -1;
  int firstFree = -1;
  Vec3 objPos;
  if (bMustUpdate)
    *bMustUpdate = false;
  if (!pObj)
    bReflect = false;
  else
  {
    if (bReflect)
    {
      Plane pl;
      pRE->mfGetPlane(pl);
      objPos = pl.MirrorPosition(Vec3(0,0,0));
    }
    else
    if (pRE)
      pRE->mfCenter(objPos, pObj);
    else
      objPos = pObj->GetTranslation();
  }
  float dist = 999999;
  for (i=0; i<MAX_ENVTEXTURES; i++)
  {
    SEnvTexture *cur = &CTexture::s_EnvTexts[i];
    if (cur->m_bReflected != bReflect)
      continue;
    float s = (cur->m_CamPos - Pos).GetLengthSquared();
    Ang3 angDelta = sDeltAngles(Angs, cur->m_Angle);
    float a = angDelta.x*angDelta.x + angDelta.y*angDelta.y + angDelta.z*angDelta.z;
    float so = 0;
    if (bReflect)
      so = (cur->m_ObjPos - objPos).GetLengthSquared();
    if (s <= dist && a <= adist && so <= distO)
    {
      dist = s;
      adist = a;
      distO = so;
      firstForUse = i;
      if (!so && !s && !a)
        break;
    }
    if (cur->m_pTex && !cur->m_pTex->m_pTexture && firstFree < 0)
      firstFree = i;
  }
  if (bMustExist && firstForUse >= 0)
    return &CTexture::s_EnvTexts[firstForUse];
  if (bReflect)
    dist = distO;

  float curTime = iTimer->GetCurrTime();
  int nUpdate = -2;
  float fTimeInterval = dist * CRenderer::CV_r_envtexupdateinterval + CRenderer::CV_r_envtexupdateinterval*0.5f;
  float fDelta = curTime - CTexture::s_EnvTexts[firstForUse].m_TimeLastUpdated;
  if (bMustExist)
    nUpdate = -2;
  else
  if (dist > MAX_ENVTEXSCANDIST)
  {
    if (firstFree >= 0)
      nUpdate = firstFree;
    else
      nUpdate = -1;
  }
  else
  if (fDelta > fTimeInterval)
    nUpdate = firstForUse;
  if (nUpdate == -2)
  {
    // No need to update (Up to date)
    return &CTexture::s_EnvTexts[firstForUse];
  }
	if(!CTexture::s_EnvTexts[nUpdate].m_pTex)
		return NULL;
  if (nUpdate >= 0)
  {
    if (!CTexture::s_EnvTexts[nUpdate].m_pTex->m_pTexture || gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvTextUpdateTime < 0.1f)
    {
      int n = nUpdate;
      CTexture::s_EnvTexts[n].m_TimeLastUpdated = curTime;
      CTexture::s_EnvTexts[n].m_CamPos = Pos;
      CTexture::s_EnvTexts[n].m_Angle = Angs;
      CTexture::s_EnvTexts[n].m_ObjPos = objPos;
      CTexture::s_EnvTexts[n].m_bReflected = bReflect;
      if (bMustUpdate)
        *bMustUpdate = true;
    }
    gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvTextUpdateTime += iTimer->GetAsyncCurTime()-time0;
    return &CTexture::s_EnvTexts[nUpdate];
  }

  dist = 0;
  firstForUse = -1;
  for (i=0; i<MAX_ENVTEXTURES; i++)
  {
    SEnvTexture *cur = &CTexture::s_EnvTexts[i];
    if (dist < curTime-cur->m_TimeLastUpdated && !cur->m_bInprogress)
    {
      dist = curTime - cur->m_TimeLastUpdated;
      firstForUse = i;
    }
  }
  if (firstForUse < 0)
  {
    return NULL;
  }
  int n = firstForUse;
  CTexture::s_EnvTexts[n].m_TimeLastUpdated = curTime;
  CTexture::s_EnvTexts[n].m_CamPos = Pos;
  CTexture::s_EnvTexts[n].m_ObjPos = objPos;
  CTexture::s_EnvTexts[n].m_Angle = Angs;
  CTexture::s_EnvTexts[n].m_bReflected = bReflect;
  if (bMustUpdate)
    *bMustUpdate = true;

  gRenDev->m_RP.m_PS[gRenDev->m_RP.m_nProcessThreadID].m_fEnvTextUpdateTime += iTimer->GetAsyncCurTime()-time0;
  return &CTexture::s_EnvTexts[n];

}


//===========================================================================

void CTexture::ShutDown()
{
  uint32 i;

  if(gRenDev->GetRenderType()==eRT_Null)	// workaround to fix crash when quitting the dedicated server - because the textures are shared
    return;																											// should be fixed soon

	FlushAllStreamingTasks();

  SAFE_RELEASE_FORCE(s_ptexWhite);
  SAFE_RELEASE_FORCE(s_ptexBlack);
  SAFE_RELEASE_FORCE(s_ptexFlatBump);
	SAFE_RELEASE_FORCE(s_ptexMipMapDebug);
	SAFE_RELEASE_FORCE(s_ptexColorBlue);
	SAFE_RELEASE_FORCE(s_ptexColorCyan);
	SAFE_RELEASE_FORCE(s_ptexColorGreen);
	SAFE_RELEASE_FORCE(s_ptexColorPurple);
	SAFE_RELEASE_FORCE(s_ptexColorRed);
	SAFE_RELEASE_FORCE(s_ptexColorWhite);
	SAFE_RELEASE_FORCE(s_ptexColorYellow);
  SAFE_RELEASE_FORCE(s_ptexPaletteDebug);
  SAFE_RELEASE_FORCE(s_ptexScreenNoiseMap);
  SAFE_RELEASE_FORCE(s_ptexVignettingMap);	
  SAFE_RELEASE_FORCE(s_ptexRT_2D);
  SAFE_RELEASE_FORCE(s_ptexRT_CM);
  SAFE_RELEASE_FORCE(s_ptexNoiseVolumeMap);
  SAFE_RELEASE_FORCE(s_ptexVectorNoiseVolMap);
  SAFE_RELEASE_FORCE(s_ptexCloudsLM);
  SAFE_RELEASE_FORCE(s_ptexWaterOcean);
  SAFE_RELEASE_FORCE(s_ptexWaterVolumeDDN);
  SAFE_RELEASE_FORCE(s_ptexWaterVolumeTemp);
  SAFE_RELEASE_FORCE(s_ptexWaterPuddles[0]);
  SAFE_RELEASE_FORCE(s_ptexWaterPuddles[1]);
  SAFE_RELEASE_FORCE(s_ptexWaterPuddlesDDN);
  SAFE_RELEASE_FORCE(s_ptexSceneNormalsMap);
	SAFE_RELEASE_FORCE(s_ptexSceneDiffuseAccMap);
	SAFE_RELEASE_FORCE(s_ptexSceneSpecularAccMap);
	SAFE_RELEASE_FORCE(s_ptexSceneTexturesMap);

  for (i=0; i<8; i++)
  {
    SAFE_RELEASE_FORCE(s_ptexFromRE[i]);
  }
  for (i=0; i<8; i++)
  {
    SAFE_RELEASE_FORCE(s_ptexShadowID[i]);
  }

	for (i=0; i<2; i++)
	{
		SAFE_RELEASE_FORCE(s_ptexFromRE_FromContainer[i]);
	}

	for (i=0; i<NUM_SCALEFORM_TEXTURES; ++i)
	{
		SAFE_RELEASE_FORCE(s_ptexText_FromSF[i]);
	}
  SAFE_RELEASE_FORCE(s_ptexFromObj);

	SAFE_RELEASE_FORCE(s_ptexVolObj_Density);
	SAFE_RELEASE_FORCE(s_ptexVolObj_Shadow);

	SAFE_RELEASE_FORCE(s_ptexColorChart);

  SAFE_RELEASE_FORCE(s_ptexFromLight);
  SAFE_RELEASE_FORCE(s_ptexFlare);  
  SAFE_RELEASE_FORCE(s_ptexScreenMap);




	// Irradiance volumes textures release
	SAFE_RELEASE_FORCE(s_ptexIrrVolumeNormalMap);
	SAFE_RELEASE_FORCE(s_ptexIrrVolumeColorMap);
	SAFE_RELEASE_FORCE(s_ptexIrrVolumeDepthMap);
	for (i=0; i<3; i++)
	{
		SAFE_RELEASE_FORCE(s_ptexIrrVolumeRT[i]);
	}

  //for (i=0; i<MAX_ENVCUBEMAPS; i++)
  //{
  //  s_EnvCMaps[i].Release();
  //}
  //for (i=0; i<MAX_ENVTEXTURES; i++)
  //{
  //  s_EnvTexts[i].Release();
  //}
  SAFE_RELEASE(s_ptexNoTexture);
  //m_EnvTexTemp.Release();

	SAFE_RELEASE(s_ptexAOTarget);
	SAFE_RELEASE(s_ptexSSGITarget);

  if (CRenderer::CV_r_releaseallresourcesonexit)
  {
    SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
    if (pRL)
    {
      int n = 0;
      ResourcesMapItor itor;
      for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); )
      {
        CTexture *pTX = (CTexture *)itor->second;
        itor++;
        if (!pTX)
          continue;
        if (CRenderer::CV_r_printmemoryleaks)
          iLog->Log("Warning: CTexture::ShutDown: Texture %s was not deleted (%d)", pTX->GetName(), pTX->GetRefCounter());
        SAFE_RELEASE_FORCE(pTX);
        n++;
      }
    }
  }

	//AUTO_LOCK(STexPool::s_csPoolLock);
	for (i=0; i < s_TexturesPools.Num(); i++)
	{
		STexPool *pPool = s_TexturesPools[i];
		SAFE_DELETE(pPool);
	}
	s_TexturesPools.clear();
}

bool CTexture::ReloadFile(const char *szFileName)
{
  char realNameBuffer[256];	
  fpConvertDOSToUnixName(realNameBuffer, szFileName);

	char gameFolderPath[256];
	strcpy(gameFolderPath, PathUtil::GetGameFolder());
	int gameFolderPathLength = strlen(gameFolderPath);
	if (gameFolderPathLength > 0 && gameFolderPath[gameFolderPathLength - 1] == '\\')
	{
		gameFolderPath[gameFolderPathLength - 1] = '/';
	}
	else if (gameFolderPathLength > 0 && gameFolderPath[gameFolderPathLength - 1] != '/')
	{
		gameFolderPath[gameFolderPathLength++] = '/';
		gameFolderPath[gameFolderPathLength] = 0;
	}

	char* realName = realNameBuffer;
	if (strlen(realNameBuffer) >= (uint32)gameFolderPathLength && memcmp(realName, gameFolderPath, gameFolderPathLength) == 0)
		realName += gameFolderPathLength;

  SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
  if (pRL)
  {
    ResourcesMapItor itor;
    for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++)
    {
      CTexture *tp = (CTexture *)itor->second;
      if (!tp)
        continue;

      if (!(tp->m_nFlags & FT_FROMIMAGE))
        continue;
			char srcNameBuffer[MAX_PATH + 1];
		  fpConvertDOSToUnixName(srcNameBuffer, tp->m_SrcName.c_str());
			char* srcName = srcNameBuffer;
			if (strlen(srcName) >= (uint32)gameFolderPathLength && _strnicmp(srcName, gameFolderPath, gameFolderPathLength) == 0)
				srcName += gameFolderPathLength;
			//CryLogAlways("realName = %s srcName = %s gameFolderPath = %s szFileName = %s", realName, srcName, gameFolderPath, szFileName);
      if (!stricmp(realName,srcName))
        return tp->Reload();
    }
  }

  return false;
}

void CTexture::ReloadTextures()
{
  SResourceContainer *pRL = CBaseResource::GetResourcesForClass(CTexture::mfGetClassName());
  if (pRL)
  {
    ResourcesMapItor itor;
    int nID = 0;
    for (itor=pRL->m_RMap.begin(); itor!=pRL->m_RMap.end(); itor++, nID++)
    {
      CTexture *tp = (CTexture *)itor->second;
      if (!tp)
        continue;
      if (!(tp->m_nFlags & FT_FROMIMAGE))
        continue;
      tp->Reload();
    }
  }
}

bool CTexture::SetNoTexture()
{
  if (s_ptexNoTexture)
  {
    m_pDevTexture = s_ptexNoTexture->m_pDevTexture;
#if defined (DIRECT3D10)
    m_pDeviceShaderResource = s_ptexNoTexture->m_pDeviceShaderResource;
#endif
    m_eTFSrc = s_ptexNoTexture->GetSrcFormat();
    m_eTFDst = s_ptexNoTexture->GetDstFormat();
    m_nMips = s_ptexNoTexture->GetNumMips();
    m_nWidth = s_ptexNoTexture->GetWidth();
    m_nHeight = s_ptexNoTexture->GetHeight();
    m_nDepth = 1;
    m_nDefState = s_ptexNoTexture->m_nDefState;

    m_bNoTexture = true;
		Unlink();
    m_nFlags |= FT_FAILED;
    m_nDeviceSize = 0;
    return true;
  }
  return false;
}

void CTexture::SetGridTexture(int nTUnit, CTexture *tp, int nTSlot)
{
  uint32 i, j;
  if (tp->GetWidth() < 8 && tp->GetHeight() < 8)
  {
    CTexture::s_ptexNoTexture->Apply();
    return;
  }
  for (i=0; i<m_TGrids.Num(); i++)
  {
    STexGrid *tg = &m_TGrids[i];
    if (tg->m_Width == tp->GetWidth() && tg->m_Height == tp->GetHeight() && tg->m_TP->GetTextureType() == tp->GetTextureType())
      break;
  }
  if (i != m_TGrids.Num())
  {
    m_TGrids[i].m_TP->Apply(nTUnit);
    return;
  }
  STexGrid tg;
  tg.m_Width = tp->GetWidth();
  tg.m_Height = tp->GetHeight();
  int nFlags = 0;
  if (nTSlot == EFTT_BUMP)
    nFlags |= FT_TEX_NORMAL_MAP;
  CTexture *tpg = ForName("Textures/Defaults/world_texel.tga", nFlags, eTF_Unknown);
  if (tpg->GetWidth() == tp->GetWidth() && tpg->GetHeight() == tp->GetHeight() && tpg->GetTextureType() == tp->GetTextureType())
  {
    tg.m_TP = tpg;
    tg.m_TP->Apply(nTUnit);
    m_TGrids.AddElem(tg);
    return;
  }
  byte *src = tpg->GetData32();
  uint32 wdt = tg.m_Width;
  uint32 hgt = tg.m_Height;
  int wdt1 = tpg->GetWidth();
  int hgt1 = tpg->GetHeight();

  SAFE_RELEASE(tpg);
  if (!src)
  {
    CTexture::s_ptexNoTexture->Apply();
    return;
  }
  byte *dst = new byte[tg.m_Width*tg.m_Height*4];

  int mwdt1 = wdt1-1;
  int mhgt1 = hgt1-1;
  for (j=0; j<hgt; j++)
  {
    for (i=0; i<wdt; i++)
    {
      dst[j*wdt*4+i*4+0] = src[(i&mwdt1)*4+(j&mhgt1)*wdt1*4+0];
      dst[j*wdt*4+i*4+1] = src[(i&mwdt1)*4+(j&mhgt1)*wdt1*4+1];
      dst[j*wdt*4+i*4+2] = src[(i&mwdt1)*4+(j&mhgt1)*wdt1*4+2];
      dst[j*wdt*4+i*4+3] = src[(i&mwdt1)*4+(j&mhgt1)*wdt1*4+3];
    }
  }
  char name[128];
  sprintf(name, "TexGrid_%d_%d", wdt, hgt);
  tg.m_TP = Create2DTexture(name, wdt, hgt, 1, FT_FORCE_MIPS, dst, eTF_A8R8G8B8, eTF_A8R8G8B8);
  delete [] dst;
  delete [] src;
  tg.m_TP->Apply(nTUnit);
  m_TGrids.AddElem(tg);
}

//===============================================================================

bool WriteJPG(byte *dat, int wdt, int hgt, const char *name);
bool WriteDDS(byte *dat, int wdt, int hgt, int Size, const char *name, ETEX_Format eF, int NumMips);


byte *sDDSData;
static TArray<byte> sDDSAData;
void WriteDTXnFile (DWORD count, void *buffer, void * userData)
{
  int n = sDDSAData.Num();
  sDDSAData.Grow(count);
  cryMemcpy(&sDDSAData[n], buffer, count);
}


//===========================================================================

void CTexture::LoadDefaultTextures()
{
  char str[256];
  int i;

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Engine textures");

  s_ptexNoTexture = CTexture::ForName("Shaders/EngineAssets/TextureMsg/ReplaceMe.tif", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  s_ptexWhite = CTexture::ForName("Textures/Defaults/White.dds",  FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown);
  s_ptexGray = CTexture::ForName("Textures/Defaults/Grey.dds", FT_DONT_RELEASE| FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  s_ptexBlack = CTexture::ForName("Textures/Defaults/Black.dds", FT_DONT_RELEASE | FT_DONT_STREAM| FT_DONT_RESIZE, eTF_Unknown);
  s_ptexFlatBump = CTexture::ForName("Textures/Defaults/White_ddn.dds", FT_DONT_RELEASE | FT_TEX_NORMAL_MAP | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown);
	s_ptexMipMapDebug = CTexture::ForName("Shaders/EngineAssets/TextureMsg/MipMapDebug.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM | FT_FILTER_BILINEAR, eTF_Unknown);
	s_ptexColorBlue = CTexture::ForName("Shaders/EngineAssets/TextureMsg/color_Blue.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  s_ptexColorCyan = CTexture::ForName("Shaders/EngineAssets/TextureMsg/color_Cyan.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
	s_ptexColorGreen = CTexture::ForName("Shaders/EngineAssets/TextureMsg/color_Green.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
	s_ptexColorPurple = CTexture::ForName("Shaders/EngineAssets/TextureMsg/color_Purple.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
	s_ptexColorRed = CTexture::ForName("Shaders/EngineAssets/TextureMsg/color_Red.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
	s_ptexColorWhite = CTexture::ForName("Shaders/EngineAssets/TextureMsg/color_White.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
	s_ptexColorYellow = CTexture::ForName("Shaders/EngineAssets/TextureMsg/color_Yellow.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  s_ptexPaletteDebug = CTexture::ForName("Textures/Defaults/palletteInst.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  s_ptexPaletteTexelsPerMeter = CTexture::ForName("Textures/Defaults/TexelsPerMeterGrad.dds", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  
	s_ptexIconShaderCompiling = CTexture::ForName("Shaders/EngineAssets/Icons/ShaderCompiling.tif",  FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  s_ptexIconStreaming = CTexture::ForName("Shaders/EngineAssets/Icons/Streaming.tif",  FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
  s_ptexIconStreamingTerrainTexture = CTexture::ForName("Shaders/EngineAssets/Icons/StreamingTerrain.tif",  FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
	s_ptexIconNullSoundSystem = CTexture::ForName("Shaders/EngineAssets/Icons/NullSoundSystem.tif",  FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);
	s_ptexIconNullMusicSystem = CTexture::ForName("Shaders/EngineAssets/Icons/NullMusicSystem.tif",  FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);

  //	s_ptexScreenNoiseMap = CTexture::ForName("Textures/Defaults/dither_pattern.dds",  FT_DONT_RELEASE, eTF_Unknown);
  s_ptexScreenNoiseMap = CTexture::ForName("Textures/Defaults/JumpNoiseHighFrequency_x27y19.dds",  FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);

  s_ptexVignettingMap = CTexture::ForName("Shaders/EngineAssets/Shading/vignetting.tif", FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM, eTF_Unknown);

  // fixme: get texture resolution from CREWaterOcean 
  s_ptexWaterOcean = CTexture::CreateTextureObject("$WaterOceanMap", 64, 64, 1, eTT_2D, FT_DONT_RELEASE | FT_NOMIPS | FT_USAGE_DYNAMIC|FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_WATEROCEANMAP);
  s_ptexWaterVolumeTemp = CTexture::CreateTextureObject("$WaterVolumeTemp", 64, 64, 1, eTT_2D, FT_DONT_RELEASE | FT_NOMIPS | FT_USAGE_DYNAMIC|FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown);
  s_ptexWaterVolumeDDN = CTexture::CreateTextureObject("$WaterVolumeDDN", 64, 64, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown,  TO_WATERVOLUMEMAP);

  // Default Template textures
  int nRTFlags =  FT_DONT_RELEASE | FT_DONT_STREAM | FT_STATE_CLAMP | FT_USAGE_RENDERTARGET | FT_DONT_RESIZE;
  s_ptexScreenMap = CTexture::CreateTextureObject("$ScreenTexMap", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown, TO_SCREENMAP);
  s_ptexMipColors_Diffuse = CTexture::CreateTextureObject("$MipColors_Diffuse", 0, 0, 1, eTT_2D,  FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_MIPCOLORS_DIFFUSE);
  s_ptexMipColors_Bump = CTexture::CreateTextureObject("$MipColors_Bump", 0, 0, 1, eTT_2D,  FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_MIPCOLORS_BUMP);

  s_ptexRT_2D = CTexture::CreateTextureObject("$RT_2D", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_RT_2D);
  s_ptexRT_CM = CTexture::CreateTextureObject("$RT_CM", 0, 0, 1, eTT_AutoCube, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_RT_CM);
  s_ptexFromLight = CTexture::CreateTextureObject("$FromLightCM", 0, 0, 1, eTT_Cube, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_FROMLIGHT);

  CTexture::s_ptexWaterPuddles[0] = CTexture::CreateTextureObject("$WaterPuddlesPrev", 256, 256, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown);
  CTexture::s_ptexWaterPuddles[1] = CTexture::CreateTextureObject("$WaterPuddlesNext", 256, 256, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown);
  CTexture::s_ptexWaterPuddlesDDN = CTexture::CreateTextureObject("$WaterPuddlesDDN", 256, 256, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_WATERPUDDLESMAP);

  s_ptexFromObj = CTexture::CreateTextureObject("FromObj", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_FROMOBJ);

  s_ptexZTarget = CTexture::CreateTextureObject("$ZTarget", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM| FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown);

  s_ptexZTargetMS = CTexture::CreateTextureObject("$ZTargetMS", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM| FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_ZTARGET_MS);

  s_ptexZTargetScaled = CTexture::CreateTextureObject("$ZTargetScaled", 0, 0, 1, eTT_2D, 
      FT_DONT_RELEASE | FT_DONT_STREAM| FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_DOWNSCALED_ZTARGET_FOR_AO);

  s_ptexSceneTarget = CTexture::CreateTextureObject("$SceneTarget", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_SCENE_TARGET);
  s_ptexCurrSceneTarget = s_ptexSceneTarget;
	
	s_ptexSceneDiffuseAccMap = CTexture::CreateTextureObject("$SceneDiffuseAcc", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_SCENE_DIFFUSE_ACC);	
  s_ptexCurrentSceneDiffuseAccMap = s_ptexSceneDiffuseAccMap;
	s_ptexSceneSpecularAccMap = CTexture::CreateTextureObject("$SceneSpecularAcc", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_SCENE_SPECULAR_ACC);		

	s_ptexSceneTexturesMap = CTexture::CreateTextureObject("$SceneTextures", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_SCENE_TEXTURES);	
  s_ptexSceneNormalsMap = CTexture::CreateTextureObject("$SceneNormalsMap", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_SCENE_NORMALMAP);

  s_ptexBackBufferScaled[0] = CTexture::CreateTextureObject("$BackBufferScaled_d2", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_BACKBUFFERSCALED_D2);
  s_ptexBackBufferScaled[1] = CTexture::CreateTextureObject("$BackBufferScaled_d4", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_BACKBUFFERSCALED_D4);
  s_ptexBackBufferScaled[2] = CTexture::CreateTextureObject("$BackBufferScaled_d8", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME | FT_DONT_RESIZE, eTF_Unknown, TO_BACKBUFFERSCALED_D8);

	s_ptexRT_ShadowPool = CTexture::CreateTextureObject("$RT_ShadowPool", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RESIZE, eTF_Unknown);

	s_ptexRT_NULL = CTexture::CreateTextureObject("$RT_NULL", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_USAGE_RENDERTARGET | FT_DONT_RESIZE, eTF_Unknown);





  // Create dummy texture object for terrain and clouds lightmap
  s_ptexCloudsLM = CTexture::CreateTextureObject("$CloudsLM", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_CLOUDS_LM);

  s_ptexScatterLayer = CTexture::CreateTextureObject("$ScatterLayer", 0, 0, 1, eTT_2D, nRTFlags, eTF_A16B16G16R16F, TO_SCATTER_LAYER, 95);

	s_ptexHDRTargetEncoded = CTexture::CreateTextureObject("$HDRTargetEncoded", 0, 0, 1, eTT_2D, nRTFlags | FT_USAGE_PREDICATED_TILING, eTF_Unknown, TO_HDRTARGET_ENCODED);

  for (i=0; i<MAX_REND_LIGHT_GROUPS; i++)
  {
    sprintf(str, "$ScreenShadowMap_%d", i);
    s_ptexScreenShadowMap[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, nRTFlags, eTF_A8R8G8B8, TO_SCREENSHADOWMAP, 95);

    s_ptexCurrentScreenShadowMap[i] = s_ptexScreenShadowMap[i];
  }

	s_ptexAOTarget = CTexture::CreateTextureObject("$AOTarget", 0, 0, 1, eTT_2D, nRTFlags, eTF_A8R8G8B8, TO_SCREEN_AO_TARGET, 95);

	s_ptexSSGITarget = CTexture::CreateTextureObject("$SSGITarget", 0, 0, 1, eTT_2D, nRTFlags, eTF_A8R8G8B8, TO_SCREEN_SSGI_TARGET, 95);

  for (i=0; i<4; i++)
  {
    if (i)
      sprintf(str, "$LightInfo_%d", i);
    else
      strcpy(str, "$LightInfo");
    uint32 nFlags =  FT_DONT_STREAM| FT_USAGE_DYNAMIC | FT_DONT_RELEASE | FT_DONT_RESIZE;
    s_ptexLightInfo[i] = CTexture::CreateTextureObject(str, 64, 8, 1, eTT_2D, nFlags, eTF_A32B32G32R32F, TO_LIGHTINFO);
  }

  for (i=0; i<8; i++)
  {
    sprintf(str, "$FromRE_%d", i);
    s_ptexFromRE[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_FROMRE0+i);
  }

  for (i=0; i<8; i++)
  {
    sprintf(str, "$ShadowID_%d", i);
    s_ptexShadowID[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_SHADOWID0+i);
  }

	for (i=0; i<2; i++)
	{
		sprintf(str, "$FromRE%d_FromContainer", i);
		s_ptexFromRE_FromContainer[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_FROMRE0_FROM_CONTAINER+i);
	}

	for (i=0; i<NUM_SCALEFORM_TEXTURES; ++i)
	{
		sprintf(str, "$FromSF%d", i);
		s_ptexText_FromSF[i] = CTexture::CreateTextureObject(str, 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_FROMSF0+i);
	}

	s_ptexVolObj_Density = CTexture::CreateTextureObject("$VolObj_Density", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_VOLOBJ_DENSITY);
	s_ptexVolObj_Shadow = CTexture::CreateTextureObject("$VolObj_Shadow", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_VOLOBJ_SHADOW);

	s_ptexColorChart = CTexture::CreateTextureObject("$ColorChart", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_COLORCHART);

	s_ptexSkyDomeMie = CTexture::CreateTextureObject("$SkyDomeMie", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_SKYDOME_MIE);
	s_ptexSkyDomeRayleigh = CTexture::CreateTextureObject("$SkyDomeRayleigh", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_SKYDOME_RAYLEIGH);
	s_ptexSkyDomeMoon = CTexture::CreateTextureObject("$SkyDomeMoon", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, eTF_Unknown, TO_SKYDOME_MOON);

	// Unused atm but kept for reference, we might revive dynamic cube maps in future
  //for (i=0; i<MAX_ENVCUBEMAPS; i++)
  //{
  //  sprintf(str, "$EnvCMap_%d", i);
  //  s_EnvCMaps[i].m_Id = i;
  //  s_EnvCMaps[i].m_pTex = new SDynTexture(0, 0, eTF_A8R8G8B8, eTT_Cube, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, str);
  //}  

  //for (i=0; i<MAX_ENVTEXTURES; i++)
  //{
  //  sprintf(str, "$EnvTex_%d", i);
  //  s_EnvTexts[i].m_Id = i;
  //  s_EnvTexts[i].m_pTex = new SDynTexture(0, 0, eTF_A8R8G8B8, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_RESIZE, "EnvLCMap");
  //}  

  for (i=0; i<EFTT_MAX; i++)
  {
    s_ShaderTemplates[i].SetCustomID(EFTT_DIFFUSE + i);
    s_ShaderTemplates[i].SetFlags(FT_DONT_RELEASE);
  }

	// Irradiance volumes textures init
	s_ptexIrrVolumeRT[0] = CTexture::CreateTextureObject("$IrradVolumeRed", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown, TO_IRRADVOLUME_R);
	s_ptexIrrVolumeRT[1] = CTexture::CreateTextureObject("$IrradVolumeGreen", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown, TO_IRRADVOLUME_G);
	s_ptexIrrVolumeRT[2] = CTexture::CreateTextureObject("$IrradVolumeBlue", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown, TO_IRRADVOLUME_B);
	s_ptexIrrVolumeNormalMap = CTexture::CreateTextureObject("$IrradVolumeNormal", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown, TO_IRRADVOLUME_NORMAL);
	s_ptexIrrVolumeColorMap = CTexture::CreateTextureObject("$IrradVolumeColor", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown, TO_IRRADVOLUME_COLOR);
	s_ptexIrrVolumeDepthMap = CTexture::CreateTextureObject("$IrradVolumeDepth", 0, 0, 1, eTT_2D, nRTFlags, eTF_Unknown, TO_IRRADVOLUME_DEPTH);

  // Stereo
  if (CRenderer::CV_r_Stereo)
  {
		if (CRenderer::CV_r_Stereo == 4)
		{
			int width = gRenDev->GetWidth();
			int height = gRenDev->GetHeight();
			s_ptexStereoL = CreateTextureObject( "$StereoL", width / 2, height, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME, eTF_Unknown );
			s_ptexStereoR = CreateTextureObject( "$StereoR", width / 2, height, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME, eTF_Unknown );
		}
		else
		{
			s_ptexStereoL = CreateTextureObject( "$StereoL", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME, eTF_Unknown );
			s_ptexStereoR = CreateTextureObject( "$StereoR", 0, 0, 1, eTT_2D, FT_DONT_RELEASE | FT_DONT_STREAM | FT_DONT_GENNAME, eTF_Unknown );
		}
  }




  GenerateFuncTextures();
}

//////////////////////////////////////////////////////////////////////////
const char* CTexture::GetFormatName() const
{
	return NameForTextureFormat(GetDstFormat());
}

//////////////////////////////////////////////////////////////////////////
const char* CTexture::GetTypeName() const
{
	return NameForTextureType(GetTextureType());
}

//////////////////////////////////////////////////////////////////////////
CDynTextureSource::CDynTextureSource()
: m_refCount(1)
, m_width(0)
, m_height(0)
, m_lastUpdateTime(0)
, m_lastUpdateFrameID(0)
, m_pDynTexture(0)
{
}

//////////////////////////////////////////////////////////////////////////
CDynTextureSource::~CDynTextureSource()
{
	SAFE_DELETE(m_pDynTexture);
}

//////////////////////////////////////////////////////////////////////////
void CDynTextureSource::AddRef()
{
	++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CDynTextureSource::Release()
{
	--m_refCount;
	if (m_refCount <= 0)
		delete this;
}

//////////////////////////////////////////////////////////////////////////
void CDynTextureSource::CalcSize(int& width, int& height, float distToCamera) const
{
	int size;
	int logSize;
	switch(CRenderer::CV_r_envtexresolution)
	{
	case 0:
	case 1:
		size = 256;
		logSize = 8;
		break;
	case 2:
	case 3:
	default:
		size = 512;
		logSize = 9;
		break;
	}

	if (distToCamera > 0)
	{
		int x(0), y(0), vpwidth(1), vpheight(1);
		gRenDev->GetViewport(&x, &y, &vpwidth, &vpheight);
		float lod = logf(max(distToCamera * 1024.0f / (float) max(vpwidth, vpheight), 1.0f));
		int lodFixed = fastround_positive(lod);
		size = 1 << max(logSize - lodFixed, 5);
	}

	width = size;
	height = size;
}

//////////////////////////////////////////////////////////////////////////
bool CDynTextureSource::Apply(int nTUnit, int nTS)
{
	assert(m_pDynTexture);
	if (!m_pDynTexture || !m_pDynTexture->IsValid())
		return false;

	m_pDynTexture->Apply(nTUnit, nTS);
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CDynTextureSource::GetTexGenInfo(float& offsX, float& offsY, float& scaleX, float& scaleY) const
{
	assert(m_pDynTexture);
	if (!m_pDynTexture || !m_pDynTexture->IsValid())
	{
		offsX = 0;
		offsY = 0;
		scaleX = 1;
		scaleY = 1;
		return;
	}

	ITexture* pSrcTex(m_pDynTexture->GetTexture());
	float invSrcWidth(1.0f / (float) pSrcTex->GetWidth());
	float invSrcHeight(1.0f / (float) pSrcTex->GetHeight());
	offsX  = m_pDynTexture->m_nX * invSrcWidth;
	offsY  = m_pDynTexture->m_nY * invSrcHeight;
	assert(m_width <= m_pDynTexture->m_nWidth && m_height <= m_pDynTexture->m_nHeight);
	scaleX = m_width * invSrcWidth;
	scaleY = m_height * invSrcHeight;
}

//////////////////////////////////////////////////////////////////////////
void CDynTextureSource::InitDynTexture(ETexPool eTexPool)
{
	CalcSize(m_width, m_height);
	m_pDynTexture = new SDynTexture2(m_width, m_height, FT_STATE_CLAMP|FT_NOMIPS, "DynTextureSource", eTexPool);
}

//////////////////////////////////////////////////////////////////////////
CFlashTextureSource::CFlashTextureSource(const char* pFlashFileName)
: CDynTextureSource()
, m_pFlashPlayer(0)
, m_highQualityRender(false)
#ifdef _DEBUG
, m_flashSrcFile(pFlashFileName)
#endif
{
	if (pFlashFileName)
	{
		m_pFlashPlayer = gEnv->pSystem->CreateFlashPlayerInstance();
		if (m_pFlashPlayer)
		{
			if (m_pFlashPlayer->Load(pFlashFileName, IFlashPlayer::DEFAULT_NO_MOUSE))
			{
				size_t buffSize = m_pFlashPlayer->GetMetadata(0, 0);
				if (buffSize)
				{
					char* pMetaData = new char[buffSize];
					if (pMetaData)
					{
						m_pFlashPlayer->GetMetadata(pMetaData, buffSize);
						m_highQualityRender = strstr(pMetaData, "CE_HQR") != 0;
					}
					delete [] pMetaData;
				}
			}
			else
			{
				SAFE_RELEASE(m_pFlashPlayer);
			}
		}
	}
	InitDynTexture(eTP_DynTexSources);
}

//////////////////////////////////////////////////////////////////////////
CFlashTextureSource::~CFlashTextureSource()
{
	SAFE_RELEASE(m_pFlashPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CFlashTextureSource::GetDynTextureSource(void*& pIDynTextureSource, EDynTextureSource& dynTextureSource)
{
	pIDynTextureSource = m_pFlashPlayer;
	dynTextureSource = DTS_I_FLASHPLAYER;
}

//////////////////////////////////////////////////////////////////////////
void CFlashTextureSource::CalcSize(int& width, int& height, float distToCamera) const
{
	if (m_pFlashPlayer && m_highQualityRender)
	{
		float s = CRenderer::CV_r_FlashMatTexResQuality < 0.1f ? 0.1f : CRenderer::CV_r_FlashMatTexResQuality;
		width = clamp_tpl((int) (m_pFlashPlayer->GetWidth() * s), 16, (int) SDynTexture::s_CurTexAtlasSize);
		height = clamp_tpl((int) (m_pFlashPlayer->GetHeight() * s), 16, (int) SDynTexture::s_CurTexAtlasSize);
	}
	else
		CDynTextureSource::CalcSize(width, height, distToCamera);
}

//////////////////////////////////////////////////////////////////////////
CVideoTextureSource::CVideoTextureSource(const char* pVideoFileName)
: CDynTextureSource()
, m_pVideoPlayer(0)
#ifdef _DEBUG
, m_videoSrcFile(pVideoFileName)
#endif
{
	if (pVideoFileName)
	{
		m_pVideoPlayer = gRenDev->CreateVideoPlayerInstance();
		if (m_pVideoPlayer)
		{
			if (!m_pVideoPlayer->Load(pVideoFileName, IVideoPlayer::LOOP_PLAYBACK|IVideoPlayer::DELAY_START))
			{
				SAFE_RELEASE(m_pVideoPlayer);
			}
		}
	}
	InitDynTexture(eTP_DynTexSources);
}

//////////////////////////////////////////////////////////////////////////
CVideoTextureSource::~CVideoTextureSource()
{
	SAFE_RELEASE(m_pVideoPlayer);
}

//////////////////////////////////////////////////////////////////////////
void CVideoTextureSource::GetDynTextureSource(void*& pIDynTextureSource, EDynTextureSource& dynTextureSource)
{	
	pIDynTextureSource = m_pVideoPlayer;
	dynTextureSource = DTS_I_VIDEOPLAYER;
}

//////////////////////////////////////////////////////////////////////////
void CRenderer::EF_AddRTStat(CTexture *pTex, int nFlags, int nW, int nH)
{
  SRTargetStat TS;
  int nSize;
  ETEX_Format eTF;
  if (!pTex)
  {
    eTF = eTF_A8R8G8B8;
    if (nW < 0)
      nW = m_width;
    if (nH < 0)
      nH = m_height;
    nSize = CTexture::TextureDataSize(nW, nH, 1, 1, eTF);
    TS.m_Name = "Back buffer";
  }
  else
  {
    eTF = pTex->GetDstFormat();
    if (nW < 0)
      nW = pTex->GetWidth();
    if (nH < 0)
      nH = pTex->GetHeight();
    nSize = CTexture::TextureDataSize(nW, nH, 1, pTex->GetNumMips(), eTF);
    const char *szName = pTex->GetName();
    if (szName && szName[0] == '$')
      TS.m_Name = string("@") + string(&szName[1]);
    else
      TS.m_Name = szName;
  }
  TS.m_eTF = eTF;

  if (nFlags > 0)
  {
    if (nFlags == 1)
      TS.m_Name += " (Target)";
    else
    if (nFlags == 2)
    {
      TS.m_Name += " (Depth)";
      nSize = nW * nH * 3;
    }
    else
    if (nFlags == 4)
    {
      TS.m_Name += " (Stencil)";
      nSize = nW * nH;
    }
    else
    if (nFlags == 3)
    {
      TS.m_Name += " (Target + Depth)";
      nSize += nW * nH * 3;
    }
    else
    if (nFlags == 6)
    {
      TS.m_Name += " (Depth + Stencil)";
      nSize = nW*nH*4;
    }
    else
    if (nFlags == 5)
    {
      TS.m_Name += " (Target + Stencil)";
      nSize += nW*nH;
    }
    else
    if (nFlags == 7)
    {
      TS.m_Name += " (Target + Depth + Stencil)";
      nSize += nW*nH*4;
    }
    else
    {
      assert(0);
    }
  }
  TS.m_nSize = nSize;
  TS.m_nWidth = nW;
  TS.m_nHeight = nH;

  m_RP.m_RTStats.push_back(TS);
}

void CRenderer::EF_PrintRTStats(const char *szName)
{
  const int nYstep = 14;
  int nY = 30; // initial Y pos
  int nX = 20; // initial X pos
  ColorF col = Col_Green;
  Draw2dLabel((float)nX, (float)nY, 1.6f, &col.r, false, szName);
  nX += 10; nY += 25;

  col = Col_White;
  int nYstart = nY;
  int nSize = 0;
  for (int i=0; i<m_RP.m_RTStats.size(); i++)
  {
    SRTargetStat *pRT = &m_RP.m_RTStats[i];

    Draw2dLabel((float)nX, (float)nY, 1.4f, &col.r, false, "%s (%d x %d x %s), Size: %.3f Mb", pRT->m_Name.c_str(), pRT->m_nWidth, pRT->m_nHeight, CTexture::NameForTextureFormat(pRT->m_eTF), (float)pRT->m_nSize / 1024.0f / 1024.0f);
    nY += nYstep;
    if (nY >= m_height)
    {
      nY = nYstart;
      nX += 300;
    }
    nSize += pRT->m_nSize;
  }
  col = Col_Yellow;
  Draw2dLabel((float)nX, (float)(nY+10), 1.4f, &col.r, false, "Total: %d RT's, Size: %.3f Mb", m_RP.m_RTStats.size(), nSize / 1024.0f / 1024.0f);
}

bool CTexture::IsFSAAChanged()
{
#if defined(DIRECT3D9) || defined(DIRECT3D10) || defined(OPENGL)
  if (m_nFSAASamples != gRenDev->m_RP.m_FSAAData.Type)
    return true;
  if (m_nFSAAQuality != gRenDev->m_RP.m_FSAAData.Quality)
    return true;
#endif
  return false;
}

byte ** CTexture::GetSystemCopy()
{
#ifdef TEXTURE_GET_SYSTEM_COPY_SUPPORT

	if(!m_pImageSystemCopy)
	{
		AUTO_LOCK(m_sGetSystemCopyLock);

		if(!m_pImageSystemCopy)
		{
			iLog->Log("CTexture::GetSystemCopy: Dim=%dx%d, Name=%s ...", GetWidth(), GetHeight(), GetName());

      // make sure texture is loaded
      m_bStreamed = false;
      m_nFlags |= FT_DONT_STREAM;
      Reload();

			m_pImageSystemCopy = new byte*[m_nImageSystemCopyMipsNum];
			memset(m_pImageSystemCopy,0,sizeof(byte*)*m_nImageSystemCopyMipsNum);

			// potential leak!

			byte * pImg = 0;

			if(m_eTFDst == eTF_3DC)
			{
				pImg = GetData32(0,0,0,eTF_DXT5);
				if(!pImg)
					return m_pImageSystemCopy;

				byte *dstData = new byte[m_nWidth*m_nHeight*4];
				gRenDev->DXTDecompress(pImg,m_nWidth*m_nHeight,dstData,m_nWidth, m_nHeight,1,eTF_DXT5, false, 4);

				pImg = dstData;
			}
			else
			{
				pImg = GetData32(0,0,0,eTF_A8R8G8B8);
				if(!pImg)
				{
					iLog->LogError("CTexture::GetSystemCopy: GetData32() failed: Dim=%dx%d, Name=%s ...", GetWidth(), GetHeight(), GetName());
					return m_pImageSystemCopy;
				}
			}

			int nImgDataSize = GetWidth()*GetHeight()*4;
			byte * pImage = new byte[nImgDataSize];   

			memcpy(pImage, pImg, nImgDataSize);

			m_pImageSystemCopy[0] = pImage;

			ColorB * pMipMain = (ColorB *)m_pImageSystemCopy[0];

			for(int nMip=1; (GetWidth()>>nMip) && (GetHeight()>>nMip) && nMip<m_nImageSystemCopyMipsNum; nMip++)
			{
				int nDimMipW = GetWidth()>>nMip;
				int nDimMipH = GetHeight()>>nMip;

				int nSubSize = 1<<nMip;

				m_pImageSystemCopy[nMip] = new byte[nDimMipW*nDimMipH*sizeof(ColorB)];

				ColorB * pMipThis = (ColorB *)m_pImageSystemCopy[nMip];

				int nMaskH = GetHeight()-1;
				int nMaskW = GetWidth()-1;
				int nHeight = GetHeight();

				for(int x=0; x<nDimMipW; x++)
				{
					for(int y=0; y<nDimMipH; y++)
					{
						ColorF colSumm(0,0,0,0);
						float fCount = 0;

						int x1 = x*nSubSize;
						int x2 = x*nSubSize + nSubSize;
						int y1 = y*nSubSize;
						int y2 = y*nSubSize + nSubSize;

						for(int _x = x1; _x < x2; _x ++)
						{
							for(int _y = y1; _y < y2; _y ++)
							{
								int id = (_x&nMaskW)*nHeight + (_y&nMaskH);
								colSumm.r += 1.f/255.f*pMipMain[id].r;
								colSumm.g += 1.f/255.f*pMipMain[id].g;
								colSumm.b += 1.f/255.f*pMipMain[id].b;
								colSumm.a += 1.f/255.f*pMipMain[id].a;
								fCount ++;
							}
						}

						colSumm /= fCount;

						colSumm.Clamp(0.f,1.f);

						pMipThis[x*nDimMipH + y] = colSumm;
					}
				}
			}

			iLog->LogPlus(" ok");
		}
	}

  return m_pImageSystemCopy;

#else

  return NULL;

#endif
}


void CTexture::SetRenderTargetTile( uint8 nTile /*= 0*/ )
{





}

const uint8 CTexture::GetRenderTargetTile() const 
{ 



	return 0;

}; 

#include UNIQUE_VIRTUAL_WRAPPER(ITexture)

