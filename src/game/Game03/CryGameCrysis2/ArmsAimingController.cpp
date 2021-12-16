#include "StdAfx.h"
#include "ArmsAimingController.h"

#include "Player.h"


CArmsAimingController::CArmsAimingController()
{

}

void CArmsAimingController::Update(const bool aimEnabled, const SPlayerParams& params, const SMovementState& curMovementState, ICharacterInstance& characterInstance)
{
	if (aimEnabled && !params.leftArmAimPose.empty() && !params.bothArmsAimPose.empty() && !params.rightArmAimPose.empty())
	{
		CryCharAnimationParams animParams;
		animParams.m_nLayerID = 1;
		animParams.m_fTransTime = 0.5f;
		animParams.m_nFlags |= CA_LOOP_ANIMATION;

		const Vec3 rotatedMovementDirection(-curMovementState.movementDirection.y, curMovementState.movementDirection.x, 0.0f);
		const float rotatedAimDotMovement = curMovementState.aimDirection.dot(rotatedMovementDirection);
		const float aimDotMovement = curMovementState.aimDirection.dot(curMovementState.movementDirection);

		const char* animName = params.bothArmsAimPose;

		if (aimDotMovement < 0.7f)
		{
			if (rotatedAimDotMovement > 0.0f)
			{	
				animName = params.leftArmAimPose;
			}
			else
			{
				animName = params.rightArmAimPose;
			}
		}


		ISkeletonAnim* pISkeletonAnim = characterInstance.GetISkeletonAnim();
		IAnimationSet* pAnimSet = characterInstance.GetIAnimationSet();
		const int animId = pAnimSet->GetAnimIDByName(animName);
		pISkeletonAnim->StartAnimationById(animId, animParams);
	}
}