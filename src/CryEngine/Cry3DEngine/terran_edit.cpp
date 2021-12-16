////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terran_edit.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: add/remove static objects, modify hmap (used by editor)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "terrain_sector.h"
#include "StatObj.h"
#include "ObjMan.h"
#include "3dEngine.h"
#include "Vegetation.h"
#include "VoxMan.h"
#include "RoadRenderNode.h"

//////////////////////////////////////////////////////////////////////////
IRenderNode* CTerrain::AddVegetationInstance( int nStaticGroupID,const Vec3 &vPos,const float fScale,uint8 ucBright,uint8 angle, int nSID )
{
	if( vPos.x<=0 || vPos.y<=0 || vPos.x>=CTerrain::GetTerrainSize() || vPos.y>=CTerrain::GetTerrainSize() || fScale<=0 )
		return 0;

	if (nStaticGroupID<0 || nStaticGroupID >= (int)GetObjManager()->m_lstStaticTypes.size())
	{
		return 0;
	}

	CVegetation * pEnt = (CVegetation*)Get3DEngine()->CreateRenderNode( eERType_Vegetation );
	pEnt->SetScale(fScale);
	pEnt->m_vPos = vPos;
	pEnt->SetStatObjGroupId(nStaticGroupID);
	pEnt->m_ucAngle = angle;

	if (!GetObjManager()->m_lstStaticTypes[nStaticGroupID].GetStatObj())
	{
		Warning("I3DEngine::AddStaticObject: Attempt to add object of undefined type");
		pEnt->ReleaseNode();
		return 0;
	}
	
	pEnt->CalcBBox();

  float fEntLengthSquared = pEnt->GetBBox().GetSize().GetLengthSquared();
  if(fEntLengthSquared > MAX_VALID_OBJECT_VOLUME || !_finite(fEntLengthSquared) || fEntLengthSquared<=0)
  {
    Warning("CTerrain::AddVegetationInstance: Object has invalid bbox: %s,%s, GetRadius() = %.2f",
      pEnt->GetName(), pEnt->GetEntityClassName(), cry_sqrtf(fEntLengthSquared)*0.5f);
  }

	pEnt->Physicalize( );
	Get3DEngine()->RegisterEntity(pEnt);
	return pEnt;
}

void CTerrain::RemoveAllStaticObjects(int nSID)
{
  if(!Get3DEngine()->m_pObjectsTree[nSID] || nSID>=Get3DEngine()->m_pObjectsTree.Count())
		return;

	PodArray<SRNInfo> lstObjects;
	Get3DEngine()->m_pObjectsTree[nSID]->MoveObjectsIntoList(&lstObjects, NULL);

	for(int i=0; i<lstObjects.Count(); i++)
	{
		IRenderNode * pNode = lstObjects.GetAt(i).pNode;
		if(pNode->GetRenderNodeType() == eERType_Vegetation && !(pNode->GetRndFlags() & ERF_PROCEDURAL))
			pNode->ReleaseNode();
	}
}

#define GET_Z_VAL(_x,_y) pTerrainBlock[(_x)*nTerrainSize+(_y)]

void CTerrain::BuildErrorsTableForArea(float * pLodErrors, int nMaxLods, 
																			 int X1, int Y1, int X2, int Y2, float * pTerrainBlock,
																			 uint8 * pSurfaceData, int nSurfOffsetX, int nSurfOffsetY,
																			 int nSurfSizeX, int nSurfSizeY)
{

	memset(pLodErrors, 0, nMaxLods*sizeof(pLodErrors[0]));
	int nSectorSize = CTerrain::GetSectorSize()/CTerrain::GetHeightMapUnitSize();
	int nTerrainSize = CTerrain::GetTerrainSize()/CTerrain::GetHeightMapUnitSize();

  bool bSectorHasHoles = false;

  {
    int nLodUnitSize = 1;
    int x1 = max(0,X1-nLodUnitSize);
    int x2 = min(nTerrainSize-nLodUnitSize,X2+nLodUnitSize);
    int y1 = max(0,Y1-nLodUnitSize);
    int y2 = min(nTerrainSize-nLodUnitSize,Y2+nLodUnitSize);

    for(int X=x1; X<x2; X+=nLodUnitSize) 
    {
      for(int Y=y1; Y<y2; Y+=nLodUnitSize)
      {
        int nSurfX = (X-nSurfOffsetX);
        int nSurfY = (Y-nSurfOffsetY);
        if( nSurfX>=0 && nSurfY>=0 && nSurfX<nSurfSizeX && nSurfY<nSurfSizeY && pSurfaceData)
        {
          int nSurfCell = nSurfX*nSurfSizeY + nSurfY;
          assert(nSurfCell>=0 && nSurfCell<nSurfSizeX*nSurfSizeY);
          if(pSurfaceData[nSurfCell] == STYPE_HOLE)
            bSectorHasHoles = true;
        }
      }
    }
  }

	for(int nLod=1; nLod<nMaxLods; nLod++)
	{ // calculate max difference between detail levels and actual height map
		float fMaxDiff = 0;

    if(bSectorHasHoles)
      fMaxDiff = max(fMaxDiff,8.f);

		int nLodUnitSize = (1<<nLod);

		assert(nLodUnitSize <= nSectorSize);

		int x1 = max(0,X1-nLodUnitSize);
		int x2 = min(nTerrainSize-nLodUnitSize,X2+nLodUnitSize);
		int y1 = max(0,Y1-nLodUnitSize);
		int y2 = min(nTerrainSize-nLodUnitSize,Y2+nLodUnitSize);

		float fSurfSwitchError = 0.5f*nLod;

		for(int X=x1; X<x2; X+=nLodUnitSize) 
		{
			for(int Y=y1; Y<y2; Y+=nLodUnitSize)
			{
				uint8 nLodedSurfType = 0;
				{
					int nSurfX = (X-nSurfOffsetX);
					int nSurfY = (Y-nSurfOffsetY);
					if( nSurfX>=0 && nSurfY>=0 && nSurfX<nSurfSizeX && nSurfY<nSurfSizeY && pSurfaceData)
					{
						int nSurfCell = nSurfX*nSurfSizeY + nSurfY;
						assert(nSurfCell>=0 && nSurfCell<nSurfSizeX*nSurfSizeY);
						nLodedSurfType = pSurfaceData[nSurfCell] & STYPE_BIT_MASK;
					}
				}

				for( int x=1; x<nLodUnitSize; x++)
				{
					float kx = (float)x/(float)nLodUnitSize;

					float z1 = (1.f-kx)*GET_Z_VAL(X+0,Y+					 0) + (kx)*GET_Z_VAL(X+nLodUnitSize,Y+					 0);
					float z2 = (1.f-kx)*GET_Z_VAL(X+0,Y+nLodUnitSize) + (kx)*GET_Z_VAL(X+nLodUnitSize,Y+nLodUnitSize);

					for( int y=1; y<nLodUnitSize; y++)
					{
						// skip map borders
						int nBorder = (nSectorSize>>2);
						if(	(X+x) < nBorder || (X+x) > (nTerrainSize-nBorder) || 
								(Y+y) < nBorder || (Y+y) > (nTerrainSize-nBorder))
							continue;

						float ky = (float)y/nLodUnitSize;
						float fInterpolatedZ = (1.f-ky)*z1 + ky*z2;
						float fRealZ = GET_Z_VAL(X+x,Y+y);
						float fDiff = fabs(fRealZ-fInterpolatedZ);

						if(fMaxDiff<fSurfSwitchError)
						{
							int nSurfX = (X+x-nSurfOffsetX);
							int nSurfY = (Y+y-nSurfOffsetY);
							if( nSurfX>=0 && nSurfY>=0 && nSurfX<nSurfSizeX && nSurfY<nSurfSizeY && pSurfaceData)
							{
								int nSurfCell = nSurfX*nSurfSizeY + nSurfY;
								assert(nSurfCell>=0 && nSurfCell<nSurfSizeX*nSurfSizeY);
								uint8 nRealSurfType = pSurfaceData[nSurfCell] & STYPE_BIT_MASK;

								// rise error if surface types will pop
								if(nRealSurfType!=nLodedSurfType)
									fDiff = max(fDiff, fSurfSwitchError);
							}
						}

						if(fDiff > fMaxDiff)
							fMaxDiff = fDiff;
					}
				}
			}
		}
		// note: values in m_arrGeomErrors table may be non incremental - this is correct
		pLodErrors[nLod] = fMaxDiff;
	}
}

void CTerrain::SetTerrainElevation(int X1, int Y1, int nSizeX, int nSizeY, float * pTerrainBlock, 
                                   unsigned char * pSurfaceData, 
                                   uint32 * pResolMap, int nResolMapSizeX, int nResolMapSizeY, int nSID)
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());
  FUNCTION_PROFILER_3DENGINE;

	float fStartTime = GetCurAsyncTimeSec();
	int nUnitSize = CTerrain::GetHeightMapUnitSize();
	int nHmapSize = CTerrain::GetTerrainSize()/nUnitSize;

  ResetHeightMapCache();

	// everything is in units in this function

	assert(nSizeX == nSizeY);
	assert(X1 == ((X1>>m_nUnitsToSectorBitShift)<<m_nUnitsToSectorBitShift));
	assert(Y1 == ((Y1>>m_nUnitsToSectorBitShift)<<m_nUnitsToSectorBitShift));
	assert(nSizeX == ((nSizeX>>m_nUnitsToSectorBitShift)<<m_nUnitsToSectorBitShift));
	assert(nSizeY == ((nSizeY>>m_nUnitsToSectorBitShift)<<m_nUnitsToSectorBitShift));

	if( X1<0 || Y1<0 || X1+nSizeX>nHmapSize || Y1+nSizeY>nHmapSize  )
	{
		Warning("CTerrain::SetTerrainHeightMapBlock: (X1,Y1) values out of range");
		return;
	}

	if(!GetParentNode(nSID))
		BuildSectorsTree(false, nSID);

	Array2d<struct CTerrainNode*> & sectorLayer = m_arrSecInfoPyramid[nSID][0];

	int rangeX1 = max(0,X1>>m_nUnitsToSectorBitShift);
	int rangeY1 = max(0,Y1>>m_nUnitsToSectorBitShift);
	int rangeX2 = min(sectorLayer.GetSize(), (X1+nSizeX)>>m_nUnitsToSectorBitShift);
	int rangeY2 = min(sectorLayer.GetSize(), (Y1+nSizeY)>>m_nUnitsToSectorBitShift);

	for(int rangeX=rangeX1; rangeX<rangeX2; rangeX++) 
	for(int rangeY=rangeY1; rangeY<rangeY2; rangeY++)
	{
		CTerrainNode * pTerrainNode = sectorLayer[rangeX][rangeY];

		int x1 =  rangeX   <<m_nUnitsToSectorBitShift;
		int y1 =  rangeY   <<m_nUnitsToSectorBitShift;
		int x2 = (rangeX+1)<<m_nUnitsToSectorBitShift;
		int y2 = (rangeY+1)<<m_nUnitsToSectorBitShift;

		// find min/max
		float fMin = pTerrainBlock[x1*nHmapSize + y1];
		float fMax = fMin;

		for(int x=x1; x<=x2; x++) 
		for(int y=y1; y<=y2; y++) 
		{
			float fHeight = pTerrainBlock[CLAMP(x, 0, nHmapSize-1)*nHmapSize + CLAMP(y, 0, nHmapSize-1)];

			if(fHeight>fMax) fMax=fHeight;
			if(fHeight<fMin) fMin=fHeight;
		}


    // reserve some space for in-game deformations
    fMin = max( 0.f, fMin-TERRAIN_DEFORMATION_MAX_DEPTH );

    float fMaxTexelSizeMeters = -1;

    if(pResolMap)
    {
      // get max allowed texture resolution here
      int nResMapX = (nResolMapSizeX * (x1/2+x2/2) / nHmapSize);
      int nResMapY = (nResolMapSizeY * (y1/2+y2/2) / nHmapSize);
      nResMapX = CLAMP(nResMapX, 0, nResolMapSizeX-1);
      nResMapY = CLAMP(nResMapY, 0, nResolMapSizeY-1);
      int nTexRes = pResolMap[nResMapY + nResMapX*nResolMapSizeY];
      int nResTileSizeMeters = GetTerrainSize()/nResolMapSizeX;
      fMaxTexelSizeMeters = (float)nResTileSizeMeters/(float)nTexRes;
    }

		// find error metrics
		if(!pTerrainNode->m_pGeomErrors)
			pTerrainNode->m_pGeomErrors = new float[m_nUnitsToSectorBitShift];    
    if(GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTer)
    {
      pTerrainNode->m_pGeomErrors[0]=0;
      for(int e=1; e<m_nUnitsToSectorBitShift; e++)
        pTerrainNode->m_pGeomErrors[e] = 1.f;
    }
    else
		  BuildErrorsTableForArea(pTerrainNode->m_pGeomErrors, m_nUnitsToSectorBitShift, x1, y1, x2, y2, pTerrainBlock,pSurfaceData, X1, Y1, nSizeX, nSizeY);
		assert(pTerrainNode->m_pGeomErrors[0] == 0);

		pTerrainNode->m_boxHeigtmapLocal.min.Set((float)(x1*nUnitSize),(float)(y1*nUnitSize),fMin);
		pTerrainNode->m_boxHeigtmapLocal.max.Set((float)(x2*nUnitSize),(float)(y2*nUnitSize),max(fMax,GetWaterLevel()));

		// same as in SetLOD()
		float fAllowedError = ( GetCVars()->e_TerrainLodRatio * 32.f ) / 180.f * 2.5f;
		int nGeomMML = m_nUnitsToSectorBitShift-1;
		for( ; nGeomMML>0; nGeomMML-- )
    {
      float fStepMeters = (float)((1<<nGeomMML)*GetHeightMapUnitSize());

      if(fStepMeters <= fMaxTexelSizeMeters)
        break;

			if(pTerrainNode->m_pGeomErrors[nGeomMML] < fAllowedError)
				break;
    }

		if(nGeomMML == m_nUnitsToSectorBitShift-1 && (fMax - fMin)<0.05f)
			nGeomMML = m_nUnitsToSectorBitShift;

		SRangeInfo & ri = pTerrainNode->m_rangeInfo;
		ri.fOffset = fMin;
		ri.fRange	 = (fMax - fMin) / 65535.f;
	
		int nStep = 1<<nGeomMML;
		int nMaxStep = 1<<m_nUnitsToSectorBitShift;

		if(ri.nSize != nMaxStep/nStep+1)
		{
			delete [] ri.pHMData;
			ri.nSize = nMaxStep/nStep+1;
			ri.pHMData = new uint16[ri.nSize*ri.nSize];
			ri.UpdateBitShift(m_nUnitsToSectorBitShift);
		}

		// fill height map data array in terrain node, all in units
		for(int x=x1; x<=x2; x+=nStep)
		for(int y=y1; y<=y2; y+=nStep)
		{
      float fHeight = 0;
      if(x>1 && y>1 && x<(nHmapSize-2) && y<(nHmapSize-2))
			  fHeight = pTerrainBlock[min(nHmapSize-1,x)*nHmapSize + min(nHmapSize-1,y)];

			float fCompHeight = (ri.fRange>(0.01f/65535.f)) ? ((fHeight - fMin)/ri.fRange) : 0;
//			assert(fCompHeight>=0 && fCompHeight<65536.f);
			uint16 usNewZValue = (uint16)CLAMP((int)fCompHeight, 0, 65535);
			
			usNewZValue &= ~STYPE_BIT_MASK;
				
			int x_local = (x - x1)/nStep;
			int y_local = (y - y1)/nStep;

			int nCellLocal = x_local*ri.nSize + y_local;
			assert(nCellLocal>=0 && nCellLocal<ri.nSize*ri.nSize);

			uint16 usSurfType = ri.pHMData[nCellLocal] & STYPE_BIT_MASK;

			assert( x>=X1 && y>=Y1 && x<=X1+nSizeX && y<=Y1+nSizeY && pSurfaceData);

			int nSurfX = (x-X1);
			int nSurfY = (y-Y1);

			if( nSurfX>=0 && nSurfY>=0 && nSurfX<nSizeX && nSurfY<nSizeY && pSurfaceData)
			{
				int nSurfCell = nSurfX*nSizeY + nSurfY;
				assert(nSurfCell>=0 && nSurfCell<nSizeX*nSizeY);
				usSurfType = pSurfaceData[nSurfCell] & STYPE_BIT_MASK;
			}

			ri.pHMData[nCellLocal] = usSurfType | usNewZValue;
		}

		// re-init surface types info and update vert buffers in entire brunch
		if(GetParentNode(nSID))
		{
			if(Get3DEngine()->m_pObjectsTree[nSID])
				if(CVoxelObject* pLinkedVoxelObject = (CVoxelObject*)Get3DEngine()->m_pObjectsTree[nSID]->FindTerrainSectorVoxObject(pTerrainNode->GetBBox()))
					pLinkedVoxelObject->ScheduleRebuild();

			CTerrainNode * pNode = pTerrainNode;
			while(pNode)
			{
				pNode->ReleaseHeightMapGeometry();
				pNode->RemoveProcObjects();
				pNode->UpdateDetailLayersInfo(false);

				// propagate bounding boxes and error metrics to parents
				if(pNode != pTerrainNode)
				{
					for(int i=0; i<m_nUnitsToSectorBitShift; i++)
					{
						pNode->m_pGeomErrors[i] = max(max(
							pNode->m_arrChilds[0]->m_pGeomErrors[i],
							pNode->m_arrChilds[1]->m_pGeomErrors[i]),max(
							pNode->m_arrChilds[2]->m_pGeomErrors[i],
							pNode->m_arrChilds[3]->m_pGeomErrors[i]));
					}

					pNode->m_boxHeigtmapLocal.min = SetMaxBB();
					pNode->m_boxHeigtmapLocal.max = SetMinBB();

					for(int nChild=0; nChild<4; nChild++)
					{
						pNode->m_boxHeigtmapLocal.min.CheckMin(pNode->m_arrChilds[nChild]->m_boxHeigtmapLocal.min);
						pNode->m_boxHeigtmapLocal.max.CheckMax(pNode->m_arrChilds[nChild]->m_boxHeigtmapLocal.max);
					}
				}

				pNode = pNode->m_pParent;
			}
		}
	}

	if(GetCurAsyncTimeSec()-fStartTime > 1)
		PrintMessage("CTerrain::SetTerrainElevation took %.2f sec", GetCurAsyncTimeSec()-fStartTime );

	if(Get3DEngine()->m_pObjectsTree[nSID])
		Get3DEngine()->m_pObjectsTree[nSID]->UpdateTerrainNodes();

  // update roads
  if(Get3DEngine()->m_pObjectsTree[nSID] && (GetCVars()->e_TerrainDeformations || m_bEditor))
  {
    PodArray<IRenderNode*> lstRoads;

    AABB aabb(
      Vec3((float)X1*(float)nUnitSize, (float)Y1*(float)nUnitSize, 0.f), 
      Vec3((float)X1*(float)nUnitSize+(float)nSizeX*(float)nUnitSize, (float)Y1*(float)nUnitSize+(float)nSizeY*(float)nUnitSize, 1024.f));

    Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstRoads, eERType_Road, &aabb);
    for(int i=0; i<lstRoads.Count(); i++)
    {
      CRoadRenderNode * pRoad = (CRoadRenderNode *)lstRoads[i];
      pRoad->OnTerrainChanged();
    }
  }

  if(GetParentNode(nSID))
    GetParentNode(nSID)->UpdateRangeInfoShift();

	m_bHeightMapModified = 0;
}

void CTerrain::SetTerrainSectorTexture( const int nTexSectorX, const int nTexSectorY, unsigned int textureId, int nSID )
{
  int nDiffTexTreeLevelOffset = 0;

  if( nTexSectorX < 0 || 
      nTexSectorY < 0 || 
      nTexSectorX >= CTerrain::GetSectorsTableSize()>>nDiffTexTreeLevelOffset || 
      nTexSectorY >= CTerrain::GetSectorsTableSize()>>nDiffTexTreeLevelOffset )
  {
    Warning("CTerrain::LockSectorTexture: (nTexSectorX, nTexSectorY) values out of range");
    return;
  }

  CTerrainNode * pNode = m_arrSecInfoPyramid[nSID][nDiffTexTreeLevelOffset][nTexSectorX][nTexSectorY];
  pNode->SetSectorTexture(textureId);

	while(pNode)
	{
		pNode->m_bMergeNotAllowed = true;
		pNode = pNode->m_pParent;
	}
}

void CTerrain::ResetTerrainVertBuffers(const AABB * pBox, int nSID)
{
  if(!this)
    return;

  if(nSID==-1)
  {
    for(int id=0; id<m_pParentNodes.Count(); id++)
    {
      if(GetParentNode(id))
        GetParentNode(id)->ReleaseHeightMapGeometry(true, pBox);
    }
  }
  else if(GetParentNode(nSID))
    GetParentNode(nSID)->ReleaseHeightMapGeometry(true, pBox);
}

void CTerrain::SetOceanWaterLevel( float fOceanWaterLevel )
{
	SetWaterLevel(fOceanWaterLevel);
}
