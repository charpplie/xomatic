////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjmandraw.cpp
//  Version:     v1.00
//  Created:     18/12/2002 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Visibility areas
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <ICryAnimation.h>

#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "terrain_sector.h"
#include "3dEngine.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "terrain.h"
#include "VoxMan.h"
#include "ILMSerializationManager.h"
#include "TimeOfDay.h"
#include "AABBSV.h"

CVisAreaManager::CVisAreaManager()
{
	m_pCurPortal = m_pCurArea = 0;
  m_bSunIsNeeded = false;
	m_pAABBTree = NULL;
}

CVisAreaManager::~CVisAreaManager()
{
	for(int i=0; i<m_lstVisAreas.Count(); i++)
		delete m_lstVisAreas[i];
	for(int i=0; i<m_lstPortals.Count(); i++)
		delete m_lstPortals[i];
	for(int i=0; i<m_lstOcclAreas.Count(); i++)
		delete m_lstOcclAreas[i];
	delete m_pAABBTree;
}

SAABBTreeNode::SAABBTreeNode( PodArray<CVisArea*> & lstAreas, AABB box, int nRecursion ) 
{ 
	memset(this,0,sizeof(*this)); 

	nodeBox = box;

	nRecursion++;
	if(nRecursion>8 || lstAreas.Count()<8)
	{
		nodeAreas.AddList(lstAreas);
		return;
	}
	
	PodArray<CVisArea*> lstAreas0, lstAreas1;
	Vec3 vSize = nodeBox.GetSize();
	Vec3 vCenter = nodeBox.GetCenter();

	AABB nodeBox0 = nodeBox;
	AABB nodeBox1 = nodeBox;

	if(vSize.x>=vSize.y && vSize.x >= vSize.z)
	{
		nodeBox0.min.x = vCenter.x;
		nodeBox1.max.x = vCenter.x;
	}
	else if(vSize.y>=vSize.x && vSize.y >= vSize.z)
	{
		nodeBox0.min.y = vCenter.y;
		nodeBox1.max.y = vCenter.y;
	}
	else 
	{
		nodeBox0.min.z = vCenter.z;
		nodeBox1.max.z = vCenter.z;
	}

	for(int i=0; i<lstAreas.Count(); i++)
	{
		if(Overlap::AABB_AABB(nodeBox0, *lstAreas[i]->GetAABBox()))
			lstAreas0.Add(lstAreas[i]);

		if(Overlap::AABB_AABB(nodeBox1, *lstAreas[i]->GetAABBox()))
			lstAreas1.Add(lstAreas[i]);
	}

	if(lstAreas0.Count())
		arrChilds[0] = new SAABBTreeNode( lstAreas0, nodeBox0, nRecursion ) ;

	if(lstAreas1.Count())
		arrChilds[1] = new SAABBTreeNode( lstAreas1, nodeBox1, nRecursion ) ;
}

SAABBTreeNode::~SAABBTreeNode( ) 
{
	delete arrChilds[0];
	delete arrChilds[1];
}

CVisArea * SAABBTreeNode::FindVisarea(const Vec3 & vPos)
{
	if(nodeBox.IsContainPoint(vPos))
	{
		if(nodeAreas.Count())
		{ // leaf
			for(int i=0; i<nodeAreas.Count(); i++)
				if(nodeAreas[i]->m_bActive && nodeAreas[i]->IsPointInsideVisArea(vPos))
					return nodeAreas[i];
		}
		else
		{ // node
			for(int i=0; i<2; i++)
				if(arrChilds[i])
					if(CVisArea * pArea = arrChilds[i]->FindVisarea(vPos))
						return pArea;
		}
	}

	return NULL;
}

SAABBTreeNode* SAABBTreeNode::GetTopNode(const AABB& box, void** pNodeCache)
{
	AABB boxClip = box;
	boxClip.ClipToBox(nodeBox);

	SAABBTreeNode* pNode = this;
	if (pNodeCache)
	{
		pNode = (SAABBTreeNode*)*pNodeCache;
		if (!pNode || !pNode->nodeBox.ContainsBox(boxClip))
			pNode = this;
	}

	// Find top node containing box.
	for (;;)
	{
		int i; for (i=0; i<2; i++)
		{
			if (pNode->arrChilds[i] && pNode->arrChilds[i]->nodeBox.ContainsBox(boxClip))
			{
				pNode = pNode->arrChilds[i];
				break;
			}
		}
		if (i == 2)
			break;
	}

	if (pNodeCache)
		*(SAABBTreeNode**)pNodeCache = pNode;
	return pNode;
}

bool SAABBTreeNode::IntersectsVisAreas(const AABB& box)
{
	if (nodeBox.IsIntersectBox(box))
	{
		if(nodeAreas.Count())
		{ // leaf
			for(int i=0; i<nodeAreas.Count(); i++)
			{
				if(nodeAreas[i]->m_bActive && nodeAreas[i]->m_boxArea.IsIntersectBox(box))
					return true;
			}
		}
		else
		{ // node
			for(int i=0; i<2; i++)
				if(arrChilds[i])
					if (arrChilds[i]->IntersectsVisAreas(box))
						return true;
		}
	}
	return false;
}

int SAABBTreeNode::ClipOutsideVisAreas(Sphere& sphere, Vec3 const& vNormal)
{
	int nClipped = 0;

	if (sphere.radius > FLT_MAX*0.01f || Overlap::Sphere_AABB(sphere, nodeBox))
	{
		if(nodeAreas.Count())
		{ // leaf
			for(int i=0; i<nodeAreas.Count(); i++)
			{
				if(nodeAreas[i]->m_bActive && Overlap::Sphere_AABB(sphere, nodeAreas[i]->m_boxArea))
					nClipped += nodeAreas[i]->ClipToVisArea(false, sphere, vNormal);
			}
		}
		else
		{ // node
			for(int i=0; i<2; i++)
				if(arrChilds[i])
					nClipped += arrChilds[i]->ClipOutsideVisAreas(sphere, vNormal);
		}
	}

	return nClipped;
}

void CVisAreaManager::UpdateAABBTree()
{
	delete m_pAABBTree;
	PodArray<CVisArea*> lstAreas;
	lstAreas.AddList(m_lstPortals);
	lstAreas.AddList(m_lstVisAreas);

	AABB nodeBox;
	nodeBox.min = Vec3(1000000,1000000,1000000);
	nodeBox.max = -nodeBox.min;
	for(int i=0; i<lstAreas.Count(); i++)
		nodeBox.Add(*lstAreas[i]->GetAABBox());

	m_pAABBTree = new SAABBTreeNode(lstAreas, nodeBox);
}

bool CVisAreaManager::IsEntityVisible(IRenderNode * pEnt)
{
	if(GetCVars()->e_Portals==3)
		return true;

	if(!pEnt->GetEntityVisArea())
		return IsOutdoorAreasVisible();

	return true;
}

void CVisAreaManager::SetCurAreas()
{
	m_pCurArea = 0;
	m_pCurPortal = 0;

	if(!GetCVars()->e_Portals)
		return;

	if(!m_pAABBTree)
		UpdateAABBTree();

	CVisArea * pFound = m_pAABBTree->FindVisarea(GetCamera().GetOccPos());

#ifdef _DEBUG

	// find camera portal id
	for(int v=0; v<m_lstPortals.Count(); v++)
		if(m_lstPortals[v]->m_bActive &&  m_lstPortals[v]->IsPointInsideVisArea(GetCamera().GetOccPos()))
	{
		m_pCurPortal = m_lstPortals[v];
		break;
	}

	// if not inside any portal - try to find area
	if( !m_pCurPortal ) 
	{
//		int nFoundAreasNum = 0;

		// find camera area
		for(int nVolumeId=0; nVolumeId<m_lstVisAreas.Count(); nVolumeId++)
		{
			if(m_lstVisAreas[nVolumeId]->IsPointInsideVisArea(GetCamera().GetOccPos()))
			{
	//			nFoundAreasNum++;
				m_pCurArea = m_lstVisAreas[nVolumeId];
				break;
			}
		}

//		if(nFoundAreasNum>1) // if more than one area found - set cur area to undefined
		{ // todo: try to set joining portal as current
		//	m_pCurArea = 0;
		}
	}

	assert(pFound == m_pCurArea || pFound == m_pCurPortal);

#endif // _DEBUG

	if(pFound)
	{
		if(pFound->IsPortal())
			m_pCurPortal = pFound;
		else
			m_pCurArea = pFound;
	}

	// camera is in outdoors 
	m_lstActiveEntransePortals.Clear();
	if(!m_pCurArea && !m_pCurPortal)
		MakeActiveEntransePortalsList(&GetCamera(), m_lstActiveEntransePortals, 0);
	
/*
	if(m_pCurArea)
	{
		IVisArea * arrAreas[8];
		int nres = m_pCurArea->GetVisAreaConnections(arrAreas, 8);
		nres=nres;
	}
	DefineTrees();*/

/*	if(GetCVars()->e_Portals == 4)
	{
		if(m_pCurPortal)
		{
			IVisArea * arrAreas[64];
			int nConnections = m_pCurPortal->GetVisAreaConnections(arrAreas,64);
			PrintMessage("CurPortal = %s, nConnections = %d", m_pCurPortal->m_sName, nConnections);
		}
		
		if(m_pCurArea)
		{
			IVisArea * arrAreas[64];
			int nConnections = m_pCurArea->GetVisAreaConnections(arrAreas,64);
			PrintMessage("CurArea = %s, nRes = %d", m_pCurArea->m_sName, nConnections);
		}
	}*/
}

bool CVisAreaManager::IsSkyVisible()
{
  return m_bSkyVisible;
}

bool CVisAreaManager::IsOceanVisible()
{
  return m_bOceanVisible;
}

bool CVisAreaManager::IsOutdoorAreasVisible()
{
	if(!m_pCurArea && !m_pCurPortal)
  {
    m_bOutdoorVisible=true;
		return m_bOutdoorVisible; // camera not in the areas
  }

	if(m_pCurPortal && m_pCurPortal->m_lstConnections.Count()==1)
  {
    m_bOutdoorVisible=true;
    return m_bOutdoorVisible; // camera is in exit portal
  }

	if(m_bOutdoorVisible)
		return true; // exit is visible

	// note: outdoor camera is no modified in this case
	return false;
}

/*void CVisAreaManager::SetAreaFogVolume(CTerrain * pTerrain, CVisArea * pVisArea)
{
	pVisArea->m_pFogVolume=0;
	for(int f=0; f<Get3DEngine()->GetFogVolumes().Count(); f++)
	{
		const Vec3 & v1Min = Get3DEngine()->GetFogVolumes()[f].box.min;
		const Vec3 & v1Max = Get3DEngine()->GetFogVolumes()[f].box.max;
		const Vec3 & v2Min = pVisArea->m_boxArea.min;
		const Vec3 & v2Max = pVisArea->m_boxArea.max;

		if(v1Max.x>v2Min.x && v2Max.x>v1Min.x)
		if(v1Max.y>v2Min.y && v2Max.y>v1Min.y)
		if(v1Max.z>v2Min.z && v2Max.z>v1Min.z)
		if(!Get3DEngine()->GetFogVolumes()[f].bOcean)
		{
      Vec3 arrVerts3d[8] = 
      {
        Vec3(v1Min.x,v1Min.y,v1Min.z),
        Vec3(v1Min.x,v1Max.y,v1Min.z),
        Vec3(v1Max.x,v1Min.y,v1Min.z),
        Vec3(v1Max.x,v1Max.y,v1Min.z),
        Vec3(v1Min.x,v1Min.y,v1Max.z),
        Vec3(v1Min.x,v1Max.y,v1Max.z),
        Vec3(v1Max.x,v1Min.y,v1Max.z),
        Vec3(v1Max.x,v1Max.y,v1Max.z)
      };

      bool bIntersect = false;
      for(int i=0; i<8; i++)
        if(pVisArea->IsPointInsideVisArea(arrVerts3d[i]))
        {
          bIntersect = true;
          break;
        }

      if(!bIntersect)
        if(pVisArea->IsPointInsideVisArea((v1Min+v1Max)*0.5f))
          bIntersect = true;

      if(!bIntersect)
      {
        for(int i=0; i<pVisArea->m_lstShapePoints.Count(); i++)
          if(Get3DEngine()->GetFogVolumes()[f].IsInsideBBox(pVisArea->m_lstShapePoints[i]))
          {
            bIntersect = true;
            break;
          }
      }

			if(!bIntersect)
			{
				Vec3 vCenter = (pVisArea->m_boxArea.min+pVisArea->m_boxArea.max)*0.5f;
				if(Get3DEngine()->GetFogVolumes()[f].IsInsideBBox(vCenter))
					bIntersect = true;
			}

      if(bIntersect)
      {
			  pVisArea->m_pFogVolume = &Get3DEngine()->GetFogVolumes()[f];
			  Get3DEngine()->GetFogVolumes()[f].bIndoorOnly = true;
			  pTerrain->UnregisterFogVolumeFromOutdoor(&Get3DEngine()->GetFogVolumes()[f]);
			  break;
      }
		}
	}
}*/

void CVisAreaManager::PortalsDrawDebug()
{
	UpdateConnections();
/*
	if(m_pCurArea)
	{
		for(int p=0; p<m_pCurArea->m_lstConnections.Count(); p++)
		{
			CVisArea * pPortal = m_pCurArea->m_lstConnections[p];
			float fBlink = gEnv->pTimer->GetFrameStartTime().GetPeriodicFraction(1.0f)>0.5f ? 1.0f : 0.0f;
			float fError = pPortal->IsPortalValid() ? 1.0f : fBlink;
			GetRenderer()->SetMaterialColor(fError,fError*(pPortal->m_lstConnections.Count()<2),0,0.25f);
			DrawBBox(pPortal->m_boxArea.min, pPortal->m_boxArea.max, DPRIM_SOLID_BOX);
			GetRenderer()->DrawLabel((pPortal->m_boxArea.min+ pPortal->m_boxArea.max)*0.5f,
				2,pPortal->m_sName);
		}
	}
	else*/
	{
		// debug draw areas
		GetRenderer()->SetMaterialColor(0,1,0,0.25f);
		Vec3 oneVec(1,1,1);
		for(int v=0; v<m_lstVisAreas.Count(); v++)
		{
			DrawBBox(m_lstVisAreas[v]->m_boxArea.min, m_lstVisAreas[v]->m_boxArea.max);//, DPRIM_SOLID_BOX);
			GetRenderer()->DrawLabelEx((m_lstVisAreas[v]->m_boxArea.min+ m_lstVisAreas[v]->m_boxArea.max)*0.5f,
        1,(float*)&oneVec,0,1,m_lstVisAreas[v]->m_sName);

			GetRenderer()->SetMaterialColor(0,1,0,0.25f);
			DrawBBox(m_lstVisAreas[v]->m_boxStatics.min, m_lstVisAreas[v]->m_boxStatics.max);
		}

		// debug draw portals
		for(int v=0; v<m_lstPortals.Count(); v++)
		{
			float fBlink = gEnv->pTimer->GetFrameStartTime().GetPeriodicFraction(1.0f)>0.5f ? 1.0f : 0.0f;
			float fError = m_lstPortals[v]->IsPortalValid() ? 1.f : fBlink;
			GetRenderer()->SetMaterialColor(fError,fError*(m_lstPortals[v]->m_lstConnections.Count()<2),0,0.25f);
			DrawBBox(m_lstPortals[v]->m_boxArea.min, m_lstPortals[v]->m_boxArea.max);//, DPRIM_SOLID_BOX);

			GetRenderer()->DrawLabelEx((m_lstPortals[v]->m_boxArea.min+ m_lstPortals[v]->m_boxArea.max)*0.5f,
				1,(float*)&oneVec,0,1,m_lstPortals[v]->m_sName);

			CVisArea * pPortal = m_lstPortals[v];
			Vec3 vCenter = (pPortal->m_boxArea.min+pPortal->m_boxArea.max)*0.5f;
			DrawBBox(vCenter-Vec3(0.1f,0.1f,0.1f), vCenter+Vec3(0.1f,0.1f,0.1f));
			for(int i=0; i<pPortal->m_lstConnections.Count() && i<2; i++)
				DrawLine(vCenter, vCenter+pPortal->m_vConnNormals[i]);

			GetRenderer()->SetMaterialColor(0,0,1,0.25f);
			DrawBBox(pPortal->m_boxStatics.min, pPortal->m_boxStatics.max);
		}

/*
		// debug draw area shape
		GetRenderer()->SetMaterialColor(0,0,1,0.25f);
		for(int v=0; v<m_lstVisAreas.Count(); v++)
		for(int p=0; p<m_lstVisAreas[v]->m_lstShapePoints.Count(); p++)
			GetRenderer()->DrawLabel(m_lstVisAreas[v]->m_lstShapePoints[p], 2,"%d", p);
		for(int v=0; v<m_lstPortals.Count(); v++)
		for(int p=0; p<m_lstPortals[v]->m_lstShapePoints.Count(); p++)
			GetRenderer()->DrawLabel(m_lstPortals[v]->m_lstShapePoints[p], 2,"%d", p);*/
	}
}

bool CVisAreaManager::SetEntityArea(IRenderNode* pEnt, const AABB & objBox, const float fObjRadiusSqr)
{
	assert(pEnt);

  Vec3 vEntCenter = Get3DEngine()->GetEntityRegisterPoint( pEnt );

	// find portal containing object center
	CVisArea * pVisArea = NULL;
	for(int v=0; v<m_lstPortals.Count(); v++)
	{
		if(m_lstPortals[v]->IsPointInsideVisArea(vEntCenter))
		{
			pVisArea = m_lstPortals[v];
			if(!pVisArea->m_pObjectsTree)
			{
				MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
				pVisArea->m_pObjectsTree = new COctreeNode(-1, pVisArea->m_boxArea, pVisArea);
			}
			pVisArea->m_pObjectsTree->InsertObject(pEnt, objBox, fObjRadiusSqr, vEntCenter);
			break;
		}
	}

  if(!pVisArea && pEnt->m_dwRndFlags & ERF_REGISTER_BY_BBOX)
  {
    for(int v=0; v<m_lstPortals.Count(); v++)
    {
      if(m_lstPortals[v]->IsBoxOverlapVisArea(pEnt->GetBBox()))
      {
        pVisArea = m_lstPortals[v];
        if(!pVisArea->m_pObjectsTree)
				{
					MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
					pVisArea->m_pObjectsTree = new COctreeNode(-1, pVisArea->m_boxArea, pVisArea);
				}
        pVisArea->m_pObjectsTree->InsertObject(pEnt, objBox, fObjRadiusSqr, vEntCenter);
        break;
      }
    }
  }

	if(!pVisArea) // if portal not found - find volume
	for(int v=0; v<m_lstVisAreas.Count(); v++)
	{
		if(m_lstVisAreas[v]->IsPointInsideVisArea(vEntCenter))
		{
			pVisArea = m_lstVisAreas[v];
			if(!pVisArea->m_pObjectsTree)
			{
				MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Terrain, EMemStatContextFlags::MSF_Instance, "Octree node");
				pVisArea->m_pObjectsTree = new COctreeNode(-1, pVisArea->m_boxArea, pVisArea);
			}
			pVisArea->m_pObjectsTree->InsertObject(pEnt, objBox, fObjRadiusSqr, vEntCenter);
			break;
		}
	}

	if(pVisArea && pEnt->GetRenderNodeType()==eERType_Brush) // update bbox of exit portal //			if((*pVisArea)->m_lstConnections.Count()==1)
		if(pVisArea->IsPortal())
			pVisArea->UpdateGeometryBBox();

	return pVisArea != 0;
}

void CVisAreaManager::DrawVisibleSectors()
{
	for(int i=0; i<m_lstVisibleAreas.Count(); i++)
	{
		CVisArea * pArea = m_lstVisibleAreas[i];	
		Vec3 vAmbColor = pArea->GetFinalAmbientColor();
		if(pArea->m_pObjectsTree)
			for(int c=0; c<pArea->m_lstCurCameras.Count(); c++)
				pArea->m_pObjectsTree->Render_Object_Nodes(false, pArea->m_lstCurCameras[c], OCTREENODE_RENDER_FLAG_OBJECTS, vAmbColor, -1);
	}
}

void CVisAreaManager::CheckVis()
{
	FUNCTION_PROFILER_3DENGINE;

	if(!m_nRenderStackLevel)
  {
    m_bOutdoorVisible = false;
    m_bSkyVisible = false;
    m_bOceanVisible = false;
  }

	m_lstOutdoorPortalCameras.Clear();
	m_lstVisibleAreas.Clear();
	m_bSunIsNeeded = false;
	GetRenderer()->EF_ClearDeferredVisAreasList();

	SetCurAreas();

  CCamera camRoot = GetCamera();
  camRoot.m_ScissorInfo.x1 = 0;
  camRoot.m_ScissorInfo.y1 = 0;
  camRoot.m_ScissorInfo.x2 = GetRenderer()->GetWidth(); // todo: use values from camera
  camRoot.m_ScissorInfo.y2 = GetRenderer()->GetHeight();

	if(GetCVars()->e_Portals==3)
	{	// draw everything for debug
		for(int i=0; i<m_lstVisAreas.Count(); i++)
			if(camRoot.IsAABBVisible_F( AABB(m_lstVisAreas[i]->m_boxArea.min,m_lstVisAreas[i]->m_boxArea.max) ))
				m_lstVisAreas[i]->PreRender(0, camRoot, 0, m_pCurPortal, &m_bOutdoorVisible, &m_lstOutdoorPortalCameras, &m_bSkyVisible, &m_bOceanVisible, m_lstVisibleAreas);

		for(int i=0; i<m_lstPortals.Count(); i++)
			if(camRoot.IsAABBVisible_F( AABB(m_lstPortals[i]->m_boxArea.min,m_lstPortals[i]->m_boxArea.max) ))
				m_lstPortals[i]->PreRender(0, camRoot, 0, m_pCurPortal, &m_bOutdoorVisible, &m_lstOutdoorPortalCameras, &m_bSkyVisible, &m_bOceanVisible, m_lstVisibleAreas);
	}
	else
	{
		if(m_nRenderStackLevel)
		{ // use another starting point for reflections
			CVisArea * pVisArea = (CVisArea *)GetVisAreaFromPos(camRoot.GetOccPos());
			if(pVisArea)
				pVisArea->PreRender(3, camRoot, 0, m_pCurPortal, &m_bOutdoorVisible, &m_lstOutdoorPortalCameras, &m_bSkyVisible, &m_bOceanVisible, m_lstVisibleAreas);
		}
		else if(m_pCurArea) 
		{ // camera inside some sector
			m_pCurArea->PreRender(8, camRoot, 0, m_pCurPortal, &m_bOutdoorVisible, &m_lstOutdoorPortalCameras, &m_bSkyVisible, &m_bOceanVisible, m_lstVisibleAreas);
			
			for(int ii=0; ii<m_lstOutdoorPortalCameras.Count(); ii++) // process all exit portals
			{ // for each portal build list of potentially visible entrances into other areas
				MakeActiveEntransePortalsList(&m_lstOutdoorPortalCameras[ii], m_lstActiveEntransePortals, (CVisArea*)m_lstOutdoorPortalCameras[ii].m_pPortal);
				for(int i=0; i<m_lstActiveEntransePortals.Count(); i++) // entrance into another building is visible
					m_lstActiveEntransePortals[i]->PreRender(i==0 ? 5 : 1, 
					m_lstOutdoorPortalCameras.Count() ? m_lstOutdoorPortalCameras[0] : camRoot, 0, m_pCurPortal, 0, 0, 0, 0, m_lstVisibleAreas);
			}

			// reset scissor if skybox is visible also thru skyboxonly portal
			if(m_bSkyVisible && m_lstOutdoorPortalCameras.Count()==1)
				m_lstOutdoorPortalCameras[0].m_ScissorInfo.x1 = 
				m_lstOutdoorPortalCameras[0].m_ScissorInfo.x2 = 
				m_lstOutdoorPortalCameras[0].m_ScissorInfo.y1 = 
				m_lstOutdoorPortalCameras[0].m_ScissorInfo.y2 = 0;
		}
		else if(m_pCurPortal) 
		{	// camera inside some portal
			m_pCurPortal->PreRender(7, camRoot, 0, m_pCurPortal, &m_bOutdoorVisible, &m_lstOutdoorPortalCameras, &m_bSkyVisible, &m_bOceanVisible, m_lstVisibleAreas);

      if(m_pCurPortal->m_lstConnections.Count()==1)
        m_lstOutdoorPortalCameras.Clear(); // camera in outdoor

			if(m_pCurPortal->m_lstConnections.Count()==1 || m_lstOutdoorPortalCameras.Count())
			{ // if camera is in exit portal or exit is visible
				MakeActiveEntransePortalsList(m_lstOutdoorPortalCameras.Count() ? &m_lstOutdoorPortalCameras[0] : &camRoot, 
					m_lstActiveEntransePortals, 
					m_lstOutdoorPortalCameras.Count() ? (CVisArea*)m_lstOutdoorPortalCameras[0].m_pPortal : m_pCurPortal);
				for(int i=0; i<m_lstActiveEntransePortals.Count(); i++) // entrance into another building is visible
					m_lstActiveEntransePortals[i]->PreRender(i==0 ? 5 : 1, 
					m_lstOutdoorPortalCameras.Count() ? m_lstOutdoorPortalCameras[0] : camRoot, 0, m_pCurPortal, 0, 0, 0, 0, m_lstVisibleAreas);
//				m_lstOutdoorPortalCameras.Clear();  // otherwise ocean in fleet was not scissored
			}
		}
		else if(m_lstActiveEntransePortals.Count())
		{ // camera in outdoors - process visible entrance portals
			for(int i=0; i<m_lstActiveEntransePortals.Count(); i++)
				m_lstActiveEntransePortals[i]->PreRender(5, camRoot, 0, m_lstActiveEntransePortals[i], &m_bOutdoorVisible, &m_lstOutdoorPortalCameras, &m_bSkyVisible, &m_bOceanVisible, m_lstVisibleAreas);
			m_lstActiveEntransePortals.Clear();

			// do not recurse to another building since we already processed all potential entrances
			m_lstOutdoorPortalCameras.Clear(); // use default camera
			m_bOutdoorVisible=true;
		}
	}

	if(GetCVars()->e_Portals==2)
		PortalsDrawDebug();
}

void CVisAreaManager::ActivatePortal(const Vec3 &vPos, bool bActivate, const char * szEntityName)
{
//  bool bFound = false;

	for(int v=0; v<m_lstPortals.Count(); v++)
	{
    AABB aabb;
    aabb.min=m_lstPortals[v]->m_boxArea.min - Vec3(0.5f, 0.5f, 0.1f);
    aabb.max=m_lstPortals[v]->m_boxArea.max + Vec3(0.5f, 0.5f, 0.0f);

    if( Overlap::Point_AABB(vPos,aabb) )
    {
			m_lstPortals[v]->m_bActive = bActivate;

      // switch to PrintComment once portals activation is working stable
      PrintComment("I3DEngine::ActivatePortal(): Portal %s is %s by entity %s at position(%.1f,%.1f,%.1f)", 
        m_lstPortals[v]->GetName(), bActivate ? "Enabled" : "Disabled", szEntityName, vPos.x, vPos.y, vPos.z);

//      bFound = true;
    }
	}

	/*
  if(!bFound)
  {
    PrintComment("I3DEngine::ActivatePortal(): Portal not found for entity %s at position(%.1f,%.1f,%.1f)", 
      szEntityName, vPos.x, vPos.y, vPos.z);
  }
	*/
}
/*
bool CVisAreaManager::IsEntityInVisibleArea(IRenderNodeState * pRS)
{
	if( pRS && pRS->plstVisAreaId && pRS->plstVisAreaId->Count() )
	{
		PodArray<int> * pVisAreas = pRS->plstVisAreaId;
		for(int n=0; n<pVisAreas->Count(); n++)
			if( m_lstVisAreas[pVisAreas->GetAt(n)].m_nVisFrameId==GetFrameID() )
				break;

		if(n==pVisAreas->Count())
			return false; // no visible areas
	}
	else
		return false; // entity is not inside

	return true;
}	*/

bool CVisAreaManager::IsValidVisAreaPointer(CVisArea * pVisArea)
{
  if( m_lstVisAreas.Find(pVisArea)<0 &&
      m_lstPortals.Find(pVisArea)<0 && 
      m_lstOcclAreas.Find(pVisArea)<0 )
    return false;

  return true;
}

bool CVisAreaManager::DeleteVisArea(CVisArea * pVisArea)
{
  bool bFound = false;
	if(m_lstVisAreas.Delete(pVisArea) || m_lstPortals.Delete(pVisArea) || m_lstOcclAreas.Delete(pVisArea))
  {
    delete pVisArea;
    bFound = true;
  }

	m_lstActiveOcclVolumes.Delete(pVisArea);
	m_lstIndoorActiveOcclVolumes.Delete(pVisArea);
	m_lstActiveEntransePortals.Delete(pVisArea);

  m_pCurArea = 0;
  m_pCurPortal = 0;
  UpdateConnections();

	delete m_pAABBTree;
	m_pAABBTree = NULL;

  return bFound;
}

/*void CVisAreaManager::LoadVisAreaShapeFromXML(XmlNodeRef pDoc)
{
	for(int i=0; i<m_lstVisAreas.Count(); i++)
	{
		delete m_lstVisAreas[i];
		m_lstVisAreas.Delete(i);
		i--;
	}

	for(int i=0; i<m_lstPortals.Count(); i++)
	{
		delete m_lstPortals[i];
		m_lstPortals.Delete(i);
		i--;
	}

	// fill list of volumes of shape points
	XmlNodeRef pObjectsNode = pDoc->findChild("Objects");
	if (pObjectsNode)
	{
		for (int i = 0; i < pObjectsNode->getChildCount(); i++)
		{
			XmlNodeRef pNode = pObjectsNode->getChild(i);
			if (pNode->isTag("Object"))
			{
        const char * pType = pNode->getAttr("Type");
				if (strstr(pType,"OccluderArea") || strstr(pType,"VisArea") || strstr(pType,"Portal"))
				{
					CVisArea * pArea = new CVisArea();

					pArea->m_boxArea.max=SetMinBB();
					pArea->m_boxArea.min=SetMaxBB();

					// set name
					strcpy(pArea->m_sName, pNode->getAttr("Name"));
					strlwr(pArea->m_sName);

					// set height
					pNode->getAttr("Height",pArea->m_fHeight);

					// set ambient color
					pNode->getAttr("AmbientColor", pArea->m_vAmbColor);

					// set dynamic ambient color
//					pNode->getAttr("DynAmbientColor", pArea->m_vDynAmbColor);

          // set SkyOnly flag
          pNode->getAttr("SkyOnly", pArea->m_bSkyOnly);

          // set AfectedByOutLights flag
          pNode->getAttr("AffectedBySun", pArea->m_bAfectedByOutLights);

					// set ViewDistRatio
					pNode->getAttr("ViewDistRatio", pArea->m_fViewDistRatio);

					// set DoubleSide flag
					pNode->getAttr("DoubleSide", pArea->m_bDoubleSide);

					// set UseInIndoors flag
					pNode->getAttr("UseInIndoors", pArea->m_bUseInIndoors);

					if(strstr(pType, "OccluderArea"))
						m_lstOcclAreas.Add(pArea);
					else if(strstr(pArea->m_sName,"portal") || strstr(pType,"Portal"))
						m_lstPortals.Add(pArea);
					else
						m_lstVisAreas.Add(pArea);

					// load vertices
					XmlNodeRef pPointsNode = pNode->findChild("Points");
					if (pPointsNode)
					for (int i = 0; i < pPointsNode->getChildCount(); i++)
					{
						XmlNodeRef pPointNode = pPointsNode->getChild(i);

						Vec3 vPos;
						if (pPointNode->isTag("Point") && pPointNode->getAttr("Pos", vPos))
						{
							pArea->m_lstShapePoints.Add(vPos);
							pArea->m_boxArea.max.CheckMax(vPos);
							pArea->m_boxArea.min.CheckMin(vPos);
							pArea->m_boxArea.max.CheckMax(vPos+Vec3(0,0,pArea->m_fHeight));
							pArea->m_boxArea.min.CheckMin(vPos+Vec3(0,0,pArea->m_fHeight));
						}
					}
					pArea->UpdateGeometryBBox();
				}
			}
		}
	}

	// load area boxes to support old way
//	LoadVisAreaBoxFromXML(pDoc);
}*/

void CVisAreaManager::UpdateVisArea(CVisArea * pArea, const Vec3 * pPoints, int nCount, const char * szName, const SVisAreaInfo & info)
{ // on first update there will be nothing to delete, area will be added into list only in this function
	m_lstPortals.Delete(pArea);
	m_lstVisAreas.Delete(pArea);
	m_lstOcclAreas.Delete(pArea);

	pArea->Update(pPoints, nCount, szName, info);

	strlwr(pArea->m_sName);
	if(strstr(pArea->m_sName,"portal"))
	{
		if(pArea->m_lstConnections.Count()==1)
			pArea->UpdateGeometryBBox();

		m_lstPortals.Add(pArea);
	}
	else if(strstr(pArea->m_sName,"visarea"))
		m_lstVisAreas.Add(pArea);
  else if(strstr(pArea->m_sName, "occlarea"))
    m_lstOcclAreas.Add(pArea);

	UpdateConnections();

	// disable terrain culling for tunnels
  pArea->UpdateOcclusionFlagInTerrain();

	delete m_pAABBTree;
	m_pAABBTree = NULL;
}

void CVisAreaManager::UpdateConnections()
{
	// Reset connectivity
	for(int p=0; p<m_lstPortals.Count(); p++)
		m_lstPortals[p]->m_lstConnections.Clear();

	for(int v=0; v<m_lstVisAreas.Count(); v++)
		m_lstVisAreas[v]->m_lstConnections.Clear();

	// Init connectivity - check intersection of all areas and portals
	for(int p=0; p<m_lstPortals.Count(); p++)
	{
//    if(strstr(m_lstPortals[p]->m_sName,"l5"))
  //    int y=0;

		for(int v=0; v<m_lstVisAreas.Count(); v++)
		{
			if(m_lstVisAreas[v]->IsPortalIntersectAreaInValidWay(m_lstPortals[p]))
			{ // if bboxes intersect
				m_lstVisAreas[v]->m_lstConnections.Add(m_lstPortals[p]);
				m_lstPortals[p]->m_lstConnections.Add(m_lstVisAreas[v]);

				// set portal direction
				Vec3 vNormal = m_lstVisAreas[v]->GetConnectionNormal(m_lstPortals[p]);
				if(m_lstPortals[p]->m_lstConnections.Count()<=2)
					m_lstPortals[p]->m_vConnNormals[m_lstPortals[p]->m_lstConnections.Count()-1] = vNormal;
			}
		}
	}
}

void CVisAreaManager::MoveObjectsIntoList(PodArray<SRNInfo> * plstVisAreasEntities, const AABB & boxArea, bool bRemoveObjects)
{
	for(int p=0; p<m_lstPortals.Count(); p++)
	{	
		if(m_lstPortals[p]->m_pObjectsTree && Overlap::AABB_AABB(m_lstPortals[p]->m_boxArea, boxArea))
      m_lstPortals[p]->m_pObjectsTree->MoveObjectsIntoList(plstVisAreasEntities, bRemoveObjects ? NULL : &boxArea, bRemoveObjects );
	}

	for(int v=0; v<m_lstVisAreas.Count(); v++)
	{
		if(m_lstVisAreas[v]->m_pObjectsTree && Overlap::AABB_AABB(m_lstVisAreas[v]->m_boxArea, boxArea))
			m_lstVisAreas[v]->m_pObjectsTree->MoveObjectsIntoList(plstVisAreasEntities, bRemoveObjects ? NULL : &boxArea, bRemoveObjects );
	}
}

IVisArea * CVisAreaManager::GetVisAreaFromPos(const Vec3 &vPos)
{
	FUNCTION_PROFILER_3DENGINE;

	if(!m_pAABBTree)
		UpdateAABBTree();

	return m_pAABBTree->FindVisarea(vPos);
}

bool CVisAreaManager::IntersectsVisAreas(const AABB& box, void** pNodeCache)
{
	FUNCTION_PROFILER_3DENGINE;

	if(!m_pAABBTree)
		UpdateAABBTree();
	SAABBTreeNode* pTopNode = m_pAABBTree->GetTopNode(box, pNodeCache);
	return pTopNode->IntersectsVisAreas(box);
}

bool CVisAreaManager::ClipOutsideVisAreas(Sphere& sphere, Vec3 const& vNormal, void* pNodeCache)
{
	FUNCTION_PROFILER_3DENGINE;

	if(!m_pAABBTree)
		UpdateAABBTree();
	AABB box(sphere.center - Vec3(sphere.radius), sphere.center + Vec3(sphere.radius));
	SAABBTreeNode* pTopNode = m_pAABBTree->GetTopNode(box, &pNodeCache);
	return pTopNode->ClipOutsideVisAreas(sphere, vNormal) > 0;
}

CVisArea * CVisAreaManager::CreateVisArea()
{
	CVisArea * p = new CVisArea();
	return p;
}

bool CVisAreaManager::IsEntityVisAreaVisibleReqursive(CVisArea * pVisArea, int nMaxReqursion, PodArray<CVisArea*> * pUnavailableAreas, const CDLight * pLight )
{
	int nAreaId = pUnavailableAreas->Count();
	pUnavailableAreas->Add(pVisArea);

	bool bFound = false;
	if(pVisArea)
	{ // check is lsource area was rendered in prev frame
		int nRndFrameId = GetFrameID();
		if(abs(pVisArea->m_nRndFrameId - nRndFrameId)>2)
		{
			if(nMaxReqursion > 1)
				for(int n=0; n<pVisArea->m_lstConnections.Count(); n++)
				{ // loop other sectors
					CVisArea * pNeibArea = (CVisArea*)pVisArea->m_lstConnections[n];
          if( -1 == pUnavailableAreas->Find( pNeibArea ) && 
            (!pLight || Overlap::Sphere_AABB(Sphere(pLight->m_Origin, pLight->m_fRadius), *pNeibArea->GetAABBox())))
						if( IsEntityVisAreaVisibleReqursive( pNeibArea, nMaxReqursion-1, pUnavailableAreas, pLight ) )
						{
							bFound = true;
							break;
						}//if visible
				}// for
		}
		else
			bFound = true;
	}
	else if(IsOutdoorAreasVisible())									//Indirect - outdoor can be a problem!
		bFound = true;

	pUnavailableAreas->Delete( nAreaId );
	return bFound;
}

bool CVisAreaManager::IsEntityVisAreaVisible(IRenderNode * pEnt, int nMaxReqursion, const CDLight * pLight )
{	
	static PodArray<CVisArea*> lUnavailableAreas;

	lUnavailableAreas.Clear();

	lUnavailableAreas.PreAllocate( nMaxReqursion,0 );

	return IsEntityVisAreaVisibleReqursive( (CVisArea*)pEnt->GetEntityVisArea(), nMaxReqursion, &lUnavailableAreas, pLight );
/*
	if(pEnt->GetEntityVisArea())
	{
		if(pEnt->GetEntityVisArea())//->IsPortal())
		{ // check is lsource area was rendered in prev frame
			CVisArea * pVisArea = pEnt->GetEntityVisArea();
			int nRndFrameId = GetFrameID();
			if(abs(pVisArea->m_nRndFrameId - nRndFrameId)>2)
			{
				if(!nCheckNeighbors)
					return false; // this area is not visible

				// try neibhour areas
				bool bFound = false;
				if(pEnt->GetEntityVisArea()->IsPortal())
				{
					CVisArea * pPort = pEnt->GetEntityVisArea();
					for(int n=0; n<pPort->m_lstConnections.Count(); n++)
					{ // loop other sectors
						CVisArea * pNeibArea = (CVisArea*)pPort->m_lstConnections[n];
						if(abs(pNeibArea->m_nRndFrameId - GetFrameID())<=2)
						{
							bFound=true;
							break;
						}
					}
				}
				else
				{
					for(int t=0; !bFound && t<pVisArea->m_lstConnections.Count(); t++)
					{ // loop portals
						CVisArea * pPort = (CVisArea*)pVisArea->m_lstConnections[t];
						if(abs(pPort->m_nRndFrameId - GetFrameID())<=2)
						{
							bFound=true;
							break;
						}

						for(int n=0; n<pPort->m_lstConnections.Count(); n++)
						{ // loop other sectors
							CVisArea * pNeibArea = (CVisArea*)pPort->m_lstConnections[n];
							if(abs(pNeibArea->m_nRndFrameId - GetFrameID())<=2)
							{
								bFound=true;
								break;
							}
						}
					}
				}
				
				if(!bFound)
					return false;

				return true;
			}
		}
		else
			return false; // not visible
	}
	else if(!IsOutdoorAreasVisible())
		return false;

	return true;
*/
}	

int __cdecl CVisAreaManager__CmpDistToPortal(const void* v1, const void* v2)
{
	CVisArea * p1 = *((CVisArea **)v1);
  CVisArea * p2 = *((CVisArea **)v2);

  if(!p1 || !p2)
    return 0;

  if(p1->m_fDistance > p2->m_fDistance)
    return 1;
  else if(p1->m_fDistance < p2->m_fDistance)
    return -1;

  return 0;
}

void CVisAreaManager::MakeActiveEntransePortalsList(const CCamera * pCamera, PodArray<CVisArea *> & lstActiveEntransePortals, CVisArea * pThisPortal)
{
	lstActiveEntransePortals.Clear();
  float fZoomFactor = pCamera ? (0.2f+0.8f*(RAD2DEG(pCamera->GetFov())/90.f)) : 1.f;

	for(int nPortalId=0; nPortalId<m_lstPortals.Count(); nPortalId++)
	{
		CVisArea * pPortal = m_lstPortals[nPortalId];

		if(pPortal->m_lstConnections.Count()==1 && pPortal != pThisPortal && pPortal->IsActive() && !pPortal->m_bSkyOnly)
		{
			if(!pCamera || pCamera->IsAABBVisible_F( pPortal->m_boxStatics ))
			{
			  Vec3 vNormal = pPortal->m_lstConnections[0]->GetConnectionNormal(pPortal);
			  Vec3 vCenter = (pPortal->m_boxArea.min+pPortal->m_boxArea.max)*0.5f;
			  if(vNormal.Dot(vCenter - (pCamera ? pCamera->GetPosition() : GetCamera().GetPosition()))<0)
				  continue;
/*
				if(pCurPortal)
				{
					vNormal = pCurPortal->m_vConnNormals[0];
					if(vNormal.Dot(vCenter - curCamera.GetPosition())<0)
						continue;
				}
	*/
        pPortal->m_fDistance = pPortal->m_boxArea.GetDistance(pCamera ? pCamera->GetPosition() : GetCamera().GetPosition());

				float fRadius = (pPortal->m_boxArea.max-pPortal->m_boxArea.min).GetLength()*0.5f;
				if(pPortal->m_fDistance*fZoomFactor > fRadius*pPortal->m_fViewDistRatio*GetCVars()->e_ViewDistRatioPortals/60.f)
					continue;

				// test occlusion
				if(GetObjManager()->IsBoxOccluded( pPortal->m_boxStatics, pPortal->m_fDistance, &pPortal->m_OcclState, false, eoot_PORTAL))
					continue;

				lstActiveEntransePortals.Add(pPortal);

//				if(GetCVars()->e_Portals==3)
	//				DrawBBox(pPortal->m_boxStatics.min, pPortal->m_boxStatics.max);
			}
		}
	}

	// sort by distance
	if(lstActiveEntransePortals.Count())
	{
		qsort(&lstActiveEntransePortals[0], lstActiveEntransePortals.Count(), 
			sizeof(lstActiveEntransePortals[0]), CVisAreaManager__CmpDistToPortal);
//		m_pCurPortal = lstActiveEntransePortals[0];
	}
}

void CVisAreaManager::MergeCameras(CCamera & cam1, const CCamera & cam2)
{	
	assert(0); // under development
/*	{
		float fDotLR1 =  cam1.GetFrustumPlane(FR_PLANE_LEFT )->n.Dot(cam1.GetFrustumPlane(FR_PLANE_RIGHT)->n);
		float fDotRL2 =  cam2.GetFrustumPlane(FR_PLANE_RIGHT)->n.Dot(cam2.GetFrustumPlane(FR_PLANE_LEFT)->n);
		int y=0;
	}
*/
  // left-right
  float fDotLR =  cam1.GetFrustumPlane(FR_PLANE_LEFT )->n.Dot(cam2.GetFrustumPlane(FR_PLANE_RIGHT)->n);
  float fDotRL =  cam1.GetFrustumPlane(FR_PLANE_RIGHT)->n.Dot(cam2.GetFrustumPlane(FR_PLANE_LEFT)->n);
  if(fabs(fDotLR)<fabs(fDotRL))
	{
    cam1.SetFrustumPlane(FR_PLANE_RIGHT, *cam2.GetFrustumPlane(FR_PLANE_RIGHT));
		cam1.SetPPVertex(2,cam2.GetPPVertex(2));
		cam1.SetPPVertex(3,cam2.GetPPVertex(3));
	}
  else
	{
    cam1.SetFrustumPlane(FR_PLANE_LEFT,  *cam2.GetFrustumPlane(FR_PLANE_LEFT));
		cam1.SetPPVertex(0,cam2.GetPPVertex(0));
		cam1.SetPPVertex(1,cam2.GetPPVertex(1));
	}

	/*
	{
		float fDotLR1 =  cam1.GetFrustumPlane(FR_PLANE_LEFT )->n.Dot(cam1.GetFrustumPlane(FR_PLANE_RIGHT)->n);
		float fDotRL2 =  cam2.GetFrustumPlane(FR_PLANE_RIGHT)->n.Dot(cam2.GetFrustumPlane(FR_PLANE_LEFT)->n);
		int y=0;
	}*/
/*
  // top-bottom
  float fDotTB =  cam1.GetFrustumPlane(FR_PLANE_TOP   )->n.Dot(cam2.GetFrustumPlane(FR_PLANE_BOTTOM)->n);
  float fDotBT =  cam1.GetFrustumPlane(FR_PLANE_BOTTOM)->n.Dot(cam2.GetFrustumPlane(FR_PLANE_TOP   )->n);
  
  if(fDotTB>fDotBT)
    cam1.SetFrustumPlane(FR_PLANE_BOTTOM, *cam2.GetFrustumPlane(FR_PLANE_BOTTOM));
  else
    cam1.SetFrustumPlane(FR_PLANE_TOP,    *cam2.GetFrustumPlane(FR_PLANE_TOP));
*/
  cam1.SetFrustumPlane(FR_PLANE_NEAR, *GetCamera().GetFrustumPlane(FR_PLANE_NEAR));
  cam1.SetFrustumPlane(FR_PLANE_FAR,  *GetCamera().GetFrustumPlane(FR_PLANE_FAR));

	cam1.SetFrustumPlane(FR_PLANE_TOP, *GetCamera().GetFrustumPlane(FR_PLANE_TOP));
	cam1.SetFrustumPlane(FR_PLANE_BOTTOM,  *GetCamera().GetFrustumPlane(FR_PLANE_BOTTOM));

  //cam1.UpdateFrustum();
/*
	if(GetCVars()->e_Portals==4)
	{
		GetRenderer()->SetMaterialColor(1,1,1,1);
		DrawBBox(pVerts[0],pVerts[1],DPRIM_LINE);
		GetRenderer()->DrawLabel(pVerts[0],2,"0");
		DrawBBox(pVerts[1],pVerts[2],DPRIM_LINE);
		GetRenderer()->DrawLabel(pVerts[1],2,"1");
		DrawBBox(pVerts[2],pVerts[3],DPRIM_LINE);
		GetRenderer()->DrawLabel(pVerts[2],2,"2");
		DrawBBox(pVerts[3],pVerts[0],DPRIM_LINE);
		GetRenderer()->DrawLabel(pVerts[3],2,"3");
	}*/
}

void CVisAreaManager::DrawOcclusionAreasIntoCBuffer(CCullBuffer * pCBuffer)
{
	FUNCTION_PROFILER_3DENGINE;
	
	m_lstActiveOcclVolumes.Clear();
	m_lstIndoorActiveOcclVolumes.Clear();

	float fZoomFactor = 0.2f+0.8f*(RAD2DEG(GetCamera().GetFov())/90.f);      
	float fDistRatio = GetCVars()->e_OcclusionVolumesViewDistRatio/fZoomFactor;

	if(GetCVars()->e_OcclusionVolumes)
  for(int i=0; i<m_lstOcclAreas.Count(); i++)
	{
		CVisArea * pArea = m_lstOcclAreas[i];
		if(GetCamera().IsAABBVisible_E( pArea->m_boxArea ))
		{
			float fRadius = (pArea->m_boxArea.min-pArea->m_boxArea.max).GetLength();
			Vec3 vPos = (pArea->m_boxArea.min+pArea->m_boxArea.max)*0.5f;
			float fDist = GetCamera().GetPosition().GetDistance(vPos);
			if(fDist<fRadius*pArea->m_fViewDistRatio*fDistRatio && pArea->m_lstShapePoints.Count()>=2)
			{
				//				pArea->DrawAreaBoundsIntoCBuffer(pCBuffer);
				if(!pArea->m_arrOcclCamera[m_nRenderStackLevel])
					pArea->m_arrOcclCamera[m_nRenderStackLevel] = new CCamera;
				*pArea->m_arrOcclCamera[m_nRenderStackLevel] = GetCamera();

				int nShift = pArea->m_lstShapePoints.Count()>2 &&
											(pArea->m_lstShapePoints[0].GetSquaredDistance(pArea->m_lstShapePoints[1])
											<pArea->m_lstShapePoints[1].GetSquaredDistance(pArea->m_lstShapePoints[2]));
				pArea->m_arrvActiveVerts[0] = pArea->m_lstShapePoints[0+nShift];
				pArea->m_arrvActiveVerts[1] = pArea->m_lstShapePoints[0+nShift]+Vec3(0,0,pArea->m_fHeight);
				pArea->m_arrvActiveVerts[2] = pArea->m_lstShapePoints[1+nShift]+Vec3(0,0,pArea->m_fHeight);
				pArea->m_arrvActiveVerts[3] = pArea->m_lstShapePoints[1+nShift];

				Plane plane;
				plane.SetPlane(pArea->m_arrvActiveVerts[0], pArea->m_arrvActiveVerts[2], pArea->m_arrvActiveVerts[1]);

				if(plane.DistFromPlane(GetCamera().GetPosition())<0)
				{
						pArea->m_arrvActiveVerts[3] = pArea->m_lstShapePoints[0+nShift];
						pArea->m_arrvActiveVerts[2] = pArea->m_lstShapePoints[0+nShift]+Vec3(0,0,pArea->m_fHeight);
						pArea->m_arrvActiveVerts[1] = pArea->m_lstShapePoints[1+nShift]+Vec3(0,0,pArea->m_fHeight);
						pArea->m_arrvActiveVerts[0] = pArea->m_lstShapePoints[1+nShift];
				}
				else if(!pArea->m_bDoubleSide)
						continue;

//				GetRenderer()->SetMaterialColor(1,0,0,1);
				pArea->UpdatePortalCameraPlanes(*pArea->m_arrOcclCamera[m_nRenderStackLevel], pArea->m_arrvActiveVerts, false);

				// make far plane never clip anything
				Plane newFarPlane;
				newFarPlane.SetPlane(Vec3(0,1,-1024), Vec3(1,0,-1024), Vec3(0,0,-1024));
				pArea->m_arrOcclCamera[m_nRenderStackLevel]->SetFrustumPlane(FR_PLANE_FAR, newFarPlane);
        //pArea->m_arrOcclCamera[m_nRenderStackLevel]->UpdateFrustum();

				m_lstActiveOcclVolumes.Add(pArea);
				pArea->m_fDistance = fDist;
			}
		}
	}

	if(m_lstActiveOcclVolumes.Count())
	{ // sort occluders by distance to the camera
		qsort(&m_lstActiveOcclVolumes[0], m_lstActiveOcclVolumes.Count(), 
			sizeof(m_lstActiveOcclVolumes[0]), CVisAreaManager__CmpDistToPortal);
	
		// remove occluded occluders
		for(int i=m_lstActiveOcclVolumes.Count()-1; i>=0; i--)
		{
			CVisArea * pArea = m_lstActiveOcclVolumes[i];
			AABB extrudedBox = pArea->m_boxStatics;
			extrudedBox.min -= Vec3(VEC_EPSILON,VEC_EPSILON,VEC_EPSILON);
			extrudedBox.max += Vec3(VEC_EPSILON,VEC_EPSILON,VEC_EPSILON);
			if(IsOccludedByOcclVolumes(extrudedBox))
				m_lstActiveOcclVolumes.Delete(i);
		}

		// put indoor occluders into separate list
		for(int i=m_lstActiveOcclVolumes.Count()-1; i>=0; i--)
		{
			CVisArea * pArea = m_lstActiveOcclVolumes[i];
			if(pArea->m_bUseInIndoors)
				m_lstIndoorActiveOcclVolumes.Add(pArea);
		}

		if(GetCVars()->e_Portals==4)
		{	// show really active occluders
			for(int i=0; i<m_lstActiveOcclVolumes.Count(); i++)
			{
				CVisArea * pArea = m_lstActiveOcclVolumes[i];
				GetRenderer()->SetMaterialColor(0,1,0,1);
				DrawBBox(pArea->m_boxStatics.min,pArea->m_boxStatics.max);
			}
		}
	}
}

bool CVisAreaManager::IsOccludedByOcclVolumes(const AABB& objBox, bool bCheckOnlyIndoorVolumes)
{
	FUNCTION_PROFILER_3DENGINE;

	PodArray<CVisArea*> & rList = bCheckOnlyIndoorVolumes ? m_lstIndoorActiveOcclVolumes : m_lstActiveOcclVolumes;

	for(int i=0; i<rList.Count(); i++)
	{
		bool bAllIn=0;
    if(rList[i]->m_arrOcclCamera[m_nRenderStackLevel])
		  if(rList[i]->m_arrOcclCamera[m_nRenderStackLevel]->IsAABBVisible_EH(objBox, &bAllIn))
        if(bAllIn)
			    return true;
	}
	
	return false;
}
/*
void CVisAreaManager::CompileObjects()
{
  // areas
  for(int v=0; v<m_lstVisAreas.Count(); v++)
    m_lstVisAreas[v]->CompileObjects(STATIC_OBJECTS);

  // portals
  for(int v=0; v<m_lstPortals.Count(); v++)
    m_lstPortals[v]->CompileObjects(STATIC_OBJECTS);
}
*/
/*
void CVisAreaManager::CheckUnload()
{
  m_nLoadedSectors=0;

  // areas
  for(int v=0; v<m_lstVisAreas.Count(); v++)
    m_nLoadedSectors += m_lstVisAreas[v]->CheckUnload();

  // portals
  for(int v=0; v<m_lstPortals.Count(); v++)
    m_nLoadedSectors += m_lstPortals[v]->CheckUnload();
}
*/
void CVisAreaManager::GetStreamingStatus(int & nLoadedSectors, int & nTotalSectors)
{
  nLoadedSectors = 0;//m_nLoadedSectors;
  nTotalSectors = m_lstPortals.Count() + m_lstVisAreas.Count();
}

void CVisAreaManager::GetMemoryUsage(ICrySizer*pSizer)
{
  // areas
  for(int v=0; v<m_lstVisAreas.Count(); v++)
    m_lstVisAreas[v]->GetMemoryUsage(pSizer);

  // portals
  for(int v=0; v<m_lstPortals.Count(); v++)
    m_lstPortals[v]->GetMemoryUsage(pSizer);

	// occl areas
	for(int v=0; v<m_lstOcclAreas.Count(); v++)
		m_lstOcclAreas[v]->GetMemoryUsage(pSizer);

  pSizer->AddObject(this,sizeof(*this));
}


void CVisAreaManager::PrecacheLevel(bool bPrecacheAllVisAreas, Vec3 * pPrecachePoints, int nPrecachePointsNum)
{
	CryLog("Precaching the level ...");
//  gEnv->pLog->UpdateLoadingScreen(0);

	float fPrecacheTimeStart = GetTimer()->GetAsyncCurTime();

	GetRenderer()->EnableSwapBuffers((GetCVars()->e_PrecacheLevel>=2) ?	true : false);

	uint32 dwPrecacheLocations=0;

	Vec3 arrCamDir[6] =
	{
		Vec3(  1, 0, 0),  
		Vec3( -1, 0, 0 ),  
		Vec3(  0, 1, 0 ),  
		Vec3(  0,-1, 0 ),  
		Vec3(  0, 0, 1 ),  
		Vec3(  0, 0,-1 )  
	};

	//loop over all sectors and place a light in the middle of the sector  
	for(int v=0; v<m_lstVisAreas.Count() && bPrecacheAllVisAreas; v++)
	{
		unsigned short * pPtr2FrameID = (unsigned short *)GetRenderer()->EF_Query(EFQ_Pointer2FrameID);
		if(pPtr2FrameID)
			(*pPtr2FrameID)++;

		m_nRenderFrameID = GetRenderer()->GetFrameID();

		++dwPrecacheLocations;

		// find real geometry bbox
/*		bool bGeomFound = false;
		Vec3 vBoxMin(100000.f,100000.f,100000.f);
		Vec3 vBoxMax(-100000.f,-100000.f,-100000.f);
		for(int s=0; s<ENTITY_LISTS_NUM; s++)
		for(int e=0; e<m_lstVisAreas[v]->m_lstEntities[s].Count(); e++)
		{
			AABB aabbBox = m_lstVisAreas[v]->m_lstEntities[s][e].aabbBox;
			vBoxMin.CheckMin(aabbBox.min);
			vBoxMax.CheckMax(aabbBox.max);
			bGeomFound = true;
		}*/

		Vec3 vAreaCenter = m_lstVisAreas[v]->m_boxArea.GetCenter();
		CryLog("  Precaching VisArea %s", m_lstVisAreas[v]->m_sName );

		//place camera in the middle of a sector and render sector from different directions
		for(int i=0; i<6 /*&& bGeomFound*/; i++)
		{
			GetRenderer()->BeginFrame();
	
			// setup camera 
			CCamera cam = GetCamera();
			Matrix33 mat = Matrix33::CreateRotationVDir( arrCamDir[i], 0 );
			cam.SetMatrix(mat);
			cam.SetPosition(vAreaCenter);
			cam.SetFrustum(GetRenderer()->GetWidth(), GetRenderer()->GetHeight(), gf_PI/2, cam.GetNearPlane(), cam.GetFarPlane());
//			Get3DEngine()->SetupCamera(cam);

			Get3DEngine()->RenderWorld(SHDF_ZPASS | SHDF_ALLOWHDR | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOW_WATER | SHDF_ALLOW_AO, &cam, 1, "PrecacheVisAreas", -1, -1);

			GetRenderer()->RenderDebug();
			GetRenderer()->EndFrame();
			
			if(GetCVars()->e_PrecacheLevel>=2)
				CrySleep(200);
		}
	}

	CryLog("Precached %d visarea sectors", dwPrecacheLocations);


//--------------------------------------------------------------------------------------
//----                  PRE-FETCHING OF RENDER-DATA IN OUTDOORS                     ----
//--------------------------------------------------------------------------------------
	
	// loop over all cam-position in the level and render this part of the level from 6 different directions
	for(int p=0; pPrecachePoints && p<nPrecachePointsNum; p++) //loop over outdoor-camera position
	{
		CryLog("  Precaching PrecacheCamera point %d of %d", p,nPrecachePointsNum );
		for(int i=0; i<6; i++) //loop over 6 camera orientations
		{
			GetRenderer()->BeginFrame();

			// setup camera 
			CCamera cam = GetCamera();
			Matrix33 mat = Matrix33::CreateRotationVDir( arrCamDir[i], 0 );
			cam.SetMatrix(mat);
			cam.SetPosition(pPrecachePoints[p]);
			cam.SetFrustum(GetRenderer()->GetWidth(), GetRenderer()->GetHeight(), gf_PI/2, cam.GetNearPlane(), cam.GetFarPlane());

			Get3DEngine()->RenderWorld(SHDF_ZPASS | SHDF_ALLOWHDR | SHDF_ALLOWPOSTPROCESS | SHDF_ALLOW_WATER | SHDF_ALLOW_AO, &cam, 1, "PrecacheOutdoor", -1, -1);

			GetRenderer()->RenderDebug();
			GetRenderer()->EndFrame();

			if(GetCVars()->e_PrecacheLevel>=2)
				CrySleep(1000);
		}
	}

	CryLog("Precached %d PrecacheCameraXX points", nPrecachePointsNum);

	GetRenderer()->EnableSwapBuffers(true);

	float fPrecacheTime = GetTimer()->GetAsyncCurTime() - fPrecacheTimeStart;
	CryLog("Level Precache finished in %.2f seconds", fPrecacheTime );
}

void CVisAreaManager::GetObjectsAround(Vec3 vExploPos, float fExploRadius, PodArray<SRNInfo> * pEntList, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects)
{
	AABB aabbBox(vExploPos - Vec3(fExploRadius,fExploRadius,fExploRadius), vExploPos + Vec3(fExploRadius,fExploRadius,fExploRadius));

	CVisArea * pVisArea = (CVisArea *)GetVisAreaFromPos(vExploPos);

	if(pVisArea && pVisArea->m_pObjectsTree)
		pVisArea->m_pObjectsTree->MoveObjectsIntoList(pEntList, &aabbBox, false, true, bSkip_ERF_NO_DECALNODE_DECALS, bSkipDynamicObjects);
/*
	// find static objects around
	for(int i=0; pVisArea && i<pVisArea->m_lstEntities[STATIC_OBJECTS].Count(); i++)
	{
		IRenderNode * pRenderNode =	pVisArea->m_lstEntities[STATIC_OBJECTS][i].pNode;
		if(bSkip_ERF_NO_DECALNODE_DECALS && pRenderNode->GetRndFlags()&ERF_NO_DECALNODE_DECALS)
			continue;

		if(pRenderNode->GetRenderNodeType() == eERType_Decal)
			continue;

		if(Overlap::Sphere_AABB(Sphere(vExploPos,fExploRadius), pRenderNode->GetBBox()))
			if(pEntList->Find(pRenderNode)<0)
				pEntList->Add(pRenderNode);
	}*/
}

void CVisAreaManager::IntersectWithBox(const AABB & aabbBox, PodArray<CVisArea*> * plstResult, bool bOnlyIfVisible)
{
	for(int p=0; p<m_lstPortals.Count(); p++)
	{
    if( m_lstPortals[p]->m_boxArea.min.x < aabbBox.max.x && m_lstPortals[p]->m_boxArea.max.x > aabbBox.min.x &&
        m_lstPortals[p]->m_boxArea.min.y < aabbBox.max.y && m_lstPortals[p]->m_boxArea.max.y > aabbBox.min.y )
    {
			plstResult->Add(m_lstPortals[p]);
    }
	}

	for(int v=0; v<m_lstVisAreas.Count(); v++)
	{
    if( m_lstVisAreas[v]->m_boxArea.min.x < aabbBox.max.x && m_lstVisAreas[v]->m_boxArea.max.x > aabbBox.min.x &&
        m_lstVisAreas[v]->m_boxArea.min.y < aabbBox.max.y && m_lstVisAreas[v]->m_boxArea.max.y > aabbBox.min.y )
    {
			plstResult->Add(m_lstVisAreas[v]);
    }
	}
}

void CVisAreaManager::DoVoxelShape(Vec3 vWSPos, float fRadius, int nSurfaceTypeId, Vec3 vBaseColor, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget, PodArray<CVoxelObject*> * pAffectedVoxAreas)
{
	// update voxel objects
	if(eTarget == evetVoxelObjects)
	{
		if(eOperation == eveoPaintHeightPos || eOperation == eveoPaintHeightNeg || eOperation == eveoCreate || eOperation == eveoSubstract || eOperation == eveoBlurNeg || eOperation == eveoBlurPos || eOperation == eveoMaterial || eOperation == eveoBaseColor)
		{
			AABB brushBox(vWSPos-Vec3(fRadius,fRadius,fRadius), vWSPos+Vec3(fRadius,fRadius,fRadius));

			PodArray<CVisArea*> lstResult;
      AABB brushBoxForAreas(vWSPos-Vec3(fRadius+4,fRadius+4,fRadius+4), vWSPos+Vec3(fRadius+4,fRadius+4,fRadius+4));
			IntersectWithBox(brushBoxForAreas, &lstResult, false);

			for(int n=0; n<lstResult.Count(); n++)
			{
				CVisArea * pNode = lstResult[n];

				PodArray<SRNInfo> lstObjects;
				if(pNode->m_pObjectsTree)
					pNode->m_pObjectsTree->MoveObjectsIntoList(&lstObjects, &brushBox, false, false, false);

				PodArray<SRNInfo> * pList = &lstObjects;
				for(int i=0; i<pList->Count(); i++)
				{
					SRNInfo * pObj = &pList->GetAt(i);
          if(pObj->pNode->GetRenderNodeType() == eERType_VoxelObject && Overlap::Sphere_AABB(pObj->objSphere, brushBox))
					{
						CVoxelObject * pVox = (CVoxelObject*)pObj->pNode;
						{
							if(pAffectedVoxAreas)
								pAffectedVoxAreas->Add(pVox);
							else
							{
								CVoxelObject * arrNeighbours[3*3*3];
								GetTerrain()->Voxel_FindNeighboursForObject(pVox, arrNeighbours);

								Matrix34 matInv = pVox->GetMatrix();
								matInv.Invert();
								float fInvScale = matInv.GetColumn(0).GetLength();
                assert(pNode->m_pObjectsTree);
								if(pVox->DoVoxelShape(eOperation, matInv.TransformPoint(vWSPos), fRadius*fInvScale, 
                  (nSurfaceTypeId>=0) ? &GetTerrain()->GetSurfaceTypes(pVox->m_nSID)[nSurfaceTypeId] : NULL, vBaseColor,
									eShape, arrNeighbours))
								{
									pVox->m_pVoxelVolume->SubmitVoxelSpace();
									pVox->ScheduleRebuild();
								}
							}
						}
					}
				}
			}
		}
		return;
	}
}

void CVisAreaManager::AddLightSourceReqursive( CDLight * pLight, CVisArea* pArea, const int32 nDeepness ) 
{
	if(pArea->m_pObjectsTree)
		pArea->m_pObjectsTree->AddLightSource(pLight);

	if( 1 < nDeepness )
		for(int v=0; v<pArea->m_lstConnections.Count(); v++)
		{
			CVisArea * pConArea = pArea->m_lstConnections[v];
			AddLightSourceReqursive(pLight,pConArea,nDeepness-1);
		}
}

void CVisAreaManager::AddLightSource(CDLight * pLight) 
{
	CVisArea * pLightArea = (CVisArea * )GetVisAreaFromPos(pLight->m_Origin);

  bool bThisAreaInly = (pLight->m_Flags & DLF_THIS_AREA_ONLY) != 0;

  if(pLightArea)
  {
	  int nPortal = pLightArea->IsPortal() ? 1 : 0;
    AddLightSourceReqursive( pLight, pLightArea, nPortal + (bThisAreaInly ? 2 : 3) );
  }
  else if(!bThisAreaInly)
  {
    AABB lightBox(
      pLight->m_Origin-Vec3(pLight->m_fRadius,pLight->m_fRadius,pLight->m_fRadius),
      pLight->m_Origin+Vec3(pLight->m_fRadius,pLight->m_fRadius,pLight->m_fRadius));
    static PodArray<CVisArea*> arrAreas; arrAreas.Clear();
    m_pVisAreaManager->IntersectWithBox(lightBox, &arrAreas, true);
    for(int i=0; i<arrAreas.Count(); i++)
      if(arrAreas[i]->IsPortal() && arrAreas[i]->IsConnectedToOutdoor())
    {
      pLightArea = arrAreas[i];
      int nPortal = pLightArea->IsPortal() ? 1 : 0;
      AddLightSourceReqursive( pLight, pLightArea, nPortal + (bThisAreaInly ? 2 : 3) );
    }
  }
}

int CVisAreaManager::GetNumberOfVisArea() const
{
	return m_lstPortals.size() + m_lstVisAreas.size();
}

IVisArea* CVisAreaManager::GetVisAreaById( int nID ) const
{
	if( nID < 0 )
		return NULL;

	if( nID < (int)m_lstPortals.size() )
	{
		return m_lstPortals[ nID ];
	}
	nID -= m_lstPortals.size();
	if( nID < (int)m_lstVisAreas.size() )
	{
		return m_lstVisAreas[ nID ];
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CVisAreaManager::AddListener( IVisAreaCallback *pListener )
{
	if (m_lstCallbacks.Find(pListener) < 0)
		m_lstCallbacks.Add(pListener);
}

//////////////////////////////////////////////////////////////////////////
void CVisAreaManager::RemoveListener( IVisAreaCallback *pListener )
{
	m_lstCallbacks.Delete(pListener);
}

void CVisAreaManager::MarkAllSectorsAsUncompiled()
{
	for(int p=0; p<m_lstPortals.Count(); p++)
	{	
		if(m_lstPortals[p]->m_pObjectsTree)
			m_lstPortals[p]->m_pObjectsTree->MarkAsUncompiled();
	}

	for(int v=0; v<m_lstVisAreas.Count(); v++)
	{
		if(m_lstVisAreas[v]->m_pObjectsTree)
			m_lstVisAreas[v]->m_pObjectsTree->MarkAsUncompiled();
	}
}


void CVisAreaManager::GetObjectsByType(PodArray<IRenderNode*> & lstObjects, EERType objType, const AABB * pBBox, float fViewDist )
{
	{
		uint32 dwSize = m_lstVisAreas.Count(); 

		for(uint32 dwI=0; dwI<dwSize; ++dwI)
			if(m_lstVisAreas[dwI]->m_pObjectsTree)
				m_lstVisAreas[dwI]->m_pObjectsTree->GetObjectsByType(lstObjects, objType, pBBox, fViewDist);
	}

	{
		uint32 dwSize = m_lstPortals.Count(); 

		for(uint32 dwI=0; dwI<dwSize; ++dwI)
			if(m_lstPortals[dwI]->m_pObjectsTree)
				m_lstPortals[dwI]->m_pObjectsTree->GetObjectsByType(lstObjects, objType, pBBox, fViewDist);
	}
}

void CVisAreaManager::GetObjects( PodArray<IRenderNode*> & lstObjects, const AABB * pBBox, float fViewDist)
{
	{
		uint32 dwSize = m_lstVisAreas.Count(); 

		for(uint32 dwI=0; dwI<dwSize; ++dwI)
			if(m_lstVisAreas[dwI]->m_pObjectsTree)
				m_lstVisAreas[dwI]->m_pObjectsTree->GetObjects(lstObjects, pBBox, fViewDist);
	}

	{
		uint32 dwSize = m_lstPortals.Count(); 

		for(uint32 dwI=0; dwI<dwSize; ++dwI)
			if(m_lstPortals[dwI]->m_pObjectsTree)
				m_lstPortals[dwI]->m_pObjectsTree->GetObjects(lstObjects, pBBox, fViewDist);
	}
}

void CVisAreaManager::GenerateStatObjAndMatTables(std::vector<IStatObj*> * pStatObjTable, std::vector<IMaterial*> * pMatTable, SHotUpdateInfo * pExportInfo)
{
  {
    uint32 dwSize = m_lstVisAreas.Count(); 
    for(uint32 dwI=0; dwI<dwSize; ++dwI)
      if(m_lstVisAreas[dwI]->m_pObjectsTree)
        m_lstVisAreas[dwI]->m_pObjectsTree->GenerateStatObjAndMatTables(pStatObjTable, pMatTable, pExportInfo);
  }

  {
    uint32 dwSize = m_lstPortals.Count(); 
    for(uint32 dwI=0; dwI<dwSize; ++dwI)
      if(m_lstPortals[dwI]->m_pObjectsTree)
        m_lstPortals[dwI]->m_pObjectsTree->GenerateStatObjAndMatTables(pStatObjTable, pMatTable, pExportInfo);
  }
}

bool CVisAreaManager::IsAABBVisibleFromPoint(AABB & box, Vec3 pos)
{
  CVisArea * pAreaBox = (CVisArea*)GetVisAreaFromPos(box.GetCenter());
  CVisArea * pAreaPos = (CVisArea*)GetVisAreaFromPos(pos);

  if(!pAreaBox && !pAreaPos)
    return true; // no indoors involved

  PodArray<CVisArea*> arrPortals;
  int nRecursion=0;
  Shadowvolume sv;
  NAABB_SV::AABB_ReceiverShadowVolume(pos, box, sv);

  bool bRes = false;

  bRes = FindShortestPathToVisArea(pAreaPos, pAreaBox, arrPortals, nRecursion, sv);
  
  GetRenderer()->DrawLabel(box.GetCenter(), 2, "-%s-", bRes ? "Y" : "N");
  GetRenderer()->DrawLabel(pos, 2, "-X-");
  DrawLine(pos, box.GetCenter());
  DrawBBox(box, bRes ? Col_White : Col_NavyBlue);

  return bRes;
}

bool CVisAreaManager::FindShortestPathToVisArea(CVisArea * pThisArea, CVisArea * pTargetArea, PodArray<CVisArea*> & arrVisitedAreas, int & nRecursion, const Shadowvolume & sv)
{
  // skip double processing
  if(arrVisitedAreas.Find(pThisArea)>=0)
    return false;

  // check if point to box frustum intersects pThisArea visarea
  if(pThisArea && !NAABB_SV::Is_AABB_In_ShadowVolume(sv, *pThisArea->GetAABBox()))
    return false;

  // check if box visarea reached
  if(pThisArea==pTargetArea)
    return true;

  // register as already processed
  arrVisitedAreas.Add(pThisArea);

  // recurse to connections
  if(pThisArea)
  {
    for(int p=0; p<pThisArea->m_lstConnections.Count(); p++)
      if(FindShortestPathToVisArea(pThisArea->m_lstConnections[p], pTargetArea, arrVisitedAreas, nRecursion, sv))
        return true;

    if(pThisArea->IsPortal() && pThisArea->m_lstConnections.Count()==1 && !pThisArea->m_bSkyOnly)
      if(FindShortestPathToVisArea(NULL, pTargetArea, arrVisitedAreas, nRecursion, sv))
        return true;
  }
  else
  {
    for(int p=0; p<m_lstPortals.Count(); p++)
      if(m_lstPortals[p]->IsPortal() && m_lstPortals[p]->m_lstConnections.Count()==1 && !m_lstPortals[p]->m_bSkyOnly)
        if(FindShortestPathToVisArea(m_lstPortals[p], pTargetArea, arrVisitedAreas, nRecursion, sv))
          return true;
  }

  return false;
}
