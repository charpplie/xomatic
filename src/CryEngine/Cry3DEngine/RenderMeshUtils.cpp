////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 2001-2007.
// -------------------------------------------------------------------------
//  File name:   RenderMeshUtils.cpp
//  Created:     14/11/2006 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RenderMeshUtils.h"

namespace
{
	enum { MAX_CACHED_HITS = 8 };
	struct SCachedHit
	{
		IRenderMesh *pRenderMesh;
		SRayHitInfo hitInfo;
		Vec3 tri[3];
	};
	static SCachedHit last_hits[MAX_CACHED_HITS];
}

template <class T> bool GetBarycentricCoordinates(T P_A, T B_A, T C_A, float & u, float & v, float & w, float fBorder)
{
  // Compute vectors        
  const T & v0 = C_A;
  const T & v1 = B_A;
  const T & v2 = P_A;

  // Compute dot products
  float dot00 = v0.Dot(v0);
  float dot01 = v0.Dot(v1);
  float dot02 = v0.Dot(v2);
  float dot11 = v1.Dot(v1);
  float dot12 = v1.Dot(v2);

  // Compute barycentric coordinates
  float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
  u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  v = (dot00 * dot12 - dot01 * dot02) * invDenom;
  w = 1.f - u - v;

  // Check if point is in triangle
  return (u >= -fBorder) && (v >= -fBorder) && (w >= -fBorder);
}

void CRenderMeshUtils::ClearHitCache()
{
  // do not allow items to stay too long in the cache, it allows to minimize wrong hit detections
  memmove( &last_hits[1],&last_hits[0],sizeof(last_hits)-sizeof(last_hits[0]) ); // Move hits to the end of array, throwing out the last one.
  memset(&last_hits[0], 0, sizeof(last_hits[0]));
}
//////////////////////////////////////////////////////////////////////////
bool CRenderMeshUtils::RayIntersection( IRenderMesh *pRenderMesh, SRayHitInfo &hitInfo,IMaterial *pCustomMtl )
{
  if(hitInfo.bGetVertColorAndTC)
    return RayIntersectionFast( pRenderMesh, hitInfo,pCustomMtl );

  FUNCTION_PROFILER_3DENGINE;

	//CTimeValue t0 = gEnv->pTimer->GetAsyncTime();

	float fMaxDist2 = hitInfo.fMaxHitDistance*hitInfo.fMaxHitDistance;

	Vec3 vHitPos(0,0,0);
	Vec3 vHitNormal(0,0,0);

	static bool bClearHitCache = true;
	if (bClearHitCache)
	{
		memset( last_hits,0,sizeof(last_hits) );
		bClearHitCache = false;
	}

	if (hitInfo.bUseCache)
	{
		Vec3 vOut;
		// Check for cached hits.
		for (int i = 0; i < MAX_CACHED_HITS; i++)
		{
			if (last_hits[i].pRenderMesh == pRenderMesh)
			{
				// If testing same render mesh, check if we hit the same triangle again.
				if (Intersect::Ray_Triangle( hitInfo.inRay,last_hits[i].tri[0], last_hits[i].tri[2], last_hits[i].tri[1],vOut ))
				{
					if (fMaxDist2)
					{
						float fDistance2 = hitInfo.inReferencePoint.GetSquaredDistance(vOut);
						if (fDistance2 > fMaxDist2)
							continue; // Ignore hits that are too far.
					}

					// Cached hit.
					hitInfo.vHitPos = vOut;
					hitInfo.vHitNormal = last_hits[i].hitInfo.vHitNormal;
					hitInfo.nHitMatID = last_hits[i].hitInfo.nHitMatID;
					hitInfo.nHitSurfaceID = last_hits[i].hitInfo.nHitSurfaceID;

					if(hitInfo.inRetTriangle)
					{
						hitInfo.vTri0 = last_hits[i].tri[0];
						hitInfo.vTri1 = last_hits[i].tri[1];
						hitInfo.vTri2 = last_hits[i].tri[2];
					}
					//CTimeValue t1 = gEnv->pTimer->GetAsyncTime();
					//CryLogAlways( "TestTime :%.2f", (t1-t0).GetMilliSeconds() );
					//static int nCount = 0; CryLogAlways( "Cached Hit %d",++nCount );
          hitInfo.pRenderMesh = pRenderMesh;
					return true;
				}
			}
		}
	}

	int nVerts = pRenderMesh->GetVerticesCount();
	int nInds = pRenderMesh->GetIndicesCount();

	if (nInds == 0 || nVerts == 0)
		return false;

	// get position offset and stride
	int nPosStride = 0;
	uint8* pPos = (uint8*)pRenderMesh->GetPosPtr(nPosStride, FSL_READ);

  int nUVStride = 0;
  uint8* pUV = NULL;

  int nColStride = 0;
  uint8* pCol = NULL;

  if (hitInfo.bGetVertColorAndTC)
  {
    pUV = (uint8*)pRenderMesh->GetUVPtr(nUVStride, FSL_READ);
    pCol = (uint8*)pRenderMesh->GetColorPtr(nColStride, FSL_READ);
  }

	// get indices
	uint16 *pInds = pRenderMesh->GetIndexPtr(FSL_READ);
	assert(nInds%3 == 0);

	float fMinDistance2 = FLT_MAX;

	IMaterial *pMtl = pRenderMesh->GetMaterial();
	if (pCustomMtl)
		pMtl = pCustomMtl;

	Ray inRay = hitInfo.inRay;

	bool bAnyHit = false;

	Vec3 vOut;
	Vec3 tri[3];

	// test tris
	PodArray<CRenderChunk>&	Chunks = pRenderMesh->GetChunks();
	int nChunkCount = Chunks.Count();
	for(int nChunkId = 0; nChunkId < nChunkCount; nChunkId++)
	{
		CRenderChunk *pChunk = &Chunks[nChunkId];
		if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
			continue;

		bool b2Sided = false;

    if (pMtl)
		{
			const SShaderItem &shaderItem = pMtl->GetShaderItem(pChunk->m_nMatID);
			if (hitInfo.bOnlyZWrite && !shaderItem.IsZWrite())
			  continue;
			if (!shaderItem.m_pShader || shaderItem.m_pShader->GetFlags2() & EF2_NODRAW || shaderItem.m_pShader->GetFlags() & EF_DECAL)
				continue;
			if (shaderItem.m_pShader->GetCull() & eCULL_None)
				b2Sided = true;
      if(shaderItem.m_pShaderResources && shaderItem.m_pShaderResources->GetResFlags() & MTL_FLAG_2SIDED) 
        b2Sided = true;
		}

    int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;

    if(nLastIndexId-1 >= nInds)
    {
      Error("%s (%s): invalid mesh chunk", __FUNCTION__, pRenderMesh->GetSourceName());
      return 0;
    }

		if (!b2Sided)
		{
			// make line triangle intersection
			for (int i = pChunk->nFirstIndexId; i < nLastIndexId; i+=3)
			{
				assert(pInds[i+0]<nVerts);
				assert(pInds[i+1]<nVerts);
				assert(pInds[i+2]<nVerts);

        if(pInds[i+2] >= nVerts)
        {
          Error("%s (%s): invalid mesh indices", __FUNCTION__, pRenderMesh->GetSourceName());
          return 0;
        }

        // get tri vertices
        Vec3 tv0 = (*(Vec3*)&pPos[nPosStride*pInds[i+0]]);
        Vec3 tv1 = (*(Vec3*)&pPos[nPosStride*pInds[i+1]]);
        Vec3 tv2 = (*(Vec3*)&pPos[nPosStride*pInds[i+2]]);

				if (Intersect::Ray_Triangle( inRay,tv0, tv2, tv1,vOut ))
				{
					float fDistance2 = hitInfo.inReferencePoint.GetSquaredDistance(vOut);
					if (fMaxDist2)
					{
						if (fDistance2 > fMaxDist2)
							continue; // Ignore hits that are too far.
					}

					bAnyHit = true;

					// for the fast test, we just need to know if there is an intersection.
					if (hitInfo.bInFirstHit)
					{
						vHitPos = vOut;
						hitInfo.nHitMatID = pChunk->m_nMatID;
						tri[0] = tv0; tri[1] = tv1; tri[2] = tv2;
						break;
					}
					if (fDistance2 < fMinDistance2)
					{
						fMinDistance2 = fDistance2;
						vHitPos = vOut;
						tri[0] = tv0; tri[1] = tv1; tri[2] = tv2;
						hitInfo.nHitMatID = pChunk->m_nMatID;
            if(hitInfo.bGetVertColorAndTC)
            {
              float u=0, v=0, w=0;
              if(GetBarycentricCoordinates(vHitPos-tv0, tv1-tv0, tv2-tv0, u, v, w, 16.0f))
              {
                float arrVertWeight[3] = { max(0.f,w), max(0.f,v), max(0.f,u) };
                float fDiv = 1.f/(arrVertWeight[0]+arrVertWeight[1]+arrVertWeight[2]);
                arrVertWeight[0]*=fDiv;
                arrVertWeight[1]*=fDiv;
                arrVertWeight[2]*=fDiv;

                Vec2 tc0 = ((Vec2f16*)&pUV[nUVStride*pInds[i+0]])->ToVec2();
                Vec2 tc1 = ((Vec2f16*)&pUV[nUVStride*pInds[i+1]])->ToVec2();
                Vec2 tc2 = ((Vec2f16*)&pUV[nUVStride*pInds[i+2]])->ToVec2();

                hitInfo.vHitTC = tc0*arrVertWeight[0] + tc1*arrVertWeight[1] + tc2*arrVertWeight[2];

                Vec4 c0 = (*(ColorB*)&pCol[nColStride*pInds[i+0]]).toVec4();
                Vec4 c1 = (*(ColorB*)&pCol[nColStride*pInds[i+1]]).toVec4();
                Vec4 c2 = (*(ColorB*)&pCol[nColStride*pInds[i+2]]).toVec4();

                hitInfo.vHitColor = (c0*arrVertWeight[0] + c1*arrVertWeight[1] + c2*arrVertWeight[2]) / 255.f;
              }
            }
					}
				}
			}
		}
		else
		{
			// 2 Sided
			for (int i = pChunk->nFirstIndexId; i < nLastIndexId; i+=3)
			{
				assert(pInds[i+0]<nVerts);
				assert(pInds[i+1]<nVerts);
				assert(pInds[i+2]<nVerts);

        if(pInds[i+2] >= nVerts)
        {
          Error("%s (%s): invalid mesh indices", __FUNCTION__, pRenderMesh->GetSourceName());
          return 0;
        }

        // get tri vertices
        Vec3 tv0 = (*(Vec3*)&pPos[nPosStride*pInds[i+0]]);
        Vec3 tv1 = (*(Vec3*)&pPos[nPosStride*pInds[i+1]]);
        Vec3 tv2 = (*(Vec3*)&pPos[nPosStride*pInds[i+2]]);

				if (Intersect::Ray_Triangle( inRay,tv0, tv2, tv1,vOut ))
				{
					float fDistance2 = hitInfo.inReferencePoint.GetSquaredDistance(vOut);
					if (fMaxDist2)
					{
						if (fDistance2 > fMaxDist2)
							continue; // Ignore hits that are too far.
					}

					bAnyHit = true;
					// Test front.
					if (hitInfo.bInFirstHit)
					{
						vHitPos = vOut;
						hitInfo.nHitMatID = pChunk->m_nMatID;
						tri[0] = tv0; tri[1] = tv1; tri[2] = tv2;
						break;
					}
					if (fDistance2 < fMinDistance2)
					{
						fMinDistance2 = fDistance2;
						vHitPos = vOut;
						hitInfo.nHitMatID = pChunk->m_nMatID;
						tri[0] = tv0; tri[1] = tv1; tri[2] = tv2;
					}
				}
				else if (Intersect::Ray_Triangle( inRay,tv0, tv1, tv2,vOut ))
				{
					float fDistance2 = hitInfo.inReferencePoint.GetSquaredDistance(vOut);
					if (fMaxDist2)
					{
						if (fDistance2 > fMaxDist2)
							continue; // Ignore hits that are too far.
					}

					bAnyHit = true;
					// Test back.
					if (hitInfo.bInFirstHit)
					{
						vHitPos = vOut;
						hitInfo.nHitMatID = pChunk->m_nMatID;
						tri[0] = tv0; tri[1] = tv2; tri[2] = tv1;
						break;
					}
					if (fDistance2 < fMinDistance2)
					{
						fMinDistance2 = fDistance2;
						vHitPos = vOut;
						hitInfo.nHitMatID = pChunk->m_nMatID;
						tri[0] = tv0; tri[1] = tv2; tri[2] = tv1;
					}
				}
			}
		}
	}

	if (bAnyHit)
	{ 
    hitInfo.pRenderMesh = pRenderMesh;

		// return closest to the shooter
		hitInfo.fDistance = (float)cry_sqrtf(fMinDistance2);
		hitInfo.vHitNormal = (tri[1]-tri[0]).Cross(tri[2]-tri[0]).GetNormalized();
		hitInfo.vHitPos = vHitPos;
		hitInfo.nHitSurfaceID = 0; 
		
		if(hitInfo.inRetTriangle)
		{
			hitInfo.vTri0 = tri[0];
			hitInfo.vTri1 = tri[1];
			hitInfo.vTri2 = tri[2];
		}

		if (pRenderMesh->GetMaterial())
		{
			pMtl = pRenderMesh->GetMaterial()->GetSafeSubMtl(hitInfo.nHitMatID);
			if (pMtl)
				hitInfo.nHitSurfaceID = pMtl->GetSurfaceTypeId();
		}
		
		//////////////////////////////////////////////////////////////////////////
		// Add to cached results.
		memmove( &last_hits[1],&last_hits[0],sizeof(last_hits)-sizeof(last_hits[0]) ); // Move hits to the end of array, throwing out the last one.
		last_hits[0].pRenderMesh = pRenderMesh;
		last_hits[0].hitInfo = hitInfo;
		memcpy( last_hits[0].tri,tri,sizeof(tri) );
		//////////////////////////////////////////////////////////////////////////
	}
	//CTimeValue t1 = gEnv->pTimer->GetAsyncTime();
	//CryLogAlways( "TestTime :%.2f", (t1-t0).GetMilliSeconds() );

	return bAnyHit;
}

//////////////////////////////////////////////////////////////////////////
bool CRenderMeshUtils::RayIntersectionFast( IRenderMesh *pRenderMesh, SRayHitInfo &hitInfo,IMaterial *pCustomMtl )
{
  float fBestDist = hitInfo.fMaxHitDistance; // squared distance works different for values less and more than 1.f

  Vec3 vHitPos(0,0,0);
  Vec3 vHitNormal(0,0,0);

  int nVerts = pRenderMesh->GetVerticesCount();
  int nInds = pRenderMesh->GetIndicesCount();

  if (nInds == 0 || nVerts == 0)
    return false;

  // get position offset and stride
  int nPosStride = 0;
  uint8* pPos = (uint8*)pRenderMesh->GetPosPtr(nPosStride, FSL_READ);

  int nUVStride = 0;
  uint8* pUV = (uint8*)pRenderMesh->GetUVPtr(nUVStride, FSL_READ);

  int nColStride = 0;
  uint8* pCol = (uint8*)pRenderMesh->GetColorPtr(nColStride, FSL_READ);

  // get indices
  uint16 *pInds = pRenderMesh->GetIndexPtr(FSL_READ);
  assert(nInds%3 == 0);

  IMaterial *pMtl = pRenderMesh->GetMaterial();
  if (pCustomMtl)
    pMtl = pCustomMtl;

  Ray inRay = hitInfo.inRay;

  bool bAnyHit = false;

  Vec3 vOut;
  Vec3 tri[3];

  // test tris

	Line inLine(inRay.origin, inRay.direction);

	if(hitInfo.nHitTriID >= 0)
	{
    if(hitInfo.nHitTriID+2 >= nInds)
      return false;

		int I0 = pInds[hitInfo.nHitTriID+0];
		int I1 = pInds[hitInfo.nHitTriID+1];
		int I2 = pInds[hitInfo.nHitTriID+2];

    if(I0<nVerts && I1<nVerts && I2<nVerts)
    {
      // get tri vertices
      Vec3 & tv0 = *((Vec3*)&pPos[nPosStride*I0]);
      Vec3 & tv1 = *((Vec3*)&pPos[nPosStride*I1]);
      Vec3 & tv2 = *((Vec3*)&pPos[nPosStride*I2]);

      if (Intersect::Line_Triangle( inLine, tv0, tv2, tv1, vOut ))// || Intersect::Line_Triangle( inLine, tv0, tv1, tv2, vOut ))
      {
        float fDistance = (hitInfo.inReferencePoint - vOut).GetLengthFast();

        if (fDistance < fBestDist)
        {
          bAnyHit = true;
          fBestDist = fDistance;
          vHitPos = vOut;
          tri[0] = tv0; tri[1] = tv1; tri[2] = tv2;
        }
      }
    }
	}

	if(hitInfo.nHitTriID == HIT_UNKNOWN)
	{			
		/*if(nInds < 64)
		{
			PodArray<CRenderChunk>&	Chunks = pRenderMesh->GetChunks();
			int nChunkCount = Chunks.Count();
			for(int nChunkId = 0; nChunkId < nChunkCount; nChunkId++)
			{
				CRenderChunk *pChunk = &Chunks[nChunkId];
				if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
					continue;

				bool b2Sided = false;

				if (pMtl)
				{
					const SShaderItem &shaderItem = pMtl->GetShaderItem(pChunk->m_nMatID);
					if (hitInfo.bOnlyZWrite && !shaderItem.IsZWrite())
						continue;
					if (!shaderItem.m_pShader || shaderItem.m_pShader->GetFlags2() & EF2_NODRAW || shaderItem.m_pShader->GetFlags() & EF_DECAL)
						continue;
					if (shaderItem.m_pShader->GetCull() & eCULL_None)
						b2Sided = true;
					if(shaderItem.m_pShaderResources && shaderItem.m_pShaderResources->GetResFlags() & MTL_FLAG_2SIDED) 
						b2Sided = true;
				}

				int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;

				if(nLastIndexId-1 >= nInds)
				{
					Error("%s (%s): invalid mesh chunk", __FUNCTION__, pRenderMesh->GetSourceName());
					return 0;
				}

				for (int i = pChunk->nFirstIndexId; i < nLastIndexId; i+=3)
				{
					int I0 = pInds[i+0];
					int I1 = pInds[i+1];
					int I2 = pInds[i+2];

					// get tri vertices
					Vec3 & tv0 = *((Vec3*)&pPos[nPosStride*I0]);
					Vec3 & tv1 = *((Vec3*)&pPos[nPosStride*I1]);
					Vec3 & tv2 = *((Vec3*)&pPos[nPosStride*I2]);

					if (Intersect::Line_Triangle( inLine, tv0, tv2, tv1, vOut ))// || Intersect::Line_Triangle( inLine, tv0, tv1, tv2, vOut ))
					{
						float fDistance = (hitInfo.inReferencePoint - vOut).GetLengthFast();

						if (fDistance < fBestDist)
						{
							bAnyHit = true;
							fBestDist = fDistance;
							vHitPos = vOut;
							tri[0] = tv0; tri[1] = tv1; tri[2] = tv2;
							hitInfo.nHitMatID = pChunk->m_nMatID;
							hitInfo.nHitTriID = i;
						}
					}
				}
			}
		}
		else */
			
		if(const PodArray<std::pair<int,int> > * pTris = pRenderMesh->GetTrisForPosition(inRay.origin + inRay.direction*0.5f))
		{
			for(int nId = 0; nId<pTris->Count(); ++nId)
			{
				std::pair<int,int> & t = pTris->GetAt(nId);

        if(t.first+2 >= nInds)
          return false;

				int I0 = pInds[t.first+0];
				int I1 = pInds[t.first+1];
				int I2 = pInds[t.first+2];

        if(I0>=nVerts || I1>=nVerts || I2>=nVerts)
          return false;

				// get tri vertices
				Vec3 & tv0 = *((Vec3*)&pPos[nPosStride*I0]);
				Vec3 & tv1 = *((Vec3*)&pPos[nPosStride*I1]);
				Vec3 & tv2 = *((Vec3*)&pPos[nPosStride*I2]);

				if (Intersect::Line_Triangle( inLine, tv0, tv2, tv1, vOut ))// || Intersect::Line_Triangle( inLine, tv0, tv1, tv2, vOut ))
				{
					float fDistance = (hitInfo.inReferencePoint - vOut).GetLengthFast();

					if (fDistance < fBestDist)
					{
						bAnyHit = true;
						fBestDist = fDistance;
						vHitPos = vOut;
						tri[0] = tv0; tri[1] = tv1; tri[2] = tv2;
						hitInfo.nHitMatID = t.second;
						hitInfo.nHitTriID = t.first;
					}
				}
			}
		}
	}

	if (bAnyHit)
	{ 
		hitInfo.pRenderMesh = pRenderMesh;

		// return closest to the shooter
		hitInfo.fDistance = fBestDist;
		hitInfo.vHitNormal = (tri[1]-tri[0]).Cross(tri[2]-tri[0]).GetNormalized();
		hitInfo.vHitPos = vHitPos;
		hitInfo.nHitSurfaceID = 0; 

		if (pRenderMesh->GetMaterial())
		{
			pMtl = pRenderMesh->GetMaterial()->GetSafeSubMtl(hitInfo.nHitMatID);
			if (pMtl)
				hitInfo.nHitSurfaceID = pMtl->GetSurfaceTypeId();
		}

		if(hitInfo.bGetVertColorAndTC && hitInfo.nHitTriID>=0)
		{
			int I0 = pInds[hitInfo.nHitTriID+0];
			int I1 = pInds[hitInfo.nHitTriID+1];
			int I2 = pInds[hitInfo.nHitTriID+2];

			// get tri vertices
			Vec3 & tv0 = *((Vec3*)&pPos[nPosStride*I0]);
			Vec3 & tv1 = *((Vec3*)&pPos[nPosStride*I1]);
			Vec3 & tv2 = *((Vec3*)&pPos[nPosStride*I2]);

			float u=0, v=0, w=0;
			if(GetBarycentricCoordinates(vHitPos-tv0, tv1-tv0, tv2-tv0, u, v, w, 16.0f))
			{
				float arrVertWeight[3] = { max(0.f,w), max(0.f,v), max(0.f,u) };
				float fDiv = 1.f/(arrVertWeight[0]+arrVertWeight[1]+arrVertWeight[2]);
				arrVertWeight[0]*=fDiv;
				arrVertWeight[1]*=fDiv;
				arrVertWeight[2]*=fDiv;

				Vec2 tc0 = ((Vec2f16*)&pUV[nUVStride*I0])->ToVec2();
				Vec2 tc1 = ((Vec2f16*)&pUV[nUVStride*I1])->ToVec2();
				Vec2 tc2 = ((Vec2f16*)&pUV[nUVStride*I2])->ToVec2();

				hitInfo.vHitTC = tc0*arrVertWeight[0] + tc1*arrVertWeight[1] + tc2*arrVertWeight[2];

				Vec4 c0 = (*(ColorB*)&pCol[nColStride*I0]).toVec4();
				Vec4 c1 = (*(ColorB*)&pCol[nColStride*I1]).toVec4();
				Vec4 c2 = (*(ColorB*)&pCol[nColStride*I2]).toVec4();

				// get tangent basis
				int nTangStride=0;
				int nBNormStride=0;
				byte * pTang = 0;
				byte * pBNorm = 0;
				pTang = pRenderMesh->GetTangentPtr(nTangStride, FSL_READ);
				pBNorm = pRenderMesh->GetBinormalPtr(nBNormStride, FSL_READ);

				Vec4 tangent[3];
				Vec4 binormal[3];
				int arrId[3] = {I0,I1,I2};
				for(int ii=0; ii<3; ii++)
				{
					Vec4sf & Tangent = *(Vec4sf*)&pTang[nTangStride*arrId[ii]];               
					Vec4sf & Binormal = *(Vec4sf*)&pBNorm[nBNormStride*arrId[ii]];
					tangent[ii]	= Vec4((Tangent.x), (Tangent.y), (Tangent.z), (Tangent.w));
					binormal[ii]	= Vec4((Binormal.x), (Binormal.y), (Binormal.z), (Binormal.w));
				}

				hitInfo.vHitTangent = (tangent[0]*arrVertWeight[0] + tangent[1]*arrVertWeight[1] + tangent[2]*arrVertWeight[2]) / 32767.0f;
				hitInfo.vHitBinormal = (binormal[0]*arrVertWeight[0] + binormal[1]*arrVertWeight[1] + binormal[2]*arrVertWeight[2]) / 32767.0f;

				hitInfo.vHitColor = (c0*arrVertWeight[0] + c1*arrVertWeight[1] + c2*arrVertWeight[2]) / 255.f;			
			}
		}
	}

  //CTimeValue t1 = gEnv->pTimer->GetAsyncTime();
  //CryLogAlways( "TestTime :%.2f", (t1-t0).GetMilliSeconds() );

  return bAnyHit;
}
