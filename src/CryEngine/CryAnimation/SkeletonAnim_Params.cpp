//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:Skeleton.cpp
//  Implementation of Skeleton class (Forward Kinematics)
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "CharacterInstance.h"
#include "Model.h"
#include "ModelSkeleton.h"
#include "CharacterManager.h"
#include <float.h>
#include "LMG.h"

//---------------------------------------------------------------------------------

// Enable/Disable direct override of blendspace weights, used from CharacterEditor.
void CSkeletonAnim::SetBlendSpaceOverride(EMotionParamID id, float value, bool enable)
{
	m_CharEditBlendSpaceOverrideEnabled[id] = enable;
	if (enable)
		m_CharEditBlendSpaceOverride[id] = value;
	else
		m_CharEditBlendSpaceOverride[id] = 0.0f;
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::SetDesiredLocalLocation(const QuatT& desiredLocalLocation, float deltaTime, float frameTime, float turnSpeedMultiplier)
{
	m_desiredLocalLocation = desiredLocalLocation;
	assert(m_desiredLocalLocation.q.IsValid());

	m_desiredArrivalDeltaTime = deltaTime;

	m_desiredTurnSpeedMultiplier = turnSpeedMultiplier;
}

//---------------------------------------------------------------------------------

SPU_NO_INLINE void CSkeletonAnim::SetDesiredMotionParamsFromDesiredLocalLocation(float frameTime)
{

	SpuStackValue<QuatT, false,false> spuStackDesiredLocation( m_desiredLocalLocation );
  	const QuatT &rDesiredLocation = spuStackDesiredLocation;
  	
  	Vec3 dir = rDesiredLocation.q * FORWARD_DIRECTION;
  	Vec2 predictedDir( dir.x, dir.y );

	float predictedAngle = RAD2DEG(atan2f(-predictedDir.x, predictedDir.y));
	float turnSpeed = m_desiredTurnSpeedMultiplier * DEG2RAD(predictedAngle) / max(0.1f, m_desiredArrivalDeltaTime);
	float turnAngle = DEG2RAD(predictedAngle);

	Vec2 deltaVector(rDesiredLocation.t.x, rDesiredLocation.t.y);
	float deltaDist = deltaVector.GetLength();

	const float thresholdDistMin = 0.05f;
  	const float thresholdDistMax = 0.15f;
	f32 uniform_scale = m_pInstance->CCharInstance::GetUniformScale();

	float travelDistScale = CLAMP((deltaDist - thresholdDistMin) / (thresholdDistMax - thresholdDistMin), 0.0f, 1.0f);
	SetDesiredMotionParam(eMotionParamID_TravelDistScale, travelDistScale/uniform_scale, frameTime);

	Vec2 deltaDir = (deltaDist > 0.0f) ? (deltaVector / deltaDist) : Vec2(0,0);
	float deltaAngle = (deltaDir.x == 0.0f && deltaDir.y == 0.0f  ? 0.0f : RAD2DEG(atan2f(-deltaDir.x, deltaDir.y)) );
	float travelAngle = DEG2RAD(deltaAngle);

	// Update travel direction only if distance bigger (more secure) than thresholdDistMax.
	// Though, also update travel direction if distance is small enough to not have any visible effect (since distance scale is zero).
	bool initOnly = ((deltaDist > thresholdDistMin) && (deltaDist < thresholdDistMax));
	SetDesiredMotionParam(eMotionParamID_TravelAngle, travelAngle, frameTime, initOnly);

	float curvingFraction = 0.0f;
	if (Console::GetInst().ca_GameControlledStrafing != 0)
	{
		// When desired local location is off to the side, but we are not allowed to strafe,
		// we need to adjust curving to curve into and catch up with the desired location.
		if (GetDesiredMotionParam(eMotionParamID_Curving) > 0.8f)
			turnSpeed += deltaAngle * 0.05f;
	}
	else
	{
		// Calculate wanted curving fraction
		float inAngle = -deltaAngle;
		float outAngle = -deltaAngle + predictedAngle;
		float diffAngle = abs(inAngle + outAngle);
		const float curvingMinDiffAngle = 20.0f;
  		const float curvingMaxDiffAngle = 40.0f;
		curvingFraction = 1.0f - CLAMP((diffAngle - curvingMinDiffAngle) / (curvingMaxDiffAngle - curvingMinDiffAngle), 0.0f, 1.0f);
		SetDesiredMotionParam(eMotionParamID_Curving, curvingFraction, frameTime);
	}

	SetDesiredMotionParam(eMotionParamID_TurnSpeed, turnSpeed, frameTime);
	SetDesiredMotionParam(eMotionParamID_TurnAngle, turnAngle, frameTime);

	// Curving Distance	
	float div = cry_cosf(DEG2RAD(90.0f - predictedAngle));
	float curveRadius = (deltaDist / 2.0f);

	if( abs(div) > 0.0f )
		curveRadius = curveRadius / div;
	float circumference = curveRadius * 2.0f * gf_PI;
	float curveDist = circumference * (predictedAngle * 2.0f / 360.0f);
	curveDist = max(deltaDist, curveDist);

	// Travel Speed/Distance
	float travelDist = LERP(deltaDist, curveDist, curvingFraction);
	float travelSpeed = travelDist / max(0.1f, m_desiredArrivalDeltaTime);
	//travelDist *= travelDistScale;
	//travelSpeed *= travelDistScale;
	SetDesiredMotionParam(eMotionParamID_TravelSpeed, travelSpeed, frameTime);
	SetDesiredMotionParam(eMotionParamID_TravelDist, travelDist, frameTime);
	SetDesiredMotionParam(eMotionParamID_Scale, uniform_scale, frameTime);




/*
	f32 AverageFrameTime = m_pInstance->GetAverageFrameTime();
	SetDesiredMotionParam2(eMotionParamID_TravelSpeed,     0.9f, AverageFrameTime);
	SetDesiredMotionParam2(eMotionParamID_TurnSpeed,       0.0f,	AverageFrameTime);
	SetDesiredMotionParam2(eMotionParamID_TurnAngle,       0.0f,	AverageFrameTime);								//thats how we set the turn-direction of an Idle2Move
	SetDesiredMotionParam2(eMotionParamID_TravelAngle,     0.2f,	AverageFrameTime);								//thats how we set the strafing direction
	SetDesiredMotionParam2(eMotionParamID_TravelSlope,     10.4f,	AverageFrameTime);	//thats how we set the slope
	SetDesiredMotionParam2(eMotionParamID_WeightShift,     0.0f,	AverageFrameTime);								//thats how we set the shallow turn
	SetDesiredMotionParam2(eMotionParamID_TravelDistScale, 1.0f, AverageFrameTime);
	SetDesiredMotionParam2(eMotionParamID_TravelDist,      -1,	  AverageFrameTime);
*/
}

void CSkeletonAnim::UpdatePMGParameters(const GlobalAnimationHeaderPMG* pmgHeader, SParametric& pmg)
{
	uint32 numParamDim = (uint32)pmgHeader->m_paramSpecPMG.size();

	for(uint32 i=0; i<numParamDim; ++i)
	{
		for(uint32 j=0; j<MAX_PMG_PARAM_DIM; ++j)
		{
			if(controlParamName[j] == pmgHeader->m_paramSpecPMG[i].name)
				pmg.m_controlParam[i] = controlParam[j];
		}
	}
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::UpdateMotionBlendSpace(SParametric& lmg, float deltaTime, float transitionBlendWeight)
{
	// Since the caps calculation is based on the old blendspace struct it must be updated first.
	// We must pass in zero frametime here, so that interpolations don't get updated extra (actual update happens elsewhere).
	for (int id = 0; id < eMotionParamID_COUNT; ++id)
	{
		UpdateMotionParamBlendSpace(lmg, (EMotionParamID)id);
	}
	UpdateOldMotionBlendSpace(lmg, deltaTime, transitionBlendWeight);
}

void CSkeletonAnim::GetCapabilities_PMG(const SParametric & lmg, LMGCapabilities & lmg_caps)
{
	lmg_caps.m_fDesiredLocalLocationLookaheadTime = 1.0f;

	lmg_caps.m_bIsValid=1;
	lmg_caps.m_bIsLMG=1;
	lmg_caps.m_bHasStrafingAsset=1;
	//lmg_caps.m_bHasTurningAsset=-1;
	lmg_caps.m_bHasTurningAsset=0;
	lmg_caps.m_bHasVelocity = 0;
	lmg_caps.m_bHasDistance = 0;
	lmg_caps.m_travelAngleChangeRate = 0.0f;
	lmg_caps.m_travelSpeedChangeRate = 0.0f;
	lmg_caps.m_fAllowDesiredTurning=0;

	f32 fmin = -100.0f;
	f32 fmax = 100.f;
	lmg_caps.m_vMinVelocity	= Vec3(0, fmin, 0);//	Vec3(0,minspeed-0.001f,0);
	lmg_caps.m_fFastTurnLeft = fmin;
	lmg_caps.m_fSlowDuration = fmin;//rsubGAH00.m_fTotalDuration*I2M.fr + rsubGAH01.m_fTotalDuration*I2M.rr + rsubGAH02.m_fTotalDuration*I2M.br   +   rsubGAH03.m_fTotalDuration*I2M.fl + rsubGAH04.m_fTotalDuration*I2M.ll + rsubGAH05.m_fTotalDuration*I2M.bl;

	lmg_caps.m_vMaxVelocity	=	Vec3(0, fmax, 0);
	lmg_caps.m_fFastTurnRight = fmax;
	lmg_caps.m_fFastDuration = fmax;

	lmg_caps.m_fAllowDesiredTurning= fmax;
}
//---------------------------------------------------------------------------------

void CSkeletonAnim::UpdateMotionParamDescs(SParametric& lmg, float transitionBlendWeight)
{
	if (lmg.m_nParametricID < 0)
		return;

	UpdateMotionBlendSpace(lmg, 0.0f, transitionBlendWeight);

	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	const ModelAnimationHeader* pAnim = SPU_MAIN_PTR( pAnimationSet->GetModelAnimationHeader(lmg.m_nParametricID) );

	LMGCapabilities caps;
	if (pAnim->m_nAssetType==LMG_File)
	{
		caps = LMG::GetCapabilities(pAnimationSet, lmg);
	}
	if (pAnim->m_nAssetType==PMG_File)
	{
		GetCapabilities_PMG(lmg, caps);
	}


	lmg.m_fDesiredLocalLocationLookaheadTime = caps.m_fDesiredLocalLocationLookaheadTime;
	for (int id = 0; id < eMotionParamID_COUNT; ++id)
	{
		ConvertLMGCapsToMotionParamDesc((EMotionParamID)id, lmg.m_params[id].desc, caps);
	}
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::ConvertLMGCapsToMotionParamDesc(EMotionParamID id, MotionParamDesc& desc, const LMGCapabilities& caps)
{
	memset(&desc, 0, sizeof(desc));

	switch (id)
	{
	case eMotionParamID_TravelAngle:
		if (caps.m_bHasStrafingAsset > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Cyclic;
			desc.m_fMin = DEG2RAD(-180.0f);
			desc.m_fMax = DEG2RAD(+180.0f);
			desc.m_fMinAsset = DEG2RAD(-180.0f);
			desc.m_fMaxAsset = DEG2RAD(+180.0f);
			desc.m_fMaxChangeRate = caps.m_travelAngleChangeRate;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	case eMotionParamID_TravelDistScale:
		if (caps.m_bHasStrafingAsset > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = 0.0f;
			desc.m_fMaxAsset = 1.0f;
			desc.m_fMin = 0.0f;
			desc.m_fMax = 1.0f;
			desc.m_fMaxChangeRate = (caps.m_travelAngleChangeRate != 0.0f) ? -1.0f : 0.0f;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	case eMotionParamID_TurnSpeed:
		if ((caps.m_bHasTurningAsset > 0) || (caps.m_fAllowDesiredTurning > 0))
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = min(caps.m_fFastTurnLeft, caps.m_fFastTurnRight);
			desc.m_fMaxAsset = max(caps.m_fFastTurnLeft, caps.m_fFastTurnRight);
			desc.m_fMin = min(desc.m_fMinAsset, LERP(desc.m_fMinAsset, DEG2RAD(-360.0f) * 1.5f, caps.m_fAllowDesiredTurning));
			desc.m_fMax = max(desc.m_fMaxAsset, LERP(desc.m_fMaxAsset, DEG2RAD(+360.0f) * 1.5f, caps.m_fAllowDesiredTurning));
			desc.m_fMaxChangeRate = caps.m_turnSpeedChangeRate;
			desc.m_fAllowProcTurn = caps.m_fAllowDesiredTurning;
		}
		break;

	case eMotionParamID_Curving:
		if (caps.m_bHasCurving > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = 0.0f;
			desc.m_fMaxAsset = 1.0f;
			desc.m_fMin = 0.0f;
			desc.m_fMax = 1.0f;
			desc.m_fMaxChangeRate = caps.m_curvingChangeRate;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	case eMotionParamID_TravelSpeed:
		if (caps.m_bHasVelocity > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = caps.m_vMinVelocity.GetLength();
			desc.m_fMaxAsset = caps.m_vMaxVelocity.GetLength();
			desc.m_fMin = desc.m_fMinAsset * Console::GetInst().ca_travelSpeedScaleMin;
			desc.m_fMax = desc.m_fMaxAsset * Console::GetInst().ca_travelSpeedScaleMax;
			desc.m_fMaxChangeRate = caps.m_travelSpeedChangeRate;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	case eMotionParamID_TravelDist:
		if (caps.m_bHasDistance > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = caps.m_vMinVelocity.GetLength()*caps.m_fSlowDuration;
			desc.m_fMaxAsset = caps.m_vMaxVelocity.GetLength()*caps.m_fFastDuration;
			desc.m_fMin = desc.m_fMinAsset;
			desc.m_fMax = desc.m_fMaxAsset;
			desc.m_fMaxChangeRate = 0.0f;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	case eMotionParamID_TurnAngle:
		if (caps.m_bHasTurningDistAsset > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = min(caps.m_fFastTurnLeft, caps.m_fFastTurnRight);
			desc.m_fMaxAsset = max(caps.m_fFastTurnLeft, caps.m_fFastTurnRight);
			desc.m_fMin = min(desc.m_fMinAsset, LERP(desc.m_fMinAsset, DEG2RAD(-180.0f) * 1.5f, caps.m_fAllowDesiredTurning));
			desc.m_fMax = max(desc.m_fMaxAsset, LERP(desc.m_fMaxAsset, DEG2RAD(+180.0f) * 1.5f, caps.m_fAllowDesiredTurning));
			desc.m_fMaxChangeRate = 0.0f;
			desc.m_fAllowProcTurn = caps.m_fAllowDesiredTurning;
		}
		break;

	case eMotionParamID_TravelSlope:
		//if (caps.m_bHasSlope)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = -21.0f;
			desc.m_fMaxAsset = +21.0f;
			desc.m_fMin = desc.m_fMinAsset;
			desc.m_fMax = desc.m_fMaxAsset;
			desc.m_fMaxChangeRate = 40.0f;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	case eMotionParamID_WeightShift:
		//if (caps.m_bHasSlope)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMinAsset = -1.0f;
			desc.m_fMaxAsset = +1.0f;
			desc.m_fMin = -1.0f; //shift weight to the left
			desc.m_fMax = +1.0f; //shift weight to the right
			desc.m_fMaxChangeRate = 4.0f;
			desc.m_fAllowProcTurn = 1.0f;
		}
		break;

	case eMotionParamID_Height:
		//if (caps.m_bHasStrafingAsset > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMin = 0.0f;
			desc.m_fMax = 5.0f;
			desc.m_fMinAsset = 0.0f;
			desc.m_fMaxAsset = 5.0f;
			desc.m_fMaxChangeRate = 0.0f;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	case eMotionParamID_Scale:
		//if (caps.m_bHasStrafingAsset > 0)
		{
			desc.m_nUsage = eMotionParamUsage_Interval;
			desc.m_fMin = 0.0f;
			desc.m_fMax = 2.0f;
			desc.m_fMinAsset = 0.0f;
			desc.m_fMaxAsset = 2.0f;
			desc.m_fMaxChangeRate = 0.0f;
			desc.m_fAllowProcTurn = 0.0f;
		}
		break;

	}

	// TravelSlope

	if (desc.m_nUsage != eMotionParamUsage_None)
	{
		desc.m_bLocked = (desc.m_fMaxChangeRate == 0.0f);
	}
}

//---------------------------------------------------------------------------------

float CSkeletonAnim::GetDesiredMotionParam(EMotionParamID id) const
{
	return m_desiredMotionParam[id];
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::SetDesiredMotionParam2(EMotionParamID id, float value, float deltaTime, bool initOnly /* = false */)
{

}

void CSkeletonAnim::SetDesiredMotionParam(EMotionParamID id, float value, float deltaTime, bool initOnly /* = false */)
{

	//assert(initOnly==0);

	m_desiredMotionParam[id] = value;

	int layer = 0;
	int animCount = GetNumAnimsInFIFO(layer);

	for (int i = 0; i < animCount; ++i)
	{
#if defined(__SPU__)	// we only have enough space on spu for 8 layers, if not enough fallback to main memory access
		IF( i < MAX_EXEC_QUEUE, true )
		{
			// redirect data in spu to use spu local array
  		CAnimation& anim = gAninmationFifoLayer0[i];

			if (anim.m_bActivated)
			{
				SetClampedMotionParam(anim.m_Parametric, id, value, deltaTime, initOnly);
				UpdateMotionParamDescs(anim.m_Parametric, anim.m_fTransitionWeight);
			}
		} else
#endif
		{
			// redirect data in spu to use spu local array
			SpuStackValue<CAnimation,true,true> StackAnim( GetAnimFromFIFO(layer, i) );
  		CAnimation& anim = StackAnim;			

			if (anim.m_bActivated)
			{				
				SetClampedMotionParam(anim.m_Parametric, id, value, deltaTime, initOnly);
				UpdateMotionParamDescs(anim.m_Parametric, anim.m_fTransitionWeight);
			}
		}	
	}
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::SetClampedMotionParam(SParametric& lmg, EMotionParamID id, float newValue, float deltaTime, bool initOnly)
{
	if (lmg.m_nParametricID < 0)
		return;

	MotionParam& param = lmg.m_params[id];
	if (param.desc.m_nUsage == eMotionParamUsage_None)
		return;

	if (param.initialized && (param.desc.m_bLocked || initOnly))
		return;


	float cycle = 0;
	if (param.desc.m_nUsage == eMotionParamUsage_Cyclic)
	{
		cycle = abs(param.desc.m_fMax - param.desc.m_fMin);
	}

	// Make sure requested value is within cycle's interval
	if (cycle > 0.0f)
	{
		while (newValue < param.desc.m_fMin)
			newValue += cycle;
		while (newValue > param.desc.m_fMax)
			newValue -= cycle;
	}

	// Perform delta clamping only if we have a value to related to already.
	if (param.initialized)
	{
		const float deltaTimeMin = 0.0f;
  		const float deltaTimeMax = 0.05f;

		deltaTime = CLAMP(deltaTime, deltaTimeMin, deltaTimeMax);

		float newDelta = (newValue - param.value);

		// Make sure delta stays within cycle's half range (since a value bigger than half a cycle will wrap around the other direction instead)
		if (cycle > 0.0f)
		{
			if (newDelta > (cycle*0.5f))
				newDelta -= cycle;
			if (newDelta < -(cycle*0.5f))
				newDelta += cycle;
		}

		// Clamp to max change rate per second
		if ((param.desc.m_fMaxChangeRate != -1.0f) && (deltaTime != 0.0f))
		{
			newDelta /= deltaTime;
			if (newDelta > param.desc.m_fMaxChangeRate)
				newDelta = param.desc.m_fMaxChangeRate;
			if (newDelta < -param.desc.m_fMaxChangeRate)
				newDelta = -param.desc.m_fMaxChangeRate;
			newDelta *= deltaTime;
		}

		// Apply anti oscillation damping
		if ((newDelta * param.delta) < 0.0f) // first derivative changes sign, meaning we are oscilating (at least for one frame)
		{
			const float anti_oscilation_scale = 0.5f;
			if (abs(newDelta) > abs(param.delta * anti_oscilation_scale)) // prevent oscillation that does not decrease in magnitude
			{
				newDelta = sgn(newDelta) * abs(param.delta * anti_oscilation_scale);
			}
		}

		// Update value
		newValue = param.value + newDelta;

		// Make sure it's within the cycle's range
		if (cycle > 0.0f)
		{
			while (newValue < param.desc.m_fMin)
				newValue += cycle;
			while (newValue > param.desc.m_fMax)
				newValue -= cycle;
		}

	}

	// Clamp to interval range
	if (param.desc.m_nUsage == eMotionParamUsage_Interval)
	{
		if (newValue < param.desc.m_fMin)
			newValue = param.desc.m_fMin;

		if (newValue > param.desc.m_fMax)
			newValue = param.desc.m_fMax;
	}

	// Update actual stored value and delta
	param.delta = param.initialized ? (newValue - param.value) : 0.0f;
	param.value = newValue;
	param.initialized = true;
}

//---------------------------------------------------------------------------------

SPU_NO_INLINE void CSkeletonAnim::UpdateMotionParamBlendSpacesForActiveMotions(float deltaTime)
{

	int layer = 0;
	int animCount = GetNumAnimsInFIFO(layer);

	CAnimationSet* pAnimationSet = &m_pInstance->m_pModel->m_AnimationSet;
	GlobalAnimationHeaderPMG* rGlobalAnimHeaderPMG=0;
	for (int i = 0; i < animCount; ++i)
	{		
#if defined(__SPU__)	// we only have enough space on spu for 8 layers, if not enough fallback to main memory access
		IF( i < MAX_EXEC_QUEUE, true )
		{
			CAnimation& anim  = gAninmationFifoLayer0[i];

			if (anim.m_bActivated)
			{
				UpdateMotionBlendSpace(anim.m_Parametric, deltaTime, anim.m_fTransitionWeight);

				//------------------------------------------------------------------------------
				// Update PMG parameters

				if (anim.m_Parametric.m_nParametricID>=0)
					rGlobalAnimHeaderPMG = pAnimationSet->GetGAH_PMG(anim.m_Parametric.m_nParametricID);

				if (rGlobalAnimHeaderPMG)
					UpdatePMGParameters(rGlobalAnimHeaderPMG, anim.m_Parametric);
			}
		} else
#endif
		{
			SpuStackValue<CAnimation,true,true> StackAnim( GetAnimFromFIFO(layer, i) );
			CAnimation& anim  = StackAnim;
			if (anim.m_bActivated)
			{
				UpdateMotionBlendSpace(anim.m_Parametric, deltaTime, anim.m_fTransitionWeight);

				//------------------------------------------------------------------------------
				// Update PMG parameters

				if (anim.m_Parametric.m_nParametricID>=0)
					rGlobalAnimHeaderPMG = pAnimationSet->GetGAH_PMG(anim.m_Parametric.m_nParametricID);

				if (rGlobalAnimHeaderPMG)
					UpdatePMGParameters(rGlobalAnimHeaderPMG, anim.m_Parametric);
			}
		}
	
	}
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::UpdateMotionParamBlendSpace(SParametric& lmg, EMotionParamID id)
{
	f32 undefined = 0.5f;
	if ((id == eMotionParamID_TravelDistScale) || (id == eMotionParamID_TravelDist))
	{
		undefined = 1.0f;
	}
	else if (id == eMotionParamID_Curving)
	{
		undefined = 0.0f;
	}

	if (lmg.m_nParametricID < 0)
	{
		//lmg.m_params[id].blendspace.m_fAssetBlend = invalid;
		//lmg.m_params[id].blendspace.m_fProceduralOffset = 0.0f;
		//lmg.m_params[id].blendspace.m_fProceduralScale = 1.0f;
		//return;
	}

	if (m_CharEditBlendSpaceOverrideEnabled[id])
	{
		lmg.m_params[id].blendspace.m_fAssetBlend = m_CharEditBlendSpaceOverride[id];
		lmg.m_params[id].blendspace.m_fProceduralOffset = 0.0f;
		lmg.m_params[id].blendspace.m_fProceduralScale = 1.0f;
		return;
	}

	/*
	if (id == eMotionParamID_TravelSpeed)
	{
		MotionParam param = lmg.m_params[id];
		f32 mina	=	param.desc.m_fMinAsset;
		f32 maxa	=	param.desc.m_fMaxAsset;
		f32 value	=	param.value;
	//	f32 fColor[4] = {1,1,0,1};
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"mina: %f  maxa: %f  value: %f",mina,maxa,value );	
	//	g_YLine+=0x20;
	}
	*/
	CalculateMotionParamBlendSpace(lmg.m_params[id], undefined);
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::CalculateMotionParamBlendSpace(MotionParam& param, float undefined)
{
	// case01, 0-0, x : offset=value, scale=1, blend=0.5
	// case11, 0-b, x<0 : offset=value, scale=1, blend=0
	// case12, 0-b, x>b : offset=0, scale=value/b, blend=1
	// case13, a-0, x<a : offset=0, scale=value/a, blend=0
	// case14, a-0, x>0 : offset=value, scale=1, blend=1
	// case21, a-a, x<a : offset=0, scale=value/a, blend=0.5
	// case22, a-a, x>a : offset=0, scale=value/a, blend=0.5
	// case23, a-a, x!=a : offset=0, scale=value/a, blend=0.5
	// case31, a-0, a<x<0 : offset=0, scale=1, blend=(value-a)/(b-a)
	// case32, 0-b, 0<x<b : offset=0, scale=1, blend=(value-a)/(b-a)
	// case33, a-b, a<x<b : offset=0, scale=1, blend=(value-a)/(b-a)

#define SETBLENDSPACE(offset,scale,blend,allow) \
	{	\
	param.blendspace.m_fProceduralOffset = offset; \
	param.blendspace.m_fProceduralScale = scale; \
	param.blendspace.m_fAssetBlend = blend; \
	param.blendspace.m_fAllowPrceduralTurn = allow; \
	return; \
} \

	float procTurn	= param.desc.m_fAllowProcTurn;

	// default
	if (!param.initialized || (param.desc.m_nUsage == eMotionParamUsage_None))
		SETBLENDSPACE(0.0f ,1.0f, undefined,procTurn);

	// case01, 0-0, x : offset=value, scale=1, blend=0.5
	if ((param.desc.m_fMinAsset == 0.0f) && (param.desc.m_fMaxAsset == 0.0f))
		SETBLENDSPACE(param.value, 1.0f, 0.5f,procTurn);

	float assetSpan = (param.desc.m_fMaxAsset - param.desc.m_fMinAsset);
	float assetBlend = (assetSpan != 0.0f) ? (param.value - param.desc.m_fMinAsset) / assetSpan : 0.0f;
	float minScale = (param.desc.m_fMinAsset != 0.0f) ? param.value / param.desc.m_fMinAsset : 0.0f;
	float maxScale = (param.desc.m_fMaxAsset != 0.0f) ? param.value / param.desc.m_fMaxAsset : 0.0f;

	// case11, 0-b, x<0 : offset=value, scale=1, blend=0
	// case12, 0-b, x>b : offset=0, scale=value/b, blend=1
	// case13, a-0, x<a : offset=0, scale=value/a, blend=0
	// case14, a-0, x>0 : offset=value, scale=1, blend=1
	if ((param.desc.m_fMinAsset == 0.0f) && (param.desc.m_fMaxAsset > 0.0f) && (param.value < param.desc.m_fMinAsset))
		SETBLENDSPACE(param.value, 1.0f, 0.0f,procTurn);

	if ((param.desc.m_fMinAsset == 0.0f) && (param.desc.m_fMaxAsset > 0.0f) && (param.value > param.desc.m_fMaxAsset))
		SETBLENDSPACE(0.0f, maxScale, 1.0f,procTurn);

	if ((param.desc.m_fMinAsset < 0.0f) && (param.desc.m_fMaxAsset == 0.0f) && (param.value < param.desc.m_fMinAsset))
		SETBLENDSPACE(0.0f, minScale, 0.0f,procTurn);

	if ((param.desc.m_fMinAsset < 0.0f) && (param.desc.m_fMaxAsset == 0.0f) && (param.value > param.desc.m_fMaxAsset))
		SETBLENDSPACE(param.value, 1.0f, 1.0f,procTurn);

	// case21, a-a, x<a : offset=0, scale=value/a, blend=0.5
	// case22, a-a, x>a : offset=0, scale=value/a, blend=0.5
	// case23, a-a, x!=a : offset=0, scale=value/a, blend=0.5
	if ((param.desc.m_fMinAsset == param.desc.m_fMaxAsset))
	{
		if (param.value < param.desc.m_fMinAsset)
			SETBLENDSPACE(0.0f, minScale, 0.0f,procTurn);
		if (param.value < param.desc.m_fMinAsset)
			SETBLENDSPACE(0.0f, maxScale, 1.0f,procTurn);

		SETBLENDSPACE(0.0f, minScale, 0.5f,procTurn);
	}

	// case33, a-b, x<a: offset=0, scale=value/a, blend=0
	if (param.value < param.desc.m_fMinAsset)
		SETBLENDSPACE(0.0f, minScale, 0.0f,procTurn);

	// case33, a-b, x>b: offset=0, scale=value/b, blend=1
	if (param.value > param.desc.m_fMaxAsset)
		SETBLENDSPACE(0.0f, maxScale, 1.0f,procTurn);

	// case31, a-0, a<x<0 : offset=0, scale=1, blend=(value-a)/(b-a)
	// case32, 0-b, 0<x<b : offset=0, scale=1, blend=(value-a)/(b-a)
	// case33, a-b, a<x<b: offset=0, scale=1, blend=(value-a)/(b-a)
	SETBLENDSPACE(0.0f, 1.0f, assetBlend,procTurn);
}

//---------------------------------------------------------------------------------

void CSkeletonAnim::UpdateOldMotionBlendSpace(SParametric& lmg, float frameTime, float transitionBlendWeight)
{

	transitionBlendWeight = CLAMP(transitionBlendWeight, 0.0f, 1.0f);

	assert(transitionBlendWeight >= 0.0f);
	assert(transitionBlendWeight <= 1.0f);

	float radiant = (lmg.m_params[eMotionParamID_TravelAngle].blendspace.m_fAssetBlend - 0.5f) * gf_PI2;
	//	Vec2 strafe = lmg.m_BlendSpace.m_strafe;



	if (lmg.m_params[eMotionParamID_TravelAngle].initialized)
	{
		Vec2 newStrafe;
		newStrafe = Vec2(-cry_sinf(radiant), cry_cosf(radiant));
		newStrafe *= lmg.m_params[eMotionParamID_TravelDistScale].blendspace.m_fAssetBlend;
		newStrafe *= lmg.m_params[eMotionParamID_TravelDist].blendspace.m_fAssetBlend;

		//smoothing old
		/*		if (!lmg.m_params[eMotionParamID_TravelAngle].desc.m_bLocked)
		{
		static float blendDuration = 0.15f;
		float blend = CLAMP(frameTime / blendDuration, 0.0f, 1.0f);
		blend *= lmg.m_params[eMotionParamID_TravelDistScale].blendspace.m_fAssetBlend;
		lmg.m_BlendSpace.m_strafe = LERP(strafe, lmg.m_BlendSpace.m_strafe, blend);
		}*/

		if (Console::GetInst().ca_SmoothStrafe)
		{			
			//smoothing new
			if (!lmg.m_params[eMotionParamID_TravelAngle].desc.m_bLocked && (lmg.m_BlendSpace.m_strafe.GetLength2() != 0.0f))
			{
				const float blendDuration = 0.1f;
				float blend = CLAMP(frameTime / blendDuration, 0.0f, 1.0f);
				blend *= lmg.m_params[eMotionParamID_TravelDistScale].blendspace.m_fAssetBlend;
				blend *= transitionBlendWeight * transitionBlendWeight * transitionBlendWeight * transitionBlendWeight;
				lmg.m_BlendSpace.m_strafe = LERP(lmg.m_BlendSpace.m_strafe, newStrafe, blend);
			}
			else
			{
				lmg.m_BlendSpace.m_strafe = newStrafe;
			}
		}
		else
		{
			lmg.m_BlendSpace.m_strafe = newStrafe;
		}

		//f32 fColor[4] = {1,1,0,1};
		//g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.3f, fColor, false,"radiant:%f  strafe:(%f %f)", radiant,lmg.m_BlendSpace.m_strafe.x,lmg.m_BlendSpace.m_strafe.y );	g_YLine+=0x20;

	}

	/*
	//smoothing new
	if (!lmg.m_params[eMotionParamID_TravelAngle].desc.m_bLocked)
	{
	static float blendDuration = 2.0f;
	float assetBlend = lmg.m_params[eMotionParamID_TravelDistScale].blendspace.m_fAssetBlend;
	float timeBlend = (blendDuration > 0.0f) ? CLAMP(frameTime / blendDuration, 0.0f, assetBlend) : 1.0f;
	float alignment = CLAMP(lmg.m_BlendSpace.m_strafe | strafe, 0.0f, 1.0f);
	float alignmentBlend = LERP(timeBlend, 1.0f, CLAMP((alignment-0.7f)/0.3f, 0.0f, 1.0f));
	lmg.m_BlendSpace.m_strafe = LERP(strafe, lmg.m_BlendSpace.m_strafe, alignmentBlend);
	float fColor[4] = {1,1,1,1};
	g_pIRenderer->Draw2dLabel(100, g_YLine, 2.0f, fColor, false, "alignmentBlend: %f", alignmentBlend); g_YLine+=0x10;
	}
	*/

	if (Console::GetInst().ca_EnableAssetStrafing==0)
		lmg.m_BlendSpace.m_strafe = Vec2(0,0); //LERP(strafe, lmg.m_BlendSpace.m_strafe, blend);

	lmg.m_BlendSpace.m_speed = lmg.m_params[eMotionParamID_TravelSpeed].blendspace.m_fAssetBlend * 2.0f - 1.0f;

	if (lmg.m_params[eMotionParamID_TurnSpeed].initialized)
		lmg.m_BlendSpace.m_turn = 1.0f - 2.0f * lmg.m_params[eMotionParamID_TurnSpeed].blendspace.m_fAssetBlend;
	else if (lmg.m_params[eMotionParamID_TurnAngle].initialized)
		lmg.m_BlendSpace.m_turn = 1.0f - 2.0f * lmg.m_params[eMotionParamID_TurnAngle].blendspace.m_fAssetBlend;
	else
		lmg.m_BlendSpace.m_turn = 0.0f;




	f32 fProcTurn = lmg.m_params[eMotionParamID_TurnSpeed].blendspace.m_fAllowPrceduralTurn;
	if (Console::GetInst().ca_AMC && fProcTurn)
	{
		//test-implementation. Not enabled in th game
		f32 fDesiredTurn=0.0f;
		if (lmg.m_params[eMotionParamID_TurnSpeed].initialized)
			fDesiredTurn = lmg.m_params[eMotionParamID_TurnSpeed].value;

		lmg.m_BlendSpace.m_turn=-(fDesiredTurn*Console::GetInst().ca_AMC_TurnLeaning*fProcTurn);
		if (lmg.m_BlendSpace.m_turn>1.0f)
			lmg.m_BlendSpace.m_turn=1.0f;
		if (lmg.m_BlendSpace.m_turn<-1.0f)
			lmg.m_BlendSpace.m_turn=-1.0f;

	}


	if (Console::GetInst().ca_EnableAssetTurning)
	{
		if (m_CharEditBlendSpaceOverrideEnabled[eMotionParamID_TurnSpeed])
			lmg.m_BlendSpace.m_turn = -(1.0f - 2.0f * lmg.m_params[eMotionParamID_TurnSpeed].blendspace.m_fAssetBlend);
	}


	//TODO
	lmg.m_BlendSpace.m_slope = lmg.m_params[eMotionParamID_TravelSlope].blendspace.m_fAssetBlend * 2.0f - 1.0f;
	lmg.m_BlendSpace.m_weightshift = lmg.m_params[eMotionParamID_WeightShift].blendspace.m_fAssetBlend * 2.0f - 1.0f;

	assert(lmg.m_BlendSpace.m_strafe.GetLength() <= 1.005f);
	assert(abs(lmg.m_BlendSpace.m_speed) <= 1.005f);
	assert(abs(lmg.m_BlendSpace.m_turn) <= 1.005f);
	assert(abs(lmg.m_BlendSpace.m_slope) <= 1.005f);
	assert(abs(lmg.m_BlendSpace.m_weightshift) <= 1.005f);
}

//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
