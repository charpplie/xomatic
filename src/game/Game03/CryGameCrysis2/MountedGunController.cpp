/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Controls player movement when using a mounted gun

-------------------------------------------------------------------------
History:
- 2:10:2009   Created by Benito Gangoso Rodriguez

*************************************************************************/

#include "StdAfx.h"
#include "MountedGunController.h"

#include "Player.h"
#include "Weapon.h"

#define PLAYER_ANIMATION_LAYER_BASE				0
#define PLAYER_ANIMATION_LAYER_AIM_UP			3
#define PLAYER_ANIMATION_LAYER_AIM_DOWN		4


void CMountedGunController::Update(EntityId mountedGunID, float frameTime)
{
	CRY_ASSERT_MESSAGE(m_pControlledPlayer, "Controlled player not initialized");

	CItem* pMountedGun = static_cast<CItem*>(gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(mountedGunID));

	bool canUpdateMountedGun = (pMountedGun != NULL) && (pMountedGun->GetStats().mounted);

	if (canUpdateMountedGun)
	{
		IMovementController * pMovementController = m_pControlledPlayer->GetMovementController();
		assert(pMovementController);

		SMovementState info;
		pMovementController->GetMovementState(info);

		IEntity* pMountedGunEntity = pMountedGun->GetEntity();
		const Matrix34& lastMountedGunWorldTM = pMountedGunEntity->GetWorldTM();

		Vec3 desiredAimDirection = info.aimDirection.GetNormalized();

		// AI can switch directions too fast, prevent snapping
		if(!m_pControlledPlayer->IsPlayer())
		{
			const Vec3 currentDir = lastMountedGunWorldTM.GetColumn1();
			const float dot = clamp(currentDir.Dot(desiredAimDirection), -1.0f, 1.0f);
			const float reqAngle = cry_acosf(dot);
			const float maxRotSpeed = 2.0f;
			const float maxAngle = frameTime * maxRotSpeed;
			if(fabs(reqAngle) > maxAngle)
			{
				const Vec3 axis = currentDir.Cross(desiredAimDirection);
				if(axis.GetLengthSquared() > 0.001f) // current dir and new dir are enough different
				{
					desiredAimDirection = currentDir.GetRotated(axis.GetNormalized(),sgn(reqAngle)*maxAngle);
				}
			}
		}				
		
		bool isUserClient = m_pControlledPlayer->IsClient();

		//Adjust the orientation of the gun based on the aim-direction (position remains)
		IEntity* pMountedGunParentEntity = pMountedGunEntity->GetParent();
		const Matrix33 parentToWorld = pMountedGunParentEntity ? Matrix33(pMountedGunParentEntity->GetWorldTM()) : Matrix33(IDENTITY);
		const Matrix33 worldToParant = parentToWorld.GetInverted();
		const Vec3 localGunDirection = worldToParant * desiredAimDirection;
		
		//For client update always, for others only when there is notable change
		if (isUserClient || (!localGunDirection.IsEquivalent(lastMountedGunWorldTM.GetColumn1(), 0.003f)))
		{
			pMountedGunEntity->SetRotation(Quat::CreateRotationVDir(localGunDirection, 0.0f));
		}

		const Vec3 vInitialAimDirection = GetMountDirection(pMountedGun, pMountedGunParentEntity);
		assert( vInitialAimDirection.IsUnit() );

		//Adjust gunner position and animations
		UpdateGunnerLocation(pMountedGun, pMountedGunParentEntity, vInitialAimDirection);
		m_pControlledPlayer->GetAnimationGraphState()->SetInput("Action","gunnerMountedHMG");

		const float aimrad = Ang3::CreateRadZ(Vec2(vInitialAimDirection),Vec2(-desiredAimDirection));
		const float pitchLimit = sin_tpl(DEG2RAD(30.0f));
		const float animHeight = fabs_tpl(clamp(desiredAimDirection.z * (float)__fres(pitchLimit), -1.0f, 1.0f));

		const float aimUp = (float)__fsel(-desiredAimDirection.z, 0.0f, animHeight); 
		const float aimDown = (float)__fsel(desiredAimDirection.z, 0.0f, animHeight);

		if (m_pControlledPlayer->IsThirdPerson())
		{
			UpdateThirdPersonAnimations(m_pControlledPlayer->GetEntity()->GetCharacter(0), aimrad, aimUp, aimDown);
		}
		else
		{
			UpdateFirstPersonAnimations(pMountedGun);
			UpdateThirdPersonAnimations(m_pControlledPlayer->GetEntity()->GetCharacter(0), aimrad, aimUp, aimDown, true);
			UpdateThirdPersonAnimations(m_pControlledPlayer->GetShadowCharacter(), aimrad, aimUp, aimDown);
		}

		UpdateIKMounted(pMountedGun);

		UpdateViewLimits(pMountedGun, pMountedGunParentEntity);
	}
}


void CMountedGunController::OnEnter(EntityId mountedGunId)
{
	CRY_ASSERT_MESSAGE(m_pControlledPlayer, "Controlled player not initialized");

	ICharacterInstance* pCharacter = m_pControlledPlayer->IsThirdPerson() ? m_pControlledPlayer->GetEntity()->GetCharacter(0) : m_pControlledPlayer->GetShadowCharacter();
	if (pCharacter)
	{
		ISkeletonAnim *pSkeletonAnim = pCharacter->GetISkeletonAnim();
		assert(pSkeletonAnim);

		CItem* pWeapon = static_cast<CItem*>(g_pGame->GetIGameFramework()->GetIItemSystem()->GetItem(mountedGunId));
		assert(pWeapon);

		const SParams& pWeaponParams = pWeapon->GetParams();

		CryCharAnimationParams animParams;
		animParams.m_nFlags = CA_LOOP_ANIMATION;
		animParams.m_fTransTime = 0.1f;

		animParams.m_nLayerID = PLAYER_ANIMATION_LAYER_AIM_UP;
		animParams.m_fLayerWeight = 0.0f;

		pSkeletonAnim->StartAnimation(pWeaponParams.mountedTPAimAnims[MountedTPAimAnim::Up], animParams);

		animParams.m_nLayerID = PLAYER_ANIMATION_LAYER_AIM_DOWN;
		animParams.m_fLayerWeight = 0.0f;
		pSkeletonAnim->StartAnimation(pWeaponParams.mountedTPAimAnims[MountedTPAimAnim::Down], animParams);
	}
}


void CMountedGunController::OnLeave( )
{
	CRY_ASSERT_MESSAGE(m_pControlledPlayer, "Controlled player not initialized");

	ICharacterInstance* pCharacter = m_pControlledPlayer->IsThirdPerson() ? m_pControlledPlayer->GetEntity()->GetCharacter(0) : m_pControlledPlayer->GetShadowCharacter();
	if (pCharacter)
	{
		ISkeletonAnim *pSkeletonAnim = pCharacter->GetISkeletonAnim();
		assert(pSkeletonAnim);

		pSkeletonAnim->StopAnimationInLayer(PLAYER_ANIMATION_LAYER_AIM_UP, 0.1f);
		pSkeletonAnim->StopAnimationInLayer(PLAYER_ANIMATION_LAYER_AIM_DOWN, 0.1f);

		m_pControlledPlayer->GetActorParams()->mountedWeaponCameraTarget.zero();
	}
}


void CMountedGunController::UpdateGunnerLocation( CItem* pMountedGun, IEntity* pParent, const Vec3& bodyDirection )
{
	Vec3 newGunnerPosition(0.f,0.f,0.f);
	const SMountParams* pMountParams = pMountedGun->GetMountedParams();

	Matrix33 vInitialPlayerOrientation = Matrix33::CreateRotationVDir(bodyDirection);

	if(pMountParams)
	{
		f32 bodyDist = pMountParams->body_distance;
		f32 groundDist = pMountParams->ground_distance;
		newGunnerPosition = GetMountedGunPosition(pMountedGun, pParent) - bodyDirection*bodyDist - Vec3(0,0,1)*groundDist; 
	}

	IEntity* pControlledPlayerEntity = m_pControlledPlayer->GetEntity();
	const Matrix34& gunnerTM = pControlledPlayerEntity->GetWorldTM();
	Matrix34 newGunnerTM = vInitialPlayerOrientation;
	newGunnerTM.SetTranslation(newGunnerPosition);

	if(m_pControlledPlayer->IsClient() || !gunnerTM.IsEquivalent(newGunnerTM))
	{
		pControlledPlayerEntity->SetWorldTM(newGunnerTM, ENTITY_XFORM_USER);
	}
}


void CMountedGunController::UpdateThirdPersonAnimations( ICharacterInstance* pCharacter, float aimRad, float aimUp, float aimDown, bool firstPerson /*= false*/ )
{
	if (pCharacter)
	{
		ISkeletonAnim *pSkeletonAnim = pCharacter->GetISkeletonAnim();
		assert(pSkeletonAnim);

		//Update manually animation time, to match current weapon orientation
		uint32 numAnimsLayer = pSkeletonAnim->GetNumAnimsInFIFO(PLAYER_ANIMATION_LAYER_BASE);
		for(uint32 i=0; i<numAnimsLayer; i++)
		{
			CAnimation &animation = pSkeletonAnim->GetAnimFromFIFO(PLAYER_ANIMATION_LAYER_BASE, i);
			if (animation.m_AnimParams.m_nFlags & CA_MANUAL_UPDATE)
			{
				animation.m_fAnimTime = clamp_tpl(aimRad/gf_PI,-1.0f,+1.0f)*0.5f + 0.5f;
				animation.m_AnimParams.m_nFlags &= ~CA_DISABLE_MULTILAYER;
			}
		}

		if (firstPerson)
			return;

		//Update additive weights for aiming up/down
		pSkeletonAnim->SetLayerBlendMultiplier(PLAYER_ANIMATION_LAYER_AIM_UP, aimUp);
		pSkeletonAnim->SetAdditiveWeight(PLAYER_ANIMATION_LAYER_AIM_UP, aimUp);

		pSkeletonAnim->SetLayerBlendMultiplier(PLAYER_ANIMATION_LAYER_AIM_DOWN, aimDown);
		pSkeletonAnim->SetAdditiveWeight(PLAYER_ANIMATION_LAYER_AIM_DOWN, aimDown);
	}
}


void CMountedGunController::UpdateFirstPersonAnimations( CItem* pMountedGun )
{
	CWeapon* pMountedWeapon = (CWeapon*)(pMountedGun->GetIWeapon());
	IZoomMode* pCurrentZoomMode = pMountedWeapon ? pMountedWeapon->GetZoomMode(pMountedWeapon->GetCurrentZoomMode()) : 0;
	const SMountParams* pMountParams = pMountedWeapon->GetMountedParams();

	float ironSightWeigth = 0.0f;
	
	if (pCurrentZoomMode && (pCurrentZoomMode->IsZoomed() || pCurrentZoomMode->IsZoomingInOrOut()))
	{
		ironSightWeigth = 1.0f;
		if (pCurrentZoomMode->IsZoomingInOrOut())
		{
			ironSightWeigth = pCurrentZoomMode->GetZoomTransition();
		}
	}
	
	const Matrix34& weaponTM = pMountedWeapon->GetEntity()->GetWorldTM();
	const Vec3 localTargetPoint = pMountParams ? LERP(pMountParams->fpBody_offset, pMountParams->fpBody_offset_ironsight, ironSightWeigth) : Vec3(0.f,0.f,0.f);
	const Vec3 desiredCameraPos = weaponTM.TransformPoint(localTargetPoint);

	SActorParams* pActorParams = m_pControlledPlayer->GetActorParams();
	pActorParams->mountedWeaponCameraTarget = desiredCameraPos;
}


void CMountedGunController::UpdateIKMounted( CItem* pMountedGun )
{
	const SMountParams* pMountParams = pMountedGun->GetMountedParams();

	bool hasIKHelpers = (pMountParams != NULL) && (!pMountParams->left_hand_helper.empty()) && (!pMountParams->right_hand_helper.empty());
	if (hasIKHelpers)
	{
		bool isThirdPerson = m_pControlledPlayer->IsThirdPerson();

		const Vec3 leftHandPos = pMountedGun->GetSlotHelperPos(isThirdPerson ? eIGS_ThirdPerson : eIGS_FirstPerson, pMountParams->left_hand_helper.c_str(), true);
		const Vec3 rightHandPos = pMountedGun->GetSlotHelperPos(isThirdPerson ? eIGS_ThirdPerson: eIGS_FirstPerson, pMountParams->right_hand_helper.c_str(), true);

		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(leftHandPos, 0.075f, ColorB(255, 255, 255, 255));
		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(rightHandPos, 0.075f, ColorB(128, 128, 128, 255));

		if (isThirdPerson)
		{
			m_pControlledPlayer->SetIKPos("leftArm",		leftHandPos, 1);
			m_pControlledPlayer->SetIKPos("rightArm",	rightHandPos, 1);
		}
		else
		{
			m_pControlledPlayer->SetIKPos("leftArmShadow",		leftHandPos, 1);
			m_pControlledPlayer->SetIKPos("rightArmShadow",	rightHandPos, 1);
		}
	}
}


void CMountedGunController::UpdateViewLimits( CItem* pMountedGun, IEntity* pParent )
{
	//Most probable case, so early out
	if (pParent == NULL)
		return;

	SActorParams* pParams = m_pControlledPlayer->GetActorParams();

	pParams->vLimitDir = GetMountDirection(pMountedGun, pParent);
}

Vec3 CMountedGunController::GetMountDirection( CItem* pMountedGun, IEntity* pParent ) const
{
	if (pParent == NULL)
	{
		return pMountedGun->GetStats().mount_dir;
	}
	else
	{
		return pParent->GetWorldTM().TransformVector(pMountedGun->GetStats().mount_dir);
	}
}

Vec3 CMountedGunController::GetMountedGunPosition( CItem* pMountedGun, IEntity* pParent ) const
{
	if (pParent == NULL)
	{
		return pMountedGun->GetEntity()->GetWorldPos();
	}
	else
	{
		return pParent->GetWorldTM().TransformPoint(pMountedGun->GetEntity()->GetLocalTM().GetTranslation());
	}
}
