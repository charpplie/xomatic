////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   statobjphys.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: make physical representation
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "StatObj.h"
#include "IndexedMesh.h"
#include "3dEngine.h"
#include "IParticles.h"
#include "CGFContent.h"
#include "ObjMan.h"
#include <CryThread.h>

#define SMALL_MESH_NUM_INDEX 30

#pragma warning(disable: 4244)

//////////////////////////////////////////////////////////////////////////
// Return tag name combined with number, ex: Name_1, Name_2
//////////////////////////////////////////////////////////////////////////
inline const char* numbered_tag(const char *s,unsigned int num)
{
	static char str[64];
	assert( strlen(s) < 20 );
	strcpy( str,s );
	sprintf( str+strlen(s),"_%d",num );
	return str;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::PhysicalizeCompiled( CNodeCGF *pNode, int bAppend )
{
  FUNCTION_PROFILER_3DENGINE;

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Physics, 0, "Physics");

	if(!GetPhysicalWorld())
		return;

	CMesh *pMesh = pNode->pMesh;
	bool bMeshNotNeeded = (pNode->nPhysicalizeFlags & CNodeCGF::ePhysicsalizeFlag_MeshNotNeeded) != 0;
	if (bMeshNotNeeded)
	{
		// Solves a crash in loading solid brushes for now, remove later
		//pMesh = 0;
	}

	bool bHavePhysicsData = false;
	for (int nPhysGeomType = 0; nPhysGeomType < 4; nPhysGeomType++)
	{
		if (!pNode->physicalGeomData[nPhysGeomType].empty())
		{
			bHavePhysicsData = true;
			break;
		}
	}
	if (!bHavePhysicsData)
		return;

	std::vector<char> faceMaterials;
	if (pMesh)
	{
		CMesh &mesh = *pMesh;
		// Fill faces material array.
		faceMaterials.resize( mesh.GetIndexCount()/3 );
		if (!faceMaterials.empty())
		{
			for (int m = 0; m < mesh.GetSubSetCount(); m++)
			{
				char *pFaceMats = &faceMaterials[0];
				SMeshSubset &subset = mesh.m_subsets[m];
				for (int nFaceIndex = subset.nFirstIndexId/3, last = subset.nFirstIndexId/3 + subset.nNumIndices/3; nFaceIndex < last; nFaceIndex++)
				{
					pFaceMats[nFaceIndex] = subset.nMatID;
				}
			}
		}
	}

	for (int nPhysGeomType = 0; nPhysGeomType < 4; nPhysGeomType++)
	{
		DynArray<char> &physData = pNode->physicalGeomData[nPhysGeomType];
		if (!physData.empty())
		{
			char *pFaceMats = 0;
			if (!faceMaterials.empty())
				pFaceMats = &faceMaterials[0];

			phys_geometry *pPhysGeom = 0;

			// We have physical geometry for this node.
			CMemStream stm( &physData.front(), physData.size(), true );
			if (pMesh)
			{
				CMesh &mesh = *pMesh;
				// Load using mesh information.
				pPhysGeom = GetPhysicalWorld()->GetGeomManager()->LoadPhysGeometry(stm,mesh.m_pPositions,mesh.m_pIndices,pFaceMats);
			}
			else
			{
				// Load only using physics saved data.
				pPhysGeom = GetPhysicalWorld()->GetGeomManager()->LoadPhysGeometry(stm,0,0,0);
			}
			if (!pPhysGeom)
			{
				FileWarning( 0,GetFilePath(),"Bad physical data in CGF node, Physicalization Failed, (%s)",GetFilePath() );
				return;
			}
      AssignPhysGeom( nPhysGeomType+PHYS_GEOM_TYPE_DEFAULT,pPhysGeom,bAppend );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::PhysicalizeGeomType( int nGeomType, CMesh &mesh, float tolerance, int bAppend)
{
	std::vector<uint16> indices;
	std::vector<char> faceMaterials;
	indices.reserve( mesh.GetIndexCount() );
	faceMaterials.reserve( mesh.GetIndexCount()/3 );

	for (int m = 0; m < mesh.GetSubSetCount(); m++)
	{
		SMeshSubset &subset = mesh.m_subsets[m];

		if (subset.nPhysicalizeType != nGeomType && !(subset.nPhysicalizeType==PHYS_GEOM_TYPE_DEFAULT_PROXY && nGeomType==PHYS_GEOM_TYPE_DEFAULT))
			continue;

		for (int idx = subset.nFirstIndexId; idx < subset.nFirstIndexId+subset.nNumIndices; idx += 3)
		{
			indices.push_back( mesh.m_pIndices[idx] );
			indices.push_back( mesh.m_pIndices[idx+1] );
			indices.push_back( mesh.m_pIndices[idx+2] );
			faceMaterials.push_back( subset.nMatID );
		}
	}

	if (indices.empty())
		return false;

	Vec3 *pVertices = NULL;
	int nVerts = mesh.GetVertexCount();

	pVertices = mesh.m_pPositions;

	if (nVerts > 2)
	{
		Vec3 ptmin = mesh.m_bbox.max;
		Vec3 ptmax = mesh.m_bbox.min;

		int flags = mesh_multicontact1;
		//float tol = 0.05f;
		flags |= indices.size() <= SMALL_MESH_NUM_INDEX ? mesh_SingleBB : mesh_OBB|mesh_AABB;
		flags |= mesh_approx_box | mesh_approx_sphere | mesh_approx_cylinder | mesh_approx_capsule;
		flags |= mesh_shared_foreign_idx; // when this flags is set and pForeignIdx is 0, the physics assumes an array of fidx[i]==i

		if (m_bEditor)
			flags |= mesh_keep_vtxmap_for_saving;

		flags |= mesh_full_serialization;

		//////////////////////////////////////////////////////////////////////////
		/*
		if (strstr(m_szGeomName,"wheel"))
		{
		flags |= mesh_approx_cylinder;
		tol = 1.0f;
		} else if (strstr(m_szFileName,"explosion") || strstr(m_szFileName,"crack"))
		flags = flags & ~mesh_multicontact1 | mesh_multicontact2;	// temporary solution
		else
		flags |= mesh_approx_box | mesh_approx_sphere | mesh_approx_cylinder;
		*/

		int nMinTrisPerNode = 2;
		int nMaxTrisPerNode = 4;
		Vec3 size = ptmax - ptmin;
		if (indices.size() < 600 && max(max(size.x,size.y),size.z) > 6) // make more dense OBBs for large (wrt terrain grid) objects
			nMinTrisPerNode = nMaxTrisPerNode = 1;

		//////////////////////////////////////////////////////////////////////////
		// Create physical mesh.
		//////////////////////////////////////////////////////////////////////////
		IGeomManager *pGeoman = GetPhysicalWorld()->GetGeomManager();
		IGeometry *pGeom = pGeoman->CreateMesh(
			pVertices,
			&indices[0],
			&faceMaterials[0],
			0,
			indices.size()/3,
			flags, tolerance, nMinTrisPerNode,nMaxTrisPerNode, 2.5f );

		phys_geometry *pPhysGeom = pGeoman->RegisterGeometry( pGeom,faceMaterials[0] );
		AssignPhysGeom( nGeomType,pPhysGeom,bAppend );
		pGeom->Release();
	}

	// Free temporary verts array.
	if (pVertices != mesh.m_pPositions)
		delete [] pVertices;

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::AssignPhysGeom( int nGeomType,phys_geometry *pPhysGeom,int bAppend )
{
	assert(pPhysGeom);
	m_arrPhysGeomInfo.SetPhysGeom(pPhysGeom, m_arrPhysGeomInfo.GetGeomCount()&-bAppend | nGeomType&~-bAppend, nGeomType);

	if (pPhysGeom->pGeom && !m_bNoHitRefinement)
	{
		pPhysGeom->pGeom->SetForeignData((IStatObj*)this,0);
	}

	//////////////////////////////////////////////////////////////////////////
	// Set material mapping to physics geometry.
	//////////////////////////////////////////////////////////////////////////
	if (m_pMaterial)
	{
		int surfaceTypesId[MAX_SUB_MATERIALS], flagsPhys[MAX_SUB_MATERIALS];
		memset( surfaceTypesId,0,sizeof(surfaceTypesId) );
		int i,j,numIds = m_pMaterial->FillSurfaceTypeIds(surfaceTypesId);

		if (nGeomType==PHYS_GEOM_TYPE_DEFAULT)
		{
			ISurfaceTypeManager *pSurfaceTypeManager = Get3DEngine()->GetMaterialManager()->GetSurfaceTypeManager();
			ISurfaceType *pMat;
			float bounciness,friction;
			unsigned int flags = 0;

			for (i=0; i<numIds; i++)
			{
				GetPhysicalWorld()->GetSurfaceParameters( surfaceTypesId[i],bounciness,friction,flags );
				flagsPhys[i] = flags >> sf_matbreakable_bit;
				if ((pMat=pSurfaceTypeManager->GetSurfaceType(surfaceTypesId[i])) && 
						(pMat->GetFlags() & SURFACE_TYPE_VEHICLE_ONLY_COLLISION))
				{
					flagsPhys[i] |= (int)1<<16;
				}
				if (flags & sf_manually_breakable)
					flagsPhys[i] |= (int)1<<17;
			}

			mesh_data *pmd;
			m_idmatBreakable=-1; m_bVehicleOnlyPhysics=0;	m_bBreakableByGame=0;
			assert(pPhysGeom->pGeom);
			if ((i=pPhysGeom->pGeom->GetType())==GEOM_TRIMESH || i==GEOM_VOXELGRID)
				for(pmd=(mesh_data*)pPhysGeom->pGeom->GetData(),i=0; i<pmd->nTris; i++)
				{
					j = pmd->pMats[i]; j &= -isneg(j-numIds);
					m_idmatBreakable = max(m_idmatBreakable, (flagsPhys[j] & 0xFFFF)-1);
					m_bVehicleOnlyPhysics |= flagsPhys[j]>>16 & 1;
					m_bBreakableByGame |= flagsPhys[j]>>17;
				}	else if (pPhysGeom->surface_idx>=0)
				{
					j = pPhysGeom->surface_idx & -isneg(pPhysGeom->surface_idx-numIds);
					m_bVehicleOnlyPhysics = flagsPhys[j]>>16;
					m_bBreakableByGame |= flagsPhys[j]>>17;
				}
		}

		GetPhysicalWorld()->GetGeomManager()->SetGeomMatMapping( pPhysGeom,surfaceTypesId,numIds );

		if (m_bBreakableByGame) 
			m_nFlags |= STATIC_OBJECT_DYNAMIC;
	}
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::SavePhysicalizeData( CNodeCGF *pNode )
{
	pNode->nPhysicalizeFlags = CNodeCGF::ePhysicsalizeFlag_MeshNotNeeded;
	for (int nGeomType = 0; nGeomType < MAX_PHYS_GEOMS_TYPES; nGeomType++)
	{
		if (!m_arrPhysGeomInfo[nGeomType])
			continue;

		CMemStream stm(false);
		IGeomManager *pGeoman = GetPhysicalWorld()->GetGeomManager();
		pGeoman->SavePhysGeometry( stm,m_arrPhysGeomInfo[nGeomType] );

		// Add physicalized data to the node.
		pNode->physicalGeomData[nGeomType].resize(stm.GetUsedSize());
		memcpy( &pNode->physicalGeomData[nGeomType][0],stm.GetBuf(),stm.GetUsedSize() );
	}
}


//////////////////////////////////////////////////////////////////////////
///////////////////////// Breakable Geometry /////////////////////////////
//////////////////////////////////////////////////////////////////////////

template<class T> struct SplitArray {
	T *ptr[2];
	T& operator[](int idx) { return ptr[idx>>15][idx & ~(1<<15)]; }
	T* operator+(int idx) { return ptr[idx>>15]+(idx & ~(1<<15)); }
	T* operator-(int idx) { return ptr[idx>>15]-(idx & ~(1<<15)); }
};

inline int mapiTri(int itri, int *pIdx2iTri) {
	int mask = (itri-BOP_NEWIDX0)>>31; 
	return itri&mask | pIdx2iTri[itri-BOP_NEWIDX0|mask];
}

static inline void swap(int *pSubsets,uint16 *pidx,int *pmap, int i1,int i2) {	
	int i = pSubsets[i1]; pSubsets[i1] = pSubsets[i2]; pSubsets[i2] = i;
	i = pmap[i1]; pmap[i1] = pmap[i2]; pmap[i2] = i;
	for(i=0;i<3;i++) {
		uint16 u = pidx[i1*3+i]; pidx[i1*3+i]=pidx[i2*3+i]; pidx[i2*3+i] = u;
	}
}
static void qsort(int *pSubsets,uint16 *pidx,int *pmap, int ileft,int iright, int iter=0)
{
	if (ileft>=iright) return;
	int i,ilast,diff=0; 
	swap(pSubsets,pidx,pmap, ileft,(ileft+iright)>>1);
	for(ilast=ileft,i=ileft+1; i<=iright; i++)
	{
		diff |= pSubsets[i]-pSubsets[ileft];
		if (pSubsets[i] < pSubsets[ileft]+iter)	// < when iter==0 and <= when iter==1
			swap(pSubsets,pidx,pmap, ++ilast,i);
	}
	swap(pSubsets,pidx,pmap, ileft,ilast);

	if (diff) 
	{
		qsort(pSubsets,pidx,pmap, ileft,ilast-1, iter^1);
		qsort(pSubsets,pidx,pmap, ilast+1,iright, iter^1);
	}
}

inline int check_mask(unsigned int *pMask, int i) {
	return pMask[i>>5]>>(i&31) & 1;
}
inline void set_mask(unsigned int *pMask, int i) {
	pMask[i>>5] |= 1u<<(i&31);
}
inline void clear_mask(unsigned int *pMask, int i) {
	pMask[i>>5] &= ~(1u<<(i&31));
}

inline Vec3 UnpVec4sf(const Vec4sf &v) { return Vec3(tPackB2F(v.x),tPackB2F(v.y),tPackB2F(v.z)); }
inline Vec4sf PakVec4sf(const Vec3 &v,int w) { return Vec4sf(tPackF2B(v.x),tPackF2B(v.y),tPackF2B(v.z),w); }

inline Vec4 clr2vec4(const SMeshColor &clr) { return Vec4(clr.r,clr.g,clr.b,clr.a); }
inline SMeshColor vec42clr(const Vec4 &vec) { SMeshColor clr={ FtoI(vec.x),FtoI(vec.y),FtoI(vec.z),FtoI(vec.w) }; return clr; }

int InjectVertices(CMesh *pMesh, int nNewVtx, SMeshBoneMapping *&pBoneMapping)
{
	int i,j,nVtx0=pMesh->m_numVertices,nVtx,nMovedSubsets=0;

	nVtx = pMesh->m_numVertices+nNewVtx;
	pMesh->ReallocStream(CMesh::POSITIONS, nVtx);
	pMesh->ReallocStream(CMesh::NORMALS, nVtx);
	pMesh->ReallocStream(CMesh::TEXCOORDS, nVtx);
	pMesh->ReallocStream(CMesh::TANGENTS, nVtx);
	if (pMesh->m_pColor0)
		pMesh->ReallocStream(CMesh::COLORS_0, nVtx);
	if (pMesh->m_pSHInfo)
		pMesh->ReallocStream(CMesh::SHCOEFFS, nVtx);

	for(i=(int)pMesh->m_subsets.size()-1; i>=0; i--) if (pMesh->m_subsets[i].nPhysicalizeType!=PHYS_GEOM_TYPE_DEFAULT)
	{	// move vertices in non-physicalized subsets up to make space for the vertices added by physics
		if (nNewVtx>0) 
		{
			j = pMesh->m_subsets[i].nFirstVertId;
			memmove(pMesh->m_pPositions+j+nNewVtx, pMesh->m_pPositions+j, sizeof(pMesh->m_pPositions[0])*pMesh->m_subsets[i].nNumVerts);
			memmove(pMesh->m_pNorms+j+nNewVtx, pMesh->m_pNorms+j, sizeof(pMesh->m_pNorms[0])*pMesh->m_subsets[i].nNumVerts);
			memmove(pMesh->m_pTexCoord+j+nNewVtx, pMesh->m_pTexCoord+j, sizeof(pMesh->m_pTexCoord[0])*pMesh->m_subsets[i].nNumVerts);
			if (pMesh->m_pTangents)
				memmove(pMesh->m_pTangents+j+nNewVtx, pMesh->m_pTangents+j, sizeof(pMesh->m_pTangents[0])*pMesh->m_subsets[i].nNumVerts);
			if (pMesh->m_pQTangents)
				memmove(pMesh->m_pQTangents+j+nNewVtx, pMesh->m_pQTangents+j, sizeof(pMesh->m_pQTangents[0])*pMesh->m_subsets[i].nNumVerts);
			if (pMesh->m_pColor0)
				memmove(pMesh->m_pColor0+j+nNewVtx, pMesh->m_pColor0+j, sizeof(pMesh->m_pColor0[0])*pMesh->m_subsets[i].nNumVerts);
			if (pMesh->m_pSHInfo)
				memmove(pMesh->m_pSHInfo->pSHCoeffs+j+nNewVtx, pMesh->m_pSHInfo->pSHCoeffs+j, 
					sizeof(pMesh->m_pSHInfo->pSHCoeffs[0])*pMesh->m_subsets[i].nNumVerts);
			pMesh->m_subsets[i].nFirstVertId += nNewVtx;
			for(j=pMesh->m_subsets[i].nFirstIndexId; j<pMesh->m_subsets[i].nFirstIndexId+pMesh->m_subsets[i].nNumIndices; j++)
				pMesh->m_pIndices[j] += nNewVtx;
		}
		if (pMesh->m_subsets[i].nNumVerts)
			nVtx0=pMesh->m_subsets[i].nFirstVertId, nMovedSubsets++;
	}
	if (pBoneMapping && nNewVtx>0)
	{
		pBoneMapping = (SMeshBoneMapping*)CryModuleRealloc(pBoneMapping,nVtx*sizeof(SMeshBoneMapping));
		if (nMovedSubsets>0)
			memmove(pBoneMapping+nVtx0, pBoneMapping+nVtx0-nNewVtx, (nVtx-nVtx0)*sizeof(SMeshBoneMapping));
		memset(pBoneMapping, 0, nVtx0*sizeof(SMeshBoneMapping));
	}

	return nVtx0;
}


IStatObj *C3DEngine::UpdateDeformableStatObj(IGeometry *pPhysGeom, bop_meshupdate *pLastUpdate, IFoliage *pSrcFoliage)
{
	int i,j,j1,itri,iop,itriOrg,ivtx,ivtxNew,nTris0,nVtx0,iSubsetB=0,nNewVtx,nNewPhysVtx,ivtxRef,bEmptyObj,wt,wb,ibox;
	int *pIdx2iTri,*piTri2Idx,*pSubsets,*pSubsets0,*pVtxWeld;
	int bHasColors[2] = {0,0};
	uint16 *pIdx0,*pIdx;
	float dist[3];
	Vec4 clrf;
	unsigned int *pRemovedVtxMask;
	IIndexedMesh *pIndexedMesh[2];
	mesh_data *pPhysMesh/*,*pPhysMesh1*/;
	bop_meshupdate *pmu,*pmu0;
	CMesh *pMesh[2],meshAppend;
	IStatObj *pStatObj[2],*pObjClone;
	IMaterial *pMaterial[2];
	SplitArray<Vec3> pVtx,pNormals[2];
	SplitArray<SMeshTexCoord> pTexCoord[2];
	SplitArray<SMeshTangents> pTangents[2];
	SplitArray<SMeshColor> pColors[2];
	SplitArray<int> pVtxMap;
	SMeshSubset mss;
	SMeshTexCoord tex0;
	SMeshColor clrDummy;
	Vec3 Tangent,Binormal,area,n;
	const char *matname;
	AABB BBox;
	int minVtx, maxVtx;
static int g_ncalls=0;
g_ncalls++;

	FUNCTION_PROFILER_3DENGINE

#define AllocVtx																																								\
	if ((nNewVtx & 31)==0)																																				\
	{																																															\
		meshAppend.ReallocStream(CMesh::POSITIONS, nNewVtx+32);																			\
		meshAppend.ReallocStream(CMesh::NORMALS, nNewVtx+32);																				\
		meshAppend.ReallocStream(CMesh::TEXCOORDS, nNewVtx+32);																			\
		meshAppend.ReallocStream(CMesh::TANGENTS, nNewVtx+32);																			\
		if (bHasColors[0])	{																																				\
			meshAppend.ReallocStream(CMesh::COLORS_0, nNewVtx+32);																		\
			pColors[0].ptr[1] = meshAppend.m_pColor0;																									\
		}																																														\
		pVtxMap.ptr[1] = (int*)CryModuleRealloc(pVtxMap.ptr[1],(nNewVtx+32)*sizeof(pVtxMap.ptr[1][0]));			\
		pVtx.ptr[1] = meshAppend.m_pPositions; pNormals[0].ptr[1] = meshAppend.m_pNorms;						\
		pTexCoord[0].ptr[1] = meshAppend.m_pTexCoord; pTangents[0].ptr[1] = meshAppend.m_pTangents;	\
	}																																															\
	ivtx = nNewVtx++ | 1<<15

	pmu0 = (bop_meshupdate*)pPhysGeom->GetForeignData(DATA_MESHUPDATE);
	pStatObj[0] = (IStatObj*)pPhysGeom->GetForeignData();
	if (!pmu0)
		return pStatObj[0];

	if (!pStatObj[0])
	{
		(pStatObj[0] = CreateStatObj());//->AddRef();
		pStatObj[0]->SetFlags(STATIC_OBJECT_CLONE|STATIC_OBJECT_GENERATED);
		((CStatObj*)pStatObj[0])->m_bTmpIndexedMesh = true;
	} else if (!(pStatObj[0]->GetFlags() & STATIC_OBJECT_CLONE)) 
	{
		if (pStatObj[0]->GetFlags() & STATIC_OBJECT_CANT_BREAK)
		{
			CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR, "Error: breaking StatObj unsuitable for breaking (%s)",pStatObj[0]->GetFilePath());
			return pStatObj[0];
		}
		(pObjClone = CreateStatObj());//->AddRef();
		if (!pStatObj[0]->GetIndexedMesh(true) || !pObjClone->GetIndexedMesh(true))
		{
			pObjClone->Release();
			CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR, "Error: breaking StatObj without a mesh (%s)",pStatObj[0]->GetFilePath());
			pStatObj[0]->SetFlags(pStatObj[0]->GetFlags() | STATIC_OBJECT_CANT_BREAK);
			return pStatObj[0];
		}
		pObjClone->GetIndexedMesh(true)->GetMesh()->Copy( *(pMesh[0]=pStatObj[0]->GetIndexedMesh(true)->GetMesh()) ); 
		pObjClone->SetMaterial( pStatObj[0]->GetMaterial() );
		pObjClone->SetFilePath(pStatObj[0]->GetFilePath());
		pStatObj[0]->CopyFoliageData(pObjClone,false,pSrcFoliage);
		(pStatObj[0] = pObjClone)->SetFlags(pStatObj[0]->GetFlags() | STATIC_OBJECT_CLONE|STATIC_OBJECT_GENERATED);
	}
	pPhysGeom->SetForeignData(pStatObj[0],0);
	if (((CStatObj*)pStatObj[0])->m_pSrcPhysMesh!=pPhysGeom) 
	{
		if (((CStatObj*)pStatObj[0])->m_pSrcPhysMesh)
			((CStatObj*)pStatObj[0])->m_pSrcPhysMesh->Release();
		(((CStatObj*)pStatObj[0])->m_pSrcPhysMesh = pPhysGeom)->AddRef();
	}
  if (gEnv->pSystem->IsDedicated())
		return pStatObj[0];
	if (!(pIndexedMesh[0] = pStatObj[0]->GetIndexedMesh(true)))
	{
		CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR, "Error: breaking StatObj without a mesh (%s)",pStatObj[0]->GetFilePath());
		pStatObj[0]->SetFlags(pStatObj[0]->GetFlags() | STATIC_OBJECT_CANT_BREAK);
		return pStatObj[0];
	}
	pPhysGeom->Lock();
	pPhysMesh = (mesh_data*)pPhysGeom->GetData();
	pMesh[0] = pIndexedMesh[0]->GetMesh();
	bHasColors[0] = pMesh[0]->m_pColor0!=0;
	nNewVtx = 0;
	pVtx.ptr[0] = pMesh[0]->m_pPositions; pNormals[0].ptr[0] = pMesh[0]->m_pNorms;
	pTexCoord[0].ptr[0] = pMesh[0]->m_pTexCoord; pTangents[0].ptr[0] = pMesh[0]->m_pTangents; 
	pColors[0].ptr[0] = pMesh[0]->m_pColor0; pVtxMap.ptr[0] = pPhysMesh->pVtxMap;
	memset(pVtxWeld = new int[pPhysMesh->nVertices], -1, pPhysMesh->nVertices*sizeof(int));
	memset(pRemovedVtxMask = new unsigned int[((pPhysMesh->nVertices-1)>>5)+1], 0, (((pPhysMesh->nVertices-1)>>5)+1)*sizeof(int));
	pVtxMap.ptr[1] = 0;
	pVtx.ptr[1] = 0;
	BBox.Reset();

	nTris0 = pMesh[0]->m_nIndexCount/3;
	// if filling a new mesh, reserve space for all source mesh indices, since we might copy them later as non-physical parts
	if (nTris0==0 && (pStatObj[1]=(IStatObj*)pmu0->pMesh[1]->GetForeignData()) && (pIndexedMesh[1]=pStatObj[1]->GetIndexedMesh(true)))
			nTris0 = pIndexedMesh[1]->GetMesh()->m_nIndexCount/3;	
	for(pmu=pmu0,itri=0; pmu; pmu=pmu->next) 
	{
		nTris0 += pmu->nNewTri;
		for(i=0;i<pmu->nNewTri;i++)
			itri = max(itri,pmu->pNewTri[i].idxNew-BOP_NEWIDX0);
		if (pmu==pLastUpdate)
			break;
	}

	// reserve space for the new vertices	(take into account even those that were added after pLastUpdate)
	nVtx0 = InjectVertices(pMesh[0], 0, ((CStatObj*)pStatObj[0])->m_pBoneMapping);
	InjectVertices(pMesh[0], nNewPhysVtx = max(0,pPhysMesh->nVertices-nVtx0), ((CStatObj*)pStatObj[0])->m_pBoneMapping);
	pVtx.ptr[0]=pMesh[0]->m_pPositions; pNormals[0].ptr[0]=pMesh[0]->m_pNorms;
	pTexCoord[0].ptr[0]=pMesh[0]->m_pTexCoord; pTangents[0].ptr[0]=pMesh[0]->m_pTangents;	pColors[0].ptr[0]=pMesh[0]->m_pColor0;
	pColors[0].ptr[1] = &clrDummy; clrDummy.r=clrDummy.g=clrDummy.b=128; clrDummy.a=255;

	nTris0 = max(nTris0,itri+1); 
	piTri2Idx = new int[nTris0]; pSubsets = new int[nTris0];
	pIdx2iTri = (new int[nTris0+1])+1; 
	for(i=0;i<nTris0;i++) piTri2Idx[i]=i,pSubsets[i]=1<<30;	pIdx2iTri[-1] = 0;
	for(i=0;i<(int)pMesh[0]->m_subsets.size();i++)
		for(j=pMesh[0]->m_subsets[i].nFirstIndexId/3; j*3<pMesh[0]->m_subsets[i].nFirstIndexId+pMesh[0]->m_subsets[i].nNumIndices; j++)
			pSubsets[j] = i + (0x1000 & ~-iszero(pMesh[0]->m_subsets[i].nPhysicalizeType-PHYS_GEOM_TYPE_DEFAULT));
	pMaterial[0] = pStatObj[0]->GetMaterial();

	for(pmu=pmu0; pmu; pmu=pmu->next) 
	{
		pStatObj[1] = (IStatObj*)pmu->pMesh[1]->GetForeignData();
		if (!pStatObj[1] || !(pIndexedMesh[1] = pStatObj[1]->GetIndexedMesh(true)))
		{
			pStatObj[0]->FreeIndexedMesh(); // to prevent further problems - we can't recover in this case
			pPhysGeom->SetForeignData(0,DATA_MESHUPDATE);
			pStatObj[0]->SetFlags(pStatObj[0]->GetFlags() | STATIC_OBJECT_CANT_BREAK);
			goto ExitUDS;
		}
		if (pStatObj[1]->GetFlags() & STATIC_OBJECT_CANT_BREAK)
		{
			CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR, "Error: breaking StatObj unsuitable for breaking (%s)",pStatObj[1]->GetFilePath());
			pPhysGeom->SetForeignData(0,DATA_MESHUPDATE);
			pStatObj[0]->SetFlags(pStatObj[0]->GetFlags() | STATIC_OBJECT_CANT_BREAK);
			goto ExitUDS;
		}
		if (pStatObj[1] && pStatObj[1]!=pStatObj[0])
		{
			((CStatObj*)pStatObj[0])->m_pLastBooleanOp = pStatObj[1];
			((CStatObj*)pStatObj[0])->m_lastBooleanOpScale = pmu->relScale;
		}
		if (pmu->pMesh[1]!=pmu->pMesh[0])
			pmu->pMesh[1]->Lock(0);
		pMesh[1] = pIndexedMesh[1]->GetMesh();
//		pPhysMesh1 = (mesh_data*)pmu->pMesh[1]->GetData();
		pNormals[1].ptr[0] = pMesh[1]->m_pNorms;
		pTexCoord[1].ptr[0] = pMesh[1]->m_pTexCoord;
		pTangents[1].ptr[0] = pMesh[1]->m_pTangents; 
		bHasColors[1] = (pColors[1].ptr[0] = pMesh[1]->m_pColor0)!=0;

		nTris0 = pMesh[0]->GetIndexCount()/3;
		pMesh[1] = pIndexedMesh[1]->GetMesh();
		// preserve previous indixes and subset ids, because new tris can overwrite tri slots before we need them
		memcpy(pIdx0 = new uint16[pMesh[0]->m_nIndexCount], pMesh[0]->m_pIndices, pMesh[0]->m_nIndexCount*sizeof(pIdx0[0]));

		if (pMesh[0]->m_nIndexCount)
		{
			pMaterial[1] = pStatObj[1]->GetMaterial();
			if (pMaterial[1]->GetSubMtlCount()>0)
				pMaterial[1] = pMaterial[1]->GetSubMtl(0);
			matname = pMaterial[1]->GetName();
			for(i=pMaterial[0]->GetSubMtlCount()-1; i>=0 && strcmp(pMaterial[0]->GetSubMtl(i)->GetName(),matname); i--);
			if (i<0)
			{	// append material slot to the destination material
				i = pMaterial[0]->GetSubMtlCount(); pMaterial[0]->SetSubMtlCount(i+1);
				pMaterial[0]->SetSubMtl(i, pMaterial[1]);
				iSubsetB = -1;
			} else	// find the subset that uses this slot
				for(iSubsetB=pMesh[0]->m_subsets.size()-1; iSubsetB>=0 && pMesh[0]->m_subsets[iSubsetB].nMatID!=i; iSubsetB--);
			if (iSubsetB<0)
			{	// append the slot that uses this material
				mss = pMesh[1]->m_subsets[0];
				mss.nFirstVertId = mss.nFirstIndexId = 0;
				mss.nNumIndices = mss.nNumVerts = 0;
				mss.nMatID = i;
				iSubsetB = pMesh[0]->m_subsets.size();
				pMesh[0]->m_subsets.push_back(mss);
			}
			memcpy(pSubsets0 = new int[nTris0], pSubsets, nTris0*sizeof(pSubsets0[0]));
			bEmptyObj = 0;
		}
		else
		{	// if we are creating an object from scratch, always copy materials and subsets verbatim from pMesh[1]
			pStatObj[0]->SetMaterial(pStatObj[1]->GetMaterial());

			pSubsets0 = new int[pMesh[1]->m_nIndexCount/3];
			for(i=pMesh[1]->m_nIndexCount/3-1;i>=0;i--) pSubsets0[i] = 1<<30;
			for(i=0;i<(int)pMesh[1]->m_subsets.size();i++)
			{
				pMesh[0]->m_subsets.push_back(pMesh[1]->m_subsets[i]);
				pMesh[0]->m_subsets[i].nFirstIndexId = pMesh[0]->m_subsets[i].nFirstVertId = 0;
				pMesh[0]->m_subsets[i].nNumVerts = pMesh[0]->m_subsets[i].nNumIndices = 0;
				for(j=pMesh[1]->m_subsets[i].nFirstIndexId/3; j*3<pMesh[1]->m_subsets[i].nFirstIndexId+pMesh[1]->m_subsets[i].nNumIndices; j++)
					pSubsets0[j] = i;
				if (pMesh[0]->m_pSHInfo && pMesh[0]->m_pSHInfo->pDecompressions && pMesh[1]->m_pSHInfo && pMesh[1]->m_pSHInfo->pDecompressions)
					pMesh[0]->m_pSHInfo->pDecompressions[i] = pMesh[1]->m_pSHInfo->pDecompressions[i];
			}
			bEmptyObj = 1;
			//bHasColors[0] |= bHasColors[1];
		}
		if (!pColors[0].ptr[0]) pColors[0].ptr[0] = &clrDummy;
		if (!pColors[1].ptr[0]) pColors[1].ptr[0] = &clrDummy;

		// reserve space for the added triangles
		pMesh[0]->SetIndexCount(pMesh[0]->GetIndexCount()+max(0,pmu->nNewTri-pmu->nRemovedTri)*3);

		for(i=0; i<pmu->nNewVtx; i++)
		{ // copy positions of the new vertices from physical vertices (they can also be shared)
			pVtx[pmu->pNewVtx[i].idx] = pPhysMesh->pVertices[pmu->pNewVtx[i].idx];
			pNormals[0][pmu->pNewVtx[i].idx] = Vec3(0,0,1);
			pTexCoord[0][pmu->pNewVtx[i].idx].s=pTexCoord[0][pmu->pNewVtx[i].idx].t = 0;
			pTangents[0][pmu->pNewVtx[i].idx].Tangent = Vec4sf(0,0,0,0);
			pTangents[0][pmu->pNewVtx[i].idx].Binormal = Vec4sf(0,0,0,0);
			pColors[0][pmu->pNewVtx[i].idx & -bHasColors[0]] = clrDummy;
		}

		// mark removed vertices from pmu->pRemovedVtx, make new render vertices that map to them map to themselves
		for(i=0; i<pmu->nRemovedVtx; i++)
			set_mask(pRemovedVtxMask, pmu->pRemovedVtx[i]);
		for(i=0; i<nNewVtx; i++) if (check_mask(pRemovedVtxMask, pVtxMap.ptr[1][i]))
			pVtxMap.ptr[1][i] = pMesh[0]->m_numVertices+i;
		for(i=0; i<pmu->nRemovedVtx; i++)
			clear_mask(pRemovedVtxMask, pmu->pRemovedVtx[i]);

		if (pmu->nWeldedVtx)
		{ // for welded vertices (which result from mesh filtering) and those that map to them, just force positions to be the same as destinations
			for(i=0; i<pmu->nWeldedVtx; i++)
				pVtxWeld[pmu->pWeldedVtx[i].ivtxWelded] = pmu->pWeldedVtx[i].ivtxDst;
			for(i=0;i<nNewVtx;i++) if ((j=pVtxMap.ptr[1][i])<pPhysMesh->nVertices && pVtxWeld[j]>=0 && j!=pVtxWeld[j])
				pVtx.ptr[1][i] = pVtx[pVtxMap.ptr[1][i] = pVtxWeld[j]];
			/*for(i=0; i<pMesh[0]->m_nIndexCount; i++) if ((j=pVtxMap[j1=pMesh[0]->m_pIndices[i]])<pPhysMesh->nVertices)
			{
				for(; pVtxWeld[j]>=0; j=pVtxWeld[j]);
				if (pVtxMap[j1]!=j || j1<pPhysMesh->nVertices && pVtxWeld[j1]>=0)
				{
					pVtxMap[pMesh[0]->m_pIndices[i]] = j;
					pVtx[pMesh[0]->m_pIndices[i]] = pVtx[j];
				}
			}*/
			for(i=0; i<pmu->nWeldedVtx; i++)
				pVtxWeld[pmu->pWeldedVtx[i].ivtxWelded] = -1;
		}

		for(i=pmu->nNewTri; i<pmu->nRemovedTri; i++)
		{	// mark unused removed slots as deleted (the list will be compacted later)
			itri = mapiTri(pmu->pRemovedTri[i], pIdx2iTri);	// remap if it's a generated idx from previous updates
			pSubsets[itri] = 1<<30;
		}
		// remove non-physical subsets marked for removal (they were already attached to a child chunk of this mesh)
		for(i=0;i<(int)pMesh[0]->m_subsets.size();i++) if (pMesh[0]->m_subsets[i].nMatFlags & 1<<30)
		{
			pMesh[0]->m_subsets[i].nMatFlags &= ~(1<<30);
			for(j=pMesh[0]->m_subsets[i].nFirstIndexId/3; j*3<pMesh[0]->m_subsets[i].nFirstIndexId+pMesh[0]->m_subsets[i].nNumIndices; j++)
				pSubsets[j] = 1<<30;
		}

		for(i=0; i<pmu->nNewTri; i++)
		{
			if (i<pmu->nRemovedTri)
				itri = mapiTri(pmu->pRemovedTri[i],pIdx2iTri);	
			else
				itri = nTris0+i-pmu->nRemovedTri;
			piTri2Idx[itri] = pmu->pNewTri[i].idxNew;
			pIdx2iTri[pmu->pNewTri[i].idxNew-BOP_NEWIDX0] = itri;
			itriOrg = mapiTri(pmu->pNewTri[i].idxOrg, pIdx2iTri);
			iop = pmu->pNewTri[i].iop;

			for(j=0;j<3;j++)
			{
				if (pmu->pNewTri[i].iVtx[j]>=0 && iop==0)
				{	// if an original vertex, find a vertex in the original triangle that maps to it and use it
					//for(j1=0; j1<2 && pVtxMap[pIdx0[itriOrg*3+j1]]!=pmu->pNewTri[i].iVtx[j]; j1++);
					for(j1=0;j1<3;j1++) dist[j1] = (pVtx[pIdx0[itriOrg*3+j1]]-pVtx[pmu->pNewTri[i].iVtx[j]]).len2();
					j1 = idxmin3(dist);
					pMesh[0]->m_pIndices[itri*3+j] = pIdx0[itriOrg*3+j1];
				}
				else
				{ 
					if ((ivtxNew = -pmu->pNewTri[i].iVtx[j]-1)<0 || pmu->pNewVtx[ivtxNew].iBvtx<0 || iop==0)
					{ // for intersection vertices - interpolate tex coords, tangent basis
						area = pmu->pNewTri[i].area[j]/pmu->pNewTri[i].areaOrg;
						pIdx = iop==0 ? pIdx0 : pMesh[1]->m_pIndices;
						for(j1=0,tex0.s=tex0.t=0,Tangent.zero(),Binormal.zero(),n.zero(),clrf.x=clrf.y=clrf.z=clrf.w=0; j1<3; j1++) 
						{
							tex0.s += pTexCoord[iop][pIdx[itriOrg*3+j1]].s*area[j1];
							tex0.t += pTexCoord[iop][pIdx[itriOrg*3+j1]].t*area[j1];
							Tangent += UnpVec4sf(pTangents[iop][pIdx[itriOrg*3+j1]].Tangent)*area[j1];
							Binormal += UnpVec4sf(pTangents[iop][pIdx[itriOrg*3+j1]].Binormal)*area[j1];
							n += pNormals[iop][pIdx[itriOrg*3+j1]]*area[j1];
							clrf += clr2vec4(pColors[iop][pIdx[itriOrg*3+j1] & -bHasColors[iop]])*area[j1];
						}
						wt = pTangents[iop][pIdx[itriOrg*3]].Tangent.w;
						wb = pTangents[iop][pIdx[itriOrg*3]].Binormal.w;
						Tangent.Normalize(); (Binormal-=Tangent*(Tangent*Binormal)).Normalize(); 
						n.Normalize();
					}
					else
					{	// for B vertices - copy data from the original tringle's vertex that maps to it
						//for(j1=0; j1<2 && pPhysMesh1->pVtxMap[pMesh[1]->m_pIndices[itriOrg*3+j1]]!=pmu->pNewVtx[ivtxNew].iBvtx; j1++);
						for(j1=0;j1<3;j1++) 
							dist[j1] = (pMesh[1]->m_pPositions[pMesh[1]->m_pIndices[itriOrg*3+j1]]-pMesh[1]->m_pPositions[pmu->pNewVtx[ivtxNew].iBvtx]).len2();
						j1 = idxmin3(dist);
						tex0.s = pTexCoord[1][pMesh[1]->m_pIndices[itriOrg*3+j1]].s; 
						tex0.t = pTexCoord[1][pMesh[1]->m_pIndices[itriOrg*3+j1]].t;
						Tangent = UnpVec4sf(pTangents[1][pMesh[1]->m_pIndices[itriOrg*3+j1]].Tangent);
						Binormal = UnpVec4sf(pTangents[1][pMesh[1]->m_pIndices[itriOrg*3+j1]].Binormal);
						wt = pTangents[1][pMesh[1]->m_pIndices[itriOrg*3+j1]].Tangent.w;
						wb = pTangents[1][pMesh[1]->m_pIndices[itriOrg*3+j1]].Binormal.w;
						n = pNormals[1][pMesh[1]->m_pIndices[itriOrg*3+j1]];
						clrf = clr2vec4(pColors[1][pMesh[1]->m_pIndices[itriOrg*3+j1] & -bHasColors[iop]]);
					}
					j1 = 0;	
					ivtxRef = pmu->pNewTri[i].iVtx[j]>=0 ? pmu->pNewTri[i].iVtx[j] : pmu->pNewVtx[ivtxNew].idx;
					// if s and t are close enough to another render vertex generated for this new vertex (if any), use it
					if (ivtxNew>=0 && (j1=-pmu->pNewVtx[ivtxNew].idxTri[iop]-1)>=0 && 
							max(fabs_tpl(pTexCoord[0][j1].s-tex0.s),fabs_tpl(pTexCoord[0][j1].t-tex0.t))<0.003f &&
							Tangent*UnpVec4sf(pTangents[0][j1].Tangent)>0.99f && Binormal*UnpVec4sf(pTangents[0][j1].Binormal)>0.99f &&
							wt==pTangents[0][j1].Tangent.w && wb==pTangents[0][j1].Binormal.w)
						ivtx = j1;
					else 
					{
						if (!iop && j1<0) // if iop==0 and this new vtx wasn't used yet, use the vertex slot pmu.pNewVtx[].idx,
							ivtx = pmu->pNewVtx[ivtxNew].idx;
						else
						{	// else generate a new vertex slot
							AllocVtx;
							pVtxMap[ivtx] = ivtxRef;
						}
						BBox.Add(pVtx[ivtx] = pMesh[0]->m_pPositions[ivtxRef]);
						if (ivtxNew>=0) // store an index of the generated slot here
							pmu->pNewVtx[ivtxNew].idxTri[iop] = -(ivtx+1); 
						pNormals[0][ivtx] = n*float(1-iop*2);
						pTexCoord[0][ivtx] = tex0;
						pTangents[0][ivtx].Tangent = PakVec4sf(Tangent, wt);
						pTangents[0][ivtx].Binormal = PakVec4sf(Binormal, wb);
						pColors[0][ivtx & -bHasColors[0]] = vec42clr(clrf);
					}
					pMesh[0]->m_pIndices[itri*3+j] = ivtx;
				}
			}
			pSubsets[itri] = (iop^1 | bEmptyObj) ? pSubsets0[itriOrg] : iSubsetB;
		}

		for(i=0; i<pmu->nTJFixes; i++)
		{
			// T-Junction fixes:
			// A _____J____ C	 (ACJ is a thin triangle on top of ABC; J is 'the junction vertex')
			//   \		.	  /		 in ABC: set A->Jnew
			//		 \	. /			 in ACJ: set J->Jnew, A -> A from original ABC, C -> B from original ABC
			//			 \/
			//			 B
			int iACJ,iABC,iAC,iCA;
			float k;
			iACJ = mapiTri(pmu->pTJFixes[i].iACJ,pIdx2iTri); iCA = pmu->pTJFixes[i].iCA;
			iABC = mapiTri(pmu->pTJFixes[i].iABC,pIdx2iTri); iAC = pmu->pTJFixes[i].iAC; 

			// allocate a new junction vtx (Jnew) and calculate tex coords for it by projecting J on AC in ABC
			n = pVtx[pIdx0[iABC*3+inc_mod3[iCA]]]-pVtx[pIdx0[iABC*3+iCA]];
			k = ((pVtx[pIdx0[iACJ*3+dec_mod3[iAC]]]-pVtx[pIdx0[iABC*3+iCA]])*n)/(n.len2()+1e-5f);
			AllocVtx;
			tex0.s = pTexCoord[0][pIdx0[iABC*3+iCA]].s*(1-k) + pTexCoord[0][pIdx0[iABC*3+inc_mod3[iCA]]].s*k;
			tex0.t = pTexCoord[0][pIdx0[iABC*3+iCA]].t*(1-k) + pTexCoord[0][pIdx0[iABC*3+inc_mod3[iCA]]].t*k;
			Tangent = UnpVec4sf(pTangents[0][pIdx0[iABC*3+iCA]].Tangent)*(1-k) + 
				UnpVec4sf(pTangents[0][pIdx0[iABC*3+inc_mod3[iCA]]].Tangent)*k;
			Binormal = UnpVec4sf(pTangents[0][pIdx0[iABC*3+iCA]].Binormal)*(1-k) + 
				UnpVec4sf(pTangents[0][pIdx0[iABC*3+inc_mod3[iCA]]].Binormal)*k;
			pVtx[ivtx] = pMesh[0]->m_pPositions[pmu->pTJFixes[i].iTJvtx];
			n = (pNormals[0][pIdx0[iABC*3+iCA]]*(1-k)+pNormals[0][pIdx0[iABC*3+inc_mod3[iCA]]]*k).normalized();
			clrf = clr2vec4(pColors[0][pIdx0[iABC*3+iCA] & -bHasColors[0]])*(1-k)+clr2vec4(pColors[0][pIdx0[iABC*3+inc_mod3[iCA]] & -bHasColors[0]])*k;
			Tangent.Normalize(); (Binormal-=Tangent*(Tangent*Binormal)).Normalize(); 
			pNormals[0][ivtx] = n;
			pTexCoord[0][ivtx] = tex0;
			pTangents[0][ivtx].Tangent = PakVec4sf(Tangent, pTangents[0][pIdx0[iABC*3]].Tangent.w);
			pTangents[0][ivtx].Binormal = PakVec4sf(Binormal, pTangents[0][pIdx0[iABC*3]].Binormal.w);
			pColors[0][ivtx & -bHasColors[0]] = vec42clr(clrf);

			pVtxMap[ivtx] = pVtxMap[pmu->pTJFixes[i].iTJvtx];
			pMesh[0]->m_pIndices[iACJ*3+inc_mod3[iAC]] = pMesh[0]->m_pIndices[iABC*3+dec_mod3[iCA]];
			pMesh[0]->m_pIndices[iACJ*3+iAC] = pMesh[0]->m_pIndices[iABC*3+inc_mod3[iCA]];
			pMesh[0]->m_pIndices[iABC*3+inc_mod3[iCA]] = ivtx;
			pMesh[0]->m_pIndices[iACJ*3+dec_mod3[iAC]] = ivtx;
		}

		// check if the source object has non-physicalized triangles, move them to this object if needed
		if (bEmptyObj) 
		{
			int *pUsedVtx=0,i1;

			for(i=iop=0;i<pMesh[1]->m_subsets.size()-1;i++) 
			if (pMesh[1]->m_subsets[i].nPhysicalizeType==PHYS_GEOM_TYPE_NONE && !(pMesh[1]->m_subsets[i].nMatFlags & (MTL_FLAG_NODRAW|1<<30)))
				if (pMesh[1]->m_subsets[i].nMatFlags & MTL_FLAG_HIDEONBREAK)
					pMesh[1]->m_subsets[i].nMatFlags |= 1<<30;
				else for(ibox=0;ibox<pmu->nMovedBoxes;ibox++) 
				{
					n = pmu->pMovedBoxes[ibox].size-(pmu->pMovedBoxes[ibox].Basis*(pMesh[1]->m_subsets[i].vCenter-pmu->pMovedBoxes[ibox].center)).abs();
					if (min(min(n.x,n.y),n.z)>0) // the subset's center is inside an obstruct box that was attached to this mesh
					{
						if (!pUsedVtx)
							memset(pUsedVtx = new int[pMesh[1]->m_numVertices], -1, pMesh[1]->m_numVertices*sizeof(pUsedVtx[0]));
						ivtxNew = pMesh[0]->m_numVertices;
						pMesh[0]->ReallocStream(CMesh::POSITIONS, ivtxNew + pMesh[1]->m_subsets[i].nNumVerts);
						pMesh[0]->ReallocStream(CMesh::NORMALS, ivtxNew + pMesh[1]->m_subsets[i].nNumVerts);
						pMesh[0]->ReallocStream(CMesh::TEXCOORDS, ivtxNew + pMesh[1]->m_subsets[i].nNumVerts);
						pMesh[0]->ReallocStream(CMesh::TANGENTS, ivtxNew + pMesh[1]->m_subsets[i].nNumVerts);
						if (bHasColors[0])
							pMesh[0]->ReallocStream(CMesh::COLORS_0, ivtxNew + pMesh[1]->m_subsets[i].nNumVerts);
						if (pMesh[1]->m_pSHInfo)
							pMesh[0]->ReallocStream(CMesh::SHCOEFFS, ivtxNew + pMesh[1]->m_subsets[i].nNumVerts);
						pMesh[0]->m_subsets[i].nFirstVertId = ivtxNew;
						itri = (pMesh[0]->m_subsets[i].nFirstIndexId = pMesh[0]->m_nIndexCount)/3;
						pMesh[0]->m_subsets[i].nNumIndices = pMesh[1]->m_subsets[i].nNumIndices;
						pMesh[0]->SetIndexCount(pMesh[0]->m_nIndexCount+pMesh[1]->m_subsets[i].nNumIndices);

						for(itriOrg=(j=pMesh[1]->m_subsets[i].nFirstIndexId)/3; j<pMesh[1]->m_subsets[i].nFirstIndexId+pMesh[1]->m_subsets[i].nNumIndices; 
								j+=3,itriOrg++,itri++) 
						{
							for(i1=0;i1<3;i1++) 
							{
								if ((ivtx = pUsedVtx[j1=pMesh[1]->m_pIndices[j+i1]]) < 0) 
								{
									ivtx = ivtxNew++;
									pMesh[0]->m_subsets[i].nNumVerts++;
									pMesh[0]->m_pPositions[ivtx] = pMesh[1]->m_pPositions[j1];
									pMesh[0]->m_pNorms[ivtx] = pMesh[1]->m_pNorms[j1];
									pMesh[0]->m_pTexCoord[ivtx] = pMesh[1]->m_pTexCoord[j1];
									pMesh[0]->m_pTangents[ivtx].Tangent = pMesh[1]->m_pTangents[j1].Tangent;
									pMesh[0]->m_pTangents[ivtx].Binormal = pMesh[1]->m_pTangents[j1].Binormal;
									if (bHasColors[0])
										pMesh[0]->m_pColor0[ivtx] = pMesh[1]->m_pColor0[j1];
									if (pMesh[0]->m_pSHInfo)
										pMesh[0]->m_pSHInfo->pSHCoeffs[ivtx] = pMesh[1]->m_pSHInfo->pSHCoeffs[j1];
									pUsedVtx[j1] = ivtx;
								}
								pMesh[0]->m_pIndices[itri*3+i1] = ivtx;
							}
							pSubsets[itri] = pSubsets0[itriOrg];
						}

						pMesh[1]->m_subsets[i].nMatFlags |= 1<<30; // tell pMesh[1] to remove its non-physical triangles on its next update
						iop++; break;
					}
				}
			if (iop)
			{
				pStatObj[1]->CopyFoliageData(pStatObj[0], true, 0, pUsedVtx, pmu->pMovedBoxes,pmu->nMovedBoxes); // move foliage data to the new stat object
				((CStatObj*)pStatObj[0])->SetPhysGeom(((CStatObj*)pStatObj[1])->m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT], PHYS_GEOM_TYPE_OBSTRUCT);
				((CStatObj*)pStatObj[1])->SetPhysGeom(0, PHYS_GEOM_TYPE_OBSTRUCT);
			}
			pStatObj[0]->SetFilePath(pStatObj[1]->GetFilePath());
			if (pUsedVtx)
				delete[] pUsedVtx;
		}

		if (pmu->pMesh[1]!=pmu->pMesh[0])
			pmu->pMesh[1]->Unlock(0);

		delete[] pSubsets0; delete[] pIdx0;
		if (pStatObj[0]!=pStatObj[1])
			((CStatObj*)pStatObj[1])->ReleaseIndexedMesh();

		if (pmu==pLastUpdate)
			break;
	}

	if (pmu) {
		pPhysGeom->SetForeignData(pmu->next, DATA_MESHUPDATE);
		pmu->next = 0;
	} else
		pPhysGeom->SetForeignData(0,DATA_MESHUPDATE);

	meshAppend.m_numVertices=meshAppend.m_nCoorCount = nNewVtx;
	meshAppend.m_streamSize[CMesh::POSITIONS]=meshAppend.m_streamSize[CMesh::NORMALS]=
		meshAppend.m_streamSize[CMesh::TEXCOORDS]=meshAppend.m_streamSize[CMesh::TANGENTS] = nNewVtx;
	if (meshAppend.m_pColor0)
		meshAppend.m_streamSize[CMesh::COLORS_0] = nNewVtx;
	//pMesh[0]->Append(meshAppend);
	InjectVertices(pMesh[0], nNewVtx, ((CStatObj*)pStatObj[0])->m_pBoneMapping);
	for (i=0; i<CMesh::LAST_STREAM; i++) if (meshAppend.m_streamSize[i])
	{
		void* pSrcStream=0,*pTrgStream=0;
		int nElementSize = 0;
		meshAppend.GetStreamInfo(i, pSrcStream,nElementSize);
		pMesh[0]->GetStreamInfo(i, pTrgStream,nElementSize);
		if (pSrcStream && pTrgStream)
			memcpy((char*)pTrgStream+(nVtx0+nNewPhysVtx)*nElementSize, pSrcStream, meshAppend.m_streamSize[i]*nElementSize );
	}
	if (pMesh[0]->m_pSHInfo) 
	{
		SMeshSHCoeffs mshc = {{0,0,0,0,0,0,0,0}};
		for(i=0;i<pMesh[0]->m_subsets.size();i++) 
			if (pMesh[0]->m_subsets[i].nPhysicalizeType==PHYS_GEOM_TYPE_NONE && !(pMesh[0]->m_subsets[i].nMatFlags & (MTL_FLAG_NODRAW|1<<30)))
				mshc = pMesh[0]->m_pSHInfo->pSHCoeffs[pMesh[0]->m_subsets[i].nFirstVertId];
		for(i=0;i<nNewVtx+nNewPhysVtx;i++)
			pMesh[0]->m_pSHInfo->pSHCoeffs[nVtx0+i] = mshc;
	}

	// sort the faces by subsetId
	qsort(pSubsets,pMesh[0]->m_pIndices,piTri2Idx, 0,(nTris0=pMesh[0]->m_nIndexCount/3)-1);
	PREFAST_SUPPRESS_WARNING(6385);
	for(; nTris0>0 && pSubsets[nTris0-1]==1<<30; nTris0--);
	pMesh[0]->SetIndexCount(nTris0*3);
	for(i=0; i<pMesh[0]->m_nIndexCount; i++) // remap indices that refer to new vertices
		pMesh[0]->m_pIndices[i] = (pMesh[0]->m_pIndices[i]&~(1<<15)) + ((nVtx0+nNewPhysVtx)&-(pMesh[0]->m_pIndices[i]>>15));

	for(i=0;i<(int)pMesh[0]->m_subsets.size();i++)
		pMesh[0]->m_subsets[i].nFirstIndexId = pMesh[0]->m_subsets[i].nNumIndices = 0;
	minVtx=pMesh[0]->m_numVertices, maxVtx=0;
	for(i=j1=0; i<nTris0; i++) 
	{
		minVtx = min(minVtx, (int)min(min(pMesh[0]->m_pIndices[i*3],pMesh[0]->m_pIndices[i*3+1]),pMesh[0]->m_pIndices[i*3+2]));
		maxVtx = max(maxVtx, (int)max(max(pMesh[0]->m_pIndices[i*3],pMesh[0]->m_pIndices[i*3+1]),pMesh[0]->m_pIndices[i*3+2]));
		if (i==nTris0-1 || pSubsets[i+1]!=pSubsets[i])
		{
			j = pSubsets[i] & ~0x1000;
			if (pMesh[0]->m_subsets[j].nPhysicalizeType==PHYS_GEOM_TYPE_DEFAULT) 
			{
				pMesh[0]->m_subsets[j].nFirstVertId = minVtx;
				pMesh[0]->m_subsets[j].nNumVerts = max(0,maxVtx-minVtx+1);
			}
			pMesh[0]->m_subsets[j].nFirstIndexId = j1*3;
			pMesh[0]->m_subsets[j].nNumIndices = (i-j1+1)*3;
			minVtx = pMesh[0]->m_numVertices; maxVtx = 0;
			j1 = i+1;
		}
	}

	// request physics to remap indices and add split vertices
	for(i=0;i<nTris0;i++) pIdx2iTri[i] = i;
	pPhysGeom->RemapForeignIdx(piTri2Idx, pIdx2iTri, nTris0);
	pPhysGeom->AppendVertices(pVtx.ptr[1], pVtxMap.ptr[1], nNewVtx);

#ifdef _DEBUG
	/*if (!pPhysGeom->GetForeignData(DATA_MESHUPDATE)) 
	{
		nTris0 = pMesh[0]->m_nIndexCount/3;
		if (nTris0!=pPhysMesh->nTris)
			i=i;
		for(i=0;i<nTris0;i++) pSubsets[i] = -1;
		for(i=0;i<pPhysMesh->nTris;i++)
			pSubsets[pPhysMesh->pForeignIdx[i]] = i;
		for(i=0;i<nTris0;i++) if (pSubsets[i]==-1)
			i=i;
		for(i=0;i<nTris0;i++) for(j=0;j<3;j++) 
			if (pPhysMesh->pVtxMap[pMesh[0]->m_pIndices[pPhysMesh->pForeignIdx[i]*3+j]]!=pPhysMesh->pIndices[i*3+j])
				i=i;
		for(i=0;i<nTris0;i++) for(j=0;j<3;j++)
			if ((pMesh[0]->m_pPositions[pMesh[0]->m_pIndices[pPhysMesh->pForeignIdx[i]*3+j]]-pPhysMesh->pVertices[pPhysMesh->pIndices[i*3+j]]).len2()>0.0001f)
				i=i;
	}*/
#endif

	ExitUDS:
	delete[] pVtxWeld;
	CryModuleFree(pVtxMap.ptr[1]);
	delete[] (pIdx2iTri-1);
	delete[] piTri2Idx;
	delete[] pSubsets;
	delete[] pRemovedVtxMask;
	pPhysGeom->Unlock();
	pmu0->Release();
	if (pStatObj[0]->GetIndexedMesh(true))
		pStatObj[0]->Invalidate();

	return pStatObj[0];
}


//////////////////////////////////////////////////////////////////////////
///////////////////////// Deformable Geometry ////////////////////////////
//////////////////////////////////////////////////////////////////////////


void CStatObj::MarkAsDeformable(IGeometry *pInternalGeom)
{
	int i,j,nVtx;
	SDeformableMeshData *dmd;
	CMesh *pMesh;
	IGeometry *pGeom;
	mesh_data *pdata;

	if (!(m_nFlags & STATIC_OBJECT_DEFORMABLE) || m_pDeformableMeshData || !GetIndexedMesh(true))
		return;
	pMesh = GetIndexedMesh(true)->GetMesh();
	m_pDeformableMeshData = dmd = new SDeformableMeshData;

	pGeom = GetPhysicalWorld()->GetGeomManager()->CreateMesh(pMesh->m_pPositions,pMesh->m_pIndices,0,0,pMesh->m_nIndexCount/3,
		mesh_SingleBB|mesh_shared_vtx|mesh_keep_vtxmap);
	pdata = (mesh_data*)pGeom->GetData();

	nVtx = pMesh->m_numVertices;
	pMesh->ReallocStream(CMesh::POSITIONS, (nVtx-1&~31)+32);
	pMesh->ReallocStream(CMesh::NORMALS, (nVtx-1&~31)+32);
	pMesh->ReallocStream(CMesh::TEXCOORDS, (nVtx-1&~31)+32);
	pMesh->ReallocStream(CMesh::TANGENTS, (nVtx-1&~31)+32);
	dmd->pVtxMap = new int[pMesh->m_numVertices];
	dmd->pUsedVtx = new unsigned int[pMesh->m_numVertices>>5];
	dmd->pVtxTri = new int[pMesh->m_numVertices+1];
	dmd->pVtxTriBuf = new int[pMesh->m_nIndexCount];
	dmd->pPrevVtx = new Vec3[pMesh->m_numVertices];
	dmd->prVtxValency = new float[pMesh->m_numVertices];
	pMesh->m_numVertices = pMesh->m_nCoorCount = 
	pMesh->m_streamSize[CMesh::POSITIONS] = pMesh->m_streamSize[CMesh::NORMALS]=
	pMesh->m_streamSize[CMesh::TEXCOORDS] = pMesh->m_streamSize[CMesh::TANGENTS] = nVtx;

	for(i=0;i<nVtx;i++) dmd->pVtxMap[i] = pdata->pVtxMap[i];
	memset(dmd->pUsedVtx, 0, ((nVtx-1&~31)>>5)*sizeof(int));
	memset(dmd->pVtxTri, 0, (nVtx+1)*sizeof(int));
	memset(dmd->prVtxValency, 0, nVtx*sizeof(float));

	for(i=0;i<pdata->nTris*3;i++) {
		dmd->pVtxTri[dmd->pVtxMap[pMesh->m_pIndices[i]]]++;
		dmd->prVtxValency[pMesh->m_pIndices[i]]++;
	}
	for(i=0;i<nVtx;i++) {
		dmd->pVtxTri[i+1] += dmd->pVtxTri[i];
		if (dmd->prVtxValency[i]>0)
			dmd->prVtxValency[i] = 1.0f/dmd->prVtxValency[i];
	}
	for(i=pdata->nTris-1;i>=0;i--) for(j=0;j<3;j++) 
		dmd->pVtxTriBuf[--dmd->pVtxTri[dmd->pVtxMap[pMesh->m_pIndices[i*3+j]]]] = i;

	pGeom->Release();
	if (dmd->pInternalGeom = pInternalGeom)
		pInternalGeom->AddRef();
	dmd->kViscosity = 0.5f;
}


void OffsetVertex(SDeformableMeshData *dmd,CMesh *pMesh,int &nUsedVtx, int ivtx, Vec3 offs, IGeometry *pRayGeom)
{
	int i,j,i1;
	geom_contact *pContacts;
	primitives::ray *pray;
//	Vec3 pos0 = pMesh->m_pPositions[ivtx];

	if (dmd->pInternalGeom)
	{
		pray = (primitives::ray*)pRayGeom->GetData();
		pray->origin = pMesh->m_pPositions[ivtx];
		pray->dir = offs;
		pRayGeom->PrepareForRayTest(0);

		if (i=dmd->pInternalGeom->Intersect(pRayGeom,0,0,0,pContacts))
			offs = (pContacts[i-1].pt-pMesh->m_pPositions[ivtx])*0.99f;
	}

	for(i=dmd->pVtxTri[ivtx]; i<dmd->pVtxTri[ivtx+1]; i++)
	{
		for(j=0; j<2 && dmd->pVtxMap[pMesh->m_pIndices[dmd->pVtxTriBuf[i]*3+j]]!=ivtx; j++);
		clear_mask(dmd->pUsedVtx, pMesh->m_pIndices[dmd->pVtxTriBuf[i]*3+j]);
	}
	for(i=dmd->pVtxTri[ivtx]; i<dmd->pVtxTri[ivtx+1]; i++)
	{
		for(j=0; j<2 && dmd->pVtxMap[pMesh->m_pIndices[dmd->pVtxTriBuf[i]*3+j]]!=ivtx; j++);
		i1 = pMesh->m_pIndices[dmd->pVtxTriBuf[i]*3+j];

		if (check_mask(dmd->pUsedVtx, i1))
			continue;
		/*{	// create a new vertex slot
			if ((nUsedVtx & 31)==0)
			{
				pMesh->ReallocStream(CMesh::POSITIONS, nUsedVtx+32);
				pMesh->ReallocStream(CMesh::NORMALS, nUsedVtx+32);
				pMesh->ReallocStream(CMesh::TEXCOORDS, nUsedVtx+32);
				pMesh->ReallocStream(CMesh::TANGENTS, nUsedVtx+32);
				dmd->pVtxMap = (int*)realloc(dmd->pVtxMap,(nUsedVtx+32)*sizeof(int));
				dmd->pUsedVtx = (unsigned int*)realloc(dmd->pUsedVtx,((nUsedVtx>>5)+1)*sizeof(int));
				dmd->pPrevVtx = (Vec3*)realloc(dmd->pPrevVtx,(nUsedVtx+32)*sizeof(Vec3));
				dmd->pVtxValency = (float*)realloc(dmd->pVtxValency,(nUsedVtx+32)*sizeof(float));
			}
			dmd->pVtxMap[nUsedVtx] = ivtx;
			dmd->pVtxValency[nUsedVtx] = 1;
			pMesh->m_pPositions[nUsedVtx]=dmd->pPrevVtx[nUsedVtx] = pos0;
			pMesh->m_pTangents[nUsedVtx] = pMesh->m_pTangents[i1];
			pMesh->m_pTexCoord[nUsedVtx] = pMesh->m_pTexCoord[i1];
			pMesh->m_pNorms[nUsedVtx] = pMesh->m_pNorms[i1];
			pMesh->m_pIndices[dmd->pVtxTriBuf[i]*3+j] = nUsedVtx;
			i1 = nUsedVtx++;
		}*/
		set_mask(dmd->pUsedVtx, i1);
		pMesh->m_pPositions[i1] += offs;
	}
}


struct SEdgeDeform {
	void set(int _i0,int _i1,float _lenorg2) { i0=_i0;i1=_i1;lenorg2=_lenorg2; }
	int i0,i1;
	float lenorg2;
};

IStatObj *C3DEngine::SmearStatObj(IStatObj *pObj, const Vec3 &pthit,const Vec3 &dir,float r,float depth)
{
	int i,j,i2,j1,ihead,itail=-1,nUsedVtx,nEdges,iedge;
	Vec3 n,dp1,dp2,edge1,c,center,norg,nnew,offs,dir1;
	Quat qrot;
	float r3,kx,ky,kxy,k1,lx,ly,l1,ka,kb,kc,D,x,y,l0,l01,k;
	const int MAXEDGES = 64;
	SEdgeDeform edgesQueue[MAXEDGES];
	CMesh *pMesh;
	IGeometry *pRayGeom;
	primitives::ray aray;
	SDeformableMeshData *dmd;

	if (!(pObj->GetFlags() & STATIC_OBJECT_DEFORMABLE))
		return pObj;
	if (!(pObj->GetFlags() & STATIC_OBJECT_CLONE)) 
		(pObj = pObj->Clone())->AddRef();
	if (pObj->GetSubObjectCount())
	{
		IStatObj::SSubObject *pSlot;
		for(i=pObj->GetSubObjectCount()-1; i>=0; i--) if ((pSlot=pObj->GetSubObject(i))->pStatObj->GetFlags() & STATIC_OBJECT_DEFORMABLE)
			pSlot->pStatObj = SmearStatObj(pSlot->pStatObj, pSlot->tm.GetInverted()*pthit, pSlot->tm.GetInverted().TransformVector(dir), r,depth);
		return pObj;
	}

	if (!pObj->GetIndexedMesh(true))
		return pObj;
	if (!(dmd = ((CStatObj*)pObj)->m_pDeformableMeshData))
		return pObj;
	pMesh = pObj->GetIndexedMesh(true)->GetMesh();
	nUsedVtx = pMesh->m_numVertices;
	pMesh->m_numVertices = pMesh->m_nCoorCount = 
	pMesh->m_streamSize[CMesh::POSITIONS] = pMesh->m_streamSize[CMesh::NORMALS]=
	pMesh->m_streamSize[CMesh::TEXCOORDS] = pMesh->m_streamSize[CMesh::TANGENTS] = (nUsedVtx-1&~31)+32;

	r3 = (r*r/depth+depth)*0.5f;
	center = pthit+dir*(depth-r3);
	aray.origin.zero(); aray.dir.Set(0,0,1);
	pRayGeom = GetPhysicalWorld()->GetGeomManager()->CreatePrimitive(primitives::ray::type, &aray);
	memset(dmd->pUsedVtx, 0, (pMesh->m_numVertices>>5)*sizeof(int));
	memcpy(dmd->pPrevVtx, pMesh->m_pPositions, nUsedVtx*sizeof(Vec3));

	for(i=0; i<nUsedVtx; i++) 
	if ((pMesh->m_pPositions[i]-center).len2()<sqr(r3) && (pMesh->m_pPositions[i]-pthit)*dir>depth*-0.6f && !check_mask(dmd->pUsedVtx,dmd->pVtxMap[i]))
	{ // if a vertex is inside the deformation region (sphere segment) - offset it; find all edges that go from it and queue those
		// whose ends are not inside the region
		for(j=dmd->pVtxTri[dmd->pVtxMap[i]]; j<dmd->pVtxTri[dmd->pVtxMap[i]+1] && itail<MAXEDGES-2; j++)
		{
			for(j1=0; j1<2 && dmd->pVtxMap[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+j1]]!=dmd->pVtxMap[i]; j1++);
			for(j1=inc_mod3[j1],iedge=0; iedge<2; j1=inc_mod3[j1],iedge++)
				if (!check_mask(dmd->pUsedVtx,i2=dmd->pVtxMap[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+j1]]) && 
					  ((pMesh->m_pPositions[i2]-center).len2()>=sqr(r3) || (pMesh->m_pPositions[i2]-pthit)*dir<=depth*-0.6f))
				{
					edgesQueue[itail=itail+1&MAXEDGES-1].set(i,i2, (pMesh->m_pPositions[i2]-pMesh->m_pPositions[i]).len2());
					set_mask(dmd->pUsedVtx, i2);
				}
		}
		// offset vertex and update normals/tangent basis in all affected triangles
		x = (pMesh->m_pPositions[i]-center-dir*(dir*(pMesh->m_pPositions[i]-center))).len2();
		y = sqrt_tpl(sqr(r3)-min(sqr(r3*0.999f),x));
		OffsetVertex(dmd,pMesh,nUsedVtx, dmd->pVtxMap[i], dir*(y-dir*(pMesh->m_pPositions[i]-center)), pRayGeom);
		set_mask(dmd->pUsedVtx, dmd->pVtxMap[i]);
	}

	// loop: get an edge from the queue; find all edges that go from the end point (+calc averaged surface normal)
	// offset the end point by average of calculated offset values; queue all outgoing edges with their original lengths
	for(ihead=0; ihead!=(itail+1 & MAXEDGES-1); ihead=ihead+1 & MAXEDGES-1)
	{
		dp1 = pMesh->m_pPositions[edgesQueue[ihead].i1] - pMesh->m_pPositions[edgesQueue[ihead].i0];
		for(n.zero(),c.zero(),j=dmd->pVtxTri[dmd->pVtxMap[edgesQueue[ihead].i1]]; j<dmd->pVtxTri[dmd->pVtxMap[edgesQueue[ihead].i1]+1]; j++)
		{
			n += pMesh->m_pPositions[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+1]] - pMesh->m_pPositions[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3]] ^
					 pMesh->m_pPositions[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+2]] - pMesh->m_pPositions[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3]];
			c += (pMesh->m_pPositions[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3]] + pMesh->m_pPositions[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+1]] +
						pMesh->m_pPositions[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+2]])*(1.0f/3) - pMesh->m_pPositions[edgesQueue[ihead].i1];
		}
		n.normalize(); n *= -sgnnz(c*n);
		for(j=dmd->pVtxTri[edgesQueue[ihead].i1],offs.zero(),nEdges=0; j<dmd->pVtxTri[edgesQueue[ihead].i1+1]; j++)
		{
			for(j1=0; j1<2 && dmd->pVtxMap[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+j1]]!=edgesQueue[ihead].i1; j1++);
			for(j1=inc_mod3[j1],iedge=0; iedge<2; j1=inc_mod3[j1],iedge++)
			{
				i2 = dmd->pVtxMap[pMesh->m_pIndices[dmd->pVtxTriBuf[j]*3+j1]];
				edge1 = pMesh->m_pPositions[i2] - pMesh->m_pPositions[edgesQueue[ihead].i1];
				if (sqr_signed(edge1*dp1)>edge1.len2()*dp1.len2()*0.25f && !check_mask(dmd->pUsedVtx,i2))
				{
					dir1 = edge1.normalized();
					l0 = edgesQueue[ihead].lenorg2; l01 = dp1.len2();	l1 = edge1.len2();
					dp2 = pMesh->m_pPositions[i2] - pMesh->m_pPositions[edgesQueue[ihead].i0];
					kx = (dp1*dir1)*2; ky = (dp1*n)*2; kxy = dir1*n; k1 = edgesQueue[ihead].lenorg2-l01;
					k = dmd->kViscosity;
					if (l0+l01<l1*sqr(1-k) || sqr(l0+l01-l1*sqr(1-k))<4*l0*l01)
						k = 1;
					lx = (dp2*dir1)*2; ly = (dp2*n)*2; l1 = k1+l1*(1-sqr(k));
					ka = lx*lx+ly*ly-kxy*lx*ly; kb = ky*lx*lx-kx*ly*lx+kxy*l1*lx-2*l1*ly; kc = l1*l1+kx*lx*l1-k1*lx*lx;
					if ((D=kb*kb-4*ka*kc)<0)
					{
						l1 = k1; kb = ky*lx*lx-kx*ly*lx+kxy*l1*lx-2*l1*ly; kc = l1*l1+kx*lx*l1-k1*lx*lx;
						D = kb*kb-4*ka*kc;
					}
					if (D>0 && sqr(ka)>D*sqr(0.01f))
					{
						y = (-kb+sqrt_tpl(D))/(ka*2); x = (l1-ly*y)/lx; offs += dir1*x+n*y;	nEdges++;
						if (y*y>l0+l01 || sqr(l0+l01-y*y)<4*l0*l01)
							y = fabs_tpl(sqrt_tpl(l0)-sqrt_tpl(l01))*sgnnz(y);
#ifdef _DEBUG
						float k0,k1;
						Vec3 posnew;
						posnew = pMesh->m_pPositions[edgesQueue[ihead].i1] + dir1*x + n*y;
						k0 = (posnew-pMesh->m_pPositions[edgesQueue[ihead].i0]).len()/sqrt_tpl(edgesQueue[ihead].lenorg2);
						k1 = (posnew-pMesh->m_pPositions[i2]).len()/edge1.len();
#endif
						if (x*x+y*y>edgesQueue[ihead].lenorg2*sqr(0.1f) && ihead!=(itail+2 & MAXEDGES-1))
							edgesQueue[itail=itail+1&MAXEDGES-1].set(edgesQueue[ihead].i1,i2, edge1.len2());
					}
					set_mask(dmd->pUsedVtx, i2);
				}
			}
		}
		if (nEdges)
			OffsetVertex(dmd,pMesh,nUsedVtx, edgesQueue[ihead].i1, offs/nEdges, pRayGeom);
	}

	for(i=pMesh->m_nIndexCount/3-1;i>=0;i--) 
	{
		for(j=0;j<3 && (pMesh->m_pPositions[pMesh->m_pIndices[i*3+j]]-dmd->pPrevVtx[pMesh->m_pIndices[i*3+j]]).len2()==0;j++);
		if (j<3) 
		{
			norg = (pMesh->m_pPositions[pMesh->m_pIndices[i*3+1]]-pMesh->m_pPositions[pMesh->m_pIndices[i*3]] ^
							pMesh->m_pPositions[pMesh->m_pIndices[i*3+2]]-pMesh->m_pPositions[pMesh->m_pIndices[i*3]]).normalized();
			nnew = (dmd->pPrevVtx[pMesh->m_pIndices[i*3+1]]-dmd->pPrevVtx[pMesh->m_pIndices[i*3]] ^
							dmd->pPrevVtx[pMesh->m_pIndices[i*3+2]]-dmd->pPrevVtx[pMesh->m_pIndices[i*3]]).normalized();
			qrot = Quat::CreateRotationV0V1(norg,nnew);
			for(j=0;j<3;j++) if ((pMesh->m_pPositions[i2=pMesh->m_pIndices[i*3+j]]-dmd->pPrevVtx[pMesh->m_pIndices[i*3+j]]).len2()>0)
			{
				if (qrot.w<0 && qrot.v.len2()<sqr(0.35f))
				{
					qrot.v *= dmd->prVtxValency[i2];
					qrot.w = sqrt_tpl(1-qrot.v.len2());
				}	else
				{
					k = atan2_tpl(l0=qrot.v.len(),qrot.w);
					qrot.v *= sin_tpl(k*dmd->prVtxValency[i2])/l0;
					qrot.w = cos_tpl(k*dmd->prVtxValency[i2]);
				}
				pMesh->m_pNorms[i2] = qrot*pMesh->m_pNorms[i2];
				pMesh->m_pTangents[i2].Tangent = PakVec4sf(qrot*UnpVec4sf(pMesh->m_pTangents[i2].Tangent), pMesh->m_pTangents[i2].Tangent.w);
				pMesh->m_pTangents[i2].Binormal = PakVec4sf(qrot*UnpVec4sf(pMesh->m_pTangents[i2].Binormal), pMesh->m_pTangents[i2].Binormal.w);
			}
		}
	}

	pMesh->m_numVertices = pMesh->m_nCoorCount = 
	pMesh->m_streamSize[CMesh::POSITIONS] = pMesh->m_streamSize[CMesh::NORMALS]=
	pMesh->m_streamSize[CMesh::TEXCOORDS] = pMesh->m_streamSize[CMesh::TANGENTS] = nUsedVtx;
	for(i=0;i<(int)pMesh->m_subsets.size();i++)
	{
		pMesh->m_subsets[i].nFirstVertId = 0;
		pMesh->m_subsets[i].nNumVerts = nUsedVtx;
	}

	GetPhysicalWorld()->GetGeomManager()->DestroyGeometry(pRayGeom);
	pObj->Invalidate();

	return pObj;
}

int CStatObj::SubobjHasDeformMorph(int iSubObj)
{
	int i;
	char nameDeformed[256];
	strcpy(nameDeformed,m_subObjects[iSubObj].name);
	strcat(nameDeformed,"_Destroyed");

	for(i=m_subObjects.size()-1; i>=0 && strcmp(m_subObjects[i].name,nameDeformed); i--);
	return i;
}

#define getBidx(islot) _getBidx(islot,pIdxABuf,pFaceToFace0A,pFace0ToFaceB,pIdxB)
inline int _getBidx(int islot, int *pIdxABuf,uint16 *pFaceToFace0A,int *pFace0ToFaceB,unsigned short *pIdxB)
{
	int idx,mask;
	idx = pFace0ToFaceB[pFaceToFace0A[pIdxABuf[islot]>>2]]*3+(pIdxABuf[islot]&3);
	mask = idx>>31;
	return (pIdxB[idx&~mask]&~mask)+mask;
}


int CStatObj::SetDeformationMorphTarget(IStatObj *pDeformed)
{
	int i,j,k,j0,it,ivtx,nVtxA,nVtxAnew,nVtxA0,nIdxA,nFacesA,nFacesB;
	unsigned short *pIdxA,*pIdxB;
	int *pVtx2IdxA,*pIdxABuf,*pFace0ToFaceB;
	uint16 *pFaceToFace0A,*pFaceToFace0B,maxFace0;
	IRenderMesh *pMeshA,*pMeshB,*pMeshBnew;
	strided_pointer<Vec3> pVtxA,pVtxB,pVtxBnew;
	strided_pointer<Vec2f16> pTexA,pTexB,pTexBnew;
	strided_pointer<Vec4sf> pTangentsA,pTangentsB,pTangentsBnew, pBinormalsA,pBinormalsB,pBinormalsBnew;
	if (!GetRenderMesh())
		MakeRenderMesh();
	if (!pDeformed->GetRenderMesh())
		((CStatObj*)pDeformed)->MakeRenderMesh();
	if (!(pMeshA=GetRenderMesh()) || !(pMeshB=pDeformed->GetRenderMesh()) || 
			!(pFaceToFace0A=m_pMapFaceToFace0) || !(pFaceToFace0B=((CStatObj*)pDeformed)->m_pMapFaceToFace0))
		return 0;
	if (pMeshA->GetMorphBuddy())
		return 1;
	/*{
		GetRenderer()->DeleteRenderMesh(pMeshA->GetMorphBuddy());
		pMeshA->SetMorphBuddy(0);
	}*/

  nFacesB = pMeshB->GetIndicesCount(); nFacesB/=3;
	pIdxB = pMeshB->GetIndexPtr(FSL_READ);
	nVtxA0=nVtxA = pMeshA->GetVerticesCount();
  nIdxA = pMeshA->GetIndicesCount(); nFacesA = nIdxA/=3;
	pIdxA = pMeshA->GetIndexPtr(FSL_READ);
	pVtxA.data = (Vec3*)pMeshA->GetPosPtr(pVtxA.iStride, FSL_READ);
	pTexA.data = (Vec2f16*)pMeshA->GetUVPtr(pTexA.iStride, FSL_READ);
	pTangentsA.data = (Vec4sf*)pMeshA->GetTangentPtr(pTangentsA.iStride, FSL_READ);
	pBinormalsA.data = (Vec4sf*)pMeshA->GetBinormalPtr(pBinormalsA.iStride, FSL_READ);
	
	pVtxB.data = (Vec3*)pMeshB->GetPosPtr(pVtxB.iStride, FSL_READ);
	pTexB.data = (Vec2f16*)pMeshB->GetUVPtr(pTexB.iStride, FSL_READ);
	pTangentsB.data = (Vec4sf*)pMeshB->GetTangentPtr(pTangentsB.iStride, FSL_READ);
	pBinormalsB.data = (Vec4sf*)pMeshB->GetBinormalPtr(pBinormalsB.iStride, FSL_READ);
	
	memset(pVtx2IdxA = new int[nVtxA+1], 0, (nVtxA+1)*sizeof(int));
	for(i=0;i<nIdxA;i++)
		pVtx2IdxA[pIdxA[i]]++;
	for(i=maxFace0=0;i<nFacesA;i++)
		maxFace0 = max(maxFace0,pFaceToFace0A[i]);
	for(i=0;i<nVtxA;i++)
		pVtx2IdxA[i+1] += pVtx2IdxA[i];
	pIdxABuf = new int[nIdxA];
	for(i=nFacesA-1;i>=0;i--) for(j=2;j>=0;j--)
		pIdxABuf[--pVtx2IdxA[pIdxA[i*3+j]]] = i*4+j;

	for(i=nFacesB-1;i>=0;i--)
		maxFace0 = max(maxFace0,pFaceToFace0B[i]);
	memset(pFace0ToFaceB = new int[maxFace0+1], -1, (maxFace0+1)*sizeof(int));
	for(i=nFacesB-1;i>=0;i--)
		pFace0ToFaceB[pFaceToFace0B[i]] = i;

	for(i=nVtxAnew=k=0;i<nVtxA;i++) 
	{
		for(j=pVtx2IdxA[i];j<pVtx2IdxA[i+1]-1;j++) for(k=pVtx2IdxA[i+1]-1;k>j;k--) if (getBidx(k-1)>getBidx(k))
			it=pIdxABuf[k-1], pIdxABuf[k-1]=pIdxABuf[k], pIdxABuf[k]=it;
		for(j=pVtx2IdxA[i]+1;j<pVtx2IdxA[i+1];j++) {
			nVtxAnew += iszero(getBidx(j)-getBidx(j-1))^1;
#ifdef _DEBUG
			if ((pVtxB[getBidx(j)]-pVtxB[getBidx(j-1)]).len2()>sqr(0.01f))
				k++;
#endif
		}
	}

	pMeshBnew = GetRenderer()->CreateRenderMesh("StatObj_Deformed", GetFilePath());
	pMeshBnew->UpdateVertices(0, nVtxA0+nVtxAnew, 0, VSF_GENERAL);
	if (nVtxAnew) 
	{
		pMeshA = GetRenderer()->CreateRenderMesh("StatObj_MorphTarget", GetFilePath());
		m_pRenderMesh->CopyTo(pMeshA,nVtxAnew);
    nIdxA = pMeshA->GetIndicesCount();
		pIdxA = pMeshA->GetIndexPtr(FSL_READ);
		pVtxA.data = (Vec3*)pMeshA->GetPosPtr(pVtxA.iStride, FSL_SYSTEM_UPDATE);
		pTexA.data = (Vec2f16*)pMeshA->GetUVPtr(pTexA.iStride, FSL_SYSTEM_UPDATE);
		pTangentsA.data = (Vec4sf*)pMeshA->GetTangentPtr(pTangentsA.iStride, FSL_SYSTEM_UPDATE);
		pBinormalsA.data = (Vec4sf*)pMeshA->GetBinormalPtr(pBinormalsA.iStride, FSL_SYSTEM_UPDATE);
		GetRenderer()->DeleteRenderMesh(m_pRenderMesh);
		m_pRenderMesh = pMeshA;
	}

	pVtxBnew.data = (Vec3*)pMeshBnew->GetPosPtr(pVtxBnew.iStride, FSL_SYSTEM_UPDATE);
	pTexBnew.data = (Vec2f16*)pMeshBnew->GetUVPtr(pTexBnew.iStride, FSL_SYSTEM_UPDATE);
	pTangentsBnew.data = (Vec4sf*)pMeshBnew->GetTangentPtr(pTangentsBnew.iStride, FSL_SYSTEM_UPDATE);
	pBinormalsBnew.data = (Vec4sf*)pMeshBnew->GetBinormalPtr(pBinormalsBnew.iStride, FSL_SYSTEM_UPDATE);

	for(i=0;i<nVtxA0;i++) for(j=j0=pVtx2IdxA[i];j<pVtx2IdxA[i+1];j++) 
		if (j==pVtx2IdxA[i+1]-1 || getBidx(j)!=getBidx(j+1))
		{
			if (j0>pVtx2IdxA[i])
			{
				ivtx = nVtxA++;
				pVtxA[ivtx] = pVtxA[i]; pTangentsA[ivtx] = pTangentsA[i]; 
				pTexA[ivtx] = pTexA[i];	pBinormalsA[ivtx] = pBinormalsA[i];
				for(k=j0;k<=j;k++) 
					pIdxA[(pIdxABuf[k]>>2)*3+(pIdxABuf[k]&3)] = ivtx;
			}	else
				ivtx = i;
			if ((it=getBidx(j))>=0)
			{
#ifdef _DEBUG
				static float maxdist = 0.1f;
				float dist = (pVtxB[it]-pVtxA[i]).len();
				if (dist>maxdist)
					k++;
#endif
				pVtxBnew[ivtx] = pVtxB[it]; pTangentsBnew[ivtx] = pTangentsB[it];
				pTexBnew[ivtx] = pTexB[it]; pBinormalsBnew[ivtx] = pBinormalsB[it];
			} else 
			{
				pVtxBnew[ivtx] = pVtxA[i]; pTangentsBnew[ivtx]=pTangentsA[i];
				pTexBnew[ivtx] = pTexA[i]; pBinormalsBnew[ivtx]=pBinormalsA[i];
			}
			j0 = j+1;
		}

	pMeshA->SetMorphBuddy(pMeshBnew);
	pDeformed->SetFlags(pDeformed->GetFlags() | STATIC_OBJECT_HIDDEN);

	return 1;
}


inline float max_fast(float op1,float op2) { return (op1+op2+fabsf(op1-op2))*0.5f; }
inline float min_fast(float op1,float op2) { return (op1+op2-fabsf(op1-op2))*0.5f; }

void UpdateWeights(const Vec3 &pt,float r,float strength, IRenderMesh *pMesh,IRenderMesh *pWeights)
{
	int i,nVtx=pMesh->GetVerticesCount();
	float r2=r*r,rr=1/r;
	strided_pointer<Vec3> pVtx;
	strided_pointer<Vec2> pWeight;
	pVtx.data = (Vec3*)pMesh->GetPosPtr(pVtx.iStride, FSL_SYSTEM_UPDATE);
	pWeight = (Vec2*)pWeights->GetPosPtr(pWeight.iStride, FSL_SYSTEM_UPDATE);

	if (r>0) {
		for(i=0;i<nVtx;i++) if ((pVtx[i]-pt).len2()<r2)
			pWeight[i].x = max_fast(0.0f,min_fast(1.0f,pWeight[i].x+strength*(1-(pVtx[i]-pt).len()*rr)));
	} else for(i=0;i<nVtx;i++)
		pWeight[i].x = max_fast(0.0f,min_fast(1.0f,pWeight[i].x+strength));
}


IStatObj *CStatObj::DeformMorph(const Vec3 &pt,float r,float strength, IRenderMesh *pWeights)
{
	int i,j;
	CStatObj *pObj = this;

	if (!GetCVars()->e_DeformableObjects)
  {
    return pObj;
  }

	if (m_bHasDeformationMorphs)
	{
		if (!(GetFlags() & STATIC_OBJECT_CLONE)) 
		{
			(pObj = (CStatObj*)Clone(false))->AddRef();
			pObj->m_bUnmergable = 1;
			for(i=pObj->GetSubObjectCount()-1;i>=0;i--) if ((j=pObj->SubobjHasDeformMorph(i))>=0)
			{
				pObj->GetSubObject(i)->pWeights = pObj->GetSubObject(i)->pStatObj->GetRenderMesh()->GenerateMorphWeights();
				pObj->GetSubObject(j)->pStatObj->SetFlags(pObj->GetSubObject(j)->pStatObj->GetFlags() | STATIC_OBJECT_HIDDEN);
			}
			return pObj->DeformMorph(pt,r,strength, pWeights);
		}
		for(i=m_subObjects.size()-1;i>=0;i--) if (m_subObjects[i].pWeights)
			UpdateWeights(m_subObjects[i].tm.GetInverted()*pt, 
				r*(fabs_tpl(m_subObjects[i].tm.GetColumn(0).len2()-1.0f)<0.01f ? 1.0f:1.0f/m_subObjects[i].tm.GetColumn(0).len()), 
				strength, m_subObjects[i].pStatObj->GetRenderMesh(), m_subObjects[i].pWeights);
	} else if (m_nSubObjectMeshCount==0 && m_pRenderMesh && m_pRenderMesh->GetMorphBuddy())
	{
		if (!pWeights)
		{
			pObj = new CStatObj;
			pObj->m_pMaterial = m_pMaterial;
			pObj->m_fObjectRadius = m_fObjectRadius;
			pObj->m_vBoxMin = m_vBoxMin;
			pObj->m_vBoxMax = m_vBoxMax;
			pObj->m_vVegCenter = m_vVegCenter;
			pObj->m_fRadiusHors = m_fRadiusHors;
			pObj->m_fRadiusVert = m_fRadiusVert;
			pObj->m_nFlags = m_nFlags | STATIC_OBJECT_CLONE;
			pObj->m_bHasDeformationMorphs = true;
			pObj->m_nSubObjectMeshCount = 1;
			pObj->m_bSharesChildren = true;
			pObj->m_subObjects.resize( 1 );
			pObj->m_subObjects[0].nType = STATIC_SUB_OBJECT_MESH;
			pObj->m_subObjects[0].name = "";
			pObj->m_subObjects[0].properties = "";
			pObj->m_subObjects[0].bIdentityMatrix = true;
			pObj->m_subObjects[0].tm.SetIdentity();
			pObj->m_subObjects[0].localTM.SetIdentity();
			pObj->m_subObjects[0].pStatObj = this;
			pObj->m_subObjects[0].nParent = -1;
			pObj->m_subObjects[0].helperSize = Vec3(0,0,0);
			pObj->m_subObjects[0].pWeights = GetRenderMesh()->GenerateMorphWeights();
			return pObj->DeformMorph(pt,r,strength, pWeights);
		}
		UpdateWeights(pt,r,strength, m_pRenderMesh,pWeights);
	}

	return this;
}


IStatObj *CStatObj::HideFoliage()
{
	int i;//,j,idx0;
	CMesh *pMesh;
	if (!GetIndexedMesh())
		return this;

	pMesh = GetIndexedMesh()->GetMesh();
	for(i=pMesh->m_subsets.size()-1; i>=0; i--) if (pMesh->m_subsets[i].nPhysicalizeType==PHYS_GEOM_TYPE_NONE)
	{
		//pMesh->m_subsets[i].nMatFlags |= MTL_FLAG_NODRAW;
		//idx0 = i>0 ? pMesh->m_subsets[i-1].nFirstIndexId+pMesh->m_subsets[i-1].nNumIndices : 0;
		pMesh->m_subsets.erase(pMesh->m_subsets.begin()+i);
		/*for(j=i;j<pMesh->m_subsets.size();j++) 
		{
			memmove(pMesh->m_pIndices+idx0, pMesh->m_pIndices+pMesh->m_subsets[j].nFirstIndexId, 
				pMesh->m_subsets[j].nNumIndices*sizeof(pMesh->m_pIndices[0]));
			pMesh->m_subsets[j].nFirstIndexId = idx0;
			idx0 += pMesh->m_subsets[j].nNumIndices;
		}
		pMesh->m_nIndexCount = idx0;*/
	}
	Invalidate();

	return this;
}


//////////////////////////////////////////////////////////////////////////
////////////////////////   SubObjects    /////////////////////////////////
//////////////////////////////////////////////////////////////////////////

inline int GetEdgeByBuddy(mesh_data *pmd, int itri,int itri_buddy) {
	int iedge=0,imask;
	imask = pmd->pTopology[itri].ibuddy[1]-itri_buddy; imask = (imask-1)>>31 ^ imask>>31; iedge = 1&imask;
	imask = pmd->pTopology[itri].ibuddy[2]-itri_buddy; imask = (imask-1)>>31 ^ imask>>31; iedge = iedge&~imask | 2&imask;
	return iedge;
}
inline float qmin(float op1,float op2) { return (op1+op2-fabsf(op1-op2))*0.5f; }
inline float qmax(float op1,float op2) { return (op1+op2+fabsf(op1-op2))*0.5f; }

IStatObj *CStatObj::UpdateVertices(strided_pointer<Vec3> pVtx, strided_pointer<Vec3> pNormals, int iVtx0,int nVtx, int *pVtxMap)
{
	CStatObj *pObj = this;
	if (m_pRenderMesh) 
	{
		strided_pointer<Vec3> pMeshVtx;
		strided_pointer<Vec4sf> pBinorms,pTangents;
		Vec3 n,t,b,edge;
		int i,j,mask=0,dummy=0,nVtxFull;
		if (!pVtxMap) 
			pVtxMap=&dummy, mask=~0;
		AABB bbox; bbox.Reset();
		SClothTangentVtx *ctd;

		if (!m_pClothTangentsData && GetPhysGeom() && GetPhysGeom()->pGeom->GetType()==GEOM_TRIMESH && m_pRenderMesh) 
		{
			ctd=m_pClothTangentsData = new SClothTangentVtx[nVtxFull=m_pRenderMesh->GetVerticesCount()];
			memset(m_pClothTangentsData, 0, sizeof(SClothTangentVtx)*nVtxFull);
			mesh_data *pmd = (mesh_data*)GetPhysGeom()->pGeom->GetData();
			pTangents.data = (Vec4sf*)m_pRenderMesh->GetTangentPtr(pTangents.iStride, FSL_READ);
			pBinorms.data = (Vec4sf*)m_pRenderMesh->GetBinormalPtr(pBinorms.iStride, FSL_READ);
			for(i=0;i<pmd->nTris;i++)	for(j=0;j<3;j++)
				ctd[pmd->pIndices[i*3+j]].ivtxT=i, ctd[pmd->pIndices[i*3+j]].sgnNorm=j;
			if (pmd->pVtxMap) for(i=0;i<pmd->nVertices;i++) 
				ctd[i].ivtxT=ctd[pmd->pVtxMap[i]].ivtxT, ctd[i].sgnNorm=ctd[pmd->pVtxMap[i]].sgnNorm;

			for(i=0; i<nVtxFull; i++)	
			{
				j = pmd->pVtxMap ? pmd->pVtxMap[i]:i;




				Vec4 v=tPackB2F(pTangents[i]); t=Vec3(v.x,v.y,v.z);
				v=tPackB2F(pBinorms[i]); b=Vec3(v.x,v.y,v.z);

				float tedge=-1,tedgeDenom=0;
				int itri=ctd[i].ivtxT,iedge=ctd[i].sgnNorm,itriT,iedgeT,itri1,loop;
				Vec3 edge0;	n.zero();

				for(int iter=0;iter<2;iter++) {
					for(edge0.zero(),loop=20;;) { // iter==0 - trace cw, 1 - ccw
						edge = (pmd->pVertices[pmd->pIndices[itri*3+inc_mod3[iedge]]] - pmd->pVertices[pmd->pIndices[itri*3+iedge]])*(1-iter*2);
						n += (edge0^edge)*(iter*2-1); edge0 = edge; 
						if (sqr(t*edge)*tedgeDenom > tedge*edge.len2())
							tedge=sqr(t*edge), tedgeDenom=edge.len2(), itriT=itri, iedgeT=iedge;
						itri1 = pmd->pTopology[itri].ibuddy[iedge];
						if (itri1==ctd[i].ivtxT || itri1<0 || --loop<0)
							break;
						iedge=GetEdgeByBuddy(pmd, itri1,itri)+1+iter; itri=itri1;
						iedge -= 3 & (2-iedge)>>31;
					} 
					if (itri1>=0 && iter==0)
						n.zero();
					itri=ctd[i].ivtxT; iedge=dec_mod3[ctd[i].sgnNorm];
				}
				n += pmd->pVertices[pmd->pIndices[ctd[i].ivtxT*3+1]]-pmd->pVertices[pmd->pIndices[ctd[i].ivtxT*3]] ^
						 pmd->pVertices[pmd->pIndices[ctd[i].ivtxT*3+2]]-pmd->pVertices[pmd->pIndices[ctd[i].ivtxT*3]];

				if ((ctd[i].ivtxT=pmd->pIndices[itriT*3+iedgeT])==j)
					ctd[i].ivtxT=pmd->pIndices[itriT*3+inc_mod3[iedgeT]]; 
				edge = (pmd->pVertices[ctd[i].ivtxT]-pmd->pVertices[j]).normalized();




				ctd[i].edge.Set(edge*t, edge*b, edge*(t^b)*v.w);
				ctd[i].sgnNorm = sgnnz(n*(t^b)*v.w);

			}
			if (pObj!=this)
				memcpy(pObj->m_pClothTangentsData=new SClothTangentVtx[nVtxFull], ctd, nVtxFull*sizeof(SClothTangentVtx));
		}

		if (GetTetrLattice() || m_pSkinInfo)
		{
			Vec3 sz = GetAABB().GetSize();
			float szmin=min(min(sz.x,sz.y),sz.z), szmax=max(max(sz.x,sz.y),sz.z), szmed=sz.x+sz.y+sz.y-szmin-szmax; 
			if (!m_pSkinInfo)
				PrepareSkinData(Matrix34(IDENTITY), 0, min(szmin*0.5f, szmed*0.15f));
			if (pVtx)
				return SkinVertices(pVtx,Matrix34(IDENTITY));
			return pObj;
		}

		if (!pVtx)
			return pObj;

		if (!(GetFlags() & STATIC_OBJECT_CLONE))
		{
			pObj = (CStatObj*)Clone();
			pObj->m_pRenderMesh->KeepSysMesh(true);
		}

    pMeshVtx.data = (Vec3*)pObj->m_pRenderMesh->GetPosPtr(pMeshVtx.iStride, FSL_SYSTEM_UPDATE);
		for(i=iVtx0; i<nVtx; i++)
    {
      Vec3 v = pVtx[j = pVtxMap[i&~mask] | i&mask];
			bbox.Add(v);
      pMeshVtx[i] = v;
    }
		pObj->m_pRenderMesh->UnlockStream(VSF_GENERAL);
		pObj->m_vBoxMin = bbox.min; pObj->m_vBoxMax = bbox.max;

		if (ctd=m_pClothTangentsData)
		{
			pTangents.data = (Vec4sf*)pObj->m_pRenderMesh->GetTangentPtr(pTangents.iStride, FSL_SYSTEM_UPDATE);
			pBinorms.data = (Vec4sf*)pObj->m_pRenderMesh->GetBinormalPtr(pBinorms.iStride, FSL_SYSTEM_UPDATE);
			for(i=iVtx0; i<nVtx; i++)
			{




				int tw	=	pTangents[i].w;
				int bw	=	pBinorms[i].w;

				n = pNormals[j = pVtxMap[i&~mask] | i&mask]*ctd[i].sgnNorm;
				edge = (pVtx[ctd[i].ivtxT]-pVtx[j]).normalized();
				Matrix33 M; float nsg = tPackB2F(tw);
				crossproduct_matrix(pNormals[j]*ctd[i].edge.y, M) *= nsg;
				M += Matrix33(IDENTITY)*ctd[i].edge.x; 
				t = M.GetInverted()*(edge-n*ctd[i].edge.z);
				(t-=n*(n*t)).Normalize();
				t.x = qmin(qmax(t.x,-0.9999f),0.9999f);
				t.y = qmin(qmax(t.y,-0.9999f),0.9999f);
				t.z = qmin(qmax(t.z,-0.9999f),0.9999f);
				Vec3 bn	=	(n^t)*nsg;
				pTangents[i] = PakVec4sf(t, tw);
				pBinorms[i] = PakVec4sf(bn, bw);
			}
			pObj->m_pRenderMesh->UnlockStream(VSF_TANGENTS);
		}
	}
	return pObj;
}


//////////////////////////////////////////////////////////////////////////
namespace
{
	CryCriticalSection g_cPrepareSkinData;
}

void CStatObj::PrepareSkinData(const Matrix34 &mtxSkelToMesh, IGeometry *pPhysSkel, float r)
{
	if (m_pSkinInfo || pPhysSkel && pPhysSkel->GetType()!=GEOM_TRIMESH)
		return;

	// protect again possible paralle calls, if streaming is here, but mainthread also reaches this function
	// before streaming thread has finished and wants to prepare data also
	AUTO_LOCK(g_cPrepareSkinData);

	// recheck again to guard again creating the object two times
	if(m_pSkinInfo)
		return;

	m_nFlags |= STATIC_OBJECT_DYNAMIC;
	if (!m_pRenderMesh)
	{
		if (!m_pDelayedSkinParams)
		{
			m_pDelayedSkinParams = new SDelayedSkinParams;
			m_pDelayedSkinParams->mtxSkelToMesh = mtxSkelToMesh;
			m_pDelayedSkinParams->pPhysSkel = pPhysSkel;
			m_pDelayedSkinParams->r = r;
		}
		return;
	}
	m_pRenderMesh->KeepSysMesh(true);

	int i,j,nVtx;
	Vec3 vtx[4];
	geom_world_data gwd[2];
	geom_contact *pcontact;
	mesh_data *md;
	IGeometry *pSphere,*pSphereBig;
	strided_pointer<Vec3> pVtx;
	Matrix34 mtxMeshToSkel = mtxSkelToMesh.GetInverted();
	ITetrLattice *pLattice = GetTetrLattice();
	if (pLattice)
		pPhysSkel = pLattice->CreateSkinMesh();

	gwd[1].scale = mtxSkelToMesh.GetColumn0().len();
	gwd[1].offset = mtxSkelToMesh.GetTranslation();
	gwd[1].R = Matrix33(mtxSkelToMesh)*(1.0f/gwd[1].scale);
	m_pRenderMesh->GetBBox(vtx[0],vtx[1]); vtx[1]-=vtx[0];
	primitives::sphere sph; sph.center.zero(); sph.r=r>0.0f ? r:min(min(vtx[1].x,vtx[1].y),vtx[1].z);
	pSphere = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::sphere::type, &sph);
	sph.r*=3.0f; pSphereBig = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::sphere::type, &sph);
	assert(pPhysSkel);
	md = (mesh_data*)pPhysSkel->GetData();
	pVtx.data = (Vec3*)m_pRenderMesh->GetPosPtr(pVtx.iStride, FSL_READ);
	m_pSkinInfo = new SSkinVtx[nVtx=m_pRenderMesh->GetVerticesCount()];

	for(i=0; i<nVtx; i++) 
	{
    Vec3 v = pVtx[i];
		if (m_pSkinInfo[i].bVolumetric = (pLattice && pLattice->CheckPoint(mtxMeshToSkel*v, m_pSkinInfo[i].idx,m_pSkinInfo[i].w)))
			m_pSkinInfo[i].M = Matrix33(md->pVertices[m_pSkinInfo[i].idx[1]]-md->pVertices[m_pSkinInfo[i].idx[0]], 
																	md->pVertices[m_pSkinInfo[i].idx[2]]-md->pVertices[m_pSkinInfo[i].idx[0]],
																	md->pVertices[m_pSkinInfo[i].idx[3]]-md->pVertices[m_pSkinInfo[i].idx[0]]).GetInverted();
		else
		{
			gwd[0].offset = v;
			if (pSphere->Intersect(pPhysSkel, gwd,gwd+1, 0, pcontact) ||
					pSphereBig->Intersect(pPhysSkel, gwd,gwd+1, 0, pcontact))
			{
				for(j=0;j<3;j++) 
					vtx[j] = mtxSkelToMesh*md->pVertices[m_pSkinInfo[i].idx[j] = md->pIndices[pcontact->iPrim[1]*3+j]];
				Vec3 n = vtx[1]-vtx[0] ^ vtx[2]-vtx[0];
				float rnlen = 1.0f/n.len();	n *= rnlen;
				vtx[3] = v-n*(m_pSkinInfo[i].w[3]=n*(v-vtx[0]));
				Vec3 edge = (vtx[1]+vtx[2]-vtx[0]*2).normalized();
				m_pSkinInfo[i].M = Matrix33(edge,n^edge,n).T();
				for(j=0,n*=rnlen; j<3; j++)
					m_pSkinInfo[i].w[j] = (vtx[inc_mod3[j]]-vtx[3]^vtx[dec_mod3[j]]-vtx[3])*n;
			}	else
				m_pSkinInfo[i].idx[0] = -1;	
		}
	}	
	if (pLattice)
		pPhysSkel->Release();
	pSphereBig->Release(); pSphere->Release();
}

IStatObj *CStatObj::SkinVertices(strided_pointer<Vec3> pSkelVtx, const Matrix34 &mtxSkelToMesh)
{
	if (!m_pSkinInfo && m_pDelayedSkinParams)
	{
		PrepareSkinData(m_pDelayedSkinParams->mtxSkelToMesh, m_pDelayedSkinParams->pPhysSkel, m_pDelayedSkinParams->r);
		if (m_pSkinInfo)
			delete m_pDelayedSkinParams, m_pDelayedSkinParams=0;
	}
	if (!m_pRenderMesh || !m_pSkinInfo)
		return this;
	CStatObj *pObj = this;
	if (!(GetFlags() & STATIC_OBJECT_CLONE))
		pObj = (CStatObj*)Clone();
	if (!pObj->m_pClonedSourceObject || !pObj->m_pClonedSourceObject->m_pRenderMesh)
		return pObj;

	strided_pointer<Vec3> pVtx;
	strided_pointer<Vec4sf> pBinorms,pTangents, pBinorms0,pTangents0;
	Vec3 vtx[4],t,b;
	Matrix33 M;
	AABB bbox; bbox.Reset();
	int i,j,nVtx;

	pVtx.data = (Vec3*)pObj->m_pRenderMesh->GetPosPtr(pVtx.iStride, FSL_SYSTEM_UPDATE);
	pTangents.data = (Vec4sf*)pObj->m_pRenderMesh->GetTangentPtr(pTangents.iStride, FSL_SYSTEM_UPDATE);
	pBinorms.data = (Vec4sf*)pObj->m_pRenderMesh->GetBinormalPtr(pBinorms.iStride, FSL_SYSTEM_UPDATE);
	pTangents0.data = (Vec4sf*)pObj->m_pClonedSourceObject->m_pRenderMesh->GetTangentPtr(pTangents0.iStride, FSL_SYSTEM_UPDATE);
	pBinorms0.data = (Vec4sf*)pObj->m_pClonedSourceObject->m_pRenderMesh->GetBinormalPtr(pBinorms0.iStride, FSL_SYSTEM_UPDATE);
	nVtx = pObj->m_pRenderMesh->GetVerticesCount();

	for(i=0; i<nVtx; i++)
  {
    Vec3 v3 = pVtx[i];
		if (m_pSkinInfo[i].idx[0]>=0) 
		{
			for(j=0,v3.zero(); j<3+m_pSkinInfo[i].bVolumetric; j++)
				v3 += m_pSkinInfo[i].w[j]*(vtx[j] = mtxSkelToMesh*pSkelVtx[m_pSkinInfo[i].idx[j]]);
			if (!m_pSkinInfo[i].bVolumetric) 
			{
				Vec3 n = (vtx[1]-vtx[0]^vtx[2]-vtx[0]).normalized();
				v3 += n*m_pSkinInfo[i].w[3];
				Vec3 edge = (vtx[1]+vtx[2]-vtx[0]*2).normalized();
				M = Matrix33(edge,n^edge,n);
			} else
				M = Matrix33(vtx[1]-vtx[0],vtx[2]-vtx[0],vtx[3]-vtx[0]);
			M *= m_pSkinInfo[i].M;






			Vec4 v=tPackB2F(pTangents0[i]); t=Vec3(v.x,v.y,v.z);
			v=tPackB2F(pBinorms0[i]); b=Vec3(v.x,v.y,v.z);
			pTangents[i] = PakVec4sf(M*t, pTangents0[i].w);
			pBinorms[i] = PakVec4sf(M*b, pBinorms0[i].w);

			pVtx[i] = v3;
		}
		bbox.Add(v3);
	}

	pObj->m_pRenderMesh->UnlockStream(VSF_GENERAL);
	pObj->m_pRenderMesh->UnlockStream(VSF_TANGENTS);
	pObj->m_vBoxMin = bbox.min; pObj->m_vBoxMax = bbox.max;
	return pObj;
}

//////////////////////////////////////////////////////////////////////////

int CStatObj::Physicalize(IPhysicalEntity *pent, pe_geomparams *pgp, int id, const char *szPropsOverride)
{
	int res = -1;
	if (GetFlags() & STATIC_OBJECT_COMPOUND) 
	{
		Matrix34 mtxId(IDENTITY);
		res = PhysicalizeSubobjects(pent, pgp->pMtx3x4 ? pgp->pMtx3x4:&mtxId, pgp->mass,pgp->density, 0,0, szPropsOverride);
	} else
	{
		int i,nNoColl,iNoColl;
		float V;
		if (pgp->mass<0 && pgp->density<0)
			GetPhysicalProperties(pgp->mass, pgp->density);
		for(i=m_arrPhysGeomInfo.GetGeomCount()-1,nNoColl=0,V=0.0f; i>=0; i--)
			if (m_arrPhysGeomInfo.GetGeomType(i)==PHYS_GEOM_TYPE_DEFAULT)
				V += m_arrPhysGeomInfo[i]->V;
			else 
				iNoColl=i,++nNoColl;
		if (pgp->mass>pgp->density && V>0.0f)	// mass is set instead of density and V is valid
			pgp->density=pgp->mass/V, pgp->mass=-1.0f;
		(pgp->flags &= ~geom_colltype_explosion) |= geom_colltype_explosion & ~-(int)m_bDontOccludeExplosions;
		(pgp->flags &= ~geom_manually_breakable) |= geom_manually_breakable &  -(int)m_bBreakableByGame;
		if (nNoColl==1 && m_arrPhysGeomInfo.GetGeomCount()==2)
		{	// one solid, one obstruct or nocoll proxy -> use single part with ray proxy
			res = pent->AddGeometry(m_arrPhysGeomInfo[iNoColl], pgp, id);
			pgp->flags |= geom_proxy;
			pent->AddGeometry(m_arrPhysGeomInfo[iNoColl^1], pgp, res);
			pgp->flags &= ~geom_proxy;
		}	else
		{	// add all solid and non-colliding geoms as individual parts
			for(i=0; i<m_arrPhysGeomInfo.GetGeomCount(); i++) if (m_arrPhysGeomInfo.GetGeomType(i)==PHYS_GEOM_TYPE_DEFAULT)
				res=pent->AddGeometry(m_arrPhysGeomInfo[i], pgp, id), id+=1024;
			pgp->idmatBreakable = -1;
			for(i=0; i<m_arrPhysGeomInfo.GetGeomCount(); i++) if (m_arrPhysGeomInfo.GetGeomType(i)==PHYS_GEOM_TYPE_NO_COLLIDE)
				pgp->flags=geom_colltype_ray, res=pent->AddGeometry(m_arrPhysGeomInfo[i], pgp, id), id+=1024;
			else if (m_arrPhysGeomInfo.GetGeomType(i)==PHYS_GEOM_TYPE_OBSTRUCT)
				pgp->flags=geom_colltype_obstruct, res=pent->AddGeometry(m_arrPhysGeomInfo[i], pgp, id), id+=1024;
		}
	}	
	return res;
}

#define isalpha(c) isalpha((unsigned char)c)
#define isdigit(c) isdigit((unsigned char)c)

inline bool IsDigitChar( char c )
{
	return (isdigit(c)||c=='.'||c=='-'||c=='+');
}

int CStatObj::PhysicalizeSubobjects(IPhysicalEntity *pent, const Matrix34 *pMtx, float mass,float density, int id0, strided_pointer<int> pJointsIdMap, const char *szPropsOverride)
{
	int i,j,i0,i1,len,len1,nObj=GetSubObjectCount(),ipart[2],nparts,bHasPlayerOnlyGeoms=0,nGeoms=0,id,idNext=0;
	float V[2],M,scale,jointsz;
	const char *pval,*properties;
	bool bHasSkeletons=false, bAutoJoints=false;
	Matrix34 mtxLoc;
	Vec3 dist;
	primitives::box joint_bbox;
	IGeometry *pJointBox = 0;

	primitives::box bbox;
	IStatObj::SSubObject *pSubObj,*pSubObj1;
	pe_articgeomparams partpos;
	pe_params_structural_joint psj;
	pe_params_flags pf;
	geom_world_data gwd;
	intersection_params ip;
	geom_contact *pcontacts;
	scale = pMtx ? pMtx->GetColumn(0).len():1.0f;
	ip.bStopAtFirstTri=ip.bNoAreaContacts=ip.bNoBorder = true;
	
	bbox.Basis.SetIdentity();
	bbox.size.Set(0.5f,0.5f,0.5f);
	bbox.center.zero();
	joint_bbox = bbox;
	pf.flagsOR = 0;

	for(i=0;i<nObj;i++)	
		if ((pSubObj=GetSubObject(i))->nType==STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && 
				pSubObj->bHidden && !strncmp(pSubObj->name,"childof_",8) && (pSubObj1=FindSubObject((const char*)pSubObj->name+8)))
				pSubObj->bHidden = pSubObj1->bHidden;

	for(i=0,V[0]=V[1]=M=0; i<nObj; i++)	
		if ((pSubObj=GetSubObject(i))->nType==STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && !pSubObj->bHidden && strncmp(pSubObj->name,"skeleton_",9))
		{
			float mi=-1.0f, dens=-1.0f;
			pSubObj->pStatObj->GetPhysicalProperties(mi,dens);
			if (dens > 0)
				mi = pSubObj->pStatObj->GetPhysGeom()->V*cube(scale)*dens; // Calc mass.

			if (mi!=0.0f)
				V[isneg(mi)] += pSubObj->pStatObj->GetPhysGeom()->V*cube(scale);
			M += max(0.0f,mi);

			if (((CStatObj*)pSubObj->pStatObj)->m_nRenderTrisCount<=0 && 
					pSubObj->pStatObj->GetPhysGeom()->pGeom->GetForeignData()==pSubObj->pStatObj && 
					strstr(pSubObj->properties,"other_rendermesh"))
			{
				Vec3 center = pSubObj->localTM*(((CStatObj*)pSubObj->pStatObj)->m_vBoxMin+((CStatObj*)pSubObj->pStatObj)->m_vBoxMax)*0.5f;
				float mindist=1e10f,curdist;
				for(j=0,i0=i; j<nObj; j++) 
					if (j!=i && (pSubObj1=GetSubObject(j)) && pSubObj1->pStatObj && ((CStatObj*)pSubObj1->pStatObj)->m_nRenderTrisCount>0 && 
							(curdist=(pSubObj1->localTM*(((CStatObj*)pSubObj1->pStatObj)->m_vBoxMin+
																					 ((CStatObj*)pSubObj1->pStatObj)->m_vBoxMax)*0.5f-center).len2())<mindist)
					{
						mindist=curdist; i0=j;
					}
				pSubObj->pStatObj->GetPhysGeom()->pGeom->SetForeignData(GetSubObject(i0)->pStatObj,0);
			}
		}
	for(i=0; i<nObj; i++)	
		GetSubObject(i)->nBreakerJoints = 0;
	if (mass<=0)
		mass = M*density;
	if (density<=0)
	{
		if ((V[0]+V[1]) != 0)
			density = mass/(V[0]+V[1]);
		else
			density = 1000.0f; // Some default.
	}
	partpos.flags = geom_collides|geom_floats;

	for(i=0; i<nObj; i++)	
		if ((pSubObj=GetSubObject(i))->nType==STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && 
				!pSubObj->bHidden && strcmp(pSubObj->name,"colltype_player")==0)
		bHasPlayerOnlyGeoms = 1;

	for(i=0; i<nObj; i++)	
		if ((pSubObj=GetSubObject(i))->nType==STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && !pSubObj->bHidden)
		{
			if (pJointsIdMap)
				continue;
			partpos.idbody = i;
			partpos.pMtx3x4 = pMtx ? &(mtxLoc = *pMtx*pSubObj->tm) : &pSubObj->tm;

			float mi=0, di=0;
			if (pSubObj->pStatObj->GetPhysicalProperties(mi,di))
			{
				if (mi >= 0)
					partpos.mass=mi, partpos.density=0;
				else
					partpos.mass=0, partpos.density=di;
			}	else
				partpos.density = density;
			if (GetCVars()->e_ObjQuality != CONFIG_LOW_SPEC) 
			{
				partpos.idmatBreakable = ((CStatObj*)pSubObj->pStatObj)->m_idmatBreakable;
				if (((CStatObj*)pSubObj->pStatObj)->m_bVehicleOnlyPhysics)
					partpos.flags = geom_colltype6;
				else
				{
					partpos.flags = (geom_colltype_solid&~(geom_colltype_player&-bHasPlayerOnlyGeoms)) | geom_colltype_ray|geom_floats|geom_colltype_explosion;
					if (bHasPlayerOnlyGeoms && strcmp(pSubObj->name,"colltype_player")==0)
						partpos.flags = geom_colltype_player;
				}
			} else 
			{
				partpos.idmatBreakable = -1;
				if (((CStatObj*)pSubObj->pStatObj)->m_bVehicleOnlyPhysics)
					partpos.flags = geom_colltype6;
			}
			partpos.flagsCollider &= ~geom_destroyed_on_break;
			if (strstr(pSubObj->properties,"pieces"))
				partpos.flagsCollider |= geom_destroyed_on_break;
			if ((id = pSubObj->pStatObj->Physicalize(pent, &partpos, i+id0))>=0) 
				nGeoms++, idNext=id+1;
			if (!bHasSkeletons && !strncmp(pSubObj->name,"skeleton_",9))
				bHasSkeletons = true;
		} else
		if (pSubObj->nType==STATIC_SUB_OBJECT_DUMMY && !strncmp(pSubObj->name,"$joint",6))
		{
			properties = szPropsOverride ? szPropsOverride : (const char*)pSubObj->properties;
			psj.pt = pSubObj->tm.GetTranslation();
			psj.n = pSubObj->tm.GetColumn(2).normalized();
			psj.axisx = pSubObj->tm.GetColumn(0).normalized();
			float maxdim = max(pSubObj->helperSize.x,pSubObj->helperSize.y);
			maxdim = max(maxdim,pSubObj->helperSize.z);
			psj.szSensor = jointsz = maxdim * pSubObj->tm.GetColumn(0).len();
			psj.partidEpicenter = -1;
			psj.bBroken = 0;
			psj.id = i;
			psj.bReplaceExisting = 1;
			if (pSubObj->name[6]!=' ') 
			{
				gwd.offset = psj.pt;
				ipart[1] = nObj;
				for(i0=nparts=0; i0<nObj && nparts<2; i0++) 
					if ((pSubObj1=GetSubObject(i0))->nType==STATIC_SUB_OBJECT_MESH && pSubObj1->pStatObj && pSubObj1->pStatObj->GetPhysGeom() && 
							strncmp(pSubObj1->name,"skeleton_",9))
					{
						gwd.offset = pSubObj1->tm.GetInverted()*psj.pt;
						pSubObj1->pStatObj->GetPhysGeom()->pGeom->GetBBox(&bbox);
						dist = bbox.Basis*(gwd.offset-bbox.center);
						for(j=0;j<3;j++)
							dist[j] = max(0.0f, fabs_tpl(dist[j])-bbox.size[j]);
						gwd.scale = jointsz;
						if (fabs_tpl(pSubObj1->tm.GetColumn(0).len2()-1.0f)>0.01f)
							gwd.scale /= pSubObj1->tm.GetColumn(0).len();
						
						// Make a geometry box for intersection.
						if (!pJointBox)
							pJointBox = GetPhysicalWorld()->GetGeomManager()->CreatePrimitive(primitives::box::type, &joint_bbox);
						if (dist.len2()<sqr(gwd.scale*0.5f) && pSubObj1->pStatObj->GetPhysGeom()->pGeom->Intersect(pJointBox,0,&gwd,&ip,pcontacts)) {
							WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
							ipart[nparts++] = i0;
						}
					}
				if (nparts==0)
					continue;
				GetSubObject(ipart[0])->pStatObj->GetPhysGeom()->pGeom->GetBBox(&bbox);
				gwd.offset = GetSubObject(ipart[0])->tm*bbox.center;
				j = isneg((gwd.offset-psj.pt)*psj.n);
				i0 = ipart[j]; i1 = ipart[1^j];
				if (!strncmp((const char*)pSubObj->name+7,"sample",6)) 
					psj.bBroken=2;
				else if (!strncmp((const char*)pSubObj->name+7,"impulse",7)) 
					psj.bBroken=2, psj.id=joint_impulse, psj.bReplaceExisting=0, bAutoJoints=true;
			}	else
			{
				for(i0=0; i0<nObj && ((pSubObj1=GetSubObject(i0))->nType!=STATIC_SUB_OBJECT_MESH || 
						strncmp((const char*)pSubObj->name+7,pSubObj1->name,len=strlen(pSubObj1->name)) || isalpha(pSubObj->name[7+len])); i0++);
				for(i1=0; i1<nObj && ((pSubObj1=GetSubObject(i1))->nType!=STATIC_SUB_OBJECT_MESH || 
						strncmp((const char*)pSubObj->name+8+len,pSubObj1->name,len1=strlen(pSubObj1->name)) || isalpha(pSubObj->name[8+len+len1])); i1++);
			}
			if (pJointsIdMap)
				i0=pJointsIdMap[i0], i1=pJointsIdMap[i1];
			psj.partid[0] = i0+id0; psj.partid[1] = i1+id0;
			psj.maxForcePush=psj.maxForcePull=psj.maxForceShift=psj.maxTorqueBend=psj.maxTorqueTwist = 1E20f;
			if (pMtx)
			{
				psj.pt = *pMtx*psj.pt;
				psj.n = pMtx->TransformVector(psj.n).normalized();
				psj.axisx = pMtx->TransformVector(psj.axisx).normalized();
			}

			if ((pval=strstr(properties, "limit")) && (pval-11<properties-11 || strncmp(pval-11,"constraint_",11))) {
				for(pval+=5;*pval && !isdigit(*pval);pval++); if (pval) psj.maxTorqueBend = atof(pval);
				//psj.maxTorqueTwist = psj.maxTorqueBend*5;
				psj.maxForcePull = psj.maxTorqueBend;
				psj.maxForceShift = psj.maxTorqueBend;
//				psj.maxForcePush = psj.maxForcePull*2.5f;
				psj.bBreakable = 1;
			}	if (pval=strstr(properties, "twist")) {
				for(pval+=5;*pval && !isdigit(*pval);pval++); if (pval) psj.maxTorqueTwist = atof(pval);
			}	if (pval=strstr(properties, "bend")) {
				for(pval+=4;*pval && !isdigit(*pval);pval++); if (pval) psj.maxTorqueBend = atof(pval);
			}	if (pval=strstr(properties, "push")) {
				for(pval+=4;*pval && !isdigit(*pval);pval++); if (pval) psj.maxForcePush = atof(pval);
			}	if (pval=strstr(properties, "pull")) {
				for(pval+=4;*pval && !isdigit(*pval);pval++); if (pval) psj.maxForcePull = atof(pval);
			}	if (pval=strstr(properties, "shift")) {
				for(pval+=5;*pval && !isdigit(*pval);pval++); if (pval) psj.maxForceShift = atof(pval);
			}
			if (psj.maxForcePush+psj.maxForcePull+psj.maxForceShift+psj.maxTorqueBend+psj.maxTorqueTwist > 4.9E20f)
				if (sscanf(properties, "%f %f %f %f %f", 
						&psj.maxForcePush,&psj.maxForcePull,&psj.maxForceShift,&psj.maxTorqueBend,&psj.maxTorqueTwist)==5)
				{
					psj.maxForcePush*=density; psj.maxForcePull*=density; psj.maxForceShift*=density;
					psj.maxTorqueBend*=density; psj.maxTorqueTwist*=density;
					psj.bBreakable = 1;
				}	else
					psj.bBreakable = 0;
			psj.limitConstraint.zero(); psj.bConstraintWillIgnoreCollisions=1;
			if ((len=16,pval=strstr(properties, "constraint_limit")) || (len=5,pval=strstr(properties, "C_lmt"))) {
				for(pval+=len;*pval && !isdigit(*pval);pval++); if (pval) psj.limitConstraint.z = atof(pval);
			}	if ((len=17,pval=strstr(properties, "constraint_minang")) || (len=5,pval=strstr(properties, "C_min"))) {
				for(pval+=len;*pval && !isdigit(*pval) && *pval!='-';pval++); if (pval) psj.limitConstraint.x = DEG2RAD(atof(pval));
			}	if ((len=17,pval=strstr(properties, "constraint_maxang")) || (len=5,pval=strstr(properties, "C_max"))) {
				for(pval+=len;*pval && !isdigit(*pval) && *pval!='-';pval++); if (pval) psj.limitConstraint.y = DEG2RAD(atof(pval));
			}	if ((len=18,pval=strstr(properties, "constraint_damping")) || (len=5,pval=strstr(properties, "C_dmp"))) {
				for(pval+=len;*pval && !isdigit(*pval);pval++); if (pval) psj.dampingConstraint = atof(pval);
			}	if (strstr(properties, "constraint_collides") || strstr(properties, "C_coll"))
				psj.bConstraintWillIgnoreCollisions = 0;
			scale = GetCVars()->e_JointStrengthScale;
			psj.maxForcePush*=scale; psj.maxForcePull*=scale; psj.maxForceShift*=scale;
			psj.maxTorqueBend*=scale; psj.maxTorqueTwist*=scale; psj.limitConstraint.z*=scale;
			pent->SetParams(&psj);
			if (strstr(properties,"gameplay_critical"))
				pf.flagsOR |= pef_override_impulse_scale;
			if (strstr(properties,"player_can_break"))
				pf.flagsOR |= pef_players_can_break;

			if (strstr(pSubObj->properties,"breaker"))
			{
				if (GetSubObject(i0))
					GetSubObject(i0)->nBreakerJoints++;
				if (GetSubObject(i1))
					GetSubObject(i1)->nBreakerJoints++;
			}
		}

	if (bAutoJoints)
	{
		psj.idx = -2; // tels the physics to try and generate joints
		pent->SetParams(&psj);
	}

	pe_params_part pp;
	if (bHasSkeletons) for(i=0;i<nObj;i++)	
		if ((pSubObj=GetSubObject(i))->nType==STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && !pSubObj->bHidden && 
				!strncmp(pSubObj->name,"skeleton_",9) && (pSubObj1=FindSubObject((const char*)pSubObj->name+9)))
		{
			pe_params_skeleton ps;
			properties = szPropsOverride ? szPropsOverride : (const char*)pSubObj->properties;
			if (pval=strstr(properties, "stiffness")) {
				for(pval+=9;*pval && !isdigit(*pval);pval++); if (pval) ps.stiffness = atof(pval);
			}	if (pval=strstr(properties, "thickness")) {
				for(pval+=9;*pval && !isdigit(*pval);pval++); if (pval) ps.thickness = atof(pval);
			}	if (pval=strstr(properties, "max_stretch")) {
				for(pval+=11;*pval && !isdigit(*pval);pval++); if (pval) ps.maxStretch = atof(pval);
			}	if (pval=strstr(properties, "max_impulse")) {
				for(pval+=11;*pval && !isdigit(*pval);pval++); if (pval) ps.maxImpulse = atof(pval);
			}	if (pval=strstr(properties, "skin_dist")) {
				for(pval+=9;*pval && !isdigit(*pval);pval++); if (pval) pp.minContactDist = atof(pval);
			}	if (pval=strstr(properties, "hardness")) {
				for(pval+=8;*pval && !isdigit(*pval);pval++); if (pval) ps.hardness = atof(pval);
			}	if (pval=strstr(properties, "explosion_scale")) {
				for(pval+=15;*pval && !isdigit(*pval);pval++); if (pval) ps.explosionScale = atof(pval);
			}

			pp.partid=ps.partid = pSubObj1-&m_subObjects[0];
			pp.idSkeleton = i;
			pent->SetParams(&pp);
			pent->SetParams(&ps);
			((CStatObj*)pSubObj1->pStatObj)->PrepareSkinData(pSubObj1->localTM.GetInverted()*pSubObj->localTM, pSubObj->pStatObj->GetPhysGeom()->pGeom, 
				is_unused(pp.minContactDist) ? 0.0f:pp.minContactDist);
		}

	new(&pp) pe_params_part;
	for(i=0;i<nObj;i++)	
		if ((pSubObj=GetSubObject(i))->nType==STATIC_SUB_OBJECT_MESH && pSubObj->pStatObj && pSubObj->pStatObj->GetPhysGeom() && !pSubObj->bHidden && 
				!strncmp(pSubObj->name,"childof_",8) && (pSubObj1=FindSubObject((const char*)pSubObj->name+8)))
		{
			pp.partid = i;
			pp.idParent = pSubObj1-&m_subObjects[0];
			pent->SetParams(&pp);
			pSubObj->bHidden = true;
		}

	if (nGeoms>=10 && pent->GetType()==PE_STATIC)
		pf.flagsOR|=pef_parts_traceable, pf.flagsAND=~pef_traceable;

	if (pf.flagsOR)
		pent->SetParams(&pf);
	if (pJointBox)
		pJointBox->Release();
	return idNext-1;
}



//////////////////////////////////////////////////////////////////////////
////////////////////////////// Foliage ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void CStatObj::CopyFoliageData(IStatObj *pObjDst, bool bMove, IFoliage *pSrcFoliage, int *pVtxMap, primitives::box *pMovedBoxes,int nMovedBoxes)
{
	CStatObj *pDst = (CStatObj*)pObjDst;
	CMesh *pMesh = 0;
	int i,j,nVtx=0,nVtxDst=0,isubs,ibone,i1;
	if (GetIndexedMesh(true))
		nVtx=GetIndexedMesh(true)->GetVertexCount(), pMesh=GetIndexedMesh(true)->GetMesh();
	if (pObjDst->GetIndexedMesh(true))
		nVtxDst = pObjDst->GetIndexedMesh(true)->GetVertexCount();

	if (pDst->m_pSpines) {
		for(i=0;i<pDst->m_nSpines;i++)
			delete[] pDst->m_pSpines[i].pVtx, delete[] pDst->m_pSpines[i].pVtxCur;
		free(pDst->m_pSpines);
		if (pDst->m_pBoneMapping)
			delete[] pDst->m_pBoneMapping;
		pDst->m_pBoneMapping = 0;
	}
	pDst->m_nSpines = m_nSpines;

	if (bMove && nMovedBoxes==-1 || !m_pSpines) {
		pDst->m_pSpines = m_pSpines; m_pSpines = 0; m_nSpines = 0;
	} else {
		memcpy(pDst->m_pSpines = (SSpine*)malloc(m_nSpines*sizeof(SSpine)), m_pSpines, m_nSpines*sizeof(SSpine));
		//for(isubs=0; isubs<pMesh->m_subsets.size() && pMesh->m_subsets[isubs].m_arrGlobalBonesPerSubset.size()<2; isubs++);
		for(i1=isubs=0,ibone=1; i1<m_chunkBoneIds.size() && m_chunkBoneIds[i1]!=ibone; i1++)
			isubs += !m_chunkBoneIds[i1];
		for(i=0; i<m_nSpines; ibone += m_pSpines[i++].nVtx-1) 
		{
			memcpy(pDst->m_pSpines[i].pVtx = new Vec3[m_pSpines[i].nVtx], m_pSpines[i].pVtx, m_pSpines[i].nVtx*sizeof(Vec3));
			memcpy(pDst->m_pSpines[i].pSegDim = new Vec4[m_pSpines[i].nVtx], m_pSpines[i].pSegDim, m_pSpines[i].nVtx*sizeof(Vec4));
			pDst->m_pSpines[i].pVtxCur = new Vec3[m_pSpines[i].nVtx];
			if (pDst->m_pSpines[i].bActive = m_pSpines[i].bActive && (!pSrcFoliage || ((CStatObjFoliage*)pSrcFoliage)->m_pRopes[i]!=0)) 
			{
				/*for(; isubs<pMesh->m_subsets.size(); isubs=isubs1) {
					for(isubs1=isubs+1; isubs1<pMesh->m_subsets.size() && pMesh->m_subsets[isubs1].m_arrGlobalBonesPerSubset.size()<2; isubs1++);
					if (pMesh->m_subsets[isubs].m_arrGlobalBonesPerSubset[1]<=ibone && 
							(isubs1>=pMesh->m_subsets.size() || pMesh->m_subsets[isubs1].m_arrGlobalBonesPerSubset[1]>ibone))
						break;
				}*/
				for(; i1<m_chunkBoneIds.size() && m_chunkBoneIds[i1]!=ibone; i1++)
					isubs += !m_chunkBoneIds[i1];
				j = 0;
				if (isubs<pMesh->m_subsets.size()) for(;j<nMovedBoxes;j++) {
					Vec3 dist = pMovedBoxes[j].size-(pMovedBoxes[j].Basis*(pMesh->m_subsets[isubs].vCenter-pMovedBoxes[j].center)).abs();
					if (min(min(dist.x,dist.y),dist.z)>0)
						break;
				}	
				if (j==nMovedBoxes)	{
					pDst->m_pSpines[i].bActive = false;
					continue;
				}
				m_pSpines[i].bActive = !bMove;
			}
		}
	}

	if (m_pSpines && m_pBoneMapping) {
		pDst->m_pBoneMapping = pVtxMap || !bMove || nMovedBoxes>=0 ? new SMeshBoneMapping[nVtxDst] : m_pBoneMapping;
		if (pVtxMap) {
			memset(pDst->m_pBoneMapping, 0, nVtxDst*sizeof(SMeshBoneMapping));
			for(i=0;i<nVtx;i++) if (pVtxMap[i]>=0 && m_pBoneMapping[i].weights[0]+m_pBoneMapping[i].weights[1]>0)
				pDst->m_pBoneMapping[pVtxMap[i]] = m_pBoneMapping[i];
			if (bMove && nMovedBoxes==-1)
				delete[] m_pBoneMapping;
		}	else if (!bMove)
			memcpy(pDst->m_pBoneMapping, m_pBoneMapping, nVtx*sizeof(SMeshBoneMapping));
		if (bMove && nMovedBoxes==-1)
			m_pBoneMapping = 0;
		pDst->m_chunkBoneIds = m_chunkBoneIds;
	}
}


int UpdatePtTriDist(const Vec3 *vtx, const Vec3 &n, const Vec3 &pt, float &minDist,float &minDenom)
{
	float rvtx[3]={ (vtx[0]-pt).len2(),(vtx[1]-pt).len2(),(vtx[2]-pt).len2() }, elen2[2],dist,denom;
	int i=idxmin3(rvtx), bInside[2];
	Vec3 edge[2],dp=pt-vtx[i];

	edge[0] = vtx[incm3(i)]-vtx[i]; elen2[0] = edge[0].len2();
	edge[1] = vtx[decm3(i)]-vtx[i]; elen2[1] = edge[1].len2();
	bInside[0] = isneg((dp^edge[0])*n);
	bInside[1] = isneg((edge[1]^dp)*n);
	rvtx[i] = rvtx[i]*elen2[bInside[0]] - sqr(max(0.0f,dp*edge[bInside[0]]))*(bInside[0]|bInside[1]); 
	denom = elen2[bInside[0]];

	if (bInside[0]&bInside[1]) {
		if (edge[0]*edge[1]<0) {
			edge[0] = vtx[decm3(i)]-vtx[incm3(i)];
			dp = pt-vtx[incm3(i)];
			if ((dp^edge[0])*n>0)	{
				dist=rvtx[incm3(i)]*edge[0].len2()-sqr(dp*edge[0]); denom=edge[0].len2();
				goto found;
			}
		}
		dist=sqr((pt-vtx[0])*n), denom=n.len2();
	}	else
		dist = rvtx[i];

	found:
	if (dist*minDenom < minDist*denom) {
		minDist=dist; minDenom=denom;
		return 1;
	}
	return 0;
}


void SerializeMtx(TSerialize ser, const char *name, Matrix34 &mtx)
{
	Vec3 vec; int i;
	if (ser.IsReading())
		for(i=0;i<4;i++) { ser.Value(numbered_tag(name,i), vec); mtx.SetColumn(i,vec); }
	else 
		for(i=0;i<4;i++) { vec = mtx.GetColumn(i); ser.Value(numbered_tag(name,i), vec); }
}

//static const int g_nRanges = 5;
static int g_rngb2a[] = { 0,'A',26,'a',52,'0',62,'+',63,'/' };
static int g_rnga2b[] = { '+',62,'/',63,'0',52,'A',0,'a',26 };
inline int mapsymb(int symb, int *pmap,int n) {
	int i,j;
	for(i=j=0;j<n;j++) i+=isneg(symb-pmap[j*2]); i=n-1-i;
	return symb-pmap[i*2]+pmap[i*2+1];
}

int Bin2ascii(const unsigned char *pin,int sz, unsigned char *pout) 
{
	int a0,a1,a2,i,j,nout,chr[4];
	for(i=nout=0;i<sz;i+=3,nout+=4) {
		a0 = pin[i]; j=isneg(i+1-sz); a1 = pin[i+j]&-j; j=isneg(i+2-sz); a2 = pin[i+j*2]&-j;
		chr[0] = a0>>2; chr[1] = a0<<4&0x30 | (a1>>4)&0x0F; 
		chr[2] = a1<<2&0x3C | a2>>6&0x03; chr[3] = a2&0x03F;
		for(j=0;j<4;j++)
			*pout++ = mapsymb(chr[j],g_rngb2a,5);
	}
	return nout;	
}
int ascii2bin(const unsigned char *pin,int sz, unsigned char *pout,int szout)
{
	int a0,a1,a2,a3,i,nout;
	for(i=nout=0;i<sz-4;i+=4,nout+=3) {
		a0 = mapsymb(pin[i+0],g_rnga2b,5); a1 = mapsymb(pin[i+1],g_rnga2b,5); 
		a2 = mapsymb(pin[i+2],g_rnga2b,5); a3 = mapsymb(pin[i+3],g_rnga2b,5);
		*pout++ = a0<<2 | a1>>4; *pout++ = a1<<4&0xF0 | a2>>2&0x0F; *pout++ = a2<<6&0xC0 | a3;
	}
	a0 = mapsymb(pin[i+0],g_rnga2b,5); a1 = mapsymb(pin[i+1],g_rnga2b,5); 
	a2 = mapsymb(pin[i+2],g_rnga2b,5); a3 = mapsymb(pin[i+3],g_rnga2b,5);
	if (nout<szout)
		*pout++ = a0<<2 | a1>>4, nout++; 
	if (nout<szout)
		*pout++ = a1<<4&0xF0 | a2>>2&0x0F, nout++;
	if (nout<szout)
		*pout++ = a2<<6&0xC0 | a3, nout++;
	return nout;
}


void SerializeData(TSerialize ser, const char *name, void *pdata, int size)
{
	/*static std::vector<int> arr;
	if (ser.IsReading()) 
	{
		ser.Value(name, arr);
		memcpy(pdata, &arr[0], size);
	} else
	{
		arr.resize(((size-1)>>2)+1);
		memcpy(&arr[0], pdata, size);
		ser.Value(name, arr);
	}*/
	static string str;
	if (!size)
		return;
	if (ser.IsReading())
	{
		ser.Value(name, str);
		int	n = ascii2bin((const unsigned char*)(const char*)str, str.length(), (unsigned char*)pdata, size);
		assert(n == size);
		(void)n;
	}	else
	{
		str.resize(((size-1)/3+1)*4);
		int n = Bin2ascii((const unsigned char*)pdata, size, (unsigned char*)(const char*)str);
		assert(n == str.length());
		(void)n;
		ser.Value(name, str);
	}
}


int CStatObj::Serialize(TSerialize ser)
{
	ser.BeginGroup("StatObj");
	ser.Value("Flags", m_nFlags);
	if (GetFlags() & STATIC_OBJECT_COMPOUND)
	{
		int i,nParts=m_subObjects.size();
		bool bVal;
		string srcObjName;
		ser.Value("nParts", nParts);
		if (m_pClonedSourceObject)
		{
			SetSubObjectCount(nParts);
			for(i=0;i<nParts;i++)
			{
				ser.BeginGroup("part");
				ser.Value("bGenerated", bVal=!ser.IsReading() && m_subObjects[i].pStatObj && 
					m_subObjects[i].pStatObj->GetFlags() & STATIC_OBJECT_GENERATED && !m_subObjects[i].bHidden);
				if (bVal)
				{
					if (ser.IsReading())
						(m_subObjects[i].pStatObj = gEnv->p3DEngine->CreateStatObj())->AddRef();
					m_subObjects[i].pStatObj->Serialize(ser);
				}	else
				{
					ser.Value("subobj", m_subObjects[i].name);
					if (ser.IsReading())
					{
						IStatObj::SSubObject *pSrcSubObj = m_pClonedSourceObject->FindSubObject(m_subObjects[i].name);
						if (pSrcSubObj)
						{
							m_subObjects[i] = *pSrcSubObj;
							if (pSrcSubObj->pStatObj)
								pSrcSubObj->pStatObj->AddRef();
						}
					}
				}
				bVal=m_subObjects[i].bHidden; ser.Value("hidden", bVal); m_subObjects[i].bHidden=bVal;
				ser.EndGroup();
			}
		}
	} else
	{
		CMesh *pMesh;
		int i,nVtx,nTris,nSubsets;
		string matName;
		IMaterial *pMat;

		if (ser.IsReading())
		{
			ser.Value("nvtx", nVtx=0);
			if (nVtx)
			{
				m_pIndexedMesh = new CIndexedMesh();
				pMesh = m_pIndexedMesh->GetMesh();
				ser.Value("ntris", nTris);
				ser.Value("nsubsets", nSubsets);
				pMesh->SetVertexCount(nVtx);
				pMesh->SetTexCoordsAndTangentsCount(nVtx);
				pMesh->SetIndexCount(nTris*3);

				for(i=0;i<nSubsets;i++) 
				{
					SMeshSubset mss;
					ser.BeginGroup("subset");
					ser.Value("matid", mss.nMatID);
					ser.Value("matflg", mss.nMatFlags);
					ser.Value("vtx0", mss.nFirstVertId);
					ser.Value("nvtx", mss.nNumVerts);
					ser.Value("idx0", mss.nFirstIndexId);
					ser.Value("nidx", mss.nNumIndices);
					ser.Value("center", mss.vCenter);
					ser.Value("radius", mss.fRadius);
					pMesh->m_subsets.push_back(mss);
					ser.EndGroup();
				}

				SerializeData(ser, "Positions", pMesh->m_pPositions, nVtx*sizeof(pMesh->m_pPositions[0]));
				SerializeData(ser, "Normals", pMesh->m_pNorms, nVtx*sizeof(pMesh->m_pNorms[0]));
				SerializeData(ser, "TexCoord", pMesh->m_pTexCoord, nVtx*sizeof(pMesh->m_pTexCoord[0]));
				SerializeData(ser, "Tangents", pMesh->m_pTangents, nVtx*sizeof(pMesh->m_pTangents[0]));
				SerializeData(ser, "Indices", pMesh->m_pIndices, nTris*3*sizeof(pMesh->m_pIndices[0]));

				ser.Value("Material", matName);
				SetMaterial(gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName));
				ser.Value("MaterialAux", matName);
				if (m_pMaterial && (pMat=gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName)))
				{
					if (pMat->GetSubMtlCount()>0)
						pMat = pMat->GetSubMtl(0);
					for(i=m_pMaterial->GetSubMtlCount()-1; i>=0 && strcmp(m_pMaterial->GetSubMtl(i)->GetName(),matName); i--);
					if (i<0)
					{	
						i = m_pMaterial->GetSubMtlCount(); m_pMaterial->SetSubMtlCount(i+1);
						m_pMaterial->SetSubMtl(i, pMat);
					}
				}

				int surfaceTypesId[MAX_SUB_MATERIALS];
				memset( surfaceTypesId,0,sizeof(surfaceTypesId) );
				int numIds=0;
				if (m_pMaterial)
					numIds = m_pMaterial->FillSurfaceTypeIds(surfaceTypesId);


				char *pIds = new char[nTris]; memset(pIds,0,nTris);
				int j,itri;
				for(i=0;i<pMesh->m_subsets.size();i++) 
					for(itri=(j=pMesh->m_subsets[i].nFirstIndexId)/3; j<pMesh->m_subsets[i].nFirstIndexId+pMesh->m_subsets[i].nNumIndices; j+=3,itri++)
						pIds[itri] = pMesh->m_subsets[i].nMatID;

				ser.Value("PhysSz", i);
				if (i) 
				{
					char *pbuf = new char[i];
					CMemStream stm(pbuf,i,false);
					SerializeData(ser, "PhysMeshData", pbuf,i);
					m_pSrcPhysMesh = GetPhysicalWorld()->GetGeomManager()->LoadGeometry(stm, pMesh->m_pPositions,pMesh->m_pIndices,pIds);
					m_pSrcPhysMesh->AddRef();
					m_pSrcPhysMesh->SetForeignData((IStatObj*)this,0);
					m_arrPhysGeomInfo.SetPhysGeom(GetPhysicalWorld()->GetGeomManager()->RegisterGeometry(m_pSrcPhysMesh, 0, surfaceTypesId,numIds));
					delete[] pbuf; 
				} delete[] pIds;

				Invalidate();
				SetFlags(STATIC_OBJECT_GENERATED);
			}
		}	else
		{
			if (GetIndexedMesh(true))
			{
				pMesh = GetIndexedMesh(true)->GetMesh();
				ser.Value("nvtx", nVtx=pMesh->GetVertexCount());
				ser.Value("ntris", nTris=pMesh->GetIndexCount()/3);
				ser.Value("nsubsets", nSubsets=pMesh->m_subsets.size());

				for(i=0;i<nSubsets;i++) 
				{
					ser.BeginGroup("subset");
					ser.Value("matid", pMesh->m_subsets[i].nMatID);
					ser.Value("matflg", pMesh->m_subsets[i].nMatFlags);
					ser.Value("vtx0", pMesh->m_subsets[i].nFirstVertId);
					ser.Value("nvtx", pMesh->m_subsets[i].nNumVerts);
					ser.Value("idx0", pMesh->m_subsets[i].nFirstIndexId);
					ser.Value("nidx", pMesh->m_subsets[i].nNumIndices);
					ser.Value("center", pMesh->m_subsets[i].vCenter);
					ser.Value("radius", pMesh->m_subsets[i].fRadius);
					ser.EndGroup();
				}

				if (m_pMaterial && m_pMaterial->GetSubMtlCount()>0)
					ser.Value("auxmatname", m_pMaterial->GetSubMtl(m_pMaterial->GetSubMtlCount()-1)->GetName());

				SerializeData(ser, "Positions", pMesh->m_pPositions, nVtx*sizeof(pMesh->m_pPositions[0]));
				SerializeData(ser, "Normals", pMesh->m_pNorms, nVtx*sizeof(pMesh->m_pNorms[0]));
				SerializeData(ser, "TexCoord", pMesh->m_pTexCoord, nVtx*sizeof(pMesh->m_pTexCoord[0]));
				SerializeData(ser, "Tangents", pMesh->m_pTangents, nVtx*sizeof(pMesh->m_pTangents[0]));
				SerializeData(ser, "Indices", pMesh->m_pIndices, nTris*3*sizeof(pMesh->m_pIndices[0]));

				ser.Value("Material", GetMaterial()->GetName());
				if (m_pMaterial && m_pMaterial->GetSubMtlCount()>0)
					ser.Value("MaterialAux", m_pMaterial->GetSubMtl(m_pMaterial->GetSubMtlCount()-1)->GetName());
				else
					ser.Value("MaterialAux", matName="");

				if (m_pSrcPhysMesh) 
				{
					CMemStream stm(false);
					GetPhysicalWorld()->GetGeomManager()->SaveGeometry(stm, m_pSrcPhysMesh);
					ser.Value("PhysSz", i=stm.GetUsedSize());
					SerializeData(ser, "PhysMeshData", stm.GetBuf(),i);
				}	else
					ser.Value("PhysSz", i=0);
			} else
				ser.Value("nvtx", nVtx=0);
		}
	}

	ser.EndGroup(); //StatObj

	return 1;
}

//////////////////////////////////////////////////////////////////////////
IParticleEffect* CStatObj::GetSurfaceBreakageEffect( const char *sType )
{
	if (!m_pRenderMesh)
		return 0;

	IMaterial *pSubMtl;
	PodArray<CRenderChunk> &meshChunks = m_pRenderMesh->GetChunks();
	for(uint32 iss=0; iss<meshChunks.size(); iss++) 
	{
		if ((meshChunks[iss].m_nMatFlags & (MTL_FLAG_NOPHYSICALIZE|MTL_FLAG_NODRAW)) == MTL_FLAG_NOPHYSICALIZE ||
			meshChunks.size()==1 && !m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT])
			if (m_pMaterial && ((pSubMtl=m_pMaterial->GetSubMtl(meshChunks[iss].m_nMatID)) || (pSubMtl=m_pMaterial)))
			{
				//idmat = pSubMtl->GetSurfaceTypeId();
				ISurfaceType *pSurface = pSubMtl->GetSurfaceType();
				ISurfaceType::SBreakageParticles *pParticles = pSurface->GetBreakageParticles(sType);
				if (pParticles)
				{
					return gEnv->pParticleManager->FindEffect(pParticles->particle_effect);
				}
			}
	}
	return 0;
}

struct SBranchPt {
	Vec3 pt;
	Vec2 tex;
	int itri;
	float minDist,minDenom;
};

struct SBranch {
	SBranchPt *pt;
	int npt;
	float len;
};

void CStatObj::FreeFoliageData()
{
  if (m_pSpines)
  {
    for(int i=0;i<m_nSpines;i++)
      delete[] m_pSpines[i].pVtx, delete[] m_pSpines[i].pVtxCur, delete[] m_pSpines[i].pSegDim;
    free(m_pSpines); m_pSpines = 0; m_nSpines = 0;
  }
  if (m_pBoneMapping)
  {
    if (m_pBoneMapping)
      delete[] m_pBoneMapping;
    m_pBoneMapping = 0;
  }
  m_chunkBoneIds.resize(0);
}

void CStatObj::AnalizeFoliage(IRenderMesh * pRenderMesh, CContentCGF *pCGF)
{
  FUNCTION_PROFILER_3DENGINE;

	if (pRenderMesh && m_nFlags & STATIC_OBJECT_DYNAMIC)
		pRenderMesh->KeepSysMesh(true);
  if(!FindSubObject_StrStr("branch"))
    return;

  FreeFoliageData();

  assert(!m_nSpines && !m_pSpines && !m_pBoneMapping);

	if (!pRenderMesh)
		return;
	int i,j,isle,iss,ivtx0,nVtx,itri,ivtx,iedge,ibran0,ibran1,ibran,nBranches,ispine,ispineDst,nGlobalBones,idmat=0, 
			*pIdxSorted,*pForeignIdx;
	int nMaxBones = 64;//48 + GetCVars()->e_VegetationSphericalSkinning * ( 72 - 48 );
	Vec3 pt[7],edge,edge1,n,nprev,axisx,axisy;
	Vec2 tex[6];
	float t,s,mindist,k,denom,len,dist[3],e,t0,t1,denomPrev;
	unsigned int *pUsedTris,*pUsedVtx;
	char sname[64];
	mesh_data *pmd;
	IGeometry *pPhysGeom;
	SBranch branch,*pBranches;
	IMaterial *pSubMtl;
	primitives::box bbox;

	int nMeshVtx,nMeshIdx;
	unsigned short *pMeshIdx;
	PodArray<CRenderChunk> &meshChunks = pRenderMesh->GetChunks();
	nMeshVtx = pRenderMesh->GetVerticesCount();
  nMeshIdx = pRenderMesh->GetIndicesCount();
	pMeshIdx = pRenderMesh->GetIndexPtr(FSL_READ);
  strided_pointer<Vec3> pMeshVtx;
  strided_pointer<Vec2> pMeshTex;
  pMeshVtx.data = (Vec3*)pRenderMesh->GetPosPtr(pMeshVtx.iStride, FSL_READ);
	pMeshTex.data = (Vec2*)pRenderMesh->GetUVPtr(pMeshTex.iStride, FSL_READ);
#ifdef FP16_MESH
  strided_pointer<Vec2f16> pMeshTexX;
  Vec2 *pTempTex = new Vec2[nMeshVtx];
  pMeshTexX.data = (Vec2f16 *)pMeshTex.data;
  pMeshTexX.iStride = pMeshTex.iStride;
  for (int kk=0; kk<nMeshVtx; kk++)
  {
    pTempTex[kk] = pMeshTexX[kk].ToVec2();
  }
  pMeshTex.data = pTempTex;
  pMeshTex.iStride = sizeof(Vec2);
#endif

	for(iss=0; iss<(int)meshChunks.size(); iss++) 
	if ((meshChunks[iss].m_nMatFlags & (MTL_FLAG_NOPHYSICALIZE|MTL_FLAG_NODRAW)) == MTL_FLAG_NOPHYSICALIZE ||
			meshChunks.size()==1 && !m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT])
		if (m_pMaterial && ((pSubMtl=m_pMaterial->GetSubMtl(meshChunks[iss].m_nMatID)) || (pSubMtl=m_pMaterial)))
		{
			idmat = pSubMtl->GetSurfaceTypeId();
			break;
		}

	if (!m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT] || m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT]->pGeom->GetType()!=GEOM_TRIMESH || !meshChunks.size())
		m_nFlags |= STATIC_OBJECT_CANT_BREAK;
	else 
	{
		pmd = (mesh_data*)m_arrPhysGeomInfo[PHYS_GEOM_TYPE_DEFAULT]->pGeom->GetData();
		for(iss=1; iss<(int)meshChunks.size() && meshChunks[iss].m_nMatFlags & MTL_FLAG_NOPHYSICALIZE; iss++);
		if (meshChunks[0].m_nMatFlags & MTL_FLAG_NOPHYSICALIZE || !pmd->pForeignIdx || pmd->nTris*3!=meshChunks[0].nNumIndices || 
			  iss<(int)meshChunks.size() || !pmd->pVtxMap)
			m_nFlags |= STATIC_OBJECT_CANT_BREAK;
		else
		{
			for(i=0; i<pmd->nTris && pmd->pForeignIdx[i]*3>=meshChunks[0].nFirstIndexId && 
					pmd->pForeignIdx[i]*3<meshChunks[0].nFirstIndexId+meshChunks[0].nNumIndices; i++);
			if (i<pmd->nTris)
				m_nFlags |= STATIC_OBJECT_CANT_BREAK;
		}
	}
	if (!(m_nFlags & STATIC_OBJECT_CANT_BREAK)) 
	{
		pRenderMesh->KeepSysMesh(true);
		m_nFlags |= STATIC_OBJECT_DYNAMIC;
	}

	phys_geometry *pgeom;
	if ((pgeom=m_arrPhysGeomInfo[PHYS_GEOM_TYPE_OBSTRUCT]) && pgeom->pMatMapping)
		idmat = pgeom->pMatMapping[pgeom->surface_idx];
	if ((pgeom=m_arrPhysGeomInfo[PHYS_GEOM_TYPE_NO_COLLIDE]) && pgeom->pMatMapping)
		idmat = pgeom->pMatMapping[pgeom->surface_idx];

	if (pCGF && pCGF->GetFoliageInfo()->nSpines)
	{
		SFoliageInfoCGF &fi = *pCGF->GetFoliageInfo();
		m_pSpines = (SSpine*)malloc((m_nSpines=fi.nSpines)*sizeof(SSpine));
		int iLastSpine=0,nBones=1;
		for(i=0; i<m_nSpines; i++)
		{
			m_pSpines[i].bActive = true;
			m_pSpines[i].idmat = idmat;
			m_pSpines[i].len = fi.pSpines[i].len;
			m_pSpines[i].navg = fi.pSpines[i].navg;
			m_pSpines[i].iAttachSpine = fi.pSpines[i].iAttachSpine;
			m_pSpines[i].iAttachSeg = fi.pSpines[i].iAttachSeg;
			m_pSpines[i].nVtx = fi.pSpines[i].nVtx;
			memcpy(m_pSpines[i].pVtx = new Vec3[m_pSpines[i].nVtx], fi.pSpines[i].pVtx, sizeof(Vec3)*m_pSpines[i].nVtx);
			memcpy(m_pSpines[i].pSegDim = new Vec4[m_pSpines[i].nVtx], fi.pSpines[i].pSegDim, sizeof(Vec4)*m_pSpines[i].nVtx);
			SwapEndian(m_pSpines[i].pVtx, m_pSpines[i].nVtx);
			SwapEndian((float*)m_pSpines[i].pSegDim, m_pSpines[i].nVtx*4);
			m_pSpines[i].pVtxCur = new Vec3[m_pSpines[i].nVtx];
			if (nBones+m_pSpines[i].nVtx-1 <= nMaxBones)
				iLastSpine=i, nBones+=m_pSpines[i].nVtx-1;
		}
		m_chunkBoneIds = fi.chunkBoneIds;

		if (GetCVars()->e_VegetationBoneInfo)
		{
			if (m_chunkBoneIds.size() <= nMaxBones)
				CryLog("Vegetation object [%s] has [%d] bones", m_szFileName.c_str(), m_chunkBoneIds.size());
			else
				CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_WARNING,"Vegetation object[%s] has [%d] bones - maximum supported is [%d]", 
					m_szFileName.c_str(), m_chunkBoneIds.size(), nMaxBones);
		}

		memcpy(m_pBoneMapping = new SMeshBoneMapping[fi.nSkinnedVtx], fi.pBoneMapping, sizeof(SMeshBoneMapping)*fi.nSkinnedVtx);
		if (m_nSpines > iLastSpine+1)
		{
			for(; m_nSpines>iLastSpine+1; m_nSpines--)
				delete[] m_pSpines[m_nSpines-1].pVtx, delete[] m_pSpines[m_nSpines-1].pVtxCur, delete[] m_pSpines[m_nSpines-1].pSegDim;
			for(i=0; i<m_chunkBoneIds.size(); i++)
				m_chunkBoneIds[i] &= (m_chunkBoneIds[i]-nBones)>>31;
			for(i=0; i<fi.nSkinnedVtx; i++) for(int jj=0;jj<4;jj++)
				m_pBoneMapping[i].boneIDs[jj] &= ((int)m_pBoneMapping[i].boneIDs[jj]-nBones)>>31;
		}
		return;
	}

	// iterate through branch points branch#_#, build a branch structure
	pBranches=0; nBranches=0;
	do {
		branch.pt=0; branch.npt=0; branch.len=0;
		do {
			sprintf(sname,"branch%d_%d",nBranches+1,branch.npt+1);
			if ((pt[0]=GetHelperPos(sname)).len2()==0)
				break;
			if ((branch.npt & 15)==0)
				branch.pt = (SBranchPt*)realloc(branch.pt, (branch.npt+16)*sizeof(SBranchPt));
			branch.pt[branch.npt].minDist = 1; branch.pt[branch.npt].minDenom = 0;
			branch.pt[branch.npt].itri = -1;
			branch.pt[branch.npt++].pt = pt[0];
			branch.len += (pt[0]-branch.pt[max(0,branch.npt-2)].pt).len();
		} while(true);
		if (branch.npt<2)	{
			if (branch.pt) free(branch.pt);
			break;
		}
		if ((nBranches & 15)==0)
			pBranches = (SBranch*)realloc(pBranches,(nBranches+16)*sizeof(SBranch));
		pBranches[nBranches++] = branch;
		// make sure the segments have equal length
		len = branch.len/(branch.npt-1);
		for(i=1;i<branch.npt;i++)
			branch.pt[i].pt = branch.pt[i-1].pt+(branch.pt[i].pt-branch.pt[i-1].pt).normalized()*len;
	} while(true);

	if (nBranches==0)
		return;
	SSpine spine;
	memset(&spine, 0, sizeof spine); spine.bActive = true;
	memset(m_pBoneMapping = new SMeshBoneMapping[nMeshVtx], 0, nMeshVtx*sizeof(SMeshBoneMapping));
	for(i=0; i<nMeshVtx; i++)
		m_pBoneMapping[i].weights[0] = 255;
	pIdxSorted = new int[nMeshVtx];	nVtx = 0;
	pForeignIdx = new int[nMeshIdx/3];
	pUsedTris = new unsigned int[i = ((nMeshIdx-1)>>5)+1]; memset(pUsedTris,0,i*sizeof(int));
	pUsedVtx = new unsigned int[i = ((nMeshVtx-1)>>5)+1];	memset(pUsedVtx,0,i*sizeof(int));

	// iterate through all triangles, track the closest for each branch point
	for(iss=0; iss<(int)meshChunks.size(); iss++) 
	if ((meshChunks[iss].m_nMatFlags & (MTL_FLAG_NOPHYSICALIZE|MTL_FLAG_NODRAW)) == MTL_FLAG_NOPHYSICALIZE)
		for(i=meshChunks[iss].nFirstIndexId; i<meshChunks[iss].nFirstIndexId+meshChunks[iss].nNumIndices; i+=3)
		{
			for(j=0;j<3;j++) pt[j] = pMeshVtx[pMeshIdx[i+j]];	n = pt[1]-pt[0] ^ pt[2]-pt[0];
			for(ibran=0; ibran<nBranches; ibran++) for(ivtx0=0; ivtx0<pBranches[ibran].npt; ivtx0++)
				if (UpdatePtTriDist(pt,n, pBranches[ibran].pt[ivtx0].pt, pBranches[ibran].pt[ivtx0].minDist, pBranches[ibran].pt[ivtx0].minDenom))
					pBranches[ibran].pt[ivtx0].itri = i;
		}

	for(ibran=0;ibran<nBranches;ibran++) for(i=0;i<pBranches[ibran].npt;i++) if(pBranches[ibran].pt[i].itri>=0)
	{	// get the closest point on the triangle for each vertex, get tex coords from it
		for(j=0;j<3;j++) pt[j] = pMeshVtx[pMeshIdx[pBranches[ibran].pt[i].itri+j]];
		n = pt[1]-pt[0] ^ pt[2]-pt[0]; pt[3] = pBranches[ibran].pt[i].pt;
		for(j=0;j<3 && (pt[incm3(j)]-pt[j] ^ pt[3]-pt[j])*n>0;j++);
		if (j==3)	
		{	// the closest pt is in triangle interior
			denom = 1.0f/n.len2(); 
			pt[3] -= n*(n*(pt[3]-pt[0]))*denom;
			for(j=0,s=0,t=0; j<3; j++) {
				k = ((pt[3]-pt[incm3(j)] ^ pt[3]-pt[decm3(j)])*n)*denom;
				s += pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].x*k;
				t += pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].y*k;
			}
		} else {
			for(j=0;j<3 && !inrange((pt[incm3(j)]-pt[j])*(pt[3]-pt[j]), 0.0f,(pt[incm3(j)]-pt[j]).len2());j++);
			if (j<3) 
			{ // the closest pt in on an edge
				k = ((pt[3]-pt[j])*(pt[incm3(j)]-pt[j]))/(pt[incm3(j)]-pt[j]).len2();
				s = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].x*(1-k)+
					  pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+incm3(j)]].x*k;
				t = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].y*(1-k)+
						pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+incm3(j)]].y*k;
			} else 
			{	// the closest pt is a vertex
				for(j=0;j<3;j++) dist[j] = (pt[j]-pt[3]).len2();
				j = idxmin3(dist);
				s = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].x;
				t = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].y;
			}
		}
		pBranches[ibran].pt[i].tex.set(s,t);
	}

	int nBones;
	for(iss=0,nGlobalBones=1; iss<(int)meshChunks.size(); iss++,nGlobalBones+=nBones-1) 
	if ((meshChunks[iss].m_nMatFlags & (MTL_FLAG_NOPHYSICALIZE|MTL_FLAG_NODRAW)) == MTL_FLAG_NOPHYSICALIZE)
	{
		// fill the foreign idx list
		for(i=0,j=meshChunks[iss].nFirstIndexId/3; j*3<meshChunks[iss].nFirstIndexId+meshChunks[iss].nNumIndices;	j++,i++)
			pForeignIdx[i] = j;
		pPhysGeom = GetPhysicalWorld()->GetGeomManager()->CreateMesh(pMeshVtx, pMeshIdx+meshChunks[iss].nFirstIndexId, 0,pForeignIdx, 
			meshChunks[iss].nNumIndices/3, mesh_shared_vtx|mesh_SingleBB|mesh_keep_vtxmap);
		pmd = (mesh_data*)pPhysGeom->GetData();
		pPhysGeom->GetBBox(&bbox);
		e = sqr((bbox.size.x+bbox.size.y+bbox.size.z)*0.002f);
		nBones = 1;
		//meshChunks[iss].m_arrChunkBoneIDs.push_back(0);
		m_chunkBoneIds.push_back(0);

		for(isle=0; isle<pmd->nIslands; isle++)	if (pmd->pIslands[isle].nTris>=4)
		{
			ibran0=0; ibran1=nBranches; ivtx=0; 
			spine.nVtx=0; spine.pVtx=0; spine.iAttachSpine=spine.iAttachSeg = -1;
			do {
				for(itri=pmd->pIslands[isle].itri,i=j=0; i<pmd->pIslands[isle].nTris; itri=pmd->pTri2Island[itri].inext,i++)
				{
					for(j=0;j<3;j++) tex[j].set(pMeshTex[pMeshIdx[pmd->pForeignIdx[itri]*3+j]].x, 
																			pMeshTex[pMeshIdx[pmd->pForeignIdx[itri]*3+j]].y);
					k = tex[1]-tex[0] ^ tex[2]-tex[0]; s = fabs_tpl(k);
					for(ibran=ibran0,j=0; ibran<ibran1; ibran++) {
						for(j=0;j<3 && sqr_signed(tex[incm3(j)]-tex[j] ^ pBranches[ibran].pt[ivtx].tex-tex[j])*k>-e*s*(tex[incm3(j)]-tex[j]).GetLength2(); j++);
						if (j==3) goto foundtri;
					}
				}
				break; // if no tri has the next point in texture space, skip the rest
				foundtri:
				if (nBones+pBranches[ibran].npt-1 > nMaxBones)
					break;

				// output phys vtx
				if (!spine.pVtx)
					spine.pVtx = new Vec3[pBranches[ibran].npt];
				denom = 1.0f/(tex[1]-tex[0] ^ tex[2]-tex[0]);
				tex[3] = pBranches[ibran].pt[ivtx].tex;
				for(j=0,pt[0].zero(); j<3; j++)
					pt[0] += pmd->pVertices[pmd->pIndices[itri*3+j]].scale((tex[3]-tex[incm3(j)]^tex[3]-tex[decm3(j)]))*denom;
				spine.pVtx[ivtx++] = pt[0];
				
				ibran0 = ibran; ibran1 = ibran+1;
			} while(ivtx<pBranches[ibran].npt);

			if (ivtx<3)	{
				if (spine.pVtx) delete[] spine.pVtx;
				continue;
			}
			spine.nVtx = ivtx;
			spine.pVtxCur = new Vec3[spine.nVtx];
			spine.pSegDim = new Vec4[spine.nVtx];
			spine.idmat = idmat;

			// enforce equal length for phys rope segs
			for(i=0,len=0; i<spine.nVtx-1; i++)
				len += (spine.pVtx[i+1]-spine.pVtx[i]).len();
			spine.len = len;
			for(i=1,len/=(spine.nVtx-1); i<spine.nVtx; i++)
				spine.pVtx[i] = spine.pVtx[i-1]+(spine.pVtx[i]-spine.pVtx[i-1]).normalized()*len;

			// append island's vertices to the vertex index list
			for(itri=pmd->pIslands[isle].itri,i=0,ivtx0=nVtx,n.zero(); i<pmd->pIslands[isle].nTris; itri=pmd->pTri2Island[itri].inext,i++) 
			{
				for(iedge=0;iedge<3;iedge++) if (!check_mask(pUsedVtx, pMeshIdx[pmd->pForeignIdx[itri]*3+iedge]))
					set_mask(pUsedVtx, pIdxSorted[nVtx++]=pMeshIdx[pmd->pForeignIdx[itri]*3+iedge]);
				nprev = pMeshVtx[pMeshIdx[pmd->pForeignIdx[itri]*3+1]].sub(pMeshVtx[pMeshIdx[pmd->pForeignIdx[itri]*3]]) ^
								pMeshVtx[pMeshIdx[pmd->pForeignIdx[itri]*3+2]].sub(pMeshVtx[pMeshIdx[pmd->pForeignIdx[itri]*3]]);
				n += nprev*sgnnz(nprev.z);
			}
			spine.navg = n.normalized();

			nprev = spine.pVtx[1]-spine.pVtx[0];
			for(ivtx=0; ivtx<spine.nVtx-1; ivtx++) 
			{	// generate a separating plane between current seg and next seg	and move vertices that are sliced by it to the front
				n = edge = spine.pVtx[ivtx+1]-spine.pVtx[ivtx];
				axisx=(n^spine.navg).normalized(); axisy=axisx^n;	spine.pSegDim[ivtx]=Vec4(0,0,0,0);
				if (ivtx<spine.nVtx-2)
					n += spine.pVtx[ivtx+2]-spine.pVtx[ivtx+1]; 
				denom = 1.0f/(edge*n); denomPrev = 1.0f/(edge*nprev);
				for(i=ivtx0;i<nVtx;i++) if ((t1 = (spine.pVtx[ivtx+1].sub(pMeshVtx[pIdxSorted[i]]))*n)>0) {
					t = (pMeshVtx[pIdxSorted[i]].sub(spine.pVtx[ivtx]))*axisx;
					spine.pSegDim[ivtx].x=min(t,spine.pSegDim[ivtx].x); spine.pSegDim[ivtx].y=max(t,spine.pSegDim[ivtx].y);
					t = (pMeshVtx[pIdxSorted[i]].sub(spine.pVtx[ivtx]))*axisy;
					spine.pSegDim[ivtx].z=min(t,spine.pSegDim[ivtx].z); spine.pSegDim[ivtx].w=max(t,spine.pSegDim[ivtx].w);
					t0 = ((spine.pVtx[ivtx].sub(pMeshVtx[pIdxSorted[i]]))*nprev)*denomPrev; t1 *= denom;
					t = min(1.0f,max(0.0f,1.0f-fabs_tpl(t0+t1)*0.5f/(t1-t0)));
					j = isneg(t0+t1);
					m_pBoneMapping[pIdxSorted[i]].boneIDs[0] = nBones+min(spine.nVtx-2,ivtx+j);
					m_pBoneMapping[pIdxSorted[i]].boneIDs[1] = nBones+max(0,ivtx-1+j);
					m_pBoneMapping[pIdxSorted[i]].weights[j^1] = 255-(m_pBoneMapping[pIdxSorted[i]].weights[j] = max(1,min(254,FtoI(t*255))));
					j=pIdxSorted[ivtx0]; pIdxSorted[ivtx0++]=pIdxSorted[i]; pIdxSorted[i]=j;
				}
				nprev = n;
				//meshChunks[iss].m_arrChunkBoneIDs.push_back(nGlobalBones+nBones-1+ivtx);
				m_chunkBoneIds.push_back(nGlobalBones+nBones-1+ivtx);
			}
			for(i=ivtx0;i<nVtx;i++) {
				m_pBoneMapping[pIdxSorted[i]].boneIDs[0] = nBones+spine.nVtx-2;
				m_pBoneMapping[pIdxSorted[i]].weights[0] = 255;
			}

			if ((m_nSpines & 7)==0)
				m_pSpines = (SSpine*)realloc(m_pSpines, (m_nSpines+8)*sizeof(SSpine));
			m_pSpines[m_nSpines++] = spine;
			nBones += spine.nVtx-1;
		}
		pPhysGeom->Release();
	} else
		nBones = 1;

	// for every spine, check if its base is close enough to a point on another spine 
	for(ispine=0; ispine<m_nSpines; ispine++)
		for(ispineDst=0,pt[0]=m_pSpines[ispine].pVtx[0]; ispineDst<m_nSpines; ispineDst++) 
			if (ispine!=ispineDst && (ispineDst>ispine || m_pSpines[ispineDst].iAttachSeg!=ispine))
			{
				mindist = sqr(m_pSpines[ispine].len*0.05f); 
				for(i=0,j=-1; i<m_pSpines[ispineDst].nVtx-1; i++) 
				{	// find the point on this seg closest to pt[0]
					if ((t=(m_pSpines[ispineDst].pVtx[i+1]-pt[0]).len2())<mindist)
						mindist=t, j=i;	// end vertex
					edge = m_pSpines[ispineDst].pVtx[i+1]-m_pSpines[ispineDst].pVtx[i];
					edge1 = pt[0]-m_pSpines[ispineDst].pVtx[i];
					if (inrange(edge1*edge, edge.len2()*-0.3f*((i-1)>>31),edge.len2()) && (t=(edge^edge1).len2()) < mindist*edge.len2())
						mindist=t/edge.len2(), j=i;	// point on edge
				}
				if (j>=0)
				{	// attach ispine to a point on iSpineDst
					m_pSpines[ispine].iAttachSpine = ispineDst;
					m_pSpines[ispine].iAttachSeg = j;
					break;
				}
			}

	delete[] pUsedVtx; delete[] pUsedTris; delete[] pForeignIdx; delete[] pIdxSorted;
	spine.pVtx=spine.pVtxCur = 0; spine.pSegDim=0;

#ifdef FP16_MESH
  SAFE_DELETE_ARRAY(pTempTex);
#endif
}


int CStatObj::PhysicalizeFoliage(IPhysicalEntity *pTrunk, const Matrix34 &mtxWorld, IFoliage *&pIRes, float lifeTime, int iSource)
{
	if (!m_pSpines || GetCVars()->e_PhysFoliage<1+(pTrunk && pTrunk->GetType()==PE_STATIC))
		return 0;
	CStatObjFoliage *pRes=(CStatObjFoliage*)pIRes,*pFirstFoliage = Get3DEngine()->m_pFirstFoliage;
	int i,j;
	pe_params_flags pf;
	pe_simulation_params sp;

	if (pIRes) {
		pRes->m_timeIdle = 0;
		if (iSource & 4)
		{
			pf.flagsAND = ~pef_ignore_areas;
			pe_action_awake aa;
			for(i=0; i<m_nSpines; i++) if (pRes->m_pRopes[i])
				pRes->m_pRopes[i]->Action(&aa), pRes->m_pRopes[i]->SetParams(&pf);
			MARK_UNUSED pf.flagsAND;
		}
		pRes->m_iActivationSource |= iSource&2;
		if (!(iSource&1) && (pRes->m_iActivationSource&1))
		{
			pRes->m_iActivationSource = iSource;
			pf.flagsOR = rope_collides;
			sp.minEnergy = sqr(0.1f);
			if (pTrunk && pTrunk->GetType()!=PE_STATIC)
				pf.flagsOR |= rope_collides_with_terrain;
			for(i=0; i<m_nSpines; i++) if (pRes->m_pRopes[i])
				pRes->m_pRopes[i]->SetParams(&pf), pRes->m_pRopes[i]->SetParams(&sp);
		}
		return 0;
	}

	pRes = new CStatObjFoliage;
	AddRef();
	m_nPhysFoliages++;

	pRes->m_next = pFirstFoliage;
	pRes->m_prev = pFirstFoliage->m_prev;
	pFirstFoliage->m_prev->m_next = pRes;
	pFirstFoliage->m_prev = pRes;
	pRes->m_lifeTime = lifeTime;
	pRes->m_ppThis = &pIRes;
	pRes->m_iActivationSource = iSource;

	pRes->m_pStatObj = this;
	pRes->m_pRopes = new IPhysicalEntity*[m_nSpines];
	for(i=0,pRes->m_pRopesActiveTime = new float[m_nSpines]; i<m_nSpines; i++)
		pRes->m_pRopesActiveTime[i] = -1.0f;
	pRes->m_nRopes = m_nSpines;

	pe_params_pos pp;
	pe_params_rope pr,pra;
	pe_action_target_vtx atv;
	pe_params_foreign_data pfd;
	pe_params_timeout pto;
	pe_params_outer_entity poe;

	sp.minEnergy = sqr(0.1f);
	if (poe.pOuterEntity=pTrunk)
		pTrunk->SetParams(&sp);

	pp.pos = mtxWorld.GetTranslation();
	pr.collDist = 0.03f; // TODO 
	pr.bTargetPoseActive = 2;
	pr.stiffnessAnim = GetCVars()->e_FoliageBranchesStiffness;
	pr.friction = 2.0f;
	pr.flagsCollider = geom_colltype_foliage;
	pr.windVariance = 0.3f;
	pr.airResistance = GetCVars()->e_FoliageWindActivationDist>0 ? 1.0f:0.0f;
	pr.noCollDist = 0.15f;
	j = -(iSource&1^1);
	pf.flagsOR = rope_collides&j|rope_target_vtx_rel0|rope_ignore_attachments|pef_traceable;
	if (pTrunk && pTrunk->GetType()!=PE_STATIC)
	{
		sp.maxTimeStep = 0.02f;
		pf.flagsOR |= rope_collides_with_terrain&j;
		pr.dampingAnim = GetCVars()->e_FoliageBrokenBranchesDamping;
	} else 
	{
		sp.maxTimeStep = 0.05f;
		if (GetCVars()->e_PhysFoliage<3)
			pf.flagsOR |= pef_ignore_areas, sp.gravity.zero();
		pr.dampingAnim = GetCVars()->e_FoliageBranchesDamping;
	}
	sp.minEnergy=sqr(0.01f-0.09f*j);
	pfd.iForeignData = PHYS_FOREIGN_ID_FOLIAGE;
	pfd.pForeignData = pRes;
	pto.maxTimeIdle = GetCVars()->e_FoliageBranchesTimeout;
	AABB bbox(AABB::RESET);
	for(i=0; i<m_nSpines; i++) if (m_pSpines[i].bActive)
		for(j=0;j<m_pSpines[i].nVtx;j++)
			bbox.Add(m_pSpines[i].pVtx[j]);
	bbox.Expand(bbox.GetSize()*0.1f);
	pr.collisionBBox[0] = bbox.min;
	pr.collisionBBox[1] = bbox.max;

	for(i=0; i<m_nSpines; i++) if (m_pSpines[i].bActive)
	{
		pRes->m_pRopes[i] = GetPhysicalWorld()->CreatePhysicalEntity(PE_ROPE,&pp);
		pr.pPoints = m_pSpines[i].pVtxCur;
		pr.nSegments = (atv.nPoints=m_pSpines[i].nVtx)-1;
		for(j=0; j<m_pSpines[i].nVtx; j++)
			pr.pPoints[j] = mtxWorld*m_pSpines[i].pVtx[j];
		pr.surface_idx = m_pSpines[i].idmat; 
		pr.collDist = 0.03f+0.07f*isneg(3.0f-m_pSpines[i].len);
		pfd.iForeignFlags = i;
		pRes->m_pRopes[i]->SetParams(&pr);
		pRes->m_pRopes[i]->SetParams(&pf);
		pRes->m_pRopes[i]->SetParams(&sp);
		pRes->m_pRopes[i]->SetParams(&pto);
		pRes->m_pRopes[i]->SetParams(&poe);

		if (m_pSpines[i].iAttachSpine<0)
			atv.points = m_pSpines[i].pVtx;
		else {
			int ivtx = m_pSpines[i].iAttachSeg;
			Vec3 pos0 = m_pSpines[j=m_pSpines[i].iAttachSpine].pVtx[ivtx];
			Quat q0 = Quat::CreateRotationV0V1(Vec3(0,0,-1), 
				mtxWorld.TransformVector((m_pSpines[j].pVtx[ivtx+1]-m_pSpines[j].pVtx[ivtx])).normalized()); 
			atv.points = m_pSpines[i].pVtxCur;
			for(j=0; j<m_pSpines[i].nVtx; j++)
				atv.points[j] = mtxWorld.TransformVector(m_pSpines[i].pVtx[j]-pos0)*q0;
		}
		pRes->m_pRopes[i]->Action(&atv);
		pRes->m_pRopes[i]->SetParams(&pfd,1);
	}	else
		pRes->m_pRopes[i] = 0;

	for(i=0; i<m_nSpines; i++) if (pRes->m_pRopes[i])
	{
		if (m_pSpines[i].iAttachSpine>=0)
		{
			pra.pEntTiedTo[0] = pRes->m_pRopes[m_pSpines[i].iAttachSpine];
			pra.idPartTiedTo[0] = m_pSpines[i].iAttachSeg;
		}	else
		{
			pra.pEntTiedTo[0] = pTrunk;
			MARK_UNUSED pra.idPartTiedTo[0];
		}
		pRes->m_pRopes[i]->SetParams(&pra,-1);
	}

	// ultra-gross hAx0R
	if (!GetSystem()->IsDedicated() && m_pRenderMesh && m_pRenderMesh->GetChunksSkinned() && m_pRenderMesh->GetAllocatedBytes(false)==0xbaad)
	{
		CryWarning(VALIDATOR_MODULE_3DENGINE,VALIDATOR_ERROR, "Warning: rebuilding rendermesh (%s)", GetFilePath());
		IRenderMesh *pBadMesh = m_pRenderMesh;
		m_pRenderMesh = GetRenderer()->CreateRenderMesh("StatObj_Skinned", GetFilePath());
		m_pRenderMesh->SetMaterial(m_pMaterial);
		pBadMesh->CopyTo(m_pRenderMesh);
		GetRenderer()->DeleteRenderMesh(pBadMesh);
	}

	if (!GetSystem()->IsDedicated() && m_pRenderMesh && !m_pRenderMesh->GetChunksSkinned())
	{
		int nBones,isubs; 
		Vec3 *pBonePos;
		std::vector<Vec3> pVtxOffs;	pVtxOffs.resize(m_pRenderMesh->GetVerticesCount());

		//m_pRenderMeshSkinned = GetRenderer()->CreateRenderMesh(eRMT_Static,"StatObj_Skinned",GetFilePath());
		//m_pRenderMeshSkinned->SetMaterial(m_pMaterial);
		//m_pRenderMesh->CopyTo(m_pRenderMeshSkinned, true);
		//IIndexedMesh *pIdxMesh = m_pRenderMesh->GetIndexedMesh();
		//m_pRenderMeshSkinned->SetMesh(*pIdxMesh->GetMesh());
		//pIdxMesh->Release();
    m_pRenderMesh->CreateChunksSkinned();

		PodArray<CRenderChunk> &meshChunks = *m_pRenderMesh->GetChunksSkinned();
		CMesh *pMesh = m_pIndexedMesh ? m_pIndexedMesh->GetMesh() : 0;
		for(i=j=isubs=0; i<(int)meshChunks.size(); i++,isubs++)
		{
			if (pMesh) for(;pMesh->m_subsets[isubs].nNumIndices==0;isubs++)
				if ((pMesh->m_subsets[isubs].nMatFlags & (MTL_FLAG_NOPHYSICALIZE|MTL_FLAG_NODRAW)) == MTL_FLAG_NOPHYSICALIZE)
					for(j++; j<m_chunkBoneIds.size() && m_chunkBoneIds[j]!=0; j++);
			if ((meshChunks[i].m_nMatFlags & (MTL_FLAG_NOPHYSICALIZE|MTL_FLAG_NODRAW)) == MTL_FLAG_NOPHYSICALIZE)	
			{
				meshChunks[i].m_arrChunkBoneIDs.clear();
				meshChunks[i].m_arrChunkBoneIDs.push_back(0);
				for(j++; j<m_chunkBoneIds.size() && m_chunkBoneIds[j]!=0; j++)
					meshChunks[i].m_arrChunkBoneIDs.push_back(m_chunkBoneIds[j]);
			}
		}

		for(i=0,nBones=1;i<m_nSpines;i++)
			nBones += m_pSpines[i].nVtx-1;
		(pBonePos = new Vec3[nBones])[0].zero();
		for(i=0,nBones=1;i<m_nSpines;i++) for(j=0;j<m_pSpines[i].nVtx-1;j++,nBones++)
			pBonePos[nBones] = m_pSpines[i].pVtx[j];
		for(i=0;i<(int)meshChunks.size();i++) 
			if (meshChunks[i].m_arrChunkBoneIDs.size()>0)
				for(j=meshChunks[i].nFirstVertId;j<meshChunks[i].nFirstVertId+meshChunks[i].nNumVerts;j++)
					pVtxOffs[j] = pBonePos[meshChunks[i].m_arrChunkBoneIDs[m_pBoneMapping[j].boneIDs[0]]];
			else for(j=meshChunks[i].nFirstVertId;j<meshChunks[i].nFirstVertId+meshChunks[i].nNumVerts;j++)
				pVtxOffs[j] = pBonePos[0], 
				m_pBoneMapping[j].boneIDs[0]=m_pBoneMapping[j].boneIDs[1]=0;
		delete[] pBonePos;



		if (0)
		{
			//--------------------------------------------------------------------------------------
			//--- this code is checking if all bone-mappings in the subsets are in a valid range ---
			//--------------------------------------------------------------------------------------
			uint32 numSubsets = meshChunks.size();
			for (uint32 s=0; s<numSubsets; s++)
			{
				uint32 nStartIndex		= meshChunks[s].nFirstVertId;
				uint32 nNumVertices		= meshChunks[s].nNumVerts;
				uint32 BonesPerSubset = meshChunks[s].m_arrChunkBoneIDs.size();

				//	assert(BonesPerSubset);
				if (BonesPerSubset)
				{
					#undef NUM_MAX_BONES_PER_GROUP
					#define NUM_MAX_BONES_PER_GROUP 100.f
					assert(BonesPerSubset<=NUM_MAX_BONES_PER_GROUP); //if this is bigger then the max-value, then we get a GPU crash
				//	if (BonesPerSubset>NUM_MAX_BONES_PER_GROUP)
				//		CryFatalError("Fatal Error: BonesPerSubset exeeds maximum of bones: %d (contact: Anton@Crytek.de or Ivo@Crytek.de)",BonesPerSubset );
					#undef NUM_MAX_BONES_PER_GROUP
					for (i=nStartIndex; i<(int)(nStartIndex+nNumVertices); i++)
					{
						uint32 v = i;
						// get bone-ids
						uint16 b0 = m_pBoneMapping[v].boneIDs[0];
						uint16 b1 = m_pBoneMapping[v].boneIDs[1];
						uint16 b2 = m_pBoneMapping[v].boneIDs[2];
						uint16 b3 = m_pBoneMapping[v].boneIDs[3];
						// get weights
						f32 w0 = m_pBoneMapping[v].weights[0];
						f32 w1 = m_pBoneMapping[v].weights[1];
						f32 w2 = m_pBoneMapping[v].weights[2];
						f32 w3 = m_pBoneMapping[v].weights[3];
						// if weight is zero set bone ID to zero as the bone has no influence anyway,
						if (w0 == 0) b0 = 0;
						if (w1 == 0) b1 = 0;
						if (w2 == 0) b2 = 0;
						if (w3 == 0) b3 = 0;											
						assert( b0 < BonesPerSubset );  
						assert( b1 < BonesPerSubset );
						assert( b2 < BonesPerSubset );
						assert( b3 < BonesPerSubset );

						/*
						if (b0 >= BonesPerSubset)
							CryFatalError("Fatal Error: index out of range: Index: %d   BonesPerSubset: %d (contact: Anton@Crytek.de or Ivo@Crytek.de)",b0,BonesPerSubset );
						if (b1 >= BonesPerSubset)
							CryFatalError("Fatal Error: index out of range: Index: %d   BonesPerSubset: %d (contact: Anton@Crytek.de or Ivo@Crytek.de)",b1,BonesPerSubset );
						if (b2 >= BonesPerSubset)
							CryFatalError("Fatal Error: index out of range: Index: %d   BonesPerSubset: %d (contact: Anton@Crytek.de or Ivo@Crytek.de)",b2,BonesPerSubset );
						if (b3 >= BonesPerSubset)
							CryFatalError("Fatal Error: index out of range: Index: %d   BonesPerSubset: %d (contact: Anton@Crytek.de or Ivo@Crytek.de)",b3,BonesPerSubset );
							*/
					}
				}
			}
		}

		m_pRenderMesh->SetSkinningDataVegetation(m_pBoneMapping);

		/*if (GetFlags() & STATIC_OBJECT_CLONE) 
		{
			if (m_pRenderMesh)
				GetRenderer()->DeleteRenderMesh(m_pRenderMesh);
			m_pRenderMesh = m_pRenderMeshSkinned;
		}*/
	}

	pIRes = pRes;
	return m_nSpines;
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// CStatObjFoliage /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

CStatObjFoliage::~CStatObjFoliage() 
{ 
	if (m_pRopes) 
	{
		for(int i=0;i<m_nRopes;i++)	if (m_pRopes[i])
			gEnv->pPhysicalWorld->DestroyPhysicalEntity(m_pRopes[i]);
		delete[] m_pRopes; 
		delete[] m_pRopesActiveTime;
	}
	if (m_pSkinQuats)
		delete[] m_pSkinQuats;
  if (m_pPrevSkinQuats)
    delete[] m_pPrevSkinQuats;
	m_next->m_prev = m_prev;
	m_prev->m_next = m_next;
	if (m_pStatObj)
    --m_pStatObj->m_nPhysFoliages;
	if (m_pStatObj)
		m_pStatObj->Release();
	if (!m_bDelete) 
	{
		if (*m_ppThis==this)
			*m_ppThis = 0;
		if(m_pVegInst)
			((CVegetation*)m_pVegInst)->UpdateFoliageState();
	}
}

uint32 CStatObjFoliage::GetSkeletonPose(int nLod, int nList, const Matrix34& RenderMat34, QuatTS*& pBoneQuatsL, QuatTS*& pBoneQuatsS, QuatTS*& pMBBoneQuatsL, QuatTS*& pMBBoneQuatsS, Vec4 shapeDeformationData[], uint32 &DoWeNeedMorphtargets, uint8*& pRemapTable )
{
	pRemapTable = 0;

	int i,j,ibone;
	pe_status_rope sr;
	Vec3 pt0,pt1;
	Quat q;
	Matrix34 mtx = RenderMat34.GetInverted();
  int bLinearSkinning = Cry3DEngineBase::GetCVars()->e_VegetationSphericalSkinning ^ 1;
  
	if (!m_pSkinQuats)
	{
		for(i=0,ibone=1; i<m_pStatObj->m_nSpines; ibone += m_pStatObj->m_pSpines[i++].nVtx-1);
		m_pSkinQuats = new QuatTS[ibone];
    m_pPrevSkinQuats = new QuatTS[ibone];
	}

  // Opt: This is called now just once per-frame (since function can be called tons of times from renderer for each rendering pass)
  int nCurrentFrameID = Cry3DEngineBase::m_pRenderer->GetFrameID();
  if(m_nFrameID != nCurrentFrameID)    
  {
    m_pPrevSkinQuats[0] = m_pSkinQuats[0]; 
	 // m_pSkinQuats[0].SetIdentity();

		m_pSkinQuats[0].q.SetIdentity();;
		m_pSkinQuats[0].t.x = 0;
		m_pSkinQuats[0].t.y = 0;
		m_pSkinQuats[0].t.z = 0;
		m_pSkinQuats[0].s		= 0;

		for(i=0,ibone=1; i<m_pStatObj->m_nSpines; i++) 
		  if (m_pRopes[i])
		  {
				sr.pPoints = 0;
				m_pRopes[i]->GetStatus(&sr);
				if (sr.timeLastActive > m_pRopesActiveTime[i]) 
				{
					sr.pPoints = m_pStatObj->m_pSpines[i].pVtxCur;
					m_pRopes[i]->GetStatus(&sr);
					for(j=0,pt0=mtx*sr.pPoints[0]; j<m_pStatObj->m_pSpines[i].nVtx-1; j++,ibone++,pt0=pt1)
					{
						pt1 = mtx*sr.pPoints[j+1];
						q = Quat::CreateRotationV0V1((m_pStatObj->m_pSpines[i].pVtx[j+1]-m_pStatObj->m_pSpines[i].pVtx[j]).normalized(), (pt1-pt0).normalized());
						m_pPrevSkinQuats[ibone] = m_pSkinQuats[ibone];
						
						QuatT qt = QuatT(q, pt0-q*m_pStatObj->m_pSpines[i].pVtx[j]);
						QuatD qd = QuatD(qt);
						m_pSkinQuats[ibone].q		= qd.nq;
						m_pSkinQuats[ibone].t.x = qd.dq.v.x;
						m_pSkinQuats[ibone].t.y = qd.dq.v.y;
						m_pSkinQuats[ibone].t.z = qd.dq.v.z;
						m_pSkinQuats[ibone].s		= qd.dq.w;
					}
					m_pRopesActiveTime[i] = sr.timeLastActive;
				}	else
					ibone += m_pStatObj->m_pSpines[i].nVtx-1;
		  } 
			else
			{ 
				for(j=0; j<m_pStatObj->m_pSpines[i].nVtx-1; j++,ibone++)
				{
					//m_pSkinQuats[ibone].SetIdentity(); 
					//m_pSkinQuats[ibone].s = 0.001f;

					m_pSkinQuats[ibone].q.SetIdentity();;
					m_pSkinQuats[ibone].t.x = 0;
					m_pSkinQuats[ibone].t.y = 0;
					m_pSkinQuats[ibone].t.z = 0;
					m_pSkinQuats[ibone].s		= 0;

					m_pPrevSkinQuats[ibone] = m_pSkinQuats[ibone];
				}
			}
    m_nFrameID=nCurrentFrameID;  
  }

	pBoneQuatsL = pBoneQuatsS = m_pSkinQuats;
  pMBBoneQuatsL = pMBBoneQuatsS = m_pPrevSkinQuats;
  
	DoWeNeedMorphtargets = 0;

	for(i=0,ibone=1; i<m_pStatObj->m_nSpines; i++) 
		ibone += m_pStatObj->m_pSpines[i].nVtx-1;
	return ibone;
}

void CStatObjFoliage::Update(float dt)
{
	if (!*m_ppThis && !m_bDelete)
		m_bDelete = 2;
	if (m_bDelete) {
		if (--m_bDelete<=0)
			delete this;
		return;
	}
  FUNCTION_PROFILER_3DENGINE;

	int i,j,nContactEnts=0,nColl;
	//int nContacts[4]={0,0,0,0};
	pe_status_rope sr;
	pe_params_bbox pbb;
	pe_status_dynamics sd;
	bool bHasBrokenRopes=false;
	IPhysicalEntity *pContactEnt, *pContactEnts[4];
//	IPhysicalEntity *pMultiContactEnt=0;
	AABB bbox; bbox.Reset();

	for(i=nColl=0;i<m_pStatObj->m_nSpines;i++) if (m_pRopes[i] && m_pRopes[i]->GetStatus(&sr)) 
	{
		pContactEnt = sr.pContactEnts[0];
		if (m_pVegInst && pContactEnt && pContactEnt->GetType()!=PE_STATIC)
		{
			for(j=0;j<nContactEnts && pContactEnts[j]!=pContactEnt;j++);
			if (j<nContactEnts)
			{
//				if (++nContacts[j]*2>=m_pStatObj->m_nSpines)
//					pMultiContactEnt = pContactEnt;
			}	else if (nContactEnts<4)
				pContactEnts[nContactEnts++] = pContactEnt;
		}
		nColl += sr.nCollDyn;
		if (m_flags & FLAG_FROZEN && sr.nCollStat+sr.nCollDyn)
			BreakBranch(i), bHasBrokenRopes=true;
		else {
			m_pRopes[i]->GetParams(&pbb);
			bbox.Add(pbb.BBox[0]); bbox.Add(pbb.BBox[1]);
		}
	}	else
		bHasBrokenRopes = true;

	// disables destruction of non-frozen vegetation
	/*if (pMultiContactEnt && !(m_flags & FLAG_FROZEN) && pMultiContactEnt->GetStatus(&sd) && sd.mass>500 && (sd.v.len2()>25 || sd.w.len()>0.5))
	{	// destroy the vegetation object, spawn particle effect
		Vec3 sz = m_pVegInst->GetBBox().GetSize();
		IParticleEffect *pDisappearEffect = m_pStatObj->GetSurfaceBreakageEffect( SURFACE_BREAKAGE_TYPE("destroy") );
		if (pDisappearEffect)
			pDisappearEffect->Spawn(true, IParticleEffect::ParticleLoc(m_pVegInst->GetBBox().GetCenter(),Vec3(0,0,1),(sz.x+sz.y+sz.z)*0.3f));
		m_pStatObj->Get3DEngine()->m_pObjectsTree[nSID]->DeleteObject(m_pVegInst);
		m_pVegInst->Dephysicalize();
		return;
	}*/

	const CCamera &cam = m_pStatObj->GetCamera();
	float clipDist = GetCVars()->e_CullVegActivation;
	int bVisible = cam.IsAABBVisible_F(bbox);
	bVisible &= isneg(((cam.GetPosition()-bbox.GetCenter()).len2()-sqr(clipDist))*clipDist-0.0001f);
	if (!bVisible) 
	{
		if (inrange(m_timeInvisible+=dt,6.0f,8.0f))
			bVisible = 1;
	} else
		m_timeInvisible = 0;
	if (bVisible!=m_bWasVisible)
	{
		pe_params_flags pf;
		pf.flagsAND = ~pef_disabled;
		pf.flagsOR = pef_disabled & ~-bVisible;
		for(i=0;i<m_pStatObj->m_nSpines;i++) if (m_pRopes[i])
			m_pRopes[i]->SetParams(&pf);
		m_bWasVisible = bVisible;
	}

	if (nColl && (m_iActivationSource&2 || bVisible))
		m_timeIdle = 0;
	if (!bHasBrokenRopes && m_lifeTime>0 && (m_timeIdle+=dt)>m_lifeTime)
	{
		*m_ppThis = 0;
		if(m_pVegInst)
			((CVegetation*)m_pVegInst)->UpdateFoliageState();
		m_bDelete = 2;
	}
}

void CStatObjFoliage::SetFlags(int flags)
{
	if (flags!=m_flags)
	{
		pe_params_rope pr;
		if (flags & FLAG_FROZEN) 
		{
			pr.collDist = 0.1f;
			pr.stiffnessAnim = 0;
		} else
		{
			pr.collDist = 0.03f;
			pr.stiffnessAnim = m_pStatObj->Get3DEngine()->GetCVars()->e_FoliageBranchesStiffness;
		}
		for(int i=0;i<m_pStatObj->m_nSpines;i++) if (m_pRopes[i])
			m_pRopes[i]->SetParams(&pr);
		m_flags = flags;
	}
}

void CStatObjFoliage::OnHit(struct EventPhysCollision *pHit) 
{
	if (m_flags & FLAG_FROZEN)
	{
		pe_params_foreign_data pfd;
		pHit->pEntity[1]->GetParams(&pfd);
		BreakBranch(pfd.iForeignFlags);
	}
}

void CStatObjFoliage::BreakBranch(int idx) 
{
	if (m_pRopes[idx])
	{
		int i,nActiveRopes;

		IParticleEffect *pBranchBreakEffect = m_pStatObj->GetSurfaceBreakageEffect( SURFACE_BREAKAGE_TYPE("freeze_shatter") );
		if (pBranchBreakEffect)
		{
			float t,maxt,kb,kc,dim;
			float effStep = max(0.1f,m_pStatObj->m_pSpines[idx].len*0.1f);
			Vec3 pos,dir;
			pe_status_rope sr;

			sr.pPoints = m_pStatObj->m_pSpines[idx].pVtxCur;
			m_pRopes[idx]->GetStatus(&sr);
			pos=sr.pPoints[0]; i=0;	t=0;
			dir=sr.pPoints[1]-sr.pPoints[0]; dir/=(maxt=dir.len());
			do {
				pos = sr.pPoints[i]+dir*t;
				dim = max(fabs_tpl(m_pStatObj->m_pSpines[idx].pSegDim[i].y-m_pStatObj->m_pSpines[idx].pSegDim[i].x),
									fabs_tpl(m_pStatObj->m_pSpines[idx].pSegDim[i].w-m_pStatObj->m_pSpines[idx].pSegDim[i].z));
				dim = max(0.05f,min(1.5f,dim));
				if ((t+=effStep)>maxt)
				{
					if (++i>=sr.nSegments)
						break;
					dir = sr.pPoints[i+1]-sr.pPoints[i];
					dir /= (maxt=dir.len());
					kb = dir*(sr.pPoints[i]-pos); kc=(sr.pPoints[i]-pos).len2()-sqr(effStep);
					pos = sr.pPoints[i]+dir*(t=kb+sqrt_tpl(max(1e-5f,kb*kb-kc)));
				}
				pBranchBreakEffect->Spawn(true, IParticleEffect::ParticleLoc(pos,(Vec3(0,0,1)-dir*dir.z).GetNormalized(),dim));
			}	while(true);
		}

		if (!m_bGeomRemoved)
		{
			for(i=nActiveRopes=0;i<m_pStatObj->m_nSpines;i++)
				nActiveRopes += m_pRopes[i]!=0;
			if (nActiveRopes*3<m_pStatObj->m_nSpines*2)
			{
				pe_params_rope pr;
				pe_params_part pp;
				m_pRopes[idx]->GetParams(&pr);
				pp.flagsCond=geom_squashy; pp.flagsAND=0; pp.flagsColliderAND=0; pp.mass=0;
				if (pr.pEntTiedTo[0])
					pr.pEntTiedTo[0]->SetParams(&pp);
				m_bGeomRemoved = 1;
			}
		}
		m_pStatObj->GetPhysicalWorld()->DestroyPhysicalEntity(m_pRopes[idx]);
		m_pRopes[idx] = 0;
		if (m_pStatObj->GetFlags() & (STATIC_OBJECT_CLONE|STATIC_OBJECT_CLONE))
			m_pStatObj->m_pSpines[idx].bActive = false;

		for(i=0;i<m_pStatObj->m_nSpines;i++) if (m_pStatObj->m_pSpines[i].iAttachSpine==idx)
			BreakBranch(i);
	}
}

int CStatObjFoliage::Serialize(TSerialize ser)
{
	int i,nRopes=m_nRopes;
	bool bVal;
	ser.Value("nropes", nRopes);

	if (!ser.IsReading()) 
	{
		for(i=0;i<m_nRopes;i++)
		{
			ser.BeginGroup("rope");
			if (m_pRopes[i])
			{
				ser.Value("active", bVal=true);
				m_pRopes[i]->GetStateSnapshot(ser);
			}	else
				ser.Value("active", bVal=false);
			ser.EndGroup();
		}
	} else 	
	{
		if (m_nRopes!=nRopes)
			return 0;
		for(i=0;i<m_nRopes;i++) 
		{
			ser.BeginGroup("rope");
			ser.Value("active", bVal);
			if (m_pRopes[i] && !bVal)
				m_pStatObj->GetPhysicalWorld()->DestroyPhysicalEntity(m_pRopes[i]), m_pRopes[i]=0;
			else if (m_pRopes[i])
				m_pRopes[i]->SetStateFromSnapshot(ser);
			m_pRopesActiveTime[i] = -1;
			ser.EndGroup();
		}
	}

	return 1;
}
