////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_light.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: generate geometry for hmap light pass
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "terrain_sector.h"
#include "RoadRenderNode.h"

IRenderMesh * CTerrain::MakeAreaRenderMesh(	const Vec3 & vPos, float fRadius, 
																						IMaterial * pMat, const char * szLSourceName,
                                            Plane * planes)
{
	static PodArray<unsigned short> lstIndices; lstIndices.Clear();
  static PodArray<Vec3> posBuffer; posBuffer.Clear();
	static PodArray<SVF_P3S_C4B_T2S> vertBuffer; vertBuffer.Clear();
	static PodArray<SPipTangents> tangBasises; tangBasises.Clear();

	int nUnitSize = GetTerrain()->GetHeightMapUnitSize();

	Vec3i vBoxMin = vPos-Vec3(fRadius,fRadius,fRadius);
	Vec3i vBoxMax = vPos+Vec3(fRadius,fRadius,fRadius);
	
	vBoxMin.x = vBoxMin.x/nUnitSize*nUnitSize;
	vBoxMin.y = vBoxMin.y/nUnitSize*nUnitSize;
	vBoxMin.z = vBoxMin.z/nUnitSize*nUnitSize;

	vBoxMax.x = vBoxMax.x/nUnitSize*nUnitSize+nUnitSize;
	vBoxMax.y = vBoxMax.y/nUnitSize*nUnitSize+nUnitSize;
	vBoxMax.z = vBoxMax.z/nUnitSize*nUnitSize+nUnitSize;

  for(int x=vBoxMin.x; x<=vBoxMax.x; x+=nUnitSize)
  {
    for(int y=vBoxMin.y; y<=vBoxMax.y; y+=nUnitSize)
    {
      Vec3 vTmp = Vec3((float)x,(float)y,(float)(GetTerrain()->GetZ(x,y,GetDefSID())));
      posBuffer.Add(vTmp);
    }
  }

	int nSizeX = (vBoxMax.x - vBoxMin.x)/nUnitSize;
	int nSizeY = (vBoxMax.y - vBoxMin.y)/nUnitSize;

	for(int x=0; x<nSizeX; x++)
	{
		for(int y=0; y<nSizeY; y++)
		{
			uint16 id0 = (x+0)*(nSizeY+1) + (y+0);
			uint16 id1 = (x+1)*(nSizeY+1) + (y+0);
			uint16 id2 = (x+0)*(nSizeY+1) + (y+1);
			uint16 id3 = (x+1)*(nSizeY+1) + (y+1);

			assert(id3<posBuffer.Count());

			lstIndices.Add(id0);
			lstIndices.Add(id1);
			lstIndices.Add(id2);

			lstIndices.Add(id2);
			lstIndices.Add(id1);
			lstIndices.Add(id3);
		}
	}

  // clip triangles
  if(planes)
  {
    int nOrigCount = lstIndices.Count();
    for(int i=0; i<nOrigCount; i+=3)
    {
      if(CRoadRenderNode::ClipTriangle(posBuffer, lstIndices, i, planes))
      {
        i-=3;
        nOrigCount-=3;
      }
    }
  }

  AABB bbox; bbox.Reset();
  for(int i=0; i<lstIndices.Count(); i++)
    bbox.Add(posBuffer[lstIndices[i]]);

  for(int i=0; i<posBuffer.Count(); i++)
  {
    SVF_P3S_C4B_T2S vTmp;
    vTmp.xyz = posBuffer[i];
    vTmp.color.dcolor = uint32(-1);
    vTmp.st = Vec2(vTmp.xyz.x, vTmp.xyz.y);
    vertBuffer.Add(vTmp);

    SPipTangents basis;

    Vec3 vBinorm = Vec3(-1,0,0); 
    vBinorm.Normalize();

    Vec3 vTang = Vec3(0,1,0); 
    vTang.Normalize();

    Vec3 vNormal = GetTerrain()->GetTerrainSurfaceNormal(vTmp.xyz.ToVec3(), 0.25f);
    vBinorm = -vNormal.Cross(vTang);
    vTang = vNormal.Cross(vBinorm);

    basis.Binormal = Vec4sf(tPackF2B(vBinorm.x),tPackF2B(vBinorm.y),tPackF2B(vBinorm.z), tPackF2B(-1));
    basis.Tangent  = Vec4sf(tPackF2B(vTang.x),tPackF2B(vTang.y),tPackF2B(vTang.z), tPackF2B(-1));

    tangBasises.Add(basis);
  }

	IRenderMesh * pMesh = GetRenderer()->CreateRenderMeshInitialized(
		vertBuffer.GetElements(), vertBuffer.Count(), eVF_P3S_C4B_T2S,
		lstIndices.GetElements(), lstIndices.Count(), R_PRIMV_TRIANGLES, 
		szLSourceName, szLSourceName, eRMT_Static, 1, 0, NULL, NULL, false, true, tangBasises.GetElements());

	float texelAreaDensity = GetRenderer()->CalculateTexelAreaDensity(vertBuffer,lstIndices, szLSourceName);

	pMesh->SetChunk(pMat, 0, vertBuffer.Count(), 0, lstIndices.Count(), texelAreaDensity);
	pMesh->SetMaterial(pMat);
  pMesh->SetBBox(bbox.min,bbox.max);

	return pMesh;
}

bool CTerrain::RenderArea(Vec3 vPos, float fRadius, IRenderMesh ** ppRenderMesh, 
  CRenderObject * pObj, IMaterial * pMaterial, const char * szComment, float * pCustomData,
  Plane * planes)
{                                                            
  if(m_nRenderStackLevel)
    return false;

	FUNCTION_PROFILER_3DENGINE;

  bool bREAdded = false;

	if(!ppRenderMesh[0])
		ppRenderMesh[0] = MakeAreaRenderMesh(vPos, fRadius, pMaterial, szComment, planes);

	if(ppRenderMesh[0] && ppRenderMesh[0]->GetIndicesCount())
	{
		ppRenderMesh[0]->SetMaterial(pMaterial);

	  ppRenderMesh[0]->SetREUserData(pCustomData);
		ppRenderMesh[0]->AddRE(pObj, 0, EFSLIST_GENERAL, 1);
		bREAdded = true;
	}

  return bREAdded;
}
