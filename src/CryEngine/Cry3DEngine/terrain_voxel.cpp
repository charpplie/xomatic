////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_voxel.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: voxel
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "terrain_sector.h"
#include "terrain.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "VoxMan.h"
#include "3dEngine.h"
#include "terrain_water.h"
#include "RoadRenderNode.h"
#include "VoxTerrain.h"

void CTerrain::DoVoxelShape(Vec3 vWSPos, float fRadius, int nSurfaceTypeId, Vec3 vBaseColor, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, EVoxelEditTarget eTarget, PodArray<CVoxelObject*> * pAffectedVoxAreas)
{
	FUNCTION_PROFILER_3DENGINE;

//	eTarget = evetVoxelTerrain;

	//GetISystem()->VTuneResume();

	// update voxel objects
	if(eTarget == evetVoxelObjects)
	{
		if(eOperation == eveoPaintHeightPos || eOperation == eveoPaintHeightNeg || eOperation == eveoCreate || eOperation == eveoSubstract || eOperation == eveoBlurNeg || eOperation == eveoBlurPos || eOperation == eveoMaterial || eOperation == eveoBaseColor || eOperation == eveoCopyTerrainPos || eOperation == eveoCopyTerrainNeg)
		{
			AABB brushBox(vWSPos-Vec3(fRadius,fRadius,fRadius), vWSPos+Vec3(fRadius,fRadius,fRadius));

//			PodArray<CTerrainNode*> lstResult;
	//		IntersectWithBox(brushBox, &lstResult, false);

//			for(int n=0; n<lstResult.Count(); n++)
      for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
			{
	//			CTerrainNode * pNode = lstResult[n];
				PodArray<SRNInfo> lstObjects;// = &pNode->m_lstEntities[STATIC_OBJECTS];
        if(Get3DEngine()->m_pObjectsTree[nSID])
  				Get3DEngine()->m_pObjectsTree[nSID]->MoveObjectsIntoList(&lstObjects, &brushBox, false, false, false);
				for(int i=0; i<lstObjects.Count(); i++)
				{
					SRNInfo * pObj = lstObjects.Get(i);
					if(pObj->pNode->GetRenderNodeType() == eERType_VoxelObject && Overlap::Sphere_AABB(pObj->objSphere, brushBox) && !(pObj->pNode->GetRndFlags()&ERF_HIDDEN))
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
								if(pVox->DoVoxelShape(eOperation, matInv.TransformPoint(vWSPos), fRadius*fInvScale, 
									(nSurfaceTypeId>=0) ? &m_SSurfaceType[pVox->m_nSID][nSurfaceTypeId] : NULL, vBaseColor,
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

	//GetISystem()->VTunePause();
}

void CTerrain::BuildVoxelSpace()
{
/*	if(!GetCVars()->e_VoxelGenerate)
		return;

	PrintMessage("Processing command");

	if(GetCVars()->e_VoxelGenerate==-1)
	{ // clear
		Vec3 vCenter((float)GetTerrainSize()/2,(float)GetTerrainSize()/2,0);
		vCenter.z = GetZApr(vCenter.x,vCenter.y) + 32 + 64;
		float fSize = (float)GetTerrainSize();
		Get3DEngine()->Voxel_Paint(vCenter, fSize, 9, Vec3(0.5f,0.5f,0.5f), eveoSubstract, evbsSphere, evetVoxelObjects, NULL, 4.f);
	}
	else if(GetCVars()->e_VoxelGenerate==1)
	{ // create single
		Vec3 vCenter((float)GetTerrainSize()/2,(float)GetTerrainSize()/2,0);
		vCenter.z = GetZApr(vCenter.x,vCenter.y) + 32 + 64;
		float fSize = 32;
		Get3DEngine()->Voxel_Paint(vCenter, fSize, 9, Vec3(0.5f,0.5f,0.5f), eveoCreate, evbsSphere, evetVoxelObjects, NULL, 4.f);
	}
	else if(GetCVars()->e_VoxelGenerate>0)
	{ // generate a lot
		srand(GetCVars()->e_VoxelGenerate);
		for(int i=0; i<GetCVars()->e_VoxelGenerate; i++)
		{
			Vec3 vPos(	
				GetTerrainSize()/4+rnd()*GetTerrainSize()/2,
				GetTerrainSize()/4+rnd()*GetTerrainSize()/2,
				rnd()*(64+64));

			Get3DEngine()->Voxel_Paint(vPos,
				rnd()*64, 9, Vec3(0.5f,0.5f,0.5f), (rnd()>0.5f) ? eveoCreate : eveoSubstract, evbsSphere, evetVoxelObjects, NULL, 4.f);

			if(GetCVars()->e_VoxelGenerate>20 && (i%(GetCVars()->e_VoxelGenerate/20))==0)
				PrintMessagePlus(" .");
		}
	}

	GetCVars()->e_VoxelGenerate = 0;*/
}

bool CTerrain::Voxel_FindNeighboursForObject(CVoxelObject * pThisObject, CVoxelObject ** arrNeighbours)
{
	// try to find valid neighbors
	memset(arrNeighbours,0,sizeof(CVoxelObject*)*3*3*3);
	arrNeighbours[1*9+1*3+1] = pThisObject;
	Matrix34 thisMatNoTrans = pThisObject->GetMatrix();
	Matrix34 thisMatInvert = pThisObject->GetMatrix();
	thisMatInvert = thisMatInvert.GetInverted();
	thisMatNoTrans.SetTranslation(Vec3(0,0,0));

	// make areas list
	AABB boxArea(pThisObject->GetBBox().min-Vec3(2,2,2),pThisObject->GetBBox().max+Vec3(2,2,2));

  bool bNeibFound = false;

  PodArray<SRNInfo> lstObjects;

  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
    if(Get3DEngine()->m_pObjectsTree[nSID])
  	  Get3DEngine()->m_pObjectsTree[nSID]->MoveObjectsIntoList(&lstObjects,NULL,false,false,false,false,eERType_VoxelObject);
  }

  if(GetVisAreaManager())
  {
	  PodArray<CVisArea*> lstResultVisAreas;
	  GetVisAreaManager()->IntersectWithBox(boxArea, &lstResultVisAreas, false);
	  for(int s=0; s<lstResultVisAreas.Count(); s++)
		  if(lstResultVisAreas[s]->m_pObjectsTree)
			  lstResultVisAreas[s]->m_pObjectsTree->MoveObjectsIntoList(&lstObjects,NULL,false,false,false,false,eERType_VoxelObject);
  }

  // find objects having same scale and rotation

  for(int i=0; i<lstObjects.Count(); i++)
  {
	  if(lstObjects[i].pNode->GetRenderNodeType() == eERType_VoxelObject && lstObjects[i].pNode != pThisObject)
	  {
		  CVoxelObject * pVoxObject = (CVoxelObject*)lstObjects[i].pNode;			
		  Matrix34 matNoTrans = pVoxObject->GetMatrix();
		  matNoTrans.SetTranslation(Vec3(0,0,0));

		  // check same space
		  bool bSameSpace=true;
		  for(int k=0; k<3 && bSameSpace; k++)
		  {
			  for(int j=0; j<4 && bSameSpace; j++)
			  {
				  if(fabs(matNoTrans(k,j) - thisMatNoTrans(k,j))>0.01f)
					  bSameSpace = false;
			  }
		  }

		  if(bSameSpace)
		  {
			  Vec3 vPos = thisMatInvert.TransformPoint(pVoxObject->GetPos(true));
			  float fSizeMeters = DEF_VOX_VOLUME_SIZE*DEF_VOX_UNIT_SIZE;

			  for(int x=-1; x<=1; x++)
			  {
				  for(int y=-1; y<=1; y++)
				  {
					  for(int z=-1; z<=1; z++)
					  {
						  if((x || y || z) && IsEquivalent(vPos,Vec3(fSizeMeters*(float)x,fSizeMeters*(float)y,fSizeMeters*(float)z)))
						  { // put neighbor into right slot
							  arrNeighbours[(x+1)*9+(y+1)*3+(z+1)] = pVoxObject; 
							  bNeibFound = true;
						  }
					  }
				  }
			  }
		  }
	  }
  }

	return bNeibFound;
}

int __cdecl CTerrain__Cmp_CVoxelObject_ViewDist(const void* v1, const void* v2)
{
	CVoxelObject *p1 = *(CVoxelObject**)v1;
	CVoxelObject *p2 = *(CVoxelObject**)v2;

  int nFrameId1 = p1->m_pVoxelVolume->m_nUpdateRequestedFrameId;
  int nFrameId2 = p2->m_pVoxelVolume->m_nUpdateRequestedFrameId;

  if(nFrameId1 > nFrameId2)
    return 1;
  else if(nFrameId1 < nFrameId2)
    return -1;

	float f1 = p1->GetDrawFrame() ? p1->m_fCurrDistance : 100000.f;
	float f2 = p2->GetDrawFrame() ? p2->m_fCurrDistance : 100000.f;

	if(f1 > f2)
		return 1;
	else if(f1 < f2)
		return -1;

	return 0;
}

bool CTerrain::Voxel_Recompile_Modified_Incrementaly_Objects()
{
	PodArray<class CVoxelObject*> & rList = Get3DEngine()->m_lstVoxelObjectsForUpdate;

	qsort(rList.GetElements(), rList.Count(), 
		sizeof(rList[0]), CTerrain__Cmp_CVoxelObject_ViewDist);

	// Compile one voxel object per frame
	if(rList.Count())
	{
		CVoxelObject * arrNeighbours[3*3*3];
		bool bNeibFound = Voxel_FindNeighboursForObject(rList[0],arrNeighbours);

		rList[0]->Compile(bNeibFound ? arrNeighbours : NULL);
		rList.Delete(0);
	}

	return rList.Count()>0;
}

bool CTerrain::Recompile_Modified_Incrementaly_RoadRenderNodes()
{
  PodArray<class CRoadRenderNode*> & rList = Get3DEngine()->m_lstRoadRenderNodesForUpdate;

  while(rList.Count())
  {
    rList[0]->Compile();
    rList.Delete(0);

    if(!GetCVars()->e_VoxTer)
      break;
  }

  return rList.Count()>0;
}

void CTerrain::Voxel_SetFlags(bool bPhysics, bool bSimplify, bool bShadows, bool bMaterials)
{
  if((GetCVars()->e_VoxelUpdatePhysics!=0) != bPhysics && Get3DEngine()->m_pVoxTerrain)
    Get3DEngine()->m_pVoxTerrain->RequestPhysicsUpdate();
  GetCVars()->e_VoxelUpdatePhysics = bPhysics;
//	GetCVars()->e_VoxelLodsNum = bSimplify ? 2 : 0;
}