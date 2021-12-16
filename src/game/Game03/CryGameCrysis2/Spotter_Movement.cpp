#include "StdAfx.h"
#include "Spotter.h"
#include "SpotterMovementController.h"


CSpotter::SMovementRequestParams::SMovementRequestParams(CMovementRequest& request) :
aimLook (false),
bodystate (0),
fDesiredSpeed (0.0f),
eActorTargetPhase (eATP_None),
bExactPositioning (false)    
{	
	aimLook = false;

	vMoveDir.zero();
	vLookTargetPos.zero();
	vAimTargetPos.zero();
	vShootTargetPos.zero();

	if (request.HasLookTarget())
	{
		vLookTargetPos = request.GetLookTarget();
	}
	if (request.HasAimTarget())
	{
		vAimTargetPos = request.GetAimTarget();
		aimLook = true;
	}
	if (request.HasFireTarget())
	{
		vShootTargetPos = request.GetFireTarget();
	}

	fDistanceToPathEnd = request.GetDistanceToPathEnd();

	if (request.HasDesiredSpeed())
		fDesiredSpeed = request.GetDesiredSpeed();

}


//-----------------------------------------
IActorMovementController* CSpotter::CreateMovementController()
{
	return new CSpotterMovementController(this);
}

//----------------------------------------
void CSpotter::SetActorMovement(SMovementRequestParams &control)
{
	m_movementRequest=control;
}

//-------------------------------------------
void CSpotter::GetActorInfo(SBodyInfo& bodyInfo)
{
	bodyInfo.vEyePos = GetEntity()->GetSlotWorldTM(0) * m_modelInfo.eyeOffset;
	bodyInfo.vFirePos = GetEntity()->GetSlotWorldTM(0) * m_modelInfo.weaponOffset;

	bodyInfo.vEyeDir = m_modelInfo.baseMtx.GetColumn(1);
	bodyInfo.vEyeDirAnim = bodyInfo.vEyeDir;

	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	if (pCharacter != NULL)
	{
		int headBoneID = GetBoneID(BONE_HEAD);
		if(headBoneID > -1)
		{
			Matrix33 headMtx( Matrix33(pCharacter->GetISkeletonPose()->GetAbsJointByID(headBoneID).q) );
			bodyInfo.vEyeDirAnim = Matrix33(GetEntity()->GetSlotWorldTM(0) * headMtx).GetColumn(1);
		}
	} 


	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(bodyInfo.vEyePos, ColorB(0,255,0,100), bodyInfo.vEyePos + bodyInfo.vEyeDir * 10.0f, ColorB(255,0,0,100));

	bodyInfo.vFwdDir = GetEntity()->GetRotation().GetColumn1();
	bodyInfo.vUpDir = m_modelInfo.baseMtx.GetColumn(2);
	bodyInfo.vFireDir = bodyInfo.vEyeDir; // TODO: Use real fire direction
	//bodyInfo.vFireDir = m_stats.fireDir;
	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(GetEntity()->GetWorldPos(), ColorB(0,255,0,100), GetEntity()->GetWorldPos() + bodyInfo.vFwdDir * 10, ColorB(255,0,0,100));

	// TODO: Read parameters from somewhere
	const SStanceInfo * pStanceInfo = GetStanceInfo(m_stance);
	bodyInfo.minSpeed = 0.0f; 
	//bodyInfo.minSpeed = min(m_params.speed_min, pStanceInfo->maxSpeed*0.01f);
	bodyInfo.normalSpeed = pStanceInfo->normalSpeed;
	bodyInfo.maxSpeed = pStanceInfo->maxSpeed;

	bodyInfo.stance = m_stance;
	bodyInfo.m_stanceSizeAABB = pStanceInfo->GetStanceBounds();
	bodyInfo.m_colliderSizeAABB = pStanceInfo->GetColliderBounds();
}

//----------------------------------------------------------
int CSpotter::GetBoneID(int ID,int slot) const
{
	if (m_boneIDs[ID]<0)
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
		if (!pCharacter)
			return -1;

		char boneStr[64];
		switch(ID)
		{
		case BONE_HEAD:		strcpy(boneStr,"weapon_bone1");break;
		case BONE_WEAPON: strcpy(boneStr,"weapon_bone1");break;
		case BONE_WEAPON2:strcpy(boneStr,"weapon_bone2");break;
		}

		m_boneIDs[ID] = pCharacter->GetISkeletonPose()->GetJointIDByName(boneStr);
	}

	return CActor::GetBoneID(ID,slot);
}



