//-----------------------------------------------
// Pinger prototype
//
// 11-08-08: Benito G.R.
//----------------------------------------------

#include "StdAfx.h"
#include "Pinger.h"
#include <IMaterialEffects.h>

CPinger::CPinger():
m_pCurrentMechanic(NULL)
{

}

CPinger::~CPinger()
{
	MyMechanicRegistry::ref().DeleteRef();  //This will delete the registry once the last pinger is removed
}

//-------------------------------------------
void CPinger::PostInit(IGameObject * pGameObject )
{
	CActor::PostInit(pGameObject);

	MyMechanicRegistry::ref().AddRef(); //This will create the registry if needed and add a ref to it
	RequestDefaultMechanic();

}
//-------------------------------------------
void CPinger::PostPhysicalize()
{
	CActor::PostPhysicalize();

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	IPhysicalEntity *pPhysEnt = pCharacter?pCharacter->GetISkeletonPose()->GetCharacterPhysics(-1):NULL;

	if (pPhysEnt)
	{
		pe_params_pos pp;
		pp.iSimClass = 2;
		pPhysEnt->SetParams(&pp);

		pe_simulation_params ps;
		ps.mass = 0;
		pPhysEnt->SetParams(&ps);
	}

	if(SActorStats *pStats = GetActorStats())
		pStats->isRagDoll = false;

}

//--------------------------------------------
void CPinger::RagDollize(bool fallAndPlay )
{
	SActorStats *pStats = GetActorStats();

	if (pStats && (!pStats->isRagDoll || gEnv->pSystem->IsSerializingFile()))
	{
		SEntityPhysicalizeParams pp;

		pp.type = PE_ARTICULATED;
		pp.nSlot = 0;
		pp.mass = 500;

		pe_player_dimensions playerDim;
		pe_player_dynamics playerDyn;

		playerDyn.gravity.z = 15.0f;
		playerDyn.kInertia = 5.5f;

		pp.pPlayerDimensions = &playerDim;
		pp.pPlayerDynamics = &playerDyn;

		IPhysicalEntity *pPhysicalEntity = GetEntity()->GetPhysics();
		if (!pPhysicalEntity || pPhysicalEntity->GetType()!=PE_LIVING)
			pp.nLod = 1;

		GetEntity()->Physicalize(pp);

		pStats->isRagDoll = true;
	}
}

//------------------------------------------------
void CPinger::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_PREPHYSICSUPDATE)
	{
		PrePhysicsUpdate();
	}

	CActor::ProcessEvent(event);
}

//------------------------------------------------
void CPinger::PrePhysicsUpdate()
{
	IEntity* pEnt = GetEntity();
	if (pEnt->IsHidden())
		return;

	float frameTime = gEnv->pTimer->GetFrameTime();

	if (GetHealth()>0)
	{	
		UpdateStats(frameTime);

		//Update movement controller
		if (m_pMovementController)
		{
			SActorFrameMovementParams params;
			m_pMovementController->Update(frameTime, params);
		}

		//Update mechanical state
		if(m_pCurrentMechanic)
		{
			bool result = m_pCurrentMechanic->Update(this, &m_pingerState, &m_mechanicParams);

			//If mechanics tells that it's done, switch to default one
			if(result == false)
				RequestDefaultMechanic();
		}
	}
}

//-----------------------------------------
void CPinger::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	IEntity* pEnt = GetEntity();
	if (pEnt->IsHidden())
		return;

	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	CActor::Update(ctx,updateSlot);

	//if (GetHealth()>0)
	//{
		//animation processing
		//ProcessAnimation(pEnt->GetCharacter(0),ctx.fFrameTime);
	//}

}

//--------------------------------------------
void CPinger::UpdateStats(float frameTime)
{
	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();

	if (!pPhysEnt)
		return;

	//retrieve some information about the status of the player
	pe_status_dynamics dynStat;
	pe_status_living livStat;
	pe_player_dynamics simPar;

	if( !pPhysEnt->GetStatus(&dynStat) ||	!pPhysEnt->GetStatus(&livStat) ||		!pPhysEnt->GetParams(&simPar) )
		return;

	m_pingerState.velocity = dynStat.v;
	m_pingerState.speed = m_pingerState.velocity.len();
	m_pingerState.angVelocity = dynStat.w;

	m_modelInfo.baseMtx = Matrix33(GetEntity()->GetWorldRotation());
	m_modelInfo.viewMtx = m_modelInfo.baseMtx;

	Interpolate(m_modelInfo.weaponOffset,GetStanceInfo(m_stance)->weaponOffset,2.0f,frameTime);
	Interpolate(m_modelInfo.eyeOffset,GetStanceInfo(m_stance)->viewOffset,2.0f,frameTime);

}

//--------------------------------------------------------------
void CPinger::PlayAction(const char *action,const char *extension, bool looping/* =false */)
{
	const SMyAction actionData = MyMechanicRegistry::ref().GetActionByName(action);

	if(actionData.animationName.empty())
	{
		//GameWarning("CPinger::PlayAction() - Action %s not defined!", action);
		return;
	}

	if (ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0))
	{

		if (ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim())
		{
			CryCharAnimationParams params;
			params.m_fTransTime = actionData.blend;
			params.m_nLayerID = actionData.layer;
			params.m_nFlags = (actionData.looped?CA_LOOP_ANIMATION:0);
			if (params.m_nLayerID>0)
				params.m_nFlags|=CA_PARTIAL_SKELETON_UPDATE;
			pSkeletonAnim->StartAnimation(actionData.animationName, params);
			pSkeletonAnim->SetLayerUpdateMultiplier(actionData.layer, actionData.speed);

			//pCharacter->GetISkeleton()->SetDebugging( true );
		}
	}
}

//---------------------------------------------------------------
void CPinger::RequestMechanic(const char* mechanicName)
{
	if(!m_pCurrentMechanic || strcmp(m_pCurrentMechanic->GetName(),mechanicName))
	{
		IMyMechanic* pRequestedMechanic = MyMechanicRegistry::ref().GetMechanicByName(mechanicName);
		if(pRequestedMechanic)
		{
			if(m_pCurrentMechanic)
				m_pCurrentMechanic->OnLeave(this, &m_pingerState);

			m_pCurrentMechanic = pRequestedMechanic;
			m_pCurrentMechanic->OnEnter(this, &m_pingerState);

		}
		else
		{
			GameWarning("CPinger::RequestMechanic() - Requested mechanic %s is not registered", mechanicName);
		}
	}
}

//--------------------------------------------------------------
void CPinger::RequestDefaultMechanic()
{
	RequestMechanic("PingerMechanic_Locomotion");
}

//-----------------------------------------------------------
void CPinger::AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC)
{
	if(const char* eventName = event.m_EventName)
	{
		if(!strcmp(eventName, "footstep"))
		{
			const char * fxName = "Crysis2_alien_effects.Ping.footstep";
			IParticleEffect* pFootStepFX = gEnv->pParticleManager->FindEffect( fxName, "Game", "CPinger::AnimationEvent");
			if(!pFootStepFX)
				return;

			Vec3 offset(0.0f,0.0f,0.0f);
			if (event.m_BonePathName && event.m_BonePathName[0])
			{
				ISkeletonPose* pSkeletonPose = (pCharacter ? pCharacter->GetISkeletonPose() : 0);

				int id = (pSkeletonPose ? pSkeletonPose->GetJointIDByName(event.m_BonePathName) : -1);
				if (pSkeletonPose && id >= 0)
				{
					QuatT boneQuat(pSkeletonPose->GetAbsJointByID(id));
					offset = boneQuat.t;
				}
			}
			
			Matrix34 fxMtx; 
			fxMtx.SetIdentity(); fxMtx.SetTranslation(GetEntity()->GetWorldTM().TransformPoint(offset));
			pFootStepFX->Spawn( true, fxMtx );
		}
		else if (!strcmp(eventName, "RagdollizeNow"))
		{
			if(GetHealth() <= 0)
				RagDollize(false);
		}
	}
}
//----------------------------------------------
void CPinger::Kill()
{
	CActor::Kill();

	GetGameObject()->SetAutoDisablePhysicsMode(eADPM_Never);

	if(GetEntity()->GetAI())
		GetEntity()->GetAI()->Event(AIEVENT_DISABLE, 0);
}

//----------------------------------------
void CPinger::GetMemoryStatistics(ICrySizer * s)
{ 
	s->Add(*this);
}