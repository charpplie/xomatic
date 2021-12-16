#include "StdAfx.h"
#include "Harvester.h"
#include <IDebugHistory.h>
#include "Game.h"




const char *CHarvester::m_animations[CHarvester::ANIM__MAX]=
{
	"harvester_movement",//ANIM_MOVEMENT,

	"harvester_idle_empty_01",
	"harvester_walk_01",
	"harvester_run_01",
	"harvester_jump_01",
	"harvester_fear_01",
	"harvester_suck_01",

	"harvester_full_movement",//ANIM_MOVEMENT_FULL,

	"harvester_idle_full_01",
	"harvester_walk_full_01",
	"harvester_run_full_01",
	"harvester_jump_full_01",
	"harvester_fear_full_01"

	//ANIM__MAX
};




CHarvester::CHarvester():m_pDebugHistoryManager(0)
{
	m_bIgnoreMovementRequests = false;
	m_bAnimationEnabled = true;
	m_currentAnim = NULL;
	m_lastAnim = NULL;
	m_target = NULL;
	m_Full = NULL;
	
}

CHarvester::~CHarvester()
{
	//release stuff..
	if (m_pDebugHistoryManager)
		m_pDebugHistoryManager->Release();
}

//----------------------------------------
void CHarvester::GetMemoryUsage(ICrySizer * pSizer) const
{ 
	pSizer->Add(*this);
	CActor::GetInternalMemoryUsage(pSizer); // collect memory of parent class
}

//------------------------------------------------
void CHarvester::ProcessEvent(SEntityEvent& theEvent)
{
	if (theEvent.event == ENTITY_EVENT_PREPHYSICSUPDATE)
	{
		PrePhysicsUpdate();
	}

	CActor::ProcessEvent(theEvent);
}

//-----------------------------------------
void CHarvester::Update(SEntityUpdateContext& theEntityUpdateContextRef, int theUpdateSlot)
{
	IEntity* anEntityPtr = GetEntity();
	if (!anEntityPtr->IsHidden())
	{
		FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);
		CActor::Update(theEntityUpdateContextRef, theUpdateSlot);
	}
}

void CHarvester::DebugValue(const char* theValueName, float theValue)
{
	if (m_pDebugHistoryManager != NULL && theValueName != NULL)
	{
		//Const issue note Copied from elsewhere when accessthing this ==>
		// NOTE: It's alright to violate the const here. The player is a good common owner for debug graphs, 
		// but it's also not non-const in all places, even though graphs might want to be added from those places.
		IDebugHistory* aDebugHistoryPtr = const_cast<IDebugHistoryManager*>(m_pDebugHistoryManager)->GetHistory(theValueName);
		if (aDebugHistoryPtr != NULL)
		{
			aDebugHistoryPtr->AddValue(theValue);
		}
	}
}

//------------------------------------------------
void CHarvester::PrePhysicsUpdate()
{
	IEntity* anEntityPtr = GetEntity();
	if (!anEntityPtr->IsHidden() && GetHealth()>0 && !GetActorStats()->isRagDoll)
	{
		//get and hold fra,e time and update stats..
		float aFrameTime = gEnv->pTimer->GetFrameTime();
		UpdateStats(aFrameTime);

		//Update more if there is a movement controller
		if (m_pMovementController)
		{
			//constant animation speed
			const float anAnimSpeed=1.0f;

			//default anim params
			CryCharAnimationParams anAnimParams;
			anAnimParams.m_fTransTime = 0.5f;
			anAnimParams.m_nLayerID = 0;
			anAnimParams.m_nFlags = CA_LOOP_ANIMATION;

			//get the skeleton we are animating
			ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
			if(!pCharacter)
				return;
			ISkeletonAnim *aSkeletonAnimPtr=pCharacter->GetISkeletonAnim();
			//TODO assert(aSkeletonAnimPtr)

			//update the movement controller and get the result
			SActorFrameMovementParams anActorMovementParams;
			SMovementState aMovementState;
			m_pMovementController->Update(aFrameTime, anActorMovementParams);
			m_pMovementController->GetMovementState(aMovementState);

			//choose the default animation
			ESpotterAnimation aAnimIndex = ANIM_IDLE;
			m_lastAnim = m_currentAnim;

			//declare physics request information
			Vec3 aMovementDirection;
			pe_action_move aPhysicsMoveRequest;
			aPhysicsMoveRequest.iJump=2;

			//get the current rotation
			const Matrix33 aCurWorldMtx = Matrix33(GetEntity()->GetWorldRotation());
			Quat aCurrentRotation(aCurWorldMtx);

			//if we are swarming a target we need to move and play animations for that, otherwise move normally
			if(m_target)
			{
				QuatT anAnimMovement = aSkeletonAnimPtr->GetRelMovement();
				IEntity* aTarget = gEnv->pEntitySystem->GetEntity(m_target);
				
				//TEMP HACK
				aMovementDirection = (aTarget->GetPos() - anEntityPtr->GetPos()).GetNormalizedSafe();
				//aMovementDirection = (aCurWorldMtx * FORWARD_DIRECTION).GetNormalizedSafe();
				aMovementDirection.z = 0.0f;
				aMovementDirection = aMovementDirection.GetNormalizedSafe();

				Matrix33 aRotationMtx;	aRotationMtx.SetRotationVDir(aMovementDirection);

				aPhysicsMoveRequest.dir = aRotationMtx * anAnimMovement.t / aFrameTime;
				aAnimIndex = ANIM_JUMP;//ANIM_IDLE_AFRAID;//or ANIM_JUMP
				//TODO linking anims stuff..
				//ap.m_nFlags = CA_LOOP_ANIMATION; can I make this not loop?
				
			}
			else
			{
				//looks like we are moving normally.
				if(!m_HarvesterMovementRequestParams.vMoveDir.IsZero())
				{
					//we have a move request so use the requested direction
					aMovementDirection = m_HarvesterMovementRequestParams.vMoveDir.GetNormalizedSafe();
				}
				else
				{
					//we have no real move request, use the existing look direction
					aMovementDirection = (aCurWorldMtx * FORWARD_DIRECTION).GetNormalizedSafe();
				}

				//set the physics move request to the requested speed and direction
				aPhysicsMoveRequest.dir = m_HarvesterMovementRequestParams.fDesiredSpeed * aMovementDirection;

				if (m_HarvesterMovementRequestParams.fDesiredSpeed>0.001f && aMovementDirection.len2()>0.0f)
				{
					//we are actually moving we should be playing a movement animation
					aAnimIndex = ANIM_MOVEMENT;
				}
			}

			//calculate the actual rotation from the current
			Matrix33 aTargetOrientationMtx;		aTargetOrientationMtx.SetRotationVDir(aMovementDirection);
			Quat aTargetRotation(aTargetOrientationMtx);
			//TODO figure why the code I originally copied interps like this.. not intuitive..
			aTargetRotation = Quat::CreateSlerp( aCurrentRotation.GetNormalized(), aTargetRotation, min(aFrameTime*10.0f, 1.0f));


			if(m_Full)
			{
				//our harvester is in the full state, so we better offset all the animations to reflect this
				*((int*)&aAnimIndex) += ((int)ANIM_FULL_BEGIN);
			}

			//the actual chosen animation is:
			m_currentAnim = m_animations[aAnimIndex];

			if(!m_bAnimationEnabled)
			{
				//stop anims, we are likely being carried by the player or something like that where we should not move.
				aSkeletonAnimPtr->StopAnimationsAllLayers();
			}
			else if (aSkeletonAnimPtr && m_currentAnim!=m_lastAnim)
			{
				//if the animation changed sense last frame we need to actually set it, otherwise we could just leave it
				aSkeletonAnimPtr->StartAnimation(m_currentAnim, anAnimParams);
				aSkeletonAnimPtr->SetLayerUpdateMultiplier(0, anAnimSpeed);
			}


			//TODO this should be conditional??? Here??
			if(!m_target)
			{
				aSkeletonAnimPtr->SetAnimationDrivenMotion(0);
				aSkeletonAnimPtr->SetDesiredMotionParam(eMotionParamID_TravelDist, m_HarvesterMovementRequestParams.fDesiredSpeed, aFrameTime);
				aSkeletonAnimPtr->SetDesiredMotionParam(eMotionParamID_TurnAngle, aTargetRotation.GetRotZ() - aCurrentRotation.GetRotZ(), aFrameTime);
				//aSkeletonAnimPtr->SetDesiredMotionParam(eMotionParamID_TravelSpeed, m_HarvesterMovementRequestParams.fDesiredSpeed, aFrameTime);
			}
			else
			{
				aSkeletonAnimPtr->SetAnimationDrivenMotion(1);
			}
			
			//TODO find out if ignore check could be at higher level to avoid calculations!!!!!!!!!!!!!!
			if(!m_bIgnoreMovementRequests)
			{
				GetEntity()->GetPhysics()->Action(&aPhysicsMoveRequest);
				GetEntity()->SetRotation(aTargetRotation, ENTITY_XFORM_USER|ENTITY_XFORM_NOT_REREGISTER);
			}


			DebugValue("EntID", (float)GetEntityId());
			DebugValue("moveDirX", aMovementDirection.x);
			DebugValue("moveDirY", aMovementDirection.y);
			DebugValue("moveDirZ", aMovementDirection.z);
		}//end if valid movement controller

	}//end if !hidden, alive, and not rag dolling
}

//-------------------------------------------
void CHarvester::PostPhysicalize()
{
	CActor::PostPhysicalize();

	ICharacterInstance *aCharacterPtr = GetEntity()->GetCharacter(0);
	IPhysicalEntity *aPhysicalEnityPtr = aCharacterPtr?aCharacterPtr->GetISkeletonPose()->GetCharacterPhysics(-1):NULL;

	if (aPhysicalEnityPtr)
	{
		pe_simulation_params aPhysicsSimParams;
		aPhysicsSimParams.iSimClass = 2;	//this is normal rigid body
		aPhysicalEnityPtr->SetParams(&aPhysicsSimParams);
	}

	if(SActorStats *anActorStatsPtr = GetActorStats())
	{
		anActorStatsPtr->isRagDoll = false;	//TODO remove??
	}
}

//-------------------------------------------
void CHarvester::PostInit(IGameObject * theGameObjectPtr)
{
	CActor::PostInit(theGameObjectPtr);

	if (false)
	{
		m_pDebugHistoryManager=g_pGame->GetIGameFramework()->CreateDebugHistoryManager();

		m_pDebugHistoryManager->LayoutHelper("EntID", NULL, true, -1000000, 1000000, 1000000, -1000000, 0.0f, 1.0f);

		m_pDebugHistoryManager->LayoutHelper("moveDirX", NULL, true, -20, 20, -5, 5, 1.0f, 0.0f);
		m_pDebugHistoryManager->LayoutHelper("moveDirY", NULL, true, -20, 20, -5, 5, 2.0f, 0.0f);
		m_pDebugHistoryManager->LayoutHelper("moveDirZ", NULL, true, -20, 20, -5, 5, 3.0f, 0.0f);
	}
}

//--------------------------------------------
void CHarvester::RagDollize(bool theFallAndPlay )
{
	SActorStats *anActorStatsPtr = GetActorStats();

	if (anActorStatsPtr && (!anActorStatsPtr->isRagDoll || gEnv->pSystem->IsSerializingFile()))
	{
		SEntityPhysicalizeParams pp;

		pp.type = PE_ARTICULATED;
		pp.nSlot = 0;
		pp.mass = 500;//20;	//TODO how to link this to script, change dynamically? Maybe this is just called at death? then it can be if full or not

		pe_player_dimensions playerDim;
		pe_player_dynamics playerDyn;

		playerDyn.gravity.z = 15.0f;	//TODO is this ok?
		playerDyn.kInertia = 5.5f;

		pp.pPlayerDimensions = &playerDim;
		pp.pPlayerDynamics = &playerDyn;

		IPhysicalEntity *pPhysicalEntity = GetEntity()->GetPhysics();
		if (!pPhysicalEntity || pPhysicalEntity->GetType()!=PE_LIVING)
			pp.nLod = 1;

		GetEntity()->Physicalize(pp);

		anActorStatsPtr->isRagDoll = true;
	}
}

//----------------------------------------------
void CHarvester::Kill()
{
	CActor::Kill();

	GetGameObject()->SetAutoDisablePhysicsMode(eADPM_Never);

	if(GetEntity()->GetAI())
		GetEntity()->GetAI()->Event(AIEVENT_DISABLE, 0);
}



//AI specific
void CHarvester::SetActorMovement(SHarvesterMovementRequestParams &control)
{
	m_HarvesterMovementRequestParams = control;
}

void CHarvester::GetActorInfo( SHarvesterBodyInfo& bodyInfo )
{
	//TODO??

	bodyInfo.vEyePos = GetEntity()->GetSlotWorldTM(0) * m_HarvesterModelInfo.eyeOffset;
	bodyInfo.vFirePos = GetEntity()->GetSlotWorldTM(0) * Vec3(0.0f,0.0f,0.0f);//m_modelInfo.weaponOffset; TODO put back with the weapon?

	bodyInfo.vEyeDir = m_HarvesterModelInfo.baseMtx.GetColumn(1);
	bodyInfo.vEyeDirAnim = bodyInfo.vEyeDir;

	ICharacterInstance* aCharacterPtr = GetEntity()->GetCharacter(0);
	if (aCharacterPtr != NULL)
	{
		int headBoneID = GetBoneID(BONE_HEAD);
		if(headBoneID > -1)
		{
			Matrix33 headMtx( Matrix33(aCharacterPtr->GetISkeletonPose()->GetAbsJointByID(headBoneID).q) );
			bodyInfo.vEyeDirAnim = Matrix33(GetEntity()->GetSlotWorldTM(0) * headMtx).GetColumn(1);
		}
	} 

	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(bodyInfo.vEyePos, ColorB(0,255,0,100), bodyInfo.vEyePos + bodyInfo.vEyeDir * 10.0f, ColorB(255,0,0,100));

	bodyInfo.vFwdDir = GetEntity()->GetRotation().GetColumn1();
	bodyInfo.vUpDir = m_HarvesterModelInfo.baseMtx.GetColumn(2);
	bodyInfo.vFireDir = bodyInfo.vEyeDir; // TODO: Use real fire direction
	//bodyInfo.vFireDir = m_stats.fireDir;
	//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(GetEntity()->GetWorldPos(), ColorB(0,255,0,100), GetEntity()->GetWorldPos() + bodyInfo.vFwdDir * 10, ColorB(255,0,0,100));

	const SStanceInfo * pStanceInfo = GetStanceInfo(m_stance);
	bodyInfo.minSpeed = 0.0f;
	bodyInfo.normalSpeed = pStanceInfo->normalSpeed;
	bodyInfo.maxSpeed = pStanceInfo->maxSpeed;
	bodyInfo.stance = m_stance;
	bodyInfo.m_stanceSizeAABB = pStanceInfo->GetStanceBounds();
	bodyInfo.m_colliderSizeAABB = pStanceInfo->GetColliderBounds();
}

//int	CHarvester::GetBoneID(int ID,int slot) const
//{
//	int aBoneID = -1;
//	ICharacterInstance* aCharacterPtr = GetEntity()->GetCharacter(slot);
//
//	if (aCharacterPtr != NULL)
//	{
//		//TODO
//		//aCharacterPtr
//		//aBoneID = GetBoneID(ID);
//	}
//	return aBoneID;
//
//	/*REFERENCE:
//	if (m_boneIDs[ID]<0)
//	{
//		ICharacterInstance *aCharacterPtr = GetEntity()->GetCharacter(slot);
//		if (!aCharacterPtr)
//			return -1;
//
//		char boneStr[64];
//		switch(ID)
//		{
//		case BONE_HEAD:		strcpy(boneStr,"weapon_bone1");break;
//		case BONE_WEAPON: strcpy(boneStr,"weapon_bone1");break;
//		case BONE_WEAPON2:strcpy(boneStr,"weapon_bone2");break;
//		}
//
//		m_boneIDs[ID] = aCharacterPtr->GetISkeletonPose()->GetJointIDByName(boneStr);
//	}
//
//	return CActor::GetBoneID(ID,slot);
//	*/
//}

bool CHarvester::CreateCodeEvent(SmartScriptTable &rTable)
{
	const char *anEventName = NULL;
	if (!rTable->GetValue("event",anEventName))
		return false;

	if(!strcmp(anEventName,"SwarmTarget"))
	{
		ScriptHandle aValue;
		if(rTable->GetValue("Target", aValue))
			m_target = (EntityId)aValue.n;
	//	else
	//		m_target = NULL;

		return true;
	}
	else if(!strcmp(anEventName,"Landed"))
	{
		m_bIgnoreMovementRequests = false;
	}
	if(!strcmp(anEventName,"Pickedup"))
	{
		m_bIgnoreMovementRequests = true;
		m_bAnimationEnabled = false;
	}
	if(!strcmp(anEventName,"FullStatus"))
	{
		bool aValue;
		if(rTable->GetValue("IsFull",aValue))
		{
			m_Full = aValue;
		}
	}

	return CActor::CreateCodeEvent(rTable);
}

//Movement controller
IActorMovementController * CHarvester::CreateMovementController()
{
	return new CHarvesterMovementController(this);
}

//Misc
void CHarvester::UpdateStats(float frameTime)
{
	IPhysicalEntity *aPhysicalEnityPtr = GetEntity()->GetPhysics();

	if (!aPhysicalEnityPtr)
		return;

	//retrieve some information about the status of the player
	pe_status_dynamics dynStat;
	pe_status_living livStat;
	pe_player_dynamics simPar;

	if(	!aPhysicalEnityPtr->GetStatus(&dynStat)	||	!aPhysicalEnityPtr->GetStatus(&livStat)	||	!aPhysicalEnityPtr->GetParams(&simPar)	)
		return;

	m_HarvesterModelInfo.baseMtx = Matrix33(GetEntity()->GetWorldRotation());
	m_HarvesterModelInfo.viewMtx = m_HarvesterModelInfo.baseMtx;

	Interpolate(m_HarvesterModelInfo.weaponOffset,GetStanceInfo(m_stance)->weaponOffset,2.0f,frameTime);
	Interpolate(m_HarvesterModelInfo.eyeOffset,GetStanceInfo(m_stance)->viewOffset,2.0f,frameTime);

	//TODO find out if this needs anything else.. Also what is this really for?
}

//Actions
void CHarvester::PlayAction(const char *action,const char *extension, bool looping)
{
	//TODO
	/*
	if(actionData.animationName.empty())
	{
	//GameWarning("CHarvester::PlayAction() - Action %s not defined!", action);
	return;
	}

	if (ICharacterInstance *aCharacterPtr = GetEntity()->GetCharacter(0))
	{

	if (ISkeletonAnim* aSkeletonAnimPtr = aCharacterPtr->GetISkeletonAnim())
	{
	int32 layer = 0;
	CryCharAnimationParams params;
	params.m_fTransTime = actionData.blend;
	params.m_nLayerID = layer;
	params.m_nFlags = (actionData.looped?CA_LOOP_ANIMATION:0);
	aSkeletonAnimPtr->StartAnimation(actionData.animationName, params);
	aSkeletonAnimPtr->SetLayerUpdateMultiplier(layer, actionData.speed);

	//aCharacterPtr->GetISkeleton()->SetDebugging( true );
	}
	}*/
}

void CHarvester::AnimationEvent(ICharacterInstance *aCharacterPtr, const AnimEventInstance &event, const uint32 eventNameCRC)
{
	//TODO
	if(const char* eventName = event.m_EventName)
	{
		if (!strcmp(eventName, "RagdollizeNow"))
		{
			if(GetHealth() <= 0)
				RagDollize(false);
		}
	}
}


CHarvester::SHarvesterMovementRequestParams::SHarvesterMovementRequestParams(CMovementRequest& request) :
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





CHarvesterMovementController::CHarvesterMovementController(CHarvester* pHarvester)
{
	m_pHarvester = pHarvester;
}

CHarvesterMovementController::~CHarvesterMovementController()
{

}

//--------------------------------------------------
void CHarvesterMovementController::Release()
{
	delete this;
}

//-------------------------------------------------
bool CHarvesterMovementController::RequestMovement(CMovementRequest& request )
{
	SMovementState state;
	GetMovementState(state);

	const Vec3 currentPos = state.pos;

	CHarvester::SHarvesterMovementRequestParams os (request);

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

	m_pHarvester->SetActorMovement(os);

	return true;
}

//-----------------------------------------
bool CHarvesterMovementController::Update(float frameTime, SActorFrameMovementParams& params )
{
	SMovementState& state(m_currentMovementState);
	CHarvester::SHarvesterBodyInfo bodyInfo;
	m_pHarvester->GetActorInfo( bodyInfo );
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
	state.bodyDirection = m_pHarvester->GetEntity()->GetWorldRotation() * Vec3(0,1,0);

	if(m_currentMovementRequest.HasAimTarget())
		state.aimDirection = (m_currentMovementRequest.GetAimTarget()-state.weaponPosition).GetNormalizedSafe();
	else
		state.aimDirection = bodyInfo.vFireDir.GetNormalizedSafe();

	if(m_currentMovementRequest.HasFireTarget())
		state.fireDirection = (m_currentMovementRequest.GetFireTarget()-state.weaponPosition).GetNormalizedSafe(state.aimDirection);
	else
		state.fireDirection = state.aimDirection;

	state.isAlive = (m_pHarvester->GetHealth()>0);

	state.isAiming = true;

	state.pos = m_pHarvester->GetEntity()->GetWorldPos();

	//state.isFiring = (m_pHarvester->GetActorStats()->inFiring>0.001f);

	if(m_currentMovementRequest.HasFireTarget())
		state.fireTarget = m_currentMovementRequest.GetFireTarget();

	return false;
}

//------------------------------------------
bool CHarvesterMovementController::GetStanceState(const SStanceStateQuery& query, SStanceState& state )
{
	const SStanceInfo*	pStance = m_pHarvester->GetStanceInfo(query.stance);
	if(!pStance)
		return false;

	state.lean = query.lean;
	state.peekOver = query.peekOver;


	if(query.defaultPose)
	{
		state.pos.Set(0,0,0);
		state.bodyDirection = FORWARD_DIRECTION;
		state.upDirection(0,0,1);
		state.weaponPosition = m_pHarvester->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pHarvester->GetEyeOffset());
		state.aimDirection = FORWARD_DIRECTION;
		state.fireDirection = FORWARD_DIRECTION;
		state.eyePosition = pStance->GetViewOffsetWithLean(query.lean, query.peekOver);
		state.eyeDirection = FORWARD_DIRECTION;
		state.m_StanceSize = pStance->GetStanceBounds();
		state.m_ColliderSize = pStance->GetColliderBounds();
	}
	else
	{
		CHarvester::SHarvesterBodyInfo bodyInfo;
		m_pHarvester->GetActorInfo( bodyInfo );

		Matrix34	tm = m_pHarvester->GetEntity()->GetWorldTM();

		state.pos = m_pHarvester->GetEntity()->GetWorldPos();
		state.bodyDirection = bodyInfo.vFwdDir;
		state.upDirection = bodyInfo.vUpDir;
		state.weaponPosition = tm.TransformPoint(m_pHarvester->GetWeaponOffsetWithLean(query.stance, query.lean, query.peekOver, m_pHarvester->GetEyeOffset()));
		state.aimDirection = bodyInfo.vFireDir;
		state.fireDirection = bodyInfo.vFireDir;
		state.eyePosition = tm.TransformPoint(pStance->GetViewOffsetWithLean(query.lean, query.peekOver));
		state.eyeDirection = bodyInfo.vEyeDir;
		state.m_StanceSize = pStance->GetStanceBounds();
		state.m_ColliderSize = pStance->GetColliderBounds();
	}

	return true;
}

void CHarvesterMovementController::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_pHarvester);
}