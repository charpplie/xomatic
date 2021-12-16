#include "StdAfx.h"
#include "CullBuffer.h"
#include "RoadRenderNode.h"
#include "VoxMan.h"
#include "Brush.h"
#include "RenderMeshMerger.h"
#include "DecalManager.h"
#include "Material.h"
#include "VisAreas.h"
#include "ICryAnimation.h"
#include "LightEntity.h"

#ifdef WIN32
#include <windows.h>
#endif //WIN32

#ifndef PI
#define PI 3.14159f
#endif

#include "CryThread.h"

#define THREAD_NAME "COctreeRenderThread"

namespace
{
//  CryCriticalSection g_cOcTreeTerminate;
//  CryCriticalSection g_cOcTreeNodes;
//  CryCriticalSection g_cOcTreeSleep;
}

struct COctreeRenderThread: CrySimpleThread<>
{
  struct COctreeRenderThreadTask
  {
    CCamera camera;
    int nRenderMask;
    Vec3 vAmbColor;
  };

  COctreeRenderThread(COctreeRenderThreadTask * pTask, int nId)
    : m_pTask(pTask), m_nId(nId)
  {	
    m_ghEventStart = CreateEvent( 
      NULL,   // default security attributes
      FALSE,  // auto-reset event object
      FALSE,  // initial state is nonsignaled
      NULL);  // unnamed object

    assert(m_ghEventStart);

    m_ghEventDone = CreateEvent( 
      NULL,   // default security attributes
      FALSE,  // auto-reset event object
      FALSE,  // initial state is nonsignaled
      NULL);  // unnamed object

    assert(m_ghEventDone);

    Start(0, THREAD_NAME);
  }

  virtual void Run()
  {
    CryThreadSetName( 0, THREAD_NAME );

    while( true )
    {
      WaitForSingleObject( m_ghEventStart, INFINITE );

      ResetEvent(m_ghEventStart);

      for(int i=0; i<m_nNodesCount; i++)
        m_ppFirstNode[i]->RenderContent(m_pTask->camera, m_pTask->nRenderMask, m_pTask->vAmbColor, m_nId);

      m_nNodesCount = 0;
      m_ppFirstNode = NULL;

      SetEvent(m_ghEventDone);
    }
  }

  virtual void Terminate()
  {
//    AUTO_LOCK(g_cOcTreeTerminate);

    m_pTask = NULL;
  }

  COctreeRenderThreadTask * m_pTask;
  HANDLE m_ghEventStart;
  HANDLE m_ghEventDone;
  COctreeNode ** m_ppFirstNode;
  int m_nNodesCount;
  int m_nId;
};

struct SRenderMeshRenderInfo
{
  CRenderObject * pRenderObject;
  IRenderMesh * pRenderMesh;
};

struct SRenderMeshRenderInfoModified
{
  CRenderObject * pRenderObject;
  IRenderMesh * pRenderMesh;
  SRenderObjectModifier roModifier;
};

PodArray<SRenderMeshRenderInfo> gArrSRenderMeshRenderList[nThreadsNum];
PodArray<SRenderMeshRenderInfoModified> gArrSRenderMeshRenderListModified[nThreadsNum];

void RenderArrSRenderMeshRenderList()
{
  FUNCTION_PROFILER_3DENGINE;

  for(int id=0; id<nThreadsNum; id++)
  {
    {
      PodArray<SRenderMeshRenderInfoModified> & arrSRenderMeshRenderList = gArrSRenderMeshRenderListModified[id];

      for(int i=0; i<arrSRenderMeshRenderList.Count(); i++)
      {
        SRenderMeshRenderInfoModified & rInfo = arrSRenderMeshRenderList[i];
        CStatObj::RenderRenderMeshReal( rInfo.pRenderObject, rInfo.pRenderMesh, NULL, &rInfo.roModifier );
      }
      arrSRenderMeshRenderList.Clear();
    }

    {
      PodArray<SRenderMeshRenderInfo> & arrSRenderMeshRenderList = gArrSRenderMeshRenderList[id];

      for(int i=0; i<arrSRenderMeshRenderList.Count(); i++)
      {
        SRenderMeshRenderInfo & rInfo = arrSRenderMeshRenderList[i];
        CStatObj::RenderRenderMeshReal( rInfo.pRenderObject, rInfo.pRenderMesh, NULL, NULL );
      }
      arrSRenderMeshRenderList.Clear();
    }

  }
}

void COctreeNode::RenderMT(const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor)
{
  //  FUNCTION_PROFILER_3DENGINE;

  //GetISystem()->GetIProfilingSystem()->VTuneResume();

  static PodArray<COctreeNode*> arrVisNodes; arrVisNodes.Clear();

  {
    FRAME_PROFILER( "COctreeNode::PreRender", GetSystem(), PROFILE_3DENGINE );
    PreRender_Object_Nodes(false, rCam, nRenderMask, vAmbColor, arrVisNodes, 0);
    if(!arrVisNodes.Count())
      return;

//    for(int i=0 ; i<arrVisNodes.Count(); i++)
  //    arrVisNodes[i]->RenderVisNode(rCam, rCB, nRenderMask, vAmbColor);
    //return true;
  }

  static COctreeRenderThread::COctreeRenderThreadTask task;
  task.camera = rCam;
  task.nRenderMask = nRenderMask;
  task.vAmbColor = vAmbColor;

  static struct COctreeRenderThread * arrThread[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  assert(nThreadsNum<=8);

  int nNodesPerThread = arrVisNodes.Count()/nThreadsNum;

  HANDLE arrEvents[nThreadsNum];

  for(int n=0; n<nThreadsNum; n++)
  {
    if(!arrThread[n])
      arrThread[n] = new COctreeRenderThread(&task, n);





    arrThread[n]->m_ppFirstNode = &arrVisNodes[nNodesPerThread*n];
    arrThread[n]->m_nNodesCount = nNodesPerThread;
    arrEvents[n] = arrThread[n]->m_ghEventDone;
  }

  for(int n=0; n<nThreadsNum; n++)
    SetEvent(arrThread[n]->m_ghEventStart);

#if defined(PS3) || defined(LINUX)
  assert(!"Not supposed to be used on PS3 or LINUX");
#else
  WaitForMultipleObjects( nThreadsNum, arrEvents, TRUE, INFINITE );
#endif

  for(int n=0; n<nThreadsNum; n++)
    ResetEvent(arrThread[n]->m_ghEventDone);

 // GetISystem()->GetIProfilingSystem()->VTunePause();
}

void COctreeNode::RenderNR_Object_Nodes(const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, PodArray<COctreeNode*> * pArrNodes, int8 nThreadId)
{
  //  FUNCTION_PROFILER_3DENGINE;

  if(m_nOccludedFrameId == GetFrameID())
    return;

  bool bNodeCompletelyInFrustum = false;
  if(!rCam.IsAABBVisible_EHM( m_objectsBox, &bNodeCompletelyInFrustum ))
    return;

  const Vec3 vCamPos = rCam.GetPosition();

  float fNodeDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos, m_objectsBox))*m_fZoomFactor;

  if(fNodeDistance > m_fObjectsMaxViewDist)
    return;

  if(m_nLastVisFrameId != GetFrameID())
  {
    if(GetObjManager()->IsBoxOccluded( m_objectsBox, fNodeDistance, &m_occlTestState, m_pVisArea != NULL, eoot_OCCELL ))
    {
      m_nOccludedFrameId = GetFrameID();
      return;
    }
  }

  m_nLastVisFrameId = GetFrameID();

  if(!m_bCompiled)
    CompileObjects();

  m_fNodeDistance = fNodeDistance;
  m_bNodeCompletelyInFrustum = bNodeCompletelyInFrustum;
  CheckManageVegetationSprites(fNodeDistance, (int)(fNodeDistance*0.2f));
  RenderContent(rCam, nRenderMask, vAmbColor, nThreadId);

  int nFirst = 
    ((vCamPos.x > m_vNodeCenter.x) ? 4 : 0) |
    ((vCamPos.y > m_vNodeCenter.y) ? 2 : 0) |
    ((vCamPos.z > m_vNodeCenter.z) ? 1 : 0);

  COctreeNode * arrNewNodes[8];
  int nNewNodesCount=0;

  if(m_arrChilds[nFirst  ])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst  ];

  if(m_arrChilds[nFirst^1])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst^1];

  if(m_arrChilds[nFirst^2])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst^2];

  if(m_arrChilds[nFirst^4])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst^4];

  if(m_arrChilds[nFirst^3])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst^3];

  if(m_arrChilds[nFirst^5])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst^5];

  if(m_arrChilds[nFirst^6])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst^6];

  if(m_arrChilds[nFirst^7])
    arrNewNodes[nNewNodesCount++] = m_arrChilds[nFirst^7];

  if(nNewNodesCount)
    pArrNodes->AddList(arrNewNodes,nNewNodesCount);
}

void COctreeNode::PreRender_Object_Nodes(bool bNodeCompletelyInFrustum, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, PodArray<COctreeNode*> & rVisNodes, int8 nThreadId)
{
  assert(nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS);

  if(m_nOccludedFrameId == GetFrameID())
    return;

  if(!bNodeCompletelyInFrustum && !rCam.IsAABBVisible_EH( m_objectsBox, &bNodeCompletelyInFrustum ))
    return;

  const Vec3 vCamPos = rCam.GetPosition();

  float fNodeDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos, m_objectsBox))*m_fZoomFactor;

  if(fNodeDistance > m_fObjectsMaxViewDist)
    return;

  if(m_nLastVisFrameId != GetFrameID())
  {
    if(GetObjManager()->IsBoxOccluded( m_objectsBox, fNodeDistance, &m_occlTestState, m_pVisArea != NULL, eoot_OCCELL ))
    {
      m_nOccludedFrameId = GetFrameID();
      return;
    }
  }

  m_nLastVisFrameId = GetFrameID();

  if(!m_bCompiled)
    CompileObjects();

  if(nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS && fNodeDistance < m_fObjectsMaxViewDist)
  {
    PodArray<CDLight*> * pAffectingLights = GetAffectingLights();

    if(HasObjects())
    {
      m_fNodeDistance = fNodeDistance;
      m_bNodeCompletelyInFrustum = bNodeCompletelyInFrustum;
      rVisNodes.Add(this);
    }
  }

  int nFirst = 
    ((vCamPos.x > m_vNodeCenter.x) ? 4 : 0) |
    ((vCamPos.y > m_vNodeCenter.y) ? 2 : 0) |
    ((vCamPos.z > m_vNodeCenter.z) ? 1 : 0);

  if(m_arrChilds[nFirst  ])
    m_arrChilds[nFirst  ]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);

  if(m_arrChilds[nFirst^1])
    m_arrChilds[nFirst^1]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);

  if(m_arrChilds[nFirst^2])
    m_arrChilds[nFirst^2]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);

  if(m_arrChilds[nFirst^4])
    m_arrChilds[nFirst^4]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);

  if(m_arrChilds[nFirst^3])
    m_arrChilds[nFirst^3]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);

  if(m_arrChilds[nFirst^5])
    m_arrChilds[nFirst^5]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);

  if(m_arrChilds[nFirst^6])
    m_arrChilds[nFirst^6]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);

  if(m_arrChilds[nFirst^7])
    m_arrChilds[nFirst^7]->PreRender_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, rVisNodes, nThreadId);
}

void COctreeNode::RenderContent(const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, int8 nThreadId)
{
  if(!GetCVars()->e_Objects)
    return;

  PodArray<CDLight*> * pAffectingLights = GetAffectingLights();

  bool bSunOnly = pAffectingLights && (pAffectingLights->Count()==1) && (pAffectingLights->GetAt(0)->m_Flags & DLF_SUN) && !m_pVisArea;

  SSectorTextureSet * pTerrainTexInfo = NULL;
  if(GetCVars()->e_VegetationUseTerrainColor)
    GetObjManager()->FillTerrainTexInfo(this, m_fNodeDistance, pTerrainTexInfo, m_objectsBox);

  if(!(nRenderMask&OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES) && GetCVars()->e_VegetationSprites && GetCVars()->e_Vegetation && m_arrVegetationSprites.Count())
    GetObjManager()->m_arrVegetationSprites[m_nRenderStackLevel][max(nThreadId,(int8)0)].AddList(m_arrVegetationSprites);

  if(m_arrObjects[eRNListType_Vegetation].m_pFirstNode && GetCVars()->e_Vegetation)
    RenderVegetations   (m_arrObjects[eRNListType_Vegetation],    rCam, m_bNodeCompletelyInFrustum!=0, pAffectingLights, bSunOnly, nThreadId, pTerrainTexInfo);
  
  if(/*GetCVars()->e_SceneMerging!=3 && */m_arrObjects[eRNListType_Brush].m_pFirstNode && GetCVars()->e_Brushes)
    RenderBrushes       (m_arrObjects[eRNListType_Brush],         rCam, m_bNodeCompletelyInFrustum!=0, pAffectingLights, bSunOnly, nThreadId, pTerrainTexInfo);

  if(m_arrObjects[eRNListType_DecalsAndRoads].m_pFirstNode && (GetCVars()->e_Decals || GetCVars()->e_Roads))
    RenderDecalsAndRoads(m_arrObjects[eRNListType_DecalsAndRoads],rCam, nRenderMask, vAmbColor, m_bNodeCompletelyInFrustum!=0, pAffectingLights, bSunOnly, nThreadId, pTerrainTexInfo);

  if(m_arrObjects[eRNListType_Unknown].m_pFirstNode)
    RenderCommonObjects (m_arrObjects[eRNListType_Unknown],       rCam, nRenderMask, vAmbColor, m_bNodeCompletelyInFrustum!=0, pAffectingLights, bSunOnly, nThreadId, pTerrainTexInfo);
}


void CStatObj::RenderRenderMesh(CRenderObject * pRenderObject, SInstancingInfo * pInstInfo, int nThreadId, const SRenderObjectModifier * pROII)
{
  if (GetCVars()->e_DebugDraw && (!GetCVars()->e_DebugDrawShowOnlyCompound || (m_bSubObject || m_pParentObject)))
  {
    for(int nLod=0; m_pLod0 && (nLod<MAX_STATOBJ_LODS_NUM); nLod++)
    {
      if(m_pLod0->m_arrpLowLODs[nLod] == this)
      { 
        m_pRenderMesh->SetMeshLod(nLod);  
        break; 
      }
    }

    if (RenderDebugInfo(pRenderObject,pROII))
      return;
  }

  if(GetCVars()->e_MtTestBuffer || nThreadId<0)
  {
    RenderRenderMeshReal(pRenderObject, m_pRenderMesh, pInstInfo, pROII);
  }
  else if(pROII && pROII->InUse())
  {
    SRenderMeshRenderInfoModified & info = gArrSRenderMeshRenderListModified[nThreadId].AddNew();
    info.pRenderObject = pRenderObject;
    info.pRenderMesh = GetRenderMesh();
    info.roModifier = *pROII;
  }
  else
  {
    SRenderMeshRenderInfo & info = gArrSRenderMeshRenderList[nThreadId].AddNew();
    info.pRenderObject = pRenderObject;
    info.pRenderMesh = GetRenderMesh();
  }
}
#undef THREAD_NAME
