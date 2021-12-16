//------------------------------------------
// Movement controller for prototype Spotter
//
// 11-08-08: Benito G.R. 
//-------------------------------------------

#include "StdAfx.h"
#include "SpotterMovementController.h"

#include "Spotter.h"

CSpotterMovementController::CSpotterMovementController(CSpotter* pSpotter)
{
	m_pSpotter = pSpotter;
}

CSpotterMovementController::~CSpotterMovementController()
{

}

//--------------------------------------------------
void CSpotterMovementController::Release()
{
	delete this;
}

//-------------------------------------------------
bool CSpotterMovementController::RequestMovement(CMovementRequest& request )
{
	SMovementState state;
	GetMovementState(state);

	const Vec3 currentPos = state.pos;

	CSpotter::SMovementRequestParams os (request);

	if (request.HasMoveTarget())
	{
		os.vMoveDir = (request.GetMoveTarget() - currentPos).GetNormalizedSafe();

		float distanceToEnd(request.GetDistanceToPathEnd());
		if (distanceToEnd>0.001f)
			m_currentMovementRequest.SetDistanceToPathEnd(distanceToEnd);

		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(request.GetMoveTarget(), 0.5f, ColorB(255, 0, 0));
	}
	else if(request.RemoveMoveTarget())
	{
		m_currentMovementRequest.ClearMoveTarget();
		m_currentMovementRequest.ClearDesiredSpeed();
		m_currentMovementRequest.ClearDistanceToPathEnd();
	}

	if (request.HasFireTarget())
		m_currentMovementRequest.SetFireTarget( request.GetFireTarget() );
	else if (request.RemoveFireTarget())
		m_currentMovementRequest.ClearFireTarget();

	m_pSpotter->SetActorMovement(os);

	return true;
}

//-----------------------------------------
bool CSpotterMovementController::Update(float frameTime, SActorFrameMovementParams& params )
{
	UpdateCurMovementState(params);

	return false;
}

//------------------------------------------
void CSpotterMovementController::UpdateCurMovementState(const SActorFrameMovementParams& params)
{
	SMovementState& state(m_currentMovementState);
	CSpotter::SBodyInfo bodyInfo;
	m_pSpotter->GetActorInfo( bodyInfo );
	state.maxSpeed = bodyInfo.maxSpeed;
	state.minSpeed = bodyInfo.minSpeed;
	state.normalSpeed = bodyInfo.normalSpeed;
	
	state.stance = bodyInfo.stance;
	state.m_StanceSize		= bodyInfo.m_stanceSizeAABB;
	state.m_ColliderSize	= bodyInfo.m_colliderSizeAABB;
	state.eyeDirection = bodyInfo.vEyeDir;
	state.animationEyeDirection = bodyInfo.vEyeDirAnim;
	state.eyePosition = bodyInfo.vEyePos;
	state.weaponPosition = bodyInfo.vFirePos;
	state.movementDirection = bodyInfo.vFwdDir;
	state.upDirection = bodyInfo.vUpDir;
	state.atMoveTarget = false; //m_atTarget;
	state.bodyDirection = m_pSpotter->GetEntity()->GetWorldRotation() * Vec3(0,1,0);

	if(m_currentMovementRequest.HasAimTarget())
		state.aimDirection = (m_currentMovementRequest.GetAimTarget()-state.weaponPosition).GetNormalizedSafe();
	else
		state.aimDirection = bodyInfo.vFireDir.GetNormalizedSafe();

	if(m_currentMovementRequest.HasFireTarget())
		state.fireDirection = (m_currentMovementRequest.GetFireTarget()-state.weaponPosition).GetNormalizedSafe(state.aimDirection);
	else
		state.fireDirection = state.aimDirection;

	state.isAlive = (m_pSpotter->GetHealth()>0);

	state.isAiming = true;

	state.pos = m_pSpotter->GetEntity()->GetWorldPos();

	//state.isFiring = (m_pSpotter->GetActorStats()->inFiring>0.001f);

	if(m_currentMovementRequest.HasFireTarget())
		state.fireTarget = m_currentMovementRequest.GetFireTarget();
}

//------------------------------------------
bool CSpotterMovementController::GetStanceState(const SStanceStateQuery& query, SStanceState& state )
{
	const SStanceInfo*	pStance = m_pSpotter->GetStanceInfo(query.stance);
	if(!pStance)
		return false;

	state.lean = query.lean;
	state.peekOver = query.peekOver;

	if(query.defaultPose)
	{
		state.pos.Set(0,0,0);
		state.bodyDirection = FORWARD_DIRECTION;
		state.upDirection(0,0,1);
		state.weaponPosition = m_pSpotter->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pSpotter->GetEyeOffset());
		state.aimDirection = FORWARD_DIRECTION;
		state.fireDirection = FORWARD_DIRECTION;
		state.eyePosition = pStance->GetViewOffsetWithLean(query.lean, query.peekOver);
		state.eyeDirection = FORWARD_DIRECTION;
		state.m_StanceSize = pStance->GetStanceBounds();
		state.m_ColliderSize = pStance->GetColliderBounds();
	}
	else
	{
		CSpotter::SBodyInfo bodyInfo;
		m_pSpotter->GetActorInfo( bodyInfo );

		Matrix34	tm = m_pSpotter->GetEntity()->GetWorldTM();

		state.pos = m_pSpotter->GetEntity()->GetWorldPos();
		state.bodyDirection = bodyInfo.vFwdDir;
		state.upDirection = bodyInfo.vUpDir;
		state.weaponPosition = tm.TransformPoint(m_pSpotter->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pSpotter->GetEyeOffset()));
		state.aimDirection = bodyInfo.vFireDir;
		state.fireDirection = bodyInfo.vFireDir;
		state.eyePosition = tm.TransformPoint(pStance->GetViewOffsetWithLean(query.lean, query.peekOver));
		state.eyeDirection = bodyInfo.vEyeDir;
		state.m_StanceSize = pStance->GetStanceBounds();
		state.m_ColliderSize = pStance->GetColliderBounds();
	}

	return true;
}

void CSpotterMovementController::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_pSpotter);
}