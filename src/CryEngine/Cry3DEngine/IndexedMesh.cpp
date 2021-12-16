////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   meshidx.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: prepare shaders for cfg object
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "IndexedMesh.h"
#include "VoxTerrain.h"
#include "DecalRenderNode.h"
#include "Brush.h"
#include "MeshCompiler/MeshCompiler.h"

#define MAX_USHORT (65536-2)

namespace
{
  CryCriticalSection g_cIndexedMeshCounter;
}

CIndexedMesh::CIndexedMesh()
{
	m_pFaces=0;
	m_pPositions=0;
	m_pTexCoord=0;
	m_pNorms=0;
	m_pColor0=m_pColor1=0;	
	m_pVertMats=0;
	m_numFaces=0;
	m_numVertices=0;
	m_nCoorCount=0;
	m_bInvalidated = false;

  {
    AUTO_LOCK(g_cIndexedMeshCounter);
    GetCounter()->Add(this);
  }
}

CIndexedMesh::~CIndexedMesh()
{
  {
    AUTO_LOCK(g_cIndexedMeshCounter);
    GetCounter()->Delete(this);
  }
}

size_t CIndexedMesh::GetMemoryUsage()
{
  return CMesh::GetMemoryUsage();
}

struct TriEdge
{
	TriEdge() { memset(this,0,sizeof(*this)); }
	bool IsRemoved() { return (fError<0); }
	void SetRemoved() { fError=-1; }

	float fError;
	union{
		uint16 arrVertId[2];
		uint32 nVertIds;
	};
	uint16 arrFaces[2];
};

struct VertInfo
{
  PodArray<int> lstFacesIds;
  PodArray<int> lstVertIds;
};

int __cdecl CIndexedMesh__Cmp_TriEdge_Errors(const void* v1, const void* v2)
{
	TriEdge * p1 = (TriEdge*)v1;
	TriEdge * p2 = (TriEdge*)v2;

	if(p1->fError>p2->fError)
		return 1;

	if(p1->fError<p2->fError)
		return -1;

	return 0;
}

int __cdecl CIndexedMesh__Cmp_SGroupInfo(const void* v1, const void* v2)
{
  SGroupInfo *p1 = (SGroupInfo*)v1;
  SGroupInfo *p2 = (SGroupInfo*)v2;

  if(p1->aabb.GetRadius() > p2->aabb.GetRadius())
    return -1;

  if(p1->aabb.GetRadius() < p2->aabb.GetRadius())
    return 1;

  return 0;
}

int __cdecl CIndexedMesh__Cmp_Int(const void* v1, const void* v2)
{
	int p1 = *(int*)v1;
	int p2 = *(int*)v2;

	if(p1 > p2)
		return -1;

	if(p1 < p2)
		return 1;

	return 0;
}

void CIndexedMesh::ShareVertices(ISystem * pSystem, const AABB & boxBoundary)
{
  FUNCTION_PROFILER( pSystem, PROFILE_3DENGINE );

  // make version of mesh without duplicated vertices
  std::vector<int> lstIndices;
  lstIndices.reserve( m_numFaces*3 );
  Vec3				*pVertsShared  = (Vec3 *)			 calloc(m_numVertices,sizeof(Vec3));
  SMeshTexCoord*pCoordShared = m_pTexCoord ? (SMeshTexCoord*)calloc(m_numVertices,sizeof(SMeshTexCoord)) : NULL;
  Vec3				*pNormsShared	 = (Vec3 *)			 calloc(m_numVertices,sizeof(Vec3));
  SMeshColor	*pColorShared0 = (SMeshColor *)calloc(m_numVertices,sizeof(SMeshColor));
  SMeshColor	*pColorShared1 = (SMeshColor *)calloc(m_numVertices,sizeof(SMeshColor));
  int					*pVertMatsShared = m_pVertMats ? (int*)calloc(m_numVertices,sizeof(int)) : NULL;

  for (int i=0; i<m_numFaces; i++) for (int v=0; v<3; v++)
  {
    lstIndices.push_back(m_pFaces[i].v[v]);
    assert(m_pFaces[i].v[v] == m_pFaces[i].t[v]);
    assert(m_pFaces[i].v[v] == m_pFaces[i].v[v]);
  }
  memcpy(pVertsShared, m_pPositions, sizeof(Vec3)*m_numVertices);
  int nVertCountShared = m_numVertices;

  // complete mesh 
  mesh_compiler::CMeshCompiler meshCompiler;
  meshCompiler.WeldPositions( pVertsShared, nVertCountShared, lstIndices, boxBoundary.GetSize().x*0.001f, boxBoundary );

  for(uint32 i=0; i<lstIndices.size(); i+=3)
  {
    if(	lstIndices[i+0] == lstIndices[i+1] || lstIndices[i+1] == lstIndices[i+2] || lstIndices[i+2] == lstIndices[i+0])
    { // degenerative triangle
      std::vector<int>::iterator remove_first = lstIndices.begin()+i;
      std::vector<int>::iterator remove_last = lstIndices.begin()+i+3;
      lstIndices.erase(remove_first,remove_last);
      int nElemId = i/3;
      memmove(&(m_pFaces[nElemId]), &(m_pFaces[nElemId+1]), sizeof(m_pFaces[0])*(m_numFaces-nElemId-1));
      i-=3;
      continue;
    }
  }

  assert(pNormsShared && pColorShared0 && pColorShared1 && pVertMatsShared);

  for(uint32 i=0; i<lstIndices.size(); i++)
  {
    int nOldVertId = m_pFaces[i/3].v[i-i/3*3];
    assert(IsEquivalent(pVertsShared[lstIndices[i]], m_pPositions[nOldVertId], boxBoundary.GetSize().x/128.f));
    if(m_pTexCoord && pCoordShared)
      pCoordShared[lstIndices[i]] = m_pTexCoord[m_pFaces[i/3].t[i-i/3*3]];
    pNormsShared[lstIndices[i]] += m_pNorms[nOldVertId];
    pColorShared0[lstIndices[i]] = m_pColor0[nOldVertId];
    pColorShared1[lstIndices[i]] = m_pColor1[nOldVertId];
    if (m_pVertMats)
      pVertMatsShared[lstIndices[i]] = m_pVertMats[nOldVertId];
  }

  for(int v=0; v<nVertCountShared; v++)
    pNormsShared[v].normalize();

  // rebuild mesh
  RebuildMesh(pVertsShared, pCoordShared, pNormsShared, 
    pColorShared0, 
    pColorShared1, 
    lstIndices, nVertCountShared, pVertMatsShared);

  // free memory
  free(pVertsShared);
  free(pCoordShared);
  free(pNormsShared);

  free(pColorShared0);
  free(pColorShared1);
  free(pVertMatsShared);
}

void CIndexedMesh::ShareVerticesTC(ISystem * pSystem, const AABB & boxBoundary)
{
	FUNCTION_PROFILER( pSystem, PROFILE_3DENGINE );

  // make version of mesh without duplicated vertices
	std::vector<int> lstIndices;
	lstIndices.reserve( m_numFaces*3 ); 
  int nMax = max(max(m_numFaces*3, m_numVertices),m_nCoorCount);
	Vec3				*pVertsShared  = (Vec3 *)			  calloc(nMax,sizeof(Vec3));
	SMeshTexCoord*pCoordShared = (SMeshTexCoord*)calloc(nMax,sizeof(SMeshTexCoord));
	Vec3				*pNormsShared	 = (Vec3 *)			  calloc(nMax,sizeof(Vec3));
	SMeshColor	*pColorShared0 = (SMeshColor *) calloc(nMax,sizeof(SMeshColor));
	SMeshColor	*pColorShared1 = (SMeshColor *) calloc(nMax,sizeof(SMeshColor));
	int					*pVertMatsShared =        (int*)calloc(nMax,sizeof(int));

	memcpy(pVertsShared, m_pPositions, sizeof(Vec3)*m_numVertices);
	int nVertCountShared = m_numVertices;

  memcpy(pCoordShared, m_pTexCoord, sizeof(SMeshTexCoord)*m_nCoorCount);
  int nTCCountShared = m_nCoorCount;

	// complete mesh 
	WeldPositionsTC( pVertsShared, nVertCountShared, 
    pCoordShared, nTCCountShared,
    lstIndices, 0.01f, boxBoundary );

	for(uint32 i=0; i<lstIndices.size(); i+=3)
	{
		if(	lstIndices[i+0] == lstIndices[i+1] || lstIndices[i+1] == lstIndices[i+2] || lstIndices[i+2] == lstIndices[i+0])
		{ // degenerative triangle
			std::vector<int>::iterator remove_first = lstIndices.begin()+i;
			std::vector<int>::iterator remove_last = lstIndices.begin()+i+3;
			lstIndices.erase(remove_first,remove_last);
			int nElemId = i/3;
			memmove(&(m_pFaces[nElemId]), &(m_pFaces[nElemId+1]), sizeof(m_pFaces[0])*(m_numFaces-nElemId-1));
			i-=3;
			continue;
		}
	}

  assert(pNormsShared && pColorShared0 && pColorShared1 && pVertMatsShared);

	for(uint32 i=0; i<lstIndices.size(); i++)
	{
		int nOldVertId = m_pFaces[i/3].v[i-i/3*3];
		assert(IsEquivalent(pVertsShared[lstIndices[i]], m_pPositions[nOldVertId], 0.5f));
		if(m_pTexCoord && pCoordShared)
			assert(pCoordShared[lstIndices[i]] == m_pTexCoord[m_pFaces[i/3].t[i-i/3*3]]);
		pNormsShared[lstIndices[i]] += m_pNorms[nOldVertId];
		pColorShared0[lstIndices[i]] = m_pColor0[nOldVertId];
		pColorShared1[lstIndices[i]] = m_pColor1[nOldVertId];
		if (m_pVertMats)
			pVertMatsShared[lstIndices[i]] = m_pVertMats[nOldVertId];
	}

	for(int v=0; v<nVertCountShared; v++)
		pNormsShared[v].normalize();

	// rebuild mesh
	RebuildMesh(pVertsShared, pCoordShared, pNormsShared, 
		pColorShared0, 
		pColorShared1, 
		lstIndices, nVertCountShared, pVertMatsShared);

  IsHeapValid();

	// free memory
	free(pVertsShared);
	free(pCoordShared);
	free(pNormsShared);

	free(pColorShared0);
	free(pColorShared1);
	free(pVertMatsShared);
}

void CIndexedMesh::MarkSharpVertices(float fMaxDot, CMesh * pMesh)
{
  return;
/*
  void* pStream; 
  int nElementSize;
  pMesh->GetStreamInfo(CMesh::POSITIONS,pStream,nElementSize);
  Vec3 * pPos = (Vec3 *)pStream;
  pMesh->GetStreamInfo(CMesh::NORMALS,pStream,nElementSize);
  Vec3 * pNor = (Vec3 *)pStream;
  pMesh->GetStreamInfo(CMesh::INDICES,pStream,nElementSize);
  uint16 * pInd = (uint16 *)pStream;
  pMesh->GetStreamInfo(CMesh::COLORS_1,pStream,nElementSize);
  SMeshColor * pCol = (SMeshColor *)pStream;

  for(int i=0; i<pMesh->GetVertexCount(); i++)
    pCol[i].r = 0;

  for(int i=0; i<pMesh->GetIndexCount(); i+=3)
  {
    Vec3 v0 = pPos[pInd[i+0]];
    Vec3 v1 = pPos[pInd[i+1]];
    Vec3 v2 = pPos[pInd[i+2]];
    Vec3 vFaceNormal = -(v0-v1).Cross(v2-v1);
    vFaceNormal.Normalize();

    for(int v=0; v<3; v++)
    {
      Vec3 vNor = pNor[i+v];
      if(vNor.Dot(vFaceNormal)<fMaxDot)
      {
        pCol[i+v].r = 255;
      }
    }
  }
	*/
}

static inline bool IsEquivalentVec3dCheckYFirst(const Vec3 & v0, const Vec3 & v1, float fEpsilon)
{
  if(fabsf(v0.y-v1.y)<fEpsilon)
    if(fabsf(v0.x-v1.x)<fEpsilon)
      if(fabsf(v0.z-v1.z)<fEpsilon)
        return true;
  return false;
}

inline int FindInPosBufferTC(const Vec3 & vPosToFind, const SMeshTexCoord & vTCToFind, 
                             const Vec3 * pVertBuff, const SMeshTexCoord * pTCBuff,
                             std::vector<int> *pHash,float fEpsilon )
{ 
  for(uint32 i=0,num = pHash->size(); i<num; i++) 
    if(IsEquivalentVec3dCheckYFirst(pVertBuff[(*pHash)[i]], vPosToFind, fEpsilon)) 
      if(pTCBuff[(*pHash)[i]].t == vTCToFind.t && pTCBuff[(*pHash)[i]].s == vTCToFind.s) 
        return (*pHash)[i];

  return -1;
}

void CIndexedMesh::WeldPositionsTC( Vec3 *pVerts, int &nVerts, 
                                    SMeshTexCoord * pTC, int &nTC,
                                    std::vector<int> &indices_out,float fEpsilon, const AABB & boxBoundary )
{
  int nMax = max(max(m_numFaces*3, nVerts),nTC);
  Vec3 *pTmpVerts = new Vec3[nMax];
  SMeshTexCoord *pTmpTC = new SMeshTexCoord[nMax];

  int nCurVertex = 0;

  std::vector<int> arrHashTable[256];

  std::vector<int> newIndices;
  newIndices.reserve( m_numFaces*3 );

  float fHashElemSize = 256.0f / max(boxBoundary.max.x - boxBoundary.min.x, 0.01f);

  for(int nFace=0; nFace < m_numFaces; nFace++) for(int nV=0; nV<3; nV++)
  {
    int v = m_pFaces[nFace].v[nV];
    int t = m_pFaces[nFace].t[nV];

    assert(v<nVerts);
    assert(t<nTC);

    unsigned char nHashValue = (unsigned char)((pVerts[v].x - boxBoundary.min.x)*fHashElemSize);

    Vec3 & vPos = pVerts[v];
    SMeshTexCoord & vTC = pTC[t];
/*
    bool bInRange(
      vPos.x>boxBoundary.min.x && 
      vPos.y>boxBoundary.min.y && 
      vPos.z>boxBoundary.min.z && 
      vPos.x<boxBoundary.max.x && 
      vPos.y<boxBoundary.max.y && 
      vPos.z<boxBoundary.max.z);
*/
    int find = FindInPosBufferTC( 
      vPos, vTC,
      pTmpVerts, pTmpTC, 
      &arrHashTable[nHashValue], /*bInRange ? fEpsilon : */0.01f);

    if(find<0)
    {
      arrHashTable[nHashValue].push_back(nCurVertex);
      pTmpVerts[nCurVertex] = vPos;
      pTmpTC[nCurVertex] = vTC;
      newIndices.push_back(nCurVertex);
      nCurVertex++;
    }
    else
    {
      newIndices.push_back(find);
    }
  }

  indices_out = newIndices;

  nVerts = nCurVertex;
  memcpy( pVerts, pTmpVerts, nCurVertex*sizeof(Vec3) );

  nTC = nCurVertex;
  memcpy( pTC, pTmpTC, nCurVertex*sizeof(SMeshTexCoord) );

  delete []pTmpVerts;
  delete []pTmpTC;
}

void CIndexedMesh::SimplifyMesh(float fMaxEdgeLen, const AABB & boxBoundary, float fSurfTypeRatio )
{
	// make version of mesh without duplicated vertices
	std::vector<int> lstIndices;
	lstIndices.reserve( m_numFaces*3 );
	Vec3		*pVertsShared = (Vec3 *)calloc(m_numVertices, sizeof(Vec3));
	SMeshTexCoord*pCoordShared = m_pTexCoord ? (SMeshTexCoord*)calloc(m_numVertices, sizeof(SMeshTexCoord)) : NULL;
	Vec3		*pNormsShared = (Vec3 *)calloc(m_numVertices, sizeof(Vec3));
	SMeshColor	*pColorShared0 = (SMeshColor *)calloc(m_numVertices, sizeof(SMeshColor));
	SMeshColor	*pColorShared1 = (SMeshColor *)calloc(m_numVertices, sizeof(SMeshColor));
  int		*pVertMatsShared = m_pVertMats ? (int *)calloc(m_numVertices, sizeof(int)) : NULL;
	for (int i=0; i<m_numFaces; i++) for (int v=0; v<3; v++)
	{
		lstIndices.push_back(m_pFaces[i].v[v]);
		assert(!pCoordShared || m_pFaces[i].v[v] == m_pFaces[i].t[v]);
		assert(m_pFaces[i].v[v] == m_pFaces[i].v[v]);
	}
	memcpy(pVertsShared, m_pPositions, sizeof(Vec3)*m_numVertices);
	int nVertCountShared = m_numVertices;

	mesh_compiler::CMeshCompiler meshCompiler;
	meshCompiler.WeldPositions( pVertsShared, nVertCountShared, lstIndices, VEC_EPSILON, boxBoundary );

	for(uint32 i=0; i<lstIndices.size(); i+=3)
	{
		if(	lstIndices[i+0] == lstIndices[i+1] || lstIndices[i+1] == lstIndices[i+2] || lstIndices[i+2] == lstIndices[i+0])
		{ // degenerative triangle		
			std::vector<int>::iterator remove_first = lstIndices.begin()+i;
			std::vector<int>::iterator remove_last = lstIndices.begin()+i+3;
			lstIndices.erase(remove_first,remove_last);
			int nElemId = i/3;
			memmove(&(m_pFaces[nElemId]), &(m_pFaces[nElemId+1]), sizeof(m_pFaces[0])*(m_numFaces-nElemId-1));
			i-=3;
			continue;
		}
	}

	// complete mesh and also make vert info: for each unique vertex store list of connected faces ids
	VertInfo * pVertsInfo = (VertInfo*)calloc(m_numVertices, sizeof(VertInfo));
	memset(pVertsInfo, 0, sizeof(VertInfo)*m_numVertices);
	for(uint32 i=0; i<lstIndices.size(); i++)
	{
		if(pVertsInfo[lstIndices[i]].lstFacesIds.Find(i/3)<0)
			pVertsInfo[lstIndices[i]].lstFacesIds.Add(i/3);

    int nVertId = m_pFaces[i/3].v[i-i/3*3];
		assert(IsEquivalent(pVertsShared[lstIndices[i]], m_pPositions[nVertId], VEC_EPSILON));
		if(m_pTexCoord && pCoordShared)
			pCoordShared[lstIndices[i]] = m_pTexCoord[m_pFaces[i/3].t[i-i/3*3]];
		pNormsShared[lstIndices[i]] = m_pNorms[nVertId];
		pColorShared0[lstIndices[i]] = m_pColor0[nVertId];
		pColorShared1[lstIndices[i]] = m_pColor1[nVertId];
    if(pVertMatsShared && m_pVertMats)
      pVertMatsShared[lstIndices[i]] = m_pVertMats[nVertId];
	}

	// make list of all edges
	static PodArray<TriEdge> lstTriEdges; lstTriEdges.Clear();
	int nDeleted=0;
	for(uint32 i=0; i<lstIndices.size(); i+=3)
	{
		if(	lstIndices[i+0] == lstIndices[i+1] ||
				lstIndices[i+1] == lstIndices[i+2] ||
				lstIndices[i+2] == lstIndices[i+0])
		{ // degenerative triangle
			lstIndices[i+0] = -1;
			lstIndices[i+1] = -1;
			lstIndices[i+2] = -1;
			assert(!"Degenerative triangle found during edges list creation");
			continue;
		}

		for(int nE=0; nE<3; nE++)
		{
			TriEdge e;
			// store edge vertices id
			assert(lstIndices[i+(nE+0)%3]<MAX_USHORT);
			assert(lstIndices[i+(nE+1)%3]<MAX_USHORT);
			e.arrVertId[0] = lstIndices[i+(nE+0)%3];
			e.arrVertId[1] = lstIndices[i+(nE+1)%3];		
			if(e.arrVertId[0]>e.arrVertId[1])
			{
				int tmp = e.arrVertId[0];
				e.arrVertId[0] = e.arrVertId[1];
				e.arrVertId[1] = tmp;
			}
			assert(e.arrVertId[0] != e.arrVertId[1]);

			// todo: do not add edges connected to boundary edges

			// find faces connected to both vertices at the same time
			if(InitEdgeFaces(e, pVertsInfo, pVertsShared, NULL, boxBoundary))
			{
				e.fError = GetEdgeError(e, pVertsShared, pVertsInfo, lstIndices, pColorShared0, pColorShared1, fSurfTypeRatio);
				if(e.fError < fMaxEdgeLen) // do not process edges with too big error
					lstTriEdges.Add(e);
				else
					nDeleted++;
			}
		}
	}

	// remove duplicated edges
	RemoveDuplicatedEdges(lstTriEdges);

	// start to remove edges
	int nPass=0;
	while(lstTriEdges.Count())
	{
		// find next edge for removing
		int nLowestId=0;
		float fLowestValue=100000;
		for(int i=0; i<lstTriEdges.Count(); i++)
		{
			if(lstTriEdges[i].fError<fLowestValue)
			{
				nLowestId = i;
				fLowestValue = lstTriEdges[i].fError;
			}
		}

		// continue until no more edges with error lower than specified
		if(lstTriEdges[nLowestId].fError>fMaxEdgeLen)
			break;

		// remove zero edge and 2 triangles
		RemoveEdge(pVertsShared, pVertsInfo, pCoordShared, pNormsShared, 
			pColorShared0, pColorShared1, pVertMatsShared,
			lstIndices, nVertCountShared,lstTriEdges, nLowestId, boxBoundary, fSurfTypeRatio);

		nPass++;
	}

	// rebuild mesh
	RebuildMesh(pVertsShared, pCoordShared, pNormsShared, 
		pColorShared0, pColorShared1, 
		lstIndices, nVertCountShared, pVertMatsShared);

	// free memory
	for(int i=0; i<nVertCountShared; i++)
		pVertsInfo[i].lstFacesIds.Reset();
	free(pVertsInfo);
	free(pVertsShared);
	free(pCoordShared);
	free(pNormsShared);

	free(pColorShared0);
	free(pColorShared1);
  free(pVertMatsShared);
}

bool CIndexedMesh::InitEdgeFaces(TriEdge & e, VertInfo * pVertsInfo, Vec3 * pVertsShared, TriEdge * pe0, const AABB & boxBoundary)
{
	// find faces connected to both vertices at the same time
	PodArray<int> & lstFacesIds0 = pVertsInfo[e.arrVertId[0]].lstFacesIds;
	PodArray<int> & lstFacesIds1 = pVertsInfo[e.arrVertId[1]].lstFacesIds;

	assert(!pe0 || lstFacesIds0.Find(pe0->arrFaces[0])<0);
	assert(!pe0 || lstFacesIds0.Find(pe0->arrFaces[1])<0);
	assert(!pe0 || lstFacesIds1.Find(pe0->arrFaces[0])<0);
	assert(!pe0 || lstFacesIds1.Find(pe0->arrFaces[1])<0);

	int nFoundFaces=0;
	for(int f=0; f<lstFacesIds0.Count() && nFoundFaces<=2; f++)
		if(lstFacesIds1.Find(lstFacesIds0[f])>=0)
		{
			assert(lstFacesIds0[f]<MAX_USHORT);
			if(nFoundFaces<2)
				e.arrFaces[nFoundFaces] = lstFacesIds0[f];
			nFoundFaces++;
		}

		if(nFoundFaces == 2)
		{ // take only nice edges between 2 triangles and not on object borders
			bool bBorder=0;
			for(int t=0; t<2; t++)
			{
				if(	pVertsShared[e.arrVertId[t]].x <= boxBoundary.min.x || 
						pVertsShared[e.arrVertId[t]].y <= boxBoundary.min.y || 
						pVertsShared[e.arrVertId[t]].z <= boxBoundary.min.z ||
						pVertsShared[e.arrVertId[t]].x >= boxBoundary.max.x || 
						pVertsShared[e.arrVertId[t]].y >= boxBoundary.max.y || 
						pVertsShared[e.arrVertId[t]].z >= boxBoundary.max.z )
				{ 
					bBorder=true; 
					break; 
				}
			}

			if(!bBorder)
				return true;
		}

		return false;
}

void CIndexedMesh::RemoveDuplicatedEdges(PodArray<struct TriEdge> & lstTriEdges)
{
	static PodArray<TriEdge*> lstHashTable[256];
	for(int i=0; i<256; i++)
		lstHashTable[i].Clear();

	// fill hash table
	for(int i=0; i<lstTriEdges.Count(); i++)
	{
		TriEdge & ei = lstTriEdges[i];
		lstHashTable[uint8(ei.nVertIds)].Add(&lstTriEdges[i]);
	}

	// search duplicates using hash
	for(int i=0; i<lstTriEdges.Count(); i++)
	{
		TriEdge & ei = lstTriEdges[i];
		PodArray<TriEdge*> & HashList = lstHashTable[uint8(ei.nVertIds)];

		int nFoundNum = 0;
		for(int n=0; n<HashList.Count(); n++)
		{
			TriEdge & en = *HashList[n];

			if(&en<=&ei)
				continue;

			if(ei.nVertIds == en.nVertIds)
			{ // if vertex ids are the same - delete second copy
				en.SetRemoved(); // mark as deleted
				nFoundNum++;
			}
		}
		assert(nFoundNum <= 1);
	}

	// delete edges marked for delete
	for(int i=0; i<lstTriEdges.Count(); i++)
	{
		if(lstTriEdges[i].IsRemoved())
		{
			lstTriEdges.DeleteFastUnsorted(i);
			i--;
		}
	}
}

bool CIndexedMesh::TestEdges(int nBeginWith, PodArray<TriEdge> & lstTriEdges, std::vector<int> & lstIndices, TriEdge * pEdgeForDelete)
{ // check: edges should not point to deleted faces
	for(int i=nBeginWith; i<lstTriEdges.Count(); i++)
	{
		//		assert(!pEdgeForDelete || pEdgeForDelete == &lstTriEdges[0]);
		TriEdge & e = lstTriEdges[i];
		for(int l=0; l<2; l++) for(int v=0; v<3; v++)
		{

			if(pEdgeForDelete)
			{
				if(pEdgeForDelete->arrFaces[0] == e.arrFaces[l] || pEdgeForDelete->arrFaces[1] == e.arrFaces[l])
					return false;
			}

			int nId = lstIndices[e.arrFaces[l]*3+v];
			if(nId<0)
				return false;
		}
	}

	return true;
}


void CIndexedMesh::MergeVertexFaceLists(int v0, int v1, struct VertInfo * pVertsInfo)
{
	VertInfo & vi0 = pVertsInfo[v0];
	VertInfo & vi1 = pVertsInfo[v1];

	vi1.lstFacesIds.AddList(vi0.lstFacesIds);
	vi0.lstFacesIds.Clear();
	vi0.lstFacesIds.AddList(vi1.lstFacesIds);

#ifdef _DEBUG
	for(int i=0; i<vi1.lstFacesIds.Count(); i++)
		for(int j=i+1; j<vi1.lstFacesIds.Count(); j++)
		{
			if(vi1.lstFacesIds[i] == vi1.lstFacesIds[j])
				assert(!"Duplicated faces found in merged list of vertex faces");
		}
#endif
}

void CIndexedMesh::MergeVertexComponents(TriEdge & e0, Vec3 * pVertsShared, struct VertInfo * pVertsInfo, 
																				 SMeshTexCoord * pCoordShared,Vec3 * pNormsShared, 
																				 SMeshColor * pColorShared0, SMeshColor * pColorShared1, 
                                         int * pVertMatsShared)
{
	// position
	Vec3 vNewPos = (pVertsShared[e0.arrVertId[0]]+pVertsShared[e0.arrVertId[1]])*0.5f;

	// normal
	Vec3 vNewNorm = (pNormsShared[e0.arrVertId[0]]+pNormsShared[e0.arrVertId[1]]).normalized();

	// texcoords
	SMeshTexCoord tcNewCoords;
	if(pCoordShared)
	{
		tcNewCoords.s = (pCoordShared[e0.arrVertId[0]].s+pCoordShared[e0.arrVertId[1]].s)*0.5f;
		tcNewCoords.t = (pCoordShared[e0.arrVertId[0]].t+pCoordShared[e0.arrVertId[1]].t)*0.5f;
	}

	// color0
	SMeshColor ucNewColor0;
	ucNewColor0.r = uint8(0.5f*pColorShared0[e0.arrVertId[0]].r + 0.5f*pColorShared0[e0.arrVertId[1]].r);
//	ucNewColor0.g = uint8(0.5f*pColorShared0[e0.arrVertId[0]].g + 0.5f*pColorShared0[e0.arrVertId[1]].g);
	ucNewColor0.g = pColorShared0[e0.arrVertId[0]].g; // material id and base color source
	ucNewColor0.b = uint8(0.5f*pColorShared0[e0.arrVertId[0]].b + 0.5f*pColorShared0[e0.arrVertId[1]].b);
	ucNewColor0.a = uint8(0.5f*pColorShared0[e0.arrVertId[0]].a + 0.5f*pColorShared0[e0.arrVertId[1]].a);

  // color1
  SMeshColor ucNewColor1;
  ucNewColor1.r = uint8(0.5f*pColorShared1[e0.arrVertId[0]].r + 0.5f*pColorShared1[e0.arrVertId[1]].r);
  ucNewColor1.g = uint8(0.5f*pColorShared1[e0.arrVertId[0]].g + 0.5f*pColorShared1[e0.arrVertId[1]].g);
  ucNewColor1.b = uint8(0.5f*pColorShared1[e0.arrVertId[0]].b + 0.5f*pColorShared1[e0.arrVertId[1]].b);
  ucNewColor1.a = uint8(0.5f*pColorShared1[e0.arrVertId[0]].a + 0.5f*pColorShared1[e0.arrVertId[1]].a);

	// update shared verts pos, vertex 0 should not be used in the end
	pVertsShared[e0.arrVertId[0]] = vNewPos;//+Vec3(0,0,8)*float(gEnv->pRenderer->GetFrameID()&1);
	pVertsShared[e0.arrVertId[1]] = vNewPos;

	// normals
	pNormsShared[e0.arrVertId[0]] = vNewNorm;
	pNormsShared[e0.arrVertId[1]] = vNewNorm;

  // vert mat id
  pVertMatsShared[e0.arrVertId[0]] = pVertMatsShared[e0.arrVertId[1]];

	// texcoords
	if(pCoordShared)
	{
		pCoordShared[e0.arrVertId[0]].s = tcNewCoords.s;
		pCoordShared[e0.arrVertId[0]].t = tcNewCoords.t;
		pCoordShared[e0.arrVertId[1]].s = tcNewCoords.s;
		pCoordShared[e0.arrVertId[1]].t = tcNewCoords.t;
	}

	// color0
	pColorShared0[e0.arrVertId[0]] = ucNewColor0;
	pColorShared0[e0.arrVertId[1]] = ucNewColor0;

	// color1
	pColorShared1[e0.arrVertId[0]] = ucNewColor1;
	pColorShared1[e0.arrVertId[1]] = ucNewColor1;
}

bool CIndexedMesh::RemoveEdge(Vec3 * pVertsShared, struct VertInfo * pVertsInfo, SMeshTexCoord * pCoordShared,Vec3 * pNormsShared, 
															SMeshColor * pColorShared0, SMeshColor * pColorShared1, int * pVertMatsShared,
															std::vector<int> & lstIndices, int nVertCountShared,
															PodArray<struct TriEdge> & lstTriEdges, int nEdgeForRemoveId, const AABB & boxBoundary,
                              float fSurfTypeRatio)
{
	int nEdgesNumBefore = lstTriEdges.Count();
	// we will remove edge 0
	TriEdge e0 = lstTriEdges[nEdgeForRemoveId];
	lstTriEdges.Delete(nEdgeForRemoveId);

	assert(e0.arrVertId[0] != e0.arrVertId[1]);

	// move both vertices into new position
	MergeVertexComponents(e0, pVertsShared, pVertsInfo, pCoordShared, pNormsShared, pColorShared0, pColorShared1, pVertMatsShared);

	// Make list of involved vertices
	static PodArray<int> lstInvolvedVerts; lstInvolvedVerts.Clear();
	for(int l=0; l<2; l++) for(int v=0; v<3; v++)
	{
		int nId = lstIndices[e0.arrFaces[l]*3+v];
		assert(nId>=0);
		if(nId>=0 && lstInvolvedVerts.Find(nId)<0)
			lstInvolvedVerts.Add(nId);
	}

	assert(lstInvolvedVerts.Find(e0.arrVertId[0])>=0);
	assert(lstInvolvedVerts.Find(e0.arrVertId[1])>=0);

	/*
	if(lstTriEdges.Count()==489)
	for(int i=0; i<lstInvolvedVerts.Count(); i++)
	{
	PodArray<int> & lstFacesIds = pVertsInfo[lstInvolvedVerts[i]].lstFacesIds;
	for(int f=0; f<lstFacesIds.Count(); f++) 
	{
	for(int v=0; v<3; v++)
	{
	Vec3 vPos = pVertsShared[lstIndices[lstFacesIds[f]*3+v]];
	gEnv->pRenderer->DrawLabel(vPos+m_vOrigin,2,"%d",lstIndices[lstFacesIds[f]*3+v]);
	}
	}
	}

	if(lstTriEdges.Count()==489)
	{
	lstTriEdges.Clear();
	return false;
	}
	*/

	// remove references to deleted faces from pVertInfo and redirect indices
	for(int i=0; i<lstInvolvedVerts.Count(); i++)
	{
		PodArray<int> & lstFacesIds = pVertsInfo[lstInvolvedVerts[i]].lstFacesIds;
		for(int f=0; f<lstFacesIds.Count(); f++) 
		{
			if(lstFacesIds[f] == e0.arrFaces[0] || lstFacesIds[f] == e0.arrFaces[1])
			{
				lstFacesIds.Delete(f,1); f--;
				continue;
			}

			for(int v=0; v<3; v++)
			{
				if(lstIndices[lstFacesIds[f]*3+v] == e0.arrVertId[0])
					lstIndices[lstFacesIds[f]*3+v] = e0.arrVertId[1];
			}
		}
	}

#ifdef _DEBUG
	// check no references to deleted faces from pVertInfo
	for(int i=0; i<nVertCountShared; i++)
	{
		VertInfo & vi =	pVertsInfo[i];
		Vec3 vPos = pVertsShared[i];
		assert(vi.lstFacesIds.Find(e0.arrFaces[0])<0);
		assert(vi.lstFacesIds.Find(e0.arrFaces[1])<0);
	}
#endif

	// combine face lists of merged vertices
	MergeVertexFaceLists(e0.arrVertId[0], e0.arrVertId[1], pVertsInfo);

	// redirect edges to new vertex, fix edge face id
	static PodArray<int> lstInvolvedEdges; lstInvolvedEdges.Clear();
	for(int nEdge=0; nEdge<lstTriEdges.Count(); nEdge++)
	{
		TriEdge & e = lstTriEdges[nEdge];
		assert(e.arrVertId[0] != e.arrVertId[1]);
		bool bUpdate = false;

		if(e.arrVertId[0] == e0.arrVertId[0] || e.arrVertId[0] == e0.arrVertId[1])
		{
			e.arrVertId[0] = e0.arrVertId[1];
			bUpdate = true;
		}

		if(e.arrVertId[1] == e0.arrVertId[0] || e.arrVertId[1] == e0.arrVertId[1])
		{
			e.arrVertId[1] = e0.arrVertId[1];
			bUpdate = true;
		}

		if(
			e.arrFaces[0] == e0.arrFaces[0] ||
			e.arrFaces[0] == e0.arrFaces[1] ||
			e.arrFaces[1] == e0.arrFaces[0] ||
			e.arrFaces[1] == e0.arrFaces[1])
			bUpdate = true;

		if(bUpdate)
		{
			bool bGood = InitEdgeFaces(e, pVertsInfo, pVertsShared, &e0, boxBoundary);
			if(!bGood)
			{
				lstTriEdges.Delete(nEdge);
				nEdge--;
				continue;
			}

			if(e.arrVertId[0]>e.arrVertId[1])
			{
				int tmp = e.arrVertId[0];
				e.arrVertId[0] = e.arrVertId[1];
				e.arrVertId[1] = tmp;
			}

			lstInvolvedEdges.Add(nEdge);
		}

		assert(e.arrVertId[0] != e.arrVertId[1]);
	}

#ifdef _DEBUG
	TestEdges(0, lstTriEdges, lstIndices, &e0);
#endif


	// mark face as deleted in array of compiled indices 
	lstIndices[e0.arrFaces[0]*3+0]=-1; lstIndices[e0.arrFaces[0]*3+1]=-1; lstIndices[e0.arrFaces[0]*3+2]=-1;
	lstIndices[e0.arrFaces[1]*3+0]=-1; lstIndices[e0.arrFaces[1]*3+1]=-1; lstIndices[e0.arrFaces[1]*3+2]=-1;

	// remove duplicated edges
	float fNextOldMinLen = (lstTriEdges.Count()) ? lstTriEdges[0].fError : 0;
	static PodArray<int> lstDeletedEdges; lstDeletedEdges.Clear();
	for(int nEdgeI=0; nEdgeI<lstInvolvedEdges.Count(); nEdgeI++)
	{
		int nEdge = lstInvolvedEdges[nEdgeI];
		TriEdge & tEdge = lstTriEdges[nEdge];

		assert(	tEdge.arrVertId[0] != tEdge.arrVertId[1] );
		assert( tEdge.arrVertId[0] != e0.arrVertId[0] );
		assert( tEdge.arrVertId[1] != e0.arrVertId[0] );
		assert(	tEdge.arrVertId[0] == e0.arrVertId[1] || tEdge.arrVertId[1] == e0.arrVertId[1] );

		// update error value
		// todo: make errors never go down or never change and try to not sort them
		if(tEdge.fError == -1)
			continue;

		tEdge.fError = GetEdgeError(tEdge, pVertsShared, pVertsInfo, lstIndices, pColorShared0, pColorShared1, fSurfTypeRatio);

		int nFoundNum = 0;
		TriEdge & ei = lstTriEdges[nEdge];
		for(int nI=nEdgeI+1; nI<lstInvolvedEdges.Count(); nI++)
		{
			int n = lstInvolvedEdges[nI];
			TriEdge & en = lstTriEdges[n];
			if(ei.nVertIds == en.nVertIds)
			{ // if vertex ids are the same - delete second copy
				nFoundNum++;
				lstDeletedEdges.Add(n);
				lstTriEdges[n].fError = -1;
			}
		}
		assert(nFoundNum <= 1);
	}

	// delete edges
	if(lstDeletedEdges.Count())
		qsort(&lstDeletedEdges[0], lstDeletedEdges.Count(), sizeof(lstDeletedEdges[0]), CIndexedMesh__Cmp_Int);
	for(int nI=0; nI<lstDeletedEdges.Count(); nI++)
	{
		assert(lstTriEdges[lstDeletedEdges[nI]].fError == -1);
		lstTriEdges.Delete(lstDeletedEdges[nI]);
	}

#ifdef _DEBUG
	// check no references to deleted faces from pVertInfo
	for(int i=0; i<nVertCountShared; i++)
	{
		VertInfo & vi =	pVertsInfo[i];
		Vec3 vPos = pVertsShared[i];
		assert(vi.lstFacesIds.Find(e0.arrFaces[0])<0);
		assert(vi.lstFacesIds.Find(e0.arrFaces[1])<0);
	}
	TestEdges(0, lstTriEdges, lstIndices, &e0);
#endif

	// merge face lists of merged vertices into vertex 1
#ifdef _DEBUG
	PodArray<int> & lstFacesIds0 = pVertsInfo[e0.arrVertId[0]].lstFacesIds;
	PodArray<int> & lstFacesIds1 = pVertsInfo[e0.arrVertId[1]].lstFacesIds;
 	assert(lstFacesIds1.Count() == lstFacesIds0.Count());
	for(int m=0; m<lstFacesIds0.Count(); m++)
		assert(lstFacesIds1[m] == lstFacesIds0[m]);
#endif
	pVertsInfo[e0.arrVertId[0]].lstFacesIds.Clear();


	int nRemovedEdges = nEdgesNumBefore - lstTriEdges.Count();

//	if(nRemovedEdges != 3)
//		int ttt=0;

	return 0;
}

void CIndexedMesh::RestoreFacesFromIndices()
{
  SetFacesCount( m_nIndexCount/3 );
  memset(m_pFaces, 0, GetFacesCount()*sizeof(SMeshFace));

  int nFaceId = 0;
  for(int i=0; i<m_nIndexCount; i+=3) 
  {
    if(m_pIndices[i] != (uint16)-1) // deleted faces has -1 here
    {
      for(int v=0; v<3; v++)
      {
        assert(m_pIndices[i+v]<m_numVertices);
        m_pFaces[nFaceId].v[v] = m_pFaces[nFaceId].t[v] = m_pIndices[i+v];
      }
      nFaceId++;
    }
  }

  SetFacesCount( nFaceId );
}

void CIndexedMesh::CheckValid()
{
#ifdef _DEBUG

  void* pStream; 
  int nElementSize;

  GetStreamInfo(CMesh::POSITIONS, pStream, nElementSize);

  for(int v=0; v<GetVertexCount(); v++)
  {
    ((Vec3*)pStream)[v].IsValid();
  }

  GetStreamInfo(CMesh::INDICES, pStream, nElementSize);

  for(int v=0; v<GetIndexCount(); v++)
  {
    uint16 id = ((uint16*)pStream)[v];
    assert(id<GetVertexCount());
  }

  GetStreamInfo(CMesh::FACES, pStream, nElementSize);

  for(int v=0; v<GetFacesCount(); v++)
    for(int c=0; c<3; c++)
    {
      SMeshFace & face = ((SMeshFace*)pStream)[v];
      assert( face.v[c]<GetVertexCount() );
    }

  if(m_pIndices)
  {
    DynArray<SMeshSubset>::const_iterator it, end=m_subsets.end();

    for(it=m_subsets.begin(); it!=end; ++it)
    {
      const SMeshSubset &ref = *it;

      assert(ref.nFirstIndexId+ref.nNumIndices<=m_nIndexCount);

      for(int32 dwI=ref.nFirstIndexId; dwI<ref.nFirstIndexId+ref.nNumIndices && dwI<m_nIndexCount; ++dwI)
      {
        uint16 val = m_pIndices[dwI];

        if(val<ref.nFirstVertId || val>=ref.nFirstVertId+ref.nNumVerts || val>=GetVertexCount())
        {
          assert(!"Invalid subset values");
        }
      }
    }
  }
#endif // _DEBUG
}

void CIndexedMesh::RebuildMesh(Vec3 * pVertsShared, SMeshTexCoord * pCoordShared,Vec3 * pNormsShared, 
															 SMeshColor * pColorShared0, SMeshColor * pColorShared1, 
															 std::vector<int> &lstIndices, int nVertCountShared, int * pVertMatsShared)
{
	SetFacesCount( lstIndices.size()/3 );
//	memset(m_pFaces,0,lstIndices.size()/3*sizeof(SMeshFace)); // flags should stay

	int nFaceId = 0;
	for(uint32 i=0; i<lstIndices.size(); i+=3) 
	{
		if(lstIndices[i]>=0) // all deleted faces has -1 here
		{
			for(int v=0; v<3; v++)
			{
				assert(lstIndices[i+v]>=0 && lstIndices[i+v]<nVertCountShared);
				m_pFaces[nFaceId].v[v] = m_pFaces[nFaceId].v[v] = m_pFaces[nFaceId].t[v] = lstIndices[i+v];
			}
			nFaceId++;
		}
	}

	m_numFaces = nFaceId;

	SetVertexCount( nVertCountShared );
	SetTexCoordsCount( pCoordShared ? nVertCountShared : 0 );
	ReallocStream( COLORS_0,nVertCountShared );
  ReallocStream( COLORS_1,nVertCountShared );

  if(pVertMatsShared)
    ReallocStream( VERT_MATS,nVertCountShared );

	memcpy(m_pPositions, pVertsShared, sizeof(Vec3)*nVertCountShared); m_numVertices = nVertCountShared;
	if(m_pTexCoord && pCoordShared)
	{
		memcpy(m_pTexCoord, pCoordShared, sizeof(SMeshTexCoord)*nVertCountShared); 
		m_nCoorCount = nVertCountShared;
	}
	memcpy(m_pNorms, pNormsShared, sizeof(Vec3)*nVertCountShared);
	memcpy(m_pColor0, pColorShared0, sizeof(SMeshColor)*nVertCountShared); 
	memcpy(m_pColor1, pColorShared1, sizeof(SMeshColor)*nVertCountShared); 
	if(m_pVertMats && pVertMatsShared)
		memcpy(m_pVertMats, pVertMatsShared, sizeof(int)*nVertCountShared); 
}

float CIndexedMesh::GetEdgeError(TriEdge & tEdge, Vec3 * pVertsShared, VertInfo * pVertsInfo, std::vector<int> &lstIndices, SMeshColor * pColorShared0, SMeshColor * pColorShared1, float fSurfTypeRatio)
{
	// remember vert positions
	Vec3 arrvOrigPos[2] = 
	{
		pVertsShared[tEdge.arrVertId[0]],
		pVertsShared[tEdge.arrVertId[1]]
	};

	// find new vertex
	Vec3 vPosNew = (arrvOrigPos[0]+arrvOrigPos[1])*0.5f;

	// temporary replace positions with new one
	pVertsShared[tEdge.arrVertId[0]] = pVertsShared[tEdge.arrVertId[1]] = vPosNew;

	float surfTypeError = 0;
	float fMaxDistFromOldVertToNewTriangle = 0;
	for(int p=0; p<2; p++)
	{
		VertInfo & Inf = pVertsInfo[tEdge.arrVertId[p]];
		for(int f=0; f<Inf.lstFacesIds.Count(); f++)
		{
			int nFaceId = Inf.lstFacesIds[f];
			if(nFaceId == tEdge.arrFaces[0] || nFaceId == tEdge.arrFaces[1])
				continue; // skip faces of this edge

			assert(lstIndices[nFaceId*3+0]>=0);

			const Vec3 & v0 = pVertsShared[lstIndices[nFaceId*3+0]];
			const Vec3 & v1 = pVertsShared[lstIndices[nFaceId*3+1]];
			const Vec3 & v2 = pVertsShared[lstIndices[nFaceId*3+2]];

			Vec3 n = ((v1-v0)%(v0-v2));
			float fLen = n.GetLength();
			if(fLen>0.01f)
			{
				n /= fLen;
				Plane plane;
				plane.n = n;
				plane.d	=	-(n | v0);

				const float fDistFromOldVertToNewTriangle = fabsf(plane.DistFromPlane(arrvOrigPos[p]));
				if(fDistFromOldVertToNewTriangle>0.01f)
					if(fDistFromOldVertToNewTriangle>fMaxDistFromOldVertToNewTriangle)
						fMaxDistFromOldVertToNewTriangle = fDistFromOldVertToNewTriangle;
			}

      // compare vertex surface types
			const uint8 surfType0 = pColorShared0[lstIndices[nFaceId*3+0]].g;
			const uint8 surfType1 = pColorShared0[lstIndices[nFaceId*3+1]].g;
			const uint8 surfType2 = pColorShared0[lstIndices[nFaceId*3+2]].g;

			if(surfType0 != surfType1 || surfType1 != surfType2 || surfType2 != surfType0)
				surfTypeError = fSurfTypeRatio;
/*
      if(fSurfTypeRatio > 0.1f && surfTypeError < fSurfTypeRatio)
      {
        // compare vertex colors
        ColorB arrColors[3];
        for(int v=0; v<3; v++)
        {
          int id = lstIndices[nFaceId*3+v];
          arrColors[v].r = pColorShared0[id].r;
          arrColors[v].g = pColorShared0[id].b;
          arrColors[v].b = pColorShared1[id].b;
          arrColors[v].a = 255;
        }

        if(arrColors[0] != arrColors[1] || arrColors[1] != arrColors[2] || arrColors[2] != arrColors[0])
          surfTypeError = fSurfTypeRatio;
      }*/
		}
	}

	// restore original positions
	pVertsShared[tEdge.arrVertId[0]] = arrvOrigPos[0];
	pVertsShared[tEdge.arrVertId[1]] = arrvOrigPos[1];

//	assert(fAlphaMax>=fAlphaMin);

	float fError = 
		5.f*fMaxDistFromOldVertToNewTriangle + 
		0.5f*pVertsShared[tEdge.arrVertId[0]].GetDistance(pVertsShared[tEdge.arrVertId[1]]) +
		surfTypeError;

	assert(fError>=0);

	return fError;
}

//////////////////////////////////////////////////////////////////////////
void CIndexedMesh::Invalidate()
{
	m_bInvalidated = true;
	CalcBBox();
}

//////////////////////////////////////////////////////////////////////////
void CIndexedMesh::Optimize( const char * szComment, std::vector<uint16> *pVertexRemapping,std::vector<uint16> *pIndexRemapping )
{
	mesh_compiler::CMeshCompiler meshCompiler;
	if (pIndexRemapping)
		meshCompiler.SetIndexRemapping( pIndexRemapping );
	if (pVertexRemapping)
		meshCompiler.SetVertexRemapping( pVertexRemapping );

  // Stripification is very slow and should not be called at run time
  if(szComment)
    Warning( "CIndexedMesh::Optimize is called at run time by %s", szComment );

	meshCompiler.Compile( *this );
}

//////////////////////////////////////////////////////////////////////////
void CIndexedMesh::SimplifyMesh( float fLevel )
{
	mesh_compiler::CMeshCompiler meshCompiler;
	meshCompiler.ReduceMeshToLevel( *this,fLevel );

}

//////////////////////////////////////////////////////////////////////////
void CIndexedMesh::CalcBBox()
{
	if (m_numVertices == 0 || !m_pPositions)
	{
		m_bbox = AABB( Vec3(0,0,0),Vec3(0,0,0) );
		return;
	}

	m_bbox.Reset();
	int i,v;
	if (m_numFaces) for (i=0; i<m_numFaces; i++) for (v=0; v<3; v++)
	{
		int nIndex = m_pFaces[i].v[v];
		assert(nIndex>=0 && nIndex<m_numVertices);
		m_bbox.Add( m_pPositions[nIndex] );
	}
	else for (i=0; i<m_nIndexCount; i++)
		m_bbox.Add( m_pPositions[m_pIndices[i]] );
}

void CIndexedMesh::InitStreamSize()
{
	m_streamSize[POSITIONS] = m_pPositions ? m_numVertices : 0;
	m_streamSize[NORMALS]		= m_pPositions ? m_numVertices : 0;
	m_streamSize[TEXCOORDS] = m_pTexCoord ? m_numVertices : 0;
	m_streamSize[FACES]			= m_pFaces ? m_numFaces : 0;
	
	m_streamSize[COLORS_0] = m_pColor0 ? m_numVertices : 0;
	m_streamSize[COLORS_1] = m_pColor1 ? m_numVertices : 0;
  m_streamSize[VERT_MATS] = m_pVertMats ? m_numVertices : 0;
	
	m_streamSize[INDICES] = m_pIndices ? m_nIndexCount : 0;
	m_streamSize[TANGENTS] = m_pTangents ? m_numVertices : 0;
	m_streamSize[QTANGENTS] = m_pQTangents ? m_numVertices : 0;

	if(m_pSHInfo && m_pSHInfo->pSHCoeffs)
		m_streamSize[SHCOEFFS] = m_numVertices;
	else
		m_streamSize[SHCOEFFS] = 0;
}

inline int GetVecProjId_3Axis(const Vec3 & vNorm)
{
  int nOpenId=0;

  Vec3 vNormAbs = vNorm.abs();

  if(     vNormAbs.x>=vNormAbs.y && vNormAbs.x>=vNormAbs.z)
    nOpenId = 0;
  else if(vNormAbs.y>=vNormAbs.x && vNormAbs.y>=vNormAbs.z)
    nOpenId = 1;
  else 
    nOpenId = 2;

  return nOpenId;
}

inline int GetVecProjId_6Axis(const Vec3 & vNorm)
{
  int nOpenId=0;

  Vec3 vNormAbs = vNorm.abs();

  if(     vNormAbs.x>=vNormAbs.y && vNormAbs.x>=vNormAbs.z)
    nOpenId = (vNorm.x>0) ? 0 : 1;
  else if(vNormAbs.y>=vNormAbs.x && vNormAbs.y>=vNormAbs.z)
    nOpenId = (vNorm.y>0) ? 2 : 3;
  else 
    nOpenId = (vNorm.z>0) ? 4 : 5;

  return nOpenId;
}

class CMeshTexAtlas
{
public:

  CMeshTexAtlas(int nSizeX, int nSizeY)
  {
    m_nLMWidth = nSizeX; m_nLMHeight = nSizeY;
    m_arrAllocatedTexels.PreAllocate(m_nLMHeight,m_nLMHeight);
    memset(&m_arrAllocatedTexels[0],0,m_nLMWidth*sizeof(m_arrAllocatedTexels[0]));
  }

  ~CMeshTexAtlas()
  {
  }

  bool AllocateGroup(int32* pOffsetX, int32* pOffsetY, int nSizeX, int nSizeY)
  {
    int32	nBestX = m_nLMWidth;
    int32	nBestY = m_nLMHeight;
    int32	nBestWaste = m_nLMWidth * m_nLMHeight;

    int nBorder = 2;

    nSizeX += nBorder*2;
    nSizeY += nBorder*2;

    if(nSizeX>m_nLMWidth || nSizeY>m_nLMHeight)
      return false;

    for(int32 Y = 0; Y < (m_nLMHeight - nSizeY); Y++)
    {
      int32	nMaxX = 0;
      int32	nWaste = 0;

      for(int32 Row = 0; Row < nSizeY; Row++)
        nMaxX = max(m_arrAllocatedTexels[Y + Row], nMaxX);

      for(int32 Row = 0; Row < nSizeY; Row++)
        nWaste += nMaxX - m_arrAllocatedTexels[Y + Row];

      if(nMaxX <= m_nLMWidth - nSizeX && nWaste < nBestWaste)
      {
        nBestX = nMaxX;
        nBestY = Y;
        nBestWaste = nWaste;
      }
    }

    if(nBestX <= m_nLMWidth - nSizeX && nBestY <= m_nLMHeight - nSizeY)
    {
      *pOffsetX = nBestX + nBorder;
      *pOffsetY = nBestY + nBorder;

      for(int32 Row = 0;Row < nSizeY;Row++)
        m_arrAllocatedTexels[nBestY + Row] = nBestX + nSizeX;

      return true;
    }

    return false;
  }

  int m_nLMWidth;
  int m_nLMHeight;
  PodArray<int> m_arrAllocatedTexels;
};
/*
template <class T> bool SameSide(T p1, T p2, T a, T b)
{
  T cp1 = (b-a).Cross(p1-a);
  T cp2 = (b-a).Cross(p2-a);
  if (cp1.Dot(cp2) >= 0)
    return true;
  else 
    return false;
}

template <class T> bool PointInTriangle(T p, T a, T b, T c)
{
  if(SameSide(p, a, b, c) && SameSide(p, b, a, c) && SameSide(p, c, a, b))
    return true;
  else 
    return false;
}
*/

inline ColorF GetColorF_255( int x, int y, ColorB * pImg, int nImgSizeW, int nImgSizeH )
{
  ColorB & colB = pImg[x+y*nImgSizeW];
  ColorF colF;
  colF.r = colB.r;
  colF.g = colB.g;
  colF.b = colB.b;
  colF.a = colB.a;

  return colF;
}

ColorF GetBilinearFilteredAt( float iniX, float iniY, byte ** pImgMips, int nDimW, int nDimH, float fBr, int nMip )
{
  if(!pImgMips[0] || !nDimW || !nDimH)
    return Col_DarkGray;

  while(nMip && (!pImgMips[nMip] || nMip>(16-1)))
    nMip--;

  ColorB * pImg = (ColorB *)pImgMips[nMip];
  int nImgSizeW = nDimW>>nMip;
  int nImgSizeH = nDimH>>nMip;

  iniX *= nImgSizeW;
  iniY *= nImgSizeH;

//  iniX -= .5f;
//  iniY -= .5f;

  int x = (int)floor(iniX);
  int y = (int)floor(iniY);

  float rx = iniX - x;		// fractional part
  float ry = iniY - y;		// fractional part

  int nMaskW = nImgSizeW - 1;
  int nMaskH = nImgSizeH - 1;

//  return GetColorF_255(nMaskW&(x  ),nMaskH&(y  ), pImg, nImgSizeW, nImgSizeH) * fBr;

  ColorF top = GetColorF_255(nMaskW&(x  ),nMaskH&(y  ), pImg, nImgSizeW, nImgSizeH)*(1.f-rx)	// left top
             + GetColorF_255(nMaskW&(x+1),nMaskH&(y  ), pImg, nImgSizeW, nImgSizeH)*rx;				// right top
  ColorF bot = GetColorF_255(nMaskW&(x  ),nMaskH&(y+1), pImg, nImgSizeW, nImgSizeH)*(1.f-rx)	// left bottom
             + GetColorF_255(nMaskW&(x+1),nMaskH&(y+1), pImg, nImgSizeW, nImgSizeH)*rx;				// right bottom

  return (top*(1.f-ry) + bot*ry) * fBr;  
}

void CIndexedMesh::ProcessMaterial(IMaterial * pMat, int nHitMatID, const Vec4 & hitVertCol, const Vec2 & vTC, int nMip, ColorF & colSumm, Vec3 & vSummNorm, 
                                   const Vec4 & vHitTangent, const Vec4 & vHitBinormal, const Matrix34 * pObjMatrix)
{
  SShaderItem & sItem = pMat->GetShaderItem(nHitMatID);

	ColorF valDetail = Col_Black;
	bool bUSeValDetail = false;

	if(SEfResTexture * pTex = sItem.m_pShaderResources->GetTexture(EFTT_DETAIL_OVERLAY))
	{
		if(ITexture * pITex = pTex->m_Sampler.m_pITex)
		{
			if(byte ** pImgMips = pITex->GetSystemCopy())
			{
				if(pImgMips[0])
				{
					Vec4 vTextureAtlasInfo(0,0,0,0);
					vTextureAtlasInfo.x = pTex->m_TexModificator.m_Offs[0];
					vTextureAtlasInfo.y = pTex->m_TexModificator.m_Offs[1];
					vTextureAtlasInfo.z = pTex->m_TexModificator.m_Tiling[0];
					vTextureAtlasInfo.w = pTex->m_TexModificator.m_Tiling[1];

					valDetail = GetBilinearFilteredAt( 
						5.f*vTC.x*vTextureAtlasInfo.z + vTextureAtlasInfo.x, 
						5.f*vTC.y*vTextureAtlasInfo.w + vTextureAtlasInfo.y, 
						pImgMips, pITex->GetWidth(), pITex->GetHeight(), 1.f/255.f, nMip );

					bUSeValDetail = true;
				}
			}
		}
	}

  if(SEfResTexture * pTex = sItem.m_pShaderResources->GetTexture(EFTT_DIFFUSE))
  {
    if(ITexture * pITex = pTex->m_Sampler.m_pITex)
    {
      if(byte ** pImgMips = pITex->GetSystemCopy())
      {
        if(pImgMips[0])
        {
          ColorF vertCol = 1.f;
          
          if(sItem.m_pShader->GetFlags2() & EF2_VERTEXCOLORS)
          {
            if(sItem.m_pShader->GetShaderType()==eST_Vegetation)
            {
              vertCol.r = hitVertCol.w;
              vertCol.g = hitVertCol.w;
              vertCol.b = hitVertCol.w;
            }
            else
            {
              vertCol.r = hitVertCol.x;
              vertCol.g = hitVertCol.y;
              vertCol.b = hitVertCol.z;
            }

            vertCol.a = 1.f;
          }

          Vec4 vTextureAtlasInfo(0,0,0,0);
          vTextureAtlasInfo.x = pTex->m_TexModificator.m_Offs[0];
          vTextureAtlasInfo.y = pTex->m_TexModificator.m_Offs[1];
          vTextureAtlasInfo.z = pTex->m_TexModificator.m_Tiling[0];
          vTextureAtlasInfo.w = pTex->m_TexModificator.m_Tiling[1];

          ColorF col = GetBilinearFilteredAt( 
            vTC.x*vTextureAtlasInfo.z + vTextureAtlasInfo.x, 
            vTC.y*vTextureAtlasInfo.w + vTextureAtlasInfo.y, 
            pImgMips, pITex->GetWidth(), pITex->GetHeight(), 1.f/255.f, nMip );

					if(bUSeValDetail && !(sItem.m_pShader->GetFlags2() & EF2_DETAILBUMPMAPPING))
					{
						col += (valDetail-0.5f)*.1f;
					}

					col.srgb2rgb();

          ColorF matColor = sItem.m_pShaderResources->GetDiffuseColor();
          float tmp = matColor.r;
          matColor.r = matColor.b;
          matColor.b = tmp;
          col = col * matColor * vertCol;

          colSumm = col;
        }
      }
    }
  }

  if(SEfResTexture * pTex = sItem.m_pShaderResources->GetTexture(EFTT_DECAL_OVERLAY))
  {
    if(ITexture * pITex = pTex->m_Sampler.m_pITex)
    {
      if(byte ** pImgMips = pITex->GetSystemCopy())
      {
        if(SDetailDecalInfo * pDecalInfo = sItem.m_pShaderResources->GetDetailDecalInfo())
          if(pImgMips[0])
        {
          ColorF col = GetBilinearFilteredAt( 
            vTC.x*pDecalInfo->vTileOffs[0].x + pDecalInfo->vTileOffs[0].z, 
            vTC.y*pDecalInfo->vTileOffs[0].y + pDecalInfo->vTileOffs[0].w, 
            pImgMips, pITex->GetWidth(), pITex->GetHeight(), 1.f/255.f, nMip );

          colSumm.lerpFloat(colSumm,colSumm*col,sqrt(col.a*1.75f));
        }
      }
    }
  }

  if(SEfResTexture * pTex = sItem.m_pShaderResources->GetTexture(EFTT_BUMP))
  {
    if(ITexture * pITex = pTex->m_Sampler.m_pITex)
    {
      if(byte ** pImgMips = pITex->GetSystemCopy())
      {
        if(pImgMips[0])
        {
          Vec4 vTextureAtlasInfo(0,0,0,0);
          vTextureAtlasInfo.x = pTex->m_TexModificator.m_Offs[0];
          vTextureAtlasInfo.y = pTex->m_TexModificator.m_Offs[1];
          vTextureAtlasInfo.z = pTex->m_TexModificator.m_Tiling[0];
          vTextureAtlasInfo.w = pTex->m_TexModificator.m_Tiling[1];

          ColorF val = GetBilinearFilteredAt( 
            vTC.x*vTextureAtlasInfo.z + vTextureAtlasInfo.x, 
            vTC.y*vTextureAtlasInfo.w + vTextureAtlasInfo.y, 
            pImgMips, pITex->GetWidth(), pITex->GetHeight(), 1.f/255.f, nMip );

          vSummNorm.x = val.g*2.f - 1.f;
          vSummNorm.y = val.a*2.f - 1.f;
          vSummNorm.z = sqrt_fast_tpl(fabs(1.f - (vSummNorm.x*vSummNorm.x + vSummNorm.y*vSummNorm.y)));

					if(bUSeValDetail && (sItem.m_pShader->GetFlags2() & EF2_DETAILBUMPMAPPING))
					{
						vSummNorm.x += (valDetail.g*2.f - 1.f);
						vSummNorm.y += (valDetail.a*2.f - 1.f);
					}
        }
      }
    }
  }

  Vec3 vTang = Vec3(vHitTangent.x,vHitTangent.y,vHitTangent.z);
  Vec3 vBinorm = Vec3(vHitBinormal.x,vHitBinormal.y,vHitBinormal.z);
  Vec3 vNormal = vBinorm.Cross(vTang);        
  Matrix33 matTang;
  matTang.SetRow(0, vTang);
  matTang.SetRow(1, vBinorm);
  matTang.SetRow(2, vNormal);
  vSummNorm = matTang.GetInverted().TransformVector(vSummNorm);
  vSummNorm *= -vHitTangent.w;

  if(pObjMatrix)
    vSummNorm = pObjMatrix->TransformVector(vSummNorm);
  vSummNorm.NormalizeFast();
  vSummNorm *= 0.5f;

  if(SEfResTexture * pTex = sItem.m_pShaderResources->GetTexture(EFTT_GLOSS))
  {
    if(ITexture * pITex = pTex->m_Sampler.m_pITex)
    {
      if(byte ** pImgMips = pITex->GetSystemCopy())
      {
        if(pImgMips[0])
        {
          Vec4 vTextureAtlasInfo(0,0,0,0);
          vTextureAtlasInfo.x = pTex->m_TexModificator.m_Offs[0];
          vTextureAtlasInfo.y = pTex->m_TexModificator.m_Offs[1];
          vTextureAtlasInfo.z = pTex->m_TexModificator.m_Tiling[0];
          vTextureAtlasInfo.w = pTex->m_TexModificator.m_Tiling[1];

          ColorF val = GetBilinearFilteredAt( 
            vTC.x*vTextureAtlasInfo.z + vTextureAtlasInfo.x, 
            vTC.y*vTextureAtlasInfo.w + vTextureAtlasInfo.y, 
            pImgMips, pITex->GetWidth(), pITex->GetHeight(), 1.f/255.f, nMip );
          val.srgb2rgb();

          val *= sItem.m_pShaderResources->GetSpecularColor() * 4.f;
          val.Clamp();
          vSummNorm *= (1.f + val.Luminance());
        }
      }
    }
  }
  else
  {
    ColorF val = colSumm.a;
    val *= sItem.m_pShaderResources->GetSpecularColor() * 4.f;
    val.Clamp();
    vSummNorm *= (1.f + val.Luminance());
  }
}

void CIndexedMesh::FillTexel(const Vec3 & vPos, Vec3 & vMinCache, SVoxValueCache & voxValueCache, ColorB * pPixel0, ColorB * pPixel1, ColorB * pPixel2, bool bBuildPixMats, bool bFilter )
{
  ColorB vWeigs(0,0,0,0);
  ColorB vTypes(0,0,0,0);
  ColorB vColor(0,0,0,0);

  ColorF vVertColorInterp;

  SSurfTypeInfo surf_type;
  Vec3 vNormal;
  if(GetCVars()->e_VoxTerHeightmapEditing)
  {
    if(C3DEngine::m_pGetLayerIdAtCallback)
    {
      {
        //FRAME_PROFILER( "GetColorAtPosition", GetSystem(), PROFILE_3DENGINE );
        ColorB colB = C3DEngine::m_pGetLayerIdAtCallback->GetColorAtPosition(vPos.y,vPos.x,bFilter);
        vVertColorInterp.b = colB.r;
        vVertColorInterp.g = colB.g;
        vVertColorInterp.r = colB.b;
      }

      if(bFilter)
      {
        float fUnitSize = (float)GetTerrain()->GetHeightMapUnitSize();

        Vec3 vMin(float(int(vPos.x/fUnitSize)*fUnitSize),float(int(vPos.y/fUnitSize)*fUnitSize),0);

        if(!vMin.IsEquivalent(vMinCache))
        {
          //FRAME_PROFILER( "GetLayerIdAtPosition", GetSystem(), PROFILE_3DENGINE );

          vMinCache = vMin;

          for(int _x=0; _x<=1; _x++)
          {
            for(int _y=0; _y<=1; _y++)
            {
              float X = vMin.x + fUnitSize*_x;
              float Y = vMin.y + fUnitSize*_y;

              if(GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo)
              {
                SSurfTypeInfo nSurfType = m_pVoxTerrain->GetLayerAtPosition((int)(Y/fUnitSize),(int)(X/fUnitSize));
                voxValueCache.values[_x][_y][0].surf_type = nSurfType;
              }
              else
              {
                int nSurfType = C3DEngine::m_pGetLayerIdAtCallback->GetLayerIdAtPosition((int)(Y/fUnitSize),(int)(X/fUnitSize));
                voxValueCache.values[_x][_y][0].surf_type = nSurfType;
              }
            }
          }

          voxValueCache.vNormal = GetTerrain()->GetTerrainSurfaceNormal(vPos,1.f,false);
        }

        Vec3 vT = (vPos-vMin);

        vT.x /= fUnitSize;
        vT.y /= fUnitSize;
        vT.z /= fUnitSize;

        {
          //FRAME_PROFILER( "Lerp", GetSystem(), PROFILE_3DENGINE );

          SSurfTypeInfo arrSType1D[2];

          for(int nX=0; nX<2; nX++)
          {
            SSurfTypeInfo & fValue0 = voxValueCache.values[nX][0][0].surf_type;
            SSurfTypeInfo & fValue1 = voxValueCache.values[nX][1][0].surf_type;
            arrSType1D[nX].Lerp(vT.y, fValue0, fValue1);
          }

          surf_type.Lerp(vT.x, arrSType1D[0], arrSType1D[1]);
        }

        vNormal = voxValueCache.vNormal;
      }
      else
      {
        //FRAME_PROFILER( "GetLayerIdAtPosition2", GetSystem(), PROFILE_3DENGINE );

        float fUnitSize = (float)GetTerrain()->GetHeightMapUnitSize();

        if(GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo)
          surf_type = m_pVoxTerrain->GetLayerAtPosition((int)(vPos.y/fUnitSize),(int)(vPos.x/fUnitSize));
        else
          surf_type = C3DEngine::m_pGetLayerIdAtCallback->GetLayerIdAtPosition((int)(vPos.y/fUnitSize),(int)(vPos.x/fUnitSize));

        vNormal = GetTerrain()->GetTerrainSurfaceNormal(vPos,1.f,false);
      }
    }
  }
  else
  {
    m_pVoxTerrain->GetVoxelValueInterpolated(NULL, vPos, vVertColorInterp, NULL, NULL, surf_type, vNormal, &voxValueCache);
    surf_type.Normalize();
    float tmp = vVertColorInterp.b;
    vVertColorInterp.b = vVertColorInterp.r;
    vVertColorInterp.r = tmp;
  }

  vWeigs[0] = surf_type.we[0];
  vWeigs[1] = surf_type.we[1];
  vWeigs[2] = surf_type.we[2];
  vWeigs[3] = (uint8)SATURATEB(127.f+vNormal.x*127.f);

  vTypes[0] = surf_type.ty[0];
  vTypes[1] = surf_type.ty[1];
  vTypes[2] = surf_type.ty[2];
  vTypes[3] = surf_type.ty[3];

  vColor[0] = (uint8)SATURATEB(vVertColorInterp[0]);
  vColor[1] = (uint8)SATURATEB(vVertColorInterp[1]);
  vColor[2] = (uint8)SATURATEB(vVertColorInterp[2]);
  vColor[3] = (uint8)SATURATEB(127.f+vNormal.y*127.f);

  *(pPixel0) = vWeigs;
  *(pPixel1) = vTypes;
  *(pPixel2) = vColor;

  // register used layers
  if(!GetCVars()->e_VoxTerHeightmapEditing || bBuildPixMats)
  {
    if(!m_pPixMats)
      m_pPixMats = new PodArray<unsigned short>;

    for(int s=0; s<4; s++)
    {
      if(surf_type.we[s])
      {
        if(m_pPixMats->Find(surf_type.ty[s])<0)
          m_pPixMats->Add(surf_type.ty[s]);
      }
    }
  }
}

void CIndexedMesh::Render2dSubTriangle(	int x0, int y0, int x1, int y1, int x2, int y2, int nTexSizeX, int nTexSizeY, 
                                      ColorB ** pTexDatas, int nTexNum, const SMeshFace & face, float fMeshNodeSize, void* node, void* pIdx, class IsoOctree * isoTree, PodArray<SDecalProjectionInfo> & decalProjectionInfo, float fInRayLen, const Matrix33 & interpMat,
                                      int &nTrisDrawn, int &nPixelsDrawn)
{
  FUNCTION_PROFILER_3DENGINE;

  y0 = CLAMP(y0, 0, nTexSizeY-1);
  y1 = CLAMP(y1, 0, nTexSizeY-1);
  y2 = CLAMP(y2, 0, nTexSizeY-1);
	ColorB * pTargetEnd = &pTexDatas[0][nTexSizeX*nTexSizeY];
  // compute slopes
  float dx0 = (float)(x2 - x0) / (y2 - y0);
  float dx1 = (float)(x2 - x1) / (y2 - y1);

  float lx = (float) x0, rx = (float) x1;

  ColorB *pLine0, *pLine1, *pLine2;
  int nLinesNum;
  int nPitch = nTexSizeX;

  if(y0 < y2)
  { 
    pLine0 = pTexDatas[0] + y0 * nPitch; 
    pLine1 = pTexDatas[1] + y0 * nPitch; 
    pLine2 = pTexDatas[2] + y0 * nPitch; 
    nLinesNum = y2 - y0; 
  }
  else  
  { 
    pLine0 = pTexDatas[0] + y2 * nPitch; 
    pLine1 = pTexDatas[1] + y2 * nPitch; 
    pLine2 = pTexDatas[2] + y2 * nPitch; 
    nLinesNum = y0 - y2; 

    lx = rx = (float)x2; 
  }

  Vec2 t0 = *(Vec2*)&m_pTexCoord[face.t[0]];
  Vec2 t1 = *(Vec2*)&m_pTexCoord[face.t[1]];
  Vec2 t2 = *(Vec2*)&m_pTexCoord[face.t[2]];

  Vec3 v0 = *(Vec3*)&m_pPositions[face.v[0]];
  Vec3 v1 = *(Vec3*)&m_pPositions[face.v[1]];
  Vec3 v2 = *(Vec3*)&m_pPositions[face.v[2]];

  Vec3 n0 = *(Vec3*)&m_pNorms[face.v[0]];
  Vec3 n1 = *(Vec3*)&m_pNorms[face.v[1]];
  Vec3 n2 = *(Vec3*)&m_pNorms[face.v[2]];

  Vec2 t1_t0 = t1-t0;
  Vec2 t2_t0 = t2-t0;

  int nMip = 0;

  float fSize = fMeshNodeSize;
  while(fSize > 4.f)
  {
    fSize *= 0.5f;
    nMip ++;
  }

  Vec3 vMinCache(-1,-1,-1);
  SVoxValueCache voxValueCache;

  //(void)pTargetEnd;
  for(int l=0; l<nLinesNum; ++l)
  {
    lx = max(lx,0.f);
    rx = min(rx,(float)nTexSizeX-1.f);

    ColorB * pPixel0 = (pLine0 + (int)(lx));
    ColorB * pPixel1 = (pLine1 + (int)(lx));
    ColorB * pPixel2 = (pLine2 + (int)(lx));

    int lxi = (int)(lx);
    int rxi = (int)(rx);

    for(int j=lxi; j<=rxi; ++j)
    {
      assert((pLine0 + j)>=pTexDatas[0]);
      assert((pLine0 + j) < pTargetEnd);

      // find TC
      int nOffset = (int)(pPixel0 - pTexDatas[0]);
      int tc_Y = nOffset/nTexSizeX;
      int tc_X = nOffset - tc_Y*nTexSizeX;

      Vec2 vTC((float)tc_X/nTexSizeX, (float)tc_Y/nTexSizeY);

      Vec3 vUVW(
        interpMat.m00*vTC.x+interpMat.m01*vTC.y+interpMat.m02, 
        interpMat.m10*vTC.x+interpMat.m11*vTC.y+interpMat.m12, 
        interpMat.m20*vTC.x+interpMat.m21*vTC.y+interpMat.m22);

      float fTestSum(vUVW[0] + vUVW[1] + vUVW[2]);
      if( fTestSum > 1.1f || fTestSum < -0.1f )
      {
        pPixel0 ++;
        pPixel1 ++;
        pPixel2 ++;
        continue;
      }

      Vec3 vPos = v0*vUVW[0] + v1*vUVW[1] + v2*vUVW[2];
      Vec3 vNor = n0*vUVW[0] + n1*vUVW[1] + n2*vUVW[2];

      if(vNor.IsZero(.0001f))
      {
        pPixel0 ++;
        pPixel1 ++;
        pPixel2 ++;
        continue;
      }

      if(pPixel1->a==255 && GetCVars()->e_VoxTerPlanarProjection)
      {
        pPixel0 ++;
        pPixel1 ++;
        pPixel2 ++;
        continue;
      }

      vNor.NormalizeFast();
      vNor *= fInRayLen;

      bool bHit = false;

      if(GetCVars()->e_VoxTerMixMask)
      {
        FillTexel(vPos, vMinCache, voxValueCache, pPixel0, pPixel1, pPixel2, fMeshNodeSize<=64.f, fMeshNodeSize<=128.f);

         nPixelsDrawn++; 
      }
      else
      {
        ColorF colSumm = Col_Green;
        Vec3 vSummNorm(0.f,0.f,0.5f);

        Ray WSRay(vPos+vNor, -vNor*2.f); 
        float fRayLen = fInRayLen;

        SRayHitInfo hitInfo;
        hitInfo.bInFirstHit = false;
        hitInfo.bUseCache = false;
        hitInfo.bGetVertColorAndTC = true;

        for(int d=0; d<decalProjectionInfo.Count(); d++)
        {
          SDecalProjectionInfo & info = decalProjectionInfo[d];

	        if( (info.nLastHitTriId == HIT_UNKNOWN) || (j==lxi) || ((j&1)==(l&1)) )
	        {
		        hitInfo.nHitTriID = HIT_UNKNOWN;
		        hitInfo.nHitMatID = HIT_UNKNOWN;
	        }
	        else
	        {
		        hitInfo.nHitTriID = info.nLastHitTriId;
		        hitInfo.nHitMatID = info.nLastHitMatId;
	        }

	        if(hitInfo.nHitTriID != HIT_NO_HIT)
	        {
		        hitInfo.inRay.origin = info.matObjInv.TransformPoint(WSRay.origin);
		        hitInfo.inRay.direction = info.matObjInv.TransformVector(WSRay.direction);
		        hitInfo.inReferencePoint = hitInfo.inRay.origin + hitInfo.inRay.direction*0.5f;
		        hitInfo.fMaxHitDistance = fRayLen / info.fScale;
	        }

	        if(hitInfo.nHitTriID != HIT_NO_HIT && info.pStatObj->RayIntersection(hitInfo, info.pMat))
          {
            assert(hitInfo.fDistance <= hitInfo.fMaxHitDistance);

            fRayLen = hitInfo.fDistance * info.fScale;

            ProcessMaterial(info.pMat, hitInfo.nHitMatID, hitInfo.vHitColor, hitInfo.vHitTC, nMip, colSumm, vSummNorm, 
              hitInfo.vHitTangent, hitInfo.vHitBinormal, &info.matObj);

		        info.nLastHitTriId = hitInfo.nHitTriID;
		        info.nLastHitMatId = hitInfo.nHitMatID;

            bHit = true;
          }
	        else
	        {
		        info.nLastHitTriId = HIT_NO_HIT;
		        info.nLastHitMatId = HIT_NO_HIT;
	        }
        }

        if(bHit)
        {
          // range conversion
          colSumm *= GetCVars()->e_VoxTerTexRangeScale;

          colSumm.Clamp(0.f,1.f);
          *(pPixel0) = colSumm;
          pPixel0->a = 255;

          pPixel1->r = SATURATEB((int)(vSummNorm.z*127.f+127.f));
          pPixel1->g = SATURATEB((int)(vSummNorm.y*127.f+127.f));
          pPixel1->b = SATURATEB((int)(vSummNorm.x*127.f+127.f));
          pPixel1->a = 255;

          nPixelsDrawn++;
        }
      }

      pPixel0 ++;
      pPixel1 ++;
      pPixel2 ++;
    }

    lx  += dx0;
    rx  += dx1;
    pLine0 += nPitch;
    pLine1 += nPitch;
    pLine2 += nPitch;
  }
}

void CIndexedMesh::Render2dTriangle(const Vec2 * pVerts, int i1, int i2, int i3,  int nTexSizeX, int nTexSizeY, ColorB ** pTexDatas, int nTexNum, 
                                   const SMeshFace & face, float fMeshNodeSize, void* node, void* pIdx, class IsoOctree * isoTree, 
                                   PodArray<SDecalProjectionInfo> & decalProjectionInfo, float fRayLen, int &nTrisDrawn, int &nPixelsDrawn)
{
  // sort verts by height
  if(pVerts[i1].y > pVerts[i2].y) mySwap(i1, i2);
  if(pVerts[i1].y > pVerts[i3].y) mySwap(i1, i3);
  if(pVerts[i2].y > pVerts[i3].y) mySwap(i2, i3);

  float fHalfPixX = 0.5f;
  float fHalfPixY = 0.0f;

  int x0 = (int)(pVerts[i1].x*nTexSizeX+fHalfPixX), y0 = (int)(pVerts[i1].y*nTexSizeY+fHalfPixY);
  int x1 = (int)(pVerts[i2].x*nTexSizeX+fHalfPixX), y1 = (int)(pVerts[i2].y*nTexSizeY+fHalfPixY);
  int x2 = (int)(pVerts[i3].x*nTexSizeX+fHalfPixX), y2 = (int)(pVerts[i3].y*nTexSizeY+fHalfPixY);

  Matrix33 interpMat;

  interpMat.SetColumn(0,Vec3(pVerts[face.t[0]].x,pVerts[face.t[0]].y,1.f));
  interpMat.SetColumn(1,Vec3(pVerts[face.t[1]].x,pVerts[face.t[1]].y,1.f));
  interpMat.SetColumn(2,Vec3(pVerts[face.t[2]].x,pVerts[face.t[2]].y,1.f));
  interpMat.Invert();

  // test for easy cases, else split triangle in two and render both half's
  if(y1 == y2)
  {
    if (y0 == y1)
      return;
    if(x1 > x2) 
    {
      mySwap(x1, x2);
    }
    Render2dSubTriangle(
      x1, y1, 
      x2, y2, 
      x0, y0, 
      nTexSizeX, nTexSizeY, pTexDatas, nTexNum, face, fMeshNodeSize, node, pIdx, isoTree, decalProjectionInfo, fRayLen, interpMat, nTrisDrawn, nPixelsDrawn);
  }
  else if(y0 == y1)
  {
    if (y1 == y2)
      return;
    if(x0 > x1) 
    {
      mySwap(x0, x1);
    }
    Render2dSubTriangle(
      x0, y0, 
      x1, y1, 
      x2, y2, 
      nTexSizeX, nTexSizeY, pTexDatas, nTexNum, face, fMeshNodeSize, node, pIdx, isoTree, decalProjectionInfo, fRayLen, interpMat, nTrisDrawn, nPixelsDrawn);
  }
  else
  {
    // compute x pos of the vert that builds the splitting line with x1
    int tmp_x = x0 + (int)(0.5f + (float)(y1-y0) * (float)(x2-x0) / (float)(y2-y0));

    if(x1 > tmp_x) 
    {
      mySwap(x1, tmp_x);
    }
    Render2dSubTriangle(
      x1, y1, 
      tmp_x, y1, 
      x0, y0, 
      nTexSizeX, nTexSizeY, pTexDatas, nTexNum, face, fMeshNodeSize, node, pIdx, isoTree, decalProjectionInfo, fRayLen, interpMat, nTrisDrawn, nPixelsDrawn);
    Render2dSubTriangle(
      x1, y1, 
      tmp_x, y1, 
      x2, y2, 
      nTexSizeX, nTexSizeY, pTexDatas, nTexNum, face, fMeshNodeSize, node, pIdx, isoTree, decalProjectionInfo, fRayLen, interpMat, nTrisDrawn, nPixelsDrawn);
  }
}

bool CIndexedMesh::RasterizeTriangles(  int nTexSizeX, int nTexSizeY, float fMeshNodeSize,
                                      PodArray<SGroupInfo> & _arrGroups, ColorB ** pTexDatas, int nTexNum, void* node, void* pIdx, class IsoOctree * isoTree, 
                                      PodArray<IRenderNode*> * pObjects, std::map<string,SImageSubInfo*> & imageInfos, float fRayLen, int &nTrisDrawn, int &nPixelsDrawn )
{
  FUNCTION_PROFILER_3DENGINE;

  PodArray<SDecalProjectionInfo> decalProjectionInfo; 

  for(int i=0; i<m_numFaces; i++)
  {
    SMeshFace & face = m_pFaces[i];

    Vec3 & v0 = (m_pPositions[face.v[0]]);
    Vec3 & v1 = (m_pPositions[face.v[1]]);
    Vec3 & v2 = (m_pPositions[face.v[2]]);

    decalProjectionInfo.Clear();

    if(GetCVars()->e_VoxTerMixMask)
    {
      Render2dTriangle((Vec2*)m_pTexCoord, face.t[0], face.t[1], face.t[2], 
        nTexSizeX, nTexSizeY, pTexDatas, nTexNum, face, fMeshNodeSize, node, pIdx, isoTree, decalProjectionInfo, fRayLen, nTrisDrawn, nPixelsDrawn);

      nTrisDrawn ++;
      continue;
    }

    for(int d=0; d<pObjects->Count(); d++)
    {
      IRenderNode * pNode = pObjects->GetAt(d);

      AABB nodeBoxEx = pNode->GetBBox();
      nodeBoxEx.min -= Vec3(fRayLen,fRayLen,fRayLen);
      nodeBoxEx.max += Vec3(fRayLen,fRayLen,fRayLen);

      if(!Overlap::AABB_Triangle(nodeBoxEx,v0,v1,v2))
        continue;

      SDecalProjectionInfo info;

      if(IMaterial * pMaterial = pNode->GetMaterial())
      {
        Matrix34A mat;
        pNode->GetEntityStatObj(0,0,&mat);
        info.matObjInv = mat.GetInverted();
        info.matObj = mat;
        info.pStatObj = (CStatObj *)pNode->GetEntityStatObj(0,0,NULL);
        
        if(pNode->GetRenderNodeType() == eERType_Brush)
          info.fScale = ((CBrush*)pNode)->m_fMatrixScale;
        else if(pNode->GetRenderNodeType() == eERType_Vegetation)
          info.fScale = ((CVegetation*)pNode)->GetScale();
        else
          assert(!"Undefined object type");

        info.pMat = pMaterial;
        decalProjectionInfo.Add(info);
      }
    }

    if(decalProjectionInfo.Count())
    {
      bool bUseRemeshing = false;

      for(int v=0; v<3; v++)
      {
        if(SImageInfo * pInfo = Get3DEngine()->GetBaseTextureData(m_pVertMats[face.v[v]]))
        {
          if(pInfo->fUseRemeshing)
          {
            bUseRemeshing = true;
            break;
          }
        }
      }

      if(bUseRemeshing)
      {
        Render2dTriangle((Vec2*)m_pTexCoord, face.t[0], face.t[1], face.t[2], 
          nTexSizeX, nTexSizeY, pTexDatas, nTexNum, face, fMeshNodeSize, node, pIdx, isoTree, decalProjectionInfo, fRayLen, nTrisDrawn, nPixelsDrawn);

        nTrisDrawn ++;
      }
    }
  }

  return nPixelsDrawn!=0;
}

bool CIndexedMesh::FillTexturesPlanar( int nTexSize, ColorB ** pTexDatas, int nTexNum, int & nPixelsDrawn, bool bBuildPixMats, bool bFilter )
{
  FUNCTION_PROFILER_3DENGINE;

  Vec3 vBoxSize = m_bbox.GetSize();
  Vec3 vPos(0,0,0);

  Vec3 vMinCache(-1,-1,-1);
  SVoxValueCache voxValueCache;

  for(int _x=0; _x<nTexSize; _x++) 
    for(int _y=0; _y<nTexSize; _y++)
  {
    ColorB * pPixel0 = &pTexDatas[0][_x*nTexSize+_y];
    ColorB * pPixel1 = &pTexDatas[1][_x*nTexSize+_y];
    ColorB * pPixel2 = &pTexDatas[2][_x*nTexSize+_y];

    vPos.x = m_bbox.min.x + vBoxSize.x*_x/nTexSize;
    vPos.y = m_bbox.min.y + vBoxSize.y*_y/nTexSize;
    
    FillTexel(vPos, vMinCache, voxValueCache, pPixel0, pPixel1, pPixel2, bBuildPixMats, bFilter);
    
    nPixelsDrawn++;
  }

  return true;
}

/*
bool CIndexedMesh::RasterizeTrianglesOLD(  int nTexSize, uint16 ** ppTriHitCache, uint16 * ppTriHitCacheSize, 
                                        float fMeshNodeSize,
                                        PodArray<SGroupInfo> & arrGroups, float fAtlasSize,
                                        ColorB * pTexData )
{
  FUNCTION_PROFILER_3DENGINE;

  if(!(*ppTriHitCache) || (*ppTriHitCacheSize) != sizeof(uint16)*nTexSize*nTexSize)
  {
    delete [] (*ppTriHitCache);
    (*ppTriHitCache) = new uint16[nTexSize*nTexSize];
    memset(*ppTriHitCache, 0, sizeof(uint16)*nTexSize*nTexSize);
    *ppTriHitCacheSize = sizeof(uint16)*nTexSize*nTexSize;
  }

  for(int g=0; g<arrGroups.Count(); g++)
  {
    SGroupInfo & group = arrGroups[g];

    int x1 = max(       0, (int)(group.aabb.min.x*fAtlasSize)-1); 
    int x2 = min(nTexSize, (int)(group.aabb.max.x*fAtlasSize)+1);
    int y1 = max(       0, (int)(group.aabb.min.y*fAtlasSize)-1); 
    int y2 = min(nTexSize, (int)(group.aabb.max.y*fAtlasSize)+1);

    for(int x=x1; x<x2; x++) for(int y=y1; y<y2; y++)
    {
      Vec2 vTexCoord((float)x/nTexSize, (float)y/nTexSize);

      assert(1||group.aabb.IsContainPoint(vTexCoord));

      SMeshFace face = m_pFaces[(*ppTriHitCache)[x*nTexSize+y]];
      Vec2 & t0 = *(Vec2*)&m_pTexCoord[face.t[0]];
      Vec2 & t1 = *(Vec2*)&m_pTexCoord[face.t[1]];
      Vec2 & t2 = *(Vec2*)&m_pTexCoord[face.t[2]];

      Vec3 vPos(0,0,0);
      float u=0, v=0, w=0;
      bool bFound = false;
      if(GetBarycentricCoordinates(vTexCoord, t0, t1, t2, u, v, w, 0))
      {
        bFound = true;
      }
      else
      {
        for(int i=0; i<group.arrTris.Count(); i++)
        {
          face = m_pFaces[group.arrTris[i]];

          Vec2 t0 = *(Vec2*)&m_pTexCoord[face.t[0]];
          Vec2 t1 = *(Vec2*)&m_pTexCoord[face.t[1]];
          Vec2 t2 = *(Vec2*)&m_pTexCoord[face.t[2]];

          if(GetBarycentricCoordinates(vTexCoord, t0, t1, t2, u, v, w, 0))
          {
            (*ppTriHitCache)[x*nTexSize+y] = group.arrTris[i];
            bFound = true;
            break;
          }
        }
      }

      ColorF rRGBA = Col_White;
      ColorB & bRGBA = pTexData[x*nTexSize+y] = Col_White;

      if(bFound)
      {
        vPos = m_pPositions[face.v[0]]*w + m_pPositions[face.v[1]]*v + m_pPositions[face.v[2]]*u;

        float arrProjWeight[3] = { 0, 0, 0 };

        {
          float vUVW[3] = { w, v, u };
          for(int v=0; v<3; v++)
          {
            int nProjId = GetVecProjId(m_pNorms[face.v[v]]);
            arrProjWeight[nProjId] += vUVW[v];
          }
        }

        int nSurfTypeId = 0;
        ColorB voxCol = Col_Black;
        Vec3 vPosWS = vMeshPos + vPos;
        m_pVoxTerrain->GetCachedVoxelValueInterpolated(vPosWS, 2.f, &nSurfTypeId, &voxCol);
        if(SImageInfo * pInfo = m_pVoxTerrain->GetBaseTextureData(nSurfTypeId))
        {
          ColorF colSumm = Col_Black;
          for(int nProjId=0; nProjId<3; nProjId++)
          {
            if(arrProjWeight[nProjId]>0)
            {
              float s=0, t=0;
              {
                float fScale = 1.f / fMeshNodeSize;
                switch(nProjId)
                {
                case 0:
                  s = vPosWS.y*fScale;
                  t = vPosWS.z*fScale;
                  break;

                case 1:
                  s = vPosWS.x*fScale;
                  t = vPosWS.z*fScale;
                  break;

                default:
                  s = vPosWS.x*fScale;
                  t = vPosWS.y*fScale;
                  break;
                }
              }

              s *= 4.f;
              t *= 4.f;

              while(s >= 1.f) s -= 1.f;
              while(s  < 0.f) s += 1.f;
              while(t >= 1.f) t -= 1.f;
              while(t  < 0.f) t += 1.f;

              float fX = s * pInfo->nDim;
              float fY = t * pInfo->nDim;

              ColorF colF = GetBilinearFilteredAt( fX, fY, (ColorB*)pInfo->pImage, pInfo->nDim );

              colSumm += arrProjWeight[nProjId] * colF;
            }
          }

          colSumm.Clamp(0.f,1.f);

          bRGBA = colSumm;
        }
      }
    }
  }

  return true;
}*/

int CIndexedMesh::GenerateTextureCoordinates(int nTexSizePlanar, float fMeshNodeSize, int * pOutTexSizeX, int * pOutTexSizeY, bool bLog,
                                             void * node, void * pIdx, PodArray<IRenderNode*> * pSrcBrushes, 
                                             ColorB ** pTexDatas, int nTexNum, float fRayLen)
{
  FUNCTION_PROFILER_3DENGINE;

  if(bLog)
    PrintMessagePlus(" generating texture coordinates ...");

  // generate connectivity info
  PodArray<VertInfo> arrVertInfo; arrVertInfo.PreAllocate(m_numVertices, m_numVertices);
  PodArray<int> arrFacesProjId; arrFacesProjId.PreAllocate(m_numFaces, m_numFaces);
  PodArray<VertInfo> arrFaceConnections; arrFaceConnections.PreAllocate(m_numFaces, m_numFaces);

  if(GetCVars()->e_VoxTerRelaxation)
  { // vertex relaxation
    ShareVertices(GetSystem(),m_bbox);

    for (int i=0; i<m_numFaces; i++) 
    {
      for (int v=0; v<3; v++)
      {
        VertInfo & rInfo = arrVertInfo[m_pFaces[i].v[v]];

        for(int n=v+1; n<=v+2; n++)
        {
          if(m_pFaces[i].v[n%3] != m_pFaces[i].v[v])
            if(rInfo.lstVertIds.Find(m_pFaces[i].v[n%3])<0)
              rInfo.lstVertIds.Add(m_pFaces[i].v[n%3]);
        }
      }
    }

    for(int nPass=0; nPass<GetCVars()->e_VoxTerRelaxation; nPass++)
    {
      Vec3 * pPositionsRelaxed = (Vec3*)malloc(sizeof(Vec3)*m_numVertices);

      AABB WSBBoxIn = m_bbox;
      Vec3 vBorder = m_bbox.GetSize()*0.05f;
      WSBBoxIn.min += vBorder;
      WSBBoxIn.max -= vBorder;

      float fMaxLen = vBorder.x*.5f;

      for(int v=0; v<m_numVertices; v++)
      {
        VertInfo & rInfo = arrVertInfo[v];

        if(rInfo.lstVertIds.Count() && Overlap::Point_AABB(m_pPositions[v],WSBBoxIn))
        {
          Vec3 vAverage(0,0,0);

          for(int n=0; n<rInfo.lstVertIds.Count(); n++)
            vAverage += m_pPositions[rInfo.lstVertIds[n]];

          vAverage /= (float)rInfo.lstVertIds.Count();

          Vec3 vDir = vAverage - m_pPositions[v];

          float fLen = vDir.GetLength();

          if(fLen>fMaxLen)
          {
            fLen = fMaxLen;
            vDir.SetLength(fLen);
          }

          pPositionsRelaxed[v] = m_pPositions[v] + vDir;
        }
        else
        {
          pPositionsRelaxed[v] = m_pPositions[v];
        }
      }

      free(m_pPositions);

      m_pPositions = pPositionsRelaxed;
    }
  }

  for (int i=0; i<m_numFaces; i++) 
  {
    // build list of faces for every vertex
    for (int v=0; v<3; v++)
      if(arrVertInfo[m_pFaces[i].v[v]].lstFacesIds.Find(i)<0)
        arrVertInfo[m_pFaces[i].v[v]].lstFacesIds.Add(i);

    // store face projection id
    Vec3 v0 = m_pPositions[m_pFaces[i].v[0]];
    Vec3 v1 = m_pPositions[m_pFaces[i].v[1]];
    Vec3 v2 = m_pPositions[m_pFaces[i].v[2]];
    
    Vec3 vNormal = (v1-v0).Cross(v2-v0).GetNormalized();
    
    if(GetCVars()->e_VoxTerHeightmapEditing && vNormal.z<=0.01f)
      vNormal = GetTerrain()->GetTerrainSurfaceNormal((v0+v1+v2)/3,.5f);
    
    int nProjId;
    if(GetCVars()->e_VoxTerPlanarProjection)
      nProjId = 4;
    else
      nProjId = GetVecProjId_6Axis(vNormal);

    arrFacesProjId[i] = nProjId;
  }

  // build for every face list of connected faces with same projection id
  for (int nMainFaceId=0; nMainFaceId<m_numFaces; nMainFaceId++) 
  {
    // build list of all faces used by all 3 vertices of the face (with duplicates)
    PodArray<int> allNeibFaces; allNeibFaces.Clear();
    for (int v=0; v<3; v++)
      allNeibFaces.AddList(arrVertInfo[m_pFaces[nMainFaceId].v[v]].lstFacesIds);

    PodArray<int> & connFaces = arrFaceConnections[nMainFaceId].lstFacesIds;

    // add faces duplicated more than once in allNeibFaces into connFaces
    for(int n=0; n<allNeibFaces.Count(); n++)
    {
      int nNeibFaceId = allNeibFaces[n];

      if(nMainFaceId != nNeibFaceId)
        if(connFaces.Find(nNeibFaceId) < 0)
          if(arrFacesProjId[nNeibFaceId] == arrFacesProjId[nMainFaceId])
      {
        int nFaceUsageNum=0;
        for(int c=0; c<allNeibFaces.Count(); c++)
          if(allNeibFaces[c] == nNeibFaceId)
            nFaceUsageNum++;

        if(nFaceUsageNum>1)
          connFaces.Add(nNeibFaceId);
      }
    }

    int nRes = connFaces.Count();
  }

  PodArray<SGroupInfo> arrGroups;

  if(GetCVars()->e_VoxTerPlanarProjection)
  {
    arrGroups.Add(SGroupInfo());
    SGroupInfo & newGroup = arrGroups.Last();
    for (int nFace=0; nFace<m_numFaces; nFace++) 
    {
      newGroup.arrTris.Add(nFace);
      arrFacesProjId[nFace] -= 100;
    }
  }
  else
  {
    int nStartFaceId=0;

    while(1)
    {
      // find next starting face
      for (; nStartFaceId<m_numFaces; nStartFaceId++) 
        if(arrFacesProjId[nStartFaceId]>=0)
          break;

      if(nStartFaceId==m_numFaces)
        break; // all faces processed

      arrGroups.Add(SGroupInfo());
      SGroupInfo & newGroup = arrGroups.Last();

      PodArray<uint32> arrFacesToProcess;
      arrFacesToProcess.Add(nStartFaceId);
      assert(arrFacesProjId[nStartFaceId]>=0);
      arrFacesProjId[nStartFaceId] -= 100;

      while(arrFacesToProcess.Count()) 
      {
        uint32 nMainFaceId = arrFacesToProcess.Last();

        arrFacesToProcess.DeleteLast();

        PodArray<int> & connFaces = arrFaceConnections[nMainFaceId].lstFacesIds;

        if(newGroup.arrTris.Find(nMainFaceId)>=0)
          continue;

        newGroup.arrTris.Add(nMainFaceId);

        for(int i=0; i<connFaces.Count(); i++)
        {
          if(arrFacesProjId[connFaces[i]] >= 0)
          {
            arrFacesProjId[connFaces[i]] -= 100;
            arrFacesToProcess.Add(connFaces[i]);
          }
        }
      }
    }
  }

  int nRes = arrGroups.Count();

  SetTexCoordsCount(m_numFaces*3);

  int nTexScaleRange = 4;
  int _nTexSize = nTexSizePlanar / nTexScaleRange;
  float fTexScale = 0.97f / fMeshNodeSize * (nTexSizePlanar / _nTexSize);

  for(int g=0; g<arrGroups.Count(); g++)
  {
    PodArray<uint32> & arrTris = arrGroups[g].arrTris;
    int nProjId = arrFacesProjId[arrTris[0]] + 100;
    assert(nProjId>=0 && nProjId<=5);
    for(int i=0; i<arrTris.Count(); i++)
    {
      SMeshFace & face = m_pFaces[arrTris[i]];
      for(int v=0; v<3; v++)
      {
        int id = arrTris[i]*3 + v;

        switch(nProjId)
        {
        case 0:
        case 1:
          m_pTexCoord[id].s = m_pPositions[face.v[v]].y*fTexScale;
          m_pTexCoord[id].t = m_pPositions[face.v[v]].z*fTexScale;
          break;

        case 2:
        case 3:
          m_pTexCoord[id].s = m_pPositions[face.v[v]].x*fTexScale;
          m_pTexCoord[id].t = m_pPositions[face.v[v]].z*fTexScale;
          break;

        case 4:
        case 5:
          m_pTexCoord[id].s = m_pPositions[face.v[v]].x*fTexScale;
          m_pTexCoord[id].t = m_pPositions[face.v[v]].y*fTexScale;
          break;

        case 6:
          m_pTexCoord[id].s = 0;
          m_pTexCoord[id].t = 0;
          break;

        default:
          assert(!"Undefined");
          break;
        }

        face.t[v] = id;

        arrGroups[g].aabb.Add(Vec3(m_pTexCoord[id].s, m_pTexCoord[id].t, 0));

//        m_pColor0[face.v[v]].r = (nProjId&1) ? 255 : 128; // R
  //      m_pColor0[face.v[v]].b = (nProjId&2) ? 255 : 128; // G
    //    m_pColor1[face.v[v]].b = (nProjId&4) ? 255 : 128; // B
      }
    }

    // normalize
    Vec3 vMin = arrGroups[g].aabb.min;
    for(int i=0; i<arrTris.Count(); i++)
    {
      for(int v=0; v<3; v++)
      {
        m_pTexCoord[arrTris[i]*3+v].s -= vMin.x;
        m_pTexCoord[arrTris[i]*3+v].t -= vMin.y;
      }
    }
    arrGroups[g].aabb.min -= vMin;
    arrGroups[g].aabb.max -= vMin;

    Vec3 vSize = arrGroups[g].aabb.GetSize();

    if(vSize.x>vSize.y)
    {
      for(int i=0; i<arrTris.Count(); i++)
      {
        for(int v=0; v<3; v++)
        {
          float tmp = m_pTexCoord[arrTris[i]*3+v].s;
          m_pTexCoord[arrTris[i]*3+v].s = m_pTexCoord[arrTris[i]*3+v].t;
          m_pTexCoord[arrTris[i]*3+v].t = tmp;
        }
      }

      AABB & aabb = arrGroups[g].aabb;

      {
        float tmp = aabb.min.x;
        aabb.min.x = aabb.min.y;
        aabb.min.y = tmp;
      }

      {
        float tmp = aabb.max.x;
        aabb.max.x = aabb.max.y;
        aabb.max.y = tmp;
      }
    }
  }

  // sort groups - big first
  qsort(arrGroups.GetElements(), arrGroups.Count(), sizeof(SGroupInfo), CIndexedMesh__Cmp_SGroupInfo);

  int nTexSizeX = _nTexSize;
  int nTexSizeY = _nTexSize;

  while(1)
  {
    bool bDoesNotFit = false;

    CMeshTexAtlas texAtlas(nTexSizeX,nTexSizeY);

    for(int g=0; g<arrGroups.Count(); g++)
    {
      SGroupInfo & group = arrGroups[g];

      Vec3 vSize = group.aabb.GetSize();
      int nOffsetX=0, nOffsetY=0;

      if(!texAtlas.AllocateGroup(&nOffsetX, &nOffsetY, (int)(vSize.x*nTexSizeX), (int)(vSize.y*nTexSizeY)))
      {
        bDoesNotFit = true;
        break;
      }

      // shift group
      Vec3 vOffset((float)nOffsetX/nTexSizeX, (float)nOffsetY/nTexSizeY, 0);
      for(int i=0; i<group.arrTris.Count(); i++)
      {
        SMeshFace & face = m_pFaces[group.arrTris[i]];
        for(int v=0; v<3; v++)
        {
          m_pTexCoord[face.t[v]].s += vOffset.x;
          m_pTexCoord[face.t[v]].t += vOffset.y;

          assert(face.t[v] == group.arrTris[i]*3+v);
        }
      }

      group.aabb.min += vOffset;
      group.aabb.max += vOffset;
    }

    if(bDoesNotFit)
    {
      if(nTexSizeX >= nTexSizePlanar * nTexScaleRange && nTexSizeY >= nTexSizePlanar * nTexScaleRange)
        break;

      float fScaleX=1.f;
      float fScaleY=1.f;

      /*if(GetCVars()->e_VoxTerTexBuildOnCPU)
      {
//        if(nTexSizeX<nTexSizeY)
        {
          if(nTexSizeX < nTexSizePlanar*nTexScaleRange)
          {
            nTexSizeX *= 2;
            fScaleX = 0.5f;
          }
        }
  //      else
        {
          if(nTexSizeY < nTexSizePlanar*nTexScaleRange)
          {
            nTexSizeY *= 2;
            fScaleY = 0.5f;
          }
        }
      }
      else*/
      {
        if(nTexSizeX<nTexSizeY)
        {
          if(nTexSizeX < nTexSizePlanar*nTexScaleRange)
          {
            nTexSizeX *= 2;
            fScaleX = 0.5f;
          }
        }
        else
        {
          if(nTexSizeY < nTexSizePlanar*nTexScaleRange)
          {
            nTexSizeY *= 2;
            fScaleY = 0.5f;
          }
        }
      }

      // downscale
      for(int g=0; g<arrGroups.Count(); g++)
      {
        SGroupInfo & group = arrGroups[g];

        Vec3 vMin = arrGroups[g].aabb.min;
        PodArray<uint32> & arrTris = arrGroups[g].arrTris;
        for(int i=0; i<arrTris.Count(); i++)
        {
          for(int v=0; v<3; v++)
          {
            m_pTexCoord[arrTris[i]*3+v].s -= vMin.x;
            m_pTexCoord[arrTris[i]*3+v].t -= vMin.y;

            m_pTexCoord[arrTris[i]*3+v].s *= fScaleX;
            m_pTexCoord[arrTris[i]*3+v].t *= fScaleY;
          }
        }

        arrGroups[g].aabb.min -= vMin;
        arrGroups[g].aabb.max -= vMin;

        arrGroups[g].aabb.min.x *= fScaleX;
        arrGroups[g].aabb.min.y *= fScaleY;

        arrGroups[g].aabb.max.x *= fScaleX;
        arrGroups[g].aabb.max.y *= fScaleY;
      }
    }
    else
      break;
  }

  if(pTexDatas)
    if(GetCVars()->e_VoxTerTexBuildOnCPU && ((pSrcBrushes && pSrcBrushes->Count()) || GetCVars()->e_VoxTerMixMask))
  {
    if(bLog)
      PrintMessagePlus(" computing texture image ...");
   
    int nPixelsDrawn = 0;

    if( GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask )
    {
      for(int t=0; t<nTexNum; t++)
      {
        pTexDatas[t] = new ColorB[nTexSizePlanar*nTexSizePlanar]; 
        for(int i=0; i<nTexSizePlanar*nTexSizePlanar; i++)
          pTexDatas[t][i] = ColorB(127,127,127,0);
      }

      FillTexturesPlanar( nTexSizePlanar, pTexDatas, nTexNum, nPixelsDrawn,fMeshNodeSize<=64.f, fMeshNodeSize<=128.f);
    }
    else
    {
      for(int t=0; t<nTexNum; t++)
      {
        pTexDatas[t] = new ColorB[nTexSizeX*nTexSizeY]; 
        for(int i=0; i<nTexSizeX*nTexSizeY; i++)
          pTexDatas[t][i] = ColorB(127,127,127,0);
      }

      int nTrisDrawn = 0;

      RasterizeTriangles( nTexSizeX, nTexSizeY, fMeshNodeSize, arrGroups, pTexDatas, nTexNum, node, pIdx, NULL, pSrcBrushes, Get3DEngine()->m_imageInfos, fRayLen, nTrisDrawn, nPixelsDrawn);

      if(bLog)
        PrintMessagePlus(" %d tris drawn ...", nTrisDrawn);
    }

    if(nPixelsDrawn)
    {
      /*if(GetCVars()->e_VoxTerTexDebug)
      {
        char szFileName[256]="";
        sprintf(szFileName, "GenTex_%d_%d.jpg", (int)fMeshNodeSize, GetMainFrameID());
        GetRenderer()->WriteJPG((byte*)pTexData,nTexSizeX,nTexSizeY,szFileName, 32);
      }*/

      if(bLog)
        PrintMessagePlus(" %d K pixels computed ...", nPixelsDrawn/1024);
    }
    else
    {
      for(int i=0; i<nTexNum; i++)
        SAFE_DELETE_ARRAY( pTexDatas[i] );
  
      if(bLog)
        PrintMessagePlus(" no pixels computed ...");
    }
  }

  {
    for(int g=0; g<arrGroups.Count(); g++)
      arrGroups[g].arrTris.Reset();

    for(int i=0; i<arrVertInfo.Count(); i++)
    {
      arrVertInfo[i].lstFacesIds.Reset();
      arrVertInfo[i].lstVertIds.Reset();
    }

    for(int i=0; i<arrFaceConnections.Count(); i++)
      arrFaceConnections[i].lstFacesIds.Reset();
  }

  *pOutTexSizeX = nTexSizeX;
  *pOutTexSizeY = nTexSizeY;

  return 0;
}

#include UNIQUE_VIRTUAL_WRAPPER(IIndexedMesh)
