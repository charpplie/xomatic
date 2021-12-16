//--------------------------------------------------------------------------------

#include "StdAfx.h"
#include "AnimatedCharacter.h"
#include "CryAction.h"
#include "AnimationGraphManager.h"
#include "AnimationGraphCVars.h"
#include "HumanBlending.h"
#include "PersistantDebug.h"
#include "IFacialAnimation.h"

#include <IViewSystem.h>

//--------------------------------------------------------------------------------

float ApplyAntiOscilationFilter(float value, float filtersize)
{
	float filterfraction = clamp(abs(value) / filtersize, 0.0f, 1.0f);
	float filter = (0.5f - 0.5f * cry_cosf(filterfraction * gf_PI));
	value *= filter;
	return value;
}

//--------------------------------------------------------------------------------

void SmoothCDQuat(Quat& current, Quat& delta, const Quat& target, float frameTime, float smoothTime)
{
	float dot = current | target;
	if (dot < 0.0f) current = -current;
	SmoothCD(current, delta, frameTime, target, smoothTime);
	current.Normalize();
	//delta.Normalize();
}

//--------------------------------------------------------------------------------

float GetQuatAbsAngle(const Quat& q)
{
	//float fwd = q.GetColumn1().y;
	float fwd = q.GetFwdY();
	fwd = clamp(fwd, -1.0f, 1.0f);
	return cry_acosf(fwd);
}

//--------------------------------------------------------------------------------

f32 GetYaw( const Vec3& v0, const Vec3& v1)
{
  float a0 = atan2f(v0.y, v0.x);
  float a1 = atan2f(v1.y, v1.x);
  float a = a1 - a0;
  if (a > gf_PI) a -= gf_PI2;
  else if (a < -gf_PI) a += gf_PI2;
  return a;
}

//--------------------------------------------------------------------------------

f32 GetYaw( const Vec2& v0, const Vec2& v1)
{
	Vec3 _v0= Vec3(v0.x, v0.y, 0);
	Vec3 _v1= Vec3(v1.x, v1.y, 0);
	return GetYaw( _v0, _v1);
}

//--------------------------------------------------------------------------------

QuatT ApplyWorldOffset(const QuatT& origin, const QuatT& offset)
{
	CRY_ASSERT(origin.IsValid());
	CRY_ASSERT(offset.IsValid());
	QuatT destination(origin.q * offset.q, origin.t + offset.t);
	destination.q.Normalize();
	CRY_ASSERT(destination.IsValid());
	return destination;
}

//--------------------------------------------------------------------------------

QuatT GetWorldOffset(const QuatT& origin, const QuatT& destination)
{
	CRY_ASSERT(origin.IsValid());
	CRY_ASSERT(destination.IsValid());
	QuatT offset(destination.t - origin.t, origin.q.GetInverted() * destination.q);
	offset.q.Normalize();
	CRY_ASSERT(offset.IsValid());
	return offset;
}

//--------------------------------------------------------------------------------

// Returns the actual unclamped distance and angle.
QuatT GetClampedOffset(const QuatT& offset, float maxDistance, float maxAngle, float& distance, float& angle)
{
	QuatT clampedOffset;

	distance = offset.t.GetLength();
	if (distance==0.0f)	distance=0.000001f;
	if (distance > maxDistance)
	{
		clampedOffset.t = offset.t * maxDistance / distance;
	}
	else
	{
		clampedOffset.t = offset.t;
	}

	angle = RAD2DEG(GetQuatAbsAngle(offset.q));
	if (angle > maxAngle)
	{
		clampedOffset.q.SetNlerp(Quat(IDENTITY), offset.q, ( angle == 0.0f ) ? 0.0f : ( maxAngle / angle ) );
	}
	else
	{
		clampedOffset.q = offset.q;
	}

	return clampedOffset;
}

//--------------------------------------------------------------------------------

void CAnimatedCharacter::SetLocationClampingOverride(float distance, float angle)
{
	m_overrideClampDistance = distance;
	m_overrideClampAngle = angle;
};

//--------------------------------------------------------------------------------

Vec3 CAnimatedCharacter::RemovePenetratingComponent(const Vec3& v, const Vec3& n, float approachDistanceMin, float approachDistanceMax)
{
	Vec3 animOffset = m_animLocation.t - m_entLocation.t;
	float offsetAlignment = n.Dot(animOffset);
	float offsetFraction = clamp((offsetAlignment - approachDistanceMin) / (approachDistanceMax - approachDistanceMin), 0.0f, 1.0f);
	if (offsetFraction == 1.0f)
		return v;

	float penetration = n.Dot(v);
	if (penetration >= 0.0f)
		return v;

	return (v - n * penetration * (1.0f - offsetFraction));
}

//--------------------------------------------------------------------------------

QuatT /*CAnimatedCharacter::*/ExtractHComponent(const QuatT& m) /*const*/
{
	ANIMCHAR_PROFILE_DETAILED;

	// NOTE: This function assumes there is no pitch/bank and totally ignores XY rotations.

	CRY_ASSERT(m.IsValid());

	QuatT ext;//(IDENTITY);
	ext.t.x = m.t.x;
	ext.t.y = m.t.y;
	ext.t.z = 0.0f;

/*
	Ang3 a(m.q);
	a.x = 0.0f;
	a.y = 0.0f;
	ext.q.SetRotationXYZ(a);
*/
	ext.q.SetRotationZ(m.q.GetRotZ());

	CRY_ASSERT(ext.IsValid());
	return ext;
}

//--------------------------------------------------------------------------------

QuatT /*CAnimatedCharacter::*/ExtractVComponent(const QuatT& m) /*const*/
{
	ANIMCHAR_PROFILE_DETAILED;

	// NOTE: This function assumes there is no pitch/bank and totally ignores XY rotations.

	CRY_ASSERT(m.IsValid());

	QuatT ext;//(IDENTITY);
	ext.t.z = m.t.z;
	ext.t.x = 0.0f;
	ext.t.y = 0.0f;

/*
	Ang3 a(m.q);
	a.z = 0.0f;
	ext.q.SetRotationXYZ(a);
*/
	ext.q.SetIdentity();

	CRY_ASSERT(ext.IsValid());
	return ext;
}

//--------------------------------------------------------------------------------

QuatT CombineHVComponents2D(const QuatT& cmpH, const QuatT& cmpV)
{
	ANIMCHAR_PROFILE_DETAILED;

	// NOTE: This function assumes there is no pitch/bank and totally ignores XY rotations.

	CRY_ASSERT(cmpH.IsValid());
	CRY_ASSERT(cmpV.IsValid());

	QuatT cmb;
	cmb.t.x = cmpH.t.x;
	cmb.t.y = cmpH.t.y;
	cmb.t.z = cmpV.t.z;
	cmb.q.SetRotationZ(cmpH.q.GetRotZ());

	CRY_ASSERT(cmb.IsValid());

	return cmb;
}

//--------------------------------------------------------------------------------

QuatT CombineHVComponents3D(const QuatT& cmpH, const QuatT& cmpV)
{
	ANIMCHAR_PROFILE_DETAILED;

	//return CombineHVComponents2D(cmpH, cmpV);

	CRY_ASSERT(cmpH.IsValid());
	CRY_ASSERT(cmpV.IsValid());

	QuatT cmb;
	cmb.t.x = cmpH.t.x;
	cmb.t.y = cmpH.t.y;
	cmb.t.z = cmpV.t.z;

	// TODO: This should be optimized!
	Ang3 ah(cmpH.q);
	Ang3 av(cmpV.q);
	Ang3 a(av.x, av.y, ah.z);
	cmb.q.SetRotationXYZ(a);

	CRY_ASSERT(cmb.IsValid());

	return cmb;
}

//--------------------------------------------------------------------------------

QuatT CAnimatedCharacter::MergeMCM(const QuatT& ent, const QuatT& anim, const QuatT& decoupled, bool flat) const
{
	ANIMCHAR_PROFILE_DETAILED;

	CRY_ASSERT(ent.IsValid());
	CRY_ASSERT(anim.IsValid());
	CRY_ASSERT(decoupled.IsValid());
	
	EMovementControlMethod mcmh = GetMCMH();
	EMovementControlMethod mcmv = GetMCMV();

	CRY_ASSERT(mcmh >= 0 && mcmh < eMCM_COUNT);
	CRY_ASSERT(mcmv >= 0 && mcmv < eMCM_COUNT);

	if (mcmh == mcmv)
	{
		switch (mcmh /*or mcmv*/)
		{
		case eMCM_Entity:
		case eMCM_ClampedEntity:
		case eMCM_SmoothedEntity:
			return ent;
		case eMCM_DecoupledCatchUp:
			return decoupled;
		case eMCM_Animation:
		case eMCM_AnimationHCollision:
			return anim;
		default:
			GameWarning("CAnimatedCharacter::MergeMCM() - Horizontal & Vertical MCM %s not implemented!", g_szMCMString[mcmh]);
			return ent;
		}
	}

	QuatT mergedH, mergedV;
	switch (mcmh)
	{
		case eMCM_Entity:
		case eMCM_ClampedEntity:
		case eMCM_SmoothedEntity: 
			//mergedH = ExtractHComponent(ent); break;
			mergedH = ent; break;
		case eMCM_Animation:
		case eMCM_AnimationHCollision:
			//mergedH = ExtractHComponent(anim); break;
			mergedH = anim; break;
		case eMCM_DecoupledCatchUp: 
			//mergedH = ExtractHComponent(decoupled); break;
			mergedH = decoupled; break;
		default:
			//mergedH = ExtractHComponent(ent);
			mergedH = ent;
			GameWarning("CAnimatedCharacter::MergeMCM() - Horizontal MCM %s not implemented!", g_szMCMString[mcmh]);
			break;
	}
	switch (mcmv)
	{
		case eMCM_Entity: 
		case eMCM_ClampedEntity: 
		case eMCM_SmoothedEntity: 
			//mergedV = ExtractVComponent(ent); break;
			mergedV = ent; break;
		case eMCM_Animation: 
			//mergedV = ExtractVComponent(anim); break;
			mergedV = anim; break;
		case eMCM_DecoupledCatchUp: 
			//mergedV = ExtractVComponent(decoupled); break;
			mergedV = decoupled; break;
		default:
			//mergedV = ExtractVComponent(ent);
			mergedV = ent;
			GameWarning("CAnimatedCharacter::MergeMCM() - Vertical MCM %s not implemented!", g_szMCMString[mcmv]);
			break;
	}

	QuatT merged;
	if (flat)
		merged = CombineHVComponents2D(mergedH, mergedV);
	else
		merged = CombineHVComponents3D(mergedH, mergedV);

	CRY_ASSERT(merged.IsValid());
	return merged;
}

//--------------------------------------------------------------------------------

void CAnimatedCharacter::UpdateMCMs()
{
	m_movementControlMethod[eMCMComponent_Horizontal][eMCMSlot_Debug] = (EMovementControlMethod)CAnimationGraphCVars::Get().m_MCMH;
	m_movementControlMethod[eMCMComponent_Vertical][eMCMSlot_Debug] = (EMovementControlMethod)CAnimationGraphCVars::Get().m_MCMV;

	UpdateMCMComponent(eMCMComponent_Horizontal);
	UpdateMCMComponent(eMCMComponent_Vertical);

#ifdef _DEBUG
	if (DebugFilter() && ((CAnimationGraphCVars::Get().m_debugText != 0) || (CAnimationGraphCVars::Get().m_debugMovementControlMethods != 0)))
	{
		//gEnv->pRenderer->Draw2dLabel(10, 75, 2.0f, (float*)&ColorF(1,1,1,1), false, "MCM H[%s] V[%s]", g_szMCMString[mcmh], g_szMCMString[mcmv]);

		EMovementControlMethod mcmh = GetMCMH();
		EMovementControlMethod mcmv = GetMCMV();
		DebugHistory_AddValue("eDH_MovementControlMethodH", (float)mcmh);
		DebugHistory_AddValue("eDH_MovementControlMethodV", (float)mcmv);
	}
#endif
}

//--------------------------------------------------------------------------------

// NOTE: To make sure the dynamic steering by PlayerMovementController doesn't overshoot:
// - Force entity controlled clamping, so that the character 'warps' to the entity.
// - Tweak the fading in of the clamping to make sure it is fully entity controlled *before* we reach the target.
// - Force 'fly' mode in physics to disable momentum effects.
void CAnimatedCharacter::PreventAnimTargetOvershooting(EMCMComponent component)
{
	const float ANIMTARGET_FORCE_ENTITY_DRIVEN_DISTANCE = 4.0f;
	const float ANIMTARGET_FORCE_FLY_MODE_DISTANCE = 1.0f;

	if (m_pAnimTarget == NULL)
		return;

	EMovementControlMethod* mcm = m_movementControlMethod[component];
	float dist = m_pAnimTarget->position.GetDistance(m_entLocation.t);

	if (dist < ANIMTARGET_FORCE_ENTITY_DRIVEN_DISTANCE)
	{
		if (mcm[eMCMSlot_Cur] == eMCM_DecoupledCatchUp)
		{
			mcm[eMCMSlot_Cur] = eMCM_Entity;

			if (mcm[eMCMSlot_Prev] != eMCM_Entity)
			{
				mcm[eMCMSlot_Prev] = eMCM_Entity;
				const float speed = m_moveRequest.velocity.GetLength2D();
				const float defaultFadeTime = GetClampDurationEntity();
				const float estimatedTimeToArrival = speed > 0.0f ? dist / speed : defaultFadeTime;
				const float maximumFadeInTime = 0.9f * estimatedTimeToArrival; // we want the entity to be fully entity driven *before* he arrives.
				m_elapsedTimeMCM[component] = max(0.0f, defaultFadeTime - maximumFadeInTime);
			}
		}
	}

	if (m_pAnimTarget->activated || (dist < ANIMTARGET_FORCE_FLY_MODE_DISTANCE))
	{
		m_requestedEntityMovementType = RequestedEntMovementType_Absolute;
		m_requestedIJump = 3; // fly mode
	}
}

//--------------------------------------------------------------------------------

void CAnimatedCharacter::UpdateMCMComponent(EMCMComponent component)
{
	ANIMCHAR_PROFILE_DETAILED;

	EMovementControlMethod* mcm = m_movementControlMethod[component];

	mcm[eMCMSlot_Cur] = mcm[eMCMSlot_AnimGraph];

#ifdef _DEBUG
	if (CAnimationGraphCVars::Get().m_MCMFilter != 0)
	{
		if (mcm[eMCMSlot_Cur] == eMCM_DecoupledCatchUp)
			mcm[eMCMSlot_Cur] = eMCM_Entity;
	}
#endif

	PreventAnimTargetOvershooting(component);

	// In Player.cpp this flag is set for local player first person only, 
	// so elsewhere we only need to look at the MCM, not dig up the actor.
	// (Though, the actor might be cached in the AC anyway, will see.)
	if ((m_params.flags & eACF_AlwaysPhysics) && (mcm[eMCMSlot_Cur] != eMCM_Animation) && (mcm[eMCMSlot_Cur] != eMCM_AnimationHCollision))
		mcm[eMCMSlot_Cur] = eMCM_Entity;

	if (m_params.flags & eACF_Frozen)
		mcm[eMCMSlot_Cur] = eMCM_Entity;

	if (NoMovementOverride())
		mcm[eMCMSlot_Cur] = eMCM_Entity;

	// This is not really needed, since m_simplifyMovement is used everywhere, but just for the sake of it...
	if (m_simplifyMovement)
		mcm[eMCMSlot_Cur] = eMCM_Entity;

	if (InCutscene())
		mcm[eMCMSlot_Cur] = eMCM_Animation;

	if (mcm[eMCMSlot_Debug] != eMCM_Undefined)
		mcm[eMCMSlot_Cur] = mcm[eMCMSlot_Debug];

	if (mcm[eMCMSlot_Cur] == eMCM_Undefined)
		mcm[eMCMSlot_Cur] = eMCM_Entity;

	if (mcm[eMCMSlot_Cur] != mcm[eMCMSlot_Prev])
	{
		mcm[eMCMSlot_Prev] = mcm[eMCMSlot_Cur];
		m_elapsedTimeMCM[component] = 0.0f;
	}
}

//--------------------------------------------------------------------------------

void CAnimatedCharacter::SetMovementControlMethods(EMovementControlMethod horizontal, EMovementControlMethod vertical)
{
	// vertical eMCM_AnimationHCollision is not allowed (H stands for horizontal)
	if (vertical == eMCM_AnimationHCollision)
		vertical = eMCM_Animation;

	m_movementControlMethod[eMCMComponent_Horizontal][eMCMSlot_AnimGraph] = horizontal;
	m_movementControlMethod[eMCMComponent_Vertical][eMCMSlot_AnimGraph] = vertical;
}

//--------------------------------------------------------------------------------

EMovementControlMethod CAnimatedCharacter::GetMCMH() const
{
	return (m_movementControlMethod[eMCMComponent_Horizontal][eMCMSlot_Cur]);
}

//--------------------------------------------------------------------------------

EMovementControlMethod CAnimatedCharacter::GetMCMV() const
{
	return (m_movementControlMethod[eMCMComponent_Vertical][eMCMSlot_Cur]);
}

//--------------------------------------------------------------------------------

void CAnimatedCharacter::SetNoMovementOverride(bool external)
{
	m_noMovementOverrideExternal = external;
}

//--------------------------------------------------------------------------------

bool CAnimatedCharacter::NoMovementOverride() const
{
	return (m_bGrabbedInViewSpace || m_noMovementOverrideExternal);
}

//--------------------------------------------------------------------------------

void CAnimatedCharacter::ForceTeleportAnimationToEntity()
{
	m_animLocation = m_entLocation; // Unlike the entity, the manimLocation really has no strings attached, just change it.
	m_desiredAnimMovement = IDENTITY; // Not sure this is needed, but what the heck, will not hurt.
}

//--------------------------------------------------------------------------------

bool CAnimatedCharacter::HasSplitUpdate() const
{
	IEntityRenderProxy* pRenderProxy = (IEntityRenderProxy*)(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));
	return ((pRenderProxy != NULL) && pRenderProxy->IsCharactersUpdatedBeforePhysics());
}

bool CAnimatedCharacter::HasAtomicUpdate() const
{
	return !HasSplitUpdate();
}

//--------------------------------------------------------------------------------

const SPredictedCharacterStates* CAnimatedCharacter::GetPredictedCharacterStates() const
{
	if (m_moveRequestFrameID != gEnv->pRenderer->GetFrameID())
		return NULL;

	return &(m_moveRequest.prediction);
}

//--------------------------------------------------------------------------------

void CAnimatedCharacter::RefreshAnimTarget()
{
	m_pAnimTarget = m_animationGraphStates.GetAnimationTarget();
}

//--------------------------------------------------------------------------------
