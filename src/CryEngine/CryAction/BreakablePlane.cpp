/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2005.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 20:1:2005: Created by Anton Knyazyev

*************************************************************************/

#include "StdAfx.h"
#include "CryAction.h"
#include "IGameRulesSystem.h"
#include "ActionGame.h"
#include "BreakablePlane.h"
#include "ParticleParams.h"


template<class T> struct triplet {
	triplet(T *ptr) { pdata = ptr; }
	void set(const T& v0, const T& v1, const T& v2) { pdata[0]=v0; pdata[1]=v1; pdata[2]=v2; }
	T *pdata;
};

inline Vec3 UnpVec4sf(const Vec4sf &v) { return Vec3(tPackB2F(v.x),tPackB2F(v.y),tPackB2F(v.z)); }
inline Vec4sf PakVec4sf(const Vec3 &v,int w) { return Vec4sf(tPackF2B(v.x),tPackF2B(v.y),tPackF2B(v.z),w); }

int CBreakablePlane::g_nPieces = 0;
float CBreakablePlane::g_maxPieceLifetime = 0;


bool CBreakablePlane::SetGeometry(IStatObj *pStatObj, IMaterial *pRenderMat, int bStatic, int seed)
{
	int i,j,nVtx=0,idir,i0,j0,i1,iup,iter0,iter1,ibest[2]={-1,-1};
	Vec3 n,pt,c,sz,axis;
	float mincos,maxcos,cosa,maxarea[2]={1E-15f,1E-15f};
	primitives::box bbox;
	phys_geometry *pPhysGeom;
	mesh_data *pPhysMesh;
	CMesh *pMesh;
	vector2df *pVtx=0;
	m_bStatic = bStatic;

	if ((pPhysGeom=pStatObj->GetPhysGeom()))
	{
		pPhysGeom->pGeom->GetBBox(&bbox);
		iup = idxmin3((float*)&bbox.size);
		if (min(bbox.size[inc_mod3[iup]],bbox.size[dec_mod3[iup]]) < bbox.size[iup]*4)
		{
			CryLog("[Breakable2d] : geometry is not thin enough, breaking denied ( %s %s)", pStatObj->GetFilePath(), pStatObj->GetGeoName());
			return false;
		}
		bbox.size[iup] = max(bbox.size[iup], m_cellSize*0.01f);
		axis = bbox.Basis.GetRow(iup); 
		if (iup!=2) 
		{
			n=bbox.Basis.GetRow(2); c=bbox.Basis.GetRow(iup^1);
			bbox.Basis.SetRow(2,axis); bbox.Basis.SetRow(iup,c); bbox.Basis.SetRow(iup^1,n);
		}
		m_R = bbox.Basis.T();
		Vec3 axisAbs(axis.abs());
		m_bAxisAligned = axis[idxmax3((float*)&axisAbs)]>0.995f;
		m_center = bbox.center;
		m_nudge = m_cellSize*0.075f;
		m_nCells.set((int)(bbox.size[inc_mod3[iup]]*2/m_cellSize+0.5f), (int)(bbox.size[dec_mod3[iup]]*2/m_cellSize+0.5f));
		m_nCells.x = max(1,m_nCells.x); m_nCells.y = max(1,m_nCells.y); 
		//if (m_nCells.x*m_nCells.y<4)
		//	return false;
		m_pGeom = 0; m_pSampleRay = 0;

		if (pPhysGeom->pGeom->GetType()==GEOM_TRIMESH)
		{
			pPhysMesh = (mesh_data*)pPhysGeom->pGeom->GetData();
			for(i=0; i<pPhysMesh->nTris; i++) if (fabs_tpl(pPhysMesh->pNormals[i]*axis)>0.95f)
			{
				for(j=0;j<3 && pPhysMesh->pTopology[i].ibuddy[j]>=0 && 
						fabs_tpl(pPhysMesh->pNormals[pPhysMesh->pTopology[i].ibuddy[j]]*pPhysMesh->pNormals[i])>0.97f; j++);
				if (j<3) break;
			}
			i0=i;	
			if (i<pPhysMesh->nTris)	
			{
				mincos = (float)sgnnz(pPhysMesh->pNormals[i]*axis);
				m_R.SetRow(iup, m_R.GetRow(iup)*mincos);
				m_R.SetRow(inc_mod3[iup], m_R.GetRow(inc_mod3[iup])*mincos);
				axis *= mincos;
				mincos=maxcos = pPhysMesh->pNormals[i]*axis;
				j0=j; iter1=0; do {
					if (!(nVtx & 31)) pVtx = (vector2df*)realloc(pVtx, (nVtx+32)*sizeof(pVtx[0]));
					pVtx[nVtx++] = Vec2((pPhysMesh->pVertices[pPhysMesh->pIndices[i*3+j]]-m_center)*m_R);
					for(iter0=0; iter0<pPhysMesh->nTris; iter0++) {
						j = inc_mod3[j];
						if ((i1=pPhysMesh->pTopology[i].ibuddy[j])<0 || pPhysMesh->pNormals[i1]*pPhysMesh->pNormals[i]<0.97f)
							break;
						for(j=0; j<2 && pPhysMesh->pTopology[i1].ibuddy[j]!=i; j++);
						i = i1;
						mincos=min(mincos, cosa=pPhysMesh->pNormals[i]*axis); maxcos=max(maxcos,cosa);
					}
				}	while(++iter1<=pPhysMesh->nTris*3 && (i!=i0 || j!=j0));
				if (iter1>pPhysMesh->nTris*3) {
					delete[] pVtx; return false;
				}
			}	else
				return false;

			if (mincos<0.5f || (cosa = sqrt_tpl(1.0f-min(0.999f,sqr(mincos)))*maxcos - sqrt_tpl(1.0f-min(0.999f,sqr(maxcos))) * mincos)>0.7f) {
				delete[] pVtx; return false;

			}
			if (cosa > 0.03f)
			{
				(m_pGeom = pPhysGeom->pGeom)->PrepareForRayTest(bbox.size[iup]*2.3f);
				primitives::ray aray; aray.origin.zero(); aray.dir = axis;
				m_pSampleRay = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::ray::type, &aray);
			}
		} else
		{
			pVtx = new vector2df[4]; nVtx = 4;
			for(i=0;i<4;i++) pVtx[i].set(bbox.size[inc_mod3[iup]]*(((i^i*2)&2)-1), bbox.size[dec_mod3[iup]]*((i&2)-1));
		}
		if (!pStatObj->GetIndexedMesh(true)) {
			delete[] pVtx; return false;
		}
		
		pMesh = pStatObj->GetIndexedMesh(true)->GetMesh();
		m_thicknessOrg = bbox.size[iup];
		bbox.size[iup] = max(bbox.size[iup], 0.005f);
		m_z[0] = -bbox.size[iup]; m_z[1] = bbox.size[iup];
		for(i=0; i<pMesh->m_nIndexCount; i+=3)
		{
			n = pMesh->m_pPositions[pMesh->m_pIndices[i+1]]-pMesh->m_pPositions[pMesh->m_pIndices[i]] ^
					pMesh->m_pPositions[pMesh->m_pIndices[i+2]]-pMesh->m_pPositions[pMesh->m_pIndices[i]];
			for(idir=0;idir<2;idir++)
				if (sqr_signed(n*axis)*(idir*2-1)>n.len2()*sqr(0.95f) && n.len2()>maxarea[idir])
					maxarea[idir]=n.len2(), ibest[idir]=i;
		}
		if (ibest[0]<0 && ibest[1]<0)
			ibest[0] = ibest[1] = 0;
		m_bOneSided = 0;

		for(idir=0;idir<2;idir++)	
		{
			if (ibest[idir]<0)
				ibest[idir] = ibest[idir^1], m_bOneSided = 1;
			for(i=0;i<3;i++) 	
			{
				m_ptRef[idir][i] = Vec2((pMesh->m_pPositions[pMesh->m_pIndices[ibest[idir]+i]]-m_center)*m_R);
				m_texRef[idir][i] = pMesh->m_pTexCoord[pMesh->m_pIndices[ibest[idir]+i]];
				m_TangentRef[idir][i] = pMesh->m_pTangents[pMesh->m_pIndices[ibest[idir]+i]];
			}
			m_refArea[idir] = (idir*2-1)/sqrt_tpl(maxarea[idir]);
		}
		if (m_pGeom) for(idir=0;idir<2;idir++)	
		{
			n = pMesh->m_pPositions[pMesh->m_pIndices[ibest[idir]+1]]-pMesh->m_pPositions[pMesh->m_pIndices[ibest[idir]]] ^
							pMesh->m_pPositions[pMesh->m_pIndices[ibest[idir]+2]]-pMesh->m_pPositions[pMesh->m_pIndices[ibest[idir]]];
			Matrix33 R = Matrix33::CreateRotationV0V1(n.GetNormalized(), axis*(float)(idir*2-1));
			for(i=0;i<3;i++) 	
			{
				m_TangentRef[idir][i].Tangent = PakVec4sf(R*UnpVec4sf(m_TangentRef[idir][i].Tangent), m_TangentRef[idir][i].Tangent.w);
				m_TangentRef[idir][i].Binormal = PakVec4sf(R*UnpVec4sf(m_TangentRef[idir][i].Binormal), m_TangentRef[idir][i].Binormal.w);
			}
		}
		m_pMat = pStatObj->GetMaterial();
		if (pMesh->m_subsets.size()>0) 
		{
			for(i=0; i<pMesh->m_subsets.size()-1 && pMesh->m_subsets[i].nNumIndices==0; i++);
			m_matSubindex = pMesh->m_subsets[i].nMatID;
			m_matFlags = pMesh->m_subsets[i].nMatFlags;
			IMaterial *pSubMtl,*pSubMtl1;
			int len; const char *name,*name1;
			if (pRenderMat)
				if ((pSubMtl=pRenderMat->GetSubMtl(m_matSubindex+1)) && strstr(pSubMtl->GetName(), "broken"))
					m_matSubindex++;
				else if ((pSubMtl=pRenderMat->GetSubMtl(m_matSubindex)) && (len=strlen(name=pSubMtl->GetName())))
				{
					for(i=pRenderMat->GetSubMtlCount()-1; i>=0; i--) 
						if ((pSubMtl1=pRenderMat->GetSubMtl(i)) && !strncmp(name1=pSubMtl1->GetName(),name,len) && !strcmp(name1+len,"_broken"))
						{
							m_matSubindex=i; break;
						}
					if (i<0 && *m_mtlSubstName)
					{
						for(i=pRenderMat->GetSubMtlCount()-1; i>=0; i--) 
							if ((pSubMtl1=pRenderMat->GetSubMtl(i)) && !strcmp(pSubMtl1->GetName(),m_mtlSubstName))
								break;
						if (i<0)
						{
							if (IMaterial *pMatSubst = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_mtlSubstName,false))
							{
								m_matSubindex = pRenderMat->GetSubMtlCount();
								pRenderMat->SetSubMtlCount(m_matSubindex+1);
								pRenderMat->SetSubMtl(m_matSubindex, pMatSubst);
							}
						}	else
							m_matSubindex = i;
					}
				}
		}
		IGeomManager *pGeoman = gEnv->pPhysicalWorld->GetGeomManager();
		m_pGrid = pGeoman->GenerateBreakableGrid(pVtx,nVtx, m_nCells, m_bStatic, seed);
		delete[] pVtx;
		pStatObj->FreeIndexedMesh();
		//pPhysGeom->pGeom->SetForeignData(this,1);
	}

	return true;
}


void CBreakablePlane::FillVertexData(CMesh *pMesh,int ivtx, const vector2df &pos, int iside)
{
	int i,ncont;
	float k;
	Quat qnRot; qnRot.SetIdentity();
	SMeshTexCoord tex;
	Vec3 Tangent(ZERO),Binormal(ZERO);
	tex.s = tex.t = 0;

	pMesh->m_pPositions[ivtx] = m_R*Vec3(pos.x,pos.y,m_z[iside])+m_center;
	pMesh->m_pNorms[ivtx] = m_R*Vec3(0,0,iside*2-1.f);
	if (m_pGeom)
	{
		geom_world_data gwd[2];
		geom_contact *pcont;
		gwd[1].offset = m_R*Vec3(pos.x*0.99f,pos.y*0.99f,m_z[iside]*1.01f)+m_center;
		i=1-iside*2; gwd[1].R(0,0)*=i; gwd[1].R(1,1)*=i; gwd[1].R(2,2)*=i;
		if (ncont = m_pGeom->Intersect(m_pSampleRay,gwd,gwd+1,0,pcont)) 
		{
			pMesh->m_pPositions[ivtx] += m_R.GetColumn(2)*(m_R.GetColumn(2)*(pcont[ncont-1].pt-pMesh->m_pPositions[ivtx]));
			qnRot = Quat::CreateRotationV0V1(pMesh->m_pNorms[ivtx], pcont[ncont-1].n);
			pMesh->m_pNorms[ivtx] = pcont[ncont-1].n;
		}
	}
	for(i=0;i<3;i++) 
	{
		k = (m_ptRef[iside][inc_mod3[i]]-pos ^ m_ptRef[iside][dec_mod3[i]]-pos)*m_refArea[iside];
		tex.s += m_texRef[iside][i].s*k; tex.t += m_texRef[iside][i].t*k;
		Tangent += UnpVec4sf(m_TangentRef[iside][i].Tangent)*k;
		Binormal += UnpVec4sf(m_TangentRef[iside][i].Binormal)*k;
	}
	Tangent.NormalizeSafe(); (Binormal-=Tangent*(Tangent*Binormal)).NormalizeSafe();
	pMesh->m_pTexCoord[ivtx] = tex;
	pMesh->m_pTangents[ivtx].Tangent = PakVec4sf(qnRot*Tangent, m_TangentRef[iside][0].Tangent.w);
	pMesh->m_pTangents[ivtx].Binormal = PakVec4sf(qnRot*Binormal, m_TangentRef[iside][0].Binormal.w);
}


IStatObj *CBreakablePlane::CreateFlatStatObj(int *&pIdx, vector2df *pt, vector2df *bounds, const Matrix34 &mtxWorld, 
																						 IParticleEffect *pEffect, bool bNoPhys, bool bUseEdgeAlpha)
{
	int i,j,nVtx,nCntVtx,nTris,itri,itriCnt,nCntVtxTri,bValid,clrmask;
	int *pVtxMap,*pCntVtxMap,*pCntVtxList;
	Vec3 n,Tangent,(*pVtxEdge)[2],direff,axis=m_R*Vec3(0,0,1);
	vector2df pos;
	float nudge,k;
	SMeshTexCoord tex;
	IStatObj *pStatObj;
	IIndexedMesh *pIdxMesh;
	CMesh *pMesh;
	SMeshColor *pColor,dummyClr;

	if (*pIdx<0)
		return 0;
	pStatObj = gEnv->p3DEngine->CreateStatObj();
	//pStatObj->AddRef();
	pMesh = (pIdxMesh=pStatObj->GetIndexedMesh())->GetMesh();
	pStatObj->SetMaterial(m_pMat);

	nVtx = (m_nCells.x+2)*(m_nCells.y+2);
	memset(pVtxMap = new int[nVtx], -1,sizeof(pVtxMap[0])*nVtx);
	memset(pCntVtxMap = new int[nVtx], -1,sizeof(pVtxMap[0])*nVtx);
	pVtxEdge = (Vec3(*)[2])(new Vec3[nVtx*2]);

	for(i=nVtx=nTris=nCntVtx=nCntVtxTri=0; pIdx[i]>=0; i+=4,nTris++) for(j=0;j<3;j++)
	{
		nVtx += pVtxMap[pIdx[i+j]]>>1 & 1; pVtxMap[pIdx[i+j]] &= ~2;
		nCntVtx += pVtxMap[pIdx[i+j]] & (pIdx[i+3]>>j^1) & 1;
		pVtxMap[pIdx[i+j]] &= pIdx[i+3]>>j & 1 | ~1;
		nCntVtxTri += (pIdx[i+3]>>j^1) & 1;
	}
	nudge = pIdx[i]==-1 ? m_nudge:0;
	int b2Sided = pIdx[i]==-1 || !m_bOneSided;

	int nOutVtx = nVtx+(nVtx+nCntVtx*4)*b2Sided;
	pIdxMesh->SetVertexCount(nOutVtx);
	pIdxMesh->SetTexCoordsAndTangentsCount(nOutVtx);
	pIdxMesh->SetIndexCount(nTris*3+(nTris*3+nCntVtxTri*6)*b2Sided);
	if (bUseEdgeAlpha) {
		pIdxMesh->SetColorsCount(nOutVtx);
		memset(pMesh->m_pColor0, 255, nOutVtx*sizeof(pMesh->m_pColor0[0]));
		pColor = pMesh->m_pColor0; clrmask = -1;
	} else
	{
		pColor = &dummyClr; clrmask = 0;
	}
	pCntVtxList = new int[nCntVtx];
	nVtx = nCntVtx = nTris = 0;
	direff = mtxWorld.TransformVector(m_R*Vec3(0,0,1));
	bounds[0].set(1E10,1E10);
	bounds[1].set(-1E10,-1E10);

	for(i=0;pIdx[i]>=0;i+=4)
	{
		for(j=0;j<3;j++) 
		{
			if (pVtxMap[pIdx[i+j]]<0)
			{
				FillVertexData(pMesh,nVtx, vector2df(pt[pIdx[i+j]].x,pt[pIdx[i+j]].y), 1);
				bounds[0].x = min(bounds[0].x,pt[pIdx[i+j]].x);	bounds[0].y = min(bounds[0].y,pt[pIdx[i+j]].y);
				bounds[1].x = max(bounds[1].x,pt[pIdx[i+j]].x);	bounds[1].y = max(bounds[1].y,pt[pIdx[i+j]].y);
				pVtxMap[pIdx[i+j]] = nVtx++;
			}
			pMesh->m_pIndices[nTris*3+j] = pVtxMap[pIdx[i+j]];
			pColor[pVtxMap[pIdx[i+j]] & clrmask].a &= -(pIdx[i+3]>>(j+6) & 1);
		}
		for(j=0;j<3;j++) if (!(pIdx[i+3] & 1<<j))
		{
			// offset vtx inward along the edge normal
			n = Vec3(pt[pIdx[i+inc_mod3[j]]].x-pt[pIdx[i+j]].x, pt[pIdx[i+inc_mod3[j]]].y-pt[pIdx[i+j]].y, 0).normalized();
			pVtxEdge[pIdx[i+j]][1] = pVtxEdge[pIdx[i+inc_mod3[j]]][0] = m_R*n;

			if (!(pIdx[i+3] & 8<<j))
			{
				n = m_R*Vec3(-n.y,n.x,0);
				pMesh->m_pPositions[pVtxMap[pIdx[i+j]]] += n*nudge;
				pMesh->m_pPositions[pVtxMap[pIdx[i+inc_mod3[j]]]] += n*nudge;
			}

			if (pCntVtxMap[pIdx[i+j]]<0)
			{
				pCntVtxMap[pIdx[i+j]] = nCntVtx;
				pCntVtxList[nCntVtx++] = pIdx[i+j];
			}
		}
		nTris++;
	}

	if (b2Sided)
	{
		for(i=0;i<nVtx;i++)	
		{ // add the bottom vertices
			FillVertexData(pMesh,nVtx+i, vector2df((pMesh->m_pPositions[i]-m_center)*m_R), 0);
			pColor[nVtx+i & clrmask] = pColor[i & clrmask];
		}

		for(i=0,n=-axis; i<nCntVtx*2; i++)
		{	// add the side vertices (split copies of top and bottom contours)
			pMesh->m_pPositions[nVtx*2+i] = pMesh->m_pPositions[j = pVtxMap[pCntVtxList[i>>1]]];
			pMesh->m_pTexCoord[nVtx*2+i] = pMesh->m_pTexCoord[j];
			Tangent = pVtxEdge[pCntVtxList[i>>1]][i&1];
			pMesh->m_pTangents[(nVtx+nCntVtx)*2+i].Tangent  = pMesh->m_pTangents[nVtx*2+i].Tangent  = PakVec4sf(Tangent,-32768);
			pMesh->m_pTangents[(nVtx+nCntVtx)*2+i].Binormal = pMesh->m_pTangents[nVtx*2+i].Binormal = PakVec4sf(n,-32768);
			pMesh->m_pNorms[(nVtx+nCntVtx)*2+i] = pMesh->m_pNorms[nVtx*2+i] = Tangent^n;
			pColor[(nVtx+nCntVtx)*2+i & clrmask] = pColor[nVtx*2+i & clrmask] = pColor[j & clrmask];
		}
		for(i=0; i<nCntVtx*2; i++) 
		{
			pMesh->m_pPositions[(nVtx+nCntVtx)*2+i] = pMesh->m_pPositions[(j = pVtxMap[pCntVtxList[i>>1]])+nVtx];
			pos = vector2df((pMesh->m_pPositions[j]-m_center)*m_R) + vector2df(pVtxEdge[pCntVtxList[i>>1]][i&1]*m_R).rot90cw()*(m_z[1]-m_z[0]);
			for(j=0,tex.s=tex.t=0; j<3; j++) 
			{
				k = (m_ptRef[1][inc_mod3[j]]-pos ^ m_ptRef[1][dec_mod3[j]]-pos)*m_refArea[1];
				tex.s += m_texRef[1][j].s*k; tex.t += m_texRef[1][j].t*k;
			}
			pMesh->m_pTexCoord[(nVtx+nCntVtx)*2+i] = tex;//pVtxMap[pCntVtxList[i>>1]]];
		}

		for(i=0,bValid=1,itri=nTris,itriCnt=nTris*2; pIdx[i]>=0; i+=4,itri++) 
		{
			for(j=0;j<3;j++)
				pMesh->m_pIndices[itri*3+2-j] = pVtxMap[pIdx[i+j]]+nVtx;
			for(j=0;j<3;j++) if (!(pIdx[i+3] & 1<<j))
			{
				triplet<uint16>(pMesh->m_pIndices+itriCnt++*3).set(pVtxMap[pIdx[i+inc_mod3[j]]], pVtxMap[pIdx[i+j]], pVtxMap[pIdx[i+j]]+nVtx);
				triplet<uint16>(pMesh->m_pIndices+itriCnt++*3).set(pVtxMap[pIdx[i+j]]+nVtx, pVtxMap[pIdx[i+inc_mod3[j]]]+nVtx, pVtxMap[pIdx[i+inc_mod3[j]]]);
			}
		}
	}

	for(i=0,bValid=1; pIdx[i]>=0; i+=4) 
	{
		n = pMesh->m_pPositions[pVtxMap[pIdx[i+1]]]-pMesh->m_pPositions[pVtxMap[pIdx[i]]] ^ 
				pMesh->m_pPositions[pVtxMap[pIdx[i+2]]]-pMesh->m_pPositions[pVtxMap[pIdx[i]]];
		bValid &= isnonneg(n*axis);
	}
	if (pEffect && pIdx[nTris*4]!=-2)
		for(i=0;pIdx[i]>=0;i+=4) for(j=0;j<3;j++) if (!(pIdx[i+3] & 9<<j))
			pEffect->Spawn( false, IParticleEffect::ParticleLoc( 
				mtxWorld*(pMesh->m_pPositions[pVtxMap[pIdx[i+j]]]*(2.0f/3)+pMesh->m_pPositions[pVtxMap[pIdx[i+inc_mod3[j]]]]*(1.0f/3)),
				direff) );

	pIdx += nTris*4;

	if (*pIdx!=-2 && !bValid)
	{
		pStatObj->Release();
		delete[] pCntVtxList; delete[] pVtxEdge;
		delete[] pCntVtxMap;  delete[] (Vec3*)pVtxMap;
		return 0;
	}

	pIdxMesh->SetSubSetCount(1);
	SMeshSubset &mss = pIdxMesh->GetSubSet(0);
	mss.nMatID = m_matSubindex;
	mss.nMatFlags = m_matFlags;
	mss.nFirstVertId = 0;
	mss.nNumVerts = pMesh->m_numVertices;
	mss.nFirstIndexId = 0;
	mss.nNumIndices = pMesh->m_nIndexCount;
	mss.vCenter = m_R*Vec3(bounds[0].x+bounds[1].x,bounds[0].y+bounds[1].y,0)*0.5f + m_center;
	mss.fRadius = sqrt_tpl(sqr(bounds[1].x-bounds[0].x)+sqr(bounds[1].y-bounds[0].y))*0.5f;

	if (bNoPhys && *pIdx!=-2)
	{
		Vec3 center = m_R*Vec3(bounds[0].x+bounds[1].x,bounds[0].y+bounds[1].y,0)*0.5f + m_center;
		for(i=0;i<pMesh->m_numVertices;i++)
			pMesh->m_pPositions[i] -= center;
	}

	IGeomManager *pGeoman = gEnv->pPhysicalWorld->GetGeomManager();
	IGeometry *pGeom = 0;
	if (*pIdx==-2 && m_bStatic && m_bAxisAligned) 
	{
		SVoxGridParams params;
		primitives::grid *pgrid = m_pGrid->GetGridData();
		params.origin = m_R*Vec3(pgrid->origin.x,pgrid->origin.y,m_z[0]-(m_z[1]-m_z[0])*0.01f)+m_center;
		n = (m_R*Vec3((float)pgrid->size.x,(float)pgrid->size.y,2)).abs();
		params.size.Set(max(1,(int)(n.x+0.5f)), max(1,(int)(n.y+0.5f)), max(1,(int)(n.z+0.5f)));
		params.step = m_R*Vec3(pgrid->step.x,pgrid->step.y,(m_z[1]-m_z[0])*0.505f);
		params.origin.x += params.size.x*min(0.0f,params.step.x);
		params.origin.y += params.size.y*min(0.0f,params.step.y);
		params.origin.z += params.size.z*min(0.0f,params.step.z);
		params.step = params.step.abs();
		pGeom = pGeoman->CreateMesh(pMesh->m_pPositions,pMesh->m_pIndices,0,0,pMesh->m_nIndexCount/3, mesh_VoxelGrid|mesh_no_vtx_merge, 0,&params);
	} 
	else if (*pIdx==-2 || !bNoPhys)
		pGeom = pGeoman->CreateMesh(pMesh->m_pPositions,pMesh->m_pIndices,0,0,pMesh->m_nIndexCount/3, mesh_OBB|mesh_no_vtx_merge|mesh_multicontact0, 0,1,2);

	if (*pIdx==-2 || bNoPhys || pGeom->GetErrorCount()==0)
	{
		if (b2Sided)
		{
			if (!m_pGeom && fabs_tpl(m_z[1]-m_z[0]-m_thicknessOrg*2)>m_thicknessOrg*0.01f)
			{
				Vec3 zax = m_R.GetColumn(2);
				for(i=0; i<pMesh->m_numVertices; i++)
				{
					float zorg = (pMesh->m_pPositions[i]-m_center)*zax;
					float znew = m_thicknessOrg*sgnnz(zorg);
					pMesh->m_pPositions[i] += zax*(znew-zorg);
				}
			}
			pIdx -= nTris*4;
			for(i=0,itri=nTris,itriCnt=nTris*2; pIdx[i]>=0; i+=4,itri++) for(j=0;j<3;j++) if (!(pIdx[i+3] & 1<<j))
			{
				triplet<uint16>(pMesh->m_pIndices+itriCnt++*3).set((pCntVtxMap[pIdx[i+inc_mod3[j]]]+nVtx)*2, (pCntVtxMap[pIdx[i+j]]+nVtx)*2+1, 
					(pCntVtxMap[pIdx[i+j]]+nVtx+nCntVtx)*2+1);
				triplet<uint16>(pMesh->m_pIndices+itriCnt++*3).set((pCntVtxMap[pIdx[i+j]]+nVtx+nCntVtx)*2+1, (pCntVtxMap[pIdx[i+inc_mod3[j]]]+nVtx+nCntVtx)*2, 
					(pCntVtxMap[pIdx[i+inc_mod3[j]]]+nVtx)*2);
			}
			pIdx += nTris*4;
		}
		pStatObj->SetFlags(STATIC_OBJECT_GENERATED);
		if (pGeom)
		{
			if (*pIdx==-2)
				pGeom->SetForeignData(this,1);
			pStatObj->SetPhysGeom(pGeoman->RegisterGeometry(pGeom, m_matId));
			pGeom->Release();
		}
		pStatObj->Invalidate();
	}
	else
	{
		pStatObj->Release();
		pStatObj = 0;
	}
	delete[] pCntVtxList; delete[] pVtxEdge;
	delete[] pCntVtxMap;  delete[] (Vec3*)pVtxMap;
	
	return pStatObj;
}


int *CBreakablePlane::Break(const Vec3 &pthit, float r, vector2df *&ptout, int seed, float filterAng,float ry)
{
	return m_pGrid->BreakIntoChunks(vector2df((pthit-m_center)*m_R),r,ptout, m_maxPatchTris,m_jointhresh, seed, filterAng,ry);
}

float circ_segment_area(float r,float h) 
{
	float a=0.0f, b=1.0f;
	if (h<0.0f)
		h=-h, a=r*r*gf_PI, b=-1.0f;
	h = min(r*0.9999f, h);
	return a+b* (r*r*acos_tpl(h/r) - h*sqrt_tpl(r*r-h*h));
}
float _boxCircleIntersArea(const Vec2 &sz, const Vec2 &c, float r)
{
	int i,sg;
	float h,a,b,area=gf_PI*r*r;
	Vec2 pt;
	for(i=0;i<2;i++) for(sg=-1;sg<=1;sg+=2)	if ((h=c[i]*sg+r-sz[i])>0)
		area -= circ_segment_area(r,r-h);
	for(sg=-1;sg<=1;sg+=2) for(i=-1;i<=1;i+=2) if ((pt=Vec2(sz.x*sg,sz.y*i)-c).GetLength2() < r*r) {
		a = sqrt_tpl(r*r-pt.y*pt.y)-pt.x*sg;
		b = sqrt_tpl(r*r-pt.x*pt.x)-pt.y*i;
		area += a*b*0.5f+circ_segment_area(r,sqrt_tpl(r*r-(a*a+b*b)*0.25f));
	}
	return area;
}

inline int qmax(int op1,int op2) { return op1 - (op1-op2 & (op1-op2)>>31); }
inline int qmin(int op1,int op2) { return op2 + (op1-op2 & (op1-op2)>>31); }
inline float qmin(float op1,float op2) { return (op1+op2-fabsf(op1-op2))*0.5f; }
inline float qmax(float op1,float op2) { return (op1+op2+fabsf(op1-op2))*0.5f; }

IStatObj *CBreakablePlane::ProcessImpact(const Vec3 &pthit, const Vec3 &hitvel,float hitmass,float &hitradius, IStatObj *pStatObj, const Matrix34 &mtx, 
																				 ISurfaceType *pMat, IMaterial *pRenderMat, int bStatic, 
																				 SBreakEvent &bev, std::vector<EntityId> &chunkIds, bool bLoading, IStatObj* &pStatObjAux)
{
	CBreakablePlane *pPlane;
	IStatObj *pNewStatObj = pStatObj,*pStatObjBase=0, *pStatObjSrc=pStatObj;
	phys_geometry *pPhysGeom;
	int *pIdx,*pIdx0,iChunk;
	float r=4,lifetime=5,filterAng=0.0f,ry=0.0f,curFracture;
	vector2df *ptout,bounds[2];
	IEntitySystem *pEntitySystem = gEnv->pEntitySystem;
	IEntityClass *pClass = 0;
	SEntitySpawnParams params;
	IEntity *pEntity;
	IParticleEffect *pEffect=0;
	IParticleEmitter* pEmitter=0;
	ISurfaceType::SBreakable2DParams *pBreak2DParams = pMat->GetBreakable2DParams();
	if (!pBreak2DParams)
		return pStatObj;
	const char *strEffectName;
	SEntityPhysicalizeParams epp;
	pe_params_particle ppp;
	pe_action_set_velocity asv;
	pe_action_impulse ai;
	Matrix33 mtxrot = Matrix33(mtx);
	Vec3 size,pos0,center,rhit;
	bool bRigidBody=true, bUseImpulse, bUseEdgeAlpha=false;
	params.vPosition = pos0 = mtx.GetTranslation();
	params.vScale.x=params.vScale.y=params.vScale.z = mtxrot.GetRow(0).len();
	mtxrot.OrthonormalizeFast();
	params.qRotation = Quat(mtxrot);
	epp.type = PE_RIGID;
	ai.point = pthit;
	ppp.areaCheckPeriod = 0;
	if (!bLoading)
	{
		bev.idxChunkId0 = chunkIds.size();
		bev.seed = *(int*)&pthit.x+*(int*)&pthit.y+*(int*)&pthit.z & 0x7FFFFFFF;
	}
	iChunk = bev.idxChunkId0;

	static ICVar *particle_limit=gEnv->pConsole->GetCVar("g_breakage_particles_limit");
	int nMaxParticles, nCurParticles=gEnv->pPhysicalWorld->GetEntityCount(PE_PARTICLE), icount=0,mask;
	float curTime = gEnv->pTimer->GetCurrTime();
	nMaxParticles = particle_limit ? particle_limit->GetIVal():200;
	if (curTime > g_maxPieceLifetime)
		g_nPieces = 0;
	nCurParticles = min(g_nPieces, nCurParticles);

	if (pPhysGeom=pStatObj->GetPhysGeom()) 
	{
		mesh_data *pmd;
		IRenderMesh *pRndMesh;
		if (bStatic && pPhysGeom->pGeom->GetType()==GEOM_TRIMESH && 
				(pmd = (mesh_data*)pPhysGeom->pGeom->GetData()) && pmd->pMats && pmd->nIslands>1 && 
				(pRndMesh=pStatObj->GetRenderMesh()) && 
				pRndMesh->GetVerticesCount()==pmd->nVertices && pRndMesh->GetIndicesCount()==pmd->nTris*3)
		{
			IGeomManager *pGeoman = gEnv->pPhysicalWorld->GetGeomManager();
			int nIdx,i,j,idxMin,ichunk,ivtxMin,ivtxMax,ivtx,imat,queue[64],ihead,itail,nNewTris;
			Vec3 ptloc = mtx.GetInverted()*pthit;
			primitives::ray aray;
			aray.dir = mtx.GetInverted().TransformVector(hitvel.normalized());
			aray.origin = ptloc-aray.dir*0.5f;
			IGeometry *pRay = pGeoman->CreatePrimitive(primitives::ray::type, &aray);
			geom_contact *pcontacts;
			geom_world_data gwd;
			for(i=(j=pPhysGeom->pGeom->Intersect(pRay,&gwd,0,0,pcontacts)-1)-1; i>=0; i--)
				j += i-j & -isneg((pcontacts[i].pt-ptloc).len2()-(pcontacts[j].pt-ptloc).len2());

			if (j>=0) 
			{
				if (!(pStatObj->GetFlags() & STATIC_OBJECT_CLONE)) 
				{
					int surfaceTypesIds[MAX_SUB_MATERIALS];
					phys_geometry *pCloneGeom = pGeoman->RegisterGeometry(pRay, pPhysGeom->surface_idx, surfaceTypesIds,pRenderMat->FillSurfaceTypeIds(surfaceTypesIds)); 
					pRay->Release();
					pCloneGeom->origin=pPhysGeom->origin; pCloneGeom->q=pPhysGeom->q; 
					pCloneGeom->Ibody=pPhysGeom->Ibody; pCloneGeom->V=pPhysGeom->V;
					pCloneGeom->pGeom = pGeoman->CloneGeometry(pPhysGeom->pGeom);
					pStatObj = pStatObj->Clone();
					pStatObj->SetFlags(pStatObj->GetFlags() | STATIC_OBJECT_CLONE);
					pStatObj->SetPhysGeom(pCloneGeom);
					pStatObj->SetFilePath(pStatObjSrc->GetFilePath());
					pmd = (mesh_data*)(pPhysGeom=pCloneGeom)->pGeom->GetData();
					pRndMesh = pStatObj->GetRenderMesh();
					pRndMesh->SetMaterial(pStatObj->GetMaterial());
				}
				pRay->Release();

				imat = pmd->pMats[queue[0] = pcontacts[j].iPrim[0]]; pmd->pMats[queue[0]] = -2;
				for(nNewTris=itail=ivtxMax=0,ihead=1,ivtxMin=pmd->nVertices; ihead!=itail && nNewTris<pmd->nTris; nNewTris++) {
					i = queue[itail]; itail = itail+1 & sizeof(queue)/sizeof(queue[0])-1;	
					for(j=0;j<3;j++) {
						if (pmd->pTopology[i].ibuddy[j]>=0 && pmd->pMats[pmd->pTopology[i].ibuddy[j]]==imat) {
							pmd->pMats[queue[ihead] = pmd->pTopology[i].ibuddy[j]] = -2;
							ihead = ihead+1 & sizeof(queue)/sizeof(queue[0])-1;
						}	
						ivtxMin = qmin(ivtxMin, pmd->pIndices[i*3+j]);
						ivtxMax = qmax(ivtxMax, pmd->pIndices[i*3+j]);
					}
				}
				unsigned short *pNewIdx = new unsigned short[nNewTris*3];
				for(i=j=0;i<pmd->nTris;i++) if (pmd->pMats[i]==-2) {
					pNewIdx[j++]=pmd->pIndices[i*3]; pNewIdx[j++]=pmd->pIndices[i*3+1]; pNewIdx[j++]=pmd->pIndices[i*3+2];
					pmd->pMats[i] = -1;
					if (j>=nNewTris*3)
						break;
				}
				for(i=0;i<j;i++) pNewIdx[i] -= ivtxMin;
				IGeometry *pIsleGeom = pGeoman->CreateMesh(pmd->pVertices+ivtxMin,pNewIdx,0,0,nNewTris, mesh_AABB|mesh_multicontact1,0);
				delete[] pNewIdx;

        Vec3 *pVtxTemp = NULL;
				strided_pointer<Vec3> pVtx;
				pVtx.data = (Vec3*)pRndMesh->GetPosPtr(pVtx.iStride, FSL_READ);
				float distMin=1.0f,distMinDenom=0;
        nIdx = pRndMesh->GetIndicesCount();
				unsigned short *pIdx = pRndMesh->GetIndexPtr(FSL_SYSTEM_UPDATE);
				PodArray<CRenderChunk> &meshChunks = pRndMesh->GetChunks();
				IMaterial *pSubMtl;
				ISurfaceType *pst;
				imat = pMat->GetId();
				for(i=idxMin=ichunk=0; i<meshChunks.size(); i++) 
					if ((pSubMtl=pRenderMat->GetSubMtl(meshChunks[i].m_nMatID)) && (pst=pSubMtl->GetSurfaceType()) && pst->GetId()==imat || 
							!pSubMtl && (pst=pRenderMat->GetSurfaceType()) && pst->GetId()==imat)
						for(j=meshChunks[i].nFirstIndexId; j<meshChunks[i].nFirstIndexId+meshChunks[i].nNumIndices; j+=3) {
							Vec3 vtx[3]={pVtx[pIdx[j]],pVtx[pIdx[j+1]],pVtx[pIdx[j+2]]},edge,l,n=vtx[1]-vtx[0]^vtx[2]-vtx[0],lxedge;
							float dist=0,distDenom=0,distVtx=(ptloc-vtx[0]).len2();
							int ivtx,nInside=0,bOnEdge=0,bInside;
							for(ivtx=0; ivtx<3; ivtx++,bOnEdge|=bInside) {
								distVtx = qmin(distVtx, (ptloc-vtx[0]).len2());
								lxedge = (l=ptloc-vtx[ivtx])^(edge=vtx[inc_mod3[ivtx]]-vtx[ivtx]);
								nInside += (bInside = isneg(lxedge*n));
								bInside = inrange(sqr_signed(l*edge),0.0f,edge.len2()) & 1-bInside;
								dist += lxedge.len2()*bInside;
								distDenom += edge.len2()*bInside;
							}
							bInside=isneg(2-nInside); dist+=sqr((ptloc-vtx[0])*n)*bInside; distDenom+=n.len2()*bInside;
							bInside=1-bInside & 1-bOnEdge; dist+=distVtx*bInside; distDenom+=bInside;
							bInside = isneg(dist*distMinDenom-distMin*distDenom);
							distMin += (dist-distMin)*bInside; distMinDenom += (distDenom-distMinDenom)*bInside;
							idxMin += j-idxMin & -bInside;
							ichunk += i-ichunk & -bInside;
						}
				Vec3 center(ZERO); float r=0;
				char *pMats = new char[nNewTris=meshChunks[ichunk].nNumIndices/3]; memset(pMats,0,nNewTris);
				IGeometry *pRndPhysGeom = pGeoman->CreateMesh(pVtx, pIdx+meshChunks[ichunk].nFirstIndexId, pMats,0, nNewTris, 
					mesh_shared_vtx|mesh_shared_mats|mesh_SingleBB, 0);
				pmd = (mesh_data*)pRndPhysGeom->GetData();
				pmd->pMats[queue[0] = (idxMin-meshChunks[ichunk].nFirstIndexId)/3] = -2;
				for(nNewTris=itail=ivtxMax=0,ihead=1,ivtxMin=pmd->nVertices; ihead!=itail; nNewTris++) {
					i = queue[itail]; itail = itail+1 & sizeof(queue)/sizeof(queue[0])-1;	
					for(j=0;j<3;j++) {
						if (pmd->pTopology[i].ibuddy[j]>=0 && pmd->pMats[pmd->pTopology[i].ibuddy[j]]!=-2) {
							pmd->pMats[queue[ihead] = pmd->pTopology[i].ibuddy[j]] = -2;
							ihead = ihead+1 & sizeof(queue)/sizeof(queue[0])-1;
						}
						ivtx = pIdx[meshChunks[ichunk].nFirstIndexId+i*3+j];
						ivtxMin = qmin(ivtxMin, ivtx);
						ivtxMax = qmax(ivtxMax, ivtx);
						center += pVtx[pmd->pIndices[i*3+j]];
					}
				}
				IIndexedMesh *pIdxMesh;
				IStatObj *pIsleStatObj = gEnv->p3DEngine->CreateStatObj();
				CMesh *pMesh = (pIdxMesh=pIsleStatObj->GetIndexedMesh())->GetMesh();
				pIsleStatObj->SetMaterial(pStatObj->GetMaterial());
				pIdxMesh->SetIndexCount(nNewTris*3);
				pIdxMesh->SetVertexCount(ivtxMax-ivtxMin+1);
				pIdxMesh->SetTexCoordsAndTangentsCount(ivtxMax-ivtxMin+1);
				strided_pointer<Vec2f16> pTex;
				strided_pointer<Vec4sf> pTang,pBinorm; 
				pTex.data    = (Vec2f16*)pRndMesh->GetUVPtr(pTex.iStride, FSL_READ);
				pTang.data   = (Vec4sf*)pRndMesh->GetTangentPtr(pTang.iStride, FSL_READ);
				pBinorm.data = (Vec4sf*)pRndMesh->GetBinormalPtr(pBinorm.iStride, FSL_READ);
				for(i=ihead=(idxMin=meshChunks[ichunk].nFirstIndexId)/3,j=0; i*3<meshChunks[ichunk].nFirstIndexId+meshChunks[ichunk].nNumIndices; i++) 
					if (pmd->pMats[i-ihead]==-2)
						for(ivtx=0;ivtx<3;ivtx++) pMesh->m_pIndices[j++]=pIdx[i*3+ivtx]-ivtxMin;
					else
						for(ivtx=0;ivtx<3;ivtx++) pIdx[idxMin++]=pIdx[i*3+ivtx];
				for(i=0,center*=(1.0f/(nNewTris*3)); i<=ivtxMax-ivtxMin; i++) {
					pMesh->m_pPositions[i] = pVtx[i+ivtxMin];
					pMesh->m_pTexCoord[i].s = pTex[i+ivtxMin].ToVec2().x;
					pMesh->m_pTexCoord[i].t = pTex[i+ivtxMin].ToVec2().y;
					pMesh->m_pTangents[i].Binormal = pBinorm[i+ivtxMin];
					pMesh->m_pTangents[i].Tangent = pTang[i+ivtxMin];
					Vec4 tng=tPackB2F(pMesh->m_pTangents[i].Tangent), bnm=tPackB2F(pMesh->m_pTangents[i].Binormal);
					pMesh->m_pNorms[i] = (*(Vec3*)&tng^*(Vec3*)&bnm)/(tng.w*bnm.w);
					r = qmax(r, (pVtx[i+ivtxMin]-center).len2());
				}
				delete[] pMats;
				pRndPhysGeom->Release();
				pIdxMesh->SetSubSetCount(1);
				SMeshSubset &mss = pIdxMesh->GetSubSet(0);
				mss.nFirstIndexId = 0; mss.nNumIndices = j;
				mss.nFirstVertId  = 0; mss.nNumVerts = ivtxMax-ivtxMin+1;
				mss.nMatID = meshChunks[ichunk].m_nMatID;
				mss.nMatFlags = meshChunks[ichunk].m_nMatFlags;
				mss.nPhysicalizeType = PHYS_GEOM_TYPE_DEFAULT;
				mss.vCenter = center;
				mss.fRadius = r;
				CRenderChunk chunk = meshChunks[ichunk];
				if (!(chunk.nNumIndices = idxMin-meshChunks[ichunk].nFirstIndexId)) {
					chunk.nNumIndices = 3; chunk.m_nMatFlags |= MTL_FLAG_NODRAW;
				}
				pRndMesh->SetChunk(ichunk, chunk);
				pIsleStatObj->SetPhysGeom(pPhysGeom = pGeoman->RegisterGeometry(pIsleGeom,imat));
				pIsleGeom->Release();
				pIsleStatObj->Invalidate(false);

				pStatObjBase = pStatObj;
				pNewStatObj=pStatObj = pIsleStatObj;

        SAFE_DELETE_ARRAY(pVtxTemp);
			} else
				return pStatObjSrc;
		}

		primitives::box bbox;
		pPhysGeom->pGeom->GetBBox(&bbox);
		int iup = idxmin3((float*)&bbox.size);

		pPlane = (CBreakablePlane*)pPhysGeom->pGeom->GetForeignData(1);
		pPhysGeom->pGeom->SetForeignData(0,1);
		curFracture = pPlane ? pPlane->m_pGrid->GetFracture() : 0.0f;
		bRigidBody = pBreak2DParams->rigid_body != 0;
		if (!pPlane && min(bbox.size[inc_mod3[iup]],bbox.size[dec_mod3[iup]]) < bbox.size[iup]*4) 
		{
			CryLog("[Breakable2d] : geometry is not thin enough, breaking denied ( %s %s)", pStatObjSrc->GetFilePath(), pStatObjSrc->GetGeoName());
			return pStatObjSrc;
		}
		if (!bRigidBody && nCurParticles>=nMaxParticles && pBreak2DParams->no_procedural_full_fracture)
			curFracture = 10.0f;

		r = pPlane ? pBreak2DParams->blast_radius : pBreak2DParams->blast_radius_first;
		lifetime = pBreak2DParams->life_time;
		strEffectName = pBreak2DParams->particle_effect;
		if (!bLoading && strEffectName[0])
			pEffect = gEnv->pParticleManager->FindEffect(strEffectName);
		bUseEdgeAlpha = pBreak2DParams->use_edge_alpha != 0;
		ry = r*(1.0f+cry_frand()*pBreak2DParams->vert_size_spread);
		ry = max(ry,hitradius);
		hitradius = r = max(r,hitradius); 
		filterAng = pBreak2DParams->filter_angle*gf_PI*(1.0f/180);
		IParticleEffect *pFractureFx = 0;
		float scale = mtx.GetColumn0().len();
		Vec2 c=Vec2((bbox.Basis*(mtx.GetInverted()*pthit-bbox.center)).GetPermutated(iup)), sz=Vec2(bbox.size.GetPermutated(iup));
		if (_boxCircleIntersArea(sz,c,r/scale)/(sz.x*sz.y*4) + curFracture >= pBreak2DParams->max_fracture)
		{
			r=ry = 1e5f;
			if (!bLoading && pBreak2DParams->full_fracture_fx[0] && 
					(pFractureFx = gEnv->pParticleManager->FindEffect(pBreak2DParams->full_fracture_fx)))
			{
				Vec3 n = mtx.TransformVector(bbox.Basis.GetRow(idxmin3((const float*)bbox.size)));
				pFractureFx->Spawn(false, IParticleEffect::ParticleLoc(mtx*bbox.center, n*(float)-sgnnz(n*hitvel), 
					scale*max(sz.x,sz.y)));
			}	
			if (pBreak2DParams->no_procedural_full_fracture || curFracture>9.0f)
			{
				hitradius = 0.0f;
				if (pPlane)
					delete pPlane;
				return pStatObjBase;
			}
		}
		if (!bLoading && !pFractureFx && pBreak2DParams->fracture_fx[0] && 
				(pFractureFx = gEnv->pParticleManager->FindEffect(pBreak2DParams->fracture_fx)))
		{
			Vec3 n = mtx.TransformVector(bbox.Basis.GetRow(idxmin3((const float*)bbox.size)));
			pFractureFx->Spawn(false, IParticleEffect::ParticleLoc(pthit, n*(float)-sgnnz(n*hitvel), scale));
		}

		if (!pPlane)
		{
			pPlane = new CBreakablePlane;
			pPlane->m_cellSize = pBreak2DParams->cell_size;
			pPlane->m_maxPatchTris = pBreak2DParams->max_patch_tris;
			pPlane->m_density = pBreak2DParams->shard_density;
			strcpy(pPlane->m_mtlSubstName, pBreak2DParams->broken_mtl);
			if (pPlane->SetGeometry(pStatObj,pRenderMat,bStatic,bev.seed))
			{
				pPlane->m_matId = pMat->GetId();
				if (pRenderMat)
					pPlane->m_pMat = pRenderMat;
			} else
				delete pPlane, pPlane=0;
		}	else
			pPlane->m_bStatic = bStatic;

		if (pPlane && (pIdx0=pIdx = pPlane->Break(mtx.GetInverted()*pthit,r,ptout,bev.seed,filterAng,ry)))
		{
			epp.density = pPlane->m_density;
			if (pPlane->m_density<=0)
				ppp.iPierceability = -1;
			do {
				if (pNewStatObj = pPlane->CreateFlatStatObj(pIdx,ptout,bounds,mtx,pEffect,!bRigidBody,bUseEdgeAlpha))
				{
					pNewStatObj->SetFilePath(pStatObj->GetFilePath());
					if (*pIdx==-1)
					{ // create a new entity for this stat obj
						if (!pClass)
							pClass = pEntitySystem->GetClassRegistry()->FindClass("Default");
						params.sName = "breakable_plane_piece";
						params.pClass = pClass;
						params.nFlags = ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_NO_PROXIMITY;
						params.id = 0;
						center = pPlane->m_R*Vec3(bounds[0].x+bounds[1].x,bounds[0].y+bounds[1].y,0)*0.5f + pPlane->m_center;
						rhit = pthit-mtx*center;
						asv.v = hitvel*0.5f;
						ai.impulse = hitvel*hitmass;
						if (!bRigidBody)
						{
							epp.type = PE_PARTICLE;
							epp.pParticle = &ppp;
							ppp.size = max(bounds[1].x-bounds[0].x,bounds[1].y-bounds[0].y);
							ppp.thickness = pPlane->m_z[1]-pPlane->m_z[0];
							ppp.surface_idx = pPlane->m_matId;
							ppp.mass = ppp.size*ppp.thickness*pPlane->m_density;
							if ((bUseImpulse = hitmass<ppp.mass) && ai.impulse.len2() > sqr(ppp.mass*10.0f))
								ai.impulse = ai.impulse.normalized()*ppp.mass*10.0f;
							ppp.flags = particle_no_path_alignment|particle_no_roll|pef_traceable|particle_no_self_collisions;
							ppp.normal = pPlane->m_R*Vec3(0,0,1);
							ppp.q0 = params.qRotation;
							params.vPosition = pos0+params.qRotation*(center*params.vScale.x);
						}	else
							bUseImpulse = hitmass<0.2f;
						asv.w = (rhit/rhit.len2())^hitvel;

						if (!bRigidBody && lifetime>0)
						{
							if (nCurParticles*7>nMaxParticles)
								mask = 7;
							else if (nCurParticles*6>nMaxParticles)
								mask = 3;
							if (nCurParticles*4>nMaxParticles)
								mask = 1;
							else
								mask = 0;
							if (bLoading || ++icount & mask) {
								pNewStatObj->Release(); continue;
							}
							ParticleParams pp;
							pp.fCount = 0;
							pp.fParticleLifeTime.Set( lifetime, 0.25f );
							pp.bRemainWhileVisible = false;
							pp.fSize = params.vScale.x;
							pp.ePhysicsType = ParticlePhysics_SimplePhysics;
							pe_params_pos ppos;
							ppos.pos = params.vPosition;
							ppos.q = params.qRotation;
							IPhysicalEntity* pent = gEnv->pPhysicalWorld->CreatePhysicalEntity(PE_PARTICLE,&ppos);
							pent->SetParams(&ppp);
							pent->Action(bUseImpulse ? (pe_action*)&ai:&asv);
							if (!pEmitter)
								pEmitter = gEnv->pParticleManager->CreateEmitter(true, Matrix34(Vec3(1.0f),params.qRotation,params.vPosition),pp);
							pNewStatObj->AddRef();
							pEmitter->EmitParticle(pNewStatObj,pent);
							if (pNewStatObj->Release()<=0)
								gEnv->pPhysicalWorld->DestroyPhysicalEntity(pent);
							nCurParticles++;
							g_nPieces++; g_maxPieceLifetime = max(g_maxPieceLifetime, curTime+lifetime);
						} else
						{
							if (bLoading)
								params.id = chunkIds[iChunk++];
							pEntity = pEntitySystem->SpawnEntity(params);
							if (!bLoading)
								chunkIds.push_back(pEntity->GetId());
							pEntity->CreateProxy(ENTITY_PROXY_RENDER);
							pEntity->SetStatObj(pNewStatObj, 0,false);
							//if (!bRigidBody)
							//	pEntity->SetSlotLocalTM(0, Matrix34::CreateTranslationMat(-center));
							pEntity->Physicalize(epp);
							if (pEntity->GetPhysics())
								pEntity->GetPhysics()->Action(bUseImpulse ? (pe_action*)&ai:&asv);
						}
						pNewStatObj=0; 
					}
				}
			}	while(*pIdx++!=-2);
			gEnv->pPhysicalWorld->GetPhysUtils()->DeletePointer(pIdx0);
		}
		if (pPlane && !pNewStatObj)
			delete pPlane;
	}
	if (!bLoading)
		bev.nChunks = chunkIds.size()-bev.idxChunkId0;

	if (pStatObjBase)
	{
		if (pStatObj)
			pStatObj->Release();
		pStatObjAux = pNewStatObj;
		pNewStatObj = pStatObjBase;
	}

	return pNewStatObj;
}
