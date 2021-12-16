//////////////////////////////////////////////////////////////////////
//
//	Rope Entity
//	
//	File: ropeentity.cpp
//	Description : CRopeEntity class implementation
//
//	History:
//	-:Created by Anton Knyazev
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "bvtree.h"
#include "geometry.h"
#include "overlapchecks.h"
#include "raybv.h"
#include "raygeom.h"
#include "singleboxtree.h"
#include "boxgeom.h"
#include "cylindergeom.h"
#include "rigidbody.h"
#include "physicalplaceholder.h"
#include "physicalentity.h"
#include "geoman.h"
#include "physicalworld.h"
#include "ropeentity.h"
#include "trimesh.h"

int FindJobQueue(int sz, CMemStream &stm, int ipass);
void SaveGeomToMem(CGeometry *pGeom, CMemStream &stm, int bForRays);
CGeometry *LoadGeomFromMem(CMemStream &stm);
int GeomOffsInJobBuf(CGeometry *pGeom, int ijob);
void SaveGeomToJobBuf(CMemStream &stm, CGeometry *pGeom, int ioffs, int ijob, int bForRays);
CGeometry *LoadGeomFromMemBufOffs(CMemStream &stm);
void UpdateJobBuf(int ijob, int szAdd);

#ifndef __SPU__
CRopeEntity::CRopeEntity(CPhysicalWorld *pWorld) : CPhysicalEntity(pWorld)
#ifdef DEBUG_ROPES
,m_segs(&m_nSegs,1),m_vtx(&m_nVtxAlloc),m_vtx1(&m_nVtxAlloc),m_idx(&m_nMaxSubVtx,2),m_vtxSolver(&m_nVtxAlloc)
#endif
{
	m_nSegs = -1;
	m_segs = 0;
	m_length = 0;
	m_pTiedTo[0] = m_pTiedTo[1] = 0;
	m_iTiedPart[0] = m_iTiedPart[1] = 0;
	m_ptTiedLoc[0].zero(); m_ptTiedLoc[1].zero();
	m_idConstraint = 0;
	m_iConstraintClient = 0;
	if(pWorld) {
		m_gravity0 = m_gravity = pWorld->m_vars.gravity;
		m_stiffness = m_pWorld->m_vars.unprojVelScale;
	} else {
		m_gravity0 = m_gravity = Vec3(0,0,0);
		m_stiffness = 10.0f;
	}
	m_bAwake = m_nSlowFrames = 0;
	m_damping = 0.2f;
	m_iSimClass = 4;
	m_Emin = sqr(0.04f);
	m_maxAllowedStep = 0.05f;
	m_mass = 1.0f;
	m_collDist = 0.01f;
	m_flags = 0;
	m_surface_idx = 0;
	m_ig[0].x=m_ig[1].x=m_ig[0].y=m_ig[1].y = -3;
	m_defpart.id = 0;
	m_friction = 0.2f;
	m_stiffnessAnim = 70.0f;
	m_stiffnessDecayAnim = 0.75f;
	m_dampingAnim = 0;
	m_bTargetPoseActive = 0;
	m_wind0=m_wind1 = m_wind.zero();
	m_windVariance = m_windTimer = 0;
	m_airResistance = 0;
	m_waterResistance = 5.0f;
	m_rdensity = 1.0f/500;
	m_jointLimit = 0;
	m_posBody[0][0]=m_posBody[0][1]=m_posBody[1][0]=m_posBody[1][1] = Vec3(0,0,0);
	m_qBody[0][0]=m_qBody[0][1]=m_qBody[1][0]=m_qBody[1][1] = quaternionf(IDENTITY);
	m_dir0dst.Set(0,0,1);
	m_szSensor = 0.05f;
	m_maxForce = 0;
	m_flagsCollider = geom_colltype0|geom_colltype_player;
	m_vtx=0; m_nVtx=m_nVtx0=m_nVtxAlloc=0; m_vtxSolver=0; m_vtx1=0;
	m_nMaxSubVtx = 3;
	m_idx = 0;
	m_bHasContacts = 0;
	m_bStrained = 0;
	m_frictionPull = 0;
	m_energy = 0;
	m_nFragments = 0;
	m_pContact = 0;
	m_lastTimeStep = 0;
	m_timeLastActive = 0;
	m_lockVtx = 0;
	m_lockStep = 0;
	m_maxIters = 650;
	m_penaltyScale = m_pWorld ? m_pWorld->m_vars.unprojVelScale*2 : 5.0f;
	m_nSleepingNeighboursFrames = 0;
	m_attachmentZone = 0;
	m_bContactsRegistered = 0;
	m_minSegLen = 0;
	MARK_UNUSED m_collBBox[0];
	m_lastposHost.zero();
	m_lastqHost.SetIdentity();
	m_jobE = -1.0f;
	m_unprojLimit = 0.5f;
	m_noCollDist = 0.5f;
	m_lockAwake = 0;
}

CRopeEntity::~CRopeEntity()
{
	if (m_segs)	delete[] m_segs;
	if (m_vtx) delete[] m_vtx;
	if (m_vtx1) delete[] m_vtx1;
	if (m_vtxSolver) delete[] m_vtxSolver;
	if (m_idx) delete[] m_idx;
	m_segs = 0; m_nSegs = -1;
}
#endif 

void CRopeEntity::AlertNeighbourhoodND(int mode)
{
	pe_params_rope pr;
	pr.pEntTiedTo[0] = pr.pEntTiedTo[1] = 0;
	SetParams(&pr);
	int i;
	if (m_nSegs>0) for(i=0;i<=m_nSegs;i++) if (m_segs[i].pContactEnt) {
		if (!(m_flags & rope_subdivide_segs))
			m_segs[i].pContactEnt->Release(); 
		m_segs[i].pContactEnt=0;
	}
	for(i=0;i<m_nColliders;i++)
		m_pColliders[i]->Release();
	m_nColliders = 0;
}

int CRopeEntity::Awake(int bAwake,int iSource)
{ 
	m_bAwake=bAwake; m_nSlowFrames=0; 
	int i;
	if (m_nSegs>0) {
		WriteLock lock(m_lockAwake);
		for(i=0;i<=m_nSegs;i++)	if (m_segs[i].pContactEnt && (m_segs[i].pContactEnt->m_iDeletionTime>0 || 
			m_segs[i].iContactPart>=m_segs[i].pContactEnt->m_nParts))
		{
			if (!(m_flags & rope_subdivide_segs))
				m_segs[i].pContactEnt->Release();
			m_segs[i].pContactEnt = 0;
			MARK_UNUSED m_segs[i].ncontact;
		}
		if (m_vtx && m_bStrained) for(i=0;i<m_nVtx;i++) if (m_vtx[i].pContactEnt && (m_vtx[i].pContactEnt->m_iDeletionTime>0 || 
			m_vtx[i].iContactPart>=m_vtx[i].pContactEnt->m_nParts))
			m_vtx[i].pContactEnt=0, MARK_UNUSED m_vtx[i].ncontact;
		for(i=0;i<2;i++) if (m_pTiedTo[i] && m_pTiedTo[i]->m_iDeletionTime>0)	{
			if (m_pTiedTo[i]->GetType()==PE_ROPE)
				m_pTiedTo[i]->RemoveCollider(this);
			m_pTiedTo[i]->Release(); m_pTiedTo[i]=0;
		}
		for(i=0;i<m_nColliders;i++) if (m_pColliders[i]->GetType()==PE_ROPE)
			m_pColliders[i]->Awake();
		if (!(m_flags & pef_step_requested))
			m_timeIdle = 0;
	}
	return 1; 
}

#ifndef __SPU__
void CRopeEntity::RecalcBBox()
{
	if (m_flags & pef_traceable) {
		m_BBox[0]=m_BBox[1] = m_nSegs>0 ? m_segs[0].pt:m_pos;
		for(int i=1;i<=m_nSegs;i++) {
			m_BBox[0] = min(m_BBox[0],m_segs[i].pt); 
			m_BBox[1] = max(m_BBox[1],m_segs[i].pt); 
		}
		m_BBox[0]-=Vec3(m_collDist*2); m_BBox[1]+=Vec3(m_collDist*2);
		AtomicAdd(&m_pWorld->m_lockGrid,-m_pWorld->RepositionEntity(this,1,m_BBox));
	}
}


int CRopeEntity::SetParams(pe_params *_params, int bThreadSafe)
{
	ChangeRequest<pe_params> req(this,m_pWorld,_params,bThreadSafe);
	if (req.IsQueued())
		return 1;

	int res;
	unsigned int flags0 = m_flags;
	Vec3 prevpos = m_pos;
	quaternionf prevq = m_qrot;
	if (res = CPhysicalEntity::SetParams(_params,1)) {
		if (_params->type==pe_params_pos::type_id) {
			WriteLock lock(m_lockUpdate);
			Matrix33 R = Matrix33(m_qrot);
			if ((prevpos-m_pos).len2()>sqr(0.0001f) || (prevq.v-m_qrot.v).len2()>sqr(0.0001f))
				m_nVtx = 0;
			for(int i=0;i<=m_nSegs;i++) {
				m_segs[i].pt = m_segs[i].pt0 = R*(m_segs[i].pt-prevpos)+m_pos;
				m_segs[i].vel = R*m_segs[i].vel;
				m_segs[i].dir = R*m_segs[i].dir;
			}
			m_qrot.SetIdentity();
		}
		if (m_nSegs>0 && !(m_flags & (rope_collides|rope_collides_with_terrain)) && (flags0 & (rope_collides|rope_collides_with_terrain)))
			for(int i=0;i<=m_nSegs;i++) m_segs[i].pContactEnt = 0;
		if (m_flags&pef_traceable && !(flags0&pef_traceable))
			RecalcBBox();
		return res;
	}

	if (_params->type==pe_simulation_params::type_id) {
		pe_simulation_params *params = (pe_simulation_params*)_params;
		if (!is_unused(params->gravity)) m_gravity = m_gravity0 = params->gravity;
		if (!is_unused(params->damping)) m_damping = params->damping;
		if (!is_unused(params->maxTimeStep)) m_maxAllowedStep = params->maxTimeStep;
		if (!is_unused(params->minEnergy)) m_Emin = params->minEnergy;
		return 1;
	}

	if (_params->type==pe_params_rope::type_id) {
		pe_params_rope *params = (pe_params_rope*)_params;
		WriteLock lock(m_lockUpdate);
		int i;
		float diff=0;
		if (!is_unused(params->length)) m_length = params->length;
		else if (!is_unused(params->nSegments) && !is_unused(params->pPoints) && params->pPoints)
			for(i=0,m_length=0;i<params->nSegments;i++) m_length += (params->pPoints[i+1]-params->pPoints[i]).len();
		if (!is_unused(params->mass)) m_mass = params->mass;
		if (!is_unused(params->collDist)) { m_collDist = params->collDist; RecalcBBox(); }
		if (!is_unused(params->surface_idx)) m_surface_idx = params->surface_idx;
		if (!is_unused(params->friction)) m_friction = params->friction;
		if (!is_unused(params->stiffnessAnim)) 
			diff=fabs_tpl(m_stiffnessAnim-params->stiffnessAnim), m_stiffnessAnim=params->stiffnessAnim;
		if (!is_unused(params->stiffnessDecayAnim)) m_stiffnessDecayAnim = params->stiffnessDecayAnim;
		if (!is_unused(params->dampingAnim)) m_dampingAnim = params->dampingAnim;
		if (!is_unused(params->bTargetPoseActive)) 
			diff+=fabs_tpl(m_bTargetPoseActive-params->bTargetPoseActive), m_bTargetPoseActive = params->bTargetPoseActive;
		if (!is_unused(params->wind)) 
			diff+=(m_wind0-params->wind).len2(), m_wind0=m_wind1=m_wind = params->wind;
		if (!is_unused(params->windVariance)) m_windVariance = params->windVariance;
		if (!is_unused(params->airResistance)) m_airResistance = params->airResistance;
		if (!is_unused(params->waterResistance)) m_waterResistance = params->waterResistance;
		if (!is_unused(params->density)) m_rdensity = 1.0f/params->density;
		if (!is_unused(params->jointLimit)) m_jointLimit = params->jointLimit; 
		if (!is_unused(params->sensorRadius)) m_szSensor = params->sensorRadius;
		if (!is_unused(params->maxForce)) m_maxForce = params->maxForce;
		if (!is_unused(params->flagsCollider)) m_flagsCollider = params->flagsCollider;
		if (!is_unused(params->nMaxSubVtx)) m_nMaxSubVtx = params->nMaxSubVtx;
		if (!is_unused(params->frictionPull)) m_frictionPull = params->frictionPull;
		if (!is_unused(params->stiffness)) m_stiffness = params->stiffness;
		if (!is_unused(params->penaltyScale)) m_penaltyScale = params->penaltyScale;
		if (!is_unused(params->maxIters)) m_maxIters = params->maxIters;
		if (!is_unused(params->attachmentZone)) m_attachmentZone = params->attachmentZone;
		if (!is_unused(params->minSegLen)) m_minSegLen = params->minSegLen;
		if (!is_unused(params->unprojLimit)) m_unprojLimit = params->unprojLimit;
		if (!is_unused(params->noCollDist)) m_noCollDist = params->noCollDist;
		if (!is_unused(params->collisionBBox[0]))
			if ((params->collisionBBox[1]-params->collisionBBox[0]).len2()>0) {
				m_collBBox[0]=params->collisionBBox[0]; m_collBBox[1]=params->collisionBBox[1];
			} else
				MARK_UNUSED m_collBBox[0];
		if (!is_unused(params->nSegments)) {
			diff++;
			Vec3 pt[2]={ m_pos, m_pos-Vec3(0,0,m_length) };
			for(i=0;i<2;i++) if (m_pTiedTo[i]) {
				Vec3 pos; quaternionf q; float scale;
				m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], pos,q,scale);
				pt[i] = pos + q*m_ptTiedLoc[i];
			}
			if (m_nSegs!=params->nSegments) {
				if (m_segs) delete[] m_segs;
				m_segs = new rope_segment[params->nSegments+1];
				memset(m_segs, 0, (params->nSegments+1)*sizeof(rope_segment));
			}
			float rnSegs = 1.0f/params->nSegments;
			if (params->nSegments!=m_nSegs || m_pTiedTo[0] && m_pTiedTo[1]) {
				m_nSegs = params->nSegments;
				for(i=0;i<=params->nSegments;i++)
					m_segs[i].pt=m_segs[i].pt0 = pt[0]+(pt[1]-pt[0])*(i*rnSegs);
				if (m_flags & rope_subdivide_segs) {
					if (m_vtx) delete[] m_vtx;
					if (m_vtx1) delete[] m_vtx1;
					memset(m_vtx = new rope_vtx[m_nVtxAlloc=(params->nSegments+1)*2], 0, (params->nSegments+1)*2*sizeof(rope_vtx));
					memset(m_vtx1 = new rope_vtx[m_nVtxAlloc], 0, (params->nSegments+1)*2*sizeof(rope_vtx));
					for(i=0;i<=params->nSegments;i++)
						m_vtx[i].pt=m_vtx[i].pt0=m_segs[i].pt;
					m_nVtx=m_nVtx0 = params->nSegments+1;
				}
			}
		}
		if (!is_unused(params->pPoints) && params->pPoints) {
			for(i=0;i<=m_nSegs;i++) m_segs[i].pt = m_segs[i].pt0 = params->pPoints[i];
			m_nVtx=m_nVtx0=0; RecalcBBox();	++diff;
		}
		if (!is_unused(params->pVelocities) && params->pVelocities)	{
			for(i=0;i<=m_nSegs;i++) m_segs[i].vel = params->pVelocities[i];
			++diff;
		}

		if (m_idConstraint && m_pTiedTo[m_iConstraintClient]) {
			pe_action_update_constraint arc;
			arc.idConstraint=m_idConstraint; arc.bRemove=1;
			m_pTiedTo[m_iConstraintClient]->Action(&arc);
			m_idConstraint = 0;
		}
		//for(i=0;i<2;i++) if (m_pTiedTo[i] && m_pTiedTo[i]->m_iSimClass==7)
		//	m_pTiedTo[i] = 0;

		if (m_segs) for(i=0;i<2;i++) if (!is_unused(params->pEntTiedTo[i])) {
			if (m_pTiedTo[i]) m_pTiedTo[i]->Release();
			CPhysicalEntity *pPrevTied = m_pTiedTo[i];
			m_pTiedTo[i] = params->pEntTiedTo[i]==WORLD_ENTITY ? &g_StaticPhysicalEntity : 
										 (params->pEntTiedTo[i] ? ((CPhysicalPlaceholder*)params->pEntTiedTo[i])->GetEntity() : 0);
			if (m_pTiedTo[i]!=pPrevTied && pPrevTied)	{
				if (pPrevTied->GetType()==PE_ROPE)
					pPrevTied->RemoveCollider(this);
				if (!(m_flags & pef_disabled)) {
					if ((unsigned int)pPrevTied->m_iSimClass<7u)
						pPrevTied->Awake();
					if (m_pTiedTo[i^1] && (unsigned int)m_pTiedTo[i^1]->m_iSimClass<7u)
						m_pTiedTo[i^1]->Awake();
					diff++;
				}
			}
			//if (m_pTiedTo[i] && m_pTiedTo[i]->m_iDeletionTime>0)
			//	m_pTiedTo[i] = 0;
			if (m_pTiedTo[i]) {
				if (!is_unused(params->idPartTiedTo[i])) if (m_pTiedTo[i]->GetType()!=PE_ROPE) {
					for(m_iTiedPart[i]=0; m_iTiedPart[i]<m_pTiedTo[i]->m_nParts && 
							params->idPartTiedTo[i]!=m_pTiedTo[i]->m_parts[m_iTiedPart[i]].id; m_iTiedPart[i]++);
					if (m_iTiedPart[i]>=m_pTiedTo[i]->m_nParts)
						m_iTiedPart[i] = 0;
				}	else
					m_iTiedPart[i] = params->idPartTiedTo[i];
				Vec3 pos; quaternionf q; float scale;
				m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], pos,q,scale);
				if (!is_unused(params->ptTiedTo[i]))
					m_ptTiedLoc[i] = params->bLocalPtTied ? params->ptTiedTo[i] : (params->ptTiedTo[i]-pos)*q;
				else if (m_nSegs>0 && (!is_unused(params->pEntTiedTo[i]) || !is_unused(params->idPartTiedTo[i])))
					m_ptTiedLoc[i] = (m_segs[m_nSegs*i].pt-pos)*q;
				m_pTiedTo[i]->AddRef();
				if (m_pTiedTo[i]->GetType()==PE_ROPE)
					m_pTiedTo[i]->AddCollider(this);
				if (m_nSegs>0 && m_segs[i*m_nSegs].pContactEnt==m_pTiedTo[i]) {
					m_pTiedTo[i]->Release();
					m_segs[i*m_nSegs].pContactEnt = 0;
				}
			}
		}
		if (diff>1E-6f) {
			m_bAwake = 1;
			m_nSlowFrames = 0;
		}

		/*if (m_pTiedTo[0] && (m_pTiedTo[0]==this || m_pTiedTo[0]->m_iSimClass==3 || m_pTiedTo[0]->GetType()==PE_ROPE || m_pTiedTo[0]->GetType()==PE_SOFT) ||
				m_pTiedTo[1] && (m_pTiedTo[1]==this || m_pTiedTo[1]->m_iSimClass==3 || m_pTiedTo[1]->GetType()==PE_ROPE || m_pTiedTo[1]->GetType()==PE_SOFT))
		{	// rope cannot be attached to such objects
			if (m_pTiedTo[0]) m_pTiedTo[0]->Release();
			if (m_pTiedTo[1]) m_pTiedTo[1]->Release();
			m_pTiedTo[0] = 0; m_pTiedTo[1] = 0;
			return 0;
		}*/

		if (m_pTiedTo[0] && m_pTiedTo[1]) {
			pe_action_add_constraint aac;
			Vec3 pos; quaternionf q; float scale;
			m_iConstraintClient = isneg(m_pTiedTo[0]->GetMassInv()-m_pTiedTo[1]->GetMassInv());
			for(i=0;i<2;i++) {
				m_pTiedTo[m_iConstraintClient^i]->GetLocTransform(m_iTiedPart[m_iConstraintClient^i], pos,q,scale);
				aac.pt[i] = pos + q*m_ptTiedLoc[m_iConstraintClient^i];
				aac.partid[i] = m_pTiedTo[m_iConstraintClient^i]->m_parts[m_iTiedPart[m_iConstraintClient^i]].id;
			}
			aac.pBuddy = m_pTiedTo[m_iConstraintClient^1];
			aac.pConstraintEntity = this;
			aac.maxPullForce = m_maxForce;
			aac.damping = m_frictionPull;
			m_idConstraint = m_pTiedTo[m_iConstraintClient]->Action(&aac);
		}

		return 1;
	}

	return 0;
}
#endif//__SPU__

int CRopeEntity::GetParams(pe_params *_params) const
{
	int res;
	if (res = CPhysicalEntity::GetParams(_params))
		return res;

	if (_params->type==pe_simulation_params::type_id) {
		pe_simulation_params *params = (pe_simulation_params*)_params;
		params->gravity = m_gravity;
		params->damping = m_damping;
		params->maxTimeStep = m_maxAllowedStep;
		params->minEnergy = m_Emin;
		return 1;
	}

	if (_params->type==pe_params_rope::type_id) {
		pe_params_rope *params = (pe_params_rope*)_params;
		ReadLock lock(m_lockUpdate);
		params->length = m_length;
		params->mass = m_mass;
		params->nSegments = m_nSegs;
		params->collDist = m_collDist;
		params->surface_idx = m_surface_idx;
		params->friction = m_friction;
		params->stiffnessAnim = m_stiffnessAnim;
		params->stiffnessDecayAnim = m_stiffnessDecayAnim;
		params->dampingAnim = m_dampingAnim;
		params->bTargetPoseActive = m_bTargetPoseActive;
		params->wind = m_wind;
		params->windVariance = m_windVariance;
		params->airResistance = m_airResistance;
		params->waterResistance = m_waterResistance;
		params->density = 1.0f/m_rdensity;
		params->jointLimit = m_jointLimit; 
		params->sensorRadius = m_szSensor;
		params->maxForce = m_maxForce;
		params->flagsCollider = m_flagsCollider;
		params->nMaxSubVtx = m_nMaxSubVtx;
		params->frictionPull = m_frictionPull;
		params->stiffness = m_stiffness;
		params->penaltyScale = m_penaltyScale;
		params->maxIters = m_maxIters;
		params->attachmentZone = m_attachmentZone;
		params->minSegLen = m_minSegLen;
		params->unprojLimit = m_unprojLimit;
		params->noCollDist = m_noCollDist;
		params->pPoints = &m_segs[0].pt;
		params->pVelocities = &m_segs[0].vel;
		params->pPoints.iStride=params->pVelocities.iStride = sizeof(rope_segment);
		for(int i=0;i<2;i++) if (params->pEntTiedTo[i] = m_pTiedTo[i]) {
			if (m_pTiedTo[i]==&g_StaticPhysicalEntity)
				params->pEntTiedTo[i] = WORLD_ENTITY;
			params->idPartTiedTo[i] = m_pTiedTo[i]->m_parts[m_iTiedPart[i]].id;
			Vec3 pos; quaternionf q; float scale;
			m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], pos,q,scale);
			params->ptTiedTo[i] = pos + q*m_ptTiedLoc[i];
		}
		if (is_unused(m_collBBox[0]))
			params->collisionBBox[0]=params->collisionBBox[1].zero();
		else {
			params->collisionBBox[0] = m_collBBox[0];
			params->collisionBBox[1] = m_collBBox[1];
		}
		return 1;
	}

	return 0;
}


int CRopeEntity::GetStatus(pe_status *_status) const
{
	int res;
	if (res = CPhysicalEntity::GetStatus(_status)) {
		if (_status->type==pe_status_caps::type_id) {
			pe_status_caps *status = (pe_status_caps*)_status;
			status->bCanAlterOrientation = 1;
		}
		return res;
	}

	if (_status->type==pe_status_rope::type_id) {
		pe_status_rope *status = (pe_status_rope*)_status;
		ReadLock lock(m_lockUpdate);
		if (m_nSegs<0)
			return 0;
		status->nSegments = m_nSegs;
		int i;
		if (!is_unused(status->pPoints) && status->pPoints)
			for(i=0;i<=m_nSegs;i++) status->pPoints[i] = m_segs[i].pt0;
		if (!is_unused(status->pVelocities) && status->pVelocities)
			for(i=0;i<=m_nSegs;i++) status->pVelocities[i] = m_segs[i].vel;
		if (!is_unused(status->pContactNorms) && status->pContactNorms)
			for(i=0;i<m_nSegs;i++) if (m_segs[i].pContactEnt)
				status->pContactNorms[i] = m_segs[i].ncontact;
		int *nCollCount[2] = { &(status->nCollStat=0), &(status->nCollDyn=0) };
		volatile CPhysicalEntity *pContactEnt;
		for(i=0;i<m_nSegs;i++) if (pContactEnt = *(volatile CPhysicalEntity**)&m_segs[i].pContactEnt)
			(*nCollCount[-pContactEnt->m_iSimClass>>31 & 1])++;
		status->bTargetPoseActive = m_bTargetPoseActive;
		status->stiffnessAnim = m_stiffnessAnim;
		status->pContactEnts = (IPhysicalEntity**)&m_segs[0].pContactEnt;
		status->pContactEnts.iStride = sizeof(rope_segment);
		if (m_flags & rope_subdivide_segs) {
			ReadLock lockVtx(m_lockVtx);
			status->nVtx = m_nVtx0;
			if (status->pVtx) for(i=0;i<m_nVtx0;i++)
				status->pVtx[i] = m_vtx[i].pt0;
		} else
			status->nVtx = 0;
		status->timeLastActive = m_timeLastActive;
		status->posHost = m_lastposHost;
		status->qHost = m_lastqHost;
		return 1;
	}

	return 0;
}


void CRopeEntity::EnforceConstraints(float seglen, const quaternionf& qtv,const Vec3& offstv,float scaletv, int bTargetPoseActive)
{
	int i,iDir,iStart,iEnd,bHasContacts;

	if (!m_pTiedTo[0] || !m_pTiedTo[1]) {
		float a,len,angleSrc,angleDst;
		Vec3 dir0src,dir1src,dir0dst,dir1dst,axis0src,axis1src,axis1dst;

		if (m_flags & rope_subdivide_segs && m_bHasContacts)
			return;

		iDir = m_pTiedTo[1] && !m_pTiedTo[0] ? -1:1;
		iStart = m_nSegs & iDir>>31;
		iEnd = m_nSegs & -iDir>>31;
		if (bTargetPoseActive==0) for(i=iStart;i!=iEnd;i+=iDir)
			m_segs[i+iDir].pt = m_segs[i].pt+(m_segs[i+(iDir>>31)].dir=(m_segs[i+iDir].pt-m_segs[i].pt).normalized())*seglen;
		else if (m_jointLimit<=0) {
			for(i=iStart,m_length=0;i!=iEnd;i+=iDir) {
				len = (m_segs[i+iDir].ptdst-m_segs[i].ptdst).len()*scaletv;	m_length += len;
				m_segs[i+iDir].pt = m_segs[i].pt+(m_segs[i+(iDir>>31)].dir=(m_segs[i+iDir].pt-m_segs[i].pt).normalized())*len;
			}
		} else {
			dir0dst = qtv*(m_segs[iStart+iDir].ptdst-m_segs[iStart].ptdst);
			m_length=len = (m_segs[iStart+iDir].ptdst-m_segs[iStart].ptdst).len();
			dir0src = (m_segs[iStart+iDir].pt-m_segs[iStart].pt).normalized();
			axis0src = dir0src^dir0dst;
			angleSrc = atan2_tpl(a=axis0src.len(), dir0src*dir0dst);
			if (angleSrc>m_jointLimit && a>1e-20f)
				dir0src = dir0src.GetRotated(axis0src/a, angleSrc-m_jointLimit);
			m_segs[iStart+iDir].pt = m_segs[iStart].pt+(m_segs[iStart+(iDir>>31)].dir=dir0src)*(len*scaletv);

			for(i=iStart+iDir;i!=iEnd;i+=iDir) {
				len = (m_segs[i+iDir].ptdst-m_segs[i].ptdst).len();	m_length += len;
				m_segs[i+(iDir>>31)].dir = (m_segs[i+iDir].pt-m_segs[i].pt).normalized();
				dir0src = m_segs[i].pt-m_segs[i-iDir].pt; dir0dst = m_segs[i].ptdst-m_segs[i-iDir].ptdst;
				dir1src = m_segs[i+iDir].pt-m_segs[i].pt; dir1dst = m_segs[i+iDir].ptdst-m_segs[i].ptdst; 
				axis1src = dir0src^dir1src; axis1dst = dir0dst^dir1dst;
				angleSrc = atan2_tpl(a=axis1src.len(), dir0src*dir1src);
				angleDst = atan2_tpl(  axis1dst.len(), dir0dst*dir1dst);
				if (fabs_tpl(angleSrc-angleDst)>m_jointLimit && a>1e-20f)
					m_segs[i+(iDir>>31)].dir = m_segs[i+(iDir>>31)].dir.GetRotated(axis1src/a, angleDst-angleSrc-m_jointLimit*sgnnz(angleDst-angleSrc));
				m_segs[i+iDir].pt = m_segs[i].pt+m_segs[i+(iDir>>31)].dir*(len*scaletv);
			}
		}
	} else if (!(m_flags & rope_subdivide_segs) && m_nSegs) {
		Vec3 dir,ptend[2] = { m_segs[0].pt,m_segs[m_nSegs].pt };
		float diff,len2,seglen2=sqr(seglen),rseglen=1.0f/max(1e-10f,seglen),rseglen2=sqr(rseglen),k;

		bHasContacts = 100;
		iDir = m_pTiedTo[1] && !m_pTiedTo[0] ? 1:-1;
		do {
			iDir = -iDir;
			iStart = m_nSegs & iDir>>31;
			iEnd = m_nSegs & -iDir>>31;

			for(i=iStart;i!=iEnd;i+=iDir)	{
				dir = m_segs[i+iDir].pt-m_segs[i].pt; len2 = dir.len2(); 
				diff = fabs_tpl(len2-seglen2);
				if (bTargetPoseActive = (diff>seglen2*0.01f)) {
					if (diff<seglen2*0.1f) { // use 3 terms of 1/sqrt(x) Taylor series expansion
						k = (len2-seglen2)*rseglen2; 
						k = 1.0f+(-0.5f+(0.375f-0.3125f*k)*k)*k;
					} else
						k = seglen/sqrt_tpl(len2);
					m_segs[i+iDir].pt = m_segs[i].pt + dir*k;
					bHasContacts--;
				}	else k = 1.0f;
				m_segs[i+(iDir>>31)].dir = dir*(k*rseglen*iDir);
			} 

			if (m_pTiedTo[0]) m_segs[0].pt = ptend[0];
			if (m_pTiedTo[1]) m_segs[m_nSegs].pt = ptend[1];
		} while(m_pTiedTo[iDir+1>>1] && bTargetPoseActive && bHasContacts>0);
	}
}

#ifndef __SPU__
int CRopeEntity::Action(pe_action *_action, int bThreadSafe)
{
	if (_action->type==pe_action_notify::type_id) {
		WriteLock lock(m_lockStep);
		pe_action_notify *action = (pe_action_notify*)_action;
		if (action->iCode==pe_action_notify::ParentChange) {
			int i; Vec3 pos; quaternionf q; float scale;
			if ((!m_pTiedTo[0] || !m_pTiedTo[1]) && m_nSegs>0 && m_segs) { // if the rope is tied with at most one end, do exact length enforcement
				for(i=0;i<2;i++) if (m_pTiedTo[i]) {
					m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], pos,q,scale);
					m_segs[m_nSegs*i].pt=m_segs[m_nSegs*i].pt0 = pos + q*m_ptTiedLoc[i];
				}
				q.SetIdentity(); pos.zero(); scale=1.0f;
				if (m_flags & (rope_target_vtx_rel0 | rope_target_vtx_rel1)) {
					i = (m_flags & rope_target_vtx_rel1)!=0;
					if (m_pTiedTo[i]) {
						m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], pos,q,scale);
						if (m_bTargetPoseActive==2)
							m_ptTiedLoc[i] = m_segs[i*m_nSegs].ptdst;
					}
				}
				EnforceConstraints(m_length/m_nSegs, q,pos,scale, m_bTargetPoseActive);
				for(i=0;i<=m_nSegs;i++)
					m_segs[i].pt0 = m_segs[i].pt;
				if (!(m_flags & pef_disabled))	{
					m_bAwake=1; m_nSlowFrames=0;
				}
				/*iDir = m_pTiedTo[1] && !m_pTiedTo[0] ? -1:1;
				iStart = m_nSegs & iDir>>31;
				iEnd = m_nSegs & -iDir>>31;
				if (m_bTargetPoseActive==0) for(i=iStart,seglen=m_length/m_nSegs;i!=iEnd;i+=iDir)
					m_segs[i+iDir].pt0=m_segs[i+iDir].pt = m_segs[i].pt+(m_segs[i+(iDir>>31)].dir=(m_segs[i+iDir].pt-m_segs[i].pt).normalized())*seglen;
				else for(i=iStart;i!=iEnd;i+=iDir)
					m_segs[i+iDir].pt0=m_segs[i+iDir].pt = m_segs[i].pt+(m_segs[i+(iDir>>31)].dir=(m_segs[i+iDir].pt-m_segs[i].pt).normalized())*
					(m_segs[i+iDir].ptdst-m_segs[i].ptdst).len();*/
			}
		}
		return 1;
	}

	ChangeRequest<pe_action> req(this,m_pWorld,_action,bThreadSafe);
	if (req.IsQueued())
		return 1;

	int i,j;
	float t=0,rmass;
	if (_action->type==pe_action_impulse::type_id) {
		pe_action_impulse *action = (pe_action_impulse*)_action;
		if (!is_unused(action->ipart)) i = action->ipart;
		else if (!is_unused(action->partid)) i = action->partid;
		else if (!is_unused(action->point)) {
			for(j=1,i=0;j<=m_nSegs;j++) if ((m_segs[j].pt-action->point).len2()<(m_segs[i].pt-action->point).len2())
				i=j;
			if ((m_segs[i].pt-action->point).len2()>sqr(m_collDist*3) && 
					(i==0 || (m_segs[i].pt-m_segs[i-1].pt^action->point-m_segs[i-1].pt).len2() > sqr(m_collDist)*(m_segs[i].pt-m_segs[i-1].pt).len2()))
				return 0;
			if (i<m_nSegs)
				t = (action->point-m_segs[i].pt)*(m_segs[i+1].pt-m_segs[i].pt).normalized();
		} else
			i = m_nSegs>>1;
		if ((unsigned int)i>(unsigned int)m_nSegs)
			return 0;

		rmass = 1.0f/m_mass;
		m_segs[i].vel_ext += action->impulse*((1.0f-t)*(m_nSegs+1)*rmass);
		if (i<m_nSegs)
			m_segs[i+1].vel_ext += action->impulse*(t*(m_nSegs+1)*rmass);

		m_bAwake = 1; m_nSlowFrames = 0;
		m_timeIdle *= isneg(-action->iSource);
		for(i=0;i<m_nColliders;i++)
			m_pColliders[i]->Awake();
		return 1;
	}

	if (_action->type==pe_action_target_vtx::type_id) {
		pe_action_target_vtx *action = (pe_action_target_vtx*)_action;
		WriteLock lock(m_lockUpdate);
		if (!is_unused(action->points) && action->nPoints==m_nSegs+1) {
			int bFar = (m_segs[0].pt-action->points[0]).len2()>sqr(m_length*5);
			for(i=0;i<=m_nSegs;i++)	{
				m_segs[i].ptdst = action->points[i];
				if (!(m_flags & (rope_target_vtx_rel0|rope_target_vtx_rel1)) && (m_flags & pef_disabled || bFar)) 
					m_segs[i].pt=m_segs[i].pt0 = action->points[i];
			}
			if (m_bTargetPoseActive==1)	for(i=0;i<2;i++) if (m_pTiedTo[i]) {
				Vec3 pos; quaternionf q; float scale;
				m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], pos,q,scale);
				m_ptTiedLoc[i] = (action->points[m_nSegs*i]-pos)*q;
				if (i==0 && m_iTiedPart[0]<m_pTiedTo[0]->m_nParts)
					m_dir0dst = (action->points[1]-action->points[0])*q;
			}
		}	else if (m_flags & (rope_target_vtx_rel0|rope_target_vtx_rel1)) {
			int iParent = (m_flags & rope_target_vtx_rel1)!=0;
			Vec3 offsParent;
			quaternionf qParent;
			float scaleParent;
			if (m_pTiedTo[iParent]) {
				m_pTiedTo[iParent]->GetLocTransform(m_iTiedPart[iParent], offsParent,qParent,scaleParent);
				scaleParent = 1.0f/scaleParent;
				for(i=0;i<=m_nSegs;i++)
					m_segs[i].ptdst = (m_segs[i].pt-offsParent)*qParent*scaleParent;
			}
		}	else for(i=0;i<=m_nSegs;i++) 
			m_segs[i].ptdst = m_segs[i].pt;
		return 1;
	}

	if (_action->type==pe_action_reset::type_id) {
		if (!(m_flags & rope_subdivide_segs)) {
			for(i=0;i<=m_nSegs;i++) {
				m_segs[i].vel.zero();
				if (m_segs[i].pContactEnt) {
					m_segs[i].pContactEnt->Release();
					m_segs[i].pContactEnt = 0;
				}
			}
		}	else {
			for(i=0;i<2;i++) if (m_pTiedTo[i])
				m_pTiedTo[i]->RemoveCollider(this);
			for(i=1;i<m_nVtx-1;i++) if (m_vtx[i].pContactEnt)	{
				m_vtx[i].pContactEnt->RemoveCollider(this);
				m_vtx[i].pContactEnt = 0;
			}
			for(i=0;i<m_nColliders;i++)
				m_pColliders[i]->Release();
			for(i=0;i<m_nSegs;i++)
				m_segs[i].pContactEnt=0, m_segs[i].vel.zero();
			m_segs[m_nSegs].vel.zero();
			m_nColliders = 0;
		}
		m_bStrained = 0;
		return 1;
	}

	return CPhysicalEntity::Action(_action,1);
}


RigidBody *CRopeEntity::GetRigidBodyData(RigidBody *pbody, int ipart)
{ 
	pbody->pos = (m_segs[ipart].pt+m_segs[ipart+1].pt)*0.5f;
	Vec3 dir = m_segs[ipart+1].pt-m_segs[ipart].pt;
	pbody->q = Quat::CreateRotationV0V1(Vec3(0,0,-1),dir.normalized());
	pbody->v = (m_segs[ipart].vel+m_segs[ipart+1].vel)*0.5f;
	pbody->w = (dir ^ m_segs[ipart+1].vel-m_segs[ipart].vel)/dir.len2();
	pbody->M = m_mass/m_nSegs;
	pbody->Minv = 1.0f/pbody->M;
	return pbody;
}

void CRopeEntity::GetLocTransform(int ipart, Vec3 &offs, quaternionf &q, float &scale)
{
	scale = 1.0f;
	/*if (m_bTargetPoseActive) {
		q = Quat::CreateRotationV0V1((m_segs[ipart+1].pt-m_segs[ipart].pt).normalized(), (m_segs[ipart+1].ptdst-m_segs[ipart].ptdst).normalized());
		offs = m_segs[ipart].pt-m_segs[ipart].ptdst;
	}	else*/ {
		q = Quat::CreateRotationV0V1(Vec3(0,0,-1), (m_segs[ipart+1].pt-m_segs[ipart].pt).normalized());
		offs = m_segs[ipart].pt;
	}

	/*if (m_flags & (rope_target_vtx_rel0 | rope_target_vtx_rel1)) {
		int i = (m_flags & rope_target_vtx_rel1)!=0;
		Vec3 offsParent;
		quaternionf qParent;
		float scaleParent;
		if (m_pTiedTo[i]) {
			m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], offsParent,qParent,scaleParent);
			q = qParent*q;
			offs = qParent*offs*scaleParent + offsParent;
			scale *= scaleParent;
		}
	}*/
}


/*void CRopeEntity::MeshVtxUpdated()
{
	int i;
	box *pbox = &((CSingleBoxTree*)m_pMesh->m_pTree)->m_Box;
	Vec3 axisx,axisy,BBox[2]={Vec3(0),Vec3(0)};

	for(i=0;i<2;i++) m_pMesh->m_pNormals[i+2] = -(m_pMesh->m_pNormals[i] = 
		(m_pMesh->m_pVertices[m_pMesh->m_pIndices[i*3+1]]-m_pMesh->m_pVertices[m_pMesh->m_pIndices[i*3]] ^
		 m_pMesh->m_pVertices[m_pMesh->m_pIndices[i*3+2]]-m_pMesh->m_pVertices[m_pMesh->m_pIndices[i*3]]).normalized());

	axisx = m_pMesh->m_pVertices[1].normalized();
	axisy = m_pMesh->m_pVertices[2]+m_pMesh->m_pVertices[3]-m_pMesh->m_pVertices[1];
	(axisy -= axisx*(axisy*axisx)).normalize();
	pbox->Basis.SetFromVectors(axisx,axisy,axisx^axisy);
	for(i=1;i<4;i++) {
		BBox[0] = min(BBox[0],m_pMesh->m_pVertices[i]);
		BBox[1] = max(BBox[1],m_pMesh->m_pVertices[i]);
	}
	pbox->center = (BBox[1]+BBox[0])*0.5f;
	pbox->size = (BBox[1]-BBox[0])*0.5f;
	float e = max(max(pbox->size.x,pbox->size.y),pbox->size.z)*0.01f;
	for(i=0;i<3;i++) pbox->size[i] = max(pbox->size[i],e);
}*/

void CRopeEntity::AllocSubVtx()
{
	if (!m_vtx)	{
		m_vtx = new rope_vtx[m_nVtxAlloc=(m_nSegs+1)*2];
		m_vtx1 = new rope_vtx[m_nVtxAlloc];
	}
	if (m_nVtx==m_nVtxAlloc) {
		rope_vtx *pVtx = m_vtx;
		NO_BUFFER_OVERRUN
		memcpy(m_vtx=new rope_vtx[m_nVtxAlloc+=16], pVtx, m_nVtx*sizeof(rope_vtx));	delete[] pVtx;
		pVtx = m_vtx1;
		memcpy(m_vtx1=new rope_vtx[m_nVtxAlloc], pVtx, m_nVtx*sizeof(rope_vtx)); delete[] pVtx;
		if (m_vtxSolver) {
			rope_solver_vtx *pVtxSolver = m_vtxSolver;
			memcpy(m_vtxSolver=new rope_solver_vtx[m_nVtxAlloc], pVtxSolver, m_nVtx*sizeof(rope_solver_vtx));
			delete[] pVtxSolver;
		}
	}
}

void CRopeEntity::FillVtxContactData(rope_vtx *pvtx,int iseg, SRopeCheckPart &cp, geom_contact *pcontact)
{
	pvtx->pt = pcontact->pt-pcontact->dir*pcontact->t;
	if (iseg>=0) {
		float t = (pvtx->pt-m_segs[iseg].pt).len(); 
		t = t/(t+(pvtx->pt-m_segs[iseg+1].pt).len());
		pvtx->vel = m_segs[iseg].vel*(1-t)+m_segs[iseg+1].vel*t;
	}
	pvtx->ncontact = pcontact->n.normalized();
	pvtx->pt += pvtx->ncontact*m_collDist;
	RigidBody *pbody = cp.pent->GetRigidBody(cp.ipart);
	pvtx->vcontact = pbody->v+(pbody->w^pvtx->pt-pbody->pos);
	pvtx->pContactEnt = cp.pent;
	pvtx->iContactPart = cp.ipart;
}


void CRopeEntity::StartStep(float time_interval)
{
	m_timeStepPerformed = 0;
	m_timeStepFull = time_interval;
	m_posBody[0][0]=m_posBody[0][1]; m_qBody[0][0]=m_qBody[0][1];
	m_posBody[1][0]=m_posBody[1][1]; m_qBody[1][0]=m_qBody[1][1];
}


float CRopeEntity::GetMaxTimeStep(float time_interval)
{
	if (m_timeStepPerformed > m_timeStepFull-0.001f)
		return time_interval;
	return min(min(m_timeStepFull-m_timeStepPerformed,m_maxAllowedStep),time_interval);
}

int __ropeframe = 0;


void CRopeEntity::StepSubdivided(float time_interval, SRopeCheckPart *checkParts,int nCheckParts, float seglen)
{
	int i,j,iStart,iEnd,i1,j1,iter,iMoveEnd[2],ncont,bIncomplete[2],bStrainRechecked=0,bRopeChanged;
	Vec3 dv,center,dir,n;
	float tmax[2],a,b,len2;
	RigidBody *pbody;
	Vec3 dirUnproj[2] = { Vec3(ZERO),Vec3(ZERO) };
	rope_vtx vtxBest;
	intersection_params ip;
	geom_world_data gwd,gwd1;
	CRayGeom aray;
	geom_contact *pcontact;
	WriteLock lockVtx(m_lockVtx);

	if (m_idConstraint && m_pTiedTo[m_iConstraintClient]) {
		pe_action_update_constraint arc;
		arc.idConstraint=m_idConstraint; arc.bRemove=1;
		m_pTiedTo[m_iConstraintClient]->Action(&arc);
		m_idConstraint = 0;
	}

	for(i=0;i<2;i++) if (m_pTiedTo[i])
		m_pTiedTo[i]->RemoveCollider(this);
	if (m_bStrained) for(i=1;i<m_nVtx-1;i++) if (m_vtx[i].pContactEnt)
		m_vtx[i].pContactEnt->RemoveCollider(this);
	for(i=0;i<m_nColliders;i++)
		m_pColliders[i]->Release();
	m_nColliders = 0;

	if (!m_idx)
		m_idx = new int[m_nMaxSubVtx+2];

	RecheckAfterStrain:
	ip.bNoAreaContacts = true;
	ip.bStopAtFirstTri = false;
	ip.iUnprojectionMode = -1;
	ip.vrel_min = 1e-6f;
	aray.m_iCollPriority = 0;
	if (m_pTiedTo[0]) {
		iStart=1; MARK_UNUSED m_segs[0].ncontact; m_segs[0].pContactEnt=0;
	}	else iStart=0;
	if (m_pTiedTo[1]) {
		iEnd = m_nSegs-1; MARK_UNUSED m_segs[m_nSegs].ncontact; m_segs[m_nSegs].pContactEnt=0;
	} else iEnd=m_nSegs;
	bRopeChanged = isneg(sqr(m_collDist*0.1f)-max((m_segs[0].pt-m_segs[0].pt0).len2(), (m_segs[m_nSegs].pt-m_segs[m_nSegs].pt0).len2()));

	for(i=iStart; i<=iEnd; i++) {
		if (!m_segs[i].pContactEnt && !m_segs[i+1].pContactEnt && m_segs[i+1].iVtx0-m_segs[i].iVtx0>1)
			++bRopeChanged;
		if (m_segs[i].pContactEnt && !is_unused(m_segs[i].ncontact)) {
			++bRopeChanged;
			pbody = m_segs[i].pContactEnt->GetRigidBody(m_segs[i].iContactPart);
			dv = pbody->v + (pbody->w^m_segs[i].pt-pbody->pos);
			if ((m_segs[i].vel-dv)*m_segs[i].ncontact<m_pWorld->m_vars.minSeparationSpeed) {
				aray.m_ray.origin.zero(); gwd1.offset = m_segs[i].pt;
				aray.m_ray.dir = (aray.m_dirn=-m_segs[i].ncontact)*m_collDist*1.5f;
				gwd.offset = m_segs[i].pContactEnt->m_pos + m_segs[i].pContactEnt->m_qrot*m_segs[i].pContactEnt->m_parts[m_segs[i].iContactPart].pos;
				gwd.R = Matrix33(m_segs[i].pContactEnt->m_qrot*m_segs[i].pContactEnt->m_parts[m_segs[i].iContactPart].q);
				gwd.scale = m_segs[i].pContactEnt->m_parts[m_segs[i].iContactPart].scale;
				if (m_segs[i].pContactEnt->m_parts[m_segs[i].iContactPart].pPhysGeomProxy->pGeom->Intersect(&aray,&gwd,&gwd1,&ip,pcontact)) {
					WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
					m_segs[i].pt = pcontact->pt+(m_segs[i].ncontact = pcontact->n)*m_collDist;
					m_segs[i].vcontact = dv;
					bRopeChanged -= isneg((m_segs[i].pt-m_segs[i].pt0).len2()-sqr(m_collDist*0.1f));
				}	else { MARK_UNUSED m_segs[i].ncontact; m_segs[i].pContactEnt=0; }
			}	else { MARK_UNUSED m_segs[i].ncontact; m_segs[i].pContactEnt=0; }
		}	else { MARK_UNUSED m_segs[i].ncontact; m_segs[i].pContactEnt=0; }

		for(j=0;j<nCheckParts;j++) if (checkParts[j].pent->m_iSimClass!=3) {
			center = (checkParts[j].pent->m_qrot*!checkParts[j].q0)*(m_segs[i].pt0-checkParts[j].pos0) + checkParts[j].pent->m_pos;
			aray.m_ray.origin = (center-checkParts[j].offset)*checkParts[j].R;
			aray.m_ray.dir = (m_segs[i].pt-checkParts[j].offset)*checkParts[j].R-aray.m_ray.origin;
			if (aray.m_ray.dir.len2()==0)//<sqr(0.001f))
				continue;
			aray.m_ray.dir += (aray.m_dirn=aray.m_ray.dir.normalized())*m_collDist;
			if (box_ray_overlap_check(&checkParts[j].bbox,&aray.m_ray)) {
				gwd.offset.zero(); gwd.R.SetIdentity(); gwd.scale=checkParts[j].scale;
				if (ncont = checkParts[j].pGeom->Intersect(&aray,&gwd,0,&ip,pcontact)) {
					WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
					for(i1=ncont-1; i1>=0 && pcontact[i1].n*aray.m_dirn>0; i1--);
					if (i1>=0) {
						float gap = min(m_collDist/max(0.3f,-(pcontact[i1].n*aray.m_dirn)), aray.m_ray.dir*aray.m_dirn);
						m_segs[i].pt = checkParts[j].R*(pcontact[i1].pt-aray.m_dirn*gap)+checkParts[j].offset;
						m_segs[i].ncontact = checkParts[j].R*pcontact[i1].n;
						m_segs[i].pContactEnt = checkParts[j].pent;
						m_segs[i].iContactPart = checkParts[j].ipart;
						pbody = checkParts[j].pent->GetRigidBody(checkParts[j].ipart);
						m_segs[i].vcontact = pbody->v + (pbody->w ^ m_segs[i].pt-pbody->pos);
						++bRopeChanged;
					}
				}
			}
		} else {
			contact acontact;
			if (checkParts[j].pGeom->UnprojectSphere(((m_segs[i].pt-checkParts[j].offset)*checkParts[j].R)*checkParts[j].rscale, 
				m_collDist*checkParts[j].rscale, m_collDist*1.3f*checkParts[j].rscale, &acontact)) 
			{
				m_segs[i].pt = checkParts[j].R*(acontact.pt*checkParts[j].scale+acontact.n*m_collDist)+checkParts[j].offset;
				m_segs[i].ncontact = checkParts[j].R*acontact.n;
				m_segs[i].pContactEnt = checkParts[j].pent;
				m_segs[i].iContactPart = checkParts[j].ipart;
				pbody = checkParts[j].pent->GetRigidBody(checkParts[j].ipart);
				m_segs[i].vcontact = pbody->v + (pbody->w ^ m_segs[i].pt-pbody->pos);
				++bRopeChanged;
			}
		}
	}
	//if (m_bStrained && !bRopeChanged && m_penaltyScale==0.0f)
	//	goto stepdone;
	ip.iUnprojectionMode = 0;
	ip.vrel_min = 0;
	aray.m_iCollPriority = 10;

	for(i=m_nVtx=0; i<m_nSegs; i++) {
		AllocSubVtx();
		m_vtx[m_segs[i].iVtx0 = m_nVtx++].pt = m_segs[i].pt;
		m_vtx[m_segs[i].iVtx0].vel = m_segs[i].vel;
		m_vtx[m_segs[i].iVtx0].ncontact = m_segs[i].ncontact;
		m_vtx[m_segs[i].iVtx0].vcontact = m_segs[i].vcontact;
		m_vtx[m_segs[i].iVtx0].pContactEnt = m_segs[i].pContactEnt;
		m_vtx[m_segs[i].iVtx0].iContactPart = m_segs[i].iContactPart;
		AllocSubVtx(); m_nVtx++;
		AllocSubVtx(); m_nVtx++;

		for(j=0;j<nCheckParts;j++) {
			aray.m_ray.origin = (m_segs[i].pt-checkParts[j].offset)*checkParts[j].R;
			aray.m_ray.dir = (m_segs[i+1].pt-checkParts[j].offset)*checkParts[j].R-aray.m_ray.origin;
			checkParts[j].bProcess = box_ray_overlap_check(&checkParts[j].bbox,&aray.m_ray);
		}

		aray.m_ray.origin.zero();	gwd1.offset = m_segs[i].pt;
		m_segs[i].dir=aray.m_dirn = (aray.m_ray.dir = m_segs[i+1].pt-m_segs[i].pt).normalized();
		if (i==0 && m_pTiedTo[0])
			aray.m_ray.origin += (aray.m_ray.dir*=0.5f);
		else if (i==m_nSegs-1 && m_pTiedTo[1])
			aray.m_ray.dir *= 0.5f;
		if (m_segs[i].ptdst.len2()>0)
			gwd1.v = -m_segs[i].ptdst;
		else {
			gwd1.v = m_segs[max(0,i-1)].pt-m_segs[i].pt; gwd1.v -= aray.m_dirn*(aray.m_dirn*gwd1.v);
			dir = m_segs[min(m_nSegs,i+2)].pt-m_segs[i+1].pt; dir -= aray.m_dirn*(aray.m_dirn*dir);
			gwd1.v += dir;
			if (gwd1.v.len2()>sqr(seglen*0.1f))
				gwd1.v.normalize();
			else 
				gwd1.v.zero();
		}
		ip.time_interval=ip.maxUnproj = seglen*2;
		iMoveEnd[0]=iMoveEnd[1] = -1;

		for(i1=0,tmax[0]=tmax[1]=0,bIncomplete[0]=bIncomplete[1]=1; i1<2; i1++)	{
			for(j=0,dir.zero(); j<nCheckParts; j++) if (checkParts[j].bProcess) {
				gwd.offset = checkParts[j].offset; gwd.R = checkParts[j].R; gwd.scale = checkParts[j].scale;
				if (ncont=checkParts[j].pGeom->Intersect(&aray,&gwd,&gwd1,&ip,pcontact)) {
					WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
					if (pcontact->iUnprojMode==0 && pcontact->t>tmax[i1]+(real)0.00011) {
						bIncomplete[i1] = pcontact->n*pcontact->dir>0;
						if (bIncomplete[i1] && i1==1 && tmax[0]==0)
							continue;
						FillVtxContactData(m_vtx+m_nVtx-2+i1,i,checkParts[j],pcontact);
						tmax[i1] = pcontact->t; dirUnproj[i1] = pcontact->dir;
						if (!(pcontact->iFeature[1] & 0x20))
							iMoveEnd[i1] = min(1,pcontact->iFeature[1] & 0x1F);
					}
				}
			}
			if (gwd1.v.len2()==0)
				gwd1.v = dirUnproj[i1];
			gwd1.v.Flip();
		}
		if (tmax[0]+tmax[1]==0) {
			m_nVtx-=2; continue;
		}
		if (tmax[1]>0 && (tmax[1]<tmax[0]*0.5f && bIncomplete[0]==bIncomplete[1] || bIncomplete[0] && !bIncomplete[1]))	{
			m_vtx[m_nVtx-2]=m_vtx[m_nVtx-1]; iMoveEnd[0]=iMoveEnd[1]; 
			tmax[0]=tmax[1]; dirUnproj[0]=dirUnproj[1];
		}
		if (iMoveEnd[0]>=0)	{
			m_segs[i+iMoveEnd[0]].pt -= dirUnproj[0]*tmax[0];
			m_segs[i].dir = (m_segs[i+1].pt-m_segs[i].pt).normalized();
			m_vtx[m_nVtx-3].pt = m_segs[i].pt;
		}
		m_vtx[m_nVtx-1].pt = m_segs[i+1].pt;
		dir = dirUnproj[0];

		for(iter=0; m_nVtx-m_segs[i].iVtx0-2<m_nMaxSubVtx && iter<3; iter++) {
			for(i1=0; i1<m_nVtx-m_segs[i].iVtx0-1; i1++) m_idx[i1] = m_segs[i].iVtx0+i1;
			for(i1=0; i1<m_nVtx-m_segs[i].iVtx0-2; i1++) for(j1=m_nVtx-m_segs[i].iVtx0-2; j1>i1; j1--) 
				if ((m_vtx[m_idx[j1]+1].pt-m_vtx[m_idx[j1]].pt).len2() > (m_vtx[m_idx[j1-1]+1].pt-m_vtx[m_idx[j1-1]].pt).len2())
					j=m_idx[j1-1], m_idx[j1-1]=m_idx[j1], m_idx[j1]=j;
			for(i1=0; i1<m_nVtx-m_segs[i].iVtx0-1; i1++) {
				// unproject the next longest subseg until some unprojection is found
				aray.m_dirn = (aray.m_ray.dir = m_vtx[m_idx[i1]+1].pt-m_vtx[m_idx[i1]].pt).normalized();
				gwd1.offset = m_vtx[m_idx[i1]].pt;
				gwd1.v = ((aray.m_dirn^dir)^aray.m_dirn).normalized();
				for(j=0,tmax[0]=0; j<nCheckParts; j++) if (checkParts[j].bProcess) {
					gwd.offset = checkParts[j].offset; gwd.R = checkParts[j].R; gwd.scale = checkParts[j].scale;
					if (checkParts[j].pGeom->Intersect(&aray,&gwd,&gwd1,&ip,pcontact)) {
						WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
						float rng[2] = { seglen*(1.5f*isneg(i-iStart)-1), seglen*(2-1.5f*isneg(iEnd-1-i)) };
						if (pcontact->t>tmax[0] && inrange(m_segs[i].dir*(pcontact->pt-m_segs[i].pt),rng[0],rng[1])) 
							FillVtxContactData(&vtxBest,i,checkParts[j],pcontact), tmax[0]=pcontact->t;
					}
				}
				if (tmax[0]>0) {
					// if a new point is close to an existing one, replace the old one (but if the old one is an end, skip the new)
					j1 = m_idx[i1]+1;
					if ((m_vtx[m_idx[i1]].pt-vtxBest.pt).len2()<sqr(seglen*0.05f)) {
						if (m_idx[i1]==m_segs[i].iVtx0)
							continue;
						j1 = m_idx[i1];
					} else if ((m_vtx[m_idx[i1]+1].pt-vtxBest.pt).len2()>sqr(seglen*0.05f)) {
						AllocSubVtx(); m_nVtx++; iter=0;
						memmove(m_vtx+m_idx[i1]+2, m_vtx+m_idx[i1]+1, (m_nVtx-m_idx[i1]-2)*sizeof(rope_vtx));
					}	else if (m_idx[i1]==m_nVtx-2)
						continue;
					m_vtx[j1] = vtxBest;
					break;
				}
			}
			if (i1==m_nVtx-m_segs[i].iVtx0-1)
				break;
		}
		m_nVtx--;
	}
	m_vtx[m_segs[i].iVtx0 = m_nVtx++].pt = m_segs[i].pt;
	m_vtx[m_segs[i].iVtx0].vel = m_segs[i].vel;
	m_vtx[m_segs[i].iVtx0].ncontact = m_segs[i].ncontact;
	m_vtx[m_segs[i].iVtx0].vcontact = m_segs[i].vcontact;
	m_vtx[m_segs[i].iVtx0].pContactEnt = m_segs[i].pContactEnt;
	m_vtx[m_segs[i].iVtx0].iContactPart = m_segs[i].iContactPart;
	for(i=0;i<m_nVtx-1;i++)
		m_vtx[i].dir = (m_vtx[i+1].pt-m_vtx[i].pt).normalized();
	m_vtx[i].dir = m_vtx[max(0,i-1)].dir;
	for(i=0;i<m_nVtx;i++)
		m_bHasContacts |= !is_unused(m_vtx[i].ncontact);
	for(i=0;i<2;i++) if (m_pTiedTo[i]) {
		m_vtx[i*(m_nVtx-1)].pContactEnt = m_pTiedTo[i];
		m_vtx[i*(m_nVtx-1)].iContactPart = m_iTiedPart[i];
		m_vtx[i*(m_nVtx-1)].ncontact = m_vtx[i*(m_nVtx-1)].dir*(1-i*2);
	}

	if (!m_pTiedTo[0] || !m_pTiedTo[1])
		m_bStrained = 0;
	else {
		for(i=0,len2=0,m_bStrained=1; i<m_nVtx-1; ) {
			for(j=i++,b=0; b+=(m_vtx[i-1].pt-m_vtx[i].pt).len(), 
										 i<m_nVtx-1 && (is_unused(m_vtx[i].ncontact) || 
										 sqr_signed(m_vtx[i].ncontact*(m_vtx[i-1].dir-m_vtx[i].dir))<(m_vtx[i-1].dir-m_vtx[i].dir).len2()*0.01f); i++);
			len2 += a=(m_vtx[i].pt-m_vtx[j].pt).len();
			//m_bStrained &= isneg(min(1e-6f-m_length, b-a*1.05f));
			dir = m_vtx[i-1].dir-m_vtx[i].dir;
			if (dir.len2()>sqr(0.3f)) {
				Vec3 axis=m_vtx[i-1].dir^m_vtx[i].dir; n=m_vtx[i].ncontact;
				if ((a=axis.len2())>sqr(0.2f))
					n -= axis*(n*axis);
				else a=1.0f;
				if (i<m_nVtx-1 && sqr_signed(n*dir)*(1+sqr(m_friction))<dir.len2()*n.len2()*sqr(a))
					m_bStrained = 0;
			}
		}
		m_bStrained &= isneg(m_length-len2);
		if (m_bStrained && m_length>0) {
			for(i=1;i<m_nVtx-1;i++) if (!is_unused(m_vtx[i].ncontact) && m_vtx[i].ncontact*(m_vtx[i-1].dir-m_vtx[i].dir)<0.1f) {
				MARK_UNUSED m_vtx[i].ncontact; m_vtx[i].pContactEnt=0;	
			}
			for(a=0,i=j=ncont=0,seglen=len2/m_nSegs,iEnd=m_nVtx-1,m_nVtx=0; i<iEnd; ncont++) {
				for(iStart=i++; is_unused(m_vtx[i].ncontact) && i<iEnd; i++);
				dir = m_vtx[i].pt-m_vtx[iStart].pt; dir /= (b=dir.len());
				if (m_nVtx>0)
					m_vtx1[m_nVtx-1].dir = dir;
				for(; a<b+seglen*0.01f && j<=m_nSegs; m_nVtx++,a+=seglen) {
					AllocSubVtx();
					m_vtx1[m_nVtx].pt = m_vtx[iStart].pt+(m_vtx1[m_nVtx].dir=dir)*a; 
					MARK_UNUSED m_vtx1[m_nVtx].ncontact; m_vtx1[m_nVtx].pContactEnt=0;
					m_vtx1[m_nVtx].vel.zero();
					m_segs[j++].iVtx0 = m_nVtx;
				}
				m_nVtx -= isneg(seglen*0.98f-(a-=b));
				AllocSubVtx();
				m_vtx1[m_nVtx++] = m_vtx[i];
			}
			m_segs[m_nSegs].iVtx0 = m_nVtx-1;
			rope_vtx *pvtx=m_vtx; m_vtx=m_vtx1; m_vtx1=pvtx;
			for(i=1;i<m_nSegs;i++) {
				m_segs[i].ncontact = m_vtx[m_segs[i].iVtx0].ncontact;
				m_segs[i].pContactEnt = m_vtx[m_segs[i].iVtx0].pContactEnt;
				m_segs[i].iContactPart = m_vtx[m_segs[i].iVtx0].iContactPart;
			}
			if (ncont<m_nFragments && !bStrainRechecked)	{
				m_nFragments=ncont; bStrainRechecked=1; 
				for(i=0;i<=m_nSegs;i++)
					m_segs[i].pt0=m_segs[i].pt, m_segs[i].pt=m_vtx[m_segs[i].iVtx0].pt;
				goto RecheckAfterStrain;
			}
			m_nFragments = ncont;
		}
	}
	//stepdone:
	if (m_minSegLen>0) for(i=1;i<m_nSegs;i++) 
		if (m_segs[i+1].iVtx0==m_segs[i].iVtx0+1 && (m_segs[i+1].pt-m_segs[i].pt).len2()<sqr(m_minSegLen)) {
			for(j=i+1;j<=m_nSegs;j++) --m_segs[j].iVtx0;
			memmove(m_segs+i, m_segs+i+1, (m_nSegs-i)*sizeof(m_segs[0]));
			memmove(m_vtx+m_segs[i].iVtx0, m_vtx+m_segs[i].iVtx0+1, (m_nVtx-1-m_segs[i].iVtx0)*sizeof(m_vtx[0]));
			--m_nVtx; --m_nSegs;
		}
	if (m_bStrained) {
		m_pTiedTo[0]->AddCollider(this); AddCollider(m_pTiedTo[0]);
		m_pTiedTo[1]->AddCollider(this); AddCollider(m_pTiedTo[1]);
		for(i=1;i<m_nVtx-1;i++) if (m_vtx[i].pContactEnt)
			m_vtx[i].pContactEnt->AddCollider(this), AddCollider(m_vtx[i].pContactEnt);
		for(i=j=iter=0;i<m_nColliders;i++)
			j+=iszero(m_pColliders[i]->m_iSimClass-1), iter+=iszero(m_pColliders[i]->m_iSimClass-2);
		if (j>0 && iter==0)	{
			if (++m_nSleepingNeighboursFrames==3 && m_nSlowFrames==0)	
				for(i=0;i<m_nColliders;i++) if (m_pColliders[i]->m_iSimClass==1)
					m_pColliders[i]->Awake();
			} else
				m_nSleepingNeighboursFrames = 0;
		if (m_pWorld->m_bWorldStep!=2) {
			int nIndep=0,nRigid=0;
			for(i=0;i<m_nColliders;i++) {
				nIndep += (j = isneg(2-m_pColliders[i]->m_iSimClass));
				nRigid += isneg(-m_pColliders[i]->m_iSimClass)-j;
			}
			if (nIndep) {
				m_pTiedTo[0]->Awake(); m_pTiedTo[1]->Awake();
				if (!nRigid) {
					InitContactSolver(time_interval);
					RegisterContacts(time_interval,1);
					float Ebefore=m_energy, Eafter=0.0f, damping=1.0f;
					for(i=0;i<m_nColliders;i++) if (m_pColliders[i]->m_iSimClass>2)
						Ebefore += m_pColliders[i]->CalcEnergy(0.0f);
					entity_contact **pContacts; int nContacts=0;
					InvokeContactSolver(time_interval,&m_pWorld->m_vars,Ebefore, pContacts,nContacts);
					for(i=0;i<m_nColliders;i++) if (m_pColliders[i]->m_iSimClass>2)
						Eafter += m_pColliders[i]->CalcEnergy(0.0f);
					if (Eafter>Ebefore)
						damping = sqrt_tpl(Ebefore/Eafter);
					for(entity_contact *pContact=m_pContact; pContact; pContact=pContact->next)	for(j=0;j<2;j++) 
						if (pContact->pent[j]->m_parts[pContact->ipart[j]].flags & geom_monitor_contacts)
							pContact->pent[j]->OnContactResolved(pContact,j,-1);
					for(i=m_nColliders-1;i>=0;i--) if (m_pColliders[i]->m_iSimClass>2)
						m_pColliders[i]->Update(time_interval,damping);
				}
			}
			Update(time_interval,1.0f);
		}
	} else for(i=0;i<=m_nSegs;i++) if (m_segs[i].pContactEnt)
		m_segs[i].pContactEnt->m_flags |= pef_always_notify_on_deletion;

	for(i=0;i<m_nSegs;i++)
		m_segs[i].dir = (m_segs[i+1].pt-m_segs[i].pt).normalized();
	for(i=0;i<m_nVtx;i++)
		m_vtx[i].pt0 = m_vtx[i].pt;
	m_nVtx0 = m_nVtx;
}


void CRopeEntity::ZeroLengthStraighten(float time_interval)
{
	int i,j,iStart;
	float k,scale,seglen=0;
	scale = 1.0f/time_interval;
	if (m_flags & rope_subdivide_segs) {
		for(i=0,seglen=0;i<m_nVtx-1;i++)
			seglen += (m_vtx[i+1].pt-m_vtx[i].pt).len();
		seglen /= m_nSegs;
	}
	if (!m_bContactsRegistered)
		for(i=0;i<m_nVtx;i++) m_vtx[i].dP=1.0f;
	m_bContactsRegistered = 0;
	for(i=0;i<m_nVtx-1;) {
		for(j=i+1; j<m_nVtx-1 && (is_unused(m_vtx[j].ncontact) || ((m_vtx[j-1].dir-m_vtx[j].dir)*m_vtx[j].ncontact)*m_vtx[j].dP<0.1f); j++);
		if (j>i+1)
			k = 1.0f/(float)(j-i);
		for(iStart=i++; i<j; i++)
			m_vtx[i].vel = (m_vtx[iStart].pt+(m_vtx[j].pt-m_vtx[iStart].pt)*(i-iStart)*k-m_vtx[i].pt)*0.2f*scale;
	}
	for(i=1;i<m_nVtx-1;i++) {
		m_vtx[i+1].vcontact.x=m_vtx[i].vcontact.z= m_vtx[i].vel*m_vtx[i+1].vel;
		m_vtx[i].dP = (m_vtx[i].pt*2-m_vtx[i-1].pt-m_vtx[i+1].pt)*m_vtx[i].vel;
	}	// a=dv, b=, c=r, d=P
	k=-0.5f/m_vtx[1].vel.len2(); m_vtx[1].vcontact.z*=k; m_vtx[1].dP*=k;
	for(i=2;i<m_nVtx-1;i++) {
		k = (m_vtx[i].vel.len2()*-2.0f-m_vtx[i].vcontact.x*m_vtx[i-1].vcontact.z);
		k = fabs_tpl(k)>1e-15f ? 1.0f/k:0.0f;
		m_vtx[i].vcontact.z *= k; 
		m_vtx[i].dP = (m_vtx[i].dP-m_vtx[i].vcontact.x*m_vtx[i-1].dP)*k;
	}
	m_vtx[i=m_nVtx-2].vcontact.x = m_vtx[m_nVtx-2].dP;
	m_vtx[0].vcontact.x=m_vtx[m_nVtx-1].vcontact.x = 0;
	for(--i;i>0;i--)
		m_vtx[i].vcontact.x = m_vtx[i].dP-m_vtx[i].vcontact.z*m_vtx[i+1].vcontact.x;
	for(i=1;i<m_nVtx-1;m_vtx[i++].vcontact.zero()) if (time_interval>m_vtx[i].vcontact.x)
		m_vtx[i].vel *= max(0.0f,m_vtx[i].vcontact.x)*scale;
	for(i=1;i<m_nSegs;i++)
		m_segs[i].vel = m_vtx[m_segs[i].iVtx0].vel;
}
#endif//__SPU__

float CRopeEntity::Solver(float time_interval, float seglen)
{
	int i,j,iStart,iter;
	float E,Ebefore,k;

	if (m_length==0.0f && !(m_flags & rope_subdivide_segs)) {
		for(i=0;i<=m_nSegs;i++) m_segs[i].vel.zero();
		return 0.0f;
	}

	for(i=0,Ebefore=0;i<=m_nSegs;i++)
		Ebefore += m_segs[i].vel.len2();

	if (m_bHasContacts || m_flags & rope_subdivide_segs) {
		FRAME_PROFILER( "Rope solver MC",GetISystem(),PROFILE_PHYSICS );
		int bBounced; iter=m_maxIters;
		float vrel,dPtang,denom;
		Vec3 dp;

		if (!(m_flags & rope_subdivide_segs)) {
			for(i=0;i<m_nSegs;i++) 
				m_segs[i].kdP=0.5f, m_segs[i].dP=0;
			if (m_pTiedTo[0]) m_segs[0].kdP=0.0f;
			if (m_pTiedTo[1]) m_segs[m_nSegs-1].kdP=1.0f;

			do { // solve for velocities using relaxation solver with friction
				for(i=bBounced=0;i<m_nSegs;i++,iter--) {
					if (fabsf(vrel=(m_segs[i+1].vel-m_segs[i].vel)*m_segs[i].dir) > seglen*0.005f) {
						m_segs[i].vel += m_segs[i].dir*(vrel*m_segs[i].kdP);
						m_segs[i+1].vel -= m_segs[i].dir*(vrel*(1.0f-m_segs[i].kdP));
						bBounced++;
					}
					if (m_segs[i].pContactEnt) {
						dp = m_segs[i].vel*(1.0f-m_segs[i].tcontact)+m_segs[i+1].vel*m_segs[i].tcontact-m_segs[i].vcontact;
						if ((vrel=dp*m_segs[i].ncontact-m_segs[i].vreq) < -seglen*0.005f) {
							if (m_friction>0.01f) {
								m_segs[i].dP -= dPtang=sqrt_tpl(max(0.0001f,dp.len2()-sqr(vrel)));
								if (max(m_segs[i].dP-vrel*m_friction, 1e-10f-fabs_tpl(denom=m_segs[i].ncontact*dp)) < 0) {	// friction cannot stop sliding
									dp += (dp-m_segs[i].ncontact*vrel)*((m_segs[i].dP-vrel*m_friction)/dPtang); // remove part of dp that friction cannot stop
									dp *= vrel/denom; // apply impulse along dp so that it stops normal component
									m_segs[i].dP = 0;
								} else 
									m_segs[i].dP -= vrel*m_friction;
							} else
								dp = m_segs[i].ncontact*vrel;
							m_segs[i].vel -= dp*((1.0f-m_segs[i].tcontact)*m_segs[i].kdP);
							m_segs[i+1].vel -= dp*(m_segs[i].tcontact*(1.0f-m_segs[i].kdP));
							bBounced += 4;
						}
					}
				}
			} while (bBounced && (iter-=bBounced)>0);
		} 
/* //Diesel cut
#ifndef __SPU__
			else if (!m_bStrained || m_length==0) {
			float kdPs[3],e=m_pWorld->m_vars.accuracyMC,vreq;
			kdPs[0] = m_pTiedTo[0] ? 0.0f : 0.5f;
			kdPs[2] = m_pTiedTo[1] ? 1.0f : 0.5f;
			kdPs[1] = 0.5f;
			if (m_length==0) {
				for(i=1;i<m_nVtx-1;i++) {
					m_vtx[i].vel += m_vtx[i].dir*m_stiffness;
					m_vtx[i+1].vel -= m_vtx[i].dir*m_stiffness;
					if (!is_unused(m_vtx[i].ncontact) && (k=m_vtx[i].ncontact*(m_vtx[i].vel-m_vtx[i].vcontact))<m_pWorld->m_vars.minSeparationSpeed)
						m_vtx[i].vel -= m_vtx[i].ncontact*k;
				}
				Ebefore = 1e10f;
			}	else {
				float ks = m_stiffness/m_length;
				for(i=0;i<m_nSegs;i++) {
					for(j=m_segs[i].iVtx0,k=0;j<m_segs[i+1].iVtx0;j++) 
						k += m_vtx[j].dir*(m_vtx[j+1].pt-m_vtx[j].pt);
					m_segs[i].vreq = ks*(m_length-min(m_length*1.5f,k*m_nSegs));
				}
				for(i=0;i<m_nSegs;i++) {
					Ebefore += sqr(m_segs[i].vreq*((m_segs[i+1].pt-m_segs[i].pt)*m_segs[i].dir))+m_segs[i].vel.len()*fabs_tpl(m_segs[i].vreq)*2;
					m_vtx[i].dP=0;
				}

				do {
					for(i=bBounced=iStart=0; i<m_nVtx; i++,iter--) {
						if (i<m_nVtx-1) {
							vreq = ((m_vtx[i+1].pt-m_vtx[i].pt)*m_vtx[i].dir)*m_segs[iStart].vreq;
							iStart += isneg(m_segs[iStart+1].iVtx0-2-i);
							if (fabs_tpl(vrel=(m_vtx[i+1].vel-m_vtx[i].vel)*m_vtx[i].dir-vreq) > e) {
								j = i-1>>31 | max(0,i-m_nVtx+3);
								m_vtx[i].vel   += m_vtx[i].dir*(vrel*kdPs[j+1]);
								m_vtx[i+1].vel -= m_vtx[i].dir*(vrel*(1.0f-kdPs[j+1]));
								bBounced++;
							}
						}
						if (!is_unused(m_vtx[i].ncontact)) {
							dp = m_vtx[i].vel-m_vtx[i].vcontact;
							if ((vrel=dp*m_vtx[i].ncontact) < -e) {
								if (m_friction>0.01f) {
									m_vtx[i].dP -= dPtang=sqrt_tpl(max(0.0f,dp.len2()-sqr(vrel)));
									if (m_vtx[i].dP-vrel*m_friction < 0) {	// friction cannot stop sliding
										dp += (dp-m_vtx[i].ncontact*vrel)*((m_vtx[i].dP-vrel*m_friction)/dPtang); // remove part of dp that friction cannot stop
										dp *= vrel/(m_vtx[i].ncontact*dp); // apply impulse along dp so that it stops normal component
										m_vtx[i].dP = 0;
									} else 
										m_vtx[i].dP -= vrel*m_friction;
								} else
									dp = m_vtx[i].ncontact*vrel;
								m_vtx[i].vel -= dp;
								bBounced += 4;
							}
						}
					}
				} while (bBounced && (iter-=bBounced)>0);
			}

			if (m_vtx) for(i=0;i<=m_nSegs;i++)
				m_segs[i].vel = m_vtx[m_segs[i].iVtx0].vel;
		}
#endif//__SPU__
*/ //Diesel cut
	}	else {
		FRAME_PROFILER( "Rope solver CG",GetISystem(),PROFILE_PHYSICS );
		m_segs[0].vcontact.x = 0;
		m_segs[0].vcontact.y = m_segs[1].dir*m_segs[0].dir;
		m_segs[0].vcontact.z = (m_segs[0].vel-m_segs[1].vel)*m_segs[0].dir;
		if (m_pTiedTo[0])
			m_segs[0].vcontact *= 2.0f;
		for(i=1;i<m_nSegs;i++) {
			m_segs[i].vcontact.x = m_segs[i-1].dir*m_segs[i].dir;	// a
			m_segs[i].vcontact.y = m_segs[i+1].dir*m_segs[i].dir;	// c
			m_segs[i].vcontact.z = (m_segs[i].vel-m_segs[i+1].vel)*m_segs[i].dir;	// d; b=-2 for all i
		}
		m_segs[m_nSegs-1].vcontact.y = 0;
		if (m_pTiedTo[1])
			m_segs[m_nSegs-1].vcontact *= 2.0f;
		for(i=1,m_segs[0].vcontact*=-0.5f; i<m_nSegs; i++) {
			k = -2.0f-m_segs[i].vcontact.x*m_segs[i-1].vcontact.y;
			k = fabs_tpl(k)>1e-10f ? 1.0f/k : 0.0f;
			m_segs[i].vcontact.y *= k;
			(m_segs[i].vcontact.z -= m_segs[i-1].vcontact.z*m_segs[i].vcontact.x) *= k;
		}
		m_segs[m_nSegs-1].dP = m_segs[m_nSegs-1].vcontact.z;
		for(i=m_nSegs-2;i>=0;i--)
			m_segs[i].dP = m_segs[i].vcontact.z-m_segs[i].vcontact.y*m_segs[i+1].dP;

		if (!m_pTiedTo[0]) m_segs[0].vel += m_segs[0].dir*m_segs[0].dP;
		if (!m_pTiedTo[1]) m_segs[m_nSegs].vel -= m_segs[m_nSegs-1].dir*m_segs[m_nSegs-1].dP;
		m_segs[1].vel -= m_segs[0].dir*m_segs[0].dP;
		m_segs[m_nSegs-1].vel += m_segs[m_nSegs-1].dir*m_segs[m_nSegs-1].dP;
		for(i=1;i<m_nSegs-1;i++) {
			m_segs[i].vel += m_segs[i].dir*m_segs[i].dP;
			m_segs[i+1].vel -= m_segs[i].dir*m_segs[i].dP;
		}
	}

	if (m_length>0 || m_nVtx==0) {
		for(i=0,E=0;i<=m_nSegs;i++) E += m_segs[i].vel.len2();
		if (E>Ebefore && E>m_Emin) {
			k = sqrt_tpl(Ebefore/E);
			for(i=0;i<=m_nSegs;i++) m_segs[i].vel*=k;
			E = Ebefore;
		}
	} else {
		for(i=0,E=0;i<=m_nSegs;i++) E += (m_segs[i].pt-m_segs[i].pt0).len2();
		E /= sqr(time_interval);
	}
	return E;
}


void CRopeEntity::ApplyStiffness(float time_interval, int bTargetPoseActive, const quaternionf &qtv,const Vec3 &offstv,float scaletv)
{
	int i;
	Vec3 dv,dw,dir0dst,dir1dst,dir0src,dir1src,axis0src,axis1src,axis0dst,axis1dst,vrel;
	float a,b,angleSrc,angleDst,rnSegs=1.0f/m_nSegs;

	if (bTargetPoseActive==1) {
		for(i=0; i<m_nSegs; i++) {
			dv = (qtv*m_segs[i+1].ptdst*scaletv+offstv-m_segs[i+1].pt)*m_stiffnessAnim*(1-m_stiffnessDecayAnim*(i+1)*rnSegs);
			a = max(0.0f, 1-m_dampingAnim*time_interval);
			m_segs[i+1].vel = m_segs[i+1].vel*a + dv*(1-a);
		}
	}	else if (bTargetPoseActive==2) {
		// dir0src, dir0dst - point to attached object's center
		// axis0src=axis0dst = some axis perpendicular to dir0src
		if (m_stiffnessAnim>0) {
			if (m_pTiedTo[0])	{
				dir0dst = (m_segs[0].pt-offstv).normalized();
				dir1dst = qtv*(m_segs[1].ptdst-m_segs[0].ptdst);
				if (sqr(dir0dst*dir1dst)>dir1dst.len2()*sqr(0.985f))
					dir0dst.zero()[idxmin3(dir1dst.abs())] = 1;
				dir0src = dir0dst;
				axis0src=axis0dst = qtv*(!qtv*dir0src).GetOrthogonal().normalized();
			} else {
				dir0src.Set(0,0,1); dir0dst = dir0src;
				axis0src.Set(0,1,0); axis0dst = axis0src;
			}

			for(i=0; i<m_nSegs; i++,dir0src=dir1src,dir0dst=dir1dst,axis0src=axis1src,axis0dst=axis1dst) {
				dir1src = (m_segs[i+1].pt-m_segs[i].pt).normalized(); 
				dir1dst = qtv*(m_segs[i+1].ptdst-m_segs[i].ptdst).normalized();
				axis1src = dir0src^dir1src; axis1dst = dir0dst^dir1dst;
				angleSrc = atan2_tpl(a=axis1src.len(), dir0src*dir1src);
				angleDst = atan2_tpl(b=axis1dst.len(), dir0dst*dir1dst);
				// rotate around axis1src to bring angleSrc to angleDst
				axis1src = a > m_length*0.001f ? axis1src/a : axis0src; 
				axis1dst = b > m_length*0.001f ? axis1dst/b : axis0dst;
				if (fabs_tpl(angleDst-angleSrc)>g_PI)
					angleDst -= 2*sgnnz(angleDst-angleSrc)*g_PI;
				dw = axis1src*(angleDst-angleSrc);
				// rotate around dir0src to match (axis1src wrt axis0src) to (axis1dst wrt axis0dst)
				angleSrc = atan2_tpl((axis1src^axis0src)*dir0src, axis0src*axis1src-(axis0src*dir0src)*(axis1src*dir0src));
				angleDst = atan2_tpl((axis1dst^axis0dst)*dir0dst, axis0dst*axis1dst-(axis0dst*dir0dst)*(axis1dst*dir0dst));
				if (fabs_tpl(angleDst-angleSrc)>g_PI)
					angleDst -= 2*sgnnz(angleDst-angleSrc)*g_PI;
				dw -= dir0src*(angleDst-angleSrc);
				// update m_segs[i+1].vel
				dv = dw*(m_stiffnessAnim*(1-m_stiffnessDecayAnim*(i+1)*rnSegs)*time_interval) ^ m_segs[i+1].pt-m_segs[i].pt;
				vrel = m_segs[i+1].vel-m_segs[i].vel;
				m_segs[i+1].vel = m_segs[i].vel + vrel*max(0.0f,1.0f-m_dampingAnim*time_interval) + dv;
			}
		} else for(i=0;i<=m_nSegs;i++)
			m_segs[i].pt = qtv*m_segs[i].ptdst*scaletv+offstv;
	}
}


void CRopeEntity::CheckCollisions(int iDir, SRopeCheckPart *checkParts,int nCheckParts, float seglen,float rseglen)
{
	int i,j,iseg,iStart,iEnd,iend;
	Vec3 n,rotax,ptres[2];
	float t,angle,dist2,diff,cost,sint;
	geom_world_data gwd;
	geom_contact *pcontact;
	CRayGeom aray; aray.m_iCollPriority = 10;
	CPhysicalEntity *pent;
	INT_PTR mask = -isneg(m_jobE);
	intersection_params ip;
	ip.bStopAtFirstTri = true;
	ip.iUnprojectionMode = 1;

	iDir = 1-iDir*2;
	iStart = m_nSegs & iDir>>31;
	iEnd = m_nSegs & -iDir>>31;
	for(i=iStart;i!=iEnd;i+=iDir)	{
		iseg = i+(iDir>>31);
		if (pent=m_segs[iseg].pContactEnt) {
			gwd.R = checkParts[m_segs[iseg].iCheckPart].R;
			gwd.offset = checkParts[m_segs[iseg].iCheckPart].offset;
			gwd.scale = checkParts[m_segs[iseg].iCheckPart].scale;
			t = iszero(i-iStart)*m_noCollDist; // don't check the first half of the first segment
			if (!m_segs[iseg].bRecheckContact &&
					checkParts[m_segs[iseg].iCheckPart].pGeom->FindClosestPoint(&gwd, m_segs[iseg].iPrim,m_segs[iseg].iFeature, 
						m_segs[i+iDir].pt,m_segs[i].pt+(m_segs[i+iDir].pt-m_segs[i].pt)*t, ptres)>=0 && 
					(dist2=(n=ptres[1]-ptres[0]).len2())<sqr(m_collDist*2) && // drop contact when gap becomes too big
					sqr_signed(m_segs[iseg].vreq=n*m_segs[iseg].ncontact)>n.len2()*sqr(0.75f)) // drop contact if normal changes abruptly
			{
				n.normalize();
				m_segs[i].tcontact = (ptres[1]-m_segs[iseg].pt).len()*rseglen;
				m_segs[i].tcontact = min(1.0f,max(0.0f,m_segs[i].tcontact));
				if (dist2<sqr(m_collDist*0.85f)) { // reinforce the distance
					t = (iDir>>31&1) + m_segs[i].tcontact*iDir; // 1.0-t for iDir==-1
					j = isneg(t-0.1f); t += j; j = -j & iDir;
					if ((unsigned int)i-j<=(unsigned int)m_nSegs) {
						rotax = (m_segs[i+iDir-j].pt-m_segs[i-j].pt^n).normalized();
						angle = (m_collDist-sqrt_tpl(dist2))/max(1e-8f, t*seglen);
						m_segs[i+iDir-j].pt = m_segs[i+iDir-j].pt.GetRotated(m_segs[i-j].pt, rotax, cos_tpl(angle),sin_tpl(angle));
						m_segs[i+iDir-j].bRecalcDir = m_segs[max(0,i+iDir-j-1)].bRecalcDir = 1;
					}
					m_segs[iseg].vreq = 0;	 
				} else
					m_segs[iseg].vreq = (m_collDist-m_segs[iseg].vreq)*10.0f;
				m_segs[i].ncontact = n;
				m_segs[iseg].vcontact = checkParts[m_segs[iseg].iCheckPart].v+(checkParts[m_segs[iseg].iCheckPart].w^ptres[0]);
				m_bHasContacts = 1;
			}	else
				m_segs[iseg].pContactEnt = 0;
		}

		if (!m_segs[iseg].pContactEnt || m_segs[iseg].tcontact<0.1f && m_nSegs<=8) for(j=0;j<nCheckParts;j++) {
			aray.m_ray.origin = (m_segs[i].pt-checkParts[j].offset)*checkParts[j].R;
			aray.m_dirn = aray.m_ray.dir = (m_segs[i+iDir].pt-checkParts[j].offset)*checkParts[j].R-aray.m_ray.origin;
			if (box_ray_overlap_check(&checkParts[j].bbox,&aray.m_ray)) {
				ip.centerOfRotation = aray.m_ray.origin; ip.axisOfRotation.zero();
				rotax = aray.m_ray.dir*(iszero(i-iStart)*m_noCollDist);
				aray.m_ray.origin += rotax; aray.m_ray.dir -= rotax;
				gwd.offset.zero(); gwd.R.SetIdentity(); gwd.scale=checkParts[j].scale;
				if (checkParts[j].pGeom->Intersect(&aray,&gwd,0,&ip,pcontact)) {
					WriteLockCond lockColl(*ip.plock,0); lockColl.SetActive();
					aray.m_ray.origin -= rotax;
					if (pcontact->iUnprojMode==1) {// && (pcontact->pt-aray.m_ray.origin)*pcontact->n<0) {
						diff = m_segs[iseg].tcontact = (pcontact->pt-aray.m_ray.origin).len();
						m_segs[iseg].tcontact = m_segs[iseg].tcontact*iDir-seglen*(iDir>>31); // flip tcontact when iDir is -1
						iend = -iseg>>31 & 1;
						m_segs[iseg].pContactEnt = checkParts[j].pent;
						m_segs[iseg].iContactPart = checkParts[j].ipart;
						m_segs[iseg].iCheckPart = j;
						rotax = checkParts[j].R*-pcontact->dir;
						pcontact->t += min((float)pcontact->t, m_collDist/diff);
						if (pcontact->t>(real)m_unprojLimit) {
							pcontact->t=(real)m_unprojLimit; m_segs[iseg].bRecheckContact=1;
						} else
							m_segs[iseg].bRecheckContact=0;
						cost = cos_tpl(pcontact->t); sint = sin_tpl(pcontact->t);
						m_segs[iseg].ncontact = checkParts[j].R*(pcontact->n.normalized().GetRotated(pcontact->dir,cost,-sint));
						m_segs[iseg].tcontact	*= rseglen;
						m_segs[iseg].tcontact = min(1.0f,max(0.0f,m_segs[iseg].tcontact));
						m_segs[i+iDir].pt = m_segs[i+iDir].pt.GetRotated(m_segs[i].pt, rotax, cost,sint);
						//m_segs[iseg].bRecheckContact = isnonneg((m_segs[i+iDir].pt-m_segs[i].pt)*m_segs[iseg].ncontact);
						m_segs[iseg].iPrim = pcontact->iPrim[0];
						m_segs[iseg].iFeature = pcontact->iFeature[0];
						m_segs[iseg].vcontact = checkParts[j].v+(checkParts[j].w^pcontact->pt);
						m_segs[i+iDir].bRecalcDir = m_segs[max(0,i+iDir-1)].bRecalcDir = 1;
						m_segs[iseg].vreq = 0;
						m_bHasContacts = 1;
					}
				}
			}
		}

#ifndef __SPU__
		if (((INT_PTR)pent & mask)!=((INT_PTR)m_segs[iseg].pContactEnt & mask)) {
			if (pent) pent->Release();
			if (m_segs[iseg].pContactEnt) m_segs[iseg].pContactEnt->AddRef();
		}
#endif
	}

	for(i=0;i<m_nSegs;i++) if (m_segs[i].bRecalcDir)
		m_segs[i].dir = (m_segs[i+1].pt-m_segs[i].pt).normalized();
}

#ifndef __SPU__
int CRopeEntity::Step(float time_interval)
{
	if (m_nSegs<=0 || !m_bAwake)
		return 1;

	FUNCTION_PROFILER( GetISystem(),PROFILE_PHYSICS );
	PHYS_ENTITY_PROFILER

__ropeframe++;

	
	float seglen=m_length/m_nSegs,seglen2=sqr(seglen), rseglen=m_nSegs/max(1e-6f,m_length),rseglen2=sqr(rseglen),scale; 
	int i,j,iDir,iEnd,iter,bTargetPoseActive=m_bTargetPoseActive,bGridLocked=0,bHasContacts=0,nCheckParts=0;
	int flags = m_flags;
	Vec3 pos,gravity,dir,ptend[2],sz,BBox[2],ptnew,dv,dw,vrel,dir0,offstv(ZERO),collBBox[2];
	float diff,a,b,E,rnSegs=1.0f/m_nSegs,rcollDist=0,scaletv=1.0f,
				damping=max(0.0f,1.0f-(m_damping-m_dampingAnim*(m_bTargetPoseActive-1>>31))*time_interval);
	quaternionf dq,qtv(1,0,0,0),q;
	RigidBody *pbody;
	pe_params_buoyancy pb[4];
	SRopeCheckPart checkParts[32];
	EventPhysPostStep event;
	WriteLock lockst(m_lockStep);

	if (m_jobE>=0) {
		E=m_jobE; m_jobE=-1.0f; 
		for(i=1,BBox[0]=BBox[1]=m_segs[0].pt;i<=m_nSegs;i++) {
			BBox[0] = min(BBox[0], m_segs[i].pt);
			BBox[1] = max(BBox[1], m_segs[i].pt);
		}
		goto StepDone;
	}

	if (m_flags & (rope_collides|rope_collides_with_terrain))
		for(i=0;i<=m_nSegs;i++)	if (m_segs[i].pContactEnt && (m_segs[i].pContactEnt->m_iSimClass==7 ||
			m_segs[i].iContactPart>=m_segs[i].pContactEnt->m_nParts)) 
		{
			if (!(m_flags & rope_subdivide_segs)) 
				m_segs[i].pContactEnt->Release();
			m_segs[i].pContactEnt=0; MARK_UNUSED m_segs[i].ncontact;
		}
	for(i=j=0;i<2;i++) 
		if (m_pTiedTo[i] && ((unsigned int)m_pTiedTo[i]->m_iSimClass>=7u || 
				(unsigned int)m_pTiedTo[i]->m_iSimClass-1u<2u && m_iTiedPart[i]>=m_pTiedTo[i]->m_nParts))
		j |= 1<<i;
	if (j) {
		pe_params_rope pr;
		if (j&1) pr.pEntTiedTo[0]=0;
		if (j&2) pr.pEntTiedTo[1]=0;
		SetParams(&pr);
	}

	if (m_flags & pef_disabled)	{
		if (m_bTargetPoseActive==2 && m_pTiedTo[0])	{
			m_pTiedTo[0]->GetLocTransform(m_iTiedPart[0], dir,q,scale);
			m_segs[0].pt = dir + q*m_ptTiedLoc[0];
			m_BBox[0] = min(m_BBox[0],m_segs[0].pt);
			m_BBox[1] = max(m_BBox[1],m_segs[0].pt);
		}
		return 1;
	}
	if (m_pWorld->m_bWorldStep!=2) {
		if (m_timeStepPerformed>m_timeStepFull-0.001f || m_timeStepPerformed>0 && (m_flags & pef_invisible))
			return 1;
	}	else if (time_interval>0)
		time_interval = max(0.001f, min(m_timeStepFull-m_timeStepPerformed, time_interval));
	else
		return 1;

	m_timeStepPerformed += time_interval;
	m_lastTimeStep = time_interval;
	m_timeLastActive = m_pWorld->m_timePhysics;

	if ((iEnd=m_pWorld->CheckAreas(this,dir,pb,4)) && !is_unused(dir))
		m_gravity = (dir-m_pWorld->m_vars.gravity).len2() < dir.len2()*sqr(0.01) ? m_gravity0 : dir;
	gravity = m_gravity;

	event.pEntity=this; event.pForeignData=m_pForeignData; event.iForeignData=m_iForeignData;
	event.dt=time_interval; event.pos=m_pos; event.q=m_qrot; event.idStep=m_pWorld->m_idStep;
	m_pWorld->OnEvent(m_flags,&event);
	for(i=0,dir=m_wind;i<iEnd;i++)
		dir += pb[i].waterFlow*pb[i].iMedium;
	if (m_flags & rope_no_stiffness_when_colliding) {
		iter = m_pTiedTo[0] ? m_pTiedTo[0]->m_id : (m_pTiedTo[1] ? m_pTiedTo[1]->m_id : -10);
		for(i=bHasContacts=0;i<=m_nSegs;i++) if (m_segs[i].pContactEnt)
			bHasContacts |= m_segs[i].pContactEnt->m_id-iter>>31 | iter-m_segs[i].pContactEnt->m_id>>31;
		if (bHasContacts)
			bTargetPoseActive = 0;
	}

	if ((m_windTimer+=time_interval*4)>1.0f) {
		m_windTimer = 0; m_wind0 = m_wind1;
		a = m_windVariance*(fabs_tpl(dir.x)+fabs_tpl(dir.y)+fabs_tpl(dir.z));
		m_wind1 = dir+Vec3(physics_frand(a)-a*0.5f,physics_frand(a)-a*0.5f,physics_frand(a)-a*0.5f);
	}
	dw = m_wind0*m_windTimer+m_wind1*(1.0f-m_windTimer);
	if (bTargetPoseActive && m_segs[0].pContactEnt && m_segs[0].tcontact<0.2f) {
		//gravity.zero(); dw.zero(); 
		bTargetPoseActive=0;
		//for(i=0;i<=m_nSegs;i++) m_segs[i].vel.zero();
	}
	if (m_length==0 && m_nVtx>0)
		ZeroLengthStraighten(time_interval);

	for(i=0;i<=m_nSegs;i++)	{
		m_segs[i].pt0 = m_segs[i].pt;
		ptnew = m_segs[i].pt + m_segs[i].vel*time_interval;
		if (bTargetPoseActive==1 && (m_stiffnessAnim==0 || (m_segs[i].pt-m_segs[i].ptdst)*(ptnew-m_segs[i].ptdst)<0 && 
				(ptnew-m_segs[i].ptdst).len2()<sqr(seglen*0.08f)))
			ptnew = m_segs[i].ptdst;
		m_segs[i].pt = ptnew;
		m_segs[i].vel += gravity*time_interval;
		m_segs[i].vel += (dw-m_segs[i].vel)*min(1.0f,m_airResistance*time_interval);
		for(iter=0;iter<iEnd;iter++) 
			if ((diff=((m_segs[i].pt-pb[iter].waterPlane.origin)*pb[iter].waterPlane.n)*(pb[iter].iMedium-1>>31))>0) {
				if (rcollDist==0)
					rcollDist = 1.0f/m_collDist;
				m_segs[i].vel += (pb[iter].waterFlow-m_segs[i].vel)*min(1.0f,m_waterResistance*time_interval)-
													gravity*(pb[iter].waterDensity*m_rdensity*min(1.0f,diff*rcollDist)*time_interval);
			}
	}

	if (m_flags & rope_subdivide_segs && m_nVtx>0) {
		for(i=0;i<m_nVtx;i++)	
			m_vtx[i].pt += m_vtx[i].vel*time_interval;
		for(i=0;i<m_nSegs;i++) if (m_segs[i+1].iVtx0>m_segs[i].iVtx0+1) {
			dir0 = (m_vtx[m_segs[i+1].iVtx0].pt-m_vtx[m_segs[i].iVtx0].pt).normalized();
			for(m_segs[i].ptdst.zero(),j=m_segs[i].iVtx0,b=0; j<m_segs[i+1].iVtx0; j++) {
				dir = m_vtx[j].pt-m_vtx[m_segs[i].iVtx0].pt;
				if ((a=(dir^dir0).len2())>b)
					m_segs[i].ptdst = dir-dir0*(dir*dir0), b=a;
			}
			m_segs[i].ptdst.normalize();
		}	else
			m_segs[i].ptdst.zero();
		if (m_bStrained && m_length>0) for(i=0;i<=m_nSegs;i++)
			m_segs[i].pt = m_vtx[m_segs[i].iVtx0].pt;
	}

	if (m_flags & rope_findiff_attached_vel)
		a = 1.0f/m_timeStepFull;
	for(i=1;i>=0;i--) if (m_pTiedTo[i]) {
		m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], pos,q,scale);
		m_posBody[i][1] = pos; m_qBody[i][1] = q;
		m_segs[m_nSegs*i].pt = ptend[i] = pos + q*m_ptTiedLoc[i];
		pbody = m_pTiedTo[i]->GetRigidBody(m_iTiedPart[i]);
		if (!(m_flags & rope_findiff_attached_vel)) {
			dv = pbody->v; dw = pbody->w;
		}	else {
			dv = (m_posBody[i][1]-m_posBody[i][0])*a;
			dq = m_qBody[i][1]*!m_qBody[i][0];
			dw = dq.v*(dq.w*2*a);
			if (dv.len2()>sqr(m_pWorld->m_vars.maxVel)) {
				dv.zero(); dw.zero();
			}
		}
		m_segs[m_nSegs*i].vel = dv + (dw^ptend[i]-pbody->pos);
		m_timeIdle *= m_pTiedTo[i]->IsAwake()^1;
	}
	if (m_flags & (rope_target_vtx_rel0 | rope_target_vtx_rel1)) {
		i = (m_flags & rope_target_vtx_rel1)!=0;
		if (m_pTiedTo[i]) 
			m_pTiedTo[i]->GetLocTransform(m_iTiedPart[i], offstv,qtv,scaletv);
	}

	if (!(m_flags & rope_subdivide_segs) && m_pTiedTo[0] && m_pTiedTo[1] && (ptend[1]-ptend[0]).len2()>sqr(m_length)) {
		float newlen=(ptend[1]-ptend[0]).len(), newseglen=newlen/m_nSegs; rnSegs=1.0f/(m_nSegs+1);
		dir = (ptend[1]-ptend[0]).normalized();
		for(i=1;i<m_nSegs;i++) {
			m_segs[i].dir = dir;
			m_segs[i].pt = ptend[0] + m_segs[i].dir*(newseglen*i);
			m_segs[i].vel = m_segs[0].vel*((m_nSegs+1-i)*rnSegs)+m_segs[m_nSegs].vel*(i*rnSegs);
		}
		m_segs[0].dir = m_segs[m_nSegs].dir = dir;
		pe_status_awake isawake;
		m_bAwake = m_pTiedTo[0]->GetStatus(&isawake) | m_pTiedTo[1]->GetStatus(&isawake) | isneg(-bTargetPoseActive);

		pe_status_constraint sc; sc.id=m_idConstraint;
		if (m_idConstraint && (!m_pTiedTo[m_iConstraintClient]->GetStatus(&sc) || sc.flags & constraint_inactive)) {
			EventPhysJointBroken epjb;
			epjb.idJoint=0; epjb.bJoint=0; MARK_UNUSED epjb.pNewEntity[0],epjb.pNewEntity[1];
			epjb.pEntity[0]=epjb.pEntity[1]=this; epjb.pForeignData[0]=epjb.pForeignData[1]=m_pForeignData; 
			epjb.iForeignData[0]=epjb.iForeignData[1]=m_iForeignData;
			epjb.pt = m_segs[m_nSegs].pt;	epjb.n = m_segs[m_nSegs-1].dir;
			epjb.partid[0] = 1;
			epjb.partid[1] = m_pTiedTo[1]->m_parts[m_iTiedPart[1]].id;
			epjb.partmat[0]=epjb.partmat[1] = -1;
			m_pWorld->OnEvent(2,&epjb);

			pe_params_rope pr; pr.pEntTiedTo[1]=0;
			SetParams(&pr);
		}

		BBox[0] = min(m_segs[0].pt,m_segs[m_nSegs].pt); BBox[1] = max(m_segs[0].pt,m_segs[m_nSegs].pt);
		a = max(max(BBox[1].x-BBox[0].x,BBox[1].y-BBox[0].y),BBox[1].z-BBox[0].z)*0.01f+m_collDist*2;
		BBox[0] -= Vec3(a,a,a); BBox[1] += Vec3(a,a,a);
		if (m_flags & pef_traceable)
			bGridLocked = m_pWorld->RepositionEntity(this,1,BBox);
		{ WriteLockPlatf0 lock(m_lockUpdate);
			m_pos = m_segs[0].pt; m_BBox[0] = BBox[0]; m_BBox[1] = BBox[1];
			for(i=0;i<=m_nSegs;i++)
				m_segs[i].pt0 = m_segs[i].pt;
			AtomicAdd(&m_pWorld->m_lockGrid,-bGridLocked);
		}
		return 1;
	}

	if (m_flags & rope_collides_with_terrain && m_pTiedTo[0] && m_pTiedTo[1] && m_pTiedTo[0]->m_iSimClass+m_pTiedTo[1]->m_iSimClass==0 &&
			sqr(m_length*0.95f) < (m_segs[0].pt-m_segs[m_nSegs].pt).len2() && m_penaltyScale>0)
		flags &= ~rope_collides_with_terrain;

#if !defined(USE_PHYS_JOBS) || !defined(USE_ROPE_JOB)
	EnforceConstraints(seglen, qtv,offstv,scaletv, m_bTargetPoseActive);
	if (bTargetPoseActive)
		ApplyStiffness(time_interval, bTargetPoseActive, qtv,offstv,scaletv);
#endif

	BBox[0] = BBox[1] = m_segs[0].pt;
	for(i=0;i<=m_nSegs;i++) {
		(m_segs[i].vel += m_segs[i].vel_ext) *= damping;
		m_segs[i].vel_ext.zero();
		m_segs[i].bRecalcDir = 0;
		BBox[0] = min(BBox[0], m_segs[i].pt);
		BBox[1] = max(BBox[1], m_segs[i].pt);
	}
	a = max(max(BBox[1].x-BBox[0].x,BBox[1].y-BBox[0].y),BBox[1].z-BBox[0].z)*0.01f+m_collDist*2;
	BBox[0] -= Vec3(a,a,a); BBox[1] += Vec3(a,a,a);
	collBBox[0]=BBox[0]; collBBox[1]=BBox[1];

	if (m_flags & (rope_collides|rope_collides_with_terrain|rope_collides_with_attachment)) {
		FRAME_PROFILER( "Rope collision",GetISystem(),PROFILE_PHYSICS );

		int iCaller = get_iCaller_int();
		CPhysicalEntity **pentlist;
		int iseg,nEnts,iend;
		box boxrope,boxpart;
		Vec3 center,ptres[2],n;
		geom_world_data gwd;
		COverlapChecker Overlapper;
		pe_status_pos sp; sp.timeBack = 1;
		m_bHasContacts = 0;

		Overlapper.Init();
		if (!is_unused(m_collBBox[0])) {
			if (!m_pOuterEntity)
				collBBox[0]=m_collBBox[0], collBBox[1]=m_collBBox[1];
			else {
				m_pOuterEntity->GetLocTransform(0,pos,q,scale);
				Matrix33 mtx = Matrix33(q);
				center = mtx*((m_collBBox[0]+m_collBBox[1])*0.5f*scale)+pos;
				sz = mtx.Fabs()*((m_collBBox[1]-m_collBBox[0])*0.5f*scale);
				collBBox[0] = center-sz; collBBox[1] = center+sz;
			}
		}
		boxrope.size = (BBox[1]-BBox[0])*0.5f;
		center = (BBox[0]+BBox[1])*0.5f;

		nEnts = m_pWorld->GetEntitiesAround(collBBox[0],collBBox[1], pentlist, 
			(flags & rope_collides_with_terrain ? ent_terrain:0) | 
			ent_static|ent_sleeping_rigid|ent_rigid|ent_living|ent_sort_by_mass|ent_ignore_noncolliding|ent_triggers, this, 0,iCaller);
		if (m_flags & rope_collides_with_attachment && pentlist) {
			for(i=j=0;i<nEnts;i++) 
				j += (pentlist[i]==m_pTiedTo[0]) | (pentlist[i]==m_pTiedTo[1])*2;
			if (m_pTiedTo[0] && !(j&1))
				pentlist[nEnts++] = m_pTiedTo[0];
			if (m_pTiedTo[1] && !(j&2))
				pentlist[nEnts++] = m_pTiedTo[1];
		}

		{
#ifndef __SPU__
		WriteLock lockEL(g_lockProcessedAux);
#endif
assert(pentlist);
		for(i=nCheckParts=0;i<nEnts;i++) 
		if (pentlist[i]!=this && !(m_flags & rope_ignore_attachments && (
			pentlist[i]==m_pTiedTo[0] || m_pTiedTo[0] && m_pTiedTo[0]->m_pForeignData && m_pTiedTo[0]->m_pForeignData==pentlist[i]->m_pForeignData ||
			pentlist[i]==m_pTiedTo[1] || m_pTiedTo[1] && m_pTiedTo[1]->m_pForeignData && m_pTiedTo[1]->m_pForeignData==pentlist[i]->m_pForeignData)))
		for(iter=0,pentlist[i]->m_bProcessed_aux=nCheckParts<<24; iter<pentlist[i]->GetUsedPartsCount(iCaller) ;iter++) 
		if (pentlist[i]->m_parts[j=pentlist[i]->GetUsedPart(iCaller,iter)].flags & m_flagsCollider) {
			checkParts[nCheckParts].offset = pentlist[i]->m_pos+pentlist[i]->m_qrot*pentlist[i]->m_parts[j].pos;
			boxrope.Basis = Matrix33(pentlist[i]->m_qrot*pentlist[i]->m_parts[j].q);
			boxrope.center = (center-checkParts[nCheckParts].offset)*boxrope.Basis;
			if (pentlist[i]->m_parts[j].pPhysGeomProxy->pGeom->GetType()!=GEOM_HEIGHTFIELD) {
				pentlist[i]->m_parts[j].pPhysGeomProxy->pGeom->GetBBox(&checkParts[nCheckParts].bbox);
				checkParts[nCheckParts].bbox.center *= pentlist[i]->m_parts[j].scale;
				checkParts[nCheckParts].bbox.size *= pentlist[i]->m_parts[j].scale;
			} else
				checkParts[nCheckParts].bbox = boxrope;
			boxrope.bOriented++;
			if (box_box_overlap_check(&boxrope,&checkParts[nCheckParts].bbox,&Overlapper)) {
				checkParts[nCheckParts].pent = pentlist[i];
				checkParts[nCheckParts].ipart = j;
				checkParts[nCheckParts].R = boxrope.Basis;
				checkParts[nCheckParts].scale = pentlist[i]->m_parts[j].scale;
				if (((CGeometry*)pentlist[i]->m_parts[j].pPhysGeomProxy->pGeom)->IsAPrimitive()) {
					n = (m_segs[iseg=(m_pTiedTo[1]==0 ? 0:m_nSegs)].pt-checkParts[nCheckParts].offset)*checkParts[nCheckParts].R;
					gwd.scale = checkParts[nCheckParts].scale;
					if (pentlist[i]->m_parts[j].pPhysGeomProxy->pGeom->FindClosestPoint(&gwd,iend,iend,n,n,ptres)>=0 && 
							((a=(ptres[1]-ptres[0])*(ptres[0]-checkParts[nCheckParts].bbox.center*gwd.scale))<0 || 
							 (ptres[0]-ptres[1]).len2()<sqr(m_collDist*1.2f))) 
					{
						n = ptres[0]+(ptres[1]-ptres[0]).normalized()*(m_collDist*1.2f*sgnnz(a));
						m_segs[iseg].pt = checkParts[nCheckParts].R*n+checkParts[nCheckParts].offset;
						//continue;
					}
				}
				pentlist[i]->m_parts[j].pPhysGeomProxy->pGeom->PrepareForRayTest(
					pentlist[i]->m_parts[j].scale==1.0f ? seglen:seglen/pentlist[i]->m_parts[j].scale);
				checkParts[nCheckParts].rscale = checkParts[nCheckParts].scale==1.0f ? 1.0f:1.0f/checkParts[nCheckParts].scale;
				if (m_flags & rope_subdivide_segs) {
					pentlist[i]->GetStatus(&sp);
					checkParts[nCheckParts].pos0 = sp.pos;
					checkParts[nCheckParts].q0 = sp.q;	
				} else {
					checkParts[nCheckParts].pos0 = pentlist[i]->m_pos;
					checkParts[nCheckParts].q0 = pentlist[i]->m_qrot;
				}
				checkParts[nCheckParts].pGeom = (CGeometry*)pentlist[i]->m_parts[j].pPhysGeomProxy->pGeom;
				checkParts[nCheckParts].bProcess = 1;
				if (pentlist[i]->m_nParts<=24)
					pentlist[i]->m_bProcessed_aux |= 1u<<j;
				else
					pentlist[i]->m_parts[j].flags |= geom_removed;
				AtomicAdd(&pentlist[i]->m_bProcessed, ~pentlist[i]->m_bProcessed&1<<iCaller);
				pbody = pentlist[i]->GetRigidBody(j);
				checkParts[nCheckParts].v = pbody->v-(pbody->w^pbody->pos);
				checkParts[nCheckParts].w = pbody->w;
				if (++nCheckParts==sizeof(checkParts)/sizeof(checkParts[0]))
					goto enoughgeoms;
			}
		} enoughgeoms:

		for(i=0; i<=m_nSegs; i++) if (m_segs[i].pContactEnt)
			if (!(m_segs[i].pContactEnt->m_bProcessed & 1<<iCaller) ||
					 ((m_segs[i].iCheckPart = m_segs[i].pContactEnt->GetCheckPart(m_segs[i].iContactPart)) & 0xFFFF)>=nCheckParts)
			{
				m_segs[i].pContactEnt->Release(); m_segs[i].pContactEnt=0;
			}
		for(i=0; i<nCheckParts; i++) if (checkParts[i].pent->m_bProcessed & 1<<iCaller) {
			AtomicAdd(&checkParts[i].pent->m_bProcessed, -(1<<iCaller));
			if (checkParts[i].pent->m_nParts>24) for(j=0;j<checkParts[i].pent->m_nParts;j++)
				checkParts[i].pent->m_parts[j].flags &= ~geom_removed;
		}	
		}	// g_lockProcessedAux

		if (!(m_flags & rope_subdivide_segs)) {
			if (m_pTiedTo[0] && m_pTiedTo[1])
				iDir = isneg(m_pTiedTo[1]->GetRigidBody(m_iTiedPart[1])->Minv-m_pTiedTo[0]->GetRigidBody(m_iTiedPart[0])->Minv);
			else 
				iDir = iszero((intptr_t)m_pTiedTo[0]);
#if defined(USE_PHYS_JOBS) && defined(USE_ROPE_JOB)
			goto CreateJob;
#endif
			CheckCollisions(iDir, checkParts,nCheckParts, seglen,rseglen);
		} else 
			StepSubdivided(time_interval,checkParts,nCheckParts, seglen);
	}	else for(i=0;i<=m_nSegs;i++)
		m_segs[i].pContactEnt = 0;

#if defined(USE_PHYS_JOBS) && defined(USE_ROPE_JOB)
	if (!(m_flags & rope_subdivide_segs)) {
		CreateJob:
		CMemStream stm,stmSizer;
		int ijob=FindJobQueue(-1,stm,3), sz;
		if (ijob>=0) {
			for(i=0;i<nCheckParts;i++) {
				checkParts[i].bProcess=0; 
				if ((j=checkParts[i].pGeom->GetType())==GEOM_HEIGHTFIELD && (!m_prev || !m_pOuterEntity || m_prev->m_pOuterEntity!=m_pOuterEntity) ||
						!(checkParts[i].bProcess = GeomOffsInJobBuf(checkParts[i].pGeom, ijob)))
				{
					if (j==GEOM_HEIGHTFIELD) {
						box bbox; bbox.Basis.SetIdentity(); bbox.bOriented=0;
						bbox.center=(collBBox[0]+collBBox[1])*0.5f; bbox.size=(collBBox[1]-collBBox[0])*0.5f;
						CBoxGeom gbox; gbox.CreateBox(&bbox);
						if (!checkParts[i].pGeom->PrepareForIntersectionTest(0,&gbox,0)) {
							checkParts[i--] = checkParts[--nCheckParts];
							continue;
						}
					}	else
						checkParts[i].pGeom->PrepareForRayTest(seglen*-2*checkParts[i].rscale);
					SaveGeomToMem(checkParts[i].pGeom, stmSizer, 1);
				}
				stmSizer.Write(i);
			}
			sz = sizeof(this) + sizeof(*this) + sizeof(m_segs[0])*(m_nSegs+1) + stmSizer.m_iPos +
					 sizeof(float)*4 + sizeof(quaternionf) + sizeof(Vec3) + sizeof(int)*4 + sizeof(checkParts[0])*nCheckParts;
			if (stm.m_nSize >= stm.m_iPos+sz) {
				if (m_flags & rope_no_stiffness_when_colliding) {
					iDir = m_pTiedTo[0] ? m_pTiedTo[0]->m_id : (m_pTiedTo[1] ? m_pTiedTo[1]->m_id : -2);
					for(i=bHasContacts=0;i<=m_nSegs;i++) if (m_segs[i].pContactEnt)
						bHasContacts |= m_segs[i].pContactEnt->m_id-iDir>>31 | iDir-m_segs[i].pContactEnt->m_id>>31;
					if (bHasContacts)
						bTargetPoseActive = 0;
				}
				stm.Write(this);
				stm.Write(sz);
				CRopeEntity *pentJob = (CRopeEntity*)(stm.m_pBuf+stm.m_iPos);
				stm.Write(*this); 
				pentJob->m_jobE = 1.0f;
				if (m_pWorld->m_bWorldStep==2)
					pentJob->m_timeStepFull = time_interval;
				stm.Write(m_segs, sizeof(m_segs[0])*(m_nSegs+1));
				stm.Write(time_interval);
				stm.Write(seglen); stm.Write(rseglen);
				stm.Write(bTargetPoseActive); stm.Write(iDir);
				stm.Write(qtv); stm.Write(offstv); stm.Write(scaletv);
				stm.Write(nCheckParts);
				stm.Write(checkParts, sizeof(checkParts[0])*nCheckParts);
				for(i=0;i<nCheckParts;i++) 
					SaveGeomToJobBuf(stm, checkParts[i].pGeom, checkParts[i].bProcess, ijob, 1);
				UpdateJobBuf(ijob, sz);
				return 1;
			}
		}
	}

	EnforceConstraints(seglen, qtv,offstv,scaletv, m_bTargetPoseActive);
	if (bTargetPoseActive)
		ApplyStiffness(time_interval, bTargetPoseActive, qtv,offstv,scaletv);
	if (nCheckParts>0)
		CheckCollisions(iDir, checkParts,nCheckParts, seglen,rseglen);
#endif

	if (!(m_flags & rope_no_solver))
		E = Solver(time_interval, seglen);
	else
		for(i=0,E=0;i<=m_nSegs;i++) E += m_segs[i].vel.len2();

StepDone:
	int bAwake = m_bAwake;
	i = -isneg(E-m_Emin*(m_nSegs+1));
	int nSlowFrames = m_nSlowFrames;
	nSlowFrames = (nSlowFrames&i)-i;
	i = isneg(0.0001f-m_maxTimeIdle); // forceful deactivation is turned on
	bAwake = (isneg(nSlowFrames-4)|(bTargetPoseActive&1)) & (i^1 | isneg((m_timeIdle+=time_interval*i)-m_maxTimeIdle));
	if (m_pTiedTo[0] && m_pTiedTo[0]->GetType()==PE_ROPE)
		bAwake |= m_pTiedTo[0]->IsAwake();
	if (!bAwake)
		for(i=iszero((intptr_t)m_pTiedTo[0])^1; i<m_nSegs+iszero((intptr_t)m_pTiedTo[1]); i++)
			m_segs[i].vel.zero();

	if (m_flags & pef_traceable)
		bGridLocked = m_pWorld->RepositionEntity(this,1,BBox);
	{ WriteLockPlatf0 lock(m_lockUpdate);
		m_BBox[0] = BBox[0]; m_BBox[1] = BBox[1]; m_pos = m_segs[0].pt;
		for(i=0;i<=m_nSegs;i++)
			m_segs[i].pt0 = m_segs[i].pt;
		if (m_pTiedTo[0])
			m_pTiedTo[0]->GetLocTransform(m_iTiedPart[0],m_lastposHost,m_lastqHost,scaletv);
		else {
			m_lastposHost=m_pos; m_lastqHost.SetIdentity();
		}
		AtomicAdd(&m_pWorld->m_lockGrid,-bGridLocked);
	}
	m_bAwake = bAwake;
	m_nSlowFrames = nSlowFrames;

	return isneg(m_timeStepFull-m_timeStepPerformed-0.001f);
}
#endif//__SPU__

void CRopeEntity::JobProc(CMemStream &stm)
{
#if defined(USE_PHYS_JOBS) && defined(USE_ROPE_JOB)
	int i,bTargetPoseActive,iDir,nCheckParts,endpos=stm.m_iPos; endpos+=stm.Read<int>();
	float time_interval,seglen,rseglen,scaletv,dt,E;
	quaternionf qtv; Vec3 offstv;

	CRopeEntity *pent = SPU_LOCAL_PTR((CRopeEntity*)(stm.m_pBuf+stm.m_iPos)); stm.m_iPos+=sizeof(CRopeEntity);
	rope_segment *psegs = SPU_LOCAL_PTR((rope_segment*)(stm.m_pBuf+stm.m_iPos)); stm.m_iPos+=sizeof(rope_segment)*(pent->m_nSegs+1);
	pent->m_segs = psegs;
	stm.Read(time_interval);
	stm.Read(seglen); stm.Read(rseglen);
	stm.Read(bTargetPoseActive); stm.Read(iDir);
	stm.Read(qtv); stm.Read(offstv); stm.Read(scaletv);
	stm.Read(nCheckParts);
	SRopeCheckPart *checkParts = SPU_LOCAL_PTR((SRopeCheckPart*)(stm.m_pBuf+stm.m_iPos)); stm.m_iPos+=sizeof(SRopeCheckPart)*nCheckParts;
	for(i=0;i<nCheckParts;i++)
		checkParts[i].pGeom = LoadGeomFromMemBufOffs(stm);

	for(dt=0; dt<pent->m_timeStepFull; dt+=pent->m_maxAllowedStep) {
		pent->EnforceConstraints(seglen, qtv,offstv,scaletv, bTargetPoseActive);
		if (bTargetPoseActive)
			pent->ApplyStiffness(time_interval, bTargetPoseActive, qtv,offstv,scaletv);
		if (nCheckParts>0)
			pent->CheckCollisions(iDir, checkParts,nCheckParts, seglen,rseglen);
		if (!(pent->m_flags & rope_no_solver))
			E = pent->Solver(time_interval, seglen);
		else
			for(i=0,E=0;i<=pent->m_nSegs;i++) E += psegs[i].vel.len2();
	}
	pent->m_jobE = E;
#endif
}

#ifndef __SPU__
void CRopeEntity::OnDelayedStep(CMemStream &stm)
{
	int i,endpos=stm.m_iPos-sizeof(void*); endpos+=stm.Read<int>();

	CRopeEntity *pent = (CRopeEntity*)(stm.m_pBuf+stm.m_iPos); stm.m_iPos+=sizeof(CRopeEntity);
	rope_segment *psegs = (rope_segment*)(stm.m_pBuf+stm.m_iPos); stm.m_iPos+=sizeof(rope_segment)*(pent->m_nSegs+1);
	for(i=0; i<=m_nSegs; i++)	{
		if (m_segs[i].pContactEnt!=psegs[i].pContactEnt) {
			if (m_segs[i].pContactEnt) m_segs[i].pContactEnt->Release();
			if (psegs[i].pContactEnt) psegs[i].pContactEnt->AddRef();
		}
		(rope_segment&)m_segs[i] = psegs[i];
	}
	m_jobE = pent->m_jobE;
	Step(m_timeStepPerformed=m_timeStepFull);
	stm.m_iPos = endpos;
}


int CRopeEntity::RegisterContacts(float time_interval,int nMaxPlaneContacts)
{
	m_energy = 0;
	if (!m_bStrained || !m_pTiedTo[0] || !m_pTiedTo[1] || m_penaltyScale==0)
		return 1;
	int i,j,iStart,iEnd,jStart,sg,nUniqueBodies,ivtx0,ivtx1,icnt0,icnt1;
	float friction,cos2a,cosa,sina,cosb,sinb,kP,len,rlen,sag,maxM;
	Vec3 n,dir0,dir1,vdir0,vdir1;
	RigidBody *pbody[3];
	entity_contact *pContact;
	DisablePreCG();

	if (!m_vtxSolver)
		m_vtxSolver = new rope_solver_vtx[m_nVtxAlloc];
	cosb = 1/sqrt_tpl(1+sqr(m_frictionPull));
	for(i=0,len=0;i<m_nVtx-1;i++)
		len += (m_vtx[i+1].pt-m_vtx[i].pt)*m_vtx[i].dir;
	rlen = 1.0f/len; sag = m_length>0 ? m_length*1.005f-len : -1.0f;
	for(ivtx0=0,len=0; ivtx0<m_nVtx-1 && len<m_attachmentZone; len+=(m_vtx[ivtx0+1].pt-m_vtx[ivtx0].pt).len(),ivtx0++);	ivtx0--;
	for(ivtx1=m_nVtx-1,len=0; ivtx1>0 && len<m_attachmentZone; len+=(m_vtx[ivtx1-1].pt-m_vtx[ivtx1].pt).len(),ivtx1--); ivtx1++;

	for(iStart=j=0; iStart<m_nVtx-1; iStart=iEnd) {
		pContact = (entity_contact*)AllocSolverTmpBuf(sizeof(entity_contact));

		for(i=iStart+1,kP=1,len=0,sg=0,icnt0=-1,icnt1=iStart; i<m_nVtx-1; i++) {
			len += m_vtx[i-1].dir*(m_vtx[i].pt-m_vtx[i-1].pt);
			if (!is_unused(m_vtx[i].ncontact))
				if ((cos2a=-(m_vtx[i].dir*m_vtx[i-1].dir)) > -0.995f && 
						sqr_signed(m_vtx[i].ncontact*(m_vtx[i-1].dir-m_vtx[i].dir))>(m_vtx[i-1].dir-m_vtx[i].dir).len2()*0.01f &&
						inrange(i,ivtx0,ivtx1)) 
				{
					pbody[2] = m_vtx[i].pContactEnt->GetRigidBody(m_vtx[i].iContactPart,1);
					pbody[2]->Fcollision.zero(); pbody[2]->Tcollision.zero();
					m_vtx[i].dP = 1;
					if ((1-cos2a) < sqr(m_frictionPull)*(1+cos2a))
						break;
					sg -= sgn((m_vtx[i-1].dir+m_vtx[i].dir)*(m_vtx[i].vel-m_vtx[i].vcontact));
					m_vtx[icnt1].iContactPart |= i<<16;
					icnt0 += i-icnt0 & icnt0>>31; icnt1 = i;
				} else 
					m_vtx[i].dP = 0;
		}
		m_vtx[icnt1].iContactPart |= i<<16;
		// TODO: switch to an infinite friction mdoe if we have too many high-friction contacts 
		// with a non-static geom (or maybe not just non-static)
		friction = m_frictionPull*sgn(sg);
		if (icnt0>=0) {
			dir0 = (m_vtx[icnt0].pt-m_vtx[iStart].pt).normalized();
			dir1 = (m_vtx[i].pt-m_vtx[icnt1].pt).normalized();
		}	else
			dir0=dir1 = (m_vtx[i].pt-m_vtx[iStart].pt).normalized();
		len += dir1*(m_vtx[i].pt-m_vtx[i-1].pt);
		iEnd = i;
		pContact->pbody[0]=pbody[0] = iStart==0 ? 
			(pContact->pent[0]=m_pTiedTo[0])->GetRigidBody(pContact->ipart[0]=m_iTiedPart[0],1) : 
			(pContact->pent[0]=m_vtx[iStart].pContactEnt)->GetRigidBody(pContact->ipart[0]=m_vtx[iStart].iContactPart,1);
		pContact->pbody[1]=pbody[1] = iEnd==m_nVtx-1 ? 
			(pContact->pent[1]=m_pTiedTo[1])->GetRigidBody(pContact->ipart[1]=m_iTiedPart[1],1) : 
			(pContact->pent[1]=m_vtx[iEnd].pContactEnt)->GetRigidBody(pContact->ipart[1]=m_vtx[iEnd].iContactPart,1);
		for(i=0;i<2;i++)
			pbody[i]->Fcollision.zero(), pbody[i]->Tcollision.zero();
		pContact->pt[0] = m_vtx[iStart].pt;
		pContact->pt[1] = m_vtx[iEnd].pt;
		//pContact->K.SetZero();
		sinb = friction*cosb;
		pbody[0]->Fcollision += dir0;
		pbody[0]->Tcollision += m_vtx[iStart].pt-pbody[0]->pos ^ dir0;
		maxM = max(pbody[0]->M, pbody[1]->M);
		nUniqueBodies = (pbody[0]!=pbody[1])+1;

		for(i=iStart+1,kP=1,jStart=j,icnt1=iStart; i<iEnd; i++) 
			if (!is_unused(m_vtx[i].ncontact) && m_vtx[i].dP>0) {
				vdir0 = (m_vtx[i].pt-m_vtx[icnt1].pt).normalized();
				vdir1 = (m_vtx[m_vtx[i].iContactPart>>16].pt-m_vtx[i].pt).normalized();
				m_vtx[i].iContactPart &= 0xFFFF; icnt1 = i;
				cos2a = -(vdir1*vdir0);
				cosa = sqrt_tpl((1+cos2a)*0.5f);
				sina = sqrt_tpl((1-cos2a)*0.5f);
				m_vtxSolver[j].ivtx = i;
				m_vtxSolver[j].v = vdir1-vdir0;
				n = m_vtxSolver[j].v/-(2*cosa);
				n += (vdir0+vdir1)*(friction/(2*sina));
				kP /= sina*cosb+cosa*sinb;
				m_vtxSolver[j].P = n*(kP*-2*sina*cosa*cosb);
				m_vtxSolver[j].pbody=pbody[2] = m_vtx[i].pContactEnt->GetRigidBody(m_vtx[i].iContactPart,1);
				maxM = max(maxM, pbody[2]->M);
				pbody[2]->Fcollision += m_vtxSolver[j].P;
				pbody[2]->Tcollision += (m_vtxSolver[j].r = m_vtx[i].pt-pbody[2]->pos)^m_vtxSolver[j].P;
				kP *= sina*cosb-cosa*sinb;
				nUniqueBodies += pbody[2]!=pbody[0] && pbody[2]!=pbody[1];
				j++;
			}
		m_vtx[iStart].iContactPart &= 0xFFFF;
		pContact->nloc.x = kP;
		pbody[1]->Fcollision -= dir1*kP;
		pbody[1]->Tcollision -= m_vtx[iEnd-1].pt-pbody[1]->pos ^ dir1;

		kP = -dir1*(pbody[1]->Fcollision*pbody[1]->Minv + (pbody[1]->Iinv*pbody[1]->Tcollision ^ m_vtx[iEnd].pt-pbody[1]->pos));
		kP += dir0*(pbody[0]->Fcollision*pbody[0]->Minv + (pbody[0]->Iinv*pbody[0]->Tcollision ^ m_vtx[iStart].pt-pbody[0]->pos));
		for(i=iStart+1,j=jStart; i<iEnd; i++) 
			if (!is_unused(m_vtx[i].ncontact) && m_vtx[i].dP>0 && (pbody[2]=m_vtx[i].pContactEnt->GetRigidBody(m_vtx[i].iContactPart,1)))
				kP += m_vtxSolver[j++].v*(pbody[2]->Fcollision*pbody[2]->Minv + 
							(pbody[2]->Iinv*pbody[2]->Tcollision^m_vtx[i].pt-pbody[2]->pos));
		pContact->nloc.y = 1/kP;
		pContact->nloc.z = m_maxForce*max(0.01f,time_interval)*1.1f;

		pContact->flags = contact_rope;
		pContact->n = dir0;
		pContact->vreq = -dir1;
		pContact->friction = len*rlen*max(0.0f,min(m_pWorld->m_vars.maxUnprojVelRope, -sag*m_penaltyScale));	
		pContact->Pspare = 0;
		pContact->pBounceCount = (int*)(m_vtxSolver+jStart);
		pContact->iCount = j-jStart;
		if (nUniqueBodies>1 && maxM>0) {
			::RegisterContact(pContact);
			m_energy += maxM*sqr(pContact->friction);
			pContact->next=m_pContact; m_pContact=pContact;
		}
	}
	m_bContactsRegistered = 1;
	m_vtx[0].dP = 0;

	return 1;
}


int CRopeEntity::Update(float time_interval, float damping)
{
	if (m_bStrained && m_pTiedTo[0] && m_pTiedTo[1]) {
		int i,j,i0;
		float vrope[2],len,rseglen,t;
		Vec3 norm,v,tang,dir;
		RigidBody *pbody;

		/*m_pTiedTo[0]->RemoveCollider(this);
		m_pTiedTo[1]->RemoveCollider(this);
		for(i=1;i<m_nVtx-1;i++) if (m_vtx[i].pContactEnt)
			m_vtx[i].pContactEnt->RemoveCollider(this);
		for(i=0;i<m_nColliders;i++)
			m_pColliders[i]->Release();
		m_nColliders = 0;*/

		if (m_length>0) {
			for(i=0;i<2;i++) {
				pbody = m_pTiedTo[i]->GetRigidBody(m_iTiedPart[i]);
				vrope[i] = m_vtx[i*(m_nVtx-2)].dir*(m_vtx[i*(m_nVtx-1)].vel = pbody->v + (pbody->w ^ m_vtx[i*(m_nVtx-1)].pt-pbody->pos));
			}
			for(i=0; i<m_nVtx-1; ) {
				for(j=i+1; j<m_nVtx-1 && is_unused(m_vtx[j].ncontact); j++);
				len = (m_vtx[j].pt-m_vtx[0].pt).len2(); t = (m_vtx[j].pt-m_vtx[m_nVtx-1].pt).len2(); t=t/(len+t);
				len = (dir=m_vtx[j].pt-m_vtx[i].pt).len(); dir *= (rseglen=1/len);
				if (j<m_nVtx-1) {
					pbody = m_vtx[j].pContactEnt->GetRigidBody(m_vtx[j].iContactPart);
					v = pbody->v + (pbody->w^m_vtx[j].pt-pbody->pos);
					norm = (m_vtx[j-1].dir-m_vtx[j].dir).normalized();
					tang = (m_vtx[j-1].dir+m_vtx[j].dir).normalized();
					m_vtx[j].vel = norm*(v*norm) + tang*(vrope[0]*(1.0f-t)+vrope[1]*t);	
				}
				for(i0=i++; i<j; i++) {
					t = (dir*(m_vtx[i].pt-m_vtx[i0].pt))*rseglen;
					m_vtx[i].vel = m_vtx[i0].vel*(1-t) + m_vtx[j].vel*t;
				}
			}
			for(i=0;i<=m_nSegs;i++)
				m_segs[i].vel = m_vtx[m_segs[i].iVtx0].vel;
		}

		for(len=0; m_pContact; m_pContact=m_pContact->next)	{
			len = max(len,m_pContact->Pspare);
			for(i=0;i<m_pContact->iCount;i++)	{
				j = ((rope_solver_vtx*)m_pContact->pBounceCount)[i].ivtx;
				if (m_vtx[j].pContactEnt && m_vtx[j].pContactEnt->m_parts[m_vtx[j].iContactPart].flags & geom_monitor_contacts)
					m_vtx[j].pContactEnt->OnContactResolved(m_pContact,i+2,-1);
			}
		}
		if (m_maxForce>0 && len>m_maxForce*max(0.01f,time_interval) && !(m_flags & rope_no_tears)) {
			EventPhysJointBroken epjb;
			epjb.idJoint=0; epjb.bJoint=0; MARK_UNUSED epjb.pNewEntity[0],epjb.pNewEntity[1];
			epjb.pEntity[0]=epjb.pEntity[1]=this; epjb.pForeignData[0]=epjb.pForeignData[1]=m_pForeignData; 
			epjb.iForeignData[0]=epjb.iForeignData[1]=m_iForeignData;
			epjb.pt = m_segs[m_nSegs].pt;	epjb.n = m_segs[m_nSegs-1].dir;
			epjb.partid[0] = 1;
			epjb.partid[1] = m_pTiedTo[1]->m_parts[m_iTiedPart[1]].id;
			epjb.partmat[0]=epjb.partmat[1] = -1;
			m_pWorld->OnEvent(2,&epjb);

			pe_params_rope pr;
			pr.pEntTiedTo[1] = 0;
			SetParams(&pr);
		}
	}
	m_pContact = 0;
	return 1;
}
#endif//__SPU__

static geom_contact g_RopeContact SPU_LOCAL;
static volatile int g_RopeLock SPU_LOCAL;

int CRopeEntity::RayTrace(SRayTraceRes& rtr)
{
	ReadLock lock(m_lockUpdate),lock0(m_lockVtx);
	Vec3 dp,dir,l,pt;
	strided_pointer<Vec3> vtx;
	float t,t1,llen2,raylen=rtr.pRay->m_ray.dir*rtr.pRay->m_dirn; 
	int i,nSegs;
	if (m_nVtx==0) {
		vtx = strided_pointer<Vec3>(&m_segs[0].pt0, sizeof(rope_segment));
		nSegs = m_nSegs;
	}	else {
		vtx = strided_pointer<Vec3>(&m_vtx[0].pt, sizeof(rope_vtx));
		nSegs = m_nVtx-1;
	}

	for(i=0;i<nSegs;i++) {
		dp = rtr.pRay->m_ray.origin-vtx[i];
		if ((dp^rtr.pRay->m_dirn).len2()<sqr(m_collDist) && inrange((vtx[i]-rtr.pRay->m_ray.origin)*rtr.pRay->m_dirn, 0.0f,raylen)) {
			pt = vtx[i]; break;
		}
		dir = vtx[i+1]-vtx[i];
		l = dir^rtr.pRay->m_dirn; llen2 = l.len2();
		t = (dp^rtr.pRay->m_dirn)*l; t1 = (dp^dir)*l;
		if (sqr(dp*l)<sqr(m_collDist)*llen2 && inrange(t, 0.0f,llen2) && inrange(t1, 0.0f,llen2*raylen)) {
			pt = vtx[i]+dir*(t/llen2); break;
		}
	}

	if (i<m_nSegs) {
		WriteLockCond lockl(g_RopeLock,1); lockl.SetActive(0);
		rtr.SetLock(&g_RopeLock);
		rtr.pcontacts = &g_RopeContact;
		rtr.pcontacts->pt = pt;
		rtr.pcontacts->t = (pt-rtr.pRay->m_ray.origin)*rtr.pRay->m_dirn;
		rtr.pcontacts->id[0] = m_surface_idx;
		rtr.pcontacts->iNode[0] = i;
		rtr.pcontacts->n = -rtr.pRay->m_dirn;
		return 1;
	}

	return 0;
}

#ifndef __SPU__
void CRopeEntity::ApplyVolumetricPressure(const Vec3 &epicenter, float kr, float rmin)
{
	if (m_nSegs>0 && m_maxForce>0 && m_pTiedTo[0] && m_pTiedTo[1]) {
		int i;
		float dP;
		Vec3 r;
		for(i=0,dP=0; i<m_nSegs; i++) {
			r = (m_segs[i].pt+m_segs[i+1].pt)*0.5f-epicenter;
			dP += (m_segs[i].dir^r).len()/(r.len()*max(r.len2(),rmin*rmin));
		}
		if (dP*kr*m_length*m_collDist*2 > m_maxForce*0.01f) {
			EventPhysJointBroken epjb;
			epjb.idJoint=0; epjb.bJoint=0; MARK_UNUSED epjb.pNewEntity[0],epjb.pNewEntity[1];
			epjb.pEntity[0]=epjb.pEntity[1]=this; epjb.pForeignData[0]=epjb.pForeignData[1]=m_pForeignData; 
			epjb.iForeignData[0]=epjb.iForeignData[1]=m_iForeignData;
			epjb.pt = m_segs[m_nSegs].pt;	epjb.n = m_segs[m_nSegs-1].dir;
			epjb.partid[0] = isneg((m_segs[m_nSegs].pt-epicenter).len2()-(m_segs[0].pt-epicenter).len2());
			epjb.partid[1] = m_pTiedTo[epjb.partid[0]]->m_parts[m_iTiedPart[epjb.partid[0]]].id;
			epjb.partmat[0]=epjb.partmat[1] = -1;
			m_pWorld->OnEvent(2,&epjb);

			pe_params_rope pr;
			pr.pEntTiedTo[epjb.partid[0]] = 0;
			SetParams(&pr);
		}
		m_bAwake = 1;
	}
}


int CRopeEntity::GetStateSnapshot(CStream &stm,float time_back,int flags)
{
	ReadLock lock(m_lockUpdate);
	stm.WriteNumberInBits(SNAPSHOT_VERSION,4);
	WritePacked(stm,m_nSegs);

	if (m_nSegs>0) for(int i=0; i<=m_nSegs; i++) {
		stm.Write(m_segs[i].pt);
		if (m_segs[i].vel.len2()>0) {
			stm.Write(true);
			stm.Write(m_segs[i].vel);
		} else stm.Write(false);
	}
	stm.Write(m_bAwake!=0);

	return 1;
}


int CRopeEntity::SetStateFromSnapshot(CStream &stm, int flags)
{
	int i,ver=0; 
	bool bnz;

	stm.ReadNumberInBits(ver,4);
	if (ver!=SNAPSHOT_VERSION)
		return 0;
	ReadPacked(stm,i);
	if (i!=m_nSegs)
		return 0;
	WriteLock lock(m_lockUpdate);

	if (!(flags & ssf_no_update)) {
		if (m_nSegs>0) for(i=0; i<=m_nSegs; i++) {
			stm.Read(m_segs[i].pt);
			stm.Read(bnz); if (bnz) 
				stm.Read(m_segs[i].vel);
			else
				m_segs[i].vel.zero();
			if (m_segs[i].pContactEnt) {
				m_segs[i].pContactEnt->Release();
				m_segs[i].pContactEnt = 0;
			}
		}
		stm.Read(bnz);
		m_bAwake = bnz ? 1:0;

		for(i=0;i<m_nSegs;i++)
			m_segs[i].dir = (m_segs[i+1].pt-m_segs[i].pt).normalized();
		m_pos = m_segs[0].pt;
		if (m_flags & pef_traceable)
			AtomicAdd(&m_pWorld->m_lockGrid,-m_pWorld->RepositionEntity(this,1));
	} else {
		for(i=0;i<=m_nSegs;i++) {
			stm.Seek(stm.GetReadPos()+sizeof(Vec3)*8);
			stm.Read(bnz); if (bnz)
				stm.Seek(stm.GetReadPos()+sizeof(Vec3)*8);
		}
		stm.Read(bnz);
	}

	return 1;
}


void CRopeEntity::OnNeighbourSplit(CPhysicalEntity *pentOrig, CPhysicalEntity *pentNew)
{
	int i,ipart;
	CPhysicalEntity *pent;
	if (m_szSensor<=0.0f) 
		return;
	for(i=0;i<2;i++) if (m_pTiedTo[i]==pentOrig) {
		pe_params_rope pr;
		pent = pentNew ? pentNew:pentOrig;
		ipart = pent->TouchesSphere(m_segs[i*m_nSegs].pt,m_szSensor);
		if (ipart>=0) {
			pr.pEntTiedTo[i] = pent;
			pr.idPartTiedTo[i] = pent->m_parts[ipart].id;
		} else if (!pentNew)
			pr.pEntTiedTo[i] = 0;
		else 
			continue;
		SetParams(&pr);
		Awake();
	}
}


int CRopeEntity::GetStateSnapshot(TSerialize ser, float time_back, int flags)
{
	int i;
	bool bAwake;
	ser.Value("nsegs", m_nSegs);

	if (m_nSegs<=0) {
		ser.Value("strained", bAwake = true);
		ser.Value("pos_start", m_pos);
		ser.Value("pos_end", m_pos);
	} else if (!m_bAwake && (m_segs[m_nSegs].pt-m_segs[0].pt).len2() > sqr(m_length)) {
		ser.Value("strained", bAwake = true);
		ser.Value("pos_start", m_segs[0].pt);
		ser.Value("pos_end", m_segs[m_nSegs].pt);
	} else {
		ser.Value("strained", false);
		ser.Value("awake", bAwake = m_bAwake!=0);

		ser.BeginGroup("Segs");
		for(i=0;i<=m_nSegs;i++) {
			ser.Value(numbered_tag("pos",i), m_segs[i].pt);
			if (m_bAwake)
			{
				ser.Value(numbered_tag("vel",i), m_segs[i].vel);
			}
		}
		ser.EndGroup();

		ser.Value("nvtx", m_nVtx);
		if (m_nVtx > 0)
		{
			ser.BeginGroup("Verts");
			for(i=0;i<m_nVtx;i++) {
				ser.Value(numbered_tag("pos",i), m_vtx[i].pt);
				if (m_bAwake)
				{
					ser.Value(numbered_tag("vel",i), m_vtx[i].vel);
				}
			}
			ser.EndGroup();
		}

		ser.Value("pt_tied0", m_ptTiedLoc[0]);
		ser.Value("pt_tied1", m_ptTiedLoc[1]);
	}

	if (!m_bTargetPoseActive && (m_pTiedTo[0] && m_pTiedTo[1] || 
			m_pTiedTo[0] && m_pTiedTo[0]->m_iSimClass<3 && (!m_pForeignData || m_pForeignData!=m_pTiedTo[0]->m_pForeignData) || 
			m_pTiedTo[1] && m_pTiedTo[1]->m_iSimClass<3 && (!m_pForeignData || m_pForeignData!=m_pTiedTo[1]->m_pForeignData)))
	{
		ser.Value("saveties", bAwake=true);
		for(i=0;i<2;i++) 
		{
			ser.BeginGroup("link");
			if (m_pTiedTo[i]) {
				ser.Value("tied", bAwake=true);
				m_pWorld->SavePhysicalEntityPtr(ser, m_pTiedTo[i]);
				ser.Value("ipart", m_iTiedPart[i]);
			}	else
				ser.Value("tied", bAwake=false);
			ser.EndGroup();
		}
	} else
		ser.Value("saveties", bAwake=false);

	return 1;
}

int CRopeEntity::SetStateFromSnapshot(TSerialize ser, int flags)
{
	int i;
	bool bAwake;
	int nSegs;

	ser.Value("nsegs", nSegs);
	if ((unsigned int)nSegs>300u)
		return 0;

	if (m_nSegs!=nSegs) {
		if (m_segs) delete[] m_segs;
		memset(m_segs = new rope_segment[nSegs+1], 0, (nSegs+1)*sizeof(rope_segment));
		m_nSegs = nSegs;
	}

	if (m_vtx) delete[] m_vtx; m_vtx=0;
	if (m_vtx1) delete[] m_vtx1; m_vtx1=0;
	if (m_vtxSolver) delete[] m_vtxSolver; m_vtxSolver=0;

	ser.Value("strained", bAwake);
	if (bAwake) {
		ser.Value("pos_start", m_segs[0].pt);
		ser.Value("pos_end", m_segs[m_nSegs].pt);
		Vec3 step = (m_segs[m_nSegs].pt-m_segs[0].pt)/m_nSegs;
		for(i=1;i<m_nSegs;i++)
			m_segs[i].pt0=m_segs[i].pt=m_segs[0].pt+step*i, m_segs[i].vel.zero();
		m_segs[0].vel.zero(); m_segs[m_nSegs].vel.zero();

		if (m_flags & rope_subdivide_segs) {
			memset(m_vtx = new rope_vtx[m_nVtxAlloc=(m_nSegs+1)*2], 0, (m_nSegs+1)*sizeof(rope_vtx));
			m_vtx1 = new rope_vtx[m_nVtxAlloc];
			for(i=0;i<=m_nSegs;i++)
				m_vtx[i].pt=m_segs[i].pt, MARK_UNUSED m_vtx[i].ncontact;
			m_nVtx = m_nSegs+1;
		}
	} else {
		ser.Value("awake", bAwake);
		m_bAwake = bAwake;

		ser.BeginGroup("Segs");
		for(i=0;i<=m_nSegs;i++) {
			ser.Value(numbered_tag("pos",i), m_segs[i].pt);
			m_segs[i].pt0 = m_segs[i].pt;
			if (m_bAwake)
				ser.Value(numbered_tag("vel",i), m_segs[i].vel);
			else m_segs[i].vel.zero();
			if (m_segs[i].pContactEnt) {
				if (!(m_flags & rope_subdivide_segs))
					m_segs[i].pContactEnt->Release();
				m_segs[i].pContactEnt = 0;
			}
			MARK_UNUSED m_segs[i].ncontact;
		}
		ser.EndGroup();

		ser.Value("nvtx", m_nVtx);
		if (m_nVtx>0) {
			memset(m_vtx =new rope_vtx[m_nVtxAlloc=(m_nSegs+1)*2], 0, m_nVtx*sizeof(rope_vtx));
			memset(m_vtx1=new rope_vtx[m_nVtxAlloc], 0, m_nVtx*sizeof(rope_vtx));
			m_vtxSolver = new rope_solver_vtx[m_nVtxAlloc];

			ser.BeginGroup("Verts");
			for(i=0;i<m_nVtx;i++) {
				ser.Value(numbered_tag("pos",i), m_vtx[i].pt);
				if (m_bAwake)
					ser.Value(numbered_tag("vel",i), m_vtx[i].vel);
				MARK_UNUSED m_vtx[i].ncontact;
			}
			ser.EndGroup();
		}

		ser.Value("pt_tied0", m_ptTiedLoc[0]);
		ser.Value("pt_tied1", m_ptTiedLoc[1]);
	}

	ser.Value("saveties", bAwake);
	if (bAwake) {
		pe_params_rope pr;
		for(i=0;i<2;i++) 
		{
			ser.BeginGroup("link");
			ser.Value("tied", bAwake);
			if (bAwake) {
				pr.pEntTiedTo[i] = m_pWorld->LoadPhysicalEntityPtr(ser);
				ser.Value("ipart", pr.idPartTiedTo[i]);
				if (pr.pEntTiedTo[i] && pr.pEntTiedTo[i]->GetType()!=PE_ROPE)
					pr.idPartTiedTo[i] = ((CPhysicalEntity*)pr.pEntTiedTo[i])->m_parts[pr.idPartTiedTo[i]].id;
			} else
				pr.pEntTiedTo[i] = 0;
			ser.EndGroup();
		}
		SetParams(&pr);
	}

	m_BBox[0]=m_BBox[1] = m_segs[m_nSegs].pt;
	for(i=0;i<m_nSegs;i++) {
		m_segs[i].dir = (m_segs[i+1].pt-m_segs[i].pt).normalized();
		m_BBox[0] = min(m_BBox[0], m_segs[i].pt);
		m_BBox[1] = max(m_BBox[1], m_segs[i].pt);
	}
	m_pos = m_segs[0].pt;
	if (m_flags & pef_traceable)
		AtomicAdd(&m_pWorld->m_lockGrid,-m_pWorld->RepositionEntity(this,1));

	return 1;
}


void CRopeEntity::DrawHelperInformation(IPhysRenderer *pRenderer, int flags)
{
	CPhysicalEntity::DrawHelperInformation(pRenderer,flags);

	int i;
	flags |= 16;
	if (flags & pe_helper_geometry) {
		ReadLock lock(m_lockUpdate);
		int nSegs;
		strided_pointer<Vec3> vtx;
		if (m_flags & rope_subdivide_segs && m_nVtx>0) { 
			vtx = strided_pointer<Vec3>(&m_vtx[0].pt0,sizeof(rope_vtx));
			nSegs = m_nVtx0-1;
		} else { 
			vtx = strided_pointer<Vec3>(&m_segs[0].pt0,sizeof(rope_segment));
			nSegs = m_nSegs;
		}
		if (flags & 16) {
			cylinder cyl; cyl.r=m_collDist;
			CCylinderGeom cylGeom;
			for(i=0;i<nSegs;i++) {
				cyl.center = (vtx[i]+vtx[i+1])*0.5f;
				cyl.axis = vtx[i+1]-vtx[i];
				cyl.hh=cyl.axis.len(); cyl.axis/=max(1e-10f,cyl.hh); cyl.hh*=0.5f;
				cylGeom.CreateCylinder(&cyl);
				pRenderer->DrawGeometry(&cylGeom,0,m_iSimClass);
			}
		} else for(i=0;i<nSegs;i++)
			pRenderer->DrawLine(vtx[i],vtx[i+1],m_iSimClass);
		if (m_flags & rope_subdivide_segs && flags & 16) for(i=0;i<m_nSegs;i++) //if (m_segs[i+1].iVtx0>m_segs[i].iVtx0+1)
			pRenderer->DrawLine(m_segs[i].pt,m_segs[i+1].pt,m_iSimClass);
	}
	if (flags & pe_helper_collisions) if (!(m_flags & rope_subdivide_segs)) {
		for(i=0;i<m_nSegs;i++) if (m_segs[i].pContactEnt) {
			Vec3 pt = m_segs[i].pt+(m_segs[i+1].pt-m_segs[i].pt)*m_segs[i].tcontact;
			pRenderer->DrawLine(pt,pt+m_segs[i].ncontact*m_pWorld->m_vars.maxContactGap*30,m_iSimClass);
		}
	} else {
		for(i=0;i<m_nVtx;i++) if (!is_unused(m_vtx[i].ncontact))
			pRenderer->DrawLine(m_vtx[i].pt,m_vtx[i].pt+m_vtx[i].ncontact*m_pWorld->m_vars.maxContactGap*30,m_iSimClass);
		/*if (m_pMesh) {
			geom_world_data gwd;
			gwd.offset = m_lastMeshOffs;
			m_pMesh->DrawWireframe(pRenderer,&gwd,0,m_iSimClass|256);
		}*/
	}
}


void CRopeEntity::GetMemoryStatistics(ICrySizer *pSizer) const
{
	CPhysicalEntity::GetMemoryStatistics(pSizer);
	if (GetType()==PE_ROPE)
		pSizer->AddObject(this, sizeof(CRopeEntity));
	pSizer->AddObject(m_segs, m_nSegs*sizeof(m_segs[0]));
}


float CRopeEntity::ComputeExtent(GeomQuery& geo, EGeomForm eForm)	const
{
	if (m_length>0)
		return m_length;
	ReadLock lock(m_lockUpdate);
	float len=0;
	for(int i=0;i<m_nSegs;i++)
		len += (m_segs[i+1].pt0-m_segs[i].pt0).len();
	return len;
}

void CRopeEntity::GetRandomPos(RandomPos& ran, GeomQuery& geo, EGeomForm eForm) const
{
	ReadLock lock(m_lockUpdate);
	int i,nSegs;
	strided_pointer<Vec3> vtx;
	if (m_flags & rope_subdivide_segs && m_nVtx>0) { 
		vtx = strided_pointer<Vec3>(&m_vtx[0].pt0,sizeof(rope_vtx));
		nSegs = m_nVtx0-1;
	} else { 
		vtx = strided_pointer<Vec3>(&m_segs[0].pt0,sizeof(rope_segment));
		nSegs = m_nSegs;
	}
	i = Random(nSegs);
	ran.vPos = vtx[i]+(vtx[i+1]-vtx[i])*Random();
	Vec3 dir=(vtx[i+1]-vtx[i]).normalized(), axisx=dir.GetOrthogonal().normalized(), axisy=dir^axisx;
	float angle=Random(2*gf_PI);
	ran.vNorm = axisx*cos_tpl(angle)+axisy*sin_tpl(angle);
}
#endif//__SPU__
