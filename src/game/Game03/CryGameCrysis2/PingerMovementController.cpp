//------------------------------------------
// Movement controller for prototype pinger
//
// 11-08-08: Benito G.R. 
//-------------------------------------------

#include "StdAfx.h"
#include "PingerMovementController.h"

#include "Pinger.h"

CPingerMovementController::CPingerMovementController(CPinger* pPinger)
{
	m_pPinger = pPinger;
}

CPingerMovementController::~CPingerMovementController()
{

}

//--------------------------------------------------
void CPingerMovementController::Release()
{
	delete this;
}

//-------------------------------------------------
bool CPingerMovementController::RequestMovement(CMovementRequest& request )
{
	SMovementState state;
	GetMovementState(state);

	const Vec3 currentPos = state.pos;

	CPinger::SMovementRequestParams os (request);

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

	if (request.HasLookTarget())
		m_currentMovementRequest.SetLookTarget( request.GetLookTarget() );
	else if (request.RemoveLookTarget())
		m_currentMovementRequest.ClearLookTarget();

	m_pPinger->SetActorMovement(os);

	return true;
}

//-----------------------------------------
bool CPingerMovementController::Update(float frameTime, SActorFrameMovementParams& params )
{
	UpdateCurMovementState(params);
	return false;
}

//------------------------------------------
void CPingerMovementController::UpdateCurMovementState(const SActorFrameMovementParams& params)
{
	SMovementState& state(m_currentMovementState);
	CPinger::SBodyInfo bodyInfo;
	m_pPinger->GetActorInfo( bodyInfo );
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
	state.bodyDirection = m_pPinger->GetEntity()->GetWorldRotation() * Vec3(0,1,0);

	if(m_currentMovementRequest.HasAimTarget())
		state.aimDirection = (m_currentMovementRequest.GetAimTarget()-state.weaponPosition).GetNormalizedSafe();
	else
		state.aimDirection = bodyInfo.vFireDir.GetNormalizedSafe();

	if(m_currentMovementRequest.HasFireTarget())
		state.fireDirection = (m_currentMovementRequest.GetFireTarget()-state.weaponPosition).GetNormalizedSafe(state.aimDirection);
	else
		state.fireDirection = state.aimDirection;

	if (m_currentMovementRequest.HasLookTarget())
		state.eyeDirection =  (m_currentMovementRequest.GetLookTarget()-state.eyePosition).GetNormalizedSafe(state.aimDirection);

	state.isAlive = (m_pPinger->GetHealth()>0);

	state.isAiming = true;

	state.pos = m_pPinger->GetEntity()->GetWorldPos();

	//state.isFiring = (m_pPinger->GetActorStats()->inFiring>0.001f);

	if(m_currentMovementRequest.HasFireTarget())
		state.fireTarget = m_currentMovementRequest.GetFireTarget();
}

//------------------------------------------
bool CPingerMovementController::GetStanceState(const SStanceStateQuery& query, SStanceState& state )
{
	const SStanceInfo*	pStance = m_pPinger->GetStanceInfo(query.stance);
	if(!pStance)
		return false;

	state.lean = query.lean;
	state.peekOver = query.peekOver;

	if(query.defaultPose)
	{
		state.pos.Set(0,0,0);
		state.bodyDirection = FORWARD_DIRECTION;
		state.upDirection(0,0,1);
		state.weaponPosition = m_pPinger->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pPinger->GetEyeOffset());
		state.aimDirection = FORWARD_DIRECTION;
		state.fireDirection = FORWARD_DIRECTION;
		state.eyePosition = pStance->GetViewOffsetWithLean(query.lean, query.peekOver);
		state.eyeDirection = FORWARD_DIRECTION;
		state.m_StanceSize = pStance->GetStanceBounds();
		state.m_ColliderSize = pStance->GetColliderBounds();
	}
	else
	{
		CPinger::SBodyInfo bodyInfo;
		m_pPinger->GetActorInfo( bodyInfo );

		Matrix34	tm = m_pPinger->GetEntity()->GetWorldTM();

		state.pos = m_pPinger->GetEntity()->GetWorldPos();
		state.bodyDirection = bodyInfo.vFwdDir;
		state.upDirection = bodyInfo.vUpDir;
		state.weaponPosition = tm.TransformPoint(m_pPinger->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pPinger->GetEyeOffset()));
		state.aimDirection = bodyInfo.vFireDir;
		state.fireDirection = bodyInfo.vFireDir;
		state.eyePosition = tm.TransformPoint(pStance->GetViewOffsetWithLean(query.lean, query.peekOver));
		state.eyeDirection = bodyInfo.vEyeDir;
		state.m_StanceSize = pStance->GetStanceBounds();
		state.m_ColliderSize = pStance->GetColliderBounds();
	}

	return true;
}

void CPingerMovementController::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_pPinger);
}