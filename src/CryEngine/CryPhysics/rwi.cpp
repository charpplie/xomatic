//////////////////////////////////////////////////////////////////////
//
//	RayWorldIntersection implementation
//	
//	File: rwi.cpp
//	Description : RayWorldIntersection and some other related functions implementation
//
//	History:
//	-:Created by Anton Knyazev
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(__SPU__) || defined(USE_RWI_JOB)

#include "bvtree.h"
#include "geometry.h"
#include "raybv.h"
#include "raygeom.h"
#include "physicalplaceholder.h"
#include "physicalentity.h"
#include "physicalworld.h"
#include "heightfieldgeom.h"
#include "geoman.h"

#include <IJobManSPU.h>

geom_contact g_RWIContacts[64] SPU_LOCAL;

#if defined(USE_RWI_JOB) && defined(USE_PHYS_JOBS)
void SaveGeomToMem(CGeometry *pGeom, CMemStream &stm, int bForRays);
CGeometry *LoadGeomFromMem(CMemStream &stm);

char g_geomBuf[16384] _ALIGN(128) SPU_LOCAL;
CPhysicalEntity *g_rwiTmpEntList[1024] _ALIGN(128) SPU_LOCAL;
int g_bRWIFailed SPU_LOCAL;
SPU_DOMAIN_LOCAL CRayGeom *g_pRay SPU_LOCAL;

SPU_DOMAIN_LOCAL IGeometry *PrepGeom(IGeometry *pGeom,int iCaller) {
#ifndef __SPU__
#ifdef PHYS_JOB_SIMULATION
	if (iCaller<=MAX_PHYS_THREADS)
#endif
		return pGeom;
#endif
#ifdef __SPU__
	CMemStream stm(g_geomBuf,sizeof(g_geomBuf),false);
  stm.bMeasureOnly = 1;
#else
	CMemStream stm;
#endif
	SaveGeomToMem((CGeometry*)pGeom, stm, 0);
	if (stm.m_iPos>sizeof(g_geomBuf)) {
		g_bRWIFailed=1; return g_pRay;
	}
	CMemStream stmReal(g_geomBuf,sizeof(g_geomBuf),false);
	SaveGeomToMem((CGeometry*)pGeom, stmReal, 0);
	stmReal.m_iPos = 0;
	return LoadGeomFromMem(stmReal);
}

#ifdef __SPU__
template<class Etype> int OnEvent(unsigned int flags, Etype *pEvent, Etype **pEventLogged=0){}
#define m_lockCaller m_SurfaceFlagsTable
#define m_lockPotentialPartListUpdate Lock_dummy_param()
#define m_lockPotentialMeshUpdate Lock_dummy_param()
#define m_lockGrid
#define m_lockAreas Lock_dummy_param()
class CPhysicalWorld {
#else
#include "physicalworld.h"
class CPhysicalWorldStub {
#endif
public:
	int RayWorldIntersection(Vec3 org,Vec3 dir, int objtypes, unsigned int flags, ray_hit *hits,int nmaxhits, SPU_DOMAIN_MAIN IPhysicalEntity* pSkipEnts[8],int nSkipEnts, 
		ray_hit_cached *phitLast, int iCaller);
	void RayHeightfield(const Vec3 &org,Vec3 &dir, ray_hit *hits, int flags, int iCaller);
	void RayWater(const Vec3 &org,const Vec3 &dir, struct entity_grid_checker &egc, int flags,int nMaxHits, ray_hit *hits);
	int TracePendingRays(int bDoTracing);
	int StartRWIJob();
	int EndRWIJob();
	void RWIJob();

	primitives::grid m_entgrid;
	pe_entgrid m_pEntGrid;
	int m_iEntAxisz;
	float m_zGran;
	int m_bGridThunksChanged;
	int m_matWater;
	unsigned int *m_SurfaceFlagsTable;
	CPhysicalEntity *m_pHeightfield[MAX_PHYS_THREADS+2];
	pe_gridthunk *m_gthunks;
	Matrix33 m_HeightfieldBasis;
	Vec3 m_HeightfieldOrigin;
	int m_bCheckWaterHits;
	int m_prevGEAobjtypes[MAX_PHYS_THREADS+1];
	CPhysArea *m_pGlobalArea;
	CPhysicalPlaceholder **m_pEntsById;
	SRwiRequest *m_jobRWI;
	int m_nJobRWIs,m_nJobRWIsDone,m_nJobRWIAlloc;
} _ALIGN(128) g_world;
#else
#include "physicalworld.h"
#define PrepGeom(pGeom,iCaller) (pGeom)
#endif

#ifdef __SPU__
CPhysicalWorld g_LocalPhysWorld _ALIGN(128) SPU_LOCAL;
#endif

ILINE void MarkProcessed(volatile unsigned int& bProcessed, int mask)
{
#ifdef __SPU__
	unsigned int curProcessedVal = bProcessed;
	bProcessed = curProcessedVal + curProcessedVal & mask ^ mask;
#else
	AtomicAdd(&bProcessed, bProcessed & mask ^ mask);
#endif
}

ILINE void UnmarkProcessed(volatile unsigned int& bProcessed, int mask)
{
#ifdef __SPU__
	unsigned int curProcessedVal = bProcessed;
	bProcessed = curProcessedVal + (-(int)(curProcessedVal & mask));
#else
	AtomicAdd(&bProcessed, -(int)(bProcessed & mask));
#endif
}

ILINE void AddSafe(volatile unsigned int& bProcessed, int val)
{
#ifdef __SPU__
	unsigned int curProcessedVal = bProcessed;
	bProcessed = curProcessedVal + val;
#else
	AtomicAdd(&bProcessed, val);
#endif
}

ILINE void MarkSkipEnts(IPhysicalEntity* pSkipEnts[8],int nSkipEnts, int mask)
{
	for(int i=0;i<nSkipEnts;i++) {
		CPhysicalPlaceholder* pEnt = (CPhysicalPlaceholder*)pSkipEnts[i];
		if (pEnt) {
			MarkProcessed(pEnt->m_bProcessed, mask);
			if (pEnt->m_pEntBuddy)
				MarkProcessed(pEnt->m_pEntBuddy->m_bProcessed, mask);
		}
	}
}
ILINE void UnmarkSkipEnts(IPhysicalEntity* pSkipEnts[8],int nSkipEnts, int mask)
{
	for(int i=0;i<nSkipEnts;i++) {
		CPhysicalPlaceholder* pEnt = (CPhysicalPlaceholder*)pSkipEnts[i];
		if (pEnt) {
			UnmarkProcessed(pEnt->m_bProcessed, mask);
			if (pEnt->m_pEntBuddy)
				UnmarkProcessed(pEnt->m_pEntBuddy->m_bProcessed, mask);
		}
	}
}

bool ray_box_overlap2d(const Vec2 &org,const Vec2 &dir, const Vec2 &boxmin,const Vec2 &boxmax)
{
	Vec2 n(dir.y,-dir.x), center=(boxmin+boxmax)*0.5f, size=(boxmax-boxmin)*0.5f;
	return max(max(fabs_tpl(center.x-org.x-dir.x*0.5f) - (size.x+fabs_tpl(dir.x)*0.5f),
								 fabs_tpl(center.y-org.y-dir.y*0.5f) - (size.y+fabs_tpl(dir.y)*0.5f)),
								 fabs_tpl(n*(center-org))-fabs_tpl(n.x)*size.x-fabs_tpl(n.y)*size.y) <= 0;
}
 
#undef _ILINE
#ifdef __SPU__
	#define _ILINE SPU_NO_INLINE
#else
	#define _ILINE ILINE
#endif

struct entity_grid_checker {
	geom_world_data gwd;
	intersection_params ip;
	int nMaxHits,nThroughHits,nThroughHitsAux,objtypes,nEnts,bCallbackUsed,nParts;
	unsigned int flags,flagsColliderAll,flagsColliderAny;
	vector2df org2d,dir2d;
	float dir2d_len,maxt;
	SPU_DOMAIN_LOCAL ray_hit *phits;
	CPhysicalWorld *pWorld;
	CPhysicalPlaceholder *pGridEnt;
	CPhysicalEntity **pTmpEntList;
	int szList;
	SPU_DOMAIN_MAIN void *pSkipForeignData;
	int iSkipForeignData;
	int iCaller;
	int bUsePhysOnDemand;
	CRayGeom aray;
	pe_gridthunk *pThunkSubst,thunkSubst;
	int ipartSubst,ipartMask;
	int iSolidNode;
	entity_grid_checker() { pThunkSubst=0; ipartSubst=ipartMask=0; }

	NO_INLINE int check_cell(const vector2di &icell, int &ilastcell) {
		quotientf t((org2d+icell)*dir2d, dir2d_len*dir2d_len);
		if (t.x>maxt && (icell.x&icell.y)!=-1)
			return 1;

		box bbox;
		bbox.Basis.SetIdentity();
		bbox.bOriented = 0;
		SPU_DOMAIN_LOCAL geom_contact *pcontacts = 0;
		pe_gridthunk *pthunk;
		int ithunk,ithunk_next;
		pe_PODcell *pPODcell;
		int i,j,ihit,imat,nCellEnts=0,nEntsChecked=0,bRecheckOtherParts,bNoThunkSubst;
#ifndef __SPU__
		pWorld->GetPODGridCellBBox(icell.x,icell.y, bbox.center,bbox.size);
		if (bbox.size.z*bUsePhysOnDemand>0 && box_ray_overlap_check(&bbox,&aray.m_ray)) {
			CryInterlockedAdd(&pWorld->m_lockGrid,-1);
			ReadLock lockPOD(pWorld->m_lockPODGrid);
			if ((pPODcell=pWorld->getPODcell(icell.x,icell.y))->lifeTime<=0) {
				CryInterlockedAdd(&pWorld->m_lockPODGrid,-1);
				{ WriteLock lockPODw(pWorld->m_lockPODGrid);
					if (pPODcell->lifeTime<=0) {
						MarkAsPODThread(pWorld);
						pWorld->m_nOnDemandListFailures=0; ++pWorld->m_iLastPODUpdate;
						if (pWorld->m_pPhysicsStreamer->CreatePhysicalEntitiesInBox(bbox.center-bbox.size,bbox.center+bbox.size)) {	
							pPODcell->lifeTime = pWorld->m_nOnDemandListFailures ? 1E10f:8.0f;
							pPODcell->inextActive = pWorld->m_iActivePODCell0;
							pWorld->m_iActivePODCell0 = icell.y<<16|icell.x;
							szList = max(szList, pWorld->GetTmpEntList(pTmpEntList, iCaller));
						}
						pWorld->m_nOnDemandListFailures=0;
					}
				} ReadLockCond lockPODr(pWorld->m_lockPODGrid,1); lockPODr.SetActive(0);
			}	else
				pPODcell->lifeTime = max(8.0f,pPODcell->lifeTime);
			UnmarkAsPODThread(pWorld);
			ReadLockCond relock(pWorld->m_lockGrid,1); relock.SetActive(0);
		}
#endif
		//fetch some memory references to avoid reloads
		primitives::grid& RESTRICT_REFERENCE entgrid = pWorld->m_entgrid;
		const Vec3 entgrid_origin			= entgrid.origin;
		const vector2df entgrid_step	= entgrid.step;
		const vector2df entgrid_stepr	= entgrid.stepr;
		pe_entgrid pEntGrid = pWorld->m_pEntGrid;
		pe_gridthunk *const __restrict pgthunks = pWorld->m_gthunks;
		const int gridIdx = entgrid.getcell_safe(icell.x,icell.y);
		const int pWorld_m_iEntAxisz = pWorld->m_iEntAxisz;
		const float pWorld_m_zGran = pWorld->m_zGran;
		const int pWorld_m_matWater = pWorld->m_matWater;

#ifdef __SPU__
		geom pent_part;
#endif
		int ithunk_prev = pgthunks[pEntGrid[gridIdx]].iprev;
		pthunk = SPU_MAIN_PTR(pgthunks+(ithunk = pEntGrid[gridIdx]));
		bNoThunkSubst = iszero_mask(pThunkSubst);
		pthunk = SPU_MAIN_PTR((pe_gridthunk*)((intptr_t)pthunk + ((intptr_t)pThunkSubst-(intptr_t)pthunk & ~bNoThunkSubst)));

		for(; ithunk; pthunk=SPU_MAIN_PTR(pgthunks+(ithunk=ithunk_next))) {
			pe_gridthunk thunk = *pthunk;
			ithunk_next = thunk.inext;
#ifdef __SPU__
			if (thunk.iprev-ithunk_prev!=0) {
				ithunk_next = pEntGrid[gridIdx];
				continue;	// the thunk list changed, restart the iteration (since no locks are used)
			}
			ithunk_prev = ithunk;
#endif
			if (objtypes & 1u<<thunk.iSimClass &&
					(!(entgrid.inrange(icell.x,icell.y)&-bNoThunkSubst) || 
							(i = pWorld_m_iEntAxisz, 
							 bbox.center[inc_mod3[i]] = (icell.x+(thunk.BBox[2]+1+thunk.BBox[0])*(1.0f/512))*entgrid_step.x,
							 bbox.center[dec_mod3[i]] = (icell.y+(thunk.BBox[3]+1+thunk.BBox[1])*(1.0f/512))*entgrid_step.y,
							 bbox.center[i]						= (thunk.BBoxZ1+thunk.BBoxZ0)*0.5f*pWorld_m_zGran,
							 bbox.center += entgrid_origin,
							 bbox.size[inc_mod3[i]] = (thunk.BBox[2]+1-thunk.BBox[0])*(1.0f/512)*entgrid_step.x,
							 bbox.size[dec_mod3[i]] = (thunk.BBox[3]+1-thunk.BBox[1])*(1.0f/512)*entgrid_step.y,
							 bbox.size[i]						= (thunk.BBoxZ1-thunk.BBoxZ0)*0.5f*pWorld_m_zGran,
							 box_ray_overlap_check(&bbox,&aray.m_ray))) &&
							//box_ray_overlap_check(Vec2(0.5f-org2d.x,0.5f-org2d.y),dir2d,
							//Vec2(icell.x+thunk.BBox[0]*(1.0f/256),icell.y+thunk.BBox[1]*(1.0f/256)),
							//Vec2(icell.x+(thunk.BBox[2]+1)*(1.0f/256),icell.y+(thunk.BBox[3]+1)*(1.0f/256)))) && 
					!(thunk.pent->m_bProcessed>>iCaller & 1) && 
					(!pSkipForeignData || SPU_MAIN_PTR(SPU_MAIN_PTR((CPhysicalPlaceholder*)thunk.pent)->CPhysicalPlaceholder::GetForeignData(iSkipForeignData))!=pSkipForeignData)) 
			{
				ReadLockPlatf0 lock(thunk.pent->m_lockUpdate);
				bbox.center = (thunk.pent->m_BBox[0]+thunk.pent->m_BBox[1])*0.5f;
				bbox.size = (thunk.pent->m_BBox[1]-thunk.pent->m_BBox[0])*0.5f;
				nCellEnts++;

				/*if ((bbox.center-aray.m_ray.origin-aray.m_dirn*((bbox.center-aray.m_ray.origin)*aray.m_dirn)).len2() > bbox.size.len2())
					continue; // skip objects that lie to far from the ray
				if ((box_ray_overlap_check(&bbox,&aray.m_ray) & nEnts-pWorld->m_nEnts>>31)==0)
					continue;*/
				if (nEnts>=szList)
#ifndef __SPU__
					szList = pWorld->ReallocTmpEntList(pTmpEntList,iCaller,szList+1024); 
#else
				{ g_bRWIFailed=1; break; }
#endif
				nEntsChecked++;	
				CPhysicalEntity *pent,*pentLog,*pentFlags,*pentList;
				bCallbackUsed=bRecheckOtherParts = 0;

				if (thunk.pent->m_iSimClass==5) {
					if (((CPhysArea*)thunk.pent)->m_pb.iMedium!=0)
						continue;
					ray_hit ahit;
					CPhysArea	*pPhysArea = SPU_MAIN_PTR((CPhysArea*)thunk.pent);
					if (pPhysArea->RayTrace(aray.m_ray.origin,aray.m_ray.dir,&ahit)) {
#ifndef __SPU__
						WriteLockCond locki(g_lockIntersect,1); locki.SetActive(0);
#endif
						ip.plock = &g_lockIntersect; pcontacts = g_RWIContacts;
						pentFlags = &g_StaticPhysicalEntity;
						pentFlags->m_parts[0].flags = geom_colltype_ray & -iszero((int)flags & rwi_force_pierceable_noncoll);
						pcontacts->t = ahit.dist;
						pcontacts->pt = ahit.pt; 
						pcontacts->n = ahit.n;
						pcontacts->id[0] = pWorld_m_matWater;
						pcontacts->iNode[0] = -1;
						pentList = pent = SPU_MAIN_PTR((CPhysicalEntity*)(pGridEnt = SPU_MAIN_PTR(thunk.pent)));
						i=0; j=1; nParts=0; pentLog=0; goto gotcontacts;
					}
					continue;
				}

				pWorld->m_bGridThunksChanged = 0;
				pentList = pentLog = pentFlags = pent = SPU_MAIN_PTR((pGridEnt=SPU_MAIN_PTR(thunk.pent))->GetEntity());
				if (pWorld->m_bGridThunksChanged)
					ithunk_next = pEntGrid[gridIdx];
				pWorld->m_bGridThunksChanged = 0;
				if ((nParts=pent->m_nParts)==0 || pent->m_flags&pef_use_geom_callbacks) {
					SRayTraceRes rtr(&aray,pcontacts,ip.plock);
					j = pent->RayTrace(rtr);
					pcontacts = rtr.pcontacts;
					i=0; bCallbackUsed=1; goto gotcontacts;
				}

				if (pent!=pGridEnt && pent->m_pEntBuddy!=pGridEnt) {
					ipartSubst = -2-pGridEnt->m_id; ipartMask = -1;	nParts = 1;
					pentList = (CPhysicalEntity*)pGridEnt;
				}

				for(i=ipartSubst; i<nParts+(ipartSubst+1-nParts & ipartMask); i++) {
#ifdef __SPU__
					memcpy(&pent_part, &pent->m_parts[i], sizeof(geom));
#else
					//goto makes it impossible to declare it as const ref, please fix
					#define pent_part (pent->m_parts[i])
#endif
					if ((pent_part.flags & flagsColliderAll)==flagsColliderAll && (pent_part.flags & flagsColliderAny)) {
						if (nParts>1) {
							bbox.center = (pent_part.BBox[0]+pent_part.BBox[1])*0.5f;
							bbox.size = (pent_part.BBox[1]-pent_part.BBox[0])*0.5f;
							if (!box_ray_overlap_check(&bbox,&aray.m_ray))
								continue;
						}
						gwd.offset = pent->m_pos + pent->m_qrot*pent_part.pos;
						//(pent->m_qrot*pent_part.q).getmatrix(gwd.R);	//Q2M_IVO 
						gwd.R = Matrix33(pent->m_qrot*pent_part.q);
						gwd.scale = pent_part.scale;

						j = PrepGeom(pent_part.pPhysGeom->pGeom,iCaller)->Intersect(&aray, &gwd,0, &ip, pcontacts);

						gotcontacts:
						bRecheckOtherParts = (j-1 & 1-nParts)>>31 & ipartMask;

						{
							WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive(isneg(-j));
							check_cell_contact(j,pcontacts,phits,flags,pentFlags,nThroughHits,nThroughHitsAux,
								imat,ilastcell,aray,iSolidNode,pentLog,i,ihit);
						}
					}/*	else 
						if (pent_part.flags & geom_mat_substitutor && phits[0].pCollider && 
								-1==((CPhysicalPlaceholder*)phits[0].pCollider)->m_id &&
								pent_part.pPhysGeom->pGeom->PointInsideStatus(((phits[0].pt-pent->m_pos)*pent->m_qrot-pent_part.pos)*pent_part.q)) 
						phits[0].surface_idx = pent->GetMatId(pent_part.pPhysGeom->surface_idx, i);*/
				}

				pTmpEntList[nEnts] = pentList; nEnts += 1+bRecheckOtherParts;
				AddSafe(pGridEnt->m_bProcessed, 1+bRecheckOtherParts<<iCaller);
				ipartMask = ipartSubst = 0;

				/*next_cell_ent:
				if (thunk.iSimClass==6 && thunk.pent->m_iForeignFlags==0x100) {
					float zlowest = max(phits[0].pt[pWorld_m_iEntAxisz], origin_grid.z*t.y + dir_grid.z*t.x - max_zcell;
					if (zlowest>thunk.pent->m_BBox[0][pWorld_m_iEntAxisz)
						goto return_res;
				}*/
			}
		}
		return (sgn((icell.y<<16|icell.x)-ilastcell)&1)^1;
#undef pent_part
	}

	//need to create extra functions as it is simpy too complex for the spu cache simulation being embedded into check_cell
	_ILINE void check_cell_contact(int& j,geom_contact *&pcontacts, ray_hit *phits, unsigned int flags,
		CPhysicalEntity *pentFlags,int& nThroughHits, int& nThroughHitsAux, int& imat, int &ilastcell,CRayGeom& aray,
		int& iSolidNode, CPhysicalEntity *pentLog,int i, int& ihit)
	{ 
		primitives::grid& RESTRICT_REFERENCE entgrid = pWorld->m_entgrid;
		const int pWorld_m_iEntAxisz = pWorld->m_iEntAxisz;
		const Vec3 entgrid_origin			= entgrid.origin;
		const vector2df entgrid_stepr	= entgrid.stepr;

		float facing;
		for(j--; j>=0; j--) 
		if (pcontacts[j].t<phits[0].dist && (flags & rwi_ignore_back_faces)*(facing=pcontacts[j].n*aray.m_dirn)<=0) {
			imat = pentFlags->GetMatId(pcontacts[j].id[0],i);
			int pierceability = pWorld->m_SurfaceFlagsTable[imat&NSURFACETYPES-1] & sf_pierceable_mask;
			ihit = -(int)(flags&rwi_force_pierceable_noncoll)>>31 & -iszero((int)pentFlags->m_parts[i].flags & (geom_colltype_solid|geom_colltype_ray));
			pierceability += sf_max_pierceable+1-pierceability & ihit;
			ihit = 0;
			if ((int)(flags & rwi_pierceability_mask) < pierceability) {
				if ((pWorld->m_SurfaceFlagsTable[imat&NSURFACETYPES-1]|flags) & sf_important) {
					for(ihit=1; ihit<=nThroughHits && phits[ihit].dist<pcontacts[j].t; ihit++);
					if (ihit<=nThroughHits)
						for(int idx=min(nThroughHits+1,nMaxHits-1)-1; idx>=ihit; idx--)
							memcpy(phits+idx+1, phits+idx, sizeof(ray_hit)-sizeof(ray_hit*)); // don't touch the *next member
						//memmove(phits+ihit+1, phits+ihit, (min(nThroughHits+1,nMaxHits-1)-ihit)*sizeof(ray_hit));
					else if (nThroughHits+1==nMaxHits)
						continue;
					nThroughHits = min(nThroughHits+1, nMaxHits-1);
					nThroughHitsAux = min(nThroughHitsAux, nMaxHits-1-nThroughHits);
				}	else {
					for(ihit=nMaxHits-1; ihit>=nMaxHits-nThroughHitsAux && phits[ihit].dist<pcontacts[j].t; ihit--);
					if (ihit>=nMaxHits-nThroughHitsAux) {
						int istart = max(nMaxHits-nThroughHitsAux-1,nThroughHits+1);
						for(int idx=istart; idx<ihit; idx++)
							memcpy(phits+idx, phits+idx+1, sizeof(ray_hit)-sizeof(ray_hit*));
						//memmove(phits+istart, phits+istart+1, (ihit-istart)*sizeof(ray_hit));
					} else if (nThroughHits+nThroughHitsAux>=nMaxHits-1)
						continue;
					nThroughHitsAux = min(nThroughHitsAux+1, nMaxHits-1-nThroughHits);
				}
			} else {
				if ((flags & rwi_ignore_solid_back_faces)*facing>0)
					continue;
				ilastcell = 
					float2int((pcontacts[j].pt[inc_mod3[pWorld_m_iEntAxisz]]-entgrid_origin[inc_mod3[pWorld_m_iEntAxisz]])*
						entgrid_stepr.x-0.5f) |
					float2int((pcontacts[j].pt[dec_mod3[pWorld_m_iEntAxisz]]-entgrid_origin[dec_mod3[pWorld_m_iEntAxisz]])*
						entgrid_stepr.y-0.5f)<<16;
				aray.m_ray.dir = pcontacts[j].pt-aray.m_ray.origin;
				iSolidNode = pcontacts[j].iNode[0];
			}
			
			phits[ihit].dist = pcontacts[j].t;
			phits[ihit].pCollider = pentLog; 
			phits[ihit].ipart = i+(pcontacts[j].iNode[0]-i & -bCallbackUsed);
			phits[ihit].partid = pcontacts[j].iPrim[0];
			phits[ihit].surface_idx = imat;
			phits[ihit].idmatOrg = pcontacts[j].id[0] + (pentFlags->m_parts[i].surface_idx+1 & pcontacts[j].id[0]>>31);
			phits[ihit].pt = pcontacts[j].pt; 
			phits[ihit].n = pcontacts[j].n;
			phits[ihit].iNode = pcontacts[j].iNode[0];
			phits[ihit].bTerrain = 0;
		}
	}
};

SPU_NO_INLINE void CPhysicalWorld::RayHeightfield(const Vec3 &org,Vec3 &dir, ray_hit *hits, int flags, int iCaller) 
{
	geom_world_data gwd;
	intersection_params ip;
	SPU_DOMAIN_LOCAL geom_contact *pcontacts;
	CRayGeom aray(org,dir);
	gwd.R = m_HeightfieldBasis.T();
	gwd.offset = m_HeightfieldOrigin;
	if (((CHeightfield*)SPU_MAIN_PTR(m_pHeightfield[iCaller]->m_parts[0].pPhysGeom->pGeom))->CHeightfield::Intersect(&aray, &gwd,SPU_LOCAL_PTR((geom_world_data*)0), &ip, pcontacts)) {
		WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
		if (pcontacts->id[0]>=0 || flags&rwi_ignore_terrain_holes) {
			dir = pcontacts->pt-org;
			hits[0].dist = pcontacts->t;
			hits[0].pCollider = m_pHeightfield[iCaller]; 
			hits[0].partid = hits[0].ipart = 0;
			hits[0].surface_idx = m_pHeightfield[iCaller]->GetMatId(pcontacts->id[0],0);
			hits[0].idmatOrg = pcontacts->id[0];
			hits[0].pt = pcontacts->pt; 
			hits[0].n = pcontacts->n;
			hits[0].bTerrain = 1;
		}
	}
}

SPU_NO_INLINE void CPhysicalWorld::RayWater(const Vec3 &org,const Vec3 &dir, entity_grid_checker &egc, int flags,int nMaxHits, ray_hit *hits)
{
	ReadLock lockAr(m_lockAreas);
	int i;
	quotientf t(1,0);//(m_pGlobalArea->m_pb.waterPlane.origin-org)*m_pGlobalArea->m_pb.waterPlane.n, dir*m_pGlobalArea->m_pb.waterPlane.n);
	CPhysArea *pArea = NULL,*pHitArea = NULL;
	ray_hit whit;
	Vec3 n = m_pGlobalArea->m_pb.waterPlane.n;
	box bbox;
	float l = egc.aray.m_ray.dir*egc.aray.m_dirn;
	bbox.Basis.SetIdentity();
	bbox.bOriented = 0;
	if (m_pGlobalArea->RayTrace(org,dir,&whit))
		t.set(whit.dist,l), n=whit.n, pHitArea=m_pGlobalArea;

	for(pArea=m_pGlobalArea->m_nextBig; pArea; pArea=pArea->m_nextBig) if (pArea->m_pb.iMedium==0) {
		const Vec3& bb0 = pArea->m_BBox[0]; 
		const Vec3& bb1 = pArea->m_BBox[1];
		bbox.center = (bb0+bb1)*0.5f;
		bbox.size = (bb1-bb0)*0.5f;
		if (box_ray_overlap_check(&bbox,&egc.aray.m_ray) && pArea->RayTrace(org,dir,&whit) && whit.dist*sqr(t.y)<=t.x*t.y*l) {
			t.set(whit.dist,l); n=whit.n, pHitArea=pArea;
		}
	}

	if (inrange(t.x, 0.0f,t.y)) {
		if ((flags & rwi_pierceability_mask) < (m_SurfaceFlagsTable[m_matWater] & sf_pierceable_mask)+((int)flags & rwi_force_pierceable_noncoll)) {
			if (nMaxHits<=1)
				goto nowater;
			if ((m_SurfaceFlagsTable[m_matWater]|(flags^rwi_separate_important_hits)) & sf_important)
				i = egc.nThroughHits = 1;
			else
				i = nMaxHits-(egc.nThroughHitsAux=1);
		}	else
			i = 0;
		hits[i].dist = (t.x=t.val())*dir.len();
		hits[i].pCollider = pHitArea; 
		hits[i].partid = hits[0].ipart = 0;
		hits[i].surface_idx = m_matWater;
		hits[i].idmatOrg = 0;
		hits[i].pt = org+dir*t.x; 
		hits[i].n = n;
		hits[i].bTerrain = 0;
		nowater:;
	}
}

SPU_NO_INLINE int CPhysicalWorld::RayWorldIntersection(Vec3 org,Vec3 dir, int objtypes, unsigned int flags, ray_hit *hits,int nMaxHits, 
																				 IPhysicalEntity* pSkipEnts[8],int nSkipEnts,
#ifndef __SPU__
																				 void *pForeignData,int iForeignData, const char *pNameTag,
#endif
																				 ray_hit_cached *phitLast, int iCaller)
{
	if (!(dir.len2()<1E20f && org.len2()<1E20f)) {
		//VALIDATOR_LOG(m_pLog,"RayWorldIntersection: ray is out of bounds");
		//if (m_vars.bBreakOnValidation) DoBreak
		return 0;
	}
	if (dir.len2()==0)
		return 0;

	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );

#ifndef __SPU__
	if (flags & rwi_queue) {
		WriteLock lockQ(m_lockRwiQueue);
		int i;
		ReallocQueue(m_rwiQueue, m_rwiQueueSz,m_rwiQueueAlloc, m_rwiQueueHead,m_rwiQueueTail, 64);
		m_rwiQueue[m_rwiQueueHead].pForeignData = pForeignData;
		m_rwiQueue[m_rwiQueueHead].iForeignData = iForeignData;
		m_rwiQueue[m_rwiQueueHead].org = org;
		m_rwiQueue[m_rwiQueueHead].dir = dir;
		m_rwiQueue[m_rwiQueueHead].objtypes = objtypes;
		m_rwiQueue[m_rwiQueueHead].flags = flags & ~rwi_queue;
		m_rwiQueue[m_rwiQueueHead].phitLast = phitLast;
		m_rwiQueue[m_rwiQueueHead].iCaller = iCaller;
		if (!(m_rwiQueue[m_rwiQueueHead].hits = hits)) {
			WriteLock lockH(m_lockRwiHitsPool);
			int nhits=0;
			ray_hit *phit=m_pRwiHitsTail->next,*pchunk=0;
			if (m_rwiPoolEmpty || phit->next!=m_pRwiHitsHead)
				for(nhits=1; nhits<nMaxHits && (m_rwiPoolEmpty || phit->next!=m_pRwiHitsHead); nhits++,phit=phit->next,m_rwiPoolEmpty=0) 
					if (phit->next!=phit+1)
						pchunk=phit,nhits=0;
			if (nhits<nMaxHits) {
				if (!pchunk)
					for(pchunk=m_pRwiHitsHead; pchunk->next==pchunk+1; pchunk++);
				phit = new ray_hit[nhits=max(nMaxHits,512)];
				for(i=0;i<nhits-1;i++) phit[i].next = phit+i+1;
				phit[nhits-1].next=pchunk->next; pchunk->next=phit;
				m_rwiHitsPoolSize += nhits;
			}	else
				phit = (pchunk ? pchunk:m_pRwiHitsTail)->next;
			m_pRwiHitsTail = phit+nMaxHits-1; m_rwiPoolEmpty = 0;
			m_rwiQueue[m_rwiQueueHead].hits = phit;
			m_rwiQueue[m_rwiQueueHead].iCaller |= 1<<16;
		}
		m_rwiQueue[m_rwiQueueHead].nMaxHits = nMaxHits;
		m_rwiQueue[m_rwiQueueHead].nSkipEnts = min(sizeof(m_rwiQueue[0].idSkipEnts)/sizeof(m_rwiQueue[0].idSkipEnts[0]),nSkipEnts);
		for(i=0;i<m_rwiQueue[m_rwiQueueHead].nSkipEnts;i++)
			m_rwiQueue[m_rwiQueueHead].idSkipEnts[i] = pSkipEnts[i] ? GetPhysicalEntityId(pSkipEnts[i]):-3;
		m_rwiQueueSz++;
		return 1;
	}
#endif


	PHYS_FUNC_PROFILER( pNameTag );

	int i,nHits; for(i=0;i<nMaxHits;i++) { hits[i].dist=1E10; hits[i].bTerrain=0; hits[i].pCollider=0; }
	entity_grid_checker egc;
	egc.nThroughHits = egc.nThroughHitsAux = 0;
	CPhysicalEntity *pentLastHit = 0;
	int ipartLastHit,inodeLastHit;
	if (phitLast) {
		pentLastHit = (CPhysicalEntity*)phitLast->pCollider;
		ipartLastHit = phitLast->ipart;
		inodeLastHit = phitLast->iNode;
	}

	if ((objtypes & ent_terrain) && SPU_MAIN_PTR(m_pHeightfield[iCaller]))
		RayHeightfield(org,dir,hits,flags,iCaller);
	egc.aray.CreateRay(org,dir);

	if (objtypes & m_bCheckWaterHits & ent_water) {
		RayWater(org,dir,egc,flags,nMaxHits,hits);
		objtypes |= ent_areas;
	}

	if (objtypes & ~(ent_terrain|ent_water)) {
		WriteLock lock(m_lockCaller[iCaller]);
		MarkSkipEnts(pSkipEnts,nSkipEnts,1<<iCaller);

		egc.phits = hits;
		egc.pWorld = this;
		egc.objtypes = objtypes;
		egc.flags = flags^rwi_separate_important_hits;
		if (!(egc.flagsColliderAll = flags>>rwi_colltype_bit))
			egc.flagsColliderAll = geom_colltype_ray;
		if (flags & rwi_ignore_noncolliding)
			egc.flagsColliderAll |= geom_colltype0;
		if (flags & rwi_colltype_any)	{
			egc.flagsColliderAny = egc.flagsColliderAll; egc.flagsColliderAll = 0;
		}	else egc.flagsColliderAny = geom_collides;
		egc.bUsePhysOnDemand = iszero((objtypes & (ent_static|ent_no_ondemand_activation))-ent_static);
		egc.nMaxHits = nMaxHits;
		egc.pSkipForeignData = (nSkipEnts>0 && SPU_MAIN_PTR(pSkipEnts[0])) ? SPU_MAIN_PTR((CPhysicalPlaceholder*)pSkipEnts[0])->CPhysicalPlaceholder::GetForeignData(egc.iSkipForeignData=SPU_MAIN_PTR((CPhysicalPlaceholder*)pSkipEnts[0])->CPhysicalPlaceholder::GetiForeignData()) : SPU_MAIN_PTR((void*)NULL);
		egc.ip.bStopAtFirstTri = (flags & rwi_any_hit)!=0;
#if defined(__SPU__) && defined(USE_RWI_JOB)
		egc.pTmpEntList = g_rwiTmpEntList;
		egc.szList = sizeof(g_rwiTmpEntList)/sizeof(g_rwiTmpEntList[0]);
		g_pRay = &egc.aray;
#elif defined(PHYS_JOB_SIMULATION) && defined(USE_RWI_JOB)
		if (iCaller>MAX_PHYS_THREADS) {
			egc.pTmpEntList = g_rwiTmpEntList;
			egc.szList = sizeof(g_rwiTmpEntList)/sizeof(g_rwiTmpEntList[0]);
			g_pRay = &egc.aray;
		} else
			egc.szList = GetTmpEntList(egc.pTmpEntList, iCaller);
#else 
		egc.szList = GetTmpEntList(egc.pTmpEntList, iCaller);
#endif
		egc.iCaller = iCaller;
		m_prevGEAobjtypes[iCaller] = -1;

		Vec3 origin_grid = (org-m_entgrid.origin).GetPermutated(m_iEntAxisz), dir_grid = dir.GetPermutated(m_iEntAxisz);
		egc.org2d.set(0.5f-origin_grid.x*m_entgrid.stepr.x, 0.5f-origin_grid.y*m_entgrid.stepr.y);
		egc.dir2d.set(dir_grid.x*m_entgrid.stepr.x, dir_grid.y*m_entgrid.stepr.y);
		egc.dir2d_len = len(egc.dir2d);
		egc.nEnts = 0;

		if (flags & rwi_reuse_last_hit && pentLastHit) {
			egc.maxt = 1E20f;
			egc.thunkSubst.inext = 0;
			egc.thunkSubst.pent = pentLastHit;
			egc.thunkSubst.iSimClass = pentLastHit->m_iSimClass;
			egc.pThunkSubst = &egc.thunkSubst;
			egc.ipartSubst = ipartLastHit; egc.ipartMask = -1;
			egc.gwd.iStartNode = inodeLastHit;
			egc.check_cell(vector2di(0,0),i);
			if (hits[0].dist>0) {
				dir=egc.aray.m_ray.dir = hits[0].pt-egc.aray.m_ray.origin;
				dir_grid = dir.GetPermutated(m_iEntAxisz);
				egc.dir2d.set(dir_grid.x*m_entgrid.stepr.x, dir_grid.y*m_entgrid.stepr.y);
				egc.dir2d_len = len(egc.dir2d);
			}
			egc.pThunkSubst=0; egc.ipartSubst=egc.ipartMask=0; egc.gwd.iStartNode=0;
		}
		egc.maxt = egc.dir2d_len*(egc.dir2d_len+sqrt2)+0.0001f;

		{ ReadLockPlatf1 lockp0(m_lockPotentialPartListUpdate),lockp1(m_lockPotentialMeshUpdate);
			ReadLock lockg(m_lockGrid);
			if (fabsf(origin_grid.x*m_entgrid.stepr.x*2-m_entgrid.size.x)>m_entgrid.size.x || 
					fabsf(origin_grid.y*m_entgrid.stepr.y*2-m_entgrid.size.y)>m_entgrid.size.y || 
					fabsf((origin_grid.x+dir_grid.x)*m_entgrid.stepr.x*2-m_entgrid.size.x)>m_entgrid.size.x || 
					fabsf((origin_grid.y+dir_grid.y)*m_entgrid.stepr.y*2-m_entgrid.size.y)>m_entgrid.size.y)
				egc.check_cell(vector2di(-1,-1),i);

			DrawRayOnGrid(&m_entgrid, origin_grid,dir_grid, egc);
		}
		
		for(i=0;i<egc.nEnts;i++)
			AddSafe((egc.pTmpEntList[i]->m_pEntBuddy && egc.pTmpEntList[i]->m_pEntBuddy==egc.pTmpEntList[i] ?
									egc.pTmpEntList[i]->m_pEntBuddy : egc.pTmpEntList[i])->m_bProcessed, -(1<<iCaller));
		
		UnmarkSkipEnts(pSkipEnts,nSkipEnts,1<<iCaller);

		if (flags & rwi_separate_important_hits) {
			int j,idx[2]; ray_hit thit;
			for(idx[0]=1,idx[1]=nMaxHits-1,i=1; idx[0]+nMaxHits-idx[1]-2<egc.nThroughHits+egc.nThroughHitsAux; i++) {
				j = isneg(hits[idx[1]].dist-hits[idx[0]].dist);	// j = hits[1].dist<hits[0].dist ? 1:0;
				j |= isneg(egc.nThroughHits-idx[0]);						// if (idx[0]>nThroughHits) j = 1; 
				j &= nMaxHits-egc.nThroughHitsAux-1-idx[1]>>31; // if (idx[1]<=nMaxHits-nThroughHits1-1) j=0;	
				hits[idx[j]].bTerrain = i;
				idx[j] += 1-j*2;
			}
			for(i=egc.nThroughHits+1; i<nMaxHits-egc.nThroughHitsAux; i++)
				hits[i].bTerrain = nMaxHits+1;
			for(i=1;i<nMaxHits;) if (hits[i].bTerrain!=i && hits[i].bTerrain<nMaxHits) {
				thit=hits[hits[i].bTerrain]; hits[hits[i].bTerrain]=hits[i]; hits[i]=thit;
			}	else i++;
		}
	}

	nHits = 0;
	if (hits[0].dist>1E9f) {
		hits[0].dist = -1;
		hits[0].pt = org+dir;
	} else {
		CPhysicalEntity *pent = (CPhysicalEntity*)hits[0].pCollider;
		if (pent && pent->m_iSimClass!=5 && hits[0].ipart<pent->m_nParts) {
			hits[0].foreignIdx = pent->m_parts[hits[0].ipart].pPhysGeom->pGeom->GetForeignIdx(hits[0].partid);
			hits[0].partid = pent->m_parts[hits[0].ipart].id;
			pentLastHit = pent;
			ipartLastHit = hits[0].ipart;
			inodeLastHit = egc.iSolidNode;
		} else
			hits[0].foreignIdx=hits[0].partid = -1;
		nHits++;
	}
#ifndef __SPU__
	if (m_vars.iDrawHelpers & 64 && m_pRenderer) {
		int nTicks=0;
#ifndef PHYS_FUNC_PROFILER_DISABLED
		nTicks = CryGetTicks()-func_profiler.m_iStartTime;
#endif
		int bQueued = iszero(m_lockTPR>>16^1 | iCaller);
		m_pRenderer->DrawLine(org,hits[0].pt,8-bQueued,1|nTicks*2);
	}
#endif
	for(i=1;i<nMaxHits;i++) {
		if (hits[i].dist>1E9f || hits[0].dist>0 && hits[i].dist>hits[0].dist)
			hits[i].dist = -1;
		else { 
			CPhysicalEntity *pent = (CPhysicalEntity*)hits[i].pCollider;
			if (pent && pent->m_iSimClass!=5 && hits[i].ipart<pent->m_nParts) {
				hits[i].foreignIdx = pent->m_parts[hits[i].ipart].pPhysGeom->pGeom->GetForeignIdx(hits[i].partid);
				hits[i].partid = pent->m_parts[hits[i].ipart].id;
			} else
				hits[i].foreignIdx=hits[i].partid = -1;
			hits[i].bTerrain=0; nHits++; 
		}
	}

	if (phitLast && flags & rwi_update_last_hit) {
		phitLast->pCollider = pentLastHit;
		phitLast->ipart = ipartLastHit;
		phitLast->iNode = inodeLastHit;
	}

	return nHits;
}

#ifndef __SPU__
int CPhysicalWorld::TracePendingRays(int bDoTracing)
{	
	int i,nChex=0;
	SPU_DOMAIN_MAIN IPhysicalEntity* pSkipEnts[8];
	int iCaller = get_iCaller();
	WriteLock lockg(m_lockTPR);

	if (bDoTracing==2)
		return EndRWIJob();

	if (!StartRWIJob()) { 
		SRwiRequest curreq;
		EventPhysRWIResult eprr;
		eprr.pEntity = &g_StaticPhysicalEntity;

		do {
			{ WriteLock lock(m_lockRwiQueue);
				if (m_rwiQueueSz==0)
					break;
				curreq = m_rwiQueue[m_rwiQueueTail];
				m_rwiQueueTail = m_rwiQueueTail+1 - (m_rwiQueueAlloc & m_rwiQueueAlloc-2-m_rwiQueueTail>>31);
				m_rwiQueueSz--; nChex++;
			}
			eprr.pForeignData = curreq.pForeignData;
			eprr.iForeignData = curreq.iForeignData;
			for(i=0; i<curreq.nSkipEnts; i++)
				pSkipEnts[i] = GetPhysicalEntityById(curreq.idSkipEnts[i]);
			eprr.nHits = bDoTracing ? RayWorldIntersection(curreq.org,curreq.dir,curreq.objtypes,curreq.flags,
				curreq.hits,curreq.nMaxHits,pSkipEnts,i,
				curreq.phitLast,iCaller) : 0;
			eprr.bHitsFromPool = curreq.iCaller>>16;
			eprr.nMaxHits = curreq.nMaxHits;
			eprr.pHits = curreq.hits;
			OnEvent(0,&eprr);
		} while(true);
	}

	{ SPwiRequest curreq;
		EventPhysPWIResult eppr;
		eppr.pEntity = &g_StaticPhysicalEntity;
		geom_contact *pcontact=0;

		do {
			{ WriteLock lock(m_lockPwiQueue);
				if (m_pwiQueueSz==0)
					break;
				curreq = m_pwiQueue[m_pwiQueueTail];
				m_pwiQueueTail = m_pwiQueueTail+1 - (m_pwiQueueAlloc & m_pwiQueueAlloc-2-m_pwiQueueTail>>31);
				m_pwiQueueSz--; nChex++;
			}
			eppr.pForeignData = curreq.pForeignData;
			eppr.iForeignData = curreq.iForeignData;
			for(i=0; i<curreq.nSkipEnts; i++)
				pSkipEnts[i] = GetPhysicalEntityById(curreq.idSkipEnts[i]);
			if ((eppr.dist = bDoTracing ? PrimitiveWorldIntersection(curreq.itype,(primitive*)curreq.pprim,curreq.sweepDir,curreq.entTypes,
				&pcontact,curreq.geomFlagsAll,curreq.geomFlagsAny,0,0,0,pSkipEnts,i) : 0) && pcontact)
			{ eppr.pt = pcontact->pt;
				eppr.n = pcontact->n;
				eppr.idxMat = pcontact->id[1];
				eppr.partId = pcontact->iPrim[1];
				if (!(eppr.pEntity = GetPhysicalEntityById(pcontact->iPrim[0])))
					eppr.pEntity = &g_StaticPhysicalEntity;
			}
			OnEvent(0,&eppr);
		} while(true);
	}

	return nChex;
}
#endif //__SPU__

#if defined(USE_RWI_JOB) && defined(USE_PHYS_JOBS)
#if !defined(CRYCG_CM)
SPU_ENTRY(RWI)
#endif
void RWIJobProc(CPhysicalWorld *pWorld)	
{	
#if defined(__SPU__)
	// HACK REMOVE ME AFTER A SUCCESSFUL INSTANTIATION OF polynomial<real, 4> has
  // been made by inclusion of other functions
  int i = 1; 
  if (i == 0) { real roots[4]; polynomial_tpl<real,4> p; p.findroots(0.f,1.f,SPU_LOCAL_PTR(roots)); }

	memtransfer_from_main(&g_LocalPhysWorld, SPU_MAIN_PTR(pWorld), (sizeof(CPhysicalWorld)+127)&~127, 0);
	memtransfer_sync(0);
	g_LocalPhysWorld.RWIJob(); 
	memtransfer_to_main(SPU_MAIN_PTR(pWorld), &g_LocalPhysWorld, (sizeof(CPhysicalWorld)+127)&~127, 0);
	memtransfer_sync(0);
#endif
}

#ifndef __SPU__
DECLARE_SPU_JOB("RWI", TRWIJob)

int CPhysicalWorld::StartRWIJob()
{
	if (!m_rwiQueueSz)
		return 0;
#if defined(PS3) && !defined(PHYS_JOB_SIMULATION)
	if (!InvokeJobOnSPU("RWI"))
		return 0;
#endif
	{ WriteLock lock(m_lockRwiQueue);
		if (m_nJobRWIsAlloc<m_rwiQueueSz)
			ReallocateList(m_jobRWI, 0,m_nJobRWIsAlloc=m_rwiQueueSz+63&~64);
		for(m_nJobRWIs=0; m_nJobRWIs<m_rwiQueueSz; m_nJobRWIs++) {
			m_jobRWI[m_nJobRWIs] = m_rwiQueue[m_rwiQueueTail];
			m_rwiQueueTail = m_rwiQueueTail+1 - (m_rwiQueueAlloc & m_rwiQueueAlloc-2-m_rwiQueueTail>>31);
			for(int i=0;i<m_jobRWI[m_nJobRWIs].nSkipEnts;i++)
				m_jobRWI[m_nJobRWIs].idSkipEnts[i] = (int)GetPhysicalEntityById(m_jobRWI[m_nJobRWIs].idSkipEnts[i]);
			m_rwiQueueSz--;
		}
		m_nJobRWIsDone = 0;
		m_rwiQueueSz = 0;
	}
	g_world.m_entgrid=m_entgrid; g_world.m_pEntGrid=m_pEntGrid; g_world.m_iEntAxisz=m_iEntAxisz;
	g_world.m_bCheckWaterHits=m_bCheckWaterHits; g_world.m_gthunks=m_gthunks;
	g_world.m_pGlobalArea=m_pGlobalArea;
	for(int i=0;i<MAX_PHYS_THREADS; ++i) m_prevGEAobjtypes[i]=0;
	g_world.m_zGran=m_zGran; g_world.m_matWater=m_matWater; g_world.m_SurfaceFlagsTable=m_SurfaceFlagsTable;
	m_pHeightfield[MAX_PHYS_THREADS+1]=g_world.m_pHeightfield[MAX_PHYS_THREADS+1]=g_world.m_pHeightfield[0] = m_pHeightfield[0];
	g_world.m_HeightfieldBasis = m_HeightfieldBasis;
	g_world.m_HeightfieldOrigin = m_HeightfieldOrigin;
	g_bRWIFailed = 0;
#if defined(PS3) && !defined(__SPU__)
	if (InvokeJobOnSPU("RWI")) {
		TRWIJob job(&g_world);
		job.SetCacheMode(NPPU::eCM_8);
		job.Run();
	} else
#endif
		RWIJob();
	return 1;
}

int CPhysicalWorld::EndRWIJob()
{
	SPU_DOMAIN_MAIN IPhysicalEntity* pSkipEnts[8];
	EventPhysRWIResult eprr;
	eprr.pEntity = &g_StaticPhysicalEntity;
	// wait for job to finish
	if(m_jobRWI)	{
		for(int i=0; i<m_nJobRWIsDone; i++) {
			SRwiRequest &curreq = m_jobRWI[i];
			eprr.pForeignData = curreq.pForeignData;
			eprr.iForeignData = curreq.iForeignData;
			if (curreq.iCaller==-1)	{
				for(int j=0; j<curreq.nSkipEnts; j++)
					pSkipEnts[j] = GetPhysicalEntityById(curreq.idSkipEnts[j]);
				eprr.nHits = RayWorldIntersection(curreq.org,curreq.dir,curreq.objtypes,curreq.flags,
					curreq.hits,curreq.nMaxHits,pSkipEnts,i,curreq.phitLast, 0);
			} else
				eprr.nHits = curreq.iCaller & 0xFFFF;
			eprr.bHitsFromPool = curreq.iCaller>>16;
			eprr.nMaxHits = curreq.nMaxHits;
			eprr.pHits = curreq.hits;
			OnEvent(0,&eprr);
		}
	}
	return m_nJobRWIsDone;
}
#endif

void CPhysicalWorld::RWIJob()
{
	SPU_DOMAIN_MAIN IPhysicalEntity* pSkipEnts[8];

	for(int i=0;i<m_nJobRWIs;i++) {
		SRwiRequest &curreq = m_jobRWI[i];
		for(int j=0; j<curreq.nSkipEnts; j++)
			pSkipEnts[j] = SPU_MAIN_PTR((IPhysicalEntity*)curreq.idSkipEnts[j]);
		g_bRWIFailed = 0;
#ifdef __SPU__		
		char *phitbuf = (char*)alloca(sizeof(ray_hit) * curreq.nMaxHits);
		ray_hit *plocalhits = (ray_hit*)phitbuf;
		memcpy(plocalhits,curreq.hits,curreq.nMaxHits*sizeof(ray_hit));
#else
		ray_hit *plocalhits = curreq.hits;
#endif
		(curreq.iCaller&=~0xFFFF) |= RayWorldIntersection(curreq.org,curreq.dir,curreq.objtypes,curreq.flags,
			plocalhits,curreq.nMaxHits,pSkipEnts,i,SPU_MAIN_PTR(curreq.phitLast),MAX_PHYS_THREADS+1);
#ifdef __SPU__		
		memcpy(curreq.hits,plocalhits,curreq.nMaxHits*sizeof(ray_hit));
#endif
		if (g_bRWIFailed)
			curreq.iCaller = -1;
	}
	m_nJobRWIsDone = m_nJobRWIs;
	m_nJobRWIs = 0;
}
#else
int CPhysicalWorld::StartRWIJob() { return 0; }
int CPhysicalWorld::EndRWIJob() { return 0; }
void CPhysicalWorld::RWIJob() {}
#endif
IGeometry *PrepGeomExt(IGeometry *pGeom) { return PrepGeom(pGeom,0); }
#endif 

#undef _ILINE
