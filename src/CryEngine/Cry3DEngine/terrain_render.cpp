////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_render.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: draw vis sectors
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "terrain.h"
#include "terrain_sector.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "CullBuffer.h"

int __cdecl CTerrain__Cmp_Sectors(const void* v1, const void* v2)
{
	CTerrainNode * p1 = *((CTerrainNode**)v1);
	CTerrainNode * p2 = *((CTerrainNode**)v2);
/*
	// sort by how far current sector texture lod from optimal lod
	int nTexLodErr1 = -p1->m_cNodeNewTexMML;
	while(!p1->m_nNodeTexSet.nTex0 && p1->m_pParent)
	{ p1 = p1->m_pParent; nTexLodErr1++; }

	int nTexLodErr2 = -p2->m_cNodeNewTexMML;
	while(!p2->m_nNodeTexSet.nTex0 && p2->m_pParent)
	{ p2 = p2->m_pParent; nTexLodErr2++; }

	if(nTexLodErr1 > nTexLodErr2)
		return -1;
	else if(nTexLodErr1 < nTexLodErr2)
		return 1;
*/
	// if same - give closest sectors higher priority
	if(p1->m_arrfDistance[p1->m_nRenderStackLevel] > p2->m_arrfDistance[p1->m_nRenderStackLevel])
		return 1;
	else if(p1->m_arrfDistance[p1->m_nRenderStackLevel] < p2->m_arrfDistance[p1->m_nRenderStackLevel])
		return -1;

	return 0;
}

float GetPointToBoxDistance(Vec3 vPos, AABB bbox);

int CTerrain::GetDetailTextureMaterials(IMaterial* materials[], int nSID)
{
	int materialNumber = 0;
	//vector<IMaterial*> materials;

  uchar szProj[] = "XYZ";

  for(int s=0; s<MAX_SURFACE_TYPES_COUNT; s++)
  {
    SSurfaceType * pSurf = &m_SSurfaceType[nSID][s];

    if(pSurf->HasMaterial())
    {
      for(int p=0; p<3; p++)
      {
        if(IMaterial * pMat = pSurf->GetMaterialOfProjection(szProj[p]))
        {
					if( materials )
					{
						materials[materialNumber] = pMat;
					}
					materialNumber++;
        }
      }
    }
  }
	
	return materialNumber;
}

void CTerrain::DrawVisibleSectors()
{
  FUNCTION_PROFILER_3DENGINE;

  if(!GetCVars()->e_Terrain || !Get3DEngine()->m_bShowTerrainSurface || (GetCVars()->e_VoxTer && GetCVars()->e_Voxel))
    return;

	// sort to get good texture streaming priority
	if(m_lstVisSectors.Count())
		qsort(m_lstVisSectors.GetElements(), m_lstVisSectors.Count(), sizeof(m_lstVisSectors[0]), CTerrain__Cmp_Sectors);

  for(int i=0; i<m_lstVisSectors.Count(); i++)
  {
    CTerrainNode * pNode = (CTerrainNode *)m_lstVisSectors[i];
		
    pNode->RenderNodeHeightmap();

    if(!pNode->GetLeafData())
      continue;

		// pre-cache surface types
		for(int s=0; s<MAX_SURFACE_TYPES_COUNT; s++)
		{
			SSurfaceType * pSurf = &m_SSurfaceType[pNode->m_nSID][s];

  	  if(pSurf->HasMaterial())
  	  {
    	  uint8 szProj[] = "XYZ";

				for(int p=0; p<3; p++)
				{
					if(IMaterial * pMat = pSurf->GetMaterialOfProjection(szProj[p]))
					{
						Get3DEngine()->m_pObjManager->PrecacheMaterial( pMat, pNode->GetDistance(), pNode->GetLeafData()->m_pRenderMesh );
					}
				}
			}
		}
	}
}

void CTerrain::RenderAOSectors(int nSID)
{
  if(m_nRenderStackLevel)
    return;

  FUNCTION_PROFILER_3DENGINE;

  static PodArray<SSectorTextureSet> AOTexNodes; AOTexNodes.Clear();

  if(GetCVars()->e_TerrainAo && (m_arrBaseTexInfos[nSID].m_hdrDiffTexInfo.dwFlags == TTFHF_AO_DATA_IS_VALID))
  {
    float fRange = 1024;

    Vec3 vCamPos = GetCamera().GetPosition();
    AABB areaBox(vCamPos - Vec3(fRange,fRange,fRange), vCamPos + Vec3(fRange,fRange,fRange));

    static PodArray<CTerrainNode*> AONodes; AONodes.Clear();
    GetParentNode(nSID)->GetTerrainAOTextureNodesInBox(areaBox, &AONodes);

//    AONodes.Clear();
  //  AONodes.AddList(m_lstVisSectors);

    for(int i=0; i<AONodes.Count(); i++)
    {
      if(AONodes[i]->m_arrfDistance[m_nRenderStackLevel]<fRange)
      {
        CTerrainNode * pTexNode = AONodes[i]->GetReadyTexSourceNode(0,ett_LM);
        if(!pTexNode || !pTexNode->m_nNodeTexSet.nTex0)
          continue;

        if(GetCVars()->e_TerrainDrawThisSectorOnly)
        {
          AABB boxWS = AONodes[i]->GetBBox();
          if(
            GetCamera().GetPosition().x > boxWS.max.x || 
            GetCamera().GetPosition().x < boxWS.min.x ||
            GetCamera().GetPosition().y > boxWS.max.y || 
            GetCamera().GetPosition().y < boxWS.min.y )
            continue;
        }

        AABB stencilBox;
        stencilBox = AONodes[i]->GetBBoxVirtual();
        stencilBox.min.z -= 08.f;
        stencilBox.max.z += 32.f;

        if(GetCamera().IsAABBVisible_E(stencilBox))
        {
          AOTexNodes.Add(pTexNode->m_nNodeTexSet);
          AOTexNodes.Last().stencilBox = stencilBox;
          AOTexNodes.Last().fTexScale = pTexNode->m_nNodeTexSet.fTexScale;

          if(GetCVars()->e_TerrainAo>1)
          {
            AABB aabb = AONodes[i]->GetBBoxVirtual();
            GetRenderer()->DrawLabel(aabb.GetCenter(), 2, "%.1f-%.1f", aabb.min.z, aabb.max.z);
            DrawBBox(AONodes[i]->GetBBoxVirtual());
          }
        }
      }
    }
  }

  GetRenderer()->SetTerrainAONodes(&AOTexNodes);
}