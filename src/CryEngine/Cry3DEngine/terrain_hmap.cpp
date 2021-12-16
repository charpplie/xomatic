////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   terrain_hmap.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: highmap
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "terrain.h"
#include "VoxTerrain.h"











































































static ILINE float GetHeightLocal( SRangeInfo const& ri, int nX, int nY )
{
	return ri.fOffset + (uint32(ri.GetDataLocal(nX, nY)) & ~STYPE_BIT_MASK) * ri.fRange;
}

static ILINE void SetHeightLocal( SRangeInfo & ri, int nX, int nY, float fHeight )
{
  float fCompHeight = (ri.fRange>(0.01f/65535.f)) ? ((fHeight - ri.fOffset)/ri.fRange) : 0;
  uint16 usNewZValue = ((uint16)CLAMP((int)fCompHeight, 0, 65535)) & ~STYPE_BIT_MASK;
  uint16 usOldSurfTypes = ri.GetDataLocal(nX, nY) & STYPE_BIT_MASK;
  ri.SetDataLocal(nX, nY, usNewZValue | usOldSurfTypes);
}

static ILINE float GetHeightLocal( SRangeInfo const& ri, int nX, int nY, float fX, float fY )
{




















	float fZ = (ri.GetDataLocal(nX  , nY  ) & ~STYPE_BIT_MASK) * (1.f-fX)*(1.f-fY)
		+ (ri.GetDataLocal(nX+1, nY  ) & ~STYPE_BIT_MASK) * fX*(1.f-fY)
		+ (ri.GetDataLocal(nX  , nY+1) & ~STYPE_BIT_MASK) * (1.f-fX)*fY
		+ (ri.GetDataLocal(nX+1, nY+1) & ~STYPE_BIT_MASK) * fX*fY;



	return ri.fOffset + fZ*ri.fRange;
}



static ILINE void SetHeightUnits( SRangeInfo & ri, int nX_units, int nY_units, float fHeight )
{
  ri.nModified = true;

  int nMask = ri.nSize-2;
  if (ri.nUnitBitShift == 0)
  {
    // full lod is available
    SetHeightLocal(ri, nX_units&nMask, nY_units&nMask, fHeight);
  }
/*  else
  {
    // interpolate
    int nX = nX_units >> ri.nUnitBitShift;
    float fX = (nX_units * ri.fInvStep) - nX;
    assert(fX + ri.fInvStep <= 1.f);

    int nY = nY_units >> ri.nUnitBitShift;
    float fY = (nY_units * ri.fInvStep) - nY;
    assert(fY + ri.fInvStep <= 1.f);

    SetHeightLocal(ri, nX&nMask, nY&nMask, fX, fY, fHeight);
  }*/
}

static ILINE float GetHeightUnits( SRangeInfo const& ri, int nX_units, int nY_units )
{
	int nMask = ri.nSize-2;
	if (ri.nUnitBitShift == 0)
	{
		// full lod is available
		return GetHeightLocal(ri, nX_units&nMask, nY_units&nMask);
	}
	else
	{


























		// interpolate
		int nX = nX_units >> ri.nUnitBitShift;
		float fX = (nX_units * ri.fInvStep) - nX;
		assert(fX + ri.fInvStep <= 1.f);

		int nY = nY_units >> ri.nUnitBitShift;
		float fY = (nY_units * ri.fInvStep) - nY;
		assert(fY + ri.fInvStep <= 1.f);

		return GetHeightLocal(ri, nX&nMask, nY&nMask, fX, fY);


	}
}

static void Get4HeightsUnits( SRangeInfo const& ri, int nX_units, int nY_units, float afZ[4] )
{
	int nMask = ri.nSize-2;
	if (ri.nUnitBitShift == 0)
	{
		// full lod is available
		nX_units &= nMask;
		nY_units &= nMask;
		afZ[0] = GetHeightLocal(ri, nX_units, nY_units);
		afZ[1] = GetHeightLocal(ri, nX_units+1, nY_units);
		afZ[2] = GetHeightLocal(ri, nX_units, nY_units+1);
		afZ[3] = GetHeightLocal(ri, nX_units+1, nY_units+1);
	}
	else
	{
		// interpolate
		int nX = nX_units >> ri.nUnitBitShift;
		float fX = (nX_units * ri.fInvStep) - nX;
		assert(fX + ri.fInvStep <= 1.f);

		int nY = nY_units >> ri.nUnitBitShift;
		float fY = (nY_units * ri.fInvStep) - nY;
		assert(fY + ri.fInvStep <= 1.f);

		nX &= nMask;
		nY &= nMask;
		afZ[0] = GetHeightLocal(ri, nX, nY, fX, fY);
		afZ[1] = GetHeightLocal(ri, nX, nY, fX+ri.fInvStep, fY);
		afZ[2] = GetHeightLocal(ri, nX, nY, fX, fY+ri.fInvStep);
		afZ[3] = GetHeightLocal(ri, nX, nY, fX+ri.fInvStep, fY+ri.fInvStep);
	}
}

SPU_NO_INLINE float CHeightMap::GetZApr(float xWS, float yWS, bool bIncludeVoxels, int nSID)
{
  if(!Cry3DEngineBase::GetTerrain()->GetParentNode(nSID))
    return TERRAIN_BOTTOM_LEVEL;

  float fZ;

	// convert into hmap space
	float x1 = xWS * CTerrain::GetInvUnitSize();
	float y1 = yWS * CTerrain::GetInvUnitSize();

  if( !Cry3DEngineBase::GetTerrain() || x1<1 || y1<1 )
    return TERRAIN_BOTTOM_LEVEL;

	int nX = fastftol_positive(x1);
	int nY = fastftol_positive(y1);

	int nHMSize = CTerrain::GetTerrainSize() / CTerrain::GetHeightMapUnitSize();

  if( !Cry3DEngineBase::GetTerrain() || nX<1 || nY<1 || nX>=nHMSize || nY>=nHMSize  )
    fZ = TERRAIN_BOTTOM_LEVEL;
  else
	{
    if(m_bEditor && C3DEngine::m_pGetLayerIdAtCallback && GetCVars()->e_VoxTer)
      return C3DEngine::m_pGetLayerIdAtCallback->GetElevationAtPosition(yWS*CTerrain::GetInvUnitSize(),xWS*CTerrain::GetInvUnitSize());

    float dx1 = x1 - nX;
    float dy1 = y1 - nY;

		float afZCorners[4];

		SRangeInfo& ri = Cry3DEngineBase::GetTerrain()->GetSecInfoUnits(nX, nY, nSID)->m_rangeInfo;
		if (ri.pHMData)
    {
      Get4HeightsUnits( ri, nX, nY, afZCorners );

      if (dx1+dy1 < 1.f)
      {
        // Lower triangle.
        fZ = afZCorners[0] * (1.f-dx1-dy1)
          + afZCorners[1] * dx1
          + afZCorners[2] * dy1;
      }
      else
      {
        // Upper triangle.
        fZ = afZCorners[3] * (dx1+dy1-1.f)
          + afZCorners[2] * (1.f-dx1)
          + afZCorners[1] * (1.f-dy1);
      }
      if(fZ < TERRAIN_BOTTOM_LEVEL)
        fZ = TERRAIN_BOTTOM_LEVEL;
    }
    else
      fZ = TERRAIN_BOTTOM_LEVEL;
  }
	//would require inclusion of RayVoxelIntersection2D for UpdateParticles which is already quiet packed in spu ls code size-wise
#ifndef __SPU__
  if(bIncludeVoxels && nSID<Get3DEngine()->m_pObjectsTree.Count() && Get3DEngine()->m_pObjectsTree[nSID])
  {
    float fClosestHitDistance = 1000000;
    Vec3 vStart(xWS,yWS,16000.f), vEnd(xWS,yWS,0.f);
    Vec3 vHitPoint(0,0,0);
    Get3DEngine()->m_pObjectsTree[nSID]->RayVoxelIntersection2D( vStart, vEnd, vHitPoint, fClosestHitDistance );
    if(vHitPoint.z>fZ)
      fZ = vHitPoint.z;
  }
#endif
  return fZ;
}

float CHeightMap::GetZMax(float x0, float y0, float x1, float y1, int nSID)
{
	// Convert to grid units.
	int nGridSize = CTerrain::GetTerrainSize() / CTerrain::GetHeightMapUnitSize();
	int nX0 = clamp_tpl( int(x0*CTerrain::GetInvUnitSize()), 0, nGridSize-1 );
	int nY0 = clamp_tpl( int(y0*CTerrain::GetInvUnitSize()), 0, nGridSize-1  );
	int nX1 = clamp_tpl( int_ceil(x1*CTerrain::GetInvUnitSize()), 0, nGridSize-1 );
	int nY1 = clamp_tpl( int_ceil(y1*CTerrain::GetInvUnitSize()), 0, nGridSize-1 );

	return GetZMaxFromUnits(nX0, nY0, nX1, nY1, nSID);
}

bool CHeightMap::RayTrace(Vec3 const& vStart, Vec3 const& vEnd, SRayTrace* prt, int nSID)
{
	FUNCTION_PROFILER_3DENGINE;

  if(!Cry3DEngineBase::GetTerrain()->GetParentNode(nSID))
    return false;

	// Temp storage to avoid tests.
	SRayTrace s_rt;
	SRayTrace& rt = prt ? *prt : s_rt;

	float fUnitSize = (float)CTerrain::GetHeightMapUnitSize();
	float fInvUnitSize = CTerrain::GetInvUnitSize();
	int nGridSize = (int)(CTerrain::GetTerrainSize() * fInvUnitSize);

	// Convert to grid units.
	Vec3 vDelta = vEnd-vStart;
	float fMinZ = min(vStart.z, vEnd.z);

	int nX = clamp_tpl( int(vStart.x*fInvUnitSize), 0, nGridSize-1 );
	int nY = clamp_tpl( int(vStart.y*fInvUnitSize), 0, nGridSize-1 );

	int nEndX = clamp_tpl( int(vEnd.x*fInvUnitSize), 0, nGridSize-1 );
	int nEndY = clamp_tpl( int(vEnd.y*fInvUnitSize), 0, nGridSize-1 );

	for (;;)
	{
		// Get heightmap node for current point.
		CTerrainNode const& node = *Cry3DEngineBase::GetTerrain()->GetSecInfoUnits(nX, nY, nSID);
		SRangeInfo const& ri = node.m_rangeInfo;
		int nStepUnits = 1;

		// When entering new node, check with bounding box.
		if (ri.pHMData && fMinZ <= node.m_boxHeigtmapLocal.max.z)
		{
			// Evaluate individual sectors.
			assert((1<<(m_nUnitsToSectorBitShift-ri.nUnitBitShift)) == ri.nSize-1);

			// Get cell for starting point.
			int nType = ri.GetDataUnits(nX, nY) & STYPE_BIT_MASK;
			if (nType != STYPE_HOLE)
			{
				// Get cell vertex values.
				float afZ[4];
				Get4HeightsUnits(ri, nX, nY, afZ);

				// Further zmin check.
				if (fMinZ <= afZ[0] || fMinZ <= afZ[1] || fMinZ <= afZ[2] || fMinZ <= afZ[3])
				{
					if (prt)
					{
						prt->pMaterial = GetTerrain()->GetSurfaceTypes(nSID)[nType].pLayerMat;
						if (prt->pMaterial && prt->pMaterial->GetSubMtlCount() > 2)
							prt->pMaterial = prt->pMaterial->GetSubMtl(2);
					}

					// Select common point on both tris.
					float fX0 = nX*fUnitSize;
					float fY0 = nY*fUnitSize;
					Vec3 vEndRel = vEnd - Vec3(fX0+fUnitSize, fY0, afZ[1]);

					//
					// Intersect with bottom triangle.
					//
					Vec3 vTriDir1(afZ[0]-afZ[1], afZ[0]-afZ[2], fUnitSize);
					float fET1 = vEndRel * vTriDir1;
					if (fET1 < 0.f)
					{
						// End point below plane. Find intersection time.
						float fDT = vDelta * vTriDir1;
						if (fDT <= fET1)
						{
							rt.fInterp = 1.f - fET1/fDT;
							rt.vHit = vStart + vDelta*rt.fInterp;

							// Check against tri boundaries.
							if (rt.vHit.x >= fX0 && rt.vHit.y >= fY0 && rt.vHit.x+rt.vHit.y <= fX0+fY0+fUnitSize)
							{
								rt.vNorm = vTriDir1.GetNormalized();
								return true;
							}
						}
					}

					//
					// Intersect with top triangle.
					//
					Vec3 vTriDir2(afZ[2]-afZ[3], afZ[1]-afZ[3], fUnitSize);
					float fET2 = vEndRel * vTriDir2;
					if (fET2 < 0.f)
					{
						// End point below plane. Find intersection time.
						float fDT = vDelta * vTriDir2;
						if (fDT <= fET2)
						{
							rt.fInterp = 1.f - fET2/fDT;
							rt.vHit = vStart + vDelta*rt.fInterp;

							// Check against tri boundaries.
							if (rt.vHit.x <= fX0+fUnitSize && rt.vHit.y <= fY0+fUnitSize && rt.vHit.x+rt.vHit.y >= fX0+fY0+fUnitSize)
							{
								rt.vNorm = vTriDir2.GetNormalized();
								return true;
							}
						}
					}

					// Check for end point below terrain, to correct for leaks.
					if (nX == nEndX && nY == nEndY)
					{
						if (fET1 < 0.f)
						{
							// Lower tri.
							if (vEnd.x+vEnd.y <= fX0+fY0+fUnitSize)
							{
								rt.fInterp = 1.f;
								rt.vNorm = vTriDir1.GetNormalized();
								rt.vHit = vEnd;
								rt.vHit.z = afZ[0] - ((vEnd.x-fX0) * rt.vNorm.x + (vEnd.y-fY0) * rt.vNorm.y) / rt.vNorm.z;
								return true;
							}
						}
						if (fET2 < 0.f)
						{
							// Upper tri.
							if (vEnd.x+vEnd.y >= fX0+fY0+fUnitSize)
							{
								rt.fInterp = 1.f;
								rt.vNorm = vTriDir2.GetNormalized();
								rt.vHit = vEnd;
								rt.vHit.z = afZ[3] - ((vEnd.x-fX0-fUnitSize) * rt.vNorm.x + (vEnd.y-fY0-fUnitSize) * rt.vNorm.y) / rt.vNorm.z;
								return true;
							}
						}
					}
				}
			}
		}
		else
		{
			// Skip entire node.
			nStepUnits = 1 << (m_nUnitsToSectorBitShift + node.m_nTreeLevel);
			assert(nStepUnits == int(node.m_boxHeigtmapLocal.max.x - node.m_boxHeigtmapLocal.min.x) * fInvUnitSize);
		}

		// Step out of cell.
		int nX1 = nX & ~(nStepUnits-1);
		if (vDelta.x >= 0.f)
			nX1 += nStepUnits;
		float fX = (float)(nX1 * CTerrain::GetHeightMapUnitSize());

		int nY1 = nY & ~(nStepUnits-1);
		if (vDelta.y >= 0.f)
			nY1 += nStepUnits;
		float fY = (float)(nY1 * CTerrain::GetHeightMapUnitSize());

		if (abs((fX-vStart.x) * vDelta.y) < abs((fY-vStart.y) * vDelta.x))
		{
			if (fX*vDelta.x >= vEnd.x*vDelta.x)
				break;
			if (nX1 > nX)
			{
				nX = nX1;
				if (nX >= nGridSize)
					break;
			}
			else
			{
				nX = nX1-1;
				if (nX < 0)
					break;
			}
		}
		else
		{
			if (fY*vDelta.y >= vEnd.y*vDelta.y)
				break;
			if (nY1 > nY)
			{
				nY = nY1;
				if (nY >= nGridSize)
					break;
			}
			else
			{
				nY = nY1-1;
				if (nY < 0)
					break;
			}
		}
	}

	return false;
}

bool CHeightMap::IntersectWithHeightMap(Vec3 vStartPoint, Vec3 vStopPoint, float fDist, int nMaxTestsToScip, int nSID)
{
  // FUNCTION_PROFILER_3DENGINE;
	// convert into hmap space
	float fInvUnitSize = CTerrain::GetInvUnitSize();
	vStopPoint.x *= fInvUnitSize;
	vStopPoint.y *= fInvUnitSize;
	vStartPoint.x *= fInvUnitSize;
	vStartPoint.y *= fInvUnitSize;
	fDist *= fInvUnitSize;

	int nHMSize = CTerrain::GetTerrainSize()/CTerrain::GetHeightMapUnitSize();

	// clamp points
	const bool cClampStart = (vStartPoint.x<0) | (vStartPoint.y<0) | (vStartPoint.x>nHMSize) | (vStartPoint.y>nHMSize);
	if(cClampStart)
	{
		const float fHMSize = (float)nHMSize;
		AABB boxHM(Vec3(0.f,0.f,0.f), Vec3(fHMSize,fHMSize,fHMSize));

		Lineseg ls;
		ls.start = vStartPoint;
		ls.end = vStopPoint;

		Vec3 vRes;
		if(Intersect::Lineseg_AABB(ls, boxHM, vRes) == 0x01)
			vStartPoint = vRes;
		else
			return false;
	}

	const bool cClampStop = (vStopPoint.x<0) | (vStopPoint.y<0) | (vStopPoint.x>nHMSize) | (vStopPoint.y>nHMSize);
	if(cClampStop)
	{
		const float fHMSize = (float)nHMSize;
		AABB boxHM(Vec3(0.f,0.f,0.f), Vec3(fHMSize,fHMSize,fHMSize));

		Lineseg ls;
		ls.start = vStopPoint;
		ls.end = vStartPoint;

		Vec3 vRes;
		if(Intersect::Lineseg_AABB(ls, boxHM, vRes) == 0x01)
			vStopPoint = vRes;
		else
			return false;
	}

	Vec3 vDir = (vStopPoint - vStartPoint);

  int nSteps = fastftol_positive(fDist/GetCVars()->e_TerrainOcclusionCullingStepSize); // every 4 units
	if (nSteps != 0)
	{
		switch(GetCVars()->e_TerrainOcclusionCulling)
		{
		case 4:											// far objects are culled less precise but with far hills as well (same culling speed)
			if(nSteps>GetCVars()->e_TerrainOcclusionCullingMaxSteps)
				nSteps=GetCVars()->e_TerrainOcclusionCullingMaxSteps;
			vDir /= (float)nSteps;
			break;
		default:											// far hills are not culling
			vDir /= (float)nSteps;
			if(nSteps>GetCVars()->e_TerrainOcclusionCullingMaxSteps)
				nSteps=GetCVars()->e_TerrainOcclusionCullingMaxSteps;
			break;
		}
	}

	// scan hmap in sector

  Vec3 vPos = vStartPoint;

	int nTest=0;

	CTerrain* const __restrict pTerrain = Cry3DEngineBase::GetTerrain();
	const int	nUnitsToSectorBitShift = m_nUnitsToSectorBitShift;
	for(nTest=0; nTest<nSteps && nTest<nMaxTestsToScip; nTest++)
  { // leave the underground first
    if(IsPointUnderGround(pTerrain, nUnitsToSectorBitShift, fastround_positive(vPos.x), fastround_positive(vPos.y), vPos.z, nSID))
      vPos += vDir;
    else
      break;
  }
  
	nMaxTestsToScip = min(nMaxTestsToScip, 4);

	for(; nTest<nSteps-nMaxTestsToScip; nTest++)
  {
    vPos += vDir;
    if(IsPointUnderGround(pTerrain, nUnitsToSectorBitShift, fastround_positive(vPos.x), fastround_positive(vPos.y),vPos.z, nSID))
			return true;
  }

  return false;
}

bool CHeightMap::GetHole(const int x, const int y, int nSID) 
{
	int nX_units = x>>m_nBitShift;
	int nY_units = y>>m_nBitShift;
	int nTerrainSize_units = (CTerrain::GetTerrainSize()>>m_nBitShift)-2;

  if(	nX_units<0 || nX_units>nTerrainSize_units || nY_units<0 || nY_units>nTerrainSize_units )
		return false;

  return GetSurfTypefromUnits(nX_units, nY_units, nSID) == STYPE_HOLE;
}

float CHeightMap::GetZSafe(int x, int y, int nSID)
{ 
  if(x>=0 && y>=0 && x<CTerrain::GetTerrainSize() && y<CTerrain::GetTerrainSize())
    return max(GetZ(x,y,nSID), (float)TERRAIN_BOTTOM_LEVEL);

  return TERRAIN_BOTTOM_LEVEL; 
}

SPU_NO_INLINE uint8 CHeightMap::GetSurfaceTypeID(int x, int y, int nSID)
{
  if(x>=0 && y>=0 && x<CTerrain::GetTerrainSize() && y<CTerrain::GetTerrainSize())
		return GetSurfTypefromUnits(x>>m_nBitShift,y>>m_nBitShift, nSID);

  return 0;
}

SPU_NO_INLINE float CHeightMap::GetZ(const int x, const int y, int nSID) 
{ 
  if(!Cry3DEngineBase::GetTerrain()->GetParentNode(nSID))
    return TERRAIN_BOTTOM_LEVEL;

	return GetZfromUnits(x>>m_nBitShift,y>>m_nBitShift,nSID);
}

void CHeightMap::SetZ(const int x, const int y, float fHeight, int nSID) 
{ 
  return SetZfromUnits(x>>m_nBitShift,y>>m_nBitShift, fHeight, nSID);
}

uint8 CHeightMap::GetSurfTypefromUnits(uint32 nX_units, uint32 nY_units, int nSID) 
{ 
	if (!Cry3DEngineBase::GetTerrain())
		return 0;
	Cry3DEngineBase::GetTerrain()->ClampUnits(nX_units, nY_units);
	SRangeInfo& ri = Cry3DEngineBase::GetTerrain()->GetSecInfoUnits(nX_units, nY_units, nSID)->m_rangeInfo;
	if (!ri.pHMData)
		return 0;
	return ri.GetDataUnits(nX_units, nY_units) & STYPE_BIT_MASK;
}

float CHeightMap::GetZfromUnits(uint32 nX_units, uint32 nY_units, int nSID)
{
	if(!Cry3DEngineBase::GetTerrain())
		return 0;

	Cry3DEngineBase::GetTerrain()->ClampUnits(nX_units, nY_units);
	SRangeInfo& ri = Cry3DEngineBase::GetTerrain()->GetSecInfoUnits(nX_units, nY_units, nSID)->m_rangeInfo;
	SpuStackValue<SRangeInfo> stackRI(ri);
	if (!stackRI->pHMData)
		return 0;

	return GetHeightUnits( stackRI, nX_units, nY_units );
}

void CHeightMap::SetZfromUnits(uint32 nX_units, uint32 nY_units, float fHeight, int nSID)
{
  if(!Cry3DEngineBase::GetTerrain())
    return;

  Cry3DEngineBase::GetTerrain()->ClampUnits(nX_units, nY_units);
  SRangeInfo& ri = Cry3DEngineBase::GetTerrain()->GetSecInfoUnits(nX_units, nY_units, nSID)->m_rangeInfo;
  if (!ri.pHMData)
    return;

  SetHeightUnits( ri, nX_units, nY_units, fHeight );
}

float CHeightMap::GetZMaxFromUnits(uint32 nX0_units, uint32 nY0_units, uint32 nX1_units, uint32 nY1_units, int nSID)
{ 
	if(!Cry3DEngineBase::GetTerrain())
		return TERRAIN_BOTTOM_LEVEL;

	Array2d<struct CTerrainNode*> & sectorLayer = Cry3DEngineBase::GetTerrain()->m_arrSecInfoPyramid[nSID][0];
	uint32 nLocalMask = (1<<m_nUnitsToSectorBitShift)-1;

	uint32 nSizeXY = Cry3DEngineBase::GetTerrain()->GetTerrainSize() >> m_nBitShift;
	assert(nX0_units<=nSizeXY && nY0_units<=nSizeXY);
	assert(nX1_units<=nSizeXY && nY1_units<=nSizeXY);

	float fZMax = TERRAIN_BOTTOM_LEVEL;

	// Iterate sectors.
	uint32 nX0_sector = nX0_units>>m_nUnitsToSectorBitShift,
			 nX1_sector = nX1_units>>m_nUnitsToSectorBitShift,
			 nY0_sector = nY0_units>>m_nUnitsToSectorBitShift,
			 nY1_sector = nY1_units>>m_nUnitsToSectorBitShift;
	for (uint32 nX_sector = nX0_sector; nX_sector <= nX1_sector; nX_sector++)
	{
		for (uint32 nY_sector = nY0_sector; nY_sector <= nY1_sector; nY_sector++)
		{
			CTerrainNode& node = *sectorLayer[nX_sector][nY_sector];
			SRangeInfo& ri = node.m_rangeInfo;
			if (!ri.pHMData)
				continue;

			assert(int_round(1.f/ri.fInvStep) == (1<<ri.nUnitBitShift));
			assert((1<<(m_nUnitsToSectorBitShift-ri.nUnitBitShift)) == ri.nSize-1);

			// Iterate points in sector.
			uint32 nX0_pt = (nX_sector == nX0_sector ? nX0_units&nLocalMask : 0) >> ri.nUnitBitShift;
			uint32 nX1_pt = (nX_sector == nX1_sector ? nX1_units&nLocalMask : nLocalMask) >> ri.nUnitBitShift;;
			uint32 nY0_pt = (nY_sector == nY0_sector ? nY0_units&nLocalMask : 0) >> ri.nUnitBitShift;;
			uint32 nY1_pt = (nY_sector == nY1_sector ? nY1_units&nLocalMask : nLocalMask) >> ri.nUnitBitShift;;

			float fSectorZMax;
			if ((nX1_pt-nX0_pt+1)*(nY1_pt-nY0_pt+1) >= (uint32)(ri.nSize-1)*(ri.nSize-1))
			{
				fSectorZMax = node.m_boxHeigtmapLocal.max.z;
			}
			else
			{
				uint32 nSectorZMax = 0;
				for (uint32 nX_pt = nX0_pt; nX_pt <= nX1_pt; nX_pt++)
				{
					for (uint32 nY_pt = nY0_pt; nY_pt <= nY1_pt; nY_pt++)
					{
						int nCellLocal = nX_pt*ri.nSize + nY_pt;
						assert(nCellLocal>=0 && nCellLocal<ri.nSize*ri.nSize);
						nSectorZMax = max(nSectorZMax, uint32(ri.pHMData[nCellLocal]));
					}
				}
				fSectorZMax = ri.fOffset + (nSectorZMax & ~STYPE_BIT_MASK)*ri.fRange;
			}

			fZMax = max(fZMax, fSectorZMax);
		}
	}
	return fZMax;
}

bool CHeightMap::IsPointUnderGround(CTerrain* const __restrict pTerrain, 
																		int	nUnitsToSectorBitShift,
																		uint32 nX_units, 
																		uint32 nY_units, 
																		float fTestZ, 
                                    int nSID) 
{ 
//  FUNCTION_PROFILER_3DENGINE;
#if !defined(__SPU__)
  if(GetCVars()->e_TerrainOcclusionCullingDebug)
  {
    Vec3 vTerPos(0,0,0);
    vTerPos.Set( (float)(nX_units*CTerrain::GetHeightMapUnitSize()), (float)(nY_units*CTerrain::GetHeightMapUnitSize()), 0);
    vTerPos.z = pTerrain->GetZfromUnits( nX_units, nY_units, nSID );
    DrawSphere(vTerPos, 1.f, Col_Red);
  }
#endif
	pTerrain->ClampUnits(nX_units, nY_units);
	CTerrainNode * pNode = pTerrain->GetSecInfoUnits(nX_units, nY_units, nSID);
	SRangeInfo & ri = pNode->m_rangeInfo;










	if(!ri.pHMData || (pNode->m_bNoOcclusion!=0) || (pNode->m_bHasHoles!=0))

		return false;

	if(fTestZ<ri.fOffset)
		return true;

	float fZ = GetHeightUnits(ri, nX_units, nY_units);

	return fTestZ < fZ;
}

CHeightMap::SCachedHeight CHeightMap::m_arrCacheHeight[nHMCacheSize][nHMCacheSize];
CHeightMap::SCachedSurfType CHeightMap::m_arrCacheSurfType[nHMCacheSize][nHMCacheSize];

float CHeightMap::GetHeightFromUnits_Callback(int ix, int iy)
{
  /*
  static int nAll=0;
  static int nBad=0;

  if(nAll && nAll/1000000*1000000 == nAll)
  {
    Error("Height_RealReads = %.2f", (float)nBad/nAll);
    nAll=0;
    nBad=0;

    if(sizeof(m_arrCache[0][0]) != 8)
      Error("CHeightMap::GetHeightFromUnits_Callback:  sizeof(m_arrCacheSurfType[0][0]) != 8");
  }

  nAll++;
  */
  CHeightMap::SCachedHeight & rCache = m_arrCacheHeight[ix&(nHMCacheSize-1)][iy&(nHMCacheSize-1)];
  if(rCache.x == ix && rCache.y == iy)
    return rCache.fHeight;

//  nBad++;

  assert(sizeof(m_arrCacheHeight[0][0]) == 8);

  rCache.x = ix;
  rCache.y = iy;
  rCache.fHeight = Cry3DEngineBase::GetTerrain()->GetZfromUnits(ix, iy, GetDefSID());

	return rCache.fHeight;
}

unsigned char CHeightMap::GetSurfaceTypeFromUnits_Callback(int ix, int iy)
{
  /*
  static int nAll=0;
  static int nBad=0;

  if(nAll && nAll/1000000*1000000 == nAll)
  {
    Error("SurfaceType_RealReads = %.2f", (float)nBad/nAll);
    nAll=0;
    nBad=0;

    if(sizeof(m_arrCacheSurfType[0][0]) != 4)
      Error("CHeightMap::GetSurfaceTypeFromUnits_Callback:  sizeof(m_arrCacheSurfType[0][0]) != 4");
  }

  nAll++;
  */

  CHeightMap::SCachedSurfType & rCache = m_arrCacheSurfType[ix&(nHMCacheSize-1)][iy&(nHMCacheSize-1)];
  if(rCache.x == ix && rCache.y == iy)
    return rCache.surfType;

//  nBad++;

  assert(sizeof(m_arrCacheSurfType[0][0]) == 4);

  rCache.x = ix;
  rCache.y = iy;
	rCache.surfType = Cry3DEngineBase::GetTerrain()->GetSurfTypefromUnits(ix, iy, GetDefSID());

  return rCache.surfType;
}
