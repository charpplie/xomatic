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
#include "IrradianceVolumeRenderNode.h"

const float fNodeMinSize = 8.f;
const float fObjectToNodeSizeRatio = 1.f/8.f;
const float fMinShadowCasterViewDist = 8.f;

PodArray<COctreeNode*> COctreeNode::m_arrEmptyNodes;

COctreeNode::COctreeNode(int nSID, const AABB & box, CVisArea * pVisArea, COctreeNode * pParent)
{ 
	memset(this, 0, sizeof(*this)); 
  m_nSID = nSID;
	m_nodeBox = box; 
  m_vNodeCenter = box.GetCenter();
  m_fNodeRadius = box.GetRadius();
	m_objectsBox.min = box.max;
	m_objectsBox.max = box.min;
	m_pTerrainNode = (nSID>=0 && GetTerrain()) ? GetTerrain()->FindMinNodeContainingBox(box, nSID) : NULL;
	m_pVisArea = pVisArea;
  m_pParent = pParent;

//	for(int n=0; n<2 && m_pTerrainNode && m_pTerrainNode->m_pParent; n++)
	//	m_pTerrainNode = m_pTerrainNode->m_pParent;
	m_vSunDirReady.Set(0,0,-1);
}

COctreeNode::~COctreeNode()
{
  for(int l=0; l<eRNListType_ListsNum; l++)
  {
    for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
      pNext = pObj->m_pNext;
		  EERType t = pObj->GetRenderNodeType();
		  if(t != eERType_Unknown)
		    pObj->ReleaseNode();
		  else
			  Get3DEngine()->UnRegisterEntity(pObj);
    }

    assert(!m_arrObjects[l].m_pFirstNode);
	}

	for(int i=0; i<8; i++) 
		delete m_arrChilds[i];

  m_arrEmptyNodes.Delete(this);

  GetObjManager()->m_arrUpdateStreamingPrioriryStack.Delete(this);
}

void COctreeNode::InsertObject(IRenderNode * pObj, const AABB & objBox, const float fObjRadiusSqr, const Vec3 & vObjCenter)
{
	FUNCTION_PROFILER_3DENGINE;

	COctreeNode * pCurrentNode = this;
	
	EERType eType = pObj->GetRenderNodeType();
	const uint32 renderFlags = (pObj->GetRndFlags() & (ERF_GOOD_OCCLUDER|ERF_CASTSHADOWMAPS|ERF_SUBSURFSCATTER));

	const bool bTypeLight = (eType == eERType_Light);
	const float fViewDistRatioVegetation = GetCVars()->e_ViewDistRatioVegetation;
	const bool bTypeVoxel = (eType == eERType_VoxelObject);
	const float fWSMaxViewDist = pObj->m_fWSMaxViewDist;
	const bool bTypeRoad	=	(eType == eERType_Road);
	
	Vec3 vObjectCentre = vObjCenter;

	while(true)
	{
		PrefetchLine(&pCurrentNode->m_arrChilds[0], 0);

		// parent bbox includes all children
		pCurrentNode->m_objectsBox.Add(objBox);

		pCurrentNode->m_fObjectsMaxViewDist = max(pCurrentNode->m_fObjectsMaxViewDist, fWSMaxViewDist);

		pCurrentNode->m_renderFlags |= renderFlags;		

		pCurrentNode->m_bHasLights	|= (bTypeLight);
		pCurrentNode->m_bHasVoxels	|= (bTypeVoxel);
		pCurrentNode->m_bHasRoads		|= (bTypeRoad);

		if(pCurrentNode->m_nodeBox.max.x-pCurrentNode->m_nodeBox.min.x > fNodeMinSize && !bTypeVoxel) // store voxels and roads in root
		{
			if(fObjRadiusSqr < sqr(pCurrentNode->m_fNodeRadius * fObjectToNodeSizeRatio))
			{
				if(fWSMaxViewDist < pCurrentNode->m_fNodeRadius * fViewDistRatioVegetation)
				{
					int nChildId = 
						((vObjCenter.x > pCurrentNode->m_vNodeCenter.x) ? 4 : 0) |
						((vObjCenter.y > pCurrentNode->m_vNodeCenter.y) ? 2 : 0) |
						((vObjCenter.z > pCurrentNode->m_vNodeCenter.z) ? 1 : 0);

					if(!pCurrentNode->m_arrChilds[nChildId])
					{
						MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
						pCurrentNode->m_arrChilds[nChildId] = new COctreeNode(pCurrentNode->m_nSID, pCurrentNode->GetChildBBox(nChildId), pCurrentNode->m_pVisArea, pCurrentNode);
					}

					pCurrentNode = pCurrentNode->m_arrChilds[nChildId];

					continue;
				}
			}
		}

		break;
	}

	PrefetchLine(&pObj->m_pOcNode, 0);	//Writing to m_pOcNode was a common L2 cache miss

	pCurrentNode->LinkObject(pObj, eType);

	pObj->m_pOcNode = pCurrentNode;
	pObj->m_nSID = pCurrentNode->m_nSID;

	pCurrentNode->m_bCompiled &= (eType == eERType_Light);

	pCurrentNode->m_nManageVegetationsFrameId = 0;
}

void COctreeNode::LinkObject(IRenderNode * pObj, EERType eERType, bool bPushFront)
{
  ERNListType eListType = pObj->GetRenderNodeListId(eERType);

  TDoublyLinkedList<IRenderNode> & rList = m_arrObjects[eListType];

  assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
  assert(!pObj->m_pNext && !pObj->m_pPrev);
  assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
  assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);

  if(bPushFront)
    rList.insertBeginning(pObj);
  else
    rList.insertEnd(pObj);

  assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
  assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);
  assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
}

void COctreeNode::UnlinkObject(IRenderNode * pObj)
{
  ERNListType eListType = pObj->GetRenderNodeListId(pObj->GetRenderNodeType());

  if(eListType == eRNListType_Vegetation && ((CVegetation*)pObj)->m_ucInSpritesList)
  {
    ((CVegetation*)pObj)->m_ucInSpritesList = false;
    for(int i=0; i<m_arrVegetationSprites.Count(); i++)
      if(m_arrVegetationSprites[i].pVegetation == pObj)
      { m_arrVegetationSprites.DeleteFastUnsorted(i); i--; }
  }

	assert( eListType >= 0 && eListType < eRNListType_ListsNum );
  TDoublyLinkedList<IRenderNode> & rList = m_arrObjects[eListType];

  assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
  assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
  assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);

  if(pObj->m_pNext || pObj->m_pPrev || pObj == rList.m_pLastNode || pObj == rList.m_pFirstNode)
    rList.remove(pObj);

  assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
  assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);
  assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
  assert(!pObj->m_pNext && !pObj->m_pPrev);
}

bool COctreeNode::DeleteObject(IRenderNode * pObj)
{
	FUNCTION_PROFILER_3DENGINE;

	if(pObj->m_pOcNode && pObj->m_pOcNode != this)
		return ((COctreeNode*)(pObj->m_pOcNode))->DeleteObject(pObj);

  UnlinkObject(pObj);

  m_lstOccluders.Delete(pObj);
//	m_lstMergedObjects.Delete(pObj);

  for(int i=0; i<m_lstCasters.Count(); i++)
    if(m_lstCasters[i].pNode == pObj)
    { m_lstCasters.Delete(i); break; }

  for(int i=0; i<m_lstSubSurfCasters.Count(); i++)
    if(m_lstSubSurfCasters[i].pNode == pObj)
    { m_lstSubSurfCasters.Delete(i); break; }

	pObj->m_pOcNode = NULL;
  pObj->m_nSID = -1;

  if( IsEmpty() && m_arrEmptyNodes.Find(this)<0 )
      m_arrEmptyNodes.Add(this);

/*  if(Get3DEngine()->m_pSceneTree && pObj->GetRenderNodeType() == eERType_Brush)
  {
    ((CBrush*)pObj)->m_bMerged = false;
    Get3DEngine()->m_pSceneTree->RequestUpdate(pObj->GetBBox());
  }*/

  return true;
}

extern float arrVegetation_fSpriteSwitchState[nThreadsNum];

void COctreeNode::RenderVegetations(TDoublyLinkedList<IRenderNode> & arrObjects, const CCamera & rCam, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo)
{
  FUNCTION_PROFILER_3DENGINE;

  AABB objBox;
  const Vec3 vCamPos = rCam.GetPosition();

  for(CVegetation * pObj = (CVegetation*)arrObjects.m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = (CVegetation *)pObj->m_pNext;
#if !defined(PS3) && !defined(XENON)
    if(pObj->m_pNext)
      cryPrefetchT0SSE(pObj->m_pNext);


#endif

    if(pObj->m_ucInSpritesList)
      break;

    pObj->FillBBox_NonVirtual(objBox);

    if(bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM( objBox ))
    {
      float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,objBox))*m_fZoomFactor;

      if(fEntDistance < pObj->m_fWSMaxViewDist)
      {
        GetObjManager()->RenderVegetation( pObj, pAffectingLights, objBox, fEntDistance, rCam, bSunOnly, nThreadId, pTerrainTexInfo );
      }
    }
  }
}

void COctreeNode::RenderBrushes(TDoublyLinkedList<IRenderNode> & lstObjects, const CCamera & rCam, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo)
{
  FUNCTION_PROFILER_3DENGINE;

  const Vec3 vCamPos = rCam.GetPosition();

  bool bCheckPerObjectOcclusion = m_fNodeRadius > GetCVars()->e_CoverageBufferCullIndividualBrushesMaxNodeSize;

  for(CBrush * pObj = (CBrush *)lstObjects.m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = (CBrush *)pObj->m_pNext;
#if !defined(PS3) && !defined(XENON)
    if(pObj->m_pNext)
      cryPrefetchT0SSE(pObj->m_pNext);



#endif

    if(bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM( pObj->m_WSBBox ))
    {
      float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,pObj->m_WSBBox))*m_fZoomFactor;
      assert(fEntDistance>=0 && _finite(fEntDistance));
      if(fEntDistance < pObj->m_fWSMaxViewDist)
      {
//        if( !GetCVars()->e_SceneMerging || !pObj->m_bMerged )
          GetObjManager()->RenderBrush( pObj, pAffectingLights, pObj->m_WSBBox, fEntDistance, bSunOnly, nThreadId, m_pVisArea, bCheckPerObjectOcclusion );
      }
    }
  }
}

void COctreeNode::RenderDecalsAndRoads(TDoublyLinkedList<IRenderNode> & lstObjects, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo)
{
  FUNCTION_PROFILER_3DENGINE;

  AABB objBox;
  const Vec3 vCamPos = rCam.GetPosition();

  for(IRenderNode * pObj = lstObjects.m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = pObj->m_pNext;
#if !defined(PS3) && !defined(XENON)
    if(pObj->m_pNext)
      cryPrefetchT0SSE(pObj->m_pNext);



#endif

    pObj->FillBBox(objBox);

    if(bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM( objBox ))
    {
      float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,objBox))*m_fZoomFactor;
      assert(fEntDistance>=0 && _finite(fEntDistance));
      if(fEntDistance < pObj->m_fWSMaxViewDist)
      {
        GetObjManager()->RenderDecalAndRoad( pObj, pAffectingLights, vAmbColor, objBox, fEntDistance, rCam, bSunOnly, nThreadId );
      }
    }
  }
}

void COctreeNode::RenderCommonObjects(TDoublyLinkedList<IRenderNode> & lstObjects, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, bool bNodeCompletelyInFrustum, PodArray<CDLight*> * pAffectingLights, bool bSunOnly, int8 nThreadId, SSectorTextureSet * pTerrainTexInfo)
{
  FUNCTION_PROFILER_3DENGINE;

  AABB objBox;
  const Vec3 vCamPos = rCam.GetPosition();

  for(IRenderNode * pObj = lstObjects.m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = pObj->m_pNext;
#if !defined(PS3) && !defined(XENON)
    if(pObj->m_pNext)
      cryPrefetchT0SSE(pObj->m_pNext);



#endif

    pObj->FillBBox(objBox);

    if(bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM( objBox ))
    {
      float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,objBox))*m_fZoomFactor;
      assert(fEntDistance>=0 && _finite(fEntDistance));
      if(fEntDistance < pObj->m_fWSMaxViewDist)
      {
        if(nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES)
        {
          if(pObj->GetRenderNodeType() != eERType_Unknown)
          {
            if(pObj->GetRenderNodeType() == eERType_Light)
            {
              CLightEntity * pEnt = (CLightEntity *)pObj;
              if(!pEnt->GetEntityVisArea() && pEnt->GetEntityTerrainNode() && !(pEnt->m_light.m_Flags & DLF_THIS_AREA_ONLY))
              { // not "this area only" outdoor light affects everything
              }
              else
                continue;
            }
            else
              continue;
          }
        }

        GetObjManager()->RenderObject( pObj, pAffectingLights, vAmbColor, objBox, fEntDistance, rCam, bSunOnly, nThreadId );
      }
    }
  }
}

void COctreeNode::CheckManageVegetationSprites(float fNodeDistance, int nMaxFrames)
{
	const uint32 iMainFrameId = GetMainFrameID();
  if((uint32)m_nManageVegetationsFrameId >= iMainFrameId-nMaxFrames)
    return;

	C3DEngine * p3DEngine = Get3DEngine();

  m_fNodeDistance = fNodeDistance;
  m_nManageVegetationsFrameId = iMainFrameId;

  FUNCTION_PROFILER_3DENGINE;

  if(m_vSunDirReady.Dot(p3DEngine->GetSunDirNormalized())<0.99f)
    m_bCompiled = false;

  const Vec3 vCamPos = GetCamera().GetPosition();
  AABB objBox;

  SSectorTextureSet * pTerrainTexInfo = NULL;
  if(GetCVars()->e_VegetationUseTerrainColor)
    GetObjManager()->FillTerrainTexInfo(this, m_fNodeDistance, pTerrainTexInfo, m_objectsBox);
/*
  Vec3 vD = GetBBox().GetCenter() - vCamPos;       
  Vec3 vCenter = GetBBox().GetCenter();
  Vec3 vD1 = vCenter + vD;
  m_dwSpritesAngle = ((uint32)(cry_atan2f(vD1.x, vD1.y)*(255.0f/(2*g_PI)))) & 0xff;
*/
  float fEngMaxViewDist = p3DEngine->GetMaxViewDistance();

  // check if we need to add some objects to sprites array
  for(CVegetation * pObj = (CVegetation*)m_arrObjects[eRNListType_Vegetation].m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = (CVegetation *)pObj->m_pNext;
#if !defined(PS3) && !defined(XENON)
    if(pObj->m_pNext)
      cryPrefetchT0SSE(pObj->m_pNext);



#endif

    if(pObj->m_ucInSpritesList)
      continue;

    pObj->FillBBox_NonVirtual(objBox);

    const float fEntDistanceSqr = Distance::Point_AABBSq(vCamPos,objBox)*m_fZoomFactor*m_fZoomFactor;

    float fEntDistance2D = cry_sqrtf(vCamPos.GetSquaredDistance2D(pObj->m_vPos))*m_fZoomFactor;

    StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[pObj->m_nObjectTypeID];

    const float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDist;

    pObj->UpdateRenderQuality(fEntDistance2D);

    if(fEntDistance2D > fSpriteSwitchDist && fEntDistanceSqr < sqr(pObj->m_fWSMaxViewDist*1.1f))
    {
      UnlinkObject(pObj);
      LinkObject(pObj, eERType_Vegetation, false); //We know that only eERType_Vegetation can get into the vegetation list, see GetRenderNodeListId()

      pObj->m_ucInSpritesList = true;

      assert(m_arrVegetationSprites.Count()<10000);

      SVegetationSpriteInfo & si = m_arrVegetationSprites.AddNew();

      ((CVegetation*)pObj)->UpdateSpriteInfo(si, 1.f, pTerrainTexInfo);
   
      m_bCompiled = false;
    }
  }

  // check if we need to remove some objects from sprites list
  for(int i=0; i<m_arrVegetationSprites.Count(); i++)
  {
    CVegetation * pObj = (CVegetation *)m_arrVegetationSprites[i].pVegetation;
#if !defined(PS3) && !defined(XENON)
    if(pObj->m_pNext)
      cryPrefetchT0SSE(pObj->m_pNext);



#endif

    assert(pObj->m_ucInSpritesList);

    pObj->FillBBox_NonVirtual(objBox);

    float fEntDistanceSqr = Distance::Point_AABBSq(vCamPos,objBox)*m_fZoomFactor*m_fZoomFactor;

    float fEntDistance2D = cry_sqrtf(vCamPos.GetSquaredDistance2D(pObj->m_vPos))*m_fZoomFactor;

    StatInstGroup & vegetGroup = GetObjManager()->m_lstStaticTypes[pObj->m_nObjectTypeID];

    const float fSpriteSwitchDist = vegetGroup.m_fSpriteSwitchDist;

    if(fEntDistance2D < fSpriteSwitchDist || fEntDistanceSqr > sqr(pObj->m_fWSMaxViewDist*1.1f))
    {
      pObj->m_ucInSpritesList = false;
      m_arrVegetationSprites.DeleteFastUnsorted(i);
      i--;

      UnlinkObject(pObj);
      LinkObject(pObj, eERType_Vegetation);	//We know that only eERType_Vegetation can get into the vegetation list, see GetRenderNodeListId()

      m_bCompiled = false;

      continue;
    }

    SVegetationSpriteInfo & si = m_arrVegetationSprites[i];

    ((CVegetation*)pObj)->UpdateSpriteInfo(si, 1.f, pTerrainTexInfo);
  }
}

void COctreeNode::Render_Object_Nodes(bool bNodeCompletelyInFrustum, const CCamera & rCam, int nRenderMask, const Vec3 & vAmbColor, int8 nThreadId)
{
  assert(nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS);

  if(m_nOccludedFrameId == GetFrameID())
      return;

  if(!bNodeCompletelyInFrustum && !rCam.IsAABBVisible_EH( m_objectsBox, &bNodeCompletelyInFrustum ))
    return;

  const Vec3 & vCamPos = rCam.GetPosition();

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

  if(GetCVars()->e_ObjectsTreeBBoxes)
  {
    if(GetCVars()->e_ObjectsTreeBBoxes == 1)
      DrawBBox(m_nodeBox, Col_Blue);
    if(GetCVars()->e_ObjectsTreeBBoxes == 2)
      DrawBBox(m_objectsBox, Col_Red);
  }

/*  if(GetCVars()->e_obj_tree_min_node_size>0)
  {
    DrawBBox(m_nodeBox, Col_Blue);
    DrawBBox(m_objectsBox, Col_Red);
  }

  if(GetCVars()->e_obj_tree_max_node_size && GetCVars()->e_obj_tree_max_node_size > (0.501f*(m_nodeBox.max.z-m_nodeBox.min.z)))
  {
    DrawBBox(m_nodeBox, Col_Blue);
    DrawBBox(m_objectsBox, Col_Red);
  }*/

  m_fNodeDistance = fNodeDistance;
  m_bNodeCompletelyInFrustum = bNodeCompletelyInFrustum;
  CheckManageVegetationSprites(fNodeDistance, (int)(fNodeDistance*0.2f));
  RenderContent(rCam, nRenderMask, vAmbColor, nThreadId);

  int nFirst = 
    ((vCamPos.x > m_vNodeCenter.x) ? 4 : 0) |
    ((vCamPos.y > m_vNodeCenter.y) ? 2 : 0) |
    ((vCamPos.z > m_vNodeCenter.z) ? 1 : 0);

  if(m_arrChilds[nFirst  ])
    m_arrChilds[nFirst  ]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);

  if(m_arrChilds[nFirst^1])
    m_arrChilds[nFirst^1]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);

  if(m_arrChilds[nFirst^2])
    m_arrChilds[nFirst^2]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);

  if(m_arrChilds[nFirst^4])
    m_arrChilds[nFirst^4]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);

  if(m_arrChilds[nFirst^3])
    m_arrChilds[nFirst^3]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);

  if(m_arrChilds[nFirst^5])
    m_arrChilds[nFirst^5]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);

  if(m_arrChilds[nFirst^6])
    m_arrChilds[nFirst^6]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);

  if(m_arrChilds[nFirst^7])
    m_arrChilds[nFirst^7]->Render_Object_Nodes(bNodeCompletelyInFrustum, rCam, nRenderMask, vAmbColor, nThreadId);
}

bool COctreeNode::Render_Occl_Nodes(const CCamera & rCam, CCullBuffer & rCB, int nRenderMask, int8 nThreadId)
{
	assert(nRenderMask & OCTREENODE_RENDER_FLAG_OCCLUDERS);

	if(m_renderFlags & ERF_GOOD_OCCLUDER)
		return false;

//	if(m_nOccludedFrameId == GetFrameID())
	//	if(&rCB == Get3DEngine()->GetCoverageBuffer())
		//	return false;

	bool bNodeCompletelyInFrustum = false;
	if(!rCam.IsAABBVisible_EHM( m_objectsBox, &bNodeCompletelyInFrustum ))
		return false;

  const Vec3 vCamPos = rCam.GetPosition();

	float fNodeDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos, m_objectsBox))*m_fZoomFactor;

	if(fNodeDistance > m_fObjectsMaxViewDist*GetCVars()->e_CoverageBufferOccludersViewDistRatio)
		return false;

	if(m_nLastVisFrameId != GetFrameID() && &rCB == Get3DEngine()->GetCoverageBuffer())
	{
		if(GetObjManager()->IsBoxOccluded( m_objectsBox, fNodeDistance, &m_occlTestState, m_pVisArea != NULL, 
			(nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS) ? eoot_OCCELL : eoot_OCCELL_OCCLUDER ))
		{
			m_nOccludedFrameId = GetFrameID();
			return false;
		}
	}

	m_nLastVisFrameId = GetFrameID();

	if(!m_bCompiled)
		CompileObjects();

	bool bObjectsFound = false;

	if(m_lstOccluders.Count() && fNodeDistance < m_fObjectsMaxViewDist*GetCVars()->e_CoverageBufferOccludersViewDistRatio)
	{
		for(int i=0; i<m_lstOccluders.Count(); i++)
		{
			IRenderNode * pObj = m_lstOccluders.GetAt(i);
			AABB objBox = pObj->GetBBox();
			float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,objBox))*m_fZoomFactor;
			assert(fEntDistance>=0 && _finite(fEntDistance));

			if(fEntDistance*GetObjManager()->m_fOcclTimeRatio < pObj->m_fWSMaxViewDist*GetCVars()->e_CoverageBufferOccludersViewDistRatio)
			{
				bool bCompletelyInFrustum = false;
				if(rCam.IsAABBVisible_EHM( objBox, &bCompletelyInFrustum ))
				{
          if(&rCB != Get3DEngine()->GetCoverageBuffer())
            bCompletelyInFrustum = false;

					GetObjManager()->RenderOccluderIntoZBuffer( pObj, fEntDistance, rCB, bCompletelyInFrustum );
					bObjectsFound = true;
				}
			}
		}
	}

	int nFirst = 
		((vCamPos.x > m_vNodeCenter.x) ? 4 : 0) |
		((vCamPos.y > m_vNodeCenter.y) ? 2 : 0) |
		((vCamPos.z > m_vNodeCenter.z) ? 1 : 0);

	if(m_arrChilds[nFirst  ])
		bObjectsFound |= m_arrChilds[nFirst  ]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	if(m_arrChilds[nFirst^1])
		bObjectsFound |= m_arrChilds[nFirst^1]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	if(m_arrChilds[nFirst^2])
		bObjectsFound |= m_arrChilds[nFirst^2]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	if(m_arrChilds[nFirst^4])
		bObjectsFound |= m_arrChilds[nFirst^4]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	if(m_arrChilds[nFirst^3])
		bObjectsFound |= m_arrChilds[nFirst^3]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	if(m_arrChilds[nFirst^5])
		bObjectsFound |= m_arrChilds[nFirst^5]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	if(m_arrChilds[nFirst^6])
		bObjectsFound |= m_arrChilds[nFirst^6]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	if(m_arrChilds[nFirst^7])
		bObjectsFound |= m_arrChilds[nFirst^7]->Render_Occl_Nodes(rCam, rCB, nRenderMask, nThreadId);

	return bObjectsFound;
}

void COctreeNode::CompileCharacter( ICharacterInstance * pChar, unsigned char &nInternalFlags )
{
  if( pChar )
  {
    IMaterial *pCharMaterial = pChar->GetMaterial();
    if(pCharMaterial && ((CMatInfo*)pCharMaterial)->IsPerObjectShadowPassNeeded())
    {
      nInternalFlags |= IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;
      return;
    }

    if(IAttachmentManager * pAttMan = pChar->GetIAttachmentManager())
    {
      int nCount = pAttMan->GetAttachmentCount();
      for(int i=0; i<nCount; i++)
      {
        if(IAttachment * pAtt = pAttMan->GetInterfaceByIndex(i))
          if(IAttachmentObject * pAttObj = pAtt->GetIAttachmentObject())
          {
            ICharacterInstance *pCharInstance = pAttObj->GetICharacterInstance();
            if( pCharInstance )
            {
//              CompileCharacter( pCharInstance, nInternalFlags);
              pCharMaterial = pCharInstance->GetMaterial();
              if(pCharMaterial && ((CMatInfo*)pCharMaterial)->IsPerObjectShadowPassNeeded())
              {
                nInternalFlags |= IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;
                return;
              }

            }

            if(IStatObj * pStatObj = pAttObj->GetIStatObj())
            {
              IMaterial * pMat = pAttObj->GetMaterial();
              if(!pMat)
                pMat = pStatObj->GetMaterial();

              if(pMat && ((CMatInfo*)pMat)->IsPerObjectShadowPassNeeded())
              {
                nInternalFlags |= IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;
                return;
              }
            }
          }
      }
    }
  }
}

void COctreeNode::CompileObjects()
{
	FUNCTION_PROFILER_3DENGINE;

	m_lstOccluders.Clear();
  m_lstCasters.Clear();
  m_lstSubSurfCasters.Clear();

//	m_bHasIndoorObjects = m_bHasOutdoorObjects = false;
/*
  if(CTerrainNode * pTerNode = GetTerrainNode())
  {
    while( !pTerNode->m_nNodeTexSet.nTex0 && pTerNode->m_pParent )
      pTerNode = pTerNode->m_pParent;

//    pTerrainTexInfo = &(pTerNode->m_nNodeTexSet);
  }
*/
  float fObjMaxViewDistance = 0;

	// update node
  for(int l=0; l<eRNListType_ListsNum; l++)
  for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = pObj->m_pNext;

    bool bVegetHasAlphaTrans = false;

		// update vegetation instances data
		EERType eRType = pObj->GetRenderNodeType();
		if(eRType == eERType_Vegetation)
		{
			CVegetation * pInst = (CVegetation*)pObj;				
			pInst->UpdateRndFlags();
			if(GetObjManager()->m_lstStaticTypes[pInst->m_nObjectTypeID].pStatObj)
				if(GetObjManager()->m_lstStaticTypes[pInst->m_nObjectTypeID].bUseAlphaBlending)
					bVegetHasAlphaTrans = true;

      if(GetCVars()->e_VegetationUseTerrainColor)
  			pInst->UpdateSunDotTerrain();							
		}

		// update max view distances
		pObj->m_fWSMaxViewDist = pObj->GetMaxViewDist();

		// update PER_OBJECT_SHADOW_PASS_NEEDED flag
    if(GetCVars()->e_ShadowsOnAlphaBlend)
    {
		  pObj->m_nInternalFlags &= ~IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;
		  if(	eRType != eERType_Light &&
			    eRType != eERType_Cloud &&
			    eRType != eERType_VoxelObject &&
			    eRType != eERType_FogVolume &&
			    eRType != eERType_Decal &&
			    eRType != eERType_Road &&
			    eRType != eERType_DistanceCloud &&
			    eRType != eERType_AutoCubeMap )
		  {
        if (eRType==eERType_ParticleEmitter)
          pObj->m_nInternalFlags |= IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;

        if(CMatInfo * pMatInfo = (CMatInfo*)pObj->GetMaterial())
          if(bVegetHasAlphaTrans || pMatInfo->IsPerObjectShadowPassNeeded())
            pObj->m_nInternalFlags |= IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;
        
        if(eRType==eERType_Unknown)
        {
          int nSlotCount = pObj->GetSlotCount();

          for(int s=0; s<nSlotCount; s++)
          {
            if(CMatInfo * pMat = (CMatInfo*)pObj->GetEntitySlotMaterial(s))
              if(pMat->IsPerObjectShadowPassNeeded())
                pObj->m_nInternalFlags |= IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;

            if(IStatObj * pStatObj = pObj->GetEntityStatObj(s))
              if(CMatInfo * pMat = (CMatInfo*)pStatObj->GetMaterial())
                if(pMat->IsPerObjectShadowPassNeeded())
                  pObj->m_nInternalFlags |= IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED;
          }
        }

        if(!(pObj->m_nInternalFlags & IRenderNode::PER_OBJECT_SHADOW_PASS_NEEDED))
          CompileCharacter( pObj->GetEntityCharacter(0), pObj->m_nInternalFlags );
		  }
    }

		int nFlags = pObj->GetRndFlags();

		// fill shadow casters list
		if(nFlags&ERF_CASTSHADOWMAPS && pObj->m_fWSMaxViewDist>fMinShadowCasterViewDist && eRType != eERType_Light)
		{
			bool bGeomIsVisible = true;
			if(pObj->GetIntegrationType() && (!(nFlags & ERF_SELECTED) || !GetCVars()->e_VoxTerOnTheFlyIntegration))
			{
				if( GetCVars()->e_Voxel && GetCVars()->e_VoxTerHideIntegrated==2 )
					bGeomIsVisible = false; // hide always
				if( GetCVars()->e_Voxel && GetCVars()->e_VoxTerHideIntegrated && Get3DEngine()->GetIVoxTerrain() )
					bGeomIsVisible = false; // hide only if voxel terrain used    
			}

			if(bGeomIsVisible)
			{
				COctreeNode * pNode = this;
				while(pNode && !(pNode->m_renderFlags & ERF_CASTSHADOWMAPS))
				{
					pNode->m_renderFlags |= ERF_CASTSHADOWMAPS;
					pNode = pNode->m_pParent;
				}

				float fMaxCastDist = pObj->m_fWSMaxViewDist*GetCVars()->e_ShadowsCastViewDistRatio;
				m_lstCasters.Add(SCasterInfo(pObj, fMaxCastDist));
			}
		}

		// fill sub-scattering casters list
    if(nFlags&ERF_SUBSURFSCATTER && pObj->m_fWSMaxViewDist>fMinShadowCasterViewDist && eRType != eERType_Light)
    {
      COctreeNode * pNode = this;
      while(pNode && !(pNode->m_renderFlags & ERF_SUBSURFSCATTER))
      {
				pNode->m_renderFlags |= ERF_SUBSURFSCATTER;
				pNode = pNode->m_pParent;
			}

      float fMaxCastDist = pObj->m_fWSMaxViewDist*GetCVars()->e_ShadowsCastViewDistRatio;
      m_lstSubSurfCasters.Add(SCasterInfo(pObj, fMaxCastDist));
    }

		// fill occluders list: take objects with proxies and (if marked as good occluder) voxel objects and solids
		if ((pObj->m_nInternalFlags&IRenderNode::HAS_OCCLUSION_PROXY) || 
      ((eRType == eERType_VoxelObject || (pObj->GetRenderMesh(0) && pObj->GetRenderMesh(0)->GetIndicesCount()==36)) && (nFlags&ERF_GOOD_OCCLUDER)))
		{
      COctreeNode * pNode = this;
      while(pNode && !(pNode->m_renderFlags & ERF_GOOD_OCCLUDER))
      {
				pNode->m_renderFlags |= ERF_GOOD_OCCLUDER;
				pNode = pNode->m_pParent;
			}

			m_lstOccluders.Add(pObj);
		}

/*		if(nFlags&ERF_OUTDOORONLY)
			m_bHasOutdoorObjects = true;
		else
			m_bHasIndoorObjects = true;
*/
    fObjMaxViewDistance = max(fObjMaxViewDistance,pObj->m_fWSMaxViewDist);
	}

  if(fObjMaxViewDistance>m_fObjectsMaxViewDist)
  {
    COctreeNode * pNode = this;
    while(pNode)
    {
      pNode->m_fObjectsMaxViewDist = max(pNode->m_fObjectsMaxViewDist, fObjMaxViewDistance);
      pNode = pNode->m_pParent;
    }
  }

	m_bCompiled = true;

	m_vSunDirReady = Get3DEngine()->GetSunDirNormalized();
}
/*
uint32 COctreeNode::GetLightMask()
{ 
	uint32 nSunLightMask = Get3DEngine()->GetTerrain()->GetSunLightMask();
	return (m_nLightMaskFrameId == GetFrameID()) ? (m_nLightMask | nSunLightMask) : nSunLightMask; 
}

void COctreeNode::AddLightMask(CDLight * pLight) 
{ 
	if(!m_objectsBox.IsOverlapSphereBounds(pLight->m_Origin,pLight->m_fRadius))
		return;

	uint32 nNewMask = 1<<pLight->m_Id;

	if(m_nLightMaskFrameId == GetFrameID())
	{
		m_nLightMask |= nNewMask; 
	}
	else
	{
		m_nLightMask = nNewMask; 
		m_nLightMaskFrameId = GetFrameID();
	}

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->AddLightMask( pLight );
}
*/


void COctreeNode::CheckInitAffectingLights()
{
  if(m_nLightMaskFrameId != GetFrameID())
  {
    m_lstAffectingLights.Clear();

    if( !m_pVisArea || m_pVisArea->IsAffectedByOutLights() )
    {
      PodArray<CDLight*> * pSceneLights = Get3DEngine()->GetDynamicLightSources();
      if(pSceneLights->Count() && (pSceneLights->GetAt(0)->m_Flags & DLF_SUN))
        m_lstAffectingLights.Add(pSceneLights->GetAt(0));
    }

    m_nLightMaskFrameId = GetFrameID();
  }
}

PodArray<CDLight*> * COctreeNode::GetAffectingLights()
{ 
  CheckInitAffectingLights();

  return &m_lstAffectingLights; 
}

void COctreeNode::AddLightSource(CDLight * pSource)
{ 
  if(!m_objectsBox.IsOverlapSphereBounds(pSource->m_Origin,pSource->m_fRadius))
    return;

  CheckInitAffectingLights();

  if(m_lstAffectingLights.Find(pSource)<0)
  {
    m_lstAffectingLights.Add(pSource); 

    for(int i=0; i<8; i++) 
      if(m_arrChilds[i])
        m_arrChilds[i]->AddLightSource( pSource );
  }
}

bool IsAABBInsideHull(SPlaneObject * pHullPlanes, int nPlanesNum, const AABB & aabbBox);
bool IsSphereInsideHull(SPlaneObject * pHullPlanes, int nPlanesNum, const Sphere & objSphere);

#if !defined(__SPU__)
void COctreeNode::FillShadowCastersList(bool bNodeCompletellyInFrustum, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest)
{
  if(!GetCVars()->e_Objects)
    return;

  if(pFr->bForSubSurfScattering)
  {
    if(m_renderFlags & ERF_SUBSURFSCATTER)
      FillSubSurfCastersList(pFr);
    return;
  }
	
	if(pFr->bReflectiveShadowMap)
	{
		FillIndirectLightingCastersList(pLight, pFr);
		return;
	}

	FUNCTION_PROFILER_3DENGINE;

	const int frameID = GetFrameID();

  if(bUseFrustumTest && !bNodeCompletellyInFrustum)
    if(!pFr->IntersectAABB(m_objectsBox, &bNodeCompletellyInFrustum))
	    return;

  bool bSun = (pLight->m_Flags&DLF_SUN) != 0;

  if(bSun && bNodeCompletellyInFrustum)
    nFillShadowCastersSkipFrameId = frameID;

	if(pShadowHull && !pFr->bUseAdditiveBlending && !IsAABBInsideHull(pShadowHull->GetElements(), pShadowHull->Count(), m_objectsBox))
  {
    nFillShadowCastersSkipFrameId = frameID;
		return;
  }

	//This has been moved up to avoid a load-hit-store in the fShadowsCastViewDistRatio branch below
	//	as vCamPos is being written to the stack.
	const Vec3 vCamPos = GetCamera().GetPosition();

	if(!m_bCompiled)
		CompileObjects();

	PrefetchLine(&m_lstCasters, 0);

  // anything closer than fGsmMinDist is not visible in this lod
  float fGsmMinDist = 0;
  if(pFr->nShadowMapLod && GetCVars()->e_GsmRange && GetCVars()->e_GsmRangeStep)
    fGsmMinDist = 1.0f / pFr->fFrustrumSize / GetCVars()->e_GsmRange / GetCVars()->e_GsmRangeStep;

	const float fShadowsCastViewDistRatio = GetCVars()->e_ShadowsCastViewDistRatio;

	if(fShadowsCastViewDistRatio != 0.0f)
	{
		float fNodeDistanceSqr = Distance::Point_AABBSq(vCamPos, m_objectsBox);
    fNodeDistanceSqr = max(fNodeDistanceSqr, sqr(fGsmMinDist));
		if(fNodeDistanceSqr > sqr(m_fObjectsMaxViewDist * fShadowsCastViewDistRatio))
    {
      nFillShadowCastersSkipFrameId = frameID;
      return;
    }
	}

	PrefetchLine(m_lstCasters.begin(), 0);
	PrefetchLine(m_lstCasters.begin(), 128);

  IRenderNode * pNotCaster = ((CLightEntity *)pLight->m_pOwner)->m_pNotCaster;

	// Initialize occlusion culler pointer if light source==sun and occ_check==1
//  CCullBuffer* pCB	=	bSun && (GetCVars()-> e_ShadowsOcclusionCulling==1) ? Get3DEngine()->GetCoverageBuffer() : 0;
  //if(pCB && !pCB->IsShadowcasterVisible(m_objectsBox,Get3DEngine()->GetSunDirNormalized()*GetCVars()->e_ShadowsCastViewDistRatio))











	
	assert(pFr->pLightOwner == pLight->m_pOwner);
	
	const int e_NegVegetation = !GetCVars()->e_Vegetation;
	const int e_NegBrushes		= !GetCVars()->e_Brushes;
	const int e_NegEntities		= !GetCVars()->e_Entities;
	const int iFrameID				= GetFrameID();

	PrefetchLine(&m_arrChilds[0]->m_nLightMaskFrameId, 0);

  SCasterInfo * pCastersEnd = m_lstCasters.end();
  for(SCasterInfo * pCaster = m_lstCasters.begin(); pCaster<pCastersEnd; pCaster++)
	{




    if(bSun && pCaster->nGSMFrameId == frameID)
      continue;

    if(fGsmMinDist > pCaster->fMaxCastingDist)
    { pCaster->nGSMFrameId = frameID; continue; }

		float fDistanceSq = Distance::Point_PointSq(vCamPos, pCaster->objSphere.center);

		if(fDistanceSq > sqr(pCaster->fMaxCastingDist+pCaster->objSphere.radius))
    { pCaster->nGSMFrameId = frameID; continue; }

    bool bObjCompletellyInFrustum = bNodeCompletellyInFrustum;
	  if(bUseFrustumTest && !bObjCompletellyInFrustum && !pFr->IntersectSphere(pCaster->objSphere, &bObjCompletellyInFrustum))
		  continue;

    if(bSun && bObjCompletellyInFrustum)
      pCaster->nGSMFrameId = frameID;

		if(pShadowHull && !pFr->bUseAdditiveBlending && !IsSphereInsideHull(pShadowHull->GetElements(), pShadowHull->Count(), pCaster->objSphere) )
    { pCaster->nGSMFrameId = frameID; continue; }

		if(e_NegVegetation && pCaster->nRType == eERType_Vegetation)
      continue;

		if(( e_NegBrushes /*|| (GetCVars()->e_SceneMerging && ((CBrush*)pCaster->pNode)->m_bMerged)*/) && pCaster->nRType == eERType_Brush)
      continue;

    if( e_NegEntities && pCaster->nRType == eERType_Unknown)
      continue;

    if(pCaster->pNode == pNotCaster)
      continue;

		pFr->pCastersList->Add(pCaster->pNode);
	}

	for(int i=0; i<8; i++)
	{
		PrefetchLine(&m_arrChilds[i+1]->m_nLightMaskFrameId, 0);
		if(m_arrChilds[i] && (m_arrChilds[i]->m_renderFlags & ERF_CASTSHADOWMAPS) && (!bSun || m_arrChilds[i]->nFillShadowCastersSkipFrameId != frameID))
	    m_arrChilds[i]->FillShadowCastersList(bNodeCompletellyInFrustum, pLight, pFr, pShadowHull, bUseFrustumTest);
	}
}


void COctreeNode::FillSubSurfCastersList(ShadowMapFrustum * pFr)
{
  FUNCTION_PROFILER_3DENGINE;

	bool bUnused = false;
  if(!pFr->IntersectAABB(m_objectsBox, &bUnused))
    return;

  if(!m_bCompiled)
    CompileObjects();

  // anything closer than fGsmMinDist is not visible in this lod
  float fGsmMinDist = 0;
  if(pFr->nShadowMapLod)
    fGsmMinDist = 2.0f / (pFr->fFrustrumSize * GetCVars()->e_GsmRange) / GetCVars()->e_GsmRange;

  const Vec3 vCamPos = GetCamera().GetPosition();

  if(GetCVars()->e_ShadowsCastViewDistRatio)
  {
    float fNodeDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos, m_objectsBox));
    fNodeDistance = max(fNodeDistance, fGsmMinDist);
    if(fNodeDistance > m_fObjectsMaxViewDist*GetCVars()->e_ShadowsCastViewDistRatio)
      return;
  }

  for(int i=0; i<m_lstSubSurfCasters.Count(); i++)
  {
    SCasterInfo * pCaster = &m_lstSubSurfCasters[i];

    if(fGsmMinDist > pCaster->fMaxCastingDist)
      continue;

    float fDistanceSq = Distance::Point_PointSq(vCamPos, pCaster->objSphere.center);

    if(fDistanceSq > sqr(pCaster->fMaxCastingDist + pCaster->objSphere.radius))
      continue;

		//This allows IntersectSphere not to branch on the presence of the bool ptr, and doesn't cost us anything.
    if(!pFr->IntersectSphere(pCaster->objSphere, &bUnused))
      continue;

    pFr->pCastersList->Add(pCaster->pNode);
  }

  for(int i=0; i<8; i++) 
    if(m_arrChilds[i] && (m_arrChilds[i]->m_renderFlags & ERF_SUBSURFSCATTER))
      m_arrChilds[i]->FillSubSurfCastersList(pFr);
}

void COctreeNode::FillIndirectLightingCastersList(const CDLight * pLight, ShadowMapFrustum * pFr)
{
	FUNCTION_PROFILER_3DENGINE;

	assert(pFr->bReflectiveShadowMap);
	assert(GetCVars()->e_GI > 0);

	if(!m_bCompiled)
		CompileObjects();

	bool bUnused = false;
	if(!pFr->IntersectAABB(m_objectsBox, &bUnused))
		return;

	PrefetchLine(&m_arrObjects, 0);
	PrefetchLine(m_arrObjects[0].m_pFirstNode, 0);

	const Vec3 vCamPos = GetCamera().GetPosition();

	const float fRadius = pLight->m_fRadius * tanf(pLight->m_fLightFrustumAngle * (gf_PI/180.0f) * 2.f);
	const float fGsmMinDist = max(0.f, fRadius * 1.5f * GetCVars()->e_ShadowsCastViewDistRatio);
	const float fMinRadius = fRadius / pFr->nTexSize * 8.f;		// EightTexelsSizeInMeters

	if(GetCVars()->e_ShadowsCastViewDistRatio)
	{
		float fNodeDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos, m_objectsBox));
		fNodeDistance = max(fNodeDistance, fGsmMinDist);
		if(fNodeDistance > m_fObjectsMaxViewDist*GetCVars()->e_ShadowsCastViewDistRatio)
			return;
	}

	PrefetchLine(&m_arrChilds[0]->m_nLightMaskFrameId, 0);

	for(int l=0; l<eRNListType_ListsNum; l++)
	{
		for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
		{




			const EERType rType = pObj->GetRenderNodeType();

			// fill indirect lighting casters list
			if( rType == eERType_Light || rType == eERType_Cloud ||
					rType == eERType_FogVolume || rType == eERType_WaterVolume ||
					rType == eERType_WaterWave || rType == eERType_DistanceCloud ||
					rType == eERType_VolumeObject || rType == eERType_AutoCubeMap ||
					rType == eERType_Rope || rType == eERType_IrradianceVolume ||
					rType == eERType_PrismObject || rType == eERType_RenderProxy ||
					rType == eERType_GameEffect )
			{
				continue;
			}

			// ignore the object that is less than 8 pixels
			if(pObj->m_fWSMaxViewDist < fMinShadowCasterViewDist)
				continue;

			const float fObjRadius = pObj->GetBBox().GetRadius();

			// ignore the object that is less than 8 pixels
			if(fObjRadius < fMinRadius)
				continue;

			const Vec3 vObjCenter = pObj->GetBBox().GetCenter();

			float fDistanceSq = Distance::Point_PointSq(vCamPos, vObjCenter);
			const float fMaxCastDist = pObj->m_fWSMaxViewDist*GetCVars()->e_ShadowsCastViewDistRatio;

			if(fDistanceSq > sqr(fMaxCastDist + fObjRadius))
				continue;

			//This allows IntersectSphere not to branch on the presence of the bool ptr, and doesn't cost us anything.
			if(!pFr->IntersectSphere(Sphere(vObjCenter, fObjRadius), &bUnused))
				continue;

			// ignore characters
			if(pObj->GetEntityCharacter(0))
				continue;

			pFr->pCastersList->Add(pObj);
		}
	}

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->FillIndirectLightingCastersList(pLight, pFr);
}
#endif
void COctreeNode::MarkAsUncompiled()
{
	m_bCompiled = false;

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->MarkAsUncompiled();
}

COctreeNode * COctreeNode::FindNodeContainingBox(const AABB & objBox)
{
	FUNCTION_PROFILER_3DENGINE;

	if(!m_nodeBox.IsContainSphere(objBox.min, -0.01f) || !m_nodeBox.IsContainSphere(objBox.max, -0.01f))
		return NULL;

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			if(COctreeNode * pFoundNode = m_arrChilds[i]->FindNodeContainingBox(objBox))
				return pFoundNode;

	return this;
}

void COctreeNode::MoveObjectsIntoList(PodArray<SRNInfo> * plstResultEntities, const AABB * pAreaBox, 
                                      bool bRemoveObjects, bool bSkipDecals, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects,
                                      EERType eRNType)
{
	FUNCTION_PROFILER_3DENGINE;

	if(pAreaBox && !Overlap::AABB_AABB(m_objectsBox, *pAreaBox))
		return;

  for(int l=0; l<eRNListType_ListsNum; l++)
  for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = pObj->m_pNext;

    if(eRNType < eERType_TypesNum && pObj->GetRenderNodeType() != eRNType)
      continue;

		if(bSkipDecals && pObj->GetRenderNodeType() == eERType_Decal)
			continue;

		if(bSkip_ERF_NO_DECALNODE_DECALS && pObj->GetRndFlags()&ERF_NO_DECALNODE_DECALS)
			continue;

		if(bSkipDynamicObjects)
		{
      EERType eRType = pObj->GetRenderNodeType();

      if(eRType == eERType_Unknown)
      {
        if(pObj->IsMovableByGame())
          continue;
      }
      else if(	
        eRType != eERType_Brush &&
				eRType != eERType_Vegetation &&
				eRType != eERType_VoxelObject )
				continue;
		}

    if(pAreaBox && !Overlap::AABB_AABB(pObj->GetBBox(), *pAreaBox))
      continue;

    if(bRemoveObjects)
    {
      UnlinkObject(pObj);

      m_bCompiled = false;
    }

    plstResultEntities->Add(pObj);
	}

	for(int i=0; i<8; i++) 
  {
		if(m_arrChilds[i])
    {
      m_arrChilds[i]->MoveObjectsIntoList(plstResultEntities, pAreaBox, bRemoveObjects, bSkipDecals, bSkip_ERF_NO_DECALNODE_DECALS, bSkipDynamicObjects, eRNType);

//      if(bRemoveObjects && !m_arrChilds[i]->m_lstObjects.m_pFirstNode && !m_lstMergedObjects.Count())
  //      SAFE_DELETE(m_arrChilds[i]);
    }
  }
}

void COctreeNode::DeleteObjectsByFlag(int nRndFlag)
{
	FUNCTION_PROFILER_3DENGINE;

  for(int l=0; l<eRNListType_ListsNum; l++)
  for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
  {
    pNext = pObj->m_pNext;

		if(pObj->GetRndFlags()&nRndFlag)
			DeleteObject(pObj);
	}

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->DeleteObjectsByFlag(nRndFlag);
}

void COctreeNode::UnregisterEngineObjectsInArea(const SHotUpdateInfo * pExportInfo, PodArray<IRenderNode *> & arrUnregisteredObjects, bool bOnlyEngineObjects)
{
  FUNCTION_PROFILER_3DENGINE;

  const AABB * pAreaBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

  if(pAreaBox && !Overlap::AABB_AABB(m_nodeBox, *pAreaBox))
    return;	

  uint32 nObjTypeMask = pExportInfo ? pExportInfo->nObjTypeMask : (uint32)~0;

  for(int l=0; l<eRNListType_ListsNum; l++)
  {
    for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
      pNext = pObj->m_pNext;

      EERType eType = pObj->GetRenderNodeType();

      if(bOnlyEngineObjects)
      {
        if(!(nObjTypeMask & (1<<eType)))
          continue;

        if((eType == eERType_Vegetation && !(pObj->GetRndFlags()&ERF_PROCEDURAL)) ||
            eType == eERType_Brush ||
            eType == eERType_VoxelObject ||
            eType == eERType_Decal ||
            eType == eERType_WaterVolume ||
            eType == eERType_Road ||
            eType == eERType_DistanceCloud ||
            eType == eERType_WaterWave ||
            eType == eERType_AutoCubeMap ||
            eType == eERType_IrradianceVolume)
        {
          if(eType == eERType_IrradianceVolume)
            if(CIrradianceVolumeRenderNode*pIrr = (CIrradianceVolumeRenderNode*)pObj)
              if(pIrr->m_pRE)
                if(pIrr->m_pRE->GetFlags() & CREIrradianceVolume::efGIVolume)
                  continue;
  
          DeleteObject(pObj); 
          arrUnregisteredObjects.Add(pObj);
          m_bCompiled = false;
        }
      }
      else
      {
        DeleteObject(pObj); 
        arrUnregisteredObjects.Add(pObj);
        m_bCompiled = false;
      }
    }
  }

  for(int i=0; i<8; i++) 
  {
    if(m_arrChilds[i])
      m_arrChilds[i]->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);
  }
}

int COctreeNode::PhysicalizeVegetationInBox(const AABB &bbox)
{
	if(!Overlap::AABB_AABB(m_objectsBox, bbox))
		return 0;	

	int nCount=0;
  for(IRenderNode * pObj = m_arrObjects[eRNListType_Vegetation].m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    assert(pObj->GetRenderNodeType()==eERType_Vegetation);

    const AABB & objBox = pObj->GetBBox();
    if(Overlap::AABB_AABB(bbox, objBox) && 
      max(objBox.max.x-objBox.min.x, objBox.max.y-objBox.min.y) <= 
      ((C3DEngine*)gEnv->p3DEngine)->GetCVars()->e_OnDemandMaxSize)
	  {
		  if (!pObj->GetPhysics())
			  pObj->Physicalize(true);
		  if (pObj->GetPhysics())
			  pObj->GetPhysics()->AddRef(), nCount++;
	  }
  }

	for(int i=0; i<8; i++) if(m_arrChilds[i])
		nCount += m_arrChilds[i]->PhysicalizeVegetationInBox(bbox);
	return nCount;
}

int COctreeNode::DephysicalizeVegetationInBox(const AABB &bbox)
{
	if(!Overlap::AABB_AABB(m_objectsBox, bbox))
		return 0;	

  int nCount=0;
  for(IRenderNode * pObj = m_arrObjects[eRNListType_Vegetation].m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    assert(pObj->GetRenderNodeType()==eERType_Vegetation);
    const AABB & objBox = pObj->GetBBox();
		if(Overlap::AABB_AABB(bbox, objBox) && 
		  max(objBox.max.x-objBox.min.x, objBox.max.y-objBox.min.y) <=
		  ((C3DEngine*)gEnv->p3DEngine)->GetCVars()->e_OnDemandMaxSize)
		pObj->Dephysicalize(true);
  }

	for(int i=0; i<8; i++) if(m_arrChilds[i])
		nCount += m_arrChilds[i]->DephysicalizeVegetationInBox(bbox);
	return nCount;
}

AABB COctreeNode::GetChildBBox(int nChildId)
{
	int x = (nChildId/4);
	int y = (nChildId-x*4)/2;
	int z = (nChildId-x*4-y*2);
	Vec3 vSize = m_nodeBox.GetSize()*0.5f;
	Vec3 vOffset = vSize;
	vOffset.x *= x;
	vOffset.y *= y;
	vOffset.z *= z;
	AABB childBox;
	childBox.min = m_nodeBox.min + vOffset;
	childBox.max = childBox.min + vSize;
	return childBox;
}

int COctreeNode::GetObjectsCount(EOcTeeNodeListType eListType)
{
  int nCount = 0;

  switch(eListType)
  {
  case eMain:
    for(int l=0; l<eRNListType_ListsNum; l++)
      for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        nCount++;
    break;
  case eCasters:
    for(int l=0; l<eRNListType_ListsNum; l++)
      for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        if(pObj->GetRndFlags()&ERF_CASTSHADOWMAPS)
          nCount++;
    break;
  case eOccluders:
    nCount = m_lstOccluders.Count();
    break;
  case eSprites:
    nCount = m_arrVegetationSprites.Count();
    break;
  case eLights:
    nCount = m_lstAffectingLights.Count();
    break;
  }

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			nCount += m_arrChilds[i]->GetObjectsCount(eListType);

	return nCount;
}

bool COctreeNode::IsRightNode(const AABB & objBox, const float fObjRadiusSqr, float fObjMaxViewDist)
{
  if(!Overlap::Point_AABB(objBox.GetCenter(), m_nodeBox))
    if(m_pParent)
      return false; // fail if center is not inside or node bbox

  if(2 != Overlap::AABB_AABB_Inside(objBox, m_objectsBox))
    return false; // fail if not completely inside of objects bbox

  float fNodeRadiusRated = sqr(m_fNodeRadius*fObjectToNodeSizeRatio);

  if(fObjRadiusSqr > fNodeRadiusRated*4.f)
    if(m_pParent)
      return false; // fail if object is too big and we need to register it some of parents

	if(m_nodeBox.max.x-m_nodeBox.min.x > fNodeMinSize)
		if(fObjRadiusSqr < fNodeRadiusRated)
//      if(fObjMaxViewDist < m_fNodeRadius*GetCVars()->e_ViewDistRatioVegetation*fObjectToNodeSizeRatio)
        return false; // fail if object is too small and we need to register it some of childs

	return true;
}

void COctreeNode::GetMemoryUsage(ICrySizer * pSizer)
{
  for(int l=0; l<eRNListType_ListsNum; l++)
  {
    for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
		  if(!(pObj->GetRndFlags()&ERF_PROCEDURAL))
			  pObj->GetMemoryUsage(pSizer);
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "ObjLists");
    pSizer->AddContainer(m_arrVegetationSprites);
    pSizer->AddContainer(m_lstOccluders);
    pSizer->AddContainer(m_lstCasters);
    pSizer->AddContainer(m_lstSubSurfCasters);
    pSizer->AddContainer(m_lstAffectingLights);
  }

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->GetMemoryUsage(pSizer);

	if (pSizer)
		pSizer->AddObject(this,sizeof(*this));
}

IRenderNode * COctreeNode::FindTerrainSectorVoxObject(const AABB & objBox)
{
	FUNCTION_PROFILER_3DENGINE;

	if(!Overlap::AABB_AABB(m_objectsBox, objBox))
		return NULL;

  for(IRenderNode * pObj = m_arrObjects[IRenderNode::GetRenderNodeListId(eERType_VoxelObject)].m_pFirstNode; pObj; pObj = pObj->m_pNext)
	{
		if(pObj->GetRenderNodeType() == eERType_VoxelObject && 
      !(((CVoxelObject*)pObj)->m_nFlags&IVOXELOBJECT_FLAG_CULL_TERRAIN_SURFACE) &&
			((CVoxelObject*)pObj)->IsSnappedToTerrainSectors() &&
			((CVoxelObject*)pObj)->GetRenderMesh(0) &&
			((CVoxelObject*)pObj)->GetRenderMesh(0)->GetIndicesCount())
		{
			if(fabs(objBox.min.x - pObj->GetBBox().min.x)<0.01f)
				if(fabs(objBox.min.y - pObj->GetBBox().min.y)<0.01f)
					if(fabs(objBox.max.x - pObj->GetBBox().max.x)<0.01f)
						if(fabs(objBox.max.y - pObj->GetBBox().max.y)<0.01f)
							return pObj;
		}
	}

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			if(IRenderNode * pObj = m_arrChilds[i]->FindTerrainSectorVoxObject(objBox))
				return pObj;

	return NULL;
}
/*
AABB CSceneTree::GetChildBBox(int nChildId)
{
  int x = (nChildId/4);
  int y = (nChildId-x*4)/2;
  int z = (nChildId-x*4-y*2);
  Vec3 vSize = m_nodeBox.GetSize()*0.5f;
  Vec3 vOffset = vSize;
  vOffset.x *= x;
  vOffset.y *= y;
  vOffset.z *= z;
  AABB childBox;
  childBox.min = m_nodeBox.min + vOffset;
  childBox.max = childBox.min + vSize;
  return childBox;
}

void CSceneTree::RequestUpdate(const AABB & areaBox)
{
  m_bAreaBrushesUpdateRequested = true;

  if(m_nodeBox.GetSize().x <= GetCVars()->e_SceneMergingMinNodeSize)
    return;

  for(int nChildId=0; nChildId<8; nChildId++) 
  {
    AABB childBox = GetChildBBox(nChildId);

    if(!Overlap::AABB_AABB(childBox, areaBox))
      continue;

    if(!m_arrChilds[nChildId])
      m_arrChilds[nChildId] = new CSceneTree(GetChildBBox(nChildId), this);

    m_arrChilds[nChildId]->RequestUpdate(areaBox);
  }
}

void CSceneTree::Render(const CCamera & rCam, bool bCompletelyInFrustum)
{
  if(!bCompletelyInFrustum && !rCam.IsAABBVisible_EHM( m_nodeBox, &bCompletelyInFrustum ))
    return;

  float fDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), m_nodeBox));

  float fBoxSize = m_nodeBox.GetSize().x;

  bool bDraw = (fDistance*GetCVars()->e_SceneMergingViewDistRatio > fBoxSize || !HasChildNodes());

  if(bDraw)
  {
    if(GetObjManager()->IsBoxOccluded( m_nodeBox, fDistance, &m_occlTestState, false, eoot_OCCELL ))
      return;

    if(GetCVars()->e_SceneMerging==2)
      DrawBBox(m_nodeBox);

    CheckUpdateAreaBrushes();

    if(GetCVars()->e_SceneMerging!=4)
    if(m_plstAreaBrush)
    {
      PodArray<CDLight*> * pAffectingLights = GetAffectingLights();

      for(int i=0; i<m_plstAreaBrush->Count(); i++)
      {
        CBrush * pEnt = m_plstAreaBrush->GetAt(i);
        if(bCompletelyInFrustum || rCam.IsAABBVisible_FM( pEnt->m_WSBBox ))
          GetObjManager()->RenderBrush( pEnt, pAffectingLights, pEnt->m_WSBBox, fDistance, false, 0, NULL, false );
      }
    }
  }
  else
  {
    for(int i=0; i<8; i++) 
      if(m_arrChilds[i])
        if(rCam.IsAABBVisible_FM( m_arrChilds[i]->m_nodeBox ))
          m_arrChilds[i]->Render(rCam, bCompletelyInFrustum);
  }
}

void CSceneTree::CheckUpdateAreaBrushes()
{
	if( !GetCVars()->e_SceneMerging )
		return FreeAreaBrushes(false);

	Vec3 vCamPos = GetCamera().GetPosition();

	if(m_plstAreaBrush && !m_bAreaBrushesUpdateRequested)
  	return;

  m_bAreaBrushesUpdateRequested = false;

	FUNCTION_PROFILER_3DENGINE;

	FreeAreaBrushes(false);

	float fStartTime = GetCurAsyncTimeSec();

	m_plstAreaBrush = new PodArray<CBrush*>;

	float fAreaMaxViewDist = 0;

	PodArray<SRenderMeshInfoOutput> lstResultRenderMeches;
	CRenderMeshMerger Merger;

	// build list of all objects in node
	PodArray<SRenderMeshInfoInput> lstRMI;

  PodArray<IRenderNode*> lstBrushes; lstBrushes.Clear();
  Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstBrushes, eERType_Brush, &m_nodeBox, m_nodeBox.GetRadius()*1.f);

  for(int i=0; i<lstBrushes.Count(); i++)
  {
    CBrush * pObj = (CBrush *)lstBrushes[i];

    if(pObj->GetIntegrationType() != eIT_SceneMesh)
      continue;

    if(pObj->GetRndFlags() & ERF_HIDDEN || pObj->GetRndFlags() & ERF_COLLISION_PROXY)
      continue;

		const char * pName = pObj->GetName();
		if(pObj->GetRenderNodeType() != eERType_Brush)
			continue;

    {
      IRenderMesh * pRM = pObj->GetRenderMesh(0);
      if(pObj->GetRenderMesh(GetCVars()->e_LodMin))
        pRM = pObj->GetRenderMesh(GetCVars()->e_LodMin);

      if(!pRM)
        continue;

      float fTrisPerChunk = pRM->GetAverageTrisNumPerChunk(pObj->GetMaterial());


      if(fTrisPerChunk > (float)GetCVars()->e_SceneMergingMaxTrisPerInputChunk)
        continue;
    }

		Matrix34A objMat; objMat.SetIdentity();
		IStatObj * pEntObject = pObj->GetEntityStatObj(0, 0, &objMat);
		if(!pEntObject)
			continue;

		int nLod = 0;

    float fLodDist = 4.f;
    while((fLodDist < m_nodeBox.GetSize().x || nLod < GetCVars()->e_LodMin) && pEntObject->GetLodObject(nLod+1))
    {
      fLodDist *= 2;
      nLod++;
    }

		if(nLod<0)
			continue; // sprite

		pEntObject = pEntObject->GetLodObject(nLod);
		if(!pEntObject)
			continue;

		float fMatScale = objMat.GetColumn0().GetLength();
		if(fMatScale<0.01f || fMatScale>100.f)
			continue;

		SRenderMeshInfoInput rmi;
		rmi.pMat = pObj->GetMaterial();
		rmi.mat = objMat;
		rmi.pMesh = pEntObject->GetRenderMesh();
		rmi.pSrcRndNode = pObj;

    // check if it makes sense to merge this object
		if(rmi.pMesh)
		{
//			float fDrawTrisPerChunk = rmi.pMesh->GetAverageTrisNumPerChunk(rmi.pMat);
//			if(fDrawTrisPerChunk>GetCVars()->e_scene_merging_max_tris_in_chunk)
	//			continue;
		}
		else
		{
			float fDrawTrisPerChunk = 0;
			float fDrawChunksNum = 0;
			
			for(int s=0; s<pEntObject->GetSubObjectCount(); s++)
			{
				CStatObj::SSubObject * pSubObj = pEntObject->GetSubObject(s);
				if (pSubObj->pStatObj && pSubObj->nType == STATIC_SUB_OBJECT_MESH)
				{
					rmi.pMesh = pSubObj->pStatObj->GetRenderMesh();
					rmi.mat = objMat * pSubObj->tm;
					if(rmi.pMesh)
					{
						fDrawTrisPerChunk += rmi.pMesh->GetAverageTrisNumPerChunk(rmi.pMat);
						fDrawChunksNum++;
					}
				}
			}

			if(!fDrawChunksNum)// || fDrawTrisPerChunk/fDrawChunksNum > GetCVars()->e_scene_merging_max_tris_in_chunk)
				continue;
		}

		if(fAreaMaxViewDist < pObj->m_fWSMaxViewDist)
			fAreaMaxViewDist = pObj->m_fWSMaxViewDist;
/*
    if(pObj->GetRenderNodeType() == eERType_Vegetation && ((CVegetation*)pObj)->m_ucInSpritesList)
    {
      UnlinkObject(pObj, m_lstSprites);
      ((CVegetation*)pObj)->m_ucInSpritesList = false;

      for(int i=0; i<m_arrVegetationSprites.Count(); i++)
        if(m_arrVegetationSprites[i].pVegetation == pObj)
        { m_arrVegetationSprites.DeleteFastUnsorted(i); i--; }
    }
    else
      UnlinkObject(pObj, m_lstObjects[pObj->GetRenderNodeType()]);

    m_lstMergedObjects.Add(pObj);
*/
/*

    pObj->m_bMerged = true;

		if(rmi.pMesh)
			lstRMI.Add(rmi);
		else
		{
			for(int s=0; s<pEntObject->GetSubObjectCount(); s++)
			{
				CStatObj::SSubObject * pSubObj = pEntObject->GetSubObject(s);
				if (pSubObj->pStatObj && pSubObj->nType == STATIC_SUB_OBJECT_MESH)
				{
					rmi.pMesh = pSubObj->pStatObj->GetRenderMesh();
					rmi.mat = objMat * pSubObj->tm;
					if(rmi.pMesh)
						lstRMI.Add(rmi);
				}
			}
		}
	}

	if(lstRMI.Count()>0)
	{
		int nRMI_Id = 0;
		while(nRMI_Id < lstRMI.Count())
		{
			int nRndFlags = 0;

      int nRMI_Id_Next = lstRMI.Count();

			char szName[256]=""; sprintf(szName, "MergedBrush_%d", m_plstAreaBrush->Count());

			lstResultRenderMeches.Clear();

			CRenderMeshMerger::SMergeInfo info;
			info.sMeshName = szName;
			info.sMeshType = "Merged_Brushes_And_Vegetations";
      info.pClipCellBox = &m_nodeBox;
      info.bCompactVertBuffer = true;
//      info.bPrintDebugMessages = true;
			Merger.MergeRenderMeshes(&lstRMI[nRMI_Id], nRMI_Id_Next-nRMI_Id,lstResultRenderMeches, info);

			if(lstResultRenderMeches.Count())
			{
				float fDrawTrisPerChunk = 0;
				for(int m=0; m<lstResultRenderMeches.Count(); m++)
				{
					IRenderMesh * pNewRM = lstResultRenderMeches[m].pMesh;
					fDrawTrisPerChunk += pNewRM->GetAverageTrisNumPerChunk(pNewRM->GetMaterial());
				}
				fDrawTrisPerChunk /= lstResultRenderMeches.Count();
//				if(fDrawTrisPerChunk<GetCVars()->e_scene_merging_max_tris_in_chunk)
	//				Warning("COctreeNode::MakeAreaBrush: Warning: Merging produces not optimal mesh: %.2f tris in average per chunk", fDrawTrisPerChunk);
			}

			for(int m=0; m<lstResultRenderMeches.Count(); m++)
			{
				IRenderMesh * pNewRM = lstResultRenderMeches[m].pMesh;

				if (!pNewRM || !pNewRM->GetVerticesCount())
				{ // make empty brush list - no geometry in sector
					continue;
				}

				// make new statobj
				CStatObj * pAreaStatObj = new CStatObj();
				pAreaStatObj->m_nLoadedTrisCount = pNewRM->GetIndicesCount()/3;
				pAreaStatObj->SetRenderMesh(pNewRM);
				Vec3 vMin, vMax; pNewRM->GetBBox(vMin, vMax);
				pAreaStatObj->SetBBoxMin(vMin);
				pAreaStatObj->SetBBoxMax(vMax);
				pAreaStatObj->AddRef();

				// make brush
				CBrush * pAreaBrush = new CBrush();

//				Matrix34 mat;
	//			mat.SetIdentity();
				//		mat.SetTranslation(Vec3(0,0,-0.5f));
				pAreaBrush->SetEntityStatObj(0,pAreaStatObj);
				pAreaBrush->SetBBox(AABB(vMin, vMax));
				pAreaBrush->SetMaterial(pNewRM->GetMaterial());

				pAreaBrush->SetRndFlags(ERF_MERGE_RESULT|nRndFlags, true);

//				Get3DEngine()->UnRegisterEntity(pAreaBrush);

				pAreaBrush->m_fWSMaxViewDist = pAreaBrush->GetMaxViewDist();

				{
//					LinkObject(pAreaBrush, m_lstObjects[eERType_Brush]);		
					pAreaBrush->m_fWSMaxViewDist = fAreaMaxViewDist;

					// fill shadow casters list
					if(nRndFlags&ERF_CASTSHADOWMAPS && pAreaBrush->m_fWSMaxViewDist>fMinShadowCasterViewDist)
					{
						m_bHasShadowCasters = true;
					}
				}

//				pAreaBrush->m_pOcNode = (COctreeNode*)this;

				m_plstAreaBrush->Add(pAreaBrush);
			}

			nRMI_Id = nRMI_Id_Next;
		}
	}
/*	else
	{
    for(int i=0; i<m_lstMergedObjects.Count(); i++)
		  LinkObject(m_lstMergedObjects[i], m_lstObjects[m_lstMergedObjects[i]->GetRenderNodeType()]);
		m_lstMergedObjects.Clear();
	}*/

//	m_fAreaMergingTimeLimit -= (GetCurAsyncTimeSec()-fStartTime);
//}
/*
void CSceneTree::FreeAreaBrushes(bool bRecursive)
{
	if(bRecursive)
		for(int i=0; i<8; i++) 
			if(m_arrChilds[i])
				m_arrChilds[i]->FreeAreaBrushes(bRecursive);

	if(!m_plstAreaBrush)
		return;

	for(int i=0; i<m_plstAreaBrush->Count(); i++)
	{
		CBrush * pBrush = m_plstAreaBrush->GetAt(i);

		CStatObj * pAreaStatObj = (CStatObj *)pBrush->GetEntityStatObj(0);
		if(pAreaStatObj)
		{
			pBrush->SetEntityStatObj(0,0);
			IRenderMesh * pAreaLB = pAreaStatObj->GetRenderMesh();
			pAreaStatObj->SetRenderMesh(0);	
			GetRenderer()->DeleteRenderMesh(pAreaLB);

      delete pAreaStatObj;
		}

		delete pBrush;
		pBrush = NULL;
	}

	SAFE_DELETE(m_plstAreaBrush);
	m_plstAreaBrush = NULL;
}

bool CSceneTree::HasChildNodes()
{
  if( !m_arrChilds[0] && !m_arrChilds[1] && !m_arrChilds[2] && !m_arrChilds[3] )
    if( !m_arrChilds[4] && !m_arrChilds[5] && !m_arrChilds[6] && !m_arrChilds[7] )
      return false;

  return true;
}

void CSceneTree::CheckInitAffectingLights()
{
if(m_nLightMaskFrameId != GetFrameID())
{
m_lstAffectingLights.Clear();

//    if( !m_pVisArea || m_pVisArea->IsAffectedByOutLights() )
{
PodArray<CDLight*> * pSceneLights = Get3DEngine()->GetDynamicLightSources();
if(pSceneLights->Count() && (pSceneLights->GetAt(0)->m_Flags & DLF_SUN))
m_lstAffectingLights.Add(pSceneLights->GetAt(0));
}

m_nLightMaskFrameId = GetFrameID();
}
}

PodArray<CDLight*> * CSceneTree::GetAffectingLights()
{ 
CheckInitAffectingLights();

return &m_lstAffectingLights; 
}

void CSceneTree::AddLightSource(CDLight * pSource)
{ 
if(!m_nodeBox.IsOverlapSphereBounds(pSource->m_Origin,pSource->m_fRadius))
return;

CheckInitAffectingLights();

if(m_lstAffectingLights.Find(pSource)<0)
{
m_lstAffectingLights.Add(pSource); 

for(int i=0; i<8; i++) 
if(m_arrChilds[i])
m_arrChilds[i]->AddLightSource( pSource );
}
}

*/
void COctreeNode::UpdateTerrainNodes()
{
  m_pTerrainNode = (m_nSID>=0 && GetTerrain()) ? GetTerrain()->FindMinNodeContainingBox(m_nodeBox, m_nSID) : NULL;

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->UpdateTerrainNodes();
}

void COctreeNode::GetObjectsForIntegrationIntoTexture(PodArray<SBuildTerrainTextureParams::SBuildItem> & arrItems, const AABB * pBBox, float fViewDist)
{
  if(fViewDist>m_fObjectsMaxViewDist)
    return;

  if(pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    return;

  for(IRenderNode * pObj = m_arrObjects[eRNListType_DecalsAndRoads].m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    if(pObj->GetIntegrationType() >= eIT_VoxelMesh)
    {
      EERType objType = pObj->GetRenderNodeType();

      if(objType == eERType_Decal || objType == eERType_Road)
      {
        if(fViewDist<pObj->m_fWSMaxViewDist)
        {
          if(!pBBox || Overlap::AABB_AABB(*pBBox, pObj->GetBBox()))
          {
            SBuildTerrainTextureParams::SBuildItem & newItem = arrItems.AddNew();
            newItem.pRenderNode = pObj;
          }
        }
      }
    }
  }

  for(int i=0; i<8; i++) 
    if(m_arrChilds[i])
      m_arrChilds[i]->GetObjectsForIntegrationIntoTexture(arrItems, pBBox, fViewDist);
}

void C3DEngine::GetObjectsByTypeGlobal(PodArray<IRenderNode*> & lstObjects, EERType objType, const AABB * pBBox, float fViewDist)
{
  for(int nSID = 0; nSID < Get3DEngine()->m_pObjectsTree.Count(); nSID++)
    if(Get3DEngine()->m_pObjectsTree[nSID])
      Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstObjects, objType, pBBox, fViewDist);
}

void C3DEngine::MoveObjectsIntoListGlobal(PodArray<SRNInfo> * plstResultEntities, const AABB * pAreaBox, 
                                          bool bRemoveObjects, bool bSkipDecals, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects,
                                          EERType eRNType)
{
  for(int nSID = 0; nSID < Get3DEngine()->m_pObjectsTree.Count(); nSID++)
    if(Get3DEngine()->m_pObjectsTree[nSID])
      Get3DEngine()->m_pObjectsTree[nSID]->MoveObjectsIntoList( plstResultEntities, pAreaBox, bRemoveObjects, bSkipDecals, bSkip_ERF_NO_DECALNODE_DECALS, bSkipDynamicObjects, eRNType );
}

SPU_NO_INLINE void COctreeNode::GetObjectsByType(PodArray<IRenderNode*> & lstObjects, EERType objType, const AABB * pBBox, float fViewDist)
{
	if(objType == eERType_Light && !m_bHasLights)
		return;

  if(pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    return;

	ERNListType eListType = IRenderNode::GetRenderNodeListId(objType);

	for(IRenderNode * pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
	{
		if(fViewDist<0 || (pObj->GetIntegrationType() >= eIT_VoxelMesh))
		{
			if(pObj->GetRenderNodeType() == objType)
			{
				if(!pBBox || Overlap::AABB_AABB(*pBBox, pObj->GetBBox()))
				{
					if(fViewDist>0 && pObj->GetIntegrationType() && (pObj->m_dwRndFlags & ERF_SELECTED) && GetCVars()->e_VoxTerOnTheFlyIntegration)
					{
						if( GetCVars()->e_Voxel && GetCVars()->e_VoxTerHideIntegrated==2 )
							continue; // hide always
						if( GetCVars()->e_Voxel && GetCVars()->e_VoxTerHideIntegrated && Get3DEngine()->GetIVoxTerrain() )
							continue; // hide only if voxel terrain used    
					}

					if(fViewDist<0 || fViewDist<pObj->m_fWSMaxViewDist)
						lstObjects.Add(pObj);
				}
			}
		}
	}

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->GetObjectsByType(lstObjects, objType, pBBox, fViewDist);
}

void COctreeNode::GetObjects(PodArray<IRenderNode*> & lstObjects, const AABB * pBBox, float fViewDist)
{
	if(pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
		return;

	unsigned int nCurrentObject(eRNListType_First);
	for (nCurrentObject=eRNListType_First;nCurrentObject<eRNListType_ListsNum;++nCurrentObject)
	{
		for(IRenderNode * pObj = m_arrObjects[nCurrentObject].m_pFirstNode; pObj; pObj = pObj->m_pNext)
		{
			if(fViewDist<0 || (pObj->GetIntegrationType() >= eIT_VoxelMesh))
			{
				if(!pBBox || Overlap::AABB_AABB(*pBBox, pObj->GetBBox()))
				{
					if(fViewDist<0 || fViewDist<pObj->m_fWSMaxViewDist)
						lstObjects.Add(pObj);
				}
			}
		}
	}

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->GetObjects(lstObjects, pBBox, fViewDist);
}

bool COctreeNode::IsObjectTypeInTheBox(EERType objType, const AABB & WSBBox)
{
  if(!Overlap::AABB_AABB(WSBBox, GetObjectsBBox()))
    return false;

  if(objType == eERType_Road && !m_bHasRoads)
    return false;

  ERNListType eListType = IRenderNode::GetRenderNodeListId(objType);

  for(IRenderNode * pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    if(pObj->GetRenderNodeType() == objType)
    {
      if(Overlap::AABB_AABB(WSBBox, pObj->GetBBox()))
        return true;
    }
  }

  for(int i=0; i<8; i++) 
    if(m_arrChilds[i])
      if(m_arrChilds[i]->IsObjectTypeInTheBox(objType, WSBBox))
        return true;

  return false;
}

bool COctreeNode::RayObjectsIntersection2D( Vec3 vStart, Vec3 vEnd, Vec3 & vClosestHitPoint, float & fClosestHitDistance, EERType eERType )
{
  FUNCTION_PROFILER_3DENGINE;

//	Vec3 vBoxHitPoint;
//	if(!Intersect::Ray_AABB(Ray(vStart, vEnd-vStart), m_objectsBox, vBoxHitPoint))
	//	return false;

	if(	vStart.x>m_objectsBox.max.x || vStart.y>m_objectsBox.max.y ||
			vStart.x<m_objectsBox.min.x || vStart.y<m_objectsBox.min.y )
				return false;

	if(!m_bCompiled)
		CompileObjects();

	float fOceanLevel = GetTerrain()->GetWaterLevel();

  ERNListType eListType = IRenderNode::GetRenderNodeListId(eERType);

  for(IRenderNode * pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
	{
		uint32 dwFlags = pObj->GetRndFlags();

		if(dwFlags&ERF_HIDDEN || !(dwFlags&ERF_CASTSHADOWMAPS) || dwFlags&ERF_PROCEDURAL || dwFlags & ERF_COLLISION_PROXY)
			continue;

		if( pObj->GetRenderNodeType() != eERType )
      continue;

//		if(!Intersect::Ray_AABB(Ray(vStart, vEnd-vStart), pObj->GetBBox(), vBoxHitPoint))
	//		continue;

		const AABB & objBox = pObj->GetBBox();

		if((objBox.max.z-objBox.min.z) < 2.f)
			continue;

		if(objBox.max.z < fOceanLevel)
			continue;

		if(	vStart.x>objBox.max.x || vStart.y>objBox.max.y || vStart.x<objBox.min.x || vStart.y<objBox.min.y )
			continue;

		Matrix34A objMatrix;
		CStatObj * pStatObj = (CStatObj*)pObj->GetEntityStatObj(0, 0, &objMatrix);

		if(pStatObj->GetOcclusionAmount() < 0.32f)
			continue;

    {
      if(pStatObj->m_nFlags&STATIC_OBJECT_HIDDEN)
        continue;

      Matrix34 matInv = objMatrix.GetInverted();
      Vec3 vOSStart = matInv.TransformPoint(vStart);
      Vec3 vOSEnd = matInv.TransformPoint(vEnd);
      Vec3 vOSHitPoint(0,0,0), vOSHitNorm(0,0,0);

      Vec3 vBoxHitPoint;
      if(!Intersect::Ray_AABB(Ray(vOSStart, vOSEnd-vOSStart), pStatObj->GetAABB(), vBoxHitPoint))
        continue;

      vOSHitPoint = vOSStart;
      vOSHitPoint.z = pStatObj->GetObjectHeight(vOSStart.x,vOSStart.y);

      if(vOSHitPoint.z!=0)
      {
        Vec3 vHitPoint = objMatrix.TransformPoint(vOSHitPoint);
        float fDist = vHitPoint.GetDistance(vStart);
        if(fDist<fClosestHitDistance)
        {
          fClosestHitDistance = fDist;
          vClosestHitPoint = vHitPoint;
        }
      }
    }
	}

	for(int i=0; i<8; i++) 
		if(m_arrChilds[i])
			m_arrChilds[i]->RayObjectsIntersection2D( vStart, vEnd, vClosestHitPoint, fClosestHitDistance, eERType );

	return false;
}

bool COctreeNode::RayVoxelIntersection2D( Vec3 vStart, Vec3 vEnd, Vec3 & vClosestHitPoint, float & fClosestHitDistance )
{
  FUNCTION_PROFILER_3DENGINE;

  if(	!m_bHasVoxels ||
      vStart.x>m_objectsBox.max.x || vStart.y>m_objectsBox.max.y ||
      vStart.x<m_objectsBox.min.x || vStart.y<m_objectsBox.min.y )
    return false;

  ERNListType eListType = IRenderNode::GetRenderNodeListId(eERType_VoxelObject);

  for(IRenderNode * pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    if( pObj->GetRenderNodeType() != eERType_VoxelObject )
      continue;

    CVoxelObject* pVox = (CVoxelObject*)pObj;

    const AABB & objBox = pVox->GetBBox();

    if(	vStart.x>objBox.max.x || vStart.y>objBox.max.y || vStart.x<objBox.min.x || vStart.y<objBox.min.y )
      continue;

    Matrix34 objMatrix = pVox->GetMatrix();
    IRenderMesh * pRenderMesh = pVox->GetRenderMesh(0);
    if(!pRenderMesh)
      continue;

    Matrix34 matInv = objMatrix.GetInverted();
    Vec3 vOSStart = matInv.TransformPoint(vStart);
    Vec3 vOSEnd = matInv.TransformPoint(vEnd);

    Vec3 vOSHitPoint(0,0,0), vOSHitNorm(0,0,0);
    if(CObjManager::RayRenderMeshIntersection(pRenderMesh, vOSStart, vOSEnd-vOSStart, vOSHitPoint, vOSHitNorm, false, NULL))
    {
      Vec3 vHitPoint = objMatrix.TransformPoint(vOSHitPoint);
      float fDist = vHitPoint.GetDistance(vStart);
      if(fDist<fClosestHitDistance)
      {
        fClosestHitDistance = fDist;
        vClosestHitPoint = vHitPoint;
      }
    }
  }

  for(int i=0; i<8; i++) 
    if(m_arrChilds[i])
      m_arrChilds[i]->RayVoxelIntersection2D( vStart, vEnd, vClosestHitPoint, fClosestHitDistance );

  return false;
}

void COctreeNode::GenerateStatObjAndMatTables(std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, SHotUpdateInfo * pExportInfo)
{
  AABB * pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

  if(pBox && !Overlap::AABB_AABB(m_nodeBox,*pBox))
    return;

  uint32 nObjTypeMask = pExportInfo ? pExportInfo->nObjTypeMask : (uint32)~0;

  for(int l=0; l<eRNListType_ListsNum; l++)
  for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    EERType eType = pObj->GetRenderNodeType();

    if(!(nObjTypeMask & (1<<eType)))
      continue;

    if(GetCVars()->e_VoxTer)
      if( pObj->GetIntegrationType() == eIT_VoxelMesh || pObj->GetIntegrationType() == eIT_VoxelTree )
        continue;

    if( eType == eERType_Brush )
    {
      CBrush * pBrush = (CBrush *)pObj;
      if(CObjManager::GetItemId<IStatObj>(pStatObjTable, pBrush->GetStatObj(), false)<0)
        pStatObjTable->push_back(pBrush->m_pStatObj);
    }

    if( eType == eERType_Brush || 
        eType == eERType_Road ||
        eType == eERType_Decal ||
        eType == eERType_WaterVolume ||
        eType == eERType_DistanceCloud ||
        eType == eERType_WaterWave )
    {
      if((eType != eERType_Brush || ((CBrush*)pObj)->m_pMaterial) && CObjManager::GetItemId(pMatTable, pObj->GetMaterial(), false)<0)
        pMatTable->push_back(pObj->GetMaterial());
    }
  }

  for(int i=0; i<8; i++) 
    if(m_arrChilds[i])
      m_arrChilds[i]->GenerateStatObjAndMatTables(pStatObjTable, pMatTable, pExportInfo);
}

int COctreeNode::Cmp_OctreeNodeSize(const void* v1, const void* v2)
{
  COctreeNode *pNode1 = *((COctreeNode**)v1);
  COctreeNode *pNode2 = *((COctreeNode**)v2);

  if(pNode1->m_fNodeRadius > pNode2->m_fNodeRadius)
    return +1;
  if(pNode1->m_fNodeRadius < pNode2->m_fNodeRadius)
    return -1;

  return 0;
}

bool COctreeNode::IsEmpty()
{
  if( m_pParent )
//  if( !m_lstOccluders.Count() && !m_lstCasters.Count() )
  if( !m_arrChilds[0] && !m_arrChilds[1] && !m_arrChilds[2] && !m_arrChilds[3] )
  if( !m_arrChilds[4] && !m_arrChilds[5] && !m_arrChilds[6] && !m_arrChilds[7] )
  if(!HasObjects())
    return true;

  return false;
}


bool COctreeNode::HasChildNodes()
{
  if( !m_arrChilds[0] && !m_arrChilds[1] && !m_arrChilds[2] && !m_arrChilds[3] )
    if( !m_arrChilds[4] && !m_arrChilds[5] && !m_arrChilds[6] && !m_arrChilds[7] )
      return false;

  return true;
}

void COctreeNode::ReleaseEmptyNodes()
{
  FUNCTION_PROFILER_3DENGINE;

  if(!m_arrEmptyNodes.Count())
    return;

  // sort childs first
  qsort(m_arrEmptyNodes.GetElements(), m_arrEmptyNodes.Count(), sizeof(m_arrEmptyNodes[0]), Cmp_OctreeNodeSize);

  int nInitCunt = m_arrEmptyNodes.Count();

  for(int i=0; i<nInitCunt && m_arrEmptyNodes.Count(); i++)
  {
    COctreeNode * pNode = m_arrEmptyNodes[0];

    // remove from list
    m_arrEmptyNodes.Delete(pNode);

    if( pNode->IsEmpty() )
    {
      COctreeNode * pParent = pNode->m_pParent;

      // unregister in parent
      for(int n=0; n<8; n++)
        if(pParent->m_arrChilds[n] == pNode)
          pParent->m_arrChilds[n] = NULL;

      delete pNode;

      // request parent validation
      if(pParent && pParent->IsEmpty() && m_arrEmptyNodes.Find(pParent)<0)
        m_arrEmptyNodes.Add(pParent);
    }
  }
}

bool COctreeNode::UpdateStreamingPrioriry(PodArray<COctreeNode*> & arrRecursion)
{
//  FUNCTION_PROFILER_3DENGINE;

  const Vec3 vCamPos = GetCamera().GetPosition();

  float fNodeDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos, m_objectsBox));

  if(!m_bCompiled)
    CompileObjects();

  if(fNodeDistance-GetCVars()->e_StreamCgfPredicitionDistance > m_fObjectsMaxViewDist)
    return true;

  CheckManageVegetationSprites(fNodeDistance,64);

  AABB objBox;

  for(int l=0; l<eRNListType_ListsNum; l++)
  {
    for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
#if !defined(PS3) && !defined(XENON)
      if(pObj->m_pNext)
        cryPrefetchT0SSE(pObj->m_pNext);



#endif

#ifdef _DEBUG
      const char * szName = pObj->GetName();
      const char * szClassName = pObj->GetEntityClassName();

      if(pObj->GetRndFlags()&ERF_SELECTED)
      {
        int selected=1;
      }
#endif // _DEBUG

      pObj->FillBBox(objBox);
      float fEntDistance = cry_sqrtf(Distance::Point_AABBSq(vCamPos,objBox))*m_fZoomFactor; // activate objects before they get really visible
      assert(fEntDistance>=0 && _finite(fEntDistance));
      if(fEntDistance-GetCVars()->e_StreamCgfPredicitionDistance < pObj->m_fWSMaxViewDist)
      {
        if(GetVisAreaManager()->GetCurVisArea())
        {
          // search from camera to entity visarea or outdoor
          AABB aabbCam;
          float fResDist = 100000.f;
          aabbCam.min = aabbCam.max = GetCamera().GetPosition();
          if(!GetVisAreaManager()->GetCurVisArea()->GetDistanceThruVisAreas(aabbCam, pObj->GetEntityVisArea(), pObj->GetBBox(), 5, fResDist))
            continue;
          fEntDistance = fResDist;
        }
        else if(pObj->GetEntityVisArea())
        {
          // search from entity to outdoor
          AABB aabbCam;
          float fResDist = 100000.f;
          aabbCam.min = aabbCam.max = GetCamera().GetPosition();
          if(!((CVisArea*)pObj->GetEntityVisArea())->GetDistanceThruVisAreas(pObj->GetBBox(), NULL, aabbCam, 5, fResDist))
            continue;
          fEntDistance = fResDist;
        }

        if(fEntDistance-GetCVars()->e_StreamCgfPredicitionDistance < pObj->m_fWSMaxViewDist)
          GetObjManager()->UpdateRenderNodeStreamingPrioriry(pObj, fEntDistance);
      }
    }
  }

  int nFirst = 
    ((vCamPos.x > m_vNodeCenter.x) ? 4 : 0) |
    ((vCamPos.y > m_vNodeCenter.y) ? 2 : 0) |
    ((vCamPos.z > m_vNodeCenter.z) ? 1 : 0);

  if(m_arrChilds[nFirst  ])
    arrRecursion.Add(m_arrChilds[nFirst  ]);

  if(m_arrChilds[nFirst^1])
    arrRecursion.Add(m_arrChilds[nFirst^1]);

  if(m_arrChilds[nFirst^2])
    arrRecursion.Add(m_arrChilds[nFirst^2]);

  if(m_arrChilds[nFirst^4])
    arrRecursion.Add(m_arrChilds[nFirst^4]);

  if(m_arrChilds[nFirst^3])
    arrRecursion.Add(m_arrChilds[nFirst^3]);

  if(m_arrChilds[nFirst^5])
    arrRecursion.Add(m_arrChilds[nFirst^5]);

  if(m_arrChilds[nFirst^6])
    arrRecursion.Add(m_arrChilds[nFirst^6]);

  if(m_arrChilds[nFirst^7])
    arrRecursion.Add(m_arrChilds[nFirst^7]);

  return true;
}

/*void COctreeNode::UpdateSceneMerging()
{
  TDoublyLinkedList<IRenderNode> & lstObjects = m_arrObjects[eRNListType_Brush];

  for(IRenderNode * pObj = lstObjects.m_pFirstNode; pObj; pObj = pObj->m_pNext)
  {
    ((CBrush*)pObj)->m_bMerged = false;
    Get3DEngine()->m_pSceneTree->RequestUpdate(pObj->GetBBox());
  }

  for(int i=0; i<8; i++) 
    if(m_arrChilds[i])
      m_arrChilds[i]->UpdateSceneMerging();
}

bool CSceneTree::FillShadowCastersList(bool bAllIn, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest)
{
  FUNCTION_PROFILER_3DENGINE;

  bool bSuccess = true;

  if(pFr->bForSubSurfScattering)
    return bSuccess;

  if(bUseFrustumTest && !bAllIn && !pFr->IntersectAABB(m_nodeBox, &bAllIn))
    return bSuccess;

  if(pShadowHull && !pFr->bUseAdditiveBlending && !IsAABBInsideHull(pShadowHull->GetElements(), pShadowHull->Count(), m_nodeBox))
    return bSuccess;

  assert(pFr->pLightOwner == pLight->m_pOwner);

  float fGSMBoxDiameter = 2.0f / (pFr->fFrustrumSize * GetCVars()->e_GsmRange);

  float fNodeHalfSize = (m_nodeBox.max.x-m_nodeBox.min.x)*0.5f;
  bool bDraw = ( !HasChildNodes() 
    || fGSMBoxDiameter*.5f > fNodeHalfSize*2 
    || sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), m_nodeBox))*GetCVars()->e_SceneMergingViewDistRatio > fNodeHalfSize
    );

  if(!bDraw)
  {
    for(int i=0;i<8;i++)
      if(m_arrChilds[i])
        if(!m_arrChilds[i]->FillShadowCastersList(bAllIn, pLight, pFr, pShadowHull, bUseFrustumTest))
          bDraw = true;
  }

  if(bDraw)
  {
    CheckUpdateAreaBrushes();

    bool bSun = (pLight->m_Flags&DLF_SUN) != 0;

    if(bSun && m_nGSMFrameId == GetFrameID())
      return bSuccess;

    if(bSun && bAllIn)
      m_nGSMFrameId = GetFrameID();

    if(m_plstAreaBrush)
    {
      PodArray<CDLight*> * pAffectingLights = GetAffectingLights();

      for(int i=0; i<m_plstAreaBrush->Count(); i++)
      {
        CBrush * pEnt = m_plstAreaBrush->GetAt(i);
        pFr->pCastersList->Add(pEnt);
      }
    }
  }

  return bSuccess;
}
*/
int COctreeNode::Load(FILE * & f, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, AABB * pBox) { return Load_T(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox); }
int COctreeNode::Load(uint8 * & f, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, AABB * pBox) { return Load_T(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox); }

template <class T>
int COctreeNode::Load_T(T * & f, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, AABB * pBox)
{
	if(pBox && !Overlap::AABB_AABB(m_nodeBox,*pBox))
		return 0;

	SOcTreeNodeChunk chunk;
	if(!CTerrain::LoadDataFromFile(&chunk, 1, f, nDataSize, eEndian))
		return 0;

	assert(chunk.nChunkVersion == OCTREENODE_CHUNK_VERSION);
	if(chunk.nChunkVersion != OCTREENODE_CHUNK_VERSION)
		return 0;

	if(chunk.nObjectsBlockSize)
	{
		if(m_mbLoadingCache.GetSize()<chunk.nObjectsBlockSize)
			m_mbLoadingCache.Allocate(chunk.nObjectsBlockSize);

		if(!CTerrain::LoadDataFromFile((uint8*)m_mbLoadingCache.GetData(), chunk.nObjectsBlockSize, f, nDataSize, eEndian))
			return 0;

		if(!m_bEditor)
			LoadObjects((byte*)m_mbLoadingCache.GetData(), (byte*)m_mbLoadingCache.GetData()+chunk.nObjectsBlockSize, pStatObjTable, pMatTable, eEndian);
	}

	// count number of nodes loaded
	int nNodesNum = 1;

	// process childs
	for(int nChildId=0; nChildId<8; nChildId++)
	{
		if(chunk.ucChildsMask & (1<<nChildId))
		{
			if(!m_arrChilds[nChildId])
			{
				MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
				m_arrChilds[nChildId] = new COctreeNode(m_nSID, GetChildBBox(nChildId), m_pVisArea, this);
			}

			int nNewNodesNum = m_arrChilds[nChildId]->Load_T(f, nDataSize, pStatObjTable, pMatTable, eEndian, pBox);

			if(!nNewNodesNum && !pBox)
				return 0; // data error

			nNodesNum += nNewNodesNum;
		}
	}

	return nNodesNum;
}

int COctreeNode::GetData(byte * & pData, int & nDataSize, std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, EEndian eEndian, SHotUpdateInfo * pExportInfo)
{
	AABB * pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

	if(pBox && !Overlap::AABB_AABB(m_nodeBox,*pBox))
		return 0;

	if(pData)
	{
		// get node data
		SOcTreeNodeChunk chunk;
		chunk.nChunkVersion = OCTREENODE_CHUNK_VERSION;
		chunk.nodeBox = m_nodeBox;

		// fill ChildsMask
		chunk.ucChildsMask = 0;
		for(int i=0; i<8; i++)
			if(m_arrChilds[i])
				chunk.ucChildsMask |= (1<<i);

		CMemoryBlock memblock;
		SaveObjects(&memblock,pStatObjTable,pMatTable,eEndian,pExportInfo);

		chunk.nObjectsBlockSize = memblock.GetSize();

		AddToPtr(pData,nDataSize,chunk,eEndian);

		AddToPtr(pData,nDataSize,(byte*)memblock.GetData(),memblock.GetSize(),eEndian);
	}
	else // just count size
	{
		nDataSize += sizeof(SOcTreeNodeChunk);
		nDataSize += SaveObjects(NULL,NULL,NULL,eEndian,pExportInfo);
	}

	// count number of nodes loaded
	int nNodesNum = 1;

	// process childs
	for(int i=0; i<8; i++)
		if(m_arrChilds[i])
			nNodesNum += m_arrChilds[i]->GetData(pData, nDataSize, pStatObjTable, pMatTable,eEndian, pExportInfo);

	return nNodesNum;
}

bool COctreeNode::CleanUpTree()
{
	//  FreeAreaBrushes();

	bool bChildObjectsFound = false;
	for(int i=0; i<8; i++)
	{
		if(m_arrChilds[i])
		{
			if(!m_arrChilds[i]->CleanUpTree())
			{
				delete m_arrChilds[i];
				m_arrChilds[i] = NULL;
			}
			else
				bChildObjectsFound = true;
		}
	}

	// update max view distances

	m_fObjectsMaxViewDist = 0.f;
	m_objectsBox = m_nodeBox;

	for(int l=0; l<eRNListType_ListsNum; l++)
		for(IRenderNode * pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
		{
			pObj->m_fWSMaxViewDist = pObj->GetMaxViewDist();
			m_fObjectsMaxViewDist = max(m_fObjectsMaxViewDist, pObj->m_fWSMaxViewDist);
			m_objectsBox.Add(pObj->GetBBox());
		}

		for(int i=0; i<8; i++)
		{
			if(m_arrChilds[i])
			{
				m_fObjectsMaxViewDist = max(m_fObjectsMaxViewDist, m_arrChilds[i]->m_fObjectsMaxViewDist);
				m_objectsBox.Add(m_arrChilds[i]->m_objectsBox);
			}
		}

		return (bChildObjectsFound || HasObjects());
}

bool COctreeNode::HasObjects()
{
	for(int l=0; l<eRNListType_ListsNum; l++)
		if(m_arrObjects[l].m_pFirstNode)
			return true;

	return false;
}

void COctreeNode::FreeLoadingCache() 
{ 
	m_mbLoadingCache.Free(); 
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::CheckRenderFlagsMinSpec( uint32 dwRndFlags )
{
	if(!m_bEditor)
    if(GetCVars()->e_VoxTer && GetCVars()->e_VoxTerInGameTextureStreaming)
			if(IRenderNode::GetIntegrationTypeFromFlag(dwRndFlags) == eIT_VoxelMesh || IRenderNode::GetIntegrationTypeFromFlag(dwRndFlags) == eIT_VoxelTree)
				return false; // not needed in game

	int nRenderNodeMinSpec = (dwRndFlags&ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT;
	return CheckMinSpec(nRenderNodeMinSpec);
}

DEVIRTUALIZATION_VTABLE_FIX_IMPL(IVoxelObject);
