#include "stdafx.h"
#include "StaticObjectCompiler.h"
#include "StatCGFPhysicalize.h"
//#include "StatCGFSHCompiler.h"
#include "MeshCompiler\MeshCompiler.h"
#include "CGF\CGFNodeMerger.h"

#define MAX_VALID_OBJECT_VOLUME (10000000000.f)

CStaticObjectCompiler::CStaticObjectCompiler(CPhysicsInterface *pPhysicsInterface)
	: m_pPhysicsInterface(pPhysicsInterface)
{
	m_bSplitLODs = false;

	for (size_t i = 0; i < MAX_NUM_LODS; i++)
	{
		m_pLODs[i] = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
CStaticObjectCompiler::~CStaticObjectCompiler()
{
	for (size_t i = 0; i < MAX_NUM_LODS; i++)
	{
		delete m_pLODs[i];
	}
}


//////////////////////////////////////////////////////////////////////////
CContentCGF* CStaticObjectCompiler::MakeCompiledCGF( CContentCGF *pCGF )
{
	if (pCGF->GetExportInfo()->bCompiledCGF)
	{
		return ProcessCompiledCGF( pCGF );
	}

	MakeLOD( 0,pCGF );

	CContentCGF *pCompiledCGF = m_pLODs[0];

	// Setup Mesh subsets for the original CGF.
	for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);
		if (pNode->pMesh)
		{
			CGFNodeMerger::SetupMeshSubsets( pCGF,*pNode->pMesh,pNode->pMaterial );
		}
	}

	if (pCGF->GetExportInfo()->bMergeAllNodes)
	{
		MakeMergedCGF( pCompiledCGF,pCGF );
	}
	else
	{
		// Compile meshes in nodes. 

		//////////////////////////////////////////////////////////////////////////
		for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
		{
			pCGF->GetNode(i)->bHasFaceMap = false;
		}
		for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
		{
			CNodeCGF *pNodeCGF = pCGF->GetNode(i);
			int len;
			if (pNodeCGF->pMesh && 
				!pNodeCGF->bPhysicsProxy && 
				pNodeCGF->type==CNodeCGF::NODE_MESH && 
				(len=strlen(pNodeCGF->name)) > 10 &&
				!strcmp((const char*)pNodeCGF->name+len-10,"_Destroyed"))
			{
				for (int j = 0; j < num; ++j)
				{
					CNodeCGF *pNodeCGF1;
					if (i!=j && !strncmp((pNodeCGF1=pCGF->GetNode(j))->name,pNodeCGF->name,len-10) && pNodeCGF1->name[len-10]==0)
					{
						pNodeCGF1->bHasFaceMap = pNodeCGF->bHasFaceMap = true;
					}
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////

		for (int i = 0, num = pCGF->GetNodeCount(); i < num; ++i)
		{
			CNodeCGF *pNodeCGF = pCGF->GetNode(i);
			pCompiledCGF->AddNode(pNodeCGF);
		}
	}

	// Compile meshes in all nodes.
	if (!CompileMeshes( pCompiledCGF ))
	{
		return 0;
	}

	// Try to find shared meshes.
	AnalyzeSharedMeshes( pCompiledCGF );

	const bool ok = Physicalize( pCompiledCGF,pCGF );
	if (!ok)
	{
		return 0;
	}

	{
		int i;
		for(i=0; i<pCompiledCGF->GetNodeCount() && pCompiledCGF->GetNode(i)->type!=CNodeCGF::NODE_MESH; i++);
		if (i<pCompiledCGF->GetNodeCount())
		{
			AnalyzeFoliage( pCompiledCGF, pCompiledCGF->GetNode(i) );
		}

		for(i=0; i<pCompiledCGF->GetNodeCount(); i++) if (!strncmp(pCompiledCGF->GetNode(i)->name,"skeleton_",9))
			for(int j=0; j<pCompiledCGF->GetNodeCount()-1; j++) if (!strcmp(pCompiledCGF->GetNode(i)->name+9, pCompiledCGF->GetNode(j)->name))
			{
				float r = 0.0f;
				CNodeCGF *pSkelNode=pCompiledCGF->GetNode(i), *pMeshNode=pCompiledCGF->GetNode(j);
				if (const char *ptr = strstr(pSkelNode->properties,"skin_dist"))
				{
					for(;*ptr && (*ptr<'0' || *ptr>'9');ptr++);
					if (*ptr)
						r = (float)atof(ptr);
				}
				PrepareSkinData(pMeshNode, pMeshNode->localTM.GetInverted()*pSkelNode->localTM, pSkelNode, r, false);
			}
	}

	if ( !ValidateBoundingBoxes( pCompiledCGF ) )
	{
		return 0;
	}

	// Try to split LODs
	if (m_bSplitLODs)
	{
		SplitLODs(pCompiledCGF);
	}

	return pCompiledCGF;
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::CompileMeshes( CContentCGF *pCGF )
{
	//////////////////////////////////////////////////////////////////////////
	// Compile Meshes in all nodes.
	//////////////////////////////////////////////////////////////////////////
	for (int i=0; i<pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNodeCGF = pCGF->GetNode(i);

		if (pNodeCGF->pMesh)
		{
			mesh_compiler::CMeshCompiler meshCompiler;
			if (pNodeCGF->bHasFaceMap)
			{
				meshCompiler.SetFaceRemapping(&pNodeCGF->mapFaceToFace0);
			}

			int nMeshCompileFlags = mesh_compiler::MESH_COMPILE_TANGENTS; 
			if (!pNodeCGF->bPhysicsProxy)
			{
				nMeshCompileFlags |= mesh_compiler::MESH_COMPILE_OPTIMIZE;
			}

			if (!meshCompiler.Compile( *pNodeCGF->pMesh, nMeshCompileFlags))
			{
				RCLogError( "Failed to compile geometry in node '%s' in file %s - %s",pNodeCGF->name,pCGF->GetFilename(),meshCompiler.GetLastError() );
				return false;
			}
			//now check whether any sh coefficients are required and if so, compute them, check all materials/submaterials for this cgf
			//if(!CalculateSHCoefficientsForMesh(pNodeCGF->pMesh, pCGF, pNodeCGF->pMaterial))
			//	LogError( "Failed to process spherical harmonic coefficients for geometry in node '%s' in file %s",pNodeCGF->name.c_str(),pCGF->GetFilename() );
		}
	}
	//////////////////////////////////////////////////////////////////////////
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::AnalyzeSharedMeshes( CContentCGF *pCGF )
{
	//////////////////////////////////////////////////////////////////////////
	// Check if any duplicate meshes exist, and try to share them.
	mesh_compiler::CMeshCompiler meshCompiler;
	uint32 numNodes = pCGF->GetNodeCount();
	for (int i = 0; i < numNodes-1; i++)
	{
		CNodeCGF *pNode1 = pCGF->GetNode(i);
		if (!pNode1->pMesh || pNode1->pSharedMesh)
			continue;

		if (!(pNode1->pMesh->m_numVertices && pNode1->pMesh->m_numFaces))
			continue;

		if (pNode1->bPhysicsProxy)
			continue;

		for (int j = i+1; j < numNodes; j++)
		{
			CNodeCGF *pNode2 = pCGF->GetNode(j);
			if (pNode1 == pNode2 || !pNode2->pMesh)
				continue;

			if (pNode2->bPhysicsProxy)
				continue;

			if (pNode1->properties == pNode2->properties)
			{
				if (meshCompiler.CompareMeshes( *pNode1->pMesh,*pNode2->pMesh ) && pNode1->pMesh != pNode2->pMesh)
				{
					// Meshes are same, share them.
					delete pNode2->pMesh;
					pNode2->pMesh = pNode1->pMesh;
					pNode2->pSharedMesh = pNode1;
				}
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::ValidateBoundingBoxes( CContentCGF *pCGF )
{
	bool found = false;
	for(int i=0; i<pCGF->GetNodeCount(); i++)
	{
		CNodeCGF * pNode = pCGF->GetNode(i);
		if ( pNode->type == CNodeCGF::NODE_MESH && pNode->pMesh->GetVertexCount() == 0 && pNode->pMesh->GetIndexCount() == 0 )
		{
			float fobjradius = pNode->pMesh->m_bbox.GetRadius();
			if(fobjradius > MAX_VALID_OBJECT_VOLUME || !_finite(fobjradius) || fobjradius<=0 )
			{
				RCLogWarning("Node '%s' in file %s has an invalid bounding box, the engine will fail to load this object. Check that the node has valid geometry and is not empty.",pNode->name,pCGF->GetFilename());		
				found = true;
			}
		}
	}

	if ( found )
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::Physicalize( CContentCGF *pCompilerCGF,CContentCGF *pSrcCGF )
{
	for (int i=0; i<pCompilerCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNode = pCompilerCGF->GetNode(i);
		if (!pNode->pMesh)
			continue;

		m_pPhysicsInterface->Physicalize( pNode,pCompilerCGF );
	}

	for (int i=0; i<pCompilerCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNode = pCompilerCGF->GetNode(i);
		if (pNode->pMesh && !pNode->bPhysicsProxy)
		{
			const bool ok = m_pPhysicsInterface->DeletePhysicalProxySubsets( pNode, pNode->bHasFaceMap );
			if (!ok)
			{
				RCLogError( "Failed to physicalize geometry in file %s",pSrcCGF->GetFilename());
				return false;
			}
		}
	}

	m_pPhysicsInterface->ProcessBreakablePhysics( pCompilerCGF,pSrcCGF );
	return true;
}

//////////////////////////////////////////////////////////////////////////
//const bool CStaticObjectCompiler::CalculateSHCoefficientsForMesh(CMesh *pMesh, CContentCGF *pCGF, const CMaterialCGF* cpMat)
//{
//	CStatCFGSHCompiler shCompiler(pMesh, pCGF, cpMat);
//	return shCompiler.Calculate();
//}


//////////////////////////////////////////////////////////////////////////

inline int check_mask(unsigned int *pMask, int i) {
	return pMask[i>>5]>>(i&31) & 1;
}
inline void set_mask(unsigned int *pMask, int i) {
	pMask[i>>5] |= 1u<<(i&31);
}
inline void clear_mask(unsigned int *pMask, int i) {
	pMask[i>>5] &= ~(1u<<(i&31));
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


void CStaticObjectCompiler::AnalyzeFoliage(CContentCGF *pCGF, CNodeCGF *pNodeCGF)
{
	if (!pCGF || !pNodeCGF)
		return;
	int i,j,isle,iss,ivtx0,nVtx,itri,ivtx,iedge,nSegs,ibran0,ibran1,ibran,nBranches,ispine,ispineDst,nBones,nGlobalBones,idmat=0, 
			*pIdxSorted,*pForeignIdx;
	int nMaxBones = 72;
	Vec3 pt[7],edge,edge1,n,nprev,axisx,axisy;
	Vec2 tex[6];
	float t,s,mindist,k,denom,len,dist[3],e,t0,t1,denomPrev;
	unsigned int *pUsedTris,*pUsedVtx;
	char sname[64];
	SFoliageInfoCGF &fi = *pCGF->GetFoliageInfo();
	CMesh &mesh = *pNodeCGF->pMesh;
	mesh_data *pmd;
	IGeometry *pPhysGeom;
	SBranch branch,*pBranches;
	primitives::box bbox;

	int nMeshVtx,nMeshIdx;
	Vec3 *pMeshVtx;
	SMeshTexCoord *pMeshTex;
	unsigned short *pMeshIdx;
	nMeshVtx = mesh.GetVertexCount(); pMeshVtx = mesh.m_pPositions;
	nMeshIdx = mesh.GetIndexCount();  pMeshIdx = mesh.m_pIndices; 
	pMeshTex = mesh.m_pTexCoord;

	// iterate through branch points branch#_#, build a branch structure
	pBranches=0; nBranches=0;
	do {
		branch.pt=0; branch.npt=0; branch.len=0;
		do {
			sprintf(sname,"branch%d_%d",nBranches+1,branch.npt+1);
			for(i=pCGF->GetNodeCount()-1; i>=0 && stricmp((const char*)pCGF->GetNode(i)->name,sname); i--); 
			if (i<0)
				break;
			pt[0].Set(pCGF->GetNode(i)->localTM.m03, pCGF->GetNode(i)->localTM.m13, pCGF->GetNode(i)->localTM.m23);
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
	SSpineRC spine;
	memset(&spine, 0, sizeof spine); 
	memset(fi.pBoneMapping = new SMeshBoneMapping[fi.nSkinnedVtx=nMeshVtx], 0, nMeshVtx*sizeof(SMeshBoneMapping));
	for(i=0; i<nMeshVtx; i++)
		fi.pBoneMapping[i].weights[0] = 255;
	pIdxSorted = new int[nMeshVtx];	nVtx = 0;
	pForeignIdx = new int[nMeshIdx/3];
	pUsedTris = new unsigned int[i = ((nMeshIdx-1)>>5)+1]; memset(pUsedTris,0,i*sizeof(int));
	pUsedVtx = new unsigned int[i = ((nMeshVtx-1)>>5)+1];	memset(pUsedVtx,0,i*sizeof(int));

	// iterate through all triangles, track the closest for each branch point
	for(iss=0; iss<mesh.GetSubSetCount(); iss++) 
	if (mesh.m_subsets[iss].nPhysicalizeType == PHYS_GEOM_TYPE_NONE)
		for(i=mesh.m_subsets[iss].nFirstIndexId; i<mesh.m_subsets[iss].nFirstIndexId+mesh.m_subsets[iss].nNumIndices; i+=3)
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
				s += pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].s*k;
				t += pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].t*k;
			}
		} else {
			for(j=0;j<3 && !inrange((pt[incm3(j)]-pt[j])*(pt[3]-pt[j]), 0.0f,(pt[incm3(j)]-pt[j]).len2());j++);
			if (j<3) 
			{ // the closest pt in on an edge
				k = ((pt[3]-pt[j])*(pt[incm3(j)]-pt[j]))/(pt[incm3(j)]-pt[j]).len2();
				s = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].s*(1-k)+
					  pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+incm3(j)]].s*k;
				t = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].t*(1-k)+
						pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+incm3(j)]].t*k;
			} else 
			{	// the closest pt is a vertex
				for(j=0;j<3;j++) dist[j] = (pt[j]-pt[3]).len2();
				j = idxmin3(dist);
				s = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].s;
				t = pMeshTex[pMeshIdx[pBranches[ibran].pt[i].itri+j]].t;
			}
		}
		pBranches[ibran].pt[i].tex.set(s,t);
	}

	for(iss=0,nGlobalBones=1; iss<mesh.GetSubSetCount(); iss++,nGlobalBones+=nBones-1) 
	if (mesh.m_subsets[iss].nPhysicalizeType == PHYS_GEOM_TYPE_NONE)
	{
		// fill the foreign idx list
		for(i=0,j=mesh.m_subsets[iss].nFirstIndexId/3; j*3<mesh.m_subsets[iss].nFirstIndexId+mesh.m_subsets[iss].nNumIndices;	j++,i++)
			pForeignIdx[i] = j;
		pPhysGeom = m_pPhysicsInterface->GetPhysicalWorld()->GetGeomManager()->CreateMesh(pMeshVtx, pMeshIdx+mesh.m_subsets[iss].nFirstIndexId, 
			0,pForeignIdx, mesh.m_subsets[iss].nNumIndices/3, mesh_shared_vtx|mesh_SingleBB|mesh_keep_vtxmap);
		pmd = (mesh_data*)pPhysGeom->GetData();
		pPhysGeom->GetBBox(&bbox);
		e = sqr((bbox.size.x+bbox.size.y+bbox.size.z)*0.002f);
		nBones = 1;
		//mesh.m_subsets[iss].m_arrChunkBoneIDs.push_back(0);
		fi.chunkBoneIds.push_back(0);

		for(isle=0; isle<pmd->nIslands; isle++)	if (pmd->pIslands[isle].nTris>=4)
		{
			ibran0=0; ibran1=nBranches; ivtx=0; nSegs=0; 
			spine.nVtx=0; spine.pVtx=0; spine.iAttachSpine=spine.iAttachSeg = -1;
			do {
				for(itri=pmd->pIslands[isle].itri,i=j=0; i<pmd->pIslands[isle].nTris; itri=pmd->pTri2Island[itri].inext,i++)
				{
					for(j=0;j<3;j++) tex[j].set(pMeshTex[pMeshIdx[pmd->pForeignIdx[itri]*3+j]].s, 
																			pMeshTex[pMeshIdx[pmd->pForeignIdx[itri]*3+j]].t);
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
			spine.pSegDim = new Vec4[spine.nVtx];

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
				n += nprev*float(sgnnz(nprev.z));
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
					fi.pBoneMapping[pIdxSorted[i]].boneIDs[0] = nBones+min(spine.nVtx-2,ivtx+j);
					fi.pBoneMapping[pIdxSorted[i]].boneIDs[1] = nBones+max(0,ivtx-1+j);
					fi.pBoneMapping[pIdxSorted[i]].weights[j^1] = 255-(fi.pBoneMapping[pIdxSorted[i]].weights[j] = max(1,min(254,FtoI(t*255))));
					j=pIdxSorted[ivtx0]; pIdxSorted[ivtx0++]=pIdxSorted[i]; pIdxSorted[i]=j;
				}
				nprev = n;
				//mesh.m_subsets[iss].m_arrChunkBoneIDs.push_back(nGlobalBones+nBones-1+ivtx);
				fi.chunkBoneIds.push_back(nGlobalBones+nBones-1+ivtx);
			}
			for(i=ivtx0;i<nVtx;i++) {
				fi.pBoneMapping[pIdxSorted[i]].boneIDs[0] = nBones+spine.nVtx-2;
				fi.pBoneMapping[pIdxSorted[i]].weights[0] = 255;
			}

			if ((fi.nSpines & 7)==0) {
				SSpineRC *pSpines = fi.pSpines;
				memcpy(fi.pSpines = new SSpineRC[fi.nSpines+8], pSpines, fi.nSpines*sizeof(SSpineRC));
				for(j=0;j<fi.nSpines;j++) 
					pSpines[j].pVtx=0, pSpines[j].pSegDim=0;
				if (pSpines) delete[] pSpines;
			}
			fi.pSpines[fi.nSpines++] = spine;
			nBones += spine.nVtx-1;
		}
		pPhysGeom->Release();
	} else
		nBones = 1;

	// for every spine, check if its base is close enough to a point on another spine 
	for(ispine=0; ispine<fi.nSpines; ispine++)
		for(ispineDst=0,pt[0]=fi.pSpines[ispine].pVtx[0]; ispineDst<fi.nSpines; ispineDst++) if (ispine!=ispineDst)
		{
			mindist = sqr(fi.pSpines[ispine].len*0.05f); 
			for(i=0,j=-1; i<fi.pSpines[ispineDst].nVtx-1; i++) 
			{	// find the point on this seg closest to pt[0]
				if ((t=(fi.pSpines[ispineDst].pVtx[i+1]-pt[0]).len2())<mindist)
					mindist=t, j=i;	// end vertex
				edge = fi.pSpines[ispineDst].pVtx[i+1]-fi.pSpines[ispineDst].pVtx[i];
				edge1 = pt[0]-fi.pSpines[ispineDst].pVtx[i];
				if (inrange(edge1*edge, edge.len2()*-0.3f*((i-1)>>31),edge.len2()) && (t=(edge^edge1).len2()) < mindist*edge.len2())
					mindist=t/edge.len2(), j=i;	// point on edge
			}
			if (j>=0)
			{	// attach ispine to a point on iSpineDst
				fi.pSpines[ispine].iAttachSpine = ispineDst;
				fi.pSpines[ispine].iAttachSeg = j;
				break;
			}
		}

	delete[] pUsedVtx; delete[] pUsedTris; delete[] pForeignIdx; delete[] pIdxSorted;
	spine.pVtx = 0; spine.pSegDim=0;
}

//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::PrepareSkinData(CNodeCGF *pNode, const Matrix34 &mtxSkelToMesh, CNodeCGF *pNodeSkel, float r, bool bSwapEndian)
{
	int i,j,nVtx;
	Vec3 vtx[4];
	geom_world_data gwd[2];
	geom_contact *pcontact;
	Vec3 *pVtx = pNode->pMesh->m_pPositions;
	Matrix34 mtxMeshToSkel = mtxSkelToMesh.GetInverted();
	IGeomManager *pGeoman = m_pPhysicsInterface->GetPhysicalWorld()->GetGeomManager();
	CMemStream stm(&pNodeSkel->physicalGeomData[0][0], pNodeSkel->physicalGeomData[0].size(), bSwapEndian);
	phys_geometry *pSkelGeom = pNodeSkel->pMesh ? pGeoman->LoadPhysGeometry(stm, pNodeSkel->pMesh->m_pPositions,pNodeSkel->pMesh->m_pIndices,0) : 
														 pGeoman->LoadPhysGeometry(stm,0,0,0);
	IGeometry *pSphere,*pSphereBig,*pPhysSkel=pSkelGeom->pGeom;

	gwd[1].scale = mtxSkelToMesh.GetColumn0().len();
	gwd[1].offset = mtxSkelToMesh.GetTranslation();
	gwd[1].R = Matrix33(mtxSkelToMesh)*(1.0f/gwd[1].scale);
	vtx[0] = pNode->pMesh->m_bbox.GetSize();
	primitives::sphere sph; sph.center.zero(); sph.r=r>0.0f ? r:min(min(vtx[0].x,vtx[0].y),vtx[0].z);
	pSphere = pGeoman->CreatePrimitive(primitives::sphere::type, &sph);
	sph.r*=3.0f; pSphereBig = pGeoman->CreatePrimitive(primitives::sphere::type, &sph);
	mesh_data *md = (mesh_data*)pPhysSkel->GetData();
	CrySkinVtx *pSkinInfo = new CrySkinVtx[(nVtx=pNode->pMesh->GetVertexCount())+1];
	memset(pSkinInfo, 0, (nVtx+1)*sizeof(CrySkinVtx));
	pSkinInfo[nVtx].w[0] = r;
	pNode->pSkinInfo = pSkinInfo;

	for(i=0; i<nVtx; i++) 
	{
    Vec3 v = pVtx[i];
		gwd[0].offset = v;
		if (pSphere->Intersect(pPhysSkel, gwd,gwd+1, 0, pcontact) ||
				pSphereBig->Intersect(pPhysSkel, gwd,gwd+1, 0, pcontact))
		{
			for(j=0;j<3;j++) 
				vtx[j] = mtxSkelToMesh*md->pVertices[pSkinInfo[i].idx[j] = md->pIndices[pcontact->iPrim[1]*3+j]];
			Vec3 n = vtx[1]-vtx[0] ^ vtx[2]-vtx[0];
			float rnlen = 1.0f/n.len();	n *= rnlen;
			vtx[3] = v-n*(pSkinInfo[i].w[3]=n*(v-vtx[0]));
			Vec3 edge = (vtx[1]+vtx[2]-vtx[0]*2).normalized();
			pSkinInfo[i].M = Matrix33(edge,n^edge,n).T();
			for(j=0,n*=rnlen; j<3; j++)
				pSkinInfo[i].w[j] = (vtx[inc_mod3[j]]-vtx[3]^vtx[dec_mod3[j]]-vtx[3])*n;
		}	else
			pSkinInfo[i].idx[0] = -1;	
	}	
	pGeoman->UnregisterGeometry(pSkelGeom);
	pSphereBig->Release(); pSphere->Release();
}

//////////////////////////////////////////////////////////////////////////
CContentCGF* CStaticObjectCompiler::ProcessCompiledCGF( CContentCGF *pCGF )
{
	// When input CGF is already compiled we just need to perform some validation and re-compiling steps.
	int numNodes = pCGF->GetNodeCount();

	for(int i=0; i < numNodes; i++) 
	{
		CNodeCGF *pNodeCGF = pCGF->GetNode(i);
		m_pPhysicsInterface->RephysicalizeNode( pNodeCGF,pCGF );
	}

	// Try to split LODs
	if (m_bSplitLODs)
	{
		SplitLODs(pCGF);
	}

	return pCGF;
}

//////////////////////////////////////////////////////////////////////////
void CStaticObjectCompiler::SplitLODs( CContentCGF *pCGF )
{
	for (int i = 0; i < pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNodeCGF = pCGF->GetNode(i);
		string name = pNodeCGF->name;
		name.MakeLower();
		if (name.find("$lod") == 0 && name.length() > 4)
		{
			int nLodNum = name.at(4) - '0'; // $lod1 - $lod9
			if (nLodNum <= 0)
				continue;
			if (nLodNum >= MAX_NUM_LODS)
				continue;

			CNodeCGF *pParent = pNodeCGF->pParent;
			if (!pParent)
				continue;

			strcpy_s(pNodeCGF->name,pParent->name);

			CContentCGF *pLodCGF = m_pLODs[nLodNum];
			if (!pLodCGF)
			{
				pLodCGF = MakeLOD( nLodNum,pCGF );
			}
			pLodCGF->AddNode( pNodeCGF );
			pCGF->RemoveNode(pNodeCGF);
			i--;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CContentCGF* CStaticObjectCompiler::MakeLOD( int nLodNum,CContentCGF *pCGF )
{
	if (m_pLODs[nLodNum])
		return 0;

	string filename = pCGF->GetFilename();
	m_pLODs[nLodNum] = new CContentCGF( filename.c_str() );

	CContentCGF *pCompiledCGF = m_pLODs[nLodNum];
	*pCompiledCGF->GetExportInfo() = *pCGF->GetExportInfo(); // Copy export info.
	*pCompiledCGF->GetPhysiclizeInfo() = *pCGF->GetPhysiclizeInfo();
	pCompiledCGF->GetExportInfo()->bCompiledCGF = true;

	pCompiledCGF->GetUsedMaterialIDs() = pCGF->GetUsedMaterialIDs();
	
	if (nLodNum > 0 && m_pLODs[0])
	{
		m_pLODs[0]->GetExportInfo()->bHaveAutoLods = true;
	}

	return m_pLODs[nLodNum];
}

//////////////////////////////////////////////////////////////////////////
bool CStaticObjectCompiler::MakeMergedCGF( CContentCGF *pCompiledCGF,CContentCGF *pCGF )
{
	std::vector<CNodeCGF*> merge_nodes;
	for (int i = 0; i < pCGF->GetNodeCount(); i++)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);
		if (pNode->pMesh && !pNode->bPhysicsProxy && pNode->type == CNodeCGF::NODE_MESH)
			merge_nodes.push_back(pNode);
	}

	if (merge_nodes.empty())
	{
		RCLogError( "Error merging nodes, No mergable geometry in CGF %s",pCGF->GetFilename() );
		return false;
	}

	CMesh *pMergedMesh = new CMesh;
	string errorMessage;
	if (!CGFNodeMerger::MergeNodes(pCGF, merge_nodes,errorMessage,pMergedMesh))
	{
		RCLogError("Error merging nodes: %s, in CGF %s", errorMessage.c_str(),pCGF->GetFilename());
		delete pMergedMesh;
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Add single node for merged mesh.
	CNodeCGF *pNode = new CNodeCGF;
	pNode->type = CNodeCGF::NODE_MESH;
	strcpy_s(pNode->name,"Merged");
	pNode->bIdentityMatrix = true;
	pNode->pMesh = pMergedMesh;
	pNode->pMaterial = pCGF->GetCommonMaterial();

	CNodeCGF *pMergedNode = pNode;
	
	// Add node to CGF contents.
	pCompiledCGF->AddNode( pNode );
	//////////////////////////////////////////////////////////////////////////

	char sLodString[32];
	for (int nLod = 1; nLod < MAX_NUM_LODS; nLod++)
	{
		sprintf_s( sLodString,"$lod%d",nLod );

		// Try to merge LOD nodes.
		merge_nodes.clear();
		for (int i = 0; i < pCGF->GetNodeCount(); i++)
		{
			CNodeCGF *pNode = pCGF->GetNode(i);
			if (pNode->pMesh && !pNode->bPhysicsProxy && pNode->type == CNodeCGF::NODE_HELPER)
			{
				string nodeName = pNode->name;
				nodeName.MakeLower();
				if (strncmp(sLodString,nodeName.c_str(),5) == 0) // 5 characters ex: $lod2
				{
					// This is a LOD helper.
					merge_nodes.push_back(pNode);
				}
			}
		}
		if (!merge_nodes.empty())
		{
			CMesh *pMergedLodMesh = new CMesh;
			string errorMessage;
			if (!CGFNodeMerger::MergeNodes(pCGF, merge_nodes,errorMessage,pMergedLodMesh))
			{
				RCLogError("Error merging LOD %d nodes: %s, in CGF %s",nLod,errorMessage.c_str(),pCGF->GetFilename());
				delete pMergedLodMesh;
				return false;
			}

			//////////////////////////////////////////////////////////////////////////
			// Add LOD node, ex: $lod1_Merged
			//////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////////////////////////
			// Add single node for merged mesh.
			CNodeCGF *pNode = new CNodeCGF;
			pNode->type = CNodeCGF::NODE_HELPER;
			pNode->helperType = HP_GEOMETRY;
			strcpy_s(pNode->name,(string(sLodString) + "_" + pMergedNode->name).c_str());
			pNode->bIdentityMatrix = true;
			pNode->pMesh = pMergedLodMesh;
			pNode->pParent = pMergedNode;
			pNode->pMaterial = pCGF->GetCommonMaterial();

			// Add node to CGF contents.
			pCompiledCGF->AddNode( pNode );
			//////////////////////////////////////////////////////////////////////////
		}
	}

	// Add rest of the helper nodes.
	int numNodes = pCGF->GetNodeCount();
	for (int i=0; i<numNodes; i++)
	{
		CNodeCGF *pNode = pCGF->GetNode(i);

		if (pNode->type != CNodeCGF::NODE_MESH)
		{
			string nodeName = pNode->name;
			nodeName.MakeLower();
			// Do not add LOD nodes.
			if (strncmp(nodeName.c_str(),"$lod",4) != 0) // 4 characters ex: $lod
			{
				if (pNode->pParent && pNode->pParent->type == CNodeCGF::NODE_MESH)
				{
					pNode->pParent = pMergedNode;
				}
				pCompiledCGF->AddNode(pNode);
			}
		}
	}

	return true;
}
