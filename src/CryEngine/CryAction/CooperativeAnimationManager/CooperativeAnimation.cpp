
/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: synchronized animation playing on multiple actors 
including exact positioning and sliding the actors into position
-------------------------------------------------------------------------
History:
- August 5th, 2008: Created by Michelle Martin
- February 5th, 2009: Moved to CryAction by David Ramos
*************************************************************************/


#include "StdAfx.h"
#include "CooperativeAnimation.h"
#include "CooperativeAnimationManager.h"
#include "CryActionCVars.h"
#include "IAnimatedCharacter.h"
#include "IGameFramework.h"

const static char *COOP_ANIM_NAME = "CoopAnimation";
const static int MY_ANIM = 54545;

CCooperativeAnimation::CCooperativeAnimation()
{
	m_isInitialized = false;
}

CCooperativeAnimation::~CCooperativeAnimation()
{
	if (m_activeState != eCS_ForceStopping)
	{
		// Delete the params from the vector
		TParamsList::iterator it = m_paramsList.begin();
		TParamsList::iterator iend = m_paramsList.end();
		
		for (; it != iend; ++it)
		{
			if (!(*it).bAnimationPlaying)
				CleanupForFinishedCharacter((*it));
		}
	}
	
	m_paramsList.clear();
}

bool CCooperativeAnimation::IsUsingActor( const IAnimatedCharacter* pActor ) const
{
	if (!m_isInitialized)
		return false;

	if(m_activeState == eCS_ForceStopping)
		return false;

	CRY_ASSERT(pActor);

	// Return true if the actor is in one of the params (and still needed)
	TParamsList::const_iterator it = m_paramsList.begin();
	TParamsList::const_iterator iend = m_paramsList.end();

	for (; it != iend; ++it)
	{
		// Check if this character is used AND if he is actually still
		// playing the animation (if he has finished, the character isn't needed anymore) AND
		// not being rough aligned or sliding
		// (bAnimationPlaying is FALSE then, but you're using the actor)
		if ((*it).pActor == pActor && 
			((*it).bAnimationPlaying ||
			(m_activeState == eCS_PlayAndSlide)))
			return true;
	}

	return false;
}

bool CCooperativeAnimation::IsUsingActor( const EntityId entID ) const
{
	if (!m_isInitialized)
		return false;

	CRY_ASSERT(entID > 0);

	// Return true if the actor is in one of the params (and still needed)
	TParamsList::const_iterator it = m_paramsList.begin();
	TParamsList::const_iterator iend = m_paramsList.end();

	for (; it != iend; ++it)
	{
		// Check if this character is used AND if he is actually still
		// playing the animation (if he has finished, the character isn't needed anymore) AND
		// not being rough aligned or sliding (bAnimationPlaying is FALSE then, but you're 
		// using the actor)
		if ((*it).pActor->GetEntityId() == entID && 
			((*it).bAnimationPlaying || (m_activeState == eCS_PlayAndSlide)))
			return true;
	}

	return false;
}

bool CCooperativeAnimation::AreActorsAlive()
{
	if (!m_isInitialized)
		return false;

	// Return true if all actors have health above zero
	TParamsList::iterator it = m_paramsList.begin();
	TParamsList::iterator iend = m_paramsList.end();

	for (; it != iend; ++it)
	{
		IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(it->pActor->GetEntityId());
		CRY_ASSERT(pActor);
		if(pActor)
		{
			if (pActor->GetHealth() <= 0)
				return false;
		}
	}

	return true;
}

bool CCooperativeAnimation::AreActorsValid() const
{
	if (!m_isInitialized)
		return false;

	TParamsList::const_iterator itEnd = m_paramsList.end();
	TParamsList::const_iterator it = std::find_if(m_paramsList.begin(), itEnd, 
		std::not1(std::mem_fun_ref(&SCharacterParams::IsActorValid)));

	return (it == itEnd);
}

bool CCooperativeAnimation::Update( float dt )
{
	if (!m_isInitialized )
		return false;

	if (!AreActorsValid())
	{
		Stop();
		return false;
	}

	if ( !m_generalParams.bIgnoreCharacterDeath && !AreActorsAlive() )
	{
		Stop();
		return false;
	}

	switch(m_activeState)
	{
	case eCS_PlayAndSlide:
		{
			bool bAnimFailure, bAllStarted, bAllDone;
			PlayAndSlideCharacters(dt, bAnimFailure, bAllStarted, bAllDone);

			if (bAnimFailure)
				return false;

			if (!bAllStarted)
				break;

			if (bAllDone)
				m_activeState = eCS_WaitForAnimationFinish;
		}
		
		// no break, so the next case gets executed as well
	case eCS_WaitForAnimationFinish:
		if (AnimationsFinished())
		{
			return false;
		}
		break;

	case eCS_ForceStopping:
		return false;
		break;
	};

	// DEBUG OUTPUT
	if (CCryActionCVars::Get().co_coopAnimDebug)
	{
		IRenderAuxGeom* pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom(); 
		pRenderAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );

		// the debug output is different depending on the alignment type
		if (m_generalParams.eAlignment == eAF_FirstActorPosition)
		{
			// draw start position
			pRenderAuxGeom->DrawSphere( m_firstActorStartPosition, 0.1f, ColorB( 255, 0, 255 ) );
			// draw start direction
			pRenderAuxGeom->DrawLine( m_firstActorStartPosition + Vec3(0,0,0.2f), ColorB( 255, 0, 255 ), m_firstActorStartPosition + m_firstActorStartDirection + Vec3(0,0,0.2f), ColorB( 255, 0, 255 ) );
			// draw target position
			pRenderAuxGeom->DrawSphere( m_firstActorTargetPosition, 0.1f, ColorB( 255, 0, 255 ) );
			// draw target direction
			pRenderAuxGeom->DrawLine( m_firstActorTargetPosition + Vec3(0,0,0.2f), ColorB( 0, 255, 0 ), m_firstActorTargetPosition + m_firstActorTargetDirection + Vec3(0,0,0.2f), ColorB( 0, 255, 0 ) );

			// draw the pushed distance
			Vec3 pushDirection = (m_firstActorTargetPosition - m_firstActorStartPosition).GetNormalized();
			pRenderAuxGeom->DrawLine( m_firstActorStartPosition + Vec3(0,0,0.05f), ColorB( 255, 255, 0 ), m_firstActorStartPosition + Vec3(0,0,0.05f) + pushDirection * m_firstActorPushedDistance, ColorB( 255, 255, 0 ) );
		}
	} // end of DEBUG OUTPUT

	return true;
}

void CCooperativeAnimation::Stop()
{
	m_activeState = eCS_ForceStopping;

	TParamsList::iterator it = m_paramsList.begin();
	TParamsList::iterator iend = m_paramsList.end();

	for (; it != iend; ++it)
	{
		// Don't check if the animation is currently playing, because we want
		// to clean up the current animations to make way for the next ones
		CleanupForFinishedCharacter((*it));
	}
}

int MovementCallback(ICharacterInstance *pCharacter,void *pEntity)
{
	IEntity * pEnt = static_cast<IEntity*>(pEntity);
	ISkeletonAnim* pISkeletonAnim = pEnt->GetCharacter(0)->GetISkeletonAnim();

	// this is the kinematic update
	const QuatT& relmove = pISkeletonAnim->GetRelMovement();

	QuatT chrLoc;
	chrLoc.q = pEnt->GetWorldRotation();
	chrLoc.t = pEnt->GetWorldPos();
	chrLoc = chrLoc * relmove; 
	chrLoc.q.NormalizeSafe();

	pEnt->SetWorldTM(Matrix34(chrLoc));

	// Vec3 dir = chrLoc.q * FORWARD_DIRECTION; <- ??

	return 0;
}


bool CCooperativeAnimation::Init( const SCharacterParams &params1, const SCharacterParams &params2, const SCooperativeAnimParams &generalParams)
{
	if (m_isInitialized)
		return true;

	m_paramsList.push_back(params1);
	m_paramsList.push_back(params2);
	m_generalParams = generalParams;

	ICVar* pUseNewCoopSystemVar = gEnv->pConsole->GetCVar( "co_usenewcoopanimsystem" );
	m_bPauseAG = generalParams.bLooping || (pUseNewCoopSystemVar && pUseNewCoopSystemVar->GetIVal());  // for now only use the new system on looping animations, where it is properly tested
	m_iOldFlags = 0;

	// sanity check parameters
	TParamsList::iterator it = m_paramsList.begin();
	TParamsList::iterator iend = m_paramsList.end();
	for (; it != iend; ++it)
	{
		if ((*it).pActor == NULL)
		{
			CryLogAlways("Cooperative Animation Manager was passed a NULL pointer as an actor");
			return false;
		}

		if ((*it).sSignalName == "")
		{
			CryLogAlways("Cooperative Animation Manager was not passed a correct signal name");
			return false;
		}
	}

	// Calculate the reference point and calculate the target
	// position and orientation of every character relative to this.
	DetermineReferencePositionAndCalculateTargets();

	// After all parameters are checked,
	// determine the type of needed character alignment
	m_activeState = eCS_PlayAndSlide;
	
	m_isInitialized = true;
	return true;
}

bool CCooperativeAnimation::InitForOne( const SCharacterParams &params, const SCooperativeAnimParams &generalParams )
{
	if (m_isInitialized)
		return true;

	m_paramsList.push_back(params);
	m_generalParams = generalParams;
	ICVar* pUseNewCoopSystemVar = gEnv->pConsole->GetCVar( "co_usenewcoopanimsystem" );
	m_bPauseAG = generalParams.bLooping || (pUseNewCoopSystemVar && pUseNewCoopSystemVar->GetIVal());  // for now only use the new system on looping animations, where it is properly tested
	m_iOldFlags = 0;

	// sanity check parameters
	if (params.pActor == NULL)
	{
		CryLogAlways("Cooperative Animation Manager was passed a NULL pointer as an actor");
		return false;
	}

	if (params.sSignalName == "")
	{
		CryLogAlways("Cooperative Animation Manager was not passed a correct animation or signal name");
		return false;
	}

	// Sanity check the alignment type - not all make sense for a single character
	if (generalParams.eAlignment == eAF_WildMatch || generalParams.eAlignment == eAF_FirstActor)
		m_generalParams.eAlignment = eAF_FirstActorNoRot;

	// Calculate the reference point and calculate the target
	// position and orientation of every character relative to this.
	DetermineReferencePositionAndCalculateTargets();

	// After all parameters are checked,
	// determine the type of needed character alignment
	m_activeState = eCS_PlayAndSlide;

	m_isInitialized = true;
	return true;
}


bool CCooperativeAnimation::StartAnimations()
{
	// start animations for all characters
	TParamsList::iterator it = m_paramsList.begin();
	TParamsList::iterator iend = m_paramsList.end();
	SCharacterParams *params;
	for (int i = 0; it != iend; ++it, ++i)
	{
		// cast into the correct class
		params = &(*it);  // looks overly complicated, but is needed to compile for console

		if ( !StartAnimation( *params, i ) )
			return false;
	}

	return true;
}

bool CCooperativeAnimation::StartAnimation( SCharacterParams &params, int characterIndex )
{
	CRY_ASSERT(params.pActor);
	IAnimatedCharacter* pAnimChar = params.pActor;
	CRY_ASSERT(pAnimChar);

	IAnimationGraphState *pGraphState = pAnimChar->GetAnimationGraphState();
	CRY_ASSERT_MESSAGE(pGraphState, "Cannot retrieve animation graph state");

	// Gather the location offset information before the animation moves the character
	GatherSlidingInformation( params );

	if (m_bPauseAG)
	{
		CAnimationPlayerProxy* pAnimProxy = pAnimChar->GetAnimationPlayerProxy(eAnimationGraphLayer_FullBody);
		CRY_ASSERT(pAnimProxy);
		CryCharAnimationParams AParams;

		// prepare animation parameters
		string sAnimName = params.sSignalName;

		AParams.m_nLayerID				= 0;
		AParams.m_fPlaybackSpeed	=	1.0f;
		AParams.m_fTransTime			=	0.25f;
		AParams.m_fAllowMultilayerAnim = 0;
		AParams.m_nFlags |= CA_REPEAT_LAST_KEY | CA_FORCE_SKELETON_UPDATE | CA_FULL_ROOT_PRIORITY | CA_DISABLE_MULTILAYER;
		AParams.m_nUserToken = MY_ANIM;
		if (m_generalParams.bLooping)
			AParams.m_nFlags |= CA_LOOP_ANIMATION;

		pAnimProxy->StartAnimation(params.pActor->GetEntity(), sAnimName.c_str(), /*0,*/ AParams);

//		if (characterIndex != 0)  // do not stop the graph for the master (if started from AG, not FG)
			pGraphState->Pause(true, eAGP_PlayAnimationNode);

		// backup animated character params
		SAnimatedCharacterParams paramsAC = pAnimChar->GetParams();
		m_iOldFlags = paramsAC.flags;
		paramsAC.flags &= ~(eACF_AlwaysAnimation | eACF_AlwaysPhysics | eACF_PerAnimGraph);
		paramsAC.flags |= eACF_AlwaysAnimation; // | eACF_Frozen | eACF_NoLMErrorCorrection | eACF_ZCoordinateFromPhysics;
		pAnimChar->SetParams( paramsAC );
	}
	else
	{
		// If we're currently in a coop anim, we need to check which of the two states we're in and 
		// switch to the other one so we can blend nicely into the next anim
		const char *cSignal = "coopanimation";
		const char *cOutPut = pGraphState->QueryOutput("CoopAnim");
		if ( cOutPut && cOutPut[0] == '1' )
		{
			cSignal = "coopanimation2";
		}

		// Set the signal (the "optional" prevents inputs to be set to defaults when the value doesn't
		// exist. Therefore, running animations won't be interrupted if this fails.
		if (!pGraphState->SetInputOptional("Signal", cSignal))
		{
			CRY_ASSERT_MESSAGE(0, "Could not set animation graph input 'coopanimation'.");
			CryLogAlways("Cooperative Animation start failed - animation graph not prepared?");
			return false;
		}

		// Set the variation
		if (!pGraphState->SetVariationInput("CoopAnimation", params.sSignalName))
		{
			CRY_ASSERT_MESSAGE(0, "Could not set animation graph variation 'CoopAnimation'.");
			CryLogAlways("Cooperative Animation start failed - animation graph not prepared?");
			return false;
		}

		// Note: This would cause the animation currently in layer1 to be removed, whether
		// or not RepeatLastKey is set to "1" in the template of the prev state.
		// So instead an update will have the same effect, since the template has
		// the property interruptCurrAnim set to "1".
		// In rare cases this might not cause the anim graph to jump to the desired state -
		// in this case it can be enforced. (see below)
		//params.pActor->GetAnimationGraphState()->ForceTeleportToQueriedState();

		//double check that the teleport worked
		pGraphState->Update();

		cOutPut = pGraphState->QueryOutput("CoopAnim");
		if ( !cOutPut || cOutPut[0] == '\0' )
		{
			CRY_ASSERT_MESSAGE(0, "Could not retrieve correct output from AG");
			CryLogAlways("Cooperative Animation - state teleport failed? (Could not retrieve correct output from AG graph state %s)", pGraphState->GetCurrentStateName());

			if (m_generalParams.bForceStart)
			{
				// if the animation should force start anyway, the new state can be teleported to,
				// but this will result in a snap.
				pGraphState->ForceTeleportToQueriedState();
			}
			else
			{
				return false;
			}
		}
	}  // old system

	params.bAnimationPlaying = true;
	params.pActor->SetNoMovementOverride(true);
	//IEntity * pEnt = params.pActor->GetEntity();
	//pEnt->GetCharacter(0)->GetISkeletonAnim()->SetLocomotionMacroCallback(MovementCallback, pEnt);
	SetColliderModeAndMCM(&params);

	return true;
}

bool CCooperativeAnimation::IsAnimationPlaying(const ISkeletonAnim *pISkeletonAnim, ICharacterInstance* pICharacterInstance, CAnimation* pAnim)
{
	if (m_generalParams.bLooping)
	{
		return true;
	}

	IAnimationSet* pAnimationSet = pICharacterInstance->GetIAnimationSet();
	f32 fDuration=0.0f;
	int32 nLMGAnimID = pAnim->m_Parametric.m_nParametricID;
	if (nLMGAnimID<0)
	{
		//caf file
		int32	nCAFID = pAnim->m_Parametric.m_nAnimID[0];
		fDuration = pAnimationSet->GetDuration_sec(nCAFID);
	}
	else
	{
		//parametric animation
		uint32 numAnims = pAnim->m_Parametric.m_numAnims;
		for (uint32 i=0; i<numAnims; i++)
		{
			int32 nAnimID   =	pAnim->m_Parametric.m_nAnimID[i];
			f32   fWeight		=	pAnim->m_Parametric.m_fBlendWeight[i];
			fDuration += pAnimationSet->GetDuration_sec(nAnimID) * fWeight;
		}
	} 

	// more than 0.2f seconds left?
	const float fLayerTime = pISkeletonAnim->GetLayerTime(0)*fDuration;
	const float fTimeRemaining = fDuration - fLayerTime;
	if ( fTimeRemaining > 0.0f)
		return true;

	return false;
}

bool CCooperativeAnimation::AnimationsFinished()
{
	// Checks if all characters have finished playing their animation and return true, if they have

	TParamsList::iterator it = m_paramsList.begin();
	TParamsList::iterator iend = m_paramsList.end();
	SCharacterParams *params;
	int finishedCount = m_paramsList.size();
	for (; it != iend; ++it)
	{
		// cast into the correct class
		params = &(*it);  // looks overly complicated, but is needed to compile for console

		bool animationPlaying = false;

		if (m_bPauseAG)
		{
			ICharacterInstance* pICharacterInstance = params->pActor->GetEntity()->GetCharacter(0);
			ISkeletonAnim *rISkeletonAnim = pICharacterInstance->GetISkeletonAnim();
			CRY_ASSERT(rISkeletonAnim);
			int iAnimCount = rISkeletonAnim->GetNumAnimsInFIFO(0);

			CAnimation* pAnim = rISkeletonAnim->FindAnimInFIFO(MY_ANIM,0, true);
			if (pAnim)
				animationPlaying = IsAnimationPlaying(rISkeletonAnim, pICharacterInstance, pAnim);
		}
		else
		{
			const char* stateName = params->pActor->GetAnimationGraphState()->GetCurrentStateName();
			if ( !strnicmp(stateName, "x_coopAnimation", 10) ) //this will match to x_coopAnimation2 as well
				animationPlaying = true;
		}

		if ( !params->bAnimationPlaying || !animationPlaying )
		{
			--finishedCount;
			if (params->bAnimationPlaying)
			{
				params->bAnimationPlaying = false;
				CleanupForFinishedCharacter(*params);
			}
		}
		else
		{
			SetColliderModeAndMCM(params);
		}
	}

	if (finishedCount <= 0)
		return true;

	return false;
}

void CCooperativeAnimation::GatherSlidingInformation( SCharacterParams &params ) const
{
	// figures out how much the characters still need to slide

	// get difference in position and rotation from current position to target position
	Quat pRot;
	Vec3 pPos;
	
	// get current position and rotation
	IEntity *pEnt = params.pActor->GetEntity();
	CRY_ASSERT( pEnt );
	pRot = pEnt->GetWorldRotation();
	pPos = pEnt->GetWorldPos();

	params.currPos.t = pPos;
	params.currPos.q = pRot;
	
	// save the remaining differences to the target pos and orientation
	params.qSlideOffset.t = params.qTarget.t - pPos;
	params.qSlideOffset.q = params.qTarget.q * (!pRot);
	params.qSlideOffset.q.NormalizeSafe();
	
	// Note: After this, the target position (qTarget) in the params can no longer be
	// be used as reference. The animations will most likely move and rotate the locator.
}

void CCooperativeAnimation::PlayAndSlideCharacters( float dt, bool &bAnimFailure, bool &bAllStarted, bool &bAllDone )
{
	// Slide characters into position quickly and return true when done
	bAllStarted = true;
	bAllDone = true;
	bAnimFailure = false;

	TParamsList::iterator it = m_paramsList.begin();
	TParamsList::iterator iend = m_paramsList.end();
	SCharacterParams *params;
	IEntity *pEnt = NULL;
	IAnimatedCharacter *pAC = NULL;
	Quat pRot;
	Vec3 pPos;

	for (int i = 0; it != iend; ++it, ++i)
	{
		// cast into the correct class
		params = &(*it);  // looks overly complicated, but is needed to compile for console
		pEnt = params->pActor->GetEntity();
		pAC = params->pActor;
		CRY_ASSERT(pEnt && pAC);

		// Delayed start handling
		if (!params->bAnimationPlaying)
		{
			if (params->fStartDelay > 0.0f)
			{
				params->fStartDelayTimer += dt;
				if (params->fStartDelayTimer < params->fStartDelay)
				{
					bAllStarted = bAllDone = false;
					continue; // Skip any more processing of this character until start delay is up
				}
			}
			
			if (!StartAnimation(*params, i))
			{
				bAnimFailure = true; // tell the calling function to abort the cooperative animation
				bAllStarted = bAllDone = false;
				return; 
			}
		}

/*
		// This commented code will teleport the characters to their target positions directly
		// and is useful for debugging this system.

		pEnt->SetPos(params->qTarget.t);
		pEnt->SetRotation(params->qTarget.q);
		params->pActor->GetAnimatedCharacter()->SetNoMovementOverride(false);
		continue;
*/

		// check if this character still needs to slide or is already done
		if (params->fSlidingTimer >= params->fSlidingDuration)
		{
			pAC->SetNoMovementOverride(false);
			//pEnt->GetCharacter(0)->GetISkeletonAnim()->SetLocomotionMacroCallback(MovementCallback, pEnt);
			continue;
		}

		// calculate the amount of movement that needs to be done this frame
		float slideAmount = dt;

		// check if this is the last step for this character
		if (params->fSlidingTimer + dt >= params->fSlidingDuration)
		{
			slideAmount = params->fSlidingDuration - params->fSlidingTimer;
			pAC->SetNoMovementOverride(false);
			//pEnt->GetCharacter(0)->GetISkeletonAnim()->SetLocomotionMacroCallback(MovementCallback, pEnt);
		}
		else
		{
			// there are more steps necessary
			bAllDone = false;
		}

		// Calculate new character position and rotation
		slideAmount = slideAmount / params->fSlidingDuration;
		pRot = pEnt->GetWorldRotation();
		pPos = pEnt->GetWorldPos();

		/*
		pPos += slideAmount * params->qSlideOffset.t;
		pRot = pRot * params->qSlideOffset.q.CreateSlerp(Quat(IDENTITY), params->qSlideOffset.q, slideAmount);
		pEnt->SetPos(pPos);
		pEnt->SetRotation(pRot);
		*/

		QuatT testQuat(IDENTITY);
		testQuat = params->currPos;
		Vec3 dist;
		dist = params->qSlideOffset.t;
		dist = dist * slideAmount;
		testQuat.t += dist;
		
		// Safety check to guarantee character will not fall through ground

		if (m_generalParams.bPreventFallingThroughTerrain)
			testQuat.t.z = max( testQuat.t.z, gEnv->p3DEngine->GetTerrainElevation( testQuat.t.x, testQuat.t.y, true ) );
		
		testQuat.q = testQuat.q * params->qSlideOffset.q.CreateSlerp(Quat(IDENTITY), params->qSlideOffset.q, slideAmount);
		testQuat.q.NormalizeSafe();
		params->currPos = testQuat;

		pEnt->SetPos(testQuat.t);
		pEnt->SetRotation(testQuat.q);


		// increase the sliding timer for this character
		params->fSlidingTimer += dt;

		// DEBUG OUTPUT
		if (CCryActionCVars::Get().co_coopAnimDebug)
		{
			IRenderAuxGeom* pRenderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom(); 
			pRenderAuxGeom->SetRenderFlags( e_Def3DPublicRenderflags );

			// draw an indication that this character is being sled
			pRenderAuxGeom->DrawSphere( pPos + Vec3(0,0,2.0f), 0.1f, ColorB( 255, 255, 0 ) );

			// add to the amount that this character has already been moved
			if (it == m_paramsList.begin())
				m_firstActorPushedDistance += (slideAmount * params->qSlideOffset.t).GetLength();
		} // end of DEBUG OUTPUT
	}

	return;
}


void CCooperativeAnimation::DetermineReferencePositionAndCalculateTargets()
{
	// Right now, only the reference type FirstCharacter is supported
	SCharacterParams *params;

	// Depending on the chosen alignment type the position and orientation of the reference
	// position is determined quite differently.
	switch(m_generalParams.eAlignment)
	{
	case eAF_Location:
		{
			// the actual reference position and orientation has been specified.

			m_refPoint = m_generalParams.qLocation;
		}
		break;
	case eAF_FirstActorPosition:
		{
			// the position and orientation for the first actor has been specified and
			// all must adjust accordingly

			// Reference Point is target position of the first character minus
			// the relative location of his start location
			params = &(*(m_paramsList.begin()));  // looks overly complicated, but is needed to compile for console
			QuatT relativeOffset = GetTargetPosition(*params);
			IEntity *pEnt = params->pActor->GetEntity();
			m_refPoint.q = m_generalParams.qLocation.q;

			// rotating the actor rotation backwards with the rotation from the animation
			m_refPoint.q = m_refPoint.q * (!relativeOffset.q);

			// now get the correct world space vector that point from the actor to the
			// ref point (also in world space)
			Vec3 chartoRef = m_refPoint.q * (-relativeOffset.t);
			m_refPoint.t = m_generalParams.qLocation.t + chartoRef;

			// DEBUG OUTPUT
			// Set all the values
			if (CCryActionCVars::Get().co_coopAnimDebug)
			{
				Matrix34 firstActorMat = pEnt->GetWorldTM();
				m_firstActorStartPosition = firstActorMat.GetTranslation();
				m_firstActorStartDirection = firstActorMat.GetColumn1();
				m_firstActorTargetPosition = m_generalParams.qLocation.t;
				m_firstActorTargetDirection = m_generalParams.qLocation.q * FORWARD_DIRECTION;
				m_firstActorPushedDistance = 0.0f;
			} // end of DEBUG OUTPUT

		}
		break;
	case eAF_FirstActor:
	case eAF_FirstActorNoRot:
		{
			// first actor (not maintaining rotation)

			// Reference Point is the position of the first character minus
			// the relative location of his start location
			params = &(*(m_paramsList.begin()));  // looks overly complicated, but is needed to compile for console
			QuatT relativeOffset = GetTargetPosition(*params);
			IEntity *pEnt = params->pActor->GetEntity();
			m_refPoint.q = pEnt->GetWorldRotation();

			if (m_generalParams.eAlignment == eAF_FirstActorNoRot)
			{
				m_refPoint.q = relativeOffset.q;
			}
			else
			{
				// rotating the actor rotation backwards with the rotation from the animation
				m_refPoint.q = m_refPoint.q * (!relativeOffset.q);
			}

			// now get the correct world space vector that points from the actor to the
			// ref point (also in world space)
			Vec3 chartoRef = m_refPoint.q * (-relativeOffset.t);
			m_refPoint.t = pEnt->GetWorldPos() + chartoRef;
		}
		break;
	case eAF_WildMatch:
	default:
		{
			// Wild Matching
			// (meaning least amount of movement, rather rotate than change position)

			// FIRST ITERATION: Only support for two characters

			// rotate towards the second character before making this the reference point
			// (if this step is left out, the entity rotation of the actor will be preserved,
			// but that causes big distances for all other characters to their target positions)

			// Reference Point is the position of the first character minus
			// the relative location of his start location
			params = &(*(m_paramsList.begin()));  // looks overly complicated, but is needed to compile for console
			QuatT relativeOffset = GetTargetPosition(*params);
			IEntity *pEnt = params->pActor->GetEntity();
			m_refPoint.q = pEnt->GetWorldRotation();

			// rotating the actor rotation backwards with the rotation from the animation
			m_refPoint.q = m_refPoint.q * (!relativeOffset.q);

			// Get the vector from first to second character as it is in the animation
			SCharacterParams paramsSecond = (m_paramsList.at(1));
			Vec3 secondPos = GetTargetPosition(paramsSecond).t;
			secondPos = (secondPos - relativeOffset.t);

			// rotate it into world space
			secondPos = m_refPoint.q * secondPos;
			secondPos.z = 0.0f; // only interested in the rotation around the up vector

			// Get the vector between first and second char in world space
			Vec3 firstToSecW = paramsSecond.pActor->GetEntity()->GetWorldPos() - pEnt->GetWorldPos();
			firstToSecW.z = 0.0f; // only interested in the rotation around the up vector

			bool invalidOffset = false;
			if (firstToSecW.IsZero() || secondPos.IsZero())
				invalidOffset = true;	// this is ok, but it means that wild match cannot be used

			secondPos.normalize();
			firstToSecW.normalize();

			// get the rotation between those vectors (z axis rotation, from worldDir to animDir)
			float fDot = secondPos * firstToSecW;
			fDot = clamp(fDot,-1.0f,1.0f);
			float angle = cry_acosf(fDot);

			// cross product to determine if the angle needs to be flipped - but only z component is relevant
			// so save processing time and not calculate the full cross
			if (((firstToSecW.x * secondPos.y) - (firstToSecW.y * secondPos.x)) < 0)
				angle = -angle;

			// Create a rotation out of this
			Quat rot(IDENTITY);
			rot.SetRotationAA(angle, Vec3(0.0f,0.0f,1.0f));

			// rotate the reference-QuatT by it
			if (rot.IsValid() || invalidOffset)  // safety check
				m_refPoint.q = m_refPoint.q * (!rot);
			else
			{
				CRY_ASSERT_MESSAGE(0, "Possibly an invalid quaternion, or only z deviation in animations.");
			}

			// now get the correct world space vector that point from the actor to the
			// ref point (also in world space)
			Vec3 chartoRef = m_refPoint.q * (-relativeOffset.t);
			m_refPoint.t = pEnt->GetWorldPos() + chartoRef;
		} // end of wild matching
		break;
	}


	// Now go through all characters and calculate their final target positions
	TParamsList::iterator it = m_paramsList.begin();
	TParamsList::iterator itend = m_paramsList.end();
	for (; it != itend; ++it)
	{
		// cast into the correct class
		params = &(*it);  // looks overly complicated, but is needed to compile for console

		// Calculate target position
		QuatT relativeOffset = GetTargetPosition(*params);
		params->qTarget = m_refPoint * relativeOffset;
		params->qTarget.q.NormalizeSafe();
		//params->qSlideOffset.q.NormalizeSafe();
	}
}

QuatT CCooperativeAnimation::GetTargetPosition( SCharacterParams &params )
{
	QuatT retVal(IDENTITY);

	if (!params.pActor)
	{
		CRY_ASSERT_MESSAGE(params.pActor, "No valid Actor received!");
		return retVal;
	}

	ICharacterInstance* pCharacter = params.pActor->GetEntity()->GetCharacter(0);
	if (pCharacter == NULL)
		return retVal;

	IAnimationSet* pAnimSet = pCharacter->GetIAnimationSet();
	if (pAnimSet == NULL)
		return retVal;

	//string animation0 = pState == NULL ? m_animations[0].c_str() : pState->ExpandVariationInputs( m_animations[0].c_str() ).c_str();
	string animation0 = params.sSignalName;

	int id = pAnimSet->GetAnimIDByName(animation0.c_str());
	if (id < 0)
		return retVal;

	uint32 flags = pAnimSet->GetAnimationFlags(id);
	if ((flags & CA_ASSET_CREATED) == 0)
		return retVal;

	float animDuration = pAnimSet->GetDuration_sec(id);
	if (animDuration < params.fSlidingDuration)
	{
		CRY_ASSERT_MESSAGE(animDuration > params.fSlidingDuration, "Incorrect parameter: Sliding Duration longer than animation.");
		CryLogAlways("Warning: sliding duration longer than actual animation. Will adjust given parameter.");
		params.fSlidingDuration = min(animDuration, animParamDefaultSlidingDuration);
	}
	
	// the first key will always be in the center so this helper function will
	// reconstruct the actual starting location
	retVal = pAnimSet->GetAnimationStartLocation(animation0.c_str());
	retVal.q.NormalizeSafe();

	return retVal;
}

void CCooperativeAnimation::CleanupForFinishedCharacter( SCharacterParams &params )
{
	if (!params.pActor)
		return;

	IAnimatedCharacter *pAC = params.pActor;

	const ICooperativeAnimationManager *const pCAManager = gEnv->pGame->GetIGameFramework()->GetICooperativeAnimationManager();
	CRY_ASSERT(pCAManager);

	CRY_ASSERT_MESSAGE(!pCAManager->IsActorBusy(params.pActor, this), "Cleaning up for a character that's already playing a second animation");

	// reset the movementOverride for the characters that haven't
	// finished their animations yet (in case of premature termination)
	if (!params.bAllowHorizontalPhysics)
	{
		pAC->SetNoMovementOverride(false);
		params.pActor->GetEntity()->GetCharacter(0)->GetISkeletonAnim()->SetPreProcessCallback(NULL, NULL);
	}

	pAC->RequestPhysicalColliderMode(eColliderMode_Pushable, eColliderModeLayer_Game, COOP_ANIM_NAME);

	// enable look ik for this character again
	pAC->AllowLookIk(true);

	if (m_bPauseAG)
	{
		ISkeletonAnim *pISkeletonAnim = params.pActor->GetEntity()->GetCharacter(0)->GetISkeletonAnim();
		CRY_ASSERT(pISkeletonAnim);

		// reactivate animation graph
		pAC->GetAnimationGraphState()->Pause(false, eAGP_PlayAnimationNode);
		//pAC->GetAnimationGraphState()->Hurry();
		//pAC->GetAnimationGraphState()->Reset();
		//pAC->GetAnimationGraphState()->Update();

		if(m_iOldFlags)
		{
			SAnimatedCharacterParams paramsAC = pAC->GetParams();
			paramsAC.flags = m_iOldFlags;
			pAC->SetParams( paramsAC );
		}

		// blend out previous animation and start a new one
/*
		int iAnimCount = pISkeletonAnim->GetNumAnimsInFIFO(0);
		
		for (int i = 0; i < iAnimCount; ++i)
		{
			CAnimation anim = pISkeletonAnim->GetAnimFromFIFO(0, i);
			if (anim.m_AnimParams.m_nUserToken == MY_ANIM)
			{
				pISkeletonAnim->RemoveAnimFromFIFO(0, i);
				//pAnimProxy->StopAnimationInLayer(0, 0.20f);
			}
		}
*/

		//pAC->GetAnimationGraphState()->Hurry();
		pAC->GetAnimationGraphState()->Update();
	}
}


void CCooperativeAnimation::SetColliderModeAndMCM(SCharacterParams *params)
{
	EMovementControlMethod eMoveVertical = eMCM_Animation;

	if (m_bPauseAG)
		params->pActor->ForceRefreshPhysicalColliderMode();

	if (params->bAllowHorizontalPhysics)
	{
		params->pActor->SetMovementControlMethods(eMCM_AnimationHCollision, eMoveVertical);
		params->pActor->RequestPhysicalColliderMode(eColliderMode_Pushable, eColliderModeLayer_Game, COOP_ANIM_NAME);
	}
	else
	{
		if (params->fSlidingTimer >= params->fSlidingDuration)
		{
			//params->pActor->GetEntity()->GetCharacter(0)->GetISkeletonAnim()->SetPreProcessCallback(MovementCallback, params->pActor->GetEntity());
			//params->pActor->SetNoMovementOverride(true);
		}
		params->pActor->SetMovementControlMethods(eMCM_Animation, eMoveVertical);
		params->pActor->RequestPhysicalColliderMode(eColliderMode_PushesPlayersOnly, eColliderModeLayer_Game, COOP_ANIM_NAME);
	}

	// disable look ik for this character
	ISkeletonPose* pISkeletonPose = params->pActor->GetEntity()->GetCharacter(0)->GetISkeletonPose();
	CRY_ASSERT(pISkeletonPose);
	pISkeletonPose->SetLookIK( false, 0, Vec3(ZERO) );
	params->pActor->AllowLookIk(false);
}

float CCooperativeAnimation::GetAnimationTime( const EntityId entID ) const
{
	ISkeletonAnim *pISkeletonAnim = gEnv->pEntitySystem->GetEntity(entID)->GetCharacter(0)->GetISkeletonAnim();
	int iAnimCount = pISkeletonAnim->GetNumAnimsInFIFO(0);

	for (int i = 0; i < iAnimCount; ++i)
	{
		CAnimation anim = pISkeletonAnim->GetAnimFromFIFO(0, i);
		if (anim.m_AnimParams.m_nUserToken == (int16)MY_ANIM)
		{
			return anim.m_fAnimTime;
		}
	}

	return -1.0f;
}
