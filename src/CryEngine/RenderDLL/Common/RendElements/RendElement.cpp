/*=============================================================================
  RendElement.cpp : common RE functions.
  Copyright (c) 2001 Crytek Studios. All Rights Reserved.

  Revision history:
    * Created by Honitch Andrey

=============================================================================*/

#include "StdAfx.h"

//TArray<CRendElementBase *> CRendElementBase::m_AllREs;

CRendElement CRendElement::m_RootGlobal;
CRendElement CRendElement::m_RootRelease[4];

//===============================================================

TArray <ColorF> gCurLightStyles;

CryCriticalSection m_sREResLock;

//============================================================================

void CRendElement::ShutDown()
{
  if (!CRenderer::CV_r_releaseallresourcesonexit)
    return;

  AUTO_LOCK(m_sREResLock); // Not thread safe without this

  CRendElement *pRE;
  CRendElement *pRENext;
  for (pRE=CRendElement::m_RootGlobal.m_NextGlobal; pRE!=&CRendElement::m_RootGlobal; pRE=pRENext)
  {
    pRENext = pRE->m_NextGlobal;
    if (CRenderer::CV_r_printmemoryleaks)
      iLog->Log("Warning: CRendElementBase::ShutDown: RenderElement %s was not deleted", pRE->mfTypeString());
    pRE->Release(true);
  }
}

void CRendElement::Tick()
{
#ifndef STRIP_RENDER_THREAD
	assert(gRenDev->m_pRT->IsMainThread(true));
#endif
  int nFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
  int nFrame = nFrameID - 3;
  CRendElement& Root = CRendElement::m_RootRelease[nFrame & 3];
  CRendElement *pRENext = NULL;

  for (CRendElement *pRE=Root.m_NextGlobal; pRE != &Root; pRE=pRENext)
  {
    pRENext = pRE->m_NextGlobal;
    SAFE_DELETE(pRE);
  }
}

CRendElement::CRendElement()
{
#ifdef _DEBUG
  //if (gRenDev && gRenDev->m_pRT)
  //  assert(gRenDev->m_pRT->IsMainThread(true));
#endif
  m_Type = eDATA_Unknown;
  if (!m_RootGlobal.m_NextGlobal)
  {
    m_RootGlobal.m_NextGlobal = &m_RootGlobal;
    m_RootGlobal.m_PrevGlobal = &m_RootGlobal;
    for (int i=0; i<4; i++)
    {
      m_RootRelease[i].m_NextGlobal = &m_RootRelease[i];
      m_RootRelease[i].m_PrevGlobal = &m_RootRelease[i];
    }
  }
}

/*TArray<CRendElement *> RES;

void sValidateRE()
{
  int i;
  CRendElement *pRE = CRendElement::m_RootGlobal.m_NextGlobal;
  while (pRE != &CRendElement::m_RootGlobal)
  {
    for (i=0; i<RES.Num(); i++)
    {
      if (RES[i] == pRE)
        break;
    }
    if (i == RES.Num())
    {
      assert(0);
    }

    pRE = pRE->m_NextGlobal;
  }
  for (i=0; i<RES.Num(); i++)
  {
    CRendElement *pRE = CRendElement::m_RootGlobal.m_NextGlobal;
    while (pRE != &CRendElement::m_RootGlobal)
    {
      if (RES[i] == pRE)
        break;
      pRE = pRE->m_NextGlobal;
    }
    if (pRE == &CRendElement::m_RootGlobal)
    {
      assert(0);
    }
  }
}
void sDeleteRE(CRendElement *pRE)
{
  int i;
  for (i=0; i<RES.Num(); i++)
  {
    if (RES[i] == pRE)
    {
      RES.Remove(i, 1);
      return;
    }
  }
  assert(0);
}
void sAddRE(CRendElement *pRE)
{
int i;
  for (i=0; i<RES.Num(); i++)
  {
    if (RES[i] == pRE)
    {
      assert(0);
      break;
    }
  }
  if (i == RES.Num())
  {
    RES.AddElem(pRE);
  }
}
*/

CRendElement::~CRendElement()
{
  assert(m_Type == eDATA_Unknown || m_Type == eDATA_Particle);

  AUTO_LOCK(m_sREResLock);
  UnlinkGlobal();
}

void CRendElement::Release(bool bForce)
{
  CRendElementBase *pThis = (CRendElementBase *)this;
  pThis->m_Flags |= FCEF_DELETED;

#ifdef _DEBUG
  //if (gRenDev && gRenDev->m_pRT)
  //  assert(gRenDev->m_pRT->IsMainThread(true));
#endif
  m_Type = eDATA_Unknown;
  if (bForce)
  {
    //sDeleteRE(this);
    delete this;
    return;
  }
  int nFrame = gRenDev->GetFrameID(false);

  AUTO_LOCK(m_sREResLock);
  CRendElement& Root = CRendElement::m_RootRelease[nFrame & 3];
  UnlinkGlobal();
  LinkGlobal(&Root);
  //sDeleteRE(this);
}

CRendElementBase::CRendElementBase()
{
#ifdef _DEBUG
  //if (gRenDev && gRenDev->m_pRT)
  //  assert(gRenDev->m_pRT->IsMainThread(true));
#endif
  m_Flags = 0;
  m_CustomData = NULL;
  m_NextGlobal = NULL;
  m_PrevGlobal = NULL;
  int i;
  for(i=0; i<MAX_CUSTOM_TEX_BINDS_NUM; i++)
    m_CustomTexBind[i] = -1;

  //sAddRE(this);

  AUTO_LOCK(m_sREResLock);
  LinkGlobal(&m_RootGlobal);
}
CRendElementBase::~CRendElementBase()
{
#ifdef _DEBUG
  //if (gRenDev && gRenDev->m_pRT)
  //  assert(gRenDev->m_pRT->IsMainThread(true));
#endif
  //assert(SRendItem::m_RecurseLevel <= 1);
  if ((m_Flags & FCEF_ALLOC_CUST_FLOAT_DATA) && m_CustomData)
  {
    delete [] ((float*)m_CustomData);
    m_CustomData=0;
  }
}



void CRendElementBase::mfPrepare()
{
}

CRenderChunk *CRendElementBase::mfGetMatInfo() {return NULL;}
PodArray<CRenderChunk> *CRendElementBase::mfGetMatInfoList() {return NULL;}
int CRendElementBase::mfGetMatId() {return -1;}
void CRendElementBase::mfReset() {}
bool CRendElementBase::mfCullByClipPlane(CRenderObject *pObj) { return false; }

const char *CRendElement::mfTypeString()
{
  switch(m_Type)
  {
    case eDATA_Sky:
      return "Sky";
    case eDATA_Beam:
      return "Beam";
    	break;
    case eDATA_ClientPoly:
      return "ClientPoly";
    	break;
    case eDATA_ClientPoly2D:
      return "ClientPoly2D";
    	break;
    case eDATA_Flare:
      return "Flare";
    	break;
    case eDATA_Terrain:
      return "Terrain";
    	break;
    case eDATA_SkyZone:
      return "SkyZone";
    	break;
    case eDATA_Mesh:
      return "Mesh";
    	break;
    case eDATA_TerrainSector:
      return "TerrainSector";
    	break;
    case eDATA_FarTreeSprites:
      return "FarTreeSprites";
    	break;
    case eDATA_ShadowMapGen:
      return "ShadowMapGen";
    	break;
    case eDATA_TerrainDetailTextureLayers:
      return "TerrainDetailTextureLayers";
    	break;
    case eDATA_TerrainParticles:
      return "TerrainParticles";
    	break;
    case eDATA_Ocean:
      return "Ocean";
    	break;
    case eDATA_PostProcess: 
      return "PostProcess";
      break;
		case eDATA_HDRSky: 
			return "HDRSky";
			break;
		case eDATA_FogVolume: 
			return "FogVolume";
			break;
  }
  return "Unknown";
}

CRendElementBase *CRendElementBase::mfCopyConstruct(void)
{
  CRendElementBase *re = new CRendElementBase;
  *re = *this;
  return re;
}
void CRendElementBase::mfCenter(Vec3& centr, CRenderObject *pObj)
{
  centr(0,0,0);
}
void CRendElementBase::mfGetPlane(Plane& pl)
{
  pl.n = Vec3(0,0,1);
  pl.d = 0;
}

bool CRendElementBase::mfDraw(CShader *ef, SShaderPass *sfm) {return false;}
void *CRendElementBase::mfGetPointer(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags) {return NULL;}
float CRendElementBase::mfDistanceToCameraSquared(Matrix34& matInst) {return 0.1f;}

//=============================================================================

void *SRendItem::mfGetPointerCommon(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags)
{
  int j;
  switch (ePT)
  {      
    case eSrcPointer_Vert:
      *Stride = gRenDev->m_RP.m_Stride;
      return gRenDev->m_RP.m_Ptr.PtrB;

    case eSrcPointer_Color:
      *Stride = gRenDev->m_RP.m_Stride;
      return gRenDev->m_RP.m_Ptr.PtrB + gRenDev->m_RP.m_OffsD;

    case eSrcPointer_Tex:
    case eSrcPointer_TexLM:
      *Stride = gRenDev->m_RP.m_Stride;
      j = ePT - eSrcPointer_Tex;
      return gRenDev->m_RP.m_Ptr.PtrB + gRenDev->m_RP.m_OffsT + j*16;
  }
  return NULL;
}

