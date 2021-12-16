/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: PlayerHologram.cpp$
$DateTime$
Description: Hologram player, a visual entity fired from the Hologram weapon mode

-------------------------------------------------------------------------
History:
- 01/04/2009 16:23:50: Created by Tom Berry

*************************************************************************/

#include "StdAfx.h"
#include "PlayerHologram.h"
#include "IPlayerInput.h"
#include "PlayerInput.h"
#include "NetPlayerInput.h"
#include "GameActions.h"
#include "Audio/AudioSignalPlayer.h"
#include "Item.h"
#include "GameCVars.h"
#include "HUD/HUD.h"

CPlayerHologram::CPlayerHologram() 
: 
	CPlayer(), 
	m_done(false), 
	m_moving(false),
	m_targetFacing(0.0f, 1.0f, 0.0f), 
	m_lastPosition(0.0f, 0.0f, 0.0f),
	m_newLookFacing(0.f, 1.f, 0.f),
	m_currentLookFacing(0.f,1.f,0.f),
	m_initialised(false),
	m_checkProxTimer(0.f),
	m_lifetime(0.f),
	m_blockedTime(0.f),
	m_updateLookTimer(0.f)
{
	m_crouch = (rand()%2) == 0;
	m_pPlayerInput.reset(new CNetPlayerInput(this));
}

CPlayerHologram::~CPlayerHologram()
{
	gEnv->pAISystem->UnregisterAIEventListener(this);

	SHUDEvent newRemoveObjective(eHUDEvent_OnRemoveObjective);
	newRemoveObjective.ReserveData(1);
	newRemoveObjective.AddData( static_cast<int>(GetEntityId()) ); /*(EntityId)*/
	CHUD::CallEvent(newRemoveObjective);
}

void CPlayerHologram::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	if (!m_initialised)
	{
		Initialise();
		m_initialised = true;
		CAudioSignalPlayer::JustPlay("HologramActivate", GetEntityId());
	}

	CPlayer::Update(ctx, updateSlot);

	const int stimFlags = (1<<AISTIM_EXPLOSION) | (1<<AISTIM_BULLET_HIT) | (1<<AISTIM_BULLET_WHIZZ);
	gEnv->pAISystem->RegisterAIEventListener(this, GetEntity()->GetWorldPos(), 4.0f, stimFlags);

	if (!GetPlayerInput() || (GetPlayerInput()->GetType() != IPlayerInput::NETPLAYER_INPUT))
	{
		m_pPlayerInput.reset( new CNetPlayerInput(this) );
	}

	if (gEnv->bServer)
	{
		static float REACHED_TARGET_SQR_DIST = (0.25f*0.25f);
		static float DESIRED_SPEED = 1.0f;
		static float MIN_LOOK_TIME = 0.5f;
		static float MAX_LOOK_TIME = 2.f;
		static float MIN_PITCH	   = 0.0f;
		static float MAX_PITCH	   = -15.0f;
		static float MIN_YAW	   = -45.0f;
		static float MAX_YAW	   = 45.0f;

		bool updateInput = (m_done == m_moving);

		if (m_done)
		{
			m_updateLookTimer -= ctx.fFrameTime;
			if (m_updateLookTimer <= 0.0f)
			{
				float baseYaw = cry_atan2f(m_targetFacing.x, m_targetFacing.y);
				float pitch   = DEG2RAD(Random(MIN_PITCH, MAX_PITCH));
				float yaw     = baseYaw + DEG2RAD(Random(MIN_YAW, MAX_YAW));
				
				Quat rotation;
				Vec3 YAxis(0.0f, 1.0f, 0.0f);
				rotation.SetRotationXYZ(Ang3(0.0f, pitch, yaw));
				m_newLookFacing = YAxis * rotation;
				m_updateLookTimer = Random(MIN_LOOK_TIME, MAX_LOOK_TIME);
			}

			updateInput = true;
			m_currentLookFacing.SetLerp(m_currentLookFacing, m_newLookFacing, 3.f*ctx.fFrameTime);
		}
		else
		{
			Vec3 curPos = GetEntity()->GetWorldPos();
			
			if(m_lifetime >= g_pGameCVars->g_hologram_initialNoMovementCheckTime)
			{
				if (m_lastPosition.GetSquaredDistance2D(curPos) < g_pGameCVars->g_hologram_negligibleMovementSqrd)
				{
					m_blockedTime += ctx.fFrameTime;
				}
				else
				{
					m_blockedTime = 0.0f;
				}
			}
			else
			{
				m_lifetime += ctx.fFrameTime;
			}
			
			m_lastPosition = curPos;

			if ((m_blockedTime > g_pGameCVars->g_hologram_maxBlockedTime) || (m_targetPosition.GetSquaredDistance2D(GetEntity()->GetWorldPos()) <= REACHED_TARGET_SQR_DIST))
			{
				//--- Send our target position to the script
				IEntity *pEntity = GetEntity(); 
				IScriptTable* pScriptTable = pEntity ? pEntity->GetScriptTable() : 0;

				if (pScriptTable)
				{
					HSCRIPTFUNCTION scriptEvent(NULL);	
					pScriptTable->GetValue("HologramAtTarget", scriptEvent);

					if (scriptEvent)
						Script::Call(gEnv->pScriptSystem,scriptEvent,pScriptTable);

					gEnv->pScriptSystem->ReleaseFunc(scriptEvent);
				}

				m_done = true;
				updateInput = true;
			}
		}


		if (updateInput)
		{
			static bool USINGLOOKIK		 = true;
			static bool SET_PSUEDO_SPEED = true;
			static bool SET_AIMING		 = true;
			Vec3 dir = m_targetPosition - GetEntity()->GetWorldPos();
			dir.z = 0.0f;
			dir.Normalize();
			Quat worldRot = GetBaseQuat();
			SSerializedPlayerInput input;
			if (m_done)
			{
				input.stance		= m_crouch ? STANCE_CROUCH : STANCE_STAND;
				input.deltaMovement.Set(0.0f, 0.0f, 0.0f);
				input.lookDirection = m_currentLookFacing;
				input.bodyDirection = m_targetFacing;
				if (SET_PSUEDO_SPEED)
					input.pseudoSpeed   = 0.0f;
				m_moving = false;
			}
			else
			{
				m_targetFacing		= dir;
				m_currentLookFacing		= dir;
				input.stance		= STANCE_STAND;
				input.deltaMovement = dir * DESIRED_SPEED;
				input.lookDirection = m_currentLookFacing;
				input.bodyDirection = m_targetFacing;
				if (SET_PSUEDO_SPEED)
					input.pseudoSpeed   = DESIRED_SPEED;
				m_moving = true;
			}
			input.leanl			= 0.0f;
			input.leanr			= 0.0f;
			input.sprint		= false;
			input.usinglookik   = USINGLOOKIK;
			input.aiming		= SET_AIMING;

			GetPlayerInput()->SetState(input);
		}

		if((m_checkProxTimer += ctx.fFrameTime) > 0.5f)
		{
			m_checkProxTimer -= 0.25f;

			IPhysicalEntity** pPhysicalEntities;

			const Vec3& center = GetEntity()->GetWorldPos();
			const Vec3& radiusVec = Vec3(0.5f, 0.5f, 0.5f);
			const Vec3 boxMin = center - radiusVec;
			const Vec3 boxMax = center + radiusVec;

			int numEntities = gEnv->pPhysicalWorld->GetEntitiesInBox(boxMin, boxMax, pPhysicalEntities, ent_living);

			if(numEntities > 1)
			{
				InitiateFlicker();
			}
		}
	}
}

void CPlayerHologram::Revive( EReasonForRevive reasonForRevive )
{
	CPlayer::Revive(reasonForRevive);

	if(m_pNanoSuit == NULL)
	{
		m_pNanoSuit = new CNanoSuit(*this);
		m_pNanoSuit->ActivateMode(eNanoSuitMode_Armor);

	}
}

void CPlayerHologram::SetTarget(const Vec3 &targetPos)
{
	m_targetPosition = targetPos;

	//--- Send our target position to the script
	IEntity *pEntity = GetEntity(); 
	IScriptTable* pScriptTable = pEntity ? pEntity->GetScriptTable() : 0;

	if (pScriptTable)
	{
		HSCRIPTFUNCTION scriptEvent(NULL);	
		pScriptTable->GetValue("HologramInit", scriptEvent);

		if (scriptEvent)
			Script::Call(gEnv->pScriptSystem,scriptEvent,pScriptTable,targetPos);

		gEnv->pScriptSystem->ReleaseFunc(scriptEvent);
	}
}

void CPlayerHologram::PostPhysicalize()
{
	CPlayer::PostPhysicalize();
}

bool CPlayerHologram::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (!CPlayer::NetSerialize(ser, aspect, profile, flags))
		return false;

	bool reading=ser.IsReading();

	if (aspect == eEA_Physics)
	{
		ser.Value("targetPosition", m_targetPosition, 'wrl3');

		if(reading)
		{
			SetTarget(m_targetPosition);
		}
	}

	return true;
}

void CPlayerHologram::SetParams(SmartScriptTable &rTable,bool resetFirst)
{
	Vec3 targetPos;
	bool yesno;
	if(rTable->GetValue("setTarget", targetPos ))
	{
		//CryLog("Setting Target: (%f, %f, %f)", targetPos.x, targetPos.y, targetPos.z);
	}
	else if (rTable->GetValue("cloak", yesno ))
	{
		//CryLog("Setting cloak: (%d)", yesno);

		IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)GetEntity()->GetProxy(ENTITY_PROXY_RENDER);
		pRenderProxy->SetMaterialLayersMask( yesno ? MTL_LAYER_CLOAK : 0 );

		if (CItem* pItem = static_cast<CItem*>(GetCurrentItem(true)))
			pItem->CloakSync(true);
	}

	CPlayer::SetParams(rTable, resetFirst);
}

void CPlayerHologram::OnAIEvent(EAIStimulusType type, const Vec3& pos, float radius, float threat, EntityId sender)
{
	InitiateFlicker();
}

void CPlayerHologram::InitiateFlicker()
{
	//--- Send our target position to the script
	IEntity *pEntity = GetEntity(); 
	IScriptTable* pScriptTable = pEntity ? pEntity->GetScriptTable() : 0;

	if (pScriptTable)
	{
		HSCRIPTFUNCTION scriptEvent(NULL);	
		pScriptTable->GetValue("HologramFlicker", scriptEvent);

		if (scriptEvent)
			Script::Call(gEnv->pScriptSystem,scriptEvent,pScriptTable);

		gEnv->pScriptSystem->ReleaseFunc(scriptEvent);
	}

	CAudioSignalPlayer::JustPlay("HologramFlicker", GetEntityId());
}

void CPlayerHologram::Initialise()
{
	IEntity *pEntity = GetEntity(); 
	IPhysicalEntity* pPhysicalEntity = pEntity->GetPhysics();
	IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
	ISkeletonPose* pSkeleton = pEntity->GetCharacter(0)->GetISkeletonPose();

	if (pPhysicalEntity)
	{
		//--- Disable collision against most types such as other characters
		pe_player_dynamics pd;
		//pd.flagsColliderOR=pp.flagsColliderAND = geom_colltype_debris;
		pd.collTypes = ent_static|ent_terrain|ent_rigid|ent_sleeping_rigid;
		pd.mass = 0;
		//pd.bActive = 0;
		//pd.bActive = false; //--- Kills gravity!
		pPhysicalEntity->SetParams(&pd);

		//pd.bActive = 0;
		//pd.collTypes = ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid|ent_living;
		//pp.flagsAND = ~geom_colltype_player;

		//--- Disable collision of other objects & chars against this
		pe_params_part pp;
		pp.flagsAND = ~geom_colltype_player;
		//This allows the char to pass through objects... a possible response but for now we'll do the stop & crouch pp.flagsColliderAND = 0;//~geom_colltype_player;
		pPhysicalEntity->SetParams(&pp);

		pe_params_flags pf;
		pf.flagsAND = ~(lef_push_objects | lef_push_players | pef_log_collisions);// | pef_pushable_by_players);
		pPhysicalEntity->SetParams(&pf);

		pSkeleton->DestroyCharacterPhysics(0);
	}

	pSkeleton->SetAimIKTargetSmoothTime(0.f);
	pRenderProxy->SetMaterialLayersBlend( MTL_LAYER_BLEND_CLOAK );

	if (CItem* pItem = static_cast<CItem*>(GetCurrentItem(true)))
		pItem->CloakSync(true);
}

void CPlayerHologram::SetHealth( int health )
{
	if(health == 0)
	{
		CAudioSignalPlayer::JustPlay("HologramDeactivate", GetEntityId());
	}
}

void CPlayerHologram::OnChangeTeam()
{
	if(IsFriendlyEntity(g_pGame->GetIGameFramework()->GetClientActorId()))
	{
		SHUDEvent newMissionObjective(eHUDEvent_OnNewObjective);
		newMissionObjective.ReserveData(2);
		newMissionObjective.AddData( static_cast<int>(GetEntityId()) ); /*(EntityId)*/
		newMissionObjective.AddData( static_cast<int>(EGRMO_FriendlyHolo) ); /*(EGameRulesMissionObjectives)*/ 
		CHUD::CallEvent(newMissionObjective);
	}
}