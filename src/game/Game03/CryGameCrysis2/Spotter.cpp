#include "StdAfx.h"
#include "Spotter.h"
#include <IMaterialEffects.h>
#include <IDebugHistory.h>
#include "Game.h"

CSpotter::CSpotter()
: m_sensorAwareness(0.0f),
	m_sensorPos(ZERO),
	m_pDebugHistoryManager(0),
	m_scanTimer(0.0f),
	m_scanTick(0.0f),
	m_scanRange(0.0f),
	m_scanFov(0.0f),
	m_alerted(false),
	m_alertedStartTimer(0.0f)
{

}

const char *CSpotter::m_animations[CSpotter::eSA_Last]={
	"fly_idle",
	"fly_loop_spotted",
	"fly_loop",
	"fly_loop_spotted",
	"fly_up",
	"fly_down",
	"fly_left",
	"fly_right",
	"fly_attract",
};

CSpotter::~CSpotter()
{
	if (m_pDebugHistoryManager)
		m_pDebugHistoryManager->Release();
}

void CSpotter::DebugValue(const char *id, float value)
{
	if (m_pDebugHistoryManager == NULL)
		return;

	if (id == NULL)
		return;

	// NOTE: It's alright to violate the const here. The player is a good common owner for debug graphs, 
	// but it's also not non-const in all places, even though graphs might want to be added from those places.
	IDebugHistory* pDH = const_cast<IDebugHistoryManager*>(m_pDebugHistoryManager)->GetHistory(id);
	if (pDH != NULL)
		pDH->AddValue(value);
}

//-------------------------------------------
void CSpotter::PostInit(IGameObject * pGameObject )
{
	CActor::PostInit(pGameObject);

	if (false)
	{
		m_pDebugHistoryManager=g_pGame->GetIGameFramework()->CreateDebugHistoryManager();

		m_pDebugHistoryManager->LayoutHelper("EntID", NULL, true, -1000000, 1000000, 1000000, -1000000, 0.0f, 1.0f);
		
		m_pDebugHistoryManager->LayoutHelper("moveDirX", NULL, true, -20, 20, -5, 5, 1.0f, 0.0f);
		m_pDebugHistoryManager->LayoutHelper("moveDirY", NULL, true, -20, 20, -5, 5, 2.0f, 0.0f);
		m_pDebugHistoryManager->LayoutHelper("moveDirZ", NULL, true, -20, 20, -5, 5, 3.0f, 0.0f);
	}
}

//-------------------------------------------
void CSpotter::PostPhysicalize()
{
	CActor::PostPhysicalize();

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	IPhysicalEntity *pPhysEnt = pCharacter?pCharacter->GetISkeletonPose()->GetCharacterPhysics(-1):NULL;

	if (pPhysEnt)
	{
		pe_simulation_params ps;
		ps.gravity.zero();

		pPhysEnt->SetParams(&ps);
	}

	if(SActorStats *pStats = GetActorStats())
		pStats->isRagDoll = false;
}

//--------------------------------------------
void CSpotter::RagDollize(bool fallAndPlay )
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
void CSpotter::ProcessEvent(SEntityEvent& event)
{
	if (event.event == ENTITY_EVENT_PREPHYSICSUPDATE)
	{
		PrePhysicsUpdate();
	}

	CActor::ProcessEvent(event);
}

//------------------------------------------------
void CSpotter::PrePhysicsUpdate()
{
	IEntity* pEnt = GetEntity();
	if (pEnt->IsHidden())
		return;

	const char **animations=m_animations;

	float frameTime = gEnv->pTimer->GetFrameTime();

	if (GetHealth()>0 && !GetActorStats()->isRagDoll)
	{	
		UpdateStats(frameTime);

		//Update movement controller
		if (m_pMovementController && !GetActorStats()->isRagDoll)
		{
			SActorFrameMovementParams params;
			m_pMovementController->Update(frameTime, params);

			SMovementState state;
			m_pMovementController->GetMovementState(state);

			const Matrix33 entityMtx = Matrix33(GetEntity()->GetWorldRotation());

			Vec3 moveDir(ZERO);
			if(!m_movementRequest.vMoveDir.IsZero())
				moveDir=m_movementRequest.vMoveDir.GetNormalizedSafe();

			// movement
			if (!m_lastMoveDir.IsZero())
			{
				moveDir.z=LERP(m_lastMoveDir.z, moveDir.z, 0.5f);
				moveDir.NormalizeSafe();
			}

			Vec3 velocity=m_movementRequest.fDesiredSpeed*moveDir;

			pe_action_move m;
			m.dir=velocity;
			m.iJump=3;

			if (IPhysicalEntity *pPE=GetEntity()->GetPhysics())
				pPE->Action(&m);

			// orientation
			Vec3 lookDir(ZERO);
			if (m_movementRequest.fDesiredSpeed>0.001f)	
				lookDir=moveDir;
			else if (!m_movementRequest.vLookTargetPos.IsZero())
			{
				lookDir=m_movementRequest.vLookTargetPos-state.eyePosition;
				lookDir.NormalizeSafe();
			}

			if (!lookDir.IsZero())
			{
				Matrix33	orientationTarget;
				orientationTarget.SetRotationVDir(lookDir);

				Quat	targetQuat(orientationTarget);
				Quat	currQuat(entityMtx);
				Quat  targetRot = Quat::CreateSlerp( currQuat.GetNormalized(), targetQuat, min(frameTime*7.5f, 1.0f));

				GetEntity()->SetRotation(targetRot, ENTITY_XFORM_USER|ENTITY_XFORM_NOT_REREGISTER);
			}

			if (m_alertedStartTimer<=0.0f)
			{
				// anims
				const float animSpeed=1.75f;

				CryCharAnimationParams ap;
				ap.m_fTransTime = 0.125f;
				ap.m_nLayerID = 0;
				ap.m_nFlags = CA_LOOP_ANIMATION;

				// TODO: this method of chosing animation is ugly, will need to change to something better later
				const char *anim0=animations[m_alerted?eSA_AlertedIdle:eSA_Idle];
				const char *anim1=0;

				if (m_scanTimer>0.0f)
					anim0=animations[m_alerted?eSA_AlertedFly:eSA_Fly];

				if (m_movementRequest.fDesiredSpeed>0.001f && moveDir.len2()>0.0f)
				{
					anim0=animations[m_alerted?eSA_AlertedFly:eSA_Fly];
					/*
					if (moveDir.z>0.125f)
					anim1=animations[eSA_Fly_Up];
					else if (moveDir.z<-0.15f)
					anim1=animations[eSA_Fly_Down];
					*/
				}

				ISkeletonAnim *pSkeletonAnim=0;
				if (ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0))
					pSkeletonAnim=pCharacter->GetISkeletonAnim();

				if (pSkeletonAnim)
				{
					pSkeletonAnim->StartAnimation(anim0, ap);
					pSkeletonAnim->SetLayerUpdateMultiplier(0, animSpeed);
				}
			}


			DebugValue("EntID", (float)GetEntityId());
			DebugValue("moveDirX", moveDir.x);
			DebugValue("moveDirY", moveDir.y);
			DebugValue("moveDirZ", moveDir.z);

			m_lastMoveDir=moveDir;
		}
	}
}

//-----------------------------------------
void CSpotter::Update(SEntityUpdateContext& ctx, int updateSlot)
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

	if (updateSlot!=0)
		return;

	if (m_scanTimer>0.0f)
	{
		m_scanTimer-=ctx.fFrameTime;
		m_scanTick-=ctx.fFrameTime;
		if (m_scanTick<=0.0f)
		{
			m_scanTick+=0.1f;

			Scan(m_scanRange, m_scanFov);
		}
	}

	if (m_alertedStartTimer>0.0f)
		m_alertedStartTimer-=ctx.fFrameTime;

	DrawSensor();
}

//--------------------------------------------
void CSpotter::UpdateStats(float frameTime)
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

	m_modelInfo.baseMtx = Matrix33(GetEntity()->GetWorldRotation());
	m_modelInfo.viewMtx = m_modelInfo.baseMtx;

	Interpolate(m_modelInfo.weaponOffset,GetStanceInfo(m_stance)->weaponOffset,2.0f,frameTime);
	Interpolate(m_modelInfo.eyeOffset,GetStanceInfo(m_stance)->viewOffset,2.0f,frameTime);
}

//--------------------------------------------------------------
void CSpotter::PlayAction(const char *action,const char *extension, bool looping/* =false */)
{
/*	if(actionData.animationName.empty())
	{
		//GameWarning("CSpotter::PlayAction() - Action %s not defined!", action);
		return;
	}

	if (ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0))
	{

		if (ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim())
		{
			int32 layer = 0;
			CryCharAnimationParams params;
			params.m_fTransTime = actionData.blend;
			params.m_nLayerID = layer;
			params.m_nFlags = (actionData.looped?CA_LOOP_ANIMATION:0);
			pSkeletonAnim->StartAnimation(actionData.animationName, params);
			pSkeletonAnim->SetLayerUpdateMultiplier(layer, actionData.speed);

			//pCharacter->GetISkeleton()->SetDebugging( true );
		}
	}
	*/
}


//-----------------------------------------------------------
void CSpotter::AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC)
{
	if(const char* eventName = event.m_EventName)
	{
		if (!strcmp(eventName, "RagdollizeNow"))
		{
			if(GetHealth() <= 0)
				RagDollize(false);
		}
	}
}
//----------------------------------------------
void CSpotter::Kill()
{
	CActor::Kill();

	GetGameObject()->SetAutoDisablePhysicsMode(eADPM_Never);

	if(GetEntity()->GetAI())
		GetEntity()->GetAI()->Event(AIEVENT_DISABLE, 0);
}

//----------------------------------------
void CSpotter::GetMemoryUsage(ICrySizer * s) const
{ 
	s->Add(*this);
	CActor::GetInternalMemoryUsage(s); // collect memory of parent class
}

//------------------------------------------------------------------------
bool CSpotter::CreateCodeEvent(SmartScriptTable &rTable)
{
	const char *event = NULL;
	if (!rTable->GetValue("event",event))
		return false;

	if (!strcmp(event,"alerted"))
	{
		bool value=false;
		char *material=0;
		rTable->GetValue("value", value);
		rTable->GetValue("material", material);

		m_alerted=value;

		IMaterial *pMaterial=0;

		if (material && material[0])
		{
			pMaterial = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(material);
			if (!pMaterial)
				pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(material);
		}

		for (int i=0; i<GetEntity()->GetSlotCount(); i++)
		{
			SEntitySlotInfo slotInfo;
			if (GetEntity()->GetSlotInfo(i, slotInfo))
			{
				if (slotInfo.pCharacter)
				{
					SetMaterialRecursive(slotInfo.pCharacter, pMaterial==0, pMaterial);
				}
			}
		}

		m_alertedStartTimer=0.0f;

		if (m_alerted)
		{
			ISkeletonAnim *pSkeletonAnim=0;
			if (ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0))
			{
				int animationId = pCharacter->GetIAnimationSet()->GetAnimIDByName(m_animations[eSA_AlertedStart]);
				if (animationId>=0)
					m_alertedStartTimer = pCharacter->GetIAnimationSet()->GetDuration_sec(animationId);

				pSkeletonAnim=pCharacter->GetISkeletonAnim();
			}

			if (pSkeletonAnim)
			{
				CryCharAnimationParams ap;
				ap.m_fTransTime = 0.01f;
				ap.m_nLayerID = 0;
				ap.m_nFlags = 0;

				pSkeletonAnim->StartAnimation(m_animations[eSA_AlertedStart], ap);
				pSkeletonAnim->SetLayerUpdateMultiplier(0, 1);
			}
		}

		return true;
	}
	else if (!strcmp(event,"interference"))
	{
		ScriptHandle actorId;
		rTable->GetValue("actorId", actorId);
		CActor *pActor=static_cast<CActor *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor((EntityId)actorId.n));

		if (pActor && pActor->HasNanoSuit())
		{

			SNanoSuitEvent event;
			event.event = eNanoSuitEvent_ACTION;
			event.sParam = "Suit_EnergyDrain";

			pActor->SendActorSuitEvent(event);
		}
	}
	else if (!strcmp(event,"emp"))
	{
		float radius=10.0f;
		float duration=2.0f;

		rTable->GetValue("duration", duration);
		rTable->GetValue("radius", radius);

		EMP(radius, duration);

		return true;
	}
	else if (!strcmp(event,"emp_windup"))
	{
		return true;
	}
	else if (!strcmp(event, "scan"))
	{
		m_scanRange=3.0f;
		m_scanFov=60.0f;

		float duration=2.0f;

		rTable->GetValue("range", m_scanRange);
		rTable->GetValue("fov", m_scanFov);
		rTable->GetValue("duration", duration);

		if (duration>0.0f)
		{
			m_scanTimer=duration;
			m_scanTick=0.0f;
		}

		m_scanned.resize(0);

		return true;
	}
	else if (!strcmp(event,"energy_sensor"))
	{
		Vec3 pos(0,0,0);
		float awareness=0.0f;

		rTable->GetValue("pos", pos);
		rTable->GetValue("awareness", awareness);

		EnergySensor(pos, awareness);

		return true;
	}

	return CActor::CreateCodeEvent(rTable);
}

void CSpotter::EMP(float radius, float duration)
{
	IPhysicalEntity **pEntities;
	Vec3 rvec(radius,radius,radius);
	Vec3 center(GetEntity()->GetWorldPos());

	if(int n = gEnv->pPhysicalWorld->GetEntitiesInBox(center-rvec,center+rvec,pEntities, ent_living))
	{
		for(int i=0; i<n; i++)
		{
			IEntity * pEntity = (IEntity*) pEntities[i]->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
			if(pEntity)
			{
				CActor *pActor=static_cast<CActor *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()));

				if (pActor && pActor->HasNanoSuit())
				{
					float distance=(pActor->GetEntity()->GetWorldPos()-center).len();
					duration=duration*CLAMP((distance/radius), 0.0f, 1.0f);

					SNanoSuitEvent event;
					event.event = eNanoSuitEvent_DISABLE_SUIT;
					event.fParam = duration;
					pActor->SendActorSuitEvent(event);
				}
			}
		}
	}
}

void CSpotter::Scan(float range, float fov)
{
	IPhysicalEntity **pEntities;
	Vec3 rvec(range*1.1f,range*1.1f,range*1.1f);
	Vec3 center(GetEntity()->GetWorldPos());

	SMovementState state;
	m_pMovementController->GetMovementState(state);

	Matrix34 scanMatrix(m_modelInfo.viewMtx);
	scanMatrix.AddTranslation(state.eyePosition);

	CCamera view;
	view.SetFrustum(800, 800, DEG2RAD(fov));
	view.SetMatrix(scanMatrix);

	if(int n = gEnv->pPhysicalWorld->GetEntitiesInBox(center-rvec,center+rvec,pEntities, ent_living))
	{
		for(int i=0; i<n; i++)
		{
			IEntity * pEntity = (IEntity*) pEntities[i]->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
			if(pEntity)
			{
				CActor *pActor=static_cast<CActor *>(g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pEntity->GetId()));

				if (pActor && pActor->HasNanoSuit())
				{
					if (stl::find(m_scanned, pEntity->GetId()))
						continue;

					if (ICharacterInstance *pCharacter=pActor->GetEntity()->GetCharacter(0))
					{
						if (IPhysicalEntity *pPhysicalEntity=pCharacter->GetISkeletonPose()->GetCharacterPhysics(-1))
						{
							pe_status_pos pos;
							if (pPhysicalEntity->GetStatus(&pos))
							{
								AABB box(pos.BBox[0], pos.BBox[1]);
								OBB obb(OBB::CreateOBBfromAABB(pos.q, box));

								SNanoSuitEvent event;
								event.event = eNanoSuitEvent_ACTION;

								if (view.IsOBBVisible_F(pos.pos, obb)!=CULL_EXCLUSION)
								{
									event.sParam = "Suit_EnergyOverload";
									
									m_scanned.push_back(pEntity->GetId());
								}
								else
									event.sParam = "Suit_Interference";

								pActor->SendActorSuitEvent(event);
							}
						}
					}
				}
			}
		}
	}
}


void CSpotter::EnergySensor(const Vec3 &pos, float awareness)
{
	m_sensorPos=pos;
	m_sensorAwareness=awareness;
}

void CSpotter::DrawSensor()
{
	IEntity *pEntity=GetEntity();
	if (m_sensorAwareness<=0.0001f)
	{
		pEntity->SetSlotFlags(1, pEntity->GetSlotFlags(1)&~ENTITY_SLOT_RENDER);
		return;
	}

	pEntity->SetSlotFlags(1, pEntity->GetSlotFlags(1)|ENTITY_SLOT_RENDER);

	Vec3 pos=pEntity->GetWorldPos()+Vec3(0,0,1.0f);
	Vec3 dir=m_sensorPos-pos;
	dir.NormalizeSafe();

	Matrix34 slotTM(Matrix33::CreateRotationVDir(dir)*Matrix33::CreateRotationX(DEG2RAD(-90.0f)));
	slotTM.AddTranslation(pos);

	float s=0.25f+m_sensorAwareness*0.75f;

	slotTM=slotTM*Matrix33::CreateScale(Vec3(0.35f*s, 0.35f*s, 1.0f*s));

	slotTM=pEntity->GetWorldTM().GetInverted()*slotTM;
	pEntity->SetSlotLocalTM(1, slotTM);

	gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_sensorPos, 0.25f, ColorB(128, 0, 255), true);
}