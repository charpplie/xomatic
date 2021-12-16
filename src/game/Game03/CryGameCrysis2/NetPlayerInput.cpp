#include "StdAfx.h"
#include "NetPlayerInput.h"
#include "Player.h"
#include "Game.h"
#include "GameCVars.h"
#include "Weapon.h"
#include "IAIActor.h"

/*
moveto (P1);                            // move pen to startpoint
for (int t=0; t < steps; t++)
{
float s = (float)t / (float)steps;    // scale s to go from 0 to 1
float h1 =  2s^3 - 3s^2 + 1;          // calculate basis function 1
float h2 = -2s^3 + 3s^2;              // calculate basis function 2
float h3 =   s^3 - 2*s^2 + s;         // calculate basis function 3
float h4 =   s^3 -  s^2;              // calculate basis function 4
vector p = h1*P1 +                    // multiply and sum all funtions
h2*P2 +                    // together to build the interpolated
h3*T1 +                    // point along the curve.
h4*T2;
lineto (p)                            // draw to calculated point on the curve
}*/

static Vec3 HermiteInterpolate( float s, const Vec3& p1, const Vec3& t1, const Vec3& p2, const Vec3& t2 )
{
	float s2 = s*s;
	float s3 = s2*s;
	float h1 = 2*s3 - 3*s2 + 1.0f;
	float h2 = -2*s3 + 3*s2;
	float h3 = s3 - 2*s2 + s;
	float h4 = s3 - s2;
	return h1*p1 + h2*p2 + h3*t1 + h4*t2;
}


CNetPlayerInput::CNetPlayerInput( CPlayer * pPlayer ) : m_pPlayer(pPlayer)
{
}

void CNetPlayerInput::PreUpdate()
{
	IPhysicalEntity * pPhysEnt = m_pPlayer->GetEntity()->GetPhysics();
	if (!pPhysEnt)
		return;

	CMovementRequest moveRequest;
	SMovementState moveState;
	m_pPlayer->GetMovementController()->GetMovementState(moveState);
	Quat worldRot = m_pPlayer->GetBaseQuat(); // m_pPlayer->GetEntity()->GetWorldRotation();
	Vec3 deltaMovement = worldRot.GetInverted().GetNormalized() * m_curInput.deltaMovement;
	// absolutely ensure length is correct
	deltaMovement = deltaMovement.GetNormalizedSafe(ZERO) * m_curInput.deltaMovement.GetLength();
	moveRequest.AddDeltaMovement( deltaMovement );
	if( IsDemoPlayback() )
	{
		Vec3 localVDir(m_pPlayer->GetViewQuatFinal().GetInverted() * m_curInput.lookDirection);
		Ang3 deltaAngles(asinf(localVDir.z),0,cry_atan2f(-localVDir.x,localVDir.y));
		moveRequest.AddDeltaRotation(deltaAngles*gEnv->pTimer->GetFrameTime());
	}
	//else
	{
		Vec3 distantTarget = moveState.eyePosition + 1000.0f * m_curInput.lookDirection;
		Vec3 lookTarget = distantTarget;

		if (m_curInput.usinglookik)
			moveRequest.SetLookTarget( lookTarget );
		else
			moveRequest.ClearLookTarget();

		if (m_curInput.aiming)
			moveRequest.SetAimTarget( lookTarget );
		else
			moveRequest.ClearAimTarget();

		if (m_curInput.deltaMovement.GetLengthSquared() > sqr(0.02f)) // 0.2f is almost stopped
			moveRequest.SetBodyTarget( distantTarget );
		else
			moveRequest.ClearBodyTarget();
	}

	moveRequest.SetAllowStrafing(m_curInput.allowStrafing);

	moveRequest.SetPseudoSpeed(CalculatePseudoSpeed());

	moveRequest.SetStance( (EStance)m_curInput.stance );
	
	m_pPlayer->GetMovementController()->RequestMovement(moveRequest);

	if (m_curInput.sprint)
		m_pPlayer->m_actions |= ACTION_SPRINT;
	else
		m_pPlayer->m_actions &= ~ACTION_SPRINT;

	// debug..
	if (g_pGameCVars->g_debugNetPlayerInput & 2)
	{
		IPersistantDebug * pPD = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
		pPD->Begin( string("update_player_input_") + m_pPlayer->GetEntity()->GetName(), true );
		Vec3 wp = m_pPlayer->GetEntity()->GetWorldPos();
		wp.z += 2.0f;
		pPD->AddSphere( moveRequest.GetLookTarget(), 0.5f, ColorF(1,0,1,0.3f), 1.0f );
		//		pPD->AddSphere( moveRequest.GetMoveTarget(), 0.5f, ColorF(1,1,0,0.3f), 1.0f );
		pPD->AddDirection( m_pPlayer->GetEntity()->GetWorldPos() + Vec3(0,0,2), 1, m_curInput.deltaMovement, ColorF(1,0,0,0.3f), 1.0f );
	}

	//m_curInput.deltaMovement.zero();
}

void CNetPlayerInput::Update()
{
	if (gEnv->bServer && (g_pGameCVars->sv_input_timeout>0) && ((gEnv->pTimer->GetFrameStartTime()-m_lastUpdate).GetMilliSeconds()>=g_pGameCVars->sv_input_timeout))
	{
		m_curInput.deltaMovement.zero();
		m_curInput.sprint=m_curInput.leanl=m_curInput.leanr=false;
		m_curInput.stance=(uint8)STANCE_NULL;

		CHANGED_NETWORK_STATE(m_pPlayer,  CPlayer::ASPECT_INPUT_CLIENT );
	}
}

void CNetPlayerInput::PostUpdate()
{
}

void CNetPlayerInput::SetState( const SSerializedPlayerInput& input )
{
	DoSetState(input);

	m_lastUpdate = gEnv->pTimer->GetCurrTime();
}

void CNetPlayerInput::GetState( SSerializedPlayerInput& input )
{
	input = m_curInput;
}

void CNetPlayerInput::Reset()
{
	SSerializedPlayerInput i(m_curInput);
	i.leanl=i.leanr=i.sprint=false;
	i.deltaMovement.zero();

	DoSetState(i);

	CHANGED_NETWORK_STATE(m_pPlayer, CPlayer::ASPECT_INPUT_CLIENT);
}

void CNetPlayerInput::DisableXI(bool disabled)
{
}

float CNetPlayerInput::CalculatePseudoSpeed() const
{
	float curSpeed = m_pPlayer->GetActorStats()->velocity.len();
	float curInputSpeed = m_curInput.deltaMovement.len();

#if 0
	const float XPOS = 500.0f;
	const float YPOS = 100.0f;
	const float COLOUR[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, 2.0f, COLOUR, false, "CurSpeed: %.2f CurISpeed: %f SetPos: %d LerpSpeed: %.2f", curSpeed, curInputSpeed, m_pPlayer->m_netSetPosition, m_pPlayer->m_netLerpSpeed);
#endif 

	if (g_pGameCVars->pl_velocityBasedInterpolation)
	{
		return m_pPlayer->CalculatePseudoSpeed(m_curInput.sprint);
	}
	else
	{
		if (m_pPlayer->m_netSetPosition)
		{
			curInputSpeed = max(curInputSpeed, m_pPlayer->m_netLerpSpeed);
		}

		return m_pPlayer->CalculatePseudoSpeed(m_curInput.sprint, curInputSpeed);
	}
}

void CNetPlayerInput::DoSetState(const SSerializedPlayerInput& input )
{
	m_curInput = input;
	CHANGED_NETWORK_STATE(m_pPlayer,  CPlayer::ASPECT_INPUT_CLIENT );

	// not having these set seems to stop a remote avatars rotation being reflected
	m_curInput.aiming = true;
	m_curInput.allowStrafing = true;
	m_curInput.usinglookik = true;

	IAIActor* pAIActor = CastToIAIActorSafe(m_pPlayer->GetEntity()->GetAI());
	if (pAIActor)
		pAIActor->GetState()->bodystate=input.bodystate;

	CMovementRequest moveRequest;
	moveRequest.SetStance( (EStance)m_curInput.stance );

	if(IsDemoPlayback())
	{
		Vec3 localVDir(m_pPlayer->GetViewQuatFinal().GetInverted() * m_curInput.lookDirection);
		Ang3 deltaAngles(asinf(localVDir.z),0,cry_atan2f(-localVDir.x,localVDir.y));
		moveRequest.AddDeltaRotation(deltaAngles*gEnv->pTimer->GetFrameTime());
	}
	{
		if (m_curInput.usinglookik)
			moveRequest.SetLookTarget( m_pPlayer->GetEntity()->GetWorldPos() + 10.0f * m_curInput.lookDirection );
		else
			moveRequest.ClearLookTarget();
		if (m_curInput.aiming)
			moveRequest.SetAimTarget(moveRequest.GetLookTarget());
		else
			moveRequest.ClearAimTarget();
	}
/*
	float pseudoSpeed = 0.0f; 
	if (m_curInput.deltaMovement.len2() > 0.0f)
	{
		pseudoSpeed = m_pPlayer->CalculatePseudoSpeed(m_curInput.sprint);
	}
	*/
	moveRequest.SetPseudoSpeed(CalculatePseudoSpeed());
	moveRequest.SetAllowStrafing(input.allowStrafing);

	float lean=0.0f;
	if (m_curInput.leanl)
		lean-=1.0f;
	if (m_curInput.leanr)
		lean+=1.0f;
	moveRequest.SetLean(lean);

	m_pPlayer->GetMovementController()->RequestMovement(moveRequest);

	IAnimationGraphState *pState=0;
	if (m_pPlayer->GetAnimatedCharacter())
		pState=m_pPlayer->GetAnimatedCharacter()->GetAnimationGraphState();

	if (pState)
	{
		pState->SetInput(m_pPlayer->m_inputAiming, m_curInput.aiming);
		pState->SetInput(m_pPlayer->m_inputUsingLookIK, m_curInput.usinglookik);
	}


	// debug..
	if (g_pGameCVars->g_debugNetPlayerInput & 1)
	{
		IPersistantDebug * pPD = gEnv->pGame->GetIGameFramework()->GetIPersistantDebug();
		pPD->Begin( string("net_player_input_") + m_pPlayer->GetEntity()->GetName(), true );
		pPD->AddSphere( moveRequest.GetLookTarget(), 0.5f, ColorF(1,0,1,1), 1.0f );
		//			pPD->AddSphere( moveRequest.GetMoveTarget(), 0.5f, ColorF(1,1,0,1), 1.0f );

		Vec3 wp(m_pPlayer->GetEntity()->GetWorldPos() + Vec3(0,0,2));
		pPD->AddDirection( wp, 1.5f, m_curInput.deltaMovement, ColorF(1,0,0,1), 1.0f );
		pPD->AddDirection( wp, 1.5f, m_curInput.lookDirection, ColorF(0,1,0,1), 1.0f );
	}
}
