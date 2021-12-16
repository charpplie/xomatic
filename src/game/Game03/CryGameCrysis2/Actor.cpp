/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 7:10:2004   14:48 : Created by Márcio Martins
												taken over by Filippo De Luca

*************************************************************************/
#include "StdAfx.h"
#include <StringUtils.h>
#include "Game.h"
#include "GameCVars.h"
#include "Actor.h"
#include "ScriptBind_Actor.h"
#include "ISerialize.h"
#include "GameUtils.h"
#include <ICryAnimation.h>
#include <IGameTokens.h>
#include <IItemSystem.h>
#include <IInteractor.h>
#include "Item.h"
#include "Weapon.h"
#include "Player.h"
#include "GameRules.h"
#include "Battlechatter.h"
#include <IMaterialEffects.h>
#include "IVehicleSystem.h"
#include "IAgent.h"
#include "IPlayerInput.h"
#include "Utility/StringUtils.h"
#include "HUD/HUD.h"
#include "IFacialAnimation.h"
#include "ScreenEffects.h"
#include "PerkIconData.h"
#include "TacticalManager.h"

#include "INetwork.h"
#include "AI/GameAISystem.h"
#include "Utility/CryWatch.h"

#include "RecordingSystem.h"
#include "AutoAimManager.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

#include "IGameStatistics.h"
#include "IAIActor.h"

IItemSystem *CActor::m_pItemSystem=0;
IGameFramework	*CActor::m_pGameFramework=0;
IGameplayRecorder	*CActor::m_pGameplayRecorder=0;

SStanceInfo CActor::m_defaultStance;

//For weapon prototype 'Volatile Spike'
static const float s_volatileSplashLength = 10.f;

#define ITEM_SWITCH_TIMER_ID	525
#define PHYSICS_COUNTER_MAX		16

static const char* s_bonestr[BONE_ID_NUM] = 
{	
	"Bip01", 			//BONE_BIP01
	"Bip01 Spine1", 		//BONE_SPINE
	"Bip01 Spine2", 	//BONE_SPINE2
	"Bip01 Spine3",		//BONE_SPINE3
	"Bip01 Head",		//BONE_HEAD
	"eye_right_bone", 	//BONE_EYE_R
	"eye_left_bone", 	//BONE_EYE_L
	"weapon_bone", 		//BONE_WEAPON
	"weapon_bone", 		//BONE_WEAPON2
	"Bip01 R Foot", 	//BONE_FOOT_R
	"Bip01 L Foot", 	//BONE_FOOT_L
	"Bip01 R Forearm", 	//BONE_ARM_R
	"Bip01 L Forearm", 	//BONE_ARM_L
	"Bip01 R Calf", 	//BONE_CALF_R
	"Bip01 L Calf",		//BONE_CALF_L
	"Bip01 Camera",		//BONE_CAMERA
	"Bip01 Spine1",		//BONE_AUTOAIMTARGET
	"Spine03"	,				//BONE_AUTOAIMTARGET_ALIEN
	"Head"						//BONE_ALIEN_HEAD
};	

#if !defined(_RELEASE)
static AUTOENUM_BUILDNAMEARRAY(s_actorStateNames, ActorStateList);
#endif


//------------------------------------------------------------------------
// "W" stands for "world"
void SIKLimb::SetWPos(IEntity *pOwner,const Vec3 &pos,const Vec3 &normal,float blend,float recover,int requestID)
{
  assert(!_isnan(pos.len2()));
  assert(!_isnan(normal.len2()));
  assert(pos.len()<25000.f);

	// NOTE Dez 13, 2006: <pvl> request ID's work like priorities - if
	// the new request has an ID lower than the one currently being performed,
	// nothing happens. 
	if (requestID<blendID)
		return;

	goalWPos = pos;
	goalNormal = normal;

	if (requestID!=blendID)
	{
		blendTime = blendTimeMax = blend;
		blendID = requestID;
	}
	else if (blendTime<0.001f)
	{
		// NOTE Dez 18, 2006: <pvl> this is just a hacky way of telling
		// the Update() function that the client code called SetWPos()
		// in this frame.  Only after the client code stops calling this
		// function, the "recovery" branch of Update() starts to be
		// executed.
		blendTime = 0.0011f;
	}

	recoverTime = recoverTimeMax = recover;
}

void SIKLimb::SetLimb(int slot,const char *limbName,int rootID,int midID,int endID,int iFlags)
{
	rootBoneID = rootID;
	endBoneID = endID;
	middleBoneID = midID;

	cry_strncpy(name,limbName,sizeof(name));

	blendID = -1;

	flags = iFlags;

	characterSlot = slot;
}

void SIKLimb::Update(IEntity *pOwner,float frameTime)
{	
  ICharacterInstance *pCharacter = pOwner->GetCharacter(characterSlot);

	lAnimPosLast = lAnimPos;
	// pvl: the correction for translation is to be removed once character offsets are redone
//	lAnimPos = pCharacter->GetISkeleton()->GetAbsJointByID(endBoneID).t - pOwner->GetSlotLocalTM (characterSlot, false).GetTranslation ();

	Vec3 vRootBone = pCharacter->GetISkeletonPose()->GetAbsJointByID(0).t; // - pOwner->GetSlotLocalTM (characterSlot, false).GetTranslation ();
	vRootBone.z=0;
	lAnimPos = pCharacter->GetISkeletonPose()->GetAbsJointByID(endBoneID).t-vRootBone;// - pOwner->GetSlotLocalTM (characterSlot, false).GetTranslation ();
  
  assert(!_isnan(lAnimPos.len2()));

	bool setLimbPos(true);
	Vec3 finalPos=Vec3(ZERO);

	if (blendTime>0.001f)
	{
		Vec3 limbWPos = currentWPos;
		finalPos = goalWPos;

		//float middle(0.5f-fabs(0.5f - (blendTime / blendTimeMax)));
		//finalPos.z += middle * 2.0f * 5.0f;

		if (blendTime == 0.0011f) blendTime = 0.0f;

		finalPos -= (finalPos - limbWPos) * min(blendTime / blendTimeMax,1.0f);
		currentWPos = finalPos;

		blendTime -= frameTime;
	}
	else if (recoverTime>0.001f)
	{
		Vec3 limbWPos = currentWPos;
		finalPos = pOwner->GetSlotWorldTM(characterSlot) * lAnimPos;

		finalPos -= (finalPos - limbWPos) * min(recoverTime / recoverTimeMax,1.0f);
		currentWPos = finalPos;
		goalNormal.zero();

		recoverTime -= frameTime;
		
		if (recoverTime<0.001f)
			blendID = -1;
	}
	else
	{
		currentWPos = pOwner->GetSlotWorldTM(characterSlot) * lAnimPos;
		setLimbPos = false;
	}

  assert(!_isnan(finalPos.len2()));
  assert(!_isnan(goalNormal.len2()));

	if (setLimbPos)
	{
		if (flags & IKLIMB_RIGHTHAND)
			pCharacter->GetISkeletonPose()->SetHumanLimbIK(finalPos,"RgtArm01"); //SetRArmIK(finalPos);
		else if (flags & IKLIMB_LEFTHAND)
			pCharacter->GetISkeletonPose()->SetHumanLimbIK(finalPos,"LftArm01");  //SetLArmIK(finalPos);
		else if (middleBoneID>-1)
		{
			pCharacter->GetISkeletonPose()->SetCustomArmIK(finalPos,rootBoneID,middleBoneID,endBoneID);
		}
		else
		{
			ISkeletonPose* pISkeletonPose = pCharacter->GetISkeletonPose();
			uint32 numJoints	= pISkeletonPose->GetJointCount();
			QuatT* pRelativeQuatIK = (QuatT*)alloca( numJoints*sizeof(QuatT) );
			QuatT* pAbsoluteQuatIK = (QuatT*)alloca( numJoints*sizeof(QuatT) );

			pISkeletonPose->CCDInitIKBuffer(pRelativeQuatIK,pAbsoluteQuatIK);
			pISkeletonPose->CCDInitIKChain(rootBoneID,endBoneID);

			Vec3 limbEndNormal(0,0,0);
			//limbEndNormal = Matrix33(pISkeleton->GetAbsJMatrixByID(endBoneID)).GetColumn(0);
			//gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(finalPos, ColorB(0,255,0,100), finalPos + limbEndNormal * 3.0f, ColorB(255,0,0,100));
			pISkeletonPose->CCDRotationSolver(finalPos,0.02f,0.25f,100,goalNormal,pRelativeQuatIK,pAbsoluteQuatIK);
			pISkeletonPose->CCDTranslationSolver(finalPos,pRelativeQuatIK,pAbsoluteQuatIK);
			pISkeletonPose->CCDUpdateSkeleton(pRelativeQuatIK,pAbsoluteQuatIK);
		}

		//gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(finalPos,0.2f,ColorB(0,255,255,255) );
	}
}

//--------------------
IVehicle *SLinkStats::GetLinkedVehicle()
{
	if (!linkID)
		return NULL;
	else
	{
		IVehicle *pVehicle = NULL;
		if(g_pGame && g_pGame->GetIGameFramework() && g_pGame->GetIGameFramework()->GetIVehicleSystem())
			pVehicle = g_pGame->GetIGameFramework()->GetIVehicleSystem()->GetVehicle(linkID);
		//if the vehicle doesnt exist and this is supposed to be a vehicle linking forget about it.
		if (!pVehicle && flags & LINKED_VEHICLE)
			UnLink();

		return pVehicle;
	}
}

void SLinkStats::Serialize(TSerialize ser)
{
	assert(ser.GetSerializationTarget() != eST_Network);

	ser.BeginGroup("PlayerLinkStats");

	//when reading, reset the structure first.
	if (ser.IsReading())
		*this = SLinkStats();

	ser.Value("linkID", linkID);
	ser.Value("flags", flags);

	ser.EndGroup();
}
//--------------------

SActorAnimationEvents CActor::s_animationEventsTable;

void SActorAnimationEvents::Init()
{
	if (!m_initialized)
	{
		Crc32Gen* pCRC32 = gEnv->pSystem->GetCrc32Gen();

		m_soundId = pCRC32->GetCRC32Lowercase("sound");
		m_plugginTriggerId = pCRC32->GetCRC32Lowercase("PluginTrigger");
		m_ragdollStartId = pCRC32->GetCRC32Lowercase( "RagdollStart");
		m_footStepId = pCRC32->GetCRC32Lowercase("footstep");
		m_swimmingStrokeId = pCRC32->GetCRC32Lowercase("swimmingStroke");
		m_stealthKillId = pCRC32->GetCRC32Lowercase("stealthkill_kill");
		m_deathReactionEndId = pCRC32->GetCRC32Lowercase("DeathReactionEnd");
		m_footStepImpulseId = pCRC32->GetCRC32Lowercase("footstep_impulse");
		m_reactionOnCollision = pCRC32->GetCRC32Lowercase("ReactionOnCollision");
	}
}

//------------------------------------------------------------------------
CActor::CActor()
: m_pAnimatedCharacter(0)
, m_isClient(false)
, m_isMigrating(false)
, m_health(100.0f)
, m_maxHealth(0)
, m_pMovementController(0)
, m_stance(STANCE_NULL)
, m_desiredStance(STANCE_NULL)
, m_pGrabHandler(NULL)
,	m_teamId(0)
, m_pInventory(0)
, m_sleepTimer(0.0f)
, m_sleepTimerOrg(0.0f)
, m_lostHelmet(0)
, m_pWeaponAM(0)
, m_ragdollOnCollision(false)
, m_registeredInAutoAimMng(false)
, m_actorState(kActorState_uninitialized)
, m_statsTracker(NULL)
{
	m_currentPhysProfile=CActor::GetDefaultProfile(eEA_Physics);
	//memset(&m_stances,0,sizeof(m_stances));
	//SetupStance(STANCE_NULL,&SStanceInfo());

	m_timeImpulseRecover = 0.0f;
	m_airResistance = 0.0f;
	m_airControl = 1.0f;
	m_inertia = 10.0f;
	m_inertiaAccel = 11.0f;
	m_netLastSelectablePickedUp = 0;
	m_enableSwitchingItems = true;
	m_enableIronSights = true;
	m_enablePickupItems = true;
	m_selfRefWrapper = *this;
	m_netFirstPhysUpdate = true;
	m_netPhysCounter = 0;
}

//------------------------------------------------------------------------
CActor::~CActor()
{
	ClearExtensionCache();
	GetGameObject()->SetMovementController(NULL);
	SAFE_RELEASE(m_pMovementController);

	IInventory *pInventory = CActor::GetInventory();
	if (pInventory)
	{
		if (gEnv->bServer)
			pInventory->Destroy();
		GetGameObject()->ReleaseExtension("Inventory");
	}
	
	if (m_pAnimatedCharacter)
		GetGameObject()->ReleaseExtension("AnimatedCharacter");
	GetGameObject()->ReleaseView( this );
	GetGameObject()->ReleaseProfileManager( this );

	if(m_lostHelmet)
		gEnv->pEntitySystem->RemoveEntity(m_lostHelmet);

	if(g_pGame && g_pGame->GetIGameFramework() && g_pGame->GetIGameFramework()->GetIActorSystem())
		g_pGame->GetIGameFramework()->GetIActorSystem()->RemoveActor( GetEntityId() );

	CGameAISystem::GetInstance()->LeaveAllModules(GetEntityId());

	UnRegisterInAutoAimManager();

#if !defined(_RELEASE)
	CRY_ASSERT_MESSAGE(m_actorState == kActorState_beingDestroyed, string().Format("Actor being destroyed but is in %s", s_actorStateNames[m_actorState]));
#endif

	SAFE_DELETE(m_pGrabHandler);
	SAFE_DELETE(m_pWeaponAM);
}

void CActor::ClearExtensionCache()
{
	if (m_pInventory)
	{
		GetGameObject()->ReleaseExtension("Inventory");
		m_pInventory = 0;
	}
}
//------------------------------------------------------------------------
void CActor::CrapDollize()
{
	// make sure dead AI is not affected by explosions and the player
#ifdef CRAPDOLLS
	IPhysicalEntity* pPhysicalEntity=GetEntity()->GetPhysics();
	if (pPhysicalEntity)
	{
		pe_params_part ppp;
		ppp.flagsAND = ~(geom_colltype_explosion|geom_colltype_ray|geom_colltype_foliage_proxy|geom_colltype_player);
		pPhysicalEntity->SetParams(&ppp);
	}
#endif //CRAPDOLLS
}

//------------------------------------------------------------------------
bool CActor::Init( IGameObject * pGameObject )
{
	SetGameObject(pGameObject);

	if (!GetGameObject()->CaptureView(this))
		return false;
	if (!GetGameObject()->CaptureProfileManager(this))
		return false;

	m_pMovementController = CreateMovementController();
	GetGameObject()->SetMovementController(m_pMovementController);

	g_pGame->GetIGameFramework()->GetIActorSystem()->AddActor( GetEntityId(), this );

	g_pGame->GetActorScriptBind()->AttachTo(this);
	m_pAnimatedCharacter = static_cast<IAnimatedCharacter*>(GetGameObject()->AcquireExtension("AnimatedCharacter"));
	if (m_pAnimatedCharacter)
	{
		BindInputs( m_pAnimatedCharacter->GetAnimationGraphState() );
	}

	pGameObject->AcquireExtension("Inventory");

	if (!m_pGameFramework)
	{
		m_pGameFramework = g_pGame->GetIGameFramework();
		m_pGameplayRecorder = g_pGame->GetIGameFramework()->GetIGameplayRecorder();
		m_pItemSystem = m_pGameFramework->GetIItemSystem();
	}

	GetGameObject()->EnablePrePhysicsUpdate( gEnv->bMultiplayer ? ePPU_Always : ePPU_WhenAIActivated );

	if (!GetGameObject()->BindToNetwork())
		return false;

	GetEntity()->SetFlags(GetEntity()->GetFlags() |
		(ENTITY_FLAG_ON_RADAR | ENTITY_FLAG_CUSTOM_VIEWDIST_RATIO | ENTITY_FLAG_TRIGGER_AREAS));

	m_damageEffectController.Init(this);

	g_pGame->GetTacticalManager()->AddEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Unit);

	SHUDEvent hudevent(eHUDEvent_AddEntity);
	hudevent.AddData(SHUDEventData((int)GetEntityId()));
	CHUD::CallEvent(hudevent);

	s_animationEventsTable.Init();
	m_telemetry.SetOwner(this);

	return true;
}

void CActor::PostInit( IGameObject * pGameObject )
{
	pGameObject->EnableUpdateSlot( this, 0 );	
	pGameObject->EnablePostUpdates( this );

	if(gEnv->bMultiplayer)
	{
		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
		if (pCharacter)
			pCharacter->GetISkeletonPose()->SetAimIKFadeOut(0);
	}

	InitActorAttachments();

	SetActorState(kActorState_initialized);
}

//----------------------------------------------------------------------
void CActor::InitActorAttachments()
{
	if(m_pWeaponAM)
		SAFE_DELETE(m_pWeaponAM);
	
	//Weapon attachments stuff
	m_pWeaponAM = new CWeaponAttachmentManager(this);
	if(!m_pWeaponAM->Init())
	{
		SAFE_DELETE(m_pWeaponAM);
	}
}

//------------------------------------------------------------------------
void CActor::HideAllAttachments(bool isHiding)
{

	if (IItem *pCurrentItem = GetCurrentItem())
	{
		pCurrentItem->GetEntity()->Hide(isHiding);
	}

	//This is only for AI, in MP players don't have back attachments
	if (!IsPlayer())
	{
		if (m_pWeaponAM)
		{
			m_pWeaponAM->HideAllAttachments(isHiding);
		}
	}
}

//------------------------------------------------------------------------
void CActor::InitClient(int channelId)
{
	if (GetHealth()<=0 && !GetSpectatorMode())
		GetGameObject()->InvokeRMI(ClSimpleKill(), NoParams(), eRMI_ToClientChannel, channelId);
}

//------------------------------------------------------------------------
void CActor::Revive( EReasonForRevive reasonForRevive )
{
	ClearExtensionCache();

	if (reasonForRevive == kRFR_FromInit)
		g_pGame->GetGameRules()->OnRevive(this, GetEntity()->GetWorldPos(), GetEntity()->GetWorldRotation(), m_teamId);

	if(gEnv->bServer)
	{
		m_damageEffectController.OnRevive();
	}


	//set the actor game parameters
	SmartScriptTable gameParams;
	if (GetEntity()->GetScriptTable() && GetEntity()->GetScriptTable()->GetValue("gameParams", gameParams))
		SetParams(gameParams,true);

	SetActorModel(); // set the model before physicalizing

	m_stance = STANCE_NULL;
	m_desiredStance = STANCE_NULL;

	if ((gEnv->bServer && reasonForRevive != kRFR_StartSpectating) || (gEnv->bClient && reasonForRevive == kRFR_FromInit))
	{
		Physicalize();
		if(gEnv->bServer)
		GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);
	}

	Freeze(false);

  if (IPhysicalEntity* pPhysics = GetEntity()->GetPhysics())
  {
    pe_action_move actionMove;    
    actionMove.dir.zero();
    actionMove.iJump = 1;

		pe_action_set_velocity actionVel;
		actionVel.v.zero();
		actionVel.w.zero();
    
    pPhysics->Action(&actionMove);
		pPhysics->Action(&actionVel);
  }

	memset(m_boneIDs,-1,sizeof(m_boneIDs));

	if (m_pAnimatedCharacter)
		m_pAnimatedCharacter->ResetState();
	if (m_pMovementController)
		m_pMovementController->Reset();
	if (m_pGrabHandler)
		m_pGrabHandler->Reset();

	m_sleepTimer = 0.0f;
	
	m_linkStats = SLinkStats();

	//	m_lastFootStepPos = ZERO;
	m_pReplacementMaterial = 0;

	m_ragdollOnCollision = false;

	//reset some AG inputs
	if (m_pAnimatedCharacter)
	{
		GetAnimationGraphState()->Pause(false, eAGP_PlayAnimationNode);
		UpdateAnimGraph(m_pAnimatedCharacter->GetAnimationGraphState());
		m_pAnimatedCharacter->GetAnimationGraphState()->SetInput("Action", "idle" );
		m_pAnimatedCharacter->SetParams( m_pAnimatedCharacter->GetParams().ModifyFlags(eACF_EnableMovementProcessing,0));
		m_pAnimatedCharacter->GetAnimationGraphState()->SetInput( m_inputHealth, GetMaxHealth() );
	}

//	m_footstepAccumDistance = 0.0f;

	ResetHelmetAttachment();

	if (ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0))
		pCharacter->EnableProceduralFacialAnimation(GetMaxHealth() > 0);

	if(!IsPlayer())
	{
		const char* const szTeamName = g_pGameCVars->sv_aiTeamName->GetString();
		if(szTeamName && (*szTeamName) != 0)
		{
			IGameRulesSystem *pIGameRulesSystem = g_pGame->GetIGameFramework()->GetIGameRulesSystem();
			CRY_ASSERT(pIGameRulesSystem);

			if (CGameRules *pGameRules=static_cast<CGameRules*>(pIGameRulesSystem->GetCurrentGameRules()))
			{
				int teamId = pGameRules->GetTeamId(szTeamName);
				if(!teamId)
				{
					teamId = pGameRules->CreateTeam(szTeamName);
					CRY_ASSERT(teamId);
				}

				// Team assignment is propagated from server to client,
				// so we should only set a unit's team on the server
				if(gEnv->bServer)
					pGameRules->SetTeam(teamId, GetEntityId());
			}
		}
	}

	//Only from scripts, when AI has already set its properties
	bool registerForAutoAimDuringRevival = (reasonForRevive == CActor::kRFR_ScriptBind) || IsPlayer();
	if (registerForAutoAimDuringRevival)
	{
		RegisterInAutoAimManager();
	}

	// Reset net lerp
	m_netDesiredLocation.zero();
	m_netCurrentLocation.zero();
	m_netDesiredSpeed = 0.f;
	m_netLerpSpeed = 0.f;
	m_netLerpVelocity.zero();
	m_netNewUpdate = false;
	m_netSetPosition = false;
	m_netDoLerp = false;
	m_netLastUpdate=gEnv->pTimer->GetFrameStartTime();
}

IGrabHandler *CActor::CreateGrabHanlder()
{
	m_pGrabHandler = new CBaseGrabHandler(this);
	return m_pGrabHandler;
}

//------------------------------------------------------------------------
void CActor::Physicalize(EStance stance)
{
	//FIXME:this code is duplicated from scriptBind_Entity.cpp, there should be a function that fill a SEntityPhysicalizeParams struct from a script table.
	IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
  assert(pScriptTable);
  if (!pScriptTable)
    return;

  SmartScriptTable physicsParams;
  if (pScriptTable->GetValue("physicsParams", physicsParams))
	{
		pe_player_dimensions playerDim;
		pe_player_dynamics playerDyn;
		SEntityPhysicalizeParams pp;
		
		pp.pPlayerDimensions = &playerDim;
		pp.pPlayerDynamics = &playerDyn;

		pp.nSlot = 0;
		pp.type = PE_LIVING;

		//Separate player's mass from AI, for testing
		CRY_HACK(02, 10, 2009, "filipe - actor mass had to be hacked by now. We need a way to parameterize BasicActorParams based on entity type (move to xml).");
		const char* entityClassName = GetEntity()->GetClass()->GetName();
		string massParamName = string("mass") + "_" + string(entityClassName);
		if (!physicsParams->GetValue(massParamName.c_str(), pp.mass))
			physicsParams->GetValue("mass",pp.mass);
		physicsParams->GetValue("density",pp.density);
		physicsParams->GetValue("flags",pp.nFlagsOR);
		physicsParams->GetValue("partid",pp.nAttachToPart);
		physicsParams->GetValue("stiffness_scale",pp.fStiffnessScale);


		SmartScriptTable props;
		if(GetEntity()->GetScriptTable()->GetValue("Properties", props))
		{
			float massMult = 1.0f;
			props->GetValue("physicMassMult", massMult);
			pp.mass *= massMult;
		}

		SmartScriptTable livingTab;
		if (physicsParams->GetValue( "Living",livingTab ))
		{
			// Player Dimensions
			if (stance==STANCE_NULL)
			{
				livingTab->GetValue( "height",playerDim.heightCollider );
				livingTab->GetValue( "size",playerDim.sizeCollider );
				livingTab->GetValue( "height_eye",playerDim.heightEye );
				livingTab->GetValue( "height_pivot",playerDim.heightPivot );
				livingTab->GetValue( "use_capsule",playerDim.bUseCapsule );
			}
			else
			{
				const SStanceInfo *sInfo = GetStanceInfo(stance);
				playerDim.heightCollider = sInfo->heightCollider;
				playerDim.sizeCollider = sInfo->size;
				playerDim.heightPivot = sInfo->heightPivot;
				playerDim.maxUnproj = max(0.0f,sInfo->heightPivot);
				playerDim.bUseCapsule = sInfo->useCapsule;
			}

			playerDim.headRadius = 0.0f;
			playerDim.heightEye = 0.0f;

			// Player Dynamics.
			livingTab->GetValue( "inertia",playerDyn.kInertia );
			livingTab->GetValue( "k_air_control",playerDyn.kAirControl);
			livingTab->GetValue( "inertiaAccel",playerDyn.kInertiaAccel );
			livingTab->GetValue( "air_resistance",playerDyn.kAirResistance );
			livingTab->GetValue( "gravity",playerDyn.gravity.z );
			//Separate player's mass from AI, for testing
			if (!physicsParams->GetValue(massParamName.c_str(), playerDyn.mass))
				physicsParams->GetValue("mass",playerDyn.mass);
			livingTab->GetValue( "min_slide_angle",playerDyn.minSlideAngle );
			livingTab->GetValue( "max_climb_angle",playerDyn.maxClimbAngle );
			livingTab->GetValue( "max_jump_angle",playerDyn.maxJumpAngle );
			livingTab->GetValue( "min_fall_angle",playerDyn.minFallAngle );
			livingTab->GetValue( "max_vel_ground",playerDyn.maxVelGround );
			livingTab->GetValue( "timeImpulseRecover",playerDyn.timeImpulseRecover );

			if(!is_unused(playerDyn.timeImpulseRecover))
				m_timeImpulseRecover = playerDyn.timeImpulseRecover;
			else
				m_timeImpulseRecover = 0.0f;

			if(!is_unused(playerDyn.kAirResistance))
				m_airResistance = playerDyn.kAirResistance;
			else
				m_airResistance = 0.0f;

			if(!is_unused(playerDyn.kAirControl))
				m_airControl = playerDyn.kAirControl;
			else
				m_airControl = 1.0f;

			if(!is_unused(playerDyn.kInertia))
				m_inertia = playerDyn.kInertia;
			else
				m_inertia = 10.0f; //Same value as scripts default

			if(!is_unused(playerDyn.kInertiaAccel))
				m_inertiaAccel = playerDyn.kInertiaAccel;
			else
				m_inertiaAccel = 11.0f; //Same value as scripts default

			//CryLogAlways("CActor::Physicalize: %s, inAir: %d inertia: %f, inertiaAccel: %f", GetEntity()->GetName(), isPlayerInAir, m_inertia, m_inertiaAccel);

			const char *colliderMat=0;
			if (livingTab->GetValue( "colliderMat", colliderMat) && colliderMat && colliderMat[0])
			{
				if (ISurfaceType *pSurfaceType=gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeByName(colliderMat))
					playerDyn.surface_idx=pSurfaceType->GetId();
			}
		}

		if (GetEntity()->GetPhysics())
		{
			Ang3 rot(GetEntity()->GetWorldAngles());
			GetEntity()->SetRotation(Quat::CreateRotationZ(rot.z));

			SEntityPhysicalizeParams nop;
			nop.type = PE_NONE;
			GetEntity()->Physicalize(nop);
		}

		GetEntity()->Physicalize(pp);
	}

	// for the client we add an additional proxy for bending vegetation to look correctly
	if (IsPlayer())
	{
		primitives::capsule prim;

		prim.axis.Set(0,0,1);
		prim.center.zero(); prim.r = 0.4f; prim.hh = 0.2f;
		IGeometry *pPrimGeom = gEnv->pPhysicalWorld->GetGeomManager()->CreatePrimitive(primitives::capsule::type, &prim);
		phys_geometry *pGeom = gEnv->pPhysicalWorld->GetGeomManager()->RegisterGeometry(pPrimGeom, 0);
		pe_geomparams gp;
 		gp.pos = Vec3(0.0f,0.2f,0.7f);
		gp.flags = geom_colltype_foliage;
		gp.flagsCollider = 0;
		pGeom->nRefCount = 0;
		//just some arbitrary id, except 100, which is the main cylinder
		GetEntity()->GetPhysics()->AddGeometry(pGeom, &gp, 101);
	}

	//the finish physicalization
	PostPhysicalize();
}

//---------------------------------------------------------
void CActor::SetActorState(EActorState newState)
{
#if !defined(_RELEASE)
	IEntity * pEntity = GetEntity();
	const char * actorName = pEntity->GetName();
	const char * className = pEntity->GetClass()->GetName();
	Vec3 pos = pEntity->GetWorldPos();

	//CryLog("[ACTOR STATE] %s %s changing from %s to %s (health=%d/%d, pos=<%.2f %.2f %.2f>)", className, actorName, s_actorStateNames[m_actorState], s_actorStateNames[newState], GetHealth(), GetMaxHealth(), pos.x, pos.y, pos.z);

#if defined(_DEBUG)
	bool validTransition = (m_actorState != kActorState_beingDestroyed) && (newState != m_actorState);
	if (validTransition)
	{
		switch (newState)
		{
			case kActorState_uninitialized:
			validTransition = false;
			break;

			case kActorState_initialized:
			validTransition = (m_actorState == kActorState_uninitialized);
			break;

			case kActorState_incapacitated:
			validTransition = (m_actorState == kActorState_alive);
			break;
		}
	}
	CRY_ASSERT_MESSAGE(validTransition, string().Format("%s %s doing unexpected/invalid state transition from %s to %s", className, actorName, s_actorStateNames[m_actorState], s_actorStateNames[newState]));
#endif

#endif

	m_actorState = newState;
}

//
void CActor::SetActorModel()
{
	// this should be pure-virtual, but for the moment to support alien scripts
  if (IScriptTable* pScriptTable = GetEntity()->GetScriptTable())
	  Script::CallMethod(pScriptTable, "SetActorModel", IsClient());
}

//------------------------------------------------------------------------
void CActor::PostPhysicalize()
{
	//force the physical proxy to be rebuilt
	m_stance = STANCE_NULL;
	SetStance(STANCE_STAND);

//	if (m_pAnimatedCharacter)
//		m_pAnimatedCharacter->ResetState();


	// [*DavidR | 1/Feb/2010]
	// Disable automatic impulses on collisions with physic particles. We want to have control over impulses on projectiles
	// in game code
	pe_params_part colltype;
	colltype.flagsColliderOR = geom_no_particle_impulse;

	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	if (pCharacter)
	{
		if (IPhysicalEntity* pCharacterPhysics = pCharacter->GetISkeletonPose()->GetCharacterPhysics())
			pCharacterPhysics->SetParams(&colltype);
	}

	if (IsPlayer())
	{
		IEntityRenderProxy *pRenderProxy = static_cast<IEntityRenderProxy *>(GetEntity()->GetProxy(ENTITY_PROXY_RENDER));

		if (pRenderProxy)
		{
			IRenderNode *pRenderNode = pRenderProxy->GetRenderNode();

			if (pRenderNode)
			{
				pRenderNode->SetViewDistRatio(g_pGameCVars->g_actorViewDistRatio);
				pRenderNode->SetLodRatio(gEnv->bMultiplayer? g_pGameCVars->g_mpPlayerLodRatio : 80); //IVO: changed to fix LOD problem in MP
			}
		}

		if (gEnv->bMultiplayer)
		{
			ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);

			if (pCharacter)
			{
				if (IPhysicalEntity* pCharacterPhysics = pCharacter->GetISkeletonPose()->GetCharacterPhysics())
				{
					// Disable particle impulses on the player so that we can filter out friendly fire
					// We will simulate these impulses on the game side
					pe_params_part colltype;
					colltype.flagsColliderOR = geom_no_particle_impulse;
					pCharacterPhysics->SetParams(&colltype);
				}
			}
	}
	}

	//CryLogAlways("CActor::PostPhysicalize: %s, inertia: %f, inertiaAccel: %f", GetEntity()->GetName(), m_inertia, m_inertiaAccel);

	if (m_pAnimatedCharacter)
	{
		SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
		params.timeImpulseRecover = GetTimeImpulseRecover();
		params.flags |= eACF_EnableMovementProcessing | eACF_ZCoordinateFromPhysics | eACF_ConstrainDesiredSpeedToXY;
		m_pAnimatedCharacter->SetParams(params);

		m_pAnimatedCharacter->ResetInertiaCache();
	}
}

//------------------------------------------------------------------------
void CActor::RagDollize( bool fallAndPlay )
{
	SActorStats *pStats = GetActorStats();

	if (GetLinkedVehicle())
		return;
	
	if (pStats && (!pStats->isRagDoll || gEnv->pSystem->IsSerializingFile()))
	{
		GetGameObject()->SetAutoDisablePhysicsMode(eADPM_Never);

		ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
		if (pCharacter)
		{
			// dead guys shouldn't blink
			pCharacter->EnableProceduralFacialAnimation(false);
			//Anton :: SetDefaultPose on serialization
			if(gEnv->pSystem->IsSerializingFile() && pCharacter->GetISkeletonPose())
				pCharacter->GetISkeletonPose()->SetDefaultPose();
		}

		SEntityPhysicalizeParams pp;

		pp.type = PE_ARTICULATED;
		pp.nSlot = 0;
		pp.bCopyJointVelocities = true;

		if(SActorStats* stats = GetActorStats())
			pp.mass = stats->mass;
		if(pp.mass <= 0)
			pp.mass = 80.0f; //never ragdollize without mass [Anton]

		if (fallAndPlay)
		{
			const SActorParams* pParams = GetActorParams();

			pp.fStiffnessScale = pParams ? pParams->fallNPlayStiffness_scale : 1200.0f;
		}

		pe_player_dimensions playerDim;
		pe_player_dynamics playerDyn;

		playerDyn.gravity.z = 15.0f;
		playerDyn.kInertia = 5.5f;

		pp.pPlayerDimensions = &playerDim;
		pp.pPlayerDynamics = &playerDyn;

		IPhysicalEntity *pPhysicalEntity=GetEntity()->GetPhysics();
		if (!pPhysicalEntity || pPhysicalEntity->GetType()!=PE_LIVING)
			pp.nLod = 1;

		// Joints velocities are copied by default for now
		pp.bCopyJointVelocities = true;

		GetEntity()->Physicalize(pp);

		// make sure dead AI is not affected by explosions
		if (!fallAndPlay || GetHealth()<=0)
			CrapDollize();

		pStats->isRagDoll = true;

		m_ragdollOnCollision = false;

		if (fallAndPlay)
		{
			IAnimationGraphState *animGraph = GetAnimationGraphState();
			if (animGraph)
				animGraph->PushForcedState( "FallAndPlay" );
				//animGraph->SetInput("Signal", "fall");
		}

		// [Mikko] 12.10.2007 Skipping the timer reset here or else QL tranquilized characters does not work.
		// Setting sleep timer here is a bug. SetAspectProfile should only affect the actor
		// physicalization and not have any other side effects.
		if (!gEnv->pSystem->IsSerializingFile())
			m_sleepTimer=0.0f;

		CCCPOINT(CActor_RagDollize);
	}
}

//------------------------------------------------------------------------
bool CActor::IsFallen() const
{
	const SActorStats *pStats = GetActorStats();
	return (pStats && pStats->isRagDoll || m_sleepTimer > 0.0f) && GetHealth() > 0;
}

//------------------------------------------------------------------------
// Returns true if the entity is playing the standup animation of the fall and play
//------------------------------------------------------------------------
bool CActor::IsStandingUp() const
{
	const SActorStats* pStats = GetActorStats();
	return pStats && pStats->isInFallNPlay && !pStats->isRagDoll;
}

//------------------------------------------------------------------------
void CActor::Fall(Vec3 hitPos, float sleepTime /*=0.0f*/)
{
	if (IsPlayer() && !g_pGameCVars->pl_health.enable_FallandPlay)
		return;

	if (IsFallen() || !CanFall())
		return;

	bool inVehicle = GetLinkedVehicle() != NULL;
	if (inVehicle)
		return;

	
	//we don't want noDeath (tutorial) AIs to loose their weapon, since we don't have pickup animations yet
	bool	dropWeapon(true);
	bool  hasDamageTable = false;
	SmartScriptTable props;
	SmartScriptTable propsDamage;
	if(GetEntity()->GetScriptTable() && GetEntity()->GetScriptTable()->GetValue("Properties", props))
		if(props->GetValue("Damage", propsDamage))
			hasDamageTable = true;

	if(!hasDamageTable)
		return;

	int canFall(0);
	if(	propsDamage->GetValue("CanFall", canFall) && (canFall == 0))
		return;

	int noDeath(0);
	int	fallPercentage(0);
	if(	propsDamage->GetValue("bNoDeath", noDeath) && noDeath!=0 ||
		propsDamage->GetValue("FallPercentage", fallPercentage) && fallPercentage>0 )
		dropWeapon = false;

	IAISystem *pAISystem=gEnv->pAISystem;
	if (pAISystem)
	{
		if(GetEntity() && GetEntity()->GetAI())
		{
			IAIActor* pAIActor = CastToIAIActorSafe(GetEntity()->GetAI());
			if(pAIActor)
			{
				IAISignalExtraData *pEData = pAISystem->CreateSignalExtraData();	// no leak - this will be deleted inside SendAnonymousSignal
				pEData->point = Vec3(0,0,0);

				pAIActor->SetSignal(1,"OnFallAndPlay",0,pEData);
			}
		}
	}

	CreateScriptEvent("sleep", 0);
	if ( inVehicle )
	{
		if ( IAnimationGraphState *animGraph = GetAnimationGraphState() )
			animGraph->SetInput( m_inputSignal, "fall" );
	}
	else
	{
		GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Sleep);
	}

	//Do we want this for the player? (Sure not for AI)
	if(IsPlayer() && dropWeapon && !inVehicle)
	{
		//DropItem(GetCurrentItemId(), 1.0f, false);
		if (GetCurrentItemId(false))
			HolsterItem(true);
	}

	// stop shooting
	if ( EntityId currentItem = GetCurrentItemId(true) )
		if ( CWeapon* pWeapon = GetWeapon(currentItem) )
			pWeapon->StopFire();

	//add some twist
	if (!IsClient() && hitPos.len() && !inVehicle)
	{
		if(IPhysicalEntity *pPE = GetEntity()->GetPhysics())
		{
			pe_action_impulse imp;

			float angularDir = 1.0f;
			if(IMovementController *pMC = GetMovementController())
			{
				//where was the hit ?
				SMovementState sMovementState;
				pMC->GetMovementState(sMovementState);
				Vec3 rightDir = sMovementState.eyeDirection.Cross(sMovementState.upDirection);
				Vec3 dir = hitPos - GetEntity()->GetWorldPos();
				float right = dir.Dot(rightDir);
				float front = sMovementState.eyeDirection.Dot(dir);

				if(right > 0)
					angularDir = -1.0f;
				if(front < 0)
					angularDir *= -1.0f;
			}

			imp.impulse = Vec3(0, 0, -200.0);
			imp.angImpulse = Vec3(0.0f, 0.0f, angularDir*200.0f);
			pPE->Action(&imp);
		}
	}
	
	float r = cry_frand();
	if(r > 0.5f && r < 0.6f)
		LooseHelmet();

	float sleep(3.0f);
	if(sleepTime>0.0f)
		SetSleepTimer(sleepTime);
	else if(!propsDamage->GetValue("FallSleepTime", sleep) || IsClient())
		SetSleepTimer(3.0f);
	else
		SetSleepTimer(sleep);

	IAnimationGraphState* pAGState = GetAnimationGraphState();
	if ( pAGState && m_pAnimatedCharacter )
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		if ( pCharacter )
		{
			ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
			if ( pSkeletonPose )
			{
				int fnpGroudId = 0;
				IAnimationSet* pAnimSet = pCharacter->GetIAnimationSet();
				if ( pAnimSet )
				{
					char stance[256];
					pAGState->GetInput( m_inputStance, stance );
					strcpy( stance, GetStanceInfo(STANCE_STAND)->name );
					char item[256];
					pAGState->GetInput( pAGState->GetInputId( "Item" ), item );
					if ( item[0] == 0 )
						strcpy( item, "nw" );

					int maxScore = 0;
					const char* groupName = NULL;
					for ( int i = 0; (groupName = pAnimSet->GetFnPAnimGroupName(i)) != NULL && groupName[0] != 0; ++i )
					{
						int score = 0;
						if ( CryStringUtils::stristr( groupName, item ) != NULL )
							score += 2;
						if ( CryStringUtils::stristr( groupName, stance ) != NULL )
							score += 1;
						if ( score > maxScore )
						{
							maxScore = score;
							fnpGroudId = i;
						}
					}
				}
				pSkeletonPose->SetFnPAnimGroup( fnpGroudId );
			}
		}
	}
}

//------------------------------------------------------------------------
void CActor::GoLimp()
{
	ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0);
	if (pCharacter && pCharacter->GetISkeletonAnim())
		pCharacter->GetISkeletonPose()->GoLimp();
}

//------------------------------------------------------------------------
void CActor::StandUp()
{
	if ( GetHealth() <= 0 )
	{
		GoLimp();
	}
	else
	{
		// if m_sleepTimer > 0.0f it means waking up is scheduled already for later so we just ignore this for now!
		if ( m_sleepTimer > 0.0f )
			return;

		if ( GetLinkedVehicle() )
			GetAnimationGraphState()->Hurry();
		else if ( m_currentPhysProfile == eAP_Sleep )
			GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);
	}
}

/*
void CActor::NotifyLeaveFallAndPlay()
{
	IAIObject* pAI;
	if(GetEntity() && (pAI = GetEntity()->GetAI()))
	{
		pAI->Event(AIEVENT_WAKEUP, 0);
		gEnv->pAISystem->SendSignal(SIGNALFILTER_SENDER, 1, "OnFallAndPlayWakeUp", pAI);
	}
}
*/

//------------------------------------------------------------------------
void CActor::SetupStance(EStance stance, SStanceInfo *info)
{
	if (stance > STANCE_NULL && stance < STANCE_LAST)
	{
		m_stances[(int)stance] = *info;
	}
}

//------------------------------------------------------
void CActor::SetStance(EStance desiredStance)
{
#if ENABLE_GAME_CODE_COVERAGE
	if (m_desiredStance != desiredStance)
	{
		switch (m_desiredStance)
		{
			case STANCE_STAND:
			CCCPOINT_IF(m_isClient,  ActorStance_LocalActorStopStand);
			CCCPOINT_IF(!m_isClient, ActorStance_OtherActorStopStand);
			break;

			case STANCE_CROUCH:
			CCCPOINT_IF(m_isClient,  ActorStance_LocalActorStopCrouch);
			CCCPOINT_IF(!m_isClient, ActorStance_OtherActorStopCrouch);
			break;

			case STANCE_SWIM:
			CCCPOINT_IF(m_isClient,  ActorStance_LocalActorStopSwim);
			CCCPOINT_IF(!m_isClient, ActorStance_OtherActorStopSwim);
			break;
		}

		switch (desiredStance)
		{
			case STANCE_STAND:
			CCCPOINT_IF(m_isClient,  ActorStance_LocalActorStartStand);
			CCCPOINT_IF(!m_isClient, ActorStance_OtherActorStartStand);
			break;

			case STANCE_CROUCH:
			CCCPOINT_IF(m_isClient,  ActorStance_LocalActorStartCrouch);
			CCCPOINT_IF(!m_isClient, ActorStance_OtherActorStartCrouch);
			break;

			case STANCE_SWIM:
			CCCPOINT_IF(m_isClient,  ActorStance_LocalActorStartSwim);
			CCCPOINT_IF(!m_isClient, ActorStance_OtherActorStartSwim);
			break;
		}
	}
#endif

	m_desiredStance = desiredStance;
}

//------------------------------------------------------
void CActor::OnStanceChanged(EStance newStance, EStance oldStance)
{
	if(!gEnv->bMultiplayer)
	{
		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();

		if (pScriptTable)
		{
			Script::CallMethod(pScriptTable, "OnStanceChanged", newStance, oldStance);
		}
	}
}

//------------------------------------------------------
IEntity *CActor::LinkToVehicle(EntityId vehicleId) 
{
	// did this link actually change, or are we just re-linking?
	bool changed=((m_linkStats.linkID!=vehicleId)||gEnv->pSystem->IsSerializingFile())?true:false;

	m_linkStats = SLinkStats(vehicleId,LINKED_VEHICLE);
	
	IVehicle *pVehicle = m_linkStats.GetLinkedVehicle();
	IEntity *pLinked = pVehicle?pVehicle->GetEntity():NULL;
  
	if (m_pAnimatedCharacter)
	{
		SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();
		
		bool enabled = pLinked?true:false;
		if (enabled)
		{
//			params.flags &= ~eACF_EnableMovementProcessing;
			params.flags |= eACF_NoLMErrorCorrection;
		}
		else
		{
//			params.flags |= eACF_EnableMovementProcessing;
			params.flags &= ~eACF_NoLMErrorCorrection;
		}
		
		if(gEnv->bServer)
		{
			if(enabled)
			{
				if (changed)
					GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Linked);
			}
			else if(IPhysicalEntity *pPhys = GetEntity()->GetPhysics())
			{
				pe_type type = pPhys->GetType();
				if(type == PE_LIVING)
					GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);
				else if(type == PE_ARTICULATED)
					GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
			}
		}

		m_pAnimatedCharacter->SetParams( params );
		m_pAnimatedCharacter->ForceRefreshPhysicalColliderMode();
		m_pAnimatedCharacter->RequestPhysicalColliderMode(enabled ? eColliderMode_Disabled : eColliderMode_Undefined, eColliderModeLayer_Game, "Actor::LinkToVehicle");
	}
  
  if (pLinked)  
    pLinked->AttachChild(GetEntity(), ENTITY_XFORM_USER|IEntity::ATTACHMENT_KEEP_TRANSFORMATION);
  else
    GetEntity()->DetachThis(IEntity::ATTACHMENT_KEEP_TRANSFORMATION,/*ENTITY_XFORM_USER*/0);
  
	return pLinked;
}

IEntity *CActor::LinkToVehicleRemotely(EntityId vehicleId)
{
	m_linkStats = SLinkStats(vehicleId,LINKED_VEHICLE);

	return m_linkStats.GetLinked();
}

IEntity *CActor::LinkToEntity(EntityId entityId, bool bKeepTransformOnDetach) 
{
	m_linkStats = SLinkStats(entityId,LINKED_FREELOOK);

	IEntity *pLinked = m_linkStats.GetLinked();

	if (m_pAnimatedCharacter)
	{
		SAnimatedCharacterParams params = m_pAnimatedCharacter->GetParams();

		bool enabled = pLinked?true:false;
		if (enabled)
		{
			params.flags &= ~eACF_EnableMovementProcessing;
			params.flags |= eACF_NoLMErrorCorrection;
		}
		else
		{
			params.flags |= eACF_EnableMovementProcessing;
			params.flags &= ~eACF_NoLMErrorCorrection;
		}

		m_pAnimatedCharacter->SetParams( params );
		m_pAnimatedCharacter->ForceRefreshPhysicalColliderMode();
		m_pAnimatedCharacter->RequestPhysicalColliderMode(enabled ? eColliderMode_Disabled : eColliderMode_Undefined, eColliderModeLayer_Game, "Actor::LinkToEntity");
	}

  if (pLinked)
    pLinked->AttachChild(GetEntity(), 0);
  else
		GetEntity()->DetachThis(bKeepTransformOnDetach ? IEntity::ATTACHMENT_KEEP_TRANSFORMATION : 0, bKeepTransformOnDetach ? ENTITY_XFORM_USER : 0);

	return pLinked;
}

void CActor::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_HIDE:
		{
			g_pGame->GetTacticalManager()->RemoveEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Unit);
			SHUDEvent hudevent(eHUDEvent_RemoveEntity);
			hudevent.AddData(SHUDEventData((int)GetEntityId()));
			CHUD::CallEvent(hudevent);

		} // no break, because the ENTITY_EVENT_INVISIBLE should be executed as well
	case ENTITY_EVENT_INVISIBLE:
		{
			HideAllAttachments(true);
		}	
		break;
	case ENTITY_EVENT_UNHIDE:
		{
			g_pGame->GetTacticalManager()->AddEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Unit);
			SHUDEvent hudevent(eHUDEvent_AddEntity);
			hudevent.AddData(SHUDEventData((int)GetEntityId()));
			CHUD::CallEvent(hudevent);

		} // no break, because the ENTITY_EVENT_VISIBLE should be executed as well
	case ENTITY_EVENT_VISIBLE:
		{
			HideAllAttachments(false);
		}	
		break;
	case ENTITY_EVENT_START_GAME:
		UpdateAnimGraph( m_pAnimatedCharacter? m_pAnimatedCharacter->GetAnimationGraphState() : 0 );
		break;
  case ENTITY_EVENT_RESET:
    Reset(event.nParam[0]==1);
    break;
	case ENTITY_EVENT_ANIM_EVENT:
		{
			const AnimEventInstance* pAnimEvent = reinterpret_cast<const AnimEventInstance*>(event.nParam[0]);
			ICharacterInstance* pCharacter = reinterpret_cast<ICharacterInstance*>(event.nParam[1]);
			if (pAnimEvent && pCharacter)
			{
				const uint32 eventNameCRC = (pAnimEvent->m_EventName != NULL) ? gEnv->pSystem->GetCrc32Gen()->GetCRC32Lowercase(pAnimEvent->m_EventName) : 0;
				AnimationEvent(pCharacter, *pAnimEvent, eventNameCRC);
			}
		}
		break;
	case ENTITY_EVENT_DONE:
		{
			SetActorState(kActorState_beingDestroyed);

			if (IsClient())
			{
				BecomeRemotePlayer();
			}
		}
		break;
	case ENTITY_EVENT_PREPHYSICSUPDATE:
		{
			if (gEnv->bMultiplayer)
			{
				float dt = event.fParam[0];
				NetPrePhysicsUpdate(dt);
			}
		}
		break;

	case ENTITY_EVENT_TIMER:
		{
			if (event.nParam[0] == ITEM_SWITCH_TIMER_ID)
			{
				SActorStats* pActorStats = GetActorStats();
				assert(pActorStats);

				if (pActorStats->exchangeItemStats.switchingToItemID != 0)
				{
					SelectItem(pActorStats->exchangeItemStats.switchingToItemID, pActorStats->exchangeItemStats.keepHistory);
					pActorStats->exchangeItemStats.switchingToItemID = 0;
				}
			}
		}
		break;
  }  
}
static const float k_actorMinInterpolateSpeed=1.8f;	// min speed that any interpolation can take place at (get reduced later to account for dt)
static const float k_actorMaxInterpolateTime=1.f;       // max time over which any interpolation can take place, some prediction will continue after this
static const float k_actorMaxInterpolateDistSq=sqr(10.f);  // beyond this snap the actor
static const float k_actorMinInterpolateDistSq=sqr(0.01f); // under this just snap


void CActor::BecomeRemotePlayer()
{
	if(g_pGame->GetScreenEffects())
		g_pGame->GetScreenEffects()->SetClientActor(NULL);

	gEnv->pCryPak->DisableRuntimeFileAccess (false);
	CPerkIconData::SetLocalPlayerEntityId(0);


	m_isClient = false;
}

void CActor::NetPrePhysicsUpdate(float dt)
{
	IEntity *pEntity = GetEntity();
	IPhysicalEntity* pPhysics = pEntity->GetPhysics();
	if (pPhysics)
	{
		pe_status_dynamics physDyn;
		pe_status_pos physPos;
		pPhysics->GetStatus(&physDyn);
		Vec3 pos = pEntity->GetWorldPos();
		Vec3 vel = physDyn.v;

		static IEntityClass* sHologramClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Hologram");
		static IEntityClass* sDummyPlayerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("DummyPlayer");
		IEntityClass* myClass = pEntity->GetClass();

		if ((IsClient()) || (gEnv->bServer && (myClass == sHologramClass || myClass == sDummyPlayerClass)) || IsDead())	// the only objects that can return false for this at the moment are CPlayerHologram and CDummyPlayer
		{
			// this sets the position/velocity we want to send via the netserialise
			m_netDesiredLocation = pos;
			m_netDesiredSpeed = vel.GetLength();		// SF: I'd like to change this to something better
		}
		else if (m_netDoLerp)
		{
			// The player input net serialise deals with
			// the direction and desired velocity.
			// Fix any positional error here.
			// Lerping taken from CXP branch
			
			CTimeValue	curTime=gEnv->pTimer->GetFrameStartTime();

			if (m_netNewUpdate)
			{
				// New packet, calculate interpolator values
				m_netNewUpdate = false;
				m_netLastUpdate = curTime;
				
				if (m_netSetPosition==false) m_netCurrentLocation = pos;

				Vec3 displacement = m_netDesiredLocation - m_netCurrentLocation;
				float distSq = displacement.len2();

				if ((distSq>k_actorMaxInterpolateDistSq) || (distSq<k_actorMinInterpolateDistSq))
				{
					m_netCurrentLocation = m_netDesiredLocation;
					pos = m_netCurrentLocation;
					m_netLerpVelocity.zero();
					m_netLerpSpeed = 0.f;
					vel.zero();
				}
				else
				{
					// setup an interpolation
					float	lerpSpeed = m_netDesiredSpeed;
					float	dist = sqrtf(distSq);

					float curSpeed = m_netLerpSpeed; // Velocity.GetLength();	// GET RID OF THIS SQRT!

					// 	Dont allow a large decrease in speed
					lerpSpeed = (float)__fsel(curSpeed*0.8f - lerpSpeed, curSpeed*0.8f, lerpSpeed);
					lerpSpeed=max(lerpSpeed,k_actorMinInterpolateSpeed);

					float	timeToComplete=dist/lerpSpeed;

					timeToComplete = (float)__fsel(timeToComplete-k_actorMaxInterpolateTime, k_actorMaxInterpolateTime, timeToComplete);
					timeToComplete = (float)__fsel(timeToComplete-dt, timeToComplete, dt);

					m_netLerpVelocity = displacement/timeToComplete;
					m_netLerpSpeed = dist/timeToComplete;
				}

				// Always set position when an update is first received.
				m_netSetPosition = (distSq>=k_actorMinInterpolateDistSq);
			}

			float lerpTime = (curTime-m_netLastUpdate).GetSeconds();

			// perform interpolation
			// position is linear
			if (m_netSetPosition)
			{
				m_netCurrentLocation = m_netCurrentLocation + m_netLerpVelocity*dt;
				m_netCurrentLocation = m_netCurrentLocation + (m_netDesiredLocation - m_netCurrentLocation) * (dt/((dt+0.01f)*(1.f + 40.f*m_netDesiredSpeed*m_netDesiredSpeed)));
			}
#if 0
			IRenderAuxGeom* pRender = gEnv->pRenderer->GetIRenderAuxGeom();

			SAuxGeomRenderFlags flags = pRender->GetRenderFlags();
			SAuxGeomRenderFlags oldFlags = pRender->GetRenderFlags();
			flags.SetDepthWriteFlag(e_DepthWriteOff);
			flags.SetDepthTestFlag(e_DepthTestOff);
			pRender->SetRenderFlags(flags);
			pRender->DrawSphere(m_netDesiredLocation, 0.07f, ColorB(255,0,0,255));
			pRender->DrawSphere(m_netCurrentLocation, 0.05f, ColorB(255,255,255,255));
			pRender->SetRenderFlags(oldFlags);

			g_pGame->GetIGameFramework()->GetIPersistantDebug()->Begin("INTERPOLATION TRAIL", false);
			g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(m_netDesiredLocation, 0.04f, ColorF(1.0f,0.0f,0.0f,0.75f), 30.f);
			g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(m_netCurrentLocation, 0.04f, ColorF(1.0f,1.0f,1.0f,0.75f), 30.f);
#endif
			bool isVisible = GetEntity()->GetCharacter(0)->IsCharacterVisible() != 0;
			if (m_netSetPosition && (!g_pGameCVars->pl_velocityBasedInterpolation || !isVisible))
			{
				pEntity->SetPos(m_netCurrentLocation);	// This will invalidate the physics, and also update the physicsEntity matrix
			}

			if (m_netSetPosition && (m_netDesiredLocation - m_netCurrentLocation).Dot(m_netLerpVelocity) < 0.f )
			{
				// We have reached the target plane, so stop setting position
				// NB: note < 0.f rather than <= 0.f so that teleporting works, since that
				// sets the velocity to zero which gives a dot product of zero.
				m_netSetPosition = false;
			}
		}
	}
}

void CActor::Update(SEntityUpdateContext& ctx, int slot)
{
	bool isHidden = GetEntity()->IsHidden();

#if !defined(_RELEASE)
	if(g_pGameCVars && g_pGameCVars->g_displayDbgText_actorState)
	{
		IItem* currentItem = GetCurrentItem();
		WATCH_ACTOR_STATE("(team %d, health %d/%d%s%s%s) is %s (%s)", m_teamId, GetHealth(), GetMaxHealth(), isHidden ? ", $6HIDDEN$o" : "", GetActorStats()->isRagDoll ? ", $5RAGDOLL$o" : "", m_ragdollOnCollision ? ", become ragdoll when hit" : "", s_actorStateNames[m_actorState], currentItem ? currentItem->GetEntity()->GetClass()->GetName() : "no item");
	}
#endif

	if (isHidden && !(GetEntity()->GetFlags() & ENTITY_FLAG_UPDATE_HIDDEN))
		return;

	m_damageEffectController.UpdateEffects(ctx);

	if (m_sleepTimer>0.0f && gEnv->bServer)
	{
		UpdateSleep(ctx.fFrameTime);
	}

	if (GetHealth() > 0)
	{

		// Only update stance for alive characters. Dead characters never request any stance changes
		// but if a different stance is requested for whatever reason (currently it happens after QL)
		// and the animation graph has different death animations for different stances (like for the
		// Hunter currently) then some other death animation may play again to better match the state.
		UpdateStance();
	}
	
	UpdateAnimGraph( m_pAnimatedCharacter?m_pAnimatedCharacter->GetAnimationGraphState():NULL );

	//
	// get stats table
	if (!m_actorStats)
	{
		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		if (pScriptTable)
			pScriptTable->GetValue("actorStats", m_actorStats);
	}
	UpdateScriptStats(m_actorStats);

	if (m_ragdollOnCollision)
	{
		UpdateRagdollOnCollision();
	}

	if(gEnv->bServer)
	{
		m_telemetry.Update();
	}
}

void CActor::UpdateSleep(float fFrameTime)
{
	float fNewSleepTimer = m_sleepTimer;

	pe_status_dynamics dynStat;
	dynStat.submergedFraction = 0;
	if (GetLinkedVehicle())
	{
		fNewSleepTimer = m_sleepTimer - fFrameTime;
	}
	else
	{
		IPhysicalEntity* pEnt = GetEntity()->GetPhysics();
		dynStat.nContacts = 0;
		if (!pEnt || !pEnt->GetStatus(&dynStat) || dynStat.nContacts>2 || dynStat.energy<100.0f || dynStat.submergedFraction>0.1f)
		{
			fNewSleepTimer = m_sleepTimer - fFrameTime;
		}

		if (dynStat.nContacts>=4 && dynStat.energy<dynStat.mass*sqr(0.3f) && fNewSleepTimer < m_sleepTimerOrg - 1.0f)
		{
			if (m_sleepTimerOrg > 10.0f) // tranquilized mode
			{
				if (fNewSleepTimer < m_sleepTimerOrg - 2.0f)
				{
					ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0);
					if (pCharacter && pCharacter->GetISkeletonPose())
						pCharacter->GetISkeletonPose()->GoLimp();
				}
			} 
			else
			{
				fNewSleepTimer = 0.0f;
			}					
		}
	}

	m_sleepTimer = fNewSleepTimer;

	// StandUp uses m_sleepTimer to know if it should wake up the entity
	if (m_sleepTimer <= 0.0f)
	{
		m_sleepTimer = 0.0f;

		if (IsClient())
		{
			StandUp();
		}
		else // Not client (Fake death handles standup separately)
		{
			if (dynStat.submergedFraction>0.5f)
			{
				Vec3 cp=GetEntity()->GetWorldPos();
				float waterLevel = gEnv->p3DEngine->GetWaterLevel(&cp);
				float terrainLevel = gEnv->p3DEngine->GetTerrainElevation(cp.x,cp.y);
				//Prevent AI dying in shallow water
				if(waterLevel>(terrainLevel+0.5f))
				{
					SetHealth(0);
					CreateScriptEvent("kill",0);
					CrapDollize();
					// drop item is not managed inside kill event
					IItem* currentItem = GetCurrentItem();
					if(currentItem)
						DropItem(currentItem->GetEntityId());
				}
				else 
					StandUp();
			}
			else
				StandUp();
		}
	}
}

void CActor::UpdateScriptStats(SmartScriptTable &rTable)
{
	CScriptSetGetChain stats(rTable);
	stats.SetValue("stance",m_stance);
	//stats.SetValue("thirdPerson",IsThirdPerson());

	SActorStats *pStats = GetActorStats();
	if (pStats)
	{
		//REUSE_VECTOR(rTable, "velocity", pStats->velocity);
	
		stats.SetValue("inAir",pStats->inAir);
		stats.SetValue("onGround",pStats->onGround);

		//stats.SetValue("inWater",pStats->inWater);
		//pStats->headUnderWater.SetDirtyValue(stats, "headUnderWater");
		//stats.SetValue("waterLevel",pStats->waterLevel);
		//stats.SetValue("bottomDepth",pStats->bottomDepth);

		stats.SetValue("flatSpeed",pStats->speedFlat);
		//stats.SetValue("speedModule",pStats->speed);

		stats.SetValue("godMode",IsGod());
		stats.SetValue("inFiring",pStats->inFiring);		
		pStats->isHidden.SetDirtyValue(stats, "isHidden");
	}
}

bool CActor::UpdateStance()
{
	if (m_stance != m_desiredStance)
	{
		// If character is animated, postpone stance change until state transition is finished (i.e. in steady state).
		if ((m_pAnimatedCharacter != NULL) && m_pAnimatedCharacter->InStanceTransition())
			return false;

		if (!TrySetStance(m_desiredStance))
			return false;

		EStance newStance = m_desiredStance;
		EStance oldStance = m_stance;
		m_stance = newStance;
		OnStanceChanged(newStance, oldStance);

		// Request new animation stance.
		// AnimatedCharacter has it's own understanding of stance, which might be in conflict.
		// Ideally the stance should be maintained only in one place. Currently the Actor (gameplay) rules.
		if (m_pAnimatedCharacter != NULL)
			m_pAnimatedCharacter->RequestStance(m_stance, GetStanceInfo(m_stance)->name);

		IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();
		if (pPhysEnt != NULL)
		{
			pe_action_awake aa;
			aa.bAwake = 1;
			pPhysEnt->Action(&aa);
		}
	}

	return true;
}

//------------------------------------------------------

bool CActor::TrySetStance(EStance stance)
{
	//  if (stance == STANCE_NULL)
	//	  return true;

	IPhysicalEntity *pPhysEnt = GetEntity()->GetPhysics();
	int result = 0;
	if (pPhysEnt)
	{
		const SStanceInfo *sInfo = GetStanceInfo(stance);
#ifdef _DEBUG
		if (sInfo == &m_defaultStance)
		{
			CryLogAlways("%s trying to set undefined stance (%d).\nPlease update the stances tables in the Lua script accordingly.", 
				GetEntity()->GetName(), stance);
		}
#endif

		pe_player_dimensions playerDim;
		playerDim.heightEye = 0.0f;
		playerDim.heightCollider = sInfo->heightCollider;
		playerDim.sizeCollider = sInfo->size;
		playerDim.heightPivot = sInfo->heightPivot;
		playerDim.maxUnproj = max(0.0f,sInfo->heightPivot);
		playerDim.bUseCapsule = sInfo->useCapsule;

		result = pPhysEnt->SetParams(&playerDim);
	}

	return (result != 0);
}



void CActor::SetStats(SmartScriptTable &rTable)
{
	SActorStats *pStats = GetActorStats();
	if (pStats)
	{
		rTable->GetValue("inFiring",pStats->inFiring);
	}
}

//------------------------------------------------------------------------
void CActor::OnAction(const ActionId& actionId, int activationMode, float value)
{
	IItem *pItem = GetCurrentItem();
	if (pItem)
		pItem->OnAction(GetGameObject()->GetEntityId(), actionId, activationMode, value);
}

//------------------------------------------------------------------------
void CActor::CreateScriptEvent(const char *event,float value,const char *str)
{
	IEntity *pEntity = GetEntity(); 
  IScriptTable* pScriptTable = pEntity->GetScriptTable();

	if (pScriptTable)
	{
		HSCRIPTFUNCTION scriptEvent(NULL);	
		pScriptTable->GetValue("ScriptEvent", scriptEvent);

		if (scriptEvent)
			Script::Call(gEnv->pScriptSystem,scriptEvent,pScriptTable,event,value,str);

		gEnv->pScriptSystem->ReleaseFunc(scriptEvent);
	}
}

bool CActor::CreateCodeEvent(SmartScriptTable &rTable)
{
	const char *event = NULL;
  if (!rTable->GetValue("event",event))
    return false;

	if (!strcmp(event,"grabObject"))
	{
		if (!m_pGrabHandler)
			CreateGrabHanlder();

		if (m_pGrabHandler)
			return m_pGrabHandler->SetGrab(rTable);
	}
	else if (!strcmp(event,"dropObject"))
	{
		if (m_pGrabHandler)
			return m_pGrabHandler->SetDrop(rTable);
	}
	else if (!strcmp(event,"replaceMaterial"))
	{
		const char *strMat = NULL;
		rTable->GetValue("material",strMat);
		ReplaceMaterial(strMat);

    int cloaked = 0;
    if (rTable->GetValue("cloak", cloaked))
      OnCloaked(cloaked!=0);

    return true;
	}


	return false;
}

void CActor::AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC)
{
	// Luciano - to do: use a proper customizable parameter type for CreateScriptEvent, now only float are allowed
	CreateScriptEvent("animationevent",(float)atof(event.m_CustomParameter),event.m_EventName);
}


void CActor::RequestFacialExpression(const char* pExpressionName /* = NULL */) 
{
	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	IFacialInstance* pFacialInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
	IFacialAnimSequence* pSequence = (pFacialInstance ? pFacialInstance->LoadSequence(pExpressionName) : 0);
	if (pFacialInstance)
		pFacialInstance->PlaySequence(pSequence, eFacialSequenceLayer_AIExpression);
}

void CActor::PrecacheFacialExpression(const char* pExpressionName)
{
	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	IFacialInstance* pFacialInstance = (pCharacter ? pCharacter->GetFacialInstance() : 0);
	if (pFacialInstance)
		pFacialInstance->PrecacheFacialExpression(pExpressionName);
}

void CActor::FullSerialize( TSerialize ser )
{	
	assert(ser.GetSerializationTarget() != eST_Network);

	ser.BeginGroup("CActor");
	int oldHealth = (int)m_health;
	ser.Value("health", m_health);		
	ser.Value("maxHealth", m_maxHealth);
	if(ser.IsReading() && oldHealth <= 0 && m_health > 0)
		Revive(kRFR_FromInit);
	if(ser.IsReading() && m_health <= 0 && oldHealth > 0)
	{
		Kill();
		RagDollize(false);
	}

	ser.Value("sleepTimer", m_sleepTimer);
	if(IPhysicalEntity *pPhys = GetEntity()->GetPhysics())
	{
		if(ser.IsReading() && m_sleepTimer)
		{
			if(pPhys->GetType() != PE_ARTICULATED)
			{
				float sleep = m_sleepTimer;
				m_sleepTimer = 0.0f;
				Fall();
				m_sleepTimer = sleep; //was reset
			}	
		}
	}

	ser.EndGroup();

	m_linkStats.Serialize(ser);

	// NOTE Okt 13, 2007: <pvl> if we're reading and we already have a handler
	// let's get rid of it now.  If a handler is in the save we're loading then
	// 'serializeGrab' will be true and with m_pGrabHandler==0 it will get recreated.
	if (ser.IsReading () && m_pGrabHandler)
	{
		SAFE_DELETE (m_pGrabHandler);
	}

	bool serializeGrab(m_pGrabHandler?true:false);
	ser.Value("serializeGrab", serializeGrab);

	m_serializeLostHelmet = m_lostHelmet;
	ser.Value("LostHelmet", m_serializeLostHelmet);
	m_serializelostHelmetObj = m_lostHelmetObj;
	ser.Value("LostHelmetObj", m_serializelostHelmetObj);
	ser.Value("LostHelmetAttachmentPosition", m_lostHelmetPos);
	ser.Value("LostHelmetMaterial", m_lostHelmetMaterial);

	//FIXME:not sure how safe this is
	if (ser.IsReading() && serializeGrab && !m_pGrabHandler)
		CreateGrabHanlder();

	if (m_pGrabHandler)
		m_pGrabHandler->Serialize(ser);

	m_telemetry.Serialize(ser);
}

void CActor::PostSerialize()
{
	//helmet serialization
	if(m_serializeLostHelmet != m_lostHelmet) //sync helmet status
	{
		if(m_serializeLostHelmet && !m_lostHelmet)
		{
			if(IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_serializeLostHelmet)) //helmet lost AND on the character (character was reset)
			{
				if(IMaterial *pMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_lostHelmetMaterial.c_str()))
					pEntity->SetMaterial(pMat);

				//get rid of new helmet attachment
				ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
				IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
				IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_lostHelmetPos.c_str());
				if(pAttachment && pAttachment->GetIAttachmentObject())
					pAttachment->ClearBinding();

				m_lostHelmet = m_serializeLostHelmet;
			}
			else
				LooseHelmet();
		}
		else if(m_lostHelmet && !m_serializeLostHelmet)
			ResetHelmetAttachment();
	}
	else if(m_lostHelmet)	//reset material for dropped helmets
	{
		if(IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_lostHelmet))
		{
			if(IMaterial *pMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_lostHelmetMaterial.c_str()))
				pEntity->SetMaterial(pMat);
		}
	}

  if(ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0))
    if(ISkeletonPose *pSkelPose = pCharacter->GetISkeletonPose())
      pSkelPose->SetForceSkeletonUpdate(2);

	m_telemetry.PostSerialize();
}

void CActor::SetChannelId(uint16 id)
{
}

void CActor::SetHealth( int health )
{
	if (health <= 0)
	{
		if (IsGod() > 0) // handled in CPlayer
			return;

		//TODO: Remove this on _RELEASE?
		if (IsClient() == false)
		{
			if (gEnv->pAISystem && GetEntity()->GetAI())
				gEnv->pAISystem->DebugReportDeath(GetEntity()->GetAI());
		}

		SActorStats *pStats = GetActorStats();

		if (pStats && pStats->isRagDoll)
		{
			CrapDollize();
		}
	}

	if (gEnv->bServer && !gEnv->bClient && gEnv->pSystem->IsDedicated() && g_pGameCVars->sv_pacifist && health < m_health)
	{
		return;
	}
	
	int prevHealth = static_cast<int>(m_health);

#ifdef _RELEASE
	m_health = static_cast<float>(min(health, m_maxHealth));
#else
	float fNewHealth = static_cast<float>(min(health, m_maxHealth));
	m_health = (float)__fsel(g_pGameCVars->g_maximumDamage, max(m_health - g_pGameCVars->g_maximumDamage, fNewHealth), fNewHealth);
#endif

	if (m_health!=prevHealth && m_health<=0)
	{
		IItem *pItem = GetCurrentItem();
		IWeapon *pWeapon = pItem ? pItem->GetIWeapon() : NULL;

		if (pWeapon)
			pWeapon->StopFire();
	}
	//m_pGameplayRecorder->Event(GetEntity(), GameplayEvent(eGE_HealthChanged, 0, m_health, 0));

}

void CActor::DamageInfo(EntityId shooterID, EntityId weaponID, IEntityClass *pProjectileClass, float damage, const char *damageType, const Vec3 hitDirection)
{
}


void InitializeHitRecoilGameEffect()
{
}


void CActor::SetMaxHealth( int maxHealth )
{
	m_maxHealth = maxHealth;
	SetHealth(maxHealth);
}

void CActor::Kill()
{
	SetActorState(kActorState_dying);

	if (m_pAnimatedCharacter)
		m_pAnimatedCharacter->SetParams( m_pAnimatedCharacter->GetParams().ModifyFlags(0,eACF_EnableMovementProcessing));

	m_sleepTimer = 0.0f;

	if (IVehicle* pVehicle = GetLinkedVehicle())
	{
		if (IVehicleSeat* pVehicleSeat = pVehicle->GetSeatForPassenger(GetEntityId()))
			pVehicleSeat->OnPassengerDeath();
	}

	g_pGame->GetTacticalManager()->RemoveEntity(GetEntityId(), CTacticalManager::eTacticalEntity_Unit);
	if(!gEnv->bMultiplayer)
	{
		SHUDEvent hudevent(eHUDEvent_RemoveEntity);
		hudevent.AddData(SHUDEventData((int)GetEntityId()));
		CHUD::CallEvent(hudevent);
	}

	RequestFacialExpression( NULL );

	if(gEnv->bMultiplayer)
	{
		IInventory *pInventory = GetInventory();
		if(gEnv->bServer)
		{
			pInventory->Destroy();	// Destroy also calls clear
		}
		else
		{
			pInventory->Clear();
		}
	}
}

void CActor::SetParams(SmartScriptTable &rTable,bool resetFirst)
{
	SmartScriptTable animTable;

	if (rTable->GetValue("stance",animTable))
	{
		IScriptTable::Iterator iter = animTable->BeginIteration();
		int stance;

		while(animTable->MoveNext(iter))
		{
			SmartScriptTable stanceTable;

			if (iter.value.CopyTo(stanceTable))
			{
				if (stanceTable->GetValue("stanceId",stance))
				{
					if (stance == STANCE_NULL)
						break;

					SStanceInfo sInfo;
					const char *name;
					{
						CScriptSetGetChain stanceTableChain(stanceTable);
						stanceTableChain.GetValue("normalSpeed",sInfo.normalSpeed);
						stanceTableChain.GetValue("maxSpeed",sInfo.maxSpeed);
						stanceTableChain.GetValue("heightCollider",sInfo.heightCollider);
						stanceTableChain.GetValue("heightPivot",sInfo.heightPivot);
						stanceTableChain.GetValue("size",sInfo.size);
						stanceTableChain.GetValue("modelOffset",sInfo.modelOffset);
	
						stanceTableChain.GetValue("viewOffset",sInfo.viewOffset);
						sInfo.leanLeftViewOffset = sInfo.leanRightViewOffset = sInfo.viewOffset;
						stanceTableChain.GetValue("leanLeftViewOffset",sInfo.leanLeftViewOffset);
						stanceTableChain.GetValue("leanRightViewOffset",sInfo.leanRightViewOffset);

						sInfo.peekOverViewOffset = sInfo.viewOffset;
						stanceTableChain.GetValue("peekOverViewOffset", sInfo.peekOverViewOffset);
						sInfo.peekOverWeaponOffset = sInfo.weaponOffset;
						stanceTableChain.GetValue("peekOverWeaponOffset", sInfo.peekOverWeaponOffset);

						stanceTableChain.GetValue("viewDownYMod",sInfo.viewDownYMod);

						stanceTableChain.GetValue("weaponOffset",sInfo.weaponOffset);
							sInfo.leanLeftWeaponOffset = sInfo.leanRightWeaponOffset = sInfo.weaponOffset;
						stanceTableChain.GetValue("leanLeftWeaponOffset",sInfo.leanLeftWeaponOffset);
							stanceTableChain.GetValue("leanRightWeaponOffset",sInfo.leanRightWeaponOffset);
						stanceTableChain.GetValue("useCapsule",sInfo.useCapsule);
						if (stanceTableChain.GetValue("name",name))
						{
							cry_strncpy(sInfo.name,name, sizeof(sInfo.name));
						}
					}
					SetupStance((EStance)stance,&sInfo);
				}
			}
		}

		animTable->EndIteration(iter);
	}

	SActorParams *pParams(GetActorParams());
	if (pParams)
	{
		rTable->GetValue("maxGrabMass",pParams->maxGrabMass);
		rTable->GetValue("maxGrabVolume",pParams->maxGrabVolume);
		rTable->GetValue("nanoSuitActive",pParams->nanoSuitActive);
		rTable->GetValue("meeleHitRagdollImpulseScale", pParams->meeleHitRagdollImpulseScale);
		if (rTable->GetValue("aimFOV",pParams->aimFOVRadians))
			pParams->aimFOVRadians = DEG2RAD(pParams->aimFOVRadians);
		if (rTable->GetValue("lookFOV",pParams->lookFOVRadians))
			pParams->lookFOVRadians = DEG2RAD(pParams->lookFOVRadians);
		if (rTable->GetValue("lookInVehicleFOV",pParams->lookInVehicleFOVRadians))
			pParams->lookInVehicleFOVRadians = DEG2RAD(pParams->lookInVehicleFOVRadians);
		else
			pParams->lookInVehicleFOVRadians = pParams->lookFOVRadians;
		if (rTable->GetValue("maxLookAimAngle",pParams->maxLookAimAngleRadians))
			pParams->maxLookAimAngleRadians = DEG2RAD(pParams->maxLookAimAngleRadians);

		// Physic-related
		SmartScriptTable physicsParams;
		IScriptTable* pEntityScriptTable = GetEntity()->GetScriptTable();
		if(pEntityScriptTable && pEntityScriptTable->GetValue("physicsParams", physicsParams))
		{
			physicsParams->GetValue("fallNPlayStiffness_scale", pParams->fallNPlayStiffness_scale);
		}

		//PROTOTYPE STUFF (Yellow NK can disable cloak)
		SmartScriptTable props;
		SmartScriptTable propsPerception;
		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		if(pScriptTable && pScriptTable->GetValue("Properties", props))
		{
			if(props->GetValue("Perception", propsPerception))
			{
				propsPerception->GetValue("bCanDisablePlayerCloak", pParams->canDisablePlayerCloak);
			}
		}
		//-----------------------------------------------------
	}
}

bool CActor::IsClient() const
{
	return m_isClient;
}

bool CActor::IsPlayer() const
{
	static IEntityClass* sPlayerClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Player");
	return GetEntity()->GetClass() == sPlayerClass;
}


bool CActor::SetAspectProfile( EEntityAspects aspect, uint8 profile )
{
	bool res(false);

	if (aspect == eEA_Physics)
	{
		if (m_currentPhysProfile==profile && !gEnv->pSystem->IsSerializingFile()) //rephysicalize when loading savegame
			return true;

		switch (profile)
		{
		case eAP_NotPhysicalized:
			{
				SEntityPhysicalizeParams params;
				params.type = PE_NONE;
				GetEntity()->Physicalize(params);
			}
			res=true;
			break;
		case eAP_Spectator:
		case eAP_Alive:
			{
				// if we were asleep, we just want to wakeup
				if (profile==eAP_Alive && (m_currentPhysProfile==eAP_Sleep))
				{
					ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0);
					if (pCharacter && pCharacter->GetISkeletonAnim())
					{
						IPhysicalEntity *pPhysicalEntity=0;
						Matrix34 delta(IDENTITY);

						pCharacter->GetISkeletonPose()->StandUp(GetEntity()->GetWorldTM(), false, pPhysicalEntity, delta);

						if (pPhysicalEntity)
						{
							IEntityPhysicalProxy *pPhysicsProxy=static_cast<IEntityPhysicalProxy *>(GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS));
							if (pPhysicsProxy)
							{
								GetEntity()->SetWorldTM(delta);
								pPhysicsProxy->AssignPhysicalEntity(pPhysicalEntity);
							}
						}
						m_pAnimatedCharacter->ForceTeleportAnimationToEntity();
						m_pAnimatedCharacter->ForceRefreshPhysicalColliderMode();

						PhysicalizeBodyDamage();
					}
				}
				else
				{
					Physicalize(STANCE_NULL);

					if (profile==eAP_Spectator)
					{
						if (ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0))
							pCharacter->GetISkeletonPose()->DestroyCharacterPhysics(1);
						m_pAnimatedCharacter->ForceRefreshPhysicalColliderMode();
						m_pAnimatedCharacter->RequestPhysicalColliderMode( eColliderMode_Spectator, eColliderModeLayer_Game, "Actor::SetAspectProfile");
					}
					else if (profile==eAP_Alive)
					{
						if (m_currentPhysProfile==eAP_Spectator)
						{
							m_pAnimatedCharacter->RequestPhysicalColliderMode( eColliderMode_Undefined, eColliderModeLayer_Game, "Actor::SetAspectProfile");
							if (IPhysicalEntity *pPhysics=GetEntity()->GetPhysics())
							{
								if (ICharacterInstance *pCharacter=GetEntity()->GetCharacter(0))
								{
									pCharacter->GetISkeletonPose()->DestroyCharacterPhysics(2);

									if (IPhysicalEntity *pCharPhysics=pCharacter->GetISkeletonPose()->GetCharacterPhysics())
									{
										pe_params_articulated_body body;
										body.pHost=pPhysics;
										pCharPhysics->SetParams(&body);
									}
								}
							}
						}
					}
				}
			}
			res=true;
			break;
		case eAP_Linked:
			// make sure we are alive, for when we transition from ragdoll to linked...
			if (!GetEntity()->GetPhysics() || GetEntity()->GetPhysics()->GetType()!=PE_LIVING)
				Physicalize();
			res=true;
			break;
		case eAP_Sleep:
			RagDollize(true);
			res=true;
			break;
		case eAP_Ragdoll:
			// killed while sleeping?
			if (m_currentPhysProfile==eAP_Sleep) 
				GoLimp();
			else
				RagDollize(false);
			res=true;
			break;
		}

		IPhysicalEntity *pPE=GetEntity()->GetPhysics();

		if (res)
			ProfileChanged(profile);

		m_currentPhysProfile = profile;
	}

	return res;
}

void CActor::ProfileChanged( uint8 newProfile )
{
	//inform scripts when the profile changes
	switch(newProfile)
	{
	case eAP_Alive:
		CreateScriptEvent("profileChanged",0,"alive");
		break;
	case eAP_Ragdoll:
		CreateScriptEvent("profileChanged",0,"ragdoll");
		break;
	}
}

static void actorSerialisePhysicsDyn(TSerialize ser, uint8& physCounter, Vec3& pos, float& speed, bool& needsUpdate)
{
	ser.Value("physcounter", physCounter, 'ui4');
	ser.Value("playerpos", pos, 'wrld');
	ser.Value("playervel", speed, 'plSp');
	needsUpdate = needsUpdate || ser.IsReading();
}

static bool allowPhysicsUpdate(uint8 newCounter, uint8 oldCounter)
{
	uint8 x = (newCounter - oldCounter)&(PHYSICS_COUNTER_MAX - 1);
  return (x <  (PHYSICS_COUNTER_MAX>>1));
}

bool CActor::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags )
{
	NET_PROFILE_SCOPE("Actor", ser.IsReading());

	NET_PROFILE_BEGIN("DamageEffects", ser.IsReading());
	m_damageEffectController.NetSerialiseEffects(ser, aspect);
	NET_PROFILE_END();

	if (aspect == eEA_Physics)
	{
		NET_PROFILE_SCOPE("Physics", ser.IsReading());

		if (ser.IsReading()) m_netDoLerp = false;

		pe_type type = PE_NONE;
		switch (profile)
		{
		case eAP_NotPhysicalized:
			type = PE_NONE;
			break;
		case eAP_Spectator:
			type = PE_LIVING;
			break;
		case eAP_Alive:
			type = PE_LIVING;
			break;
		case eAP_Sleep:
		case eAP_Ragdoll:
			type = PE_NONE;
			break;
		case eAP_Linked: 	//if actor is attached to a vehicle - don't serialize actor physics additionally
			return true;
			break;
		default:
			return false;
		}

		// TODO: remove this when craig fixes it in the network system
		if (profile==eAP_Spectator)
		{
			int x=0;	
			ser.Value("unused", x, 'skip');
		}
		else if (profile==eAP_Sleep)
		{
			int x=0;	
			ser.Value("unused1", x, 'skip');
			ser.Value("unused2", x, 'skip');
		}

		if (type == PE_NONE)
		{
			return true;
		}

		IEntityPhysicalProxy * pEPP = (IEntityPhysicalProxy *) GetEntity()->GetProxy(ENTITY_PROXY_PHYSICS);
		if (ser.IsWriting())
		{
			if (!pEPP || !pEPP->GetPhysicalEntity() || pEPP->GetPhysicalEntity()->GetType() != type)
			{
				if(type!=PE_LIVING)
				{
					gEnv->pPhysicalWorld->SerializeGarbageTypedSnapshot( ser, type, 0 );
				}
				else
				{
					Vec3 zero(0.f,0.f,0.f);
					float z = 0.f;
					uint8 counter = m_netPhysCounter;
					actorSerialisePhysicsDyn(ser, counter, zero, z, m_netNewUpdate);
				}
				return true;
			}
		}
		else if (!pEPP)
		{
			return false;
		}

		if(type!=PE_LIVING)
		{
			pEPP->SerializeTyped( ser, type, pflags );
		}
		else
		{
			Vec3 desiredLocation = m_netDesiredLocation;
			float desiredSpeed = m_netDesiredSpeed;
			uint8 physCounter = m_netPhysCounter;
			bool newUpdate = m_netNewUpdate;

			actorSerialisePhysicsDyn(ser, physCounter, desiredLocation, desiredSpeed, newUpdate);
		
			if (ser.IsReading())
			{
#if 0
				const float XPOS = 500.0f;
				const float YPOS = 150.0f;
				const float COLOUR[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				gEnv->pRenderer->Draw2dLabel(XPOS, YPOS, 2.0f, COLOUR, false, "NetSerialise: DesSpeed: %.2f PhysCount: %d", desiredSpeed, physCounter);
#endif

				// check counter
				if(m_netFirstPhysUpdate || allowPhysicsUpdate(physCounter, m_netPhysCounter))
				{
					m_netDesiredLocation = desiredLocation;
					m_netDesiredSpeed = desiredSpeed;
					m_netPhysCounter = physCounter;
					m_netNewUpdate = newUpdate;
					m_netFirstPhysUpdate = false;

					float distSq = (m_netDesiredLocation - m_netCurrentLocation).GetLengthSquared();
					if (distSq>k_actorMaxInterpolateDistSq)
					{
						CCCPOINT(PlayerState_NetSerializeReadNewPosition_DoSnap);

						// We need to snap immediately
						m_netCurrentLocation = m_netDesiredLocation;
						m_netLerpVelocity.zero();
						GetEntity()->SetPos(m_netCurrentLocation);	// This will invalidate the physics, and also update the physicsEntity matrix
					}
					else
					{
						m_netDoLerp = true;
					}
				}
			}
		}
	}

	return true;
}


void CActor::ProceduralRecoil( float duration, float strength )
{
	if(m_pAnimatedCharacter)
		m_pAnimatedCharacter->TriggerRecoil(duration, strength);
}

void CActor::HandleEvent( const SGameObjectEvent& event )
{
	if (event.event == eCGE_OnShoot)
	{
		SActorStats *pStats = GetActorStats();
		if (pStats)
			pStats->inFiring = 10.0f;
	}
	else if ((event.event == eCGE_Ragdoll) || (event.event == eCGE_TurnRagdoll))
	{
		GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
	}
	else if (event.event == eCGE_EnableFallAndPlay)
	{
		RagDollize(true);

		SActorStats* pStats = GetActorStats();
		if (pStats)
			pStats->isInFallNPlay = true;
	}
	else if (event.event == eCGE_DisableFallAndPlay)
	{
		SActorStats* pStats = GetActorStats();
		if (pStats)
			pStats->isInFallNPlay = false;
	}
	else if (event.event == eCGE_EnablePhysicalCollider)
	{
		m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Undefined, eColliderModeLayer_Game, "Actor::HandleEvent");
	}
	else if (event.event == eCGE_DisablePhysicalCollider)
	{
		m_pAnimatedCharacter->RequestPhysicalColliderMode(eColliderMode_Disabled, eColliderModeLayer_Game, "Actor::HandleEvent");
	}
	else if (event.event == eCGE_RebindAnimGraphInputs)
	{
		BindInputs( m_pAnimatedCharacter?m_pAnimatedCharacter->GetAnimationGraphState():NULL );
	}
	else if (event.event == eCGE_BeginReloadLoop)
	{
		SetAnimationInput( "Action", "reload" );
	}
	else if (event.event == eCGE_EndReloadLoop)
	{
		SetAnimationInput( "Action", "idle" );
	}
	else if (event.event == eGFE_BecomeLocalPlayer)
	{
		gEnv->pCryPak->DisableRuntimeFileAccess (true);
		if (!(gEnv->bHostMigrating && gEnv->bServer))
		{
			CPerkIconData::SetLocalPlayerEntityId(GetEntityId());
		}

		m_isClient = true;
		GetGameObject()->EnablePrePhysicsUpdate( ePPU_Always );

		// always update client's character
		if (ICharacterInstance * pCharacter = GetEntity()->GetCharacter(0))
			pCharacter->SetFlags(pCharacter->GetFlags() | CS_FLAG_UPDATE_ALWAYS);

		// We get this after we've been revived, so start recording now.
		if (g_pGame->GetRecordingSystem())
		{
			g_pGame->GetRecordingSystem()->StartRecording();
		}
	}
}

void CActor::BindInputs( IAnimationGraphState * pAGState )
{
	if (pAGState)
	{
		m_inputHealth = pAGState->GetInputId("Health");
		m_inputStance = pAGState->GetInputId("Stance");
		m_inputFiring = pAGState->GetInputId("Firing");
		m_inputSignal = pAGState->GetInputId("Signal");
		m_inputAction = pAGState->GetInputId("Action");

		m_pMovementController->BindInputs( pAGState );
	}
}

void CActor::ResetAnimGraph()
{
	SetStance(STANCE_RELAXED);
}

void CActor::UpdateAnimGraph( IAnimationGraphState * pState )
{
	if (pState)
	{
		pState->SetInput( m_inputHealth, max(m_health, 0.0f));

		SActorStats *pStats = GetActorStats();
		if (pStats)
		{
			pState->SetInput(m_inputFiring, pStats->inFiring);
		}
	}
}

void CActor::QueueAnimationState( const char * state )
{
	if (m_pAnimatedCharacter)
		m_pAnimatedCharacter->PushForcedState( state );
}

int CActor::GetBoneID(int ID,int slot) const
{
	CRY_ASSERT((ID >= 0) && (ID < BONE_ID_NUM));
	if((ID >= 0) && (ID < BONE_ID_NUM))
	{
	if (m_boneIDs[ID]>=0)
		return m_boneIDs[ID];

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (pCharacter)
	{
		m_boneIDs[ID] = pCharacter->GetISkeletonPose()->GetJointIDByName(s_bonestr[ID]);
		return m_boneIDs[ID];
	}
	}

	return -1;
}

Vec3 CActor::GetLocalEyePos(int slot) const
{

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (pCharacter)
	{
		int id_camera = GetBoneID(BONE_CAMERA);
		if (id_camera>-1)
		{
			Vec3 camera = pCharacter->GetISkeletonPose()->GetAbsJointByID(id_camera).t;
			if (camera.IsValid())
				return camera;
		}

		int id_right = GetBoneID(BONE_EYE_R);
		int id_left = GetBoneID(BONE_EYE_L);
		if (id_right>-1 && id_left>-1)
		{
			Vec3 reye = pCharacter->GetISkeletonPose()->GetAbsJointByID(id_right).t;
			Vec3 leye = pCharacter->GetISkeletonPose()->GetAbsJointByID(id_left).t;
			if (reye.IsValid() && leye.IsValid())
				return (reye+leye)*0.5f;
		}
	}
	return GetStanceInfo(m_stance)->viewOffset;
}

QuatT CActor::GetCameraTran(int slot) const
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);
	if (pCharacter)
	{
		int id_camera = GetBoneID(BONE_CAMERA);
		if (id_camera>-1)
		{
			QuatT camera = pCharacter->GetISkeletonPose()->GetAbsJointByID(id_camera);
			CRY_ASSERT(camera.IsValid());

			return camera;
		}
	}

	CRY_ASSERT(false);
	return QuatT();
}

Vec3 CActor::GetLocalEyePos2(int slot) const
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(slot);

	if (pCharacter)
	{
		int id_pelvis = GetBoneID(1); //how the hell do I get the pelvis ID in this system
		int id_right = GetBoneID(BONE_EYE_R);
		int id_left = GetBoneID(BONE_EYE_L);

		Vec3 PelvisPos=Vec3(ZERO);
		if (id_pelvis>-1)
		//	PelvisPos = Quat::CreateRotationZ(-gf_PI/2)*pCharacter->GetISkeleton()->GetAbsJointByID(id_pelvis).t;
		PelvisPos = pCharacter->GetISkeletonPose()->GetAbsJointByID(id_pelvis).t;

		if (id_right>-1 && id_left>-1)
		{
			Vec3 reye = pCharacter->GetISkeletonPose()->GetAbsJointByID(id_right).t;
			Vec3 leye = pCharacter->GetISkeletonPose()->GetAbsJointByID(id_left).t;
			PelvisPos.z=0;
			return ((reye+leye)*0.5f - PelvisPos);
		}
	}

	return GetStanceInfo(m_stance)->viewOffset;
}

void CActor::ProcessBonesRotation(ICharacterInstance *pCharacter,float frameTime)
{
	ProcessIKLimbs(pCharacter,frameTime);
}

bool CActor::CheckInventoryRestrictions(const char *itemClassName)
{
	if (g_pGameCVars->g_inventoryNoLimits != 0)
		return true;

	IInventory *pInventory = GetInventory();
	if (pInventory)
		return pInventory->IsAvailableSlotForItemClass(itemClassName);


	return false;
}

//grabbing and such
bool CActor::CanPickUpObject(IEntity *obj, float& heavyness, float& volume)
{
	if (!obj)
		return false;

	IPhysicalEntity *pEnt(obj->GetPhysics());
	if (!pEnt)
		return false;

	float mass(0);
	pe_status_dynamics dynStat;
	if (pEnt->GetStatus(&dynStat))
		mass = dynStat.mass;
	/*pe_simulation_params sp;	
	if (pEnt->GetParams(&sp))
		mass = sp.mass;	*/
	
	AABB lBounds;
	obj->GetLocalBounds(lBounds);
	Vec3 delta(lBounds.min - lBounds.max);
	volume = fabs(delta.x * delta.y * delta.z);

	bool canPickUp = false;
	float strength(1.0f);
	SActorParams *pParams(GetActorParams());

	if (pParams && mass <= pParams->maxGrabMass*strength && volume <= pParams->maxGrabVolume)
	{
		canPickUp = true;
		heavyness = 0.3f;

		if(mass > 30.0f)
			heavyness = 0.6f;
	}

	return canPickUp;
}

bool CActor::CanPickUpObject(float mass, float volume)
{
	float strength(1.0f);
	SActorParams *pParams(GetActorParams());
	if (pParams && mass <= pParams->maxGrabMass*strength && volume <= pParams->maxGrabVolume*strength)
		return true;

	return false;
}

void CActor::UpdateGrab(float frameTime)
{
	if (m_pGrabHandler)
		m_pGrabHandler->Update(frameTime);
}

//IK stuff
void CActor::SetIKPos(const char *pLimbName, const Vec3& goalPos, int priority)
{
	int limbID = GetIKLimbIndex(pLimbName);
	if (limbID > -1)
	{
		Vec3 pos(goalPos);
		m_IKLimbs[limbID].SetWPos(GetEntity(),pos,ZERO,0.5f,0.5f,priority);
	}
}

void CActor::CreateIKLimb(int characterSlot, const char *limbName, const char *rootBone, const char *midBone, const char *endBone, int flags)
{
	for (int i=0;i<m_IKLimbs.size();++i)
	{
		if (!strcmp(limbName,m_IKLimbs[i].name))
			return;
	}

	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(characterSlot);
	if (pCharacter)
	{
		SIKLimb newLimb;
		newLimb.SetLimb(characterSlot,limbName,pCharacter->GetISkeletonPose()->GetJointIDByName(rootBone),pCharacter->GetISkeletonPose()->GetJointIDByName(midBone),pCharacter->GetISkeletonPose()->GetJointIDByName(endBone),flags);

		if (newLimb.endBoneID>-1 && newLimb.rootBoneID>-1)
			m_IKLimbs.push_back(newLimb);
	}
}

int CActor::GetIKLimbIndex(const char *limbName)
{
	for (int i=0;i<m_IKLimbs.size();++i)
		if (!strcmp(limbName,m_IKLimbs[i].name))
			return i;

	return -1;
}

void CActor::ProcessIKLimbs(ICharacterInstance *pCharacter,float frameTime)
{
	//first thing: restore the original animation pose if there was some IK
	//FIXME: it could get some optimization.
	for (int i=0;i<m_IKLimbs.size();++i)
		m_IKLimbs[i].Update(GetEntity(),frameTime);

	if (m_pGrabHandler)
	{
		m_pGrabHandler->ProcessIKLimbs(pCharacter);
	}
}

IAnimationGraphState * CActor::GetAnimationGraphState()
{
	if (m_pAnimatedCharacter)
		return m_pAnimatedCharacter->GetAnimationGraphState();
	else
		return NULL;
}

void CActor::SetFacialAlertnessLevel(int alertness)
{
	if (m_pAnimatedCharacter)
		m_pAnimatedCharacter->SetFacialAlertnessLevel(alertness);
}

Vec3 CActor::GetAIAttentionPos()
{
	IPipeUser* pPipeUser = CastToIPipeUserSafe(GetEntity()->GetAI());
	if (pPipeUser)
	{
		IAIObject* pTarget = pPipeUser->GetAttentionTarget();
		if (pTarget)
			return pTarget->GetPos();
	}

	return ZERO;
}

//------------------------------------------------------------------------
void CActor::SelectNextItem(int direction, bool keepHistory, const char *category)
{
	IInventory *pInventory = GetInventory();
	if (!pInventory)
		return;

	if (pInventory->GetCount() < 1)
		return;

	int startSlot = -1;
	int delta = direction;
	EntityId currentItemId = pInventory->GetCurrentItem();
	IItem* pCurrentItem = m_pItemSystem->GetItem(currentItemId);

	bool currWeaponExplosive = false; 

	IInventory::EInventorySlots inventorySlot = IInventory::eInventorySlot_Last;
	
	if (pCurrentItem)
	{
		if (const char* const currentWeaponCat = m_pItemSystem->GetItemCategory(pCurrentItem->GetEntity()->GetClass()->GetName()))
		{
			inventorySlot = pInventory->GetSlotForItemCategory(currentWeaponCat);
			currWeaponExplosive = (inventorySlot == IInventory::eInventorySlot_Explosives) || (inventorySlot == IInventory::eInventorySlot_Grenades);
		}
	}

	if( category 
			&& inventorySlot != IInventory::eInventorySlot_Last 
			&& inventorySlot != IInventory::eInventorySlot_Weapon 
			&& (strstr(category, "primary") || strstr(category, "secondary")))
	{
		startSlot = pInventory->GetLastSelectedInSlot(IInventory::eInventorySlot_Weapon) - delta;
	}
	else if (currentItemId)
	{
		startSlot = pInventory->FindItem(currentItemId);
	}

	const SActorStats *pActorStats = GetActorStats();
	assert(pActorStats);

	int currentSlot = startSlot;
	int skip = pInventory->GetCount(); // maximum number of interactions
	while(skip)
	{
		int slot = currentSlot+delta;

		if (slot<0)
			slot = pInventory->GetCount()-1;
		else if (slot >= pInventory->GetCount())
			slot = 0;

		if (startSlot==slot)
			return;

		EntityId itemId = pInventory->GetItem(slot);
		IItem *pItem = m_pItemSystem->GetItem(itemId);

		const char* name = pItem->GetEntity()->GetClass()->GetName();
		const char *nextWeaponCat = m_pItemSystem->GetItemCategory(name);
		IItem* pCurrentItem = GetCurrentItem();
		bool isDiferentItemFromCurrent = pCurrentItem ? pItem->GetEntity()->GetClass() != pCurrentItem->GetEntity()->GetClass() : true;
		if (pItem->CanSelect() && !pItem->GetDualWieldMasterId() && (!category || strstr(category, nextWeaponCat)) && isDiferentItemFromCurrent)
		{
			IInventory::EInventorySlots nextWeaponSlot = nextWeaponCat ? pInventory->GetSlotForItemCategory(nextWeaponCat) : IInventory::eInventorySlot_Last;
			if (currWeaponExplosive && ((nextWeaponSlot == IInventory::eInventorySlot_Explosives) || (nextWeaponSlot == IInventory::eInventorySlot_Grenades)))
			{
				//don't keep the history when switching from explosive to an explosive as the switch back after wants to go to the last primary/secondary weapon
				keepHistory = false;
			}

			ScheduleItemSwitch(itemId, keepHistory);

			return;
		}

		currentSlot = slot;
		--skip;
	}
}

//------------------------------------------------------------------------
CItem *CActor::GetItem(EntityId itemId) const
{
	return static_cast<CItem*>(m_pItemSystem->GetItem(itemId));
}

//------------------------------------------------------------------------
CItem *CActor::GetItemByClass(IEntityClass* pClass) const
{
	IInventory *pInventory=GetInventory();
	if (pInventory)
	{
		return static_cast<CItem *>(m_pItemSystem->GetItem(pInventory->GetItemByClass(pClass)));
	}
	return 0;
}

//------------------------------------------------------------------------
CWeapon *CActor::GetWeapon(EntityId itemId) const
{
	CItem *pItem = static_cast<CItem *>(m_pItemSystem->GetItem(itemId));
	if (pItem)
		return static_cast<CWeapon *>(pItem->GetIWeapon());

	return 0;
}

//------------------------------------------------------------------------
CWeapon *CActor::GetWeaponByClass(IEntityClass* pClass) const
{
	CItem* pItem = GetItemByClass(pClass);
	if (pItem)
		return static_cast<CWeapon *>(pItem->GetIWeapon());
	return 0;
}

//------------------------------------------------------------------------
void CActor::SelectLastItem(bool keepHistory, bool forceNext /* = false */)
{
	IInventory *pInventory = GetInventory();
	if (!pInventory)
		return;

	EntityId itemId = pInventory->GetLastItem();
	IItem *pItem = m_pItemSystem->GetItem(itemId);

	if (pItem)
		ScheduleItemSwitch(pItem->GetEntityId(), keepHistory);
	else if(forceNext)
		SelectNextItem(1,keepHistory,NULL); //Prevent binoculars to get stuck under certain circumstances
}

//-----------------------------------------------------------------------
bool CActor::ScheduleItemSwitch(EntityId itemId, bool keepHistory)
{
	CItem* pCurrentItem = static_cast<CItem*>(GetCurrentItem());
	CItem* pNextItem = GetItem(itemId);
	bool fastDeselect = false;
	if (pNextItem && pNextItem->GetParams().fast_select)
		fastDeselect = true;
	if (pCurrentItem && (pCurrentItem->IsRippedOff() || pCurrentItem->IsMounted()))
		fastDeselect = true;
	
	const bool isEditing = 	(gEnv->pSystem->IsEditorMode());

	if (!fastDeselect && (isEditing == false))
	{
		//If it's the same item we have, just ignore it
		if (pCurrentItem && pCurrentItem->GetEntityId() == itemId)
		{
			return false;
		}

		SActorStats* pActorStats = GetActorStats();
		assert(pActorStats);

		//If already switching, ignore
		if (pActorStats->exchangeItemStats.switchingToItemID != 0)
		{
			return false;
		}

		uint32 deselectDelay = pCurrentItem ? pCurrentItem->StartDeselection() : 0;

		if(deselectDelay > 0)
		{
		pActorStats->exchangeItemStats.switchingToItemID = itemId;
		pActorStats->exchangeItemStats.keepHistory = keepHistory;

			GetEntity()->SetTimer(ITEM_SWITCH_TIMER_ID, deselectDelay);

			return true;
	}
	}
	
		SelectItem(itemId, keepHistory);

	return false;
}

//------------------------------------------------------------------------
void CActor::SelectItemByName(const char *name, bool keepHistory)
{
	IInventory *pInventory = GetInventory();
	if (!pInventory)
		return;

	if (pInventory->GetCount() < 1)
		return;

	IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(name);
	EntityId itemId = pInventory->GetItemByClass(pClass);
	IItem *pItem = m_pItemSystem->GetItem(itemId);

	if (pItem)
	{
		ScheduleItemSwitch(itemId, keepHistory);
	}
}

//------------------------------------------------------------------------
void CActor::SelectItem(EntityId itemId, bool keepHistory)
{
	if (IItem * pItem = m_pItemSystem->GetItem(itemId))
	{
		IInventory *pInventory = GetInventory();
		if (!pInventory)
			return;

		if (pInventory->GetCount() < 1)
			return;

		if (pInventory->FindItem(itemId) < 0)
		{
			//GameWarning("Trying to select an item which is not in %s's inventory!", GetEntity()->GetName());
			return;
		}

		if (pItem->GetEntityId() == pInventory->GetCurrentItem())	
			return;

		if(pItem->GetEntityId() == pInventory->GetHolsteredItem()) //unholster selected weapon
			pInventory->HolsterItem(false);

		const char *WeaponCat = m_pItemSystem->GetItemCategory(pItem->GetEntity()->GetClass()->GetName());

		if(strcmp(WeaponCat, "secondary") == 0)
		{
			BATTLECHATTER(BC_SecondaryWeapon, GetEntityId())
		}

		m_pItemSystem->SetActorItem(this, pItem->GetEntityId(), keepHistory);
	}
}

//------------------------------------------------------------------------
void CActor::HolsterItem(bool holster)
{
	IInventory *pInventory = GetInventory();
	if (!pInventory)
		return;

	pInventory->HolsterItem(holster);
}

//------------------------------------------------------------------------
bool CActor::UseItem(EntityId itemId)
{
	if (GetHealth()<=0)
		return false;

	CItem *pItem=GetItem(itemId);
	bool canBeUsed = pItem && pItem->CanUse(GetEntityId());
	
	CryLog ("UseItem: %s %s wants to use %s %s %s", GetEntity()->GetClass()->GetName(), GetEntity()->GetName(), pItem ? pItem->GetEntity()->GetClass()->GetName() : "NULL", pItem ? pItem->GetEntity()->GetName() : "item", canBeUsed ? "which is fine!" : "but it's not allowed!");

	if (!canBeUsed)
		return false;

	if (gEnv->bServer || (pItem->GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)))
		pItem->Use(GetEntityId());
	else
		GetGameObject()->InvokeRMI(SvRequestUseItem(), ItemIdParam(itemId), eRMI_ToServer);

	return true;
}

//------------------------------------------------------------------------
bool CActor::PickUpItem(EntityId itemId, bool sound)
{
	IItem *pItem = m_pItemSystem->GetItem(itemId);
	if (!pItem || GetHealth()<=0)
		return false;

	if (gEnv->bServer || (pItem->GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)))
	{
		pItem->PickUp(GetEntityId(), true);
		m_pGameplayRecorder->Event(GetEntity(), GameplayEvent(eGE_ItemPickedUp, 0, 0, (void *)pItem->GetEntityId()));
	}
	else
		GetGameObject()->InvokeRMI(SvRequestPickUpItem(), ItemIdParam(itemId), eRMI_ToServer);

	return true;
}

//------------------------------------------------------------------------
bool CActor::PickUpItemAmmo(EntityId itemId)
{
	CItem *pItem = static_cast<CItem*>(m_pItemSystem->GetItem(itemId));
	if (!pItem || (GetHealth() <= 0))
		return false;

	if (gEnv->bServer || (pItem->GetEntity()->GetFlags()&(ENTITY_FLAG_CLIENT_ONLY|ENTITY_FLAG_SERVER_ONLY)))
	{
		pItem->PickUpAmmo(GetEntityId());
		//m_pGameplayRecorder->Event(GetEntity(), GameplayEvent(eGE_ItemPickedUp, 0, 0, (void *)pItem->GetEntityId()));
	}
	else
	{
		GetGameObject()->InvokeRMI(SvRequestPickUpItem(), ItemIdParam(itemId, true), eRMI_ToServer);
	}

	return true;
}

//------------------------------------------------------------------------
bool CActor::DropItem(EntityId itemId, float impulseScale, bool selectNext, bool bydeath)
{
	CItem *pItem = static_cast<CItem*>(m_pItemSystem->GetItem(itemId));
	if (!pItem)
		return false;

	//Fix editor reseting issue
	//Player dies - Don't drop weapon
	//m_noDrop is only true when leaving the game mode into the editor (see EVENT_RESET in Item.cpp)
	if(IsClient() && gEnv->IsEditor() && ((GetHealth()<=0)||pItem->m_noDrop))
	{
		return false;
	}

	bool bOK = false;
	if (pItem->CanDrop())
	{
		bool performCloakFade = IsCloaked();
		if (pItem->IsDualWield())
			performCloakFade = false;

		if (gEnv->bServer)
		{
			EntityId slaveId = pItem->GetDualWieldSlaveId();
			pItem->Drop(impulseScale, selectNext, bydeath);

			if (!bydeath)
			{
				// send game event
				// check also if the dropped item was actually the slave (akimbo guns)
				m_pGameplayRecorder->Event(GetEntity(), GameplayEvent(eGE_ItemDropped, 0, 0, (void *)(slaveId ? slaveId: itemId)));
			}
		}
		else
			GetGameObject()->InvokeRMI(SvRequestDropItem(), DropItemParams(itemId, impulseScale, selectNext, bydeath), eRMI_ToServer);

		if (performCloakFade)
			pItem->CloakEnable(false, true);

		bOK = true;
	}
	return bOK;
}

//---------------------------------------------------------------------
void CActor::DropAttachedItems()
{
	//Drop weapons attached to the back
	if(gEnv->bServer && GetWeaponAttachmentManager())
	{
		CWeaponAttachmentManager::TAttachedWeaponsList weaponList = GetWeaponAttachmentManager()->GetAttachedWeapons();
		CWeaponAttachmentManager::TAttachedWeaponsList::iterator it = weaponList.begin();
		while(it!=weaponList.end())
		{
			CItem* pItemBack = static_cast<CItem*>(m_pItemSystem->GetItem(*it));
			if(pItemBack)
			{
				pItemBack->Drop(1.0f,false,true);
				m_pGameplayRecorder->Event(GetEntity(), GameplayEvent(eGE_ItemDropped, 0, 0, (void *)pItemBack->GetEntityId()));	
			}

			++it;
		}
	}
}
//------------------------------------------------------------------------
IItem *CActor::GetCurrentItem(bool includeVehicle/*=false*/) const
{
  if (EntityId itemId = GetCurrentItemId(includeVehicle))
	  return m_pItemSystem->GetItem(itemId);

  return 0;
}

//------------------------------------------------------------------------
EntityId CActor::GetCurrentItemId(bool includeVehicle/*=false*/) const
{
  if (includeVehicle)
  {
    if (IVehicle* pVehicle = GetLinkedVehicle())
    {
      if (EntityId itemId = pVehicle->GetCurrentWeaponId(GetEntityId()))
        return itemId;
    }
  }
  
	IInventory *pInventory = GetInventory();
	if (!pInventory)
		return 0;

	return pInventory->GetCurrentItem();
}

//------------------------------------------------------------------------
IItem *CActor::GetHolsteredItem() const
{
	IInventory *pInventory = GetInventory();
	if (!pInventory)
		return 0;

	return m_pItemSystem->GetItem(pInventory->GetHolsteredItem());
}

//------------------------------------------------------------------------
IInventory* CActor::GetInventory() const
{
	if (!m_pInventory)
		m_pInventory = (IInventory*) GetGameObject()->AcquireExtension("Inventory");
	return m_pInventory;
}

//------------------------------------------------------------------------
EntityId CActor::NetGetCurrentItem() const
{
	IInventory *pInventory = GetInventory();
	if (!pInventory)
		return 0;

	return pInventory->GetCurrentItem();
}

//------------------------------------------------------------------------
void CActor::NetSetCurrentItem(EntityId id)
{	
	SelectItem(id, false);
}

//------------------------------------------------------------------------
EntityId CActor::NetGetScheduledItem() const
{
	return GetActorStats()->exchangeItemStats.switchingToItemID;
}

//------------------------------------------------------------------------
void CActor::NetSetScheduledItem(EntityId id)
{	
	
	SActorStats* pStats = GetActorStats();

	if(id && id != pStats->exchangeItemStats.switchingToItemID)
	{
		CItem* pCurrentItem = static_cast<CItem*>(GetCurrentItem());
		if(pCurrentItem)
		{
			pCurrentItem->StartDeselection();
		}	
	}

	pStats->exchangeItemStats.switchingToItemID = id;
}


void CActor::PostUpdate(float frameTime)
{

}

void CActor::ReplaceMaterial(const char *strMaterial)
{
	//FIXME:check a bit more the case of replaceMaterial being called twice before reset it with strMaterial == NULL
	IMaterial *mat(NULL);
	if (strMaterial)
	{
		mat = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(strMaterial);
		if (!mat)
				mat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(strMaterial);
	}

	//replace material
	if (mat)
	{
		for (int i = 0; i < GetEntity()->GetSlotCount(); i++)
		{
			SEntitySlotInfo slotInfo;
			if (GetEntity()->GetSlotInfo(i, slotInfo))
			{
				if (slotInfo.pCharacter)
				{
					SetMaterialRecursive(slotInfo.pCharacter, false, mat);
				}
			}
		}

		CItem *curItem = static_cast<CItem*>(gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem()));
		if (curItem)
			curItem->Cloak(true, mat);
	}
	//restore original material
	else
	{
		for (int i = 0; i < GetEntity()->GetSlotCount(); i++)
		{
			SEntitySlotInfo slotInfo;
			if (GetEntity()->GetSlotInfo(i, slotInfo))
			{
				if (slotInfo.pCharacter)
				{
					SetMaterialRecursive(slotInfo.pCharacter, true);
				}
			}
		}

		CItem *curItem =  static_cast<CItem*>(gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetItem(GetInventory()->GetCurrentItem()));
		if (curItem)
			curItem->Cloak(false);
		
		m_testOldMats.clear();
		m_attchObjMats.clear();
	}
	m_pReplacementMaterial = mat;
}

void CActor::SetMaterialRecursive(ICharacterInstance *charInst, bool undo, IMaterial *newMat)
{
	if (!charInst || (!undo && !newMat))
		return;
	if ((!undo && m_testOldMats.find(charInst) != m_testOldMats.end()) || (undo && m_testOldMats.find(charInst) == m_testOldMats.end()))
		return;

	if (undo)
	{
		charInst->SetMaterial(m_testOldMats[charInst]);
		m_testOldMats.erase(charInst);
	}
	else
	{
		IMaterial *oldMat = charInst->GetMaterial();
		if (newMat != oldMat)
		{
			m_testOldMats[charInst] = oldMat;
			charInst->SetMaterial(newMat);
		}
	}
	for (int i = 0; i < charInst->GetIAttachmentManager()->GetAttachmentCount(); i++)
	{
		IAttachment *attch = charInst->GetIAttachmentManager()->GetInterfaceByIndex(i);
		if (attch)
		{
			IAttachmentObject *obj = attch->GetIAttachmentObject();
			if (obj)
			{
				SetMaterialRecursive(obj->GetICharacterInstance(), undo, newMat);
				if (/*!obj->GetICharacterInstance() &&*/ ((!undo && m_attchObjMats.find(obj) == m_attchObjMats.end()) || undo && m_attchObjMats.find(obj) != m_attchObjMats.end()))
				{

					if (undo)
					{
						obj->SetMaterial(NULL);
						m_attchObjMats.erase(obj);
					}
					else
					{
						IMaterial *oldMat = obj->GetMaterial();
						if (oldMat != newMat)
						{
							m_attchObjMats[obj] = obj->GetMaterial();
							obj->SetMaterial(newMat);
						}
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CActor::SerializeSpawnInfo(TSerialize ser )
{
	ser.Value("teamId", m_teamId, 'team');
}

void CActor::SerializeLevelToLevel( TSerialize &ser )
{
	GetInventory()->SerializeInventoryForLevelChange(ser);
}

//------------------------------------------------------------------------
ISerializableInfoPtr CActor::GetSpawnInfo()
{
	struct SInfo : public ISerializableInfo
	{
		int teamId;
		void SerializeWith( TSerialize ser )
		{
			ser.Value("teamId", teamId, 'team');
		}
	};

	SInfo *p = new SInfo();

	CGameRules *pGameRules=g_pGame->GetGameRules();
	p->teamId = pGameRules?pGameRules->GetTeam(GetEntityId()):0;
	return p;
}

//------------------------------------------------------------------------
void CActor::SetSleepTimer(float timer)
{
	m_sleepTimerOrg=m_sleepTimer=timer;
}


//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, SvRequestDropItem)
{
	IItem *pItem = GetItem(params.itemId);
	if (!pItem)
	{
		GameWarning("[gamenet] Failed to drop item. Item not found!");
		return false;
	}

	//CryLogAlways("%s::SvRequestDropItem(%s)", GetEntity()->GetName(), pItem->GetEntity()->GetName());

	pItem->Drop(params.impulseScale, params.selectNext, params.byDeath);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, SvRequestPickUpItem)
{
	CItem *pItem = static_cast<CItem*>(GetItem(params.itemId));

	if (!pItem)
	{
		// this may occur if the item has been deleted but the client has not yet been informed
		GameWarning("[gamenet] Failed to pickup item. Item not found!");
		return true;
	}

	if (GetHealth()<=0)
		return true;
/*
	// probably should check for ownerId==clientChannelOwnerId
	IActor *pChannelActor=m_pGameFramework->GetIActorSystem()->GetActorByChannelId(m_pGameFramework->GetGameChannelId(pNetChannel));
	assert(pChannelActor);
*/
	EntityId ownerId = pItem->GetOwnerId();
	if (!ownerId)
	{
		if (params.pickOnlyAmmo)
			pItem->PickUpAmmo(GetEntityId());
		else
			pItem->PickUp(GetEntityId(), true);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, SvRequestUseItem)
{
	UseItem(params.itemId);
	return true;
}

//------------------------------------------------------------------------
CItem * CActor::StartRevive(int teamId)
{
	// stop using any mounted weapons before reviving
	CItem *pItem=static_cast<CItem *>(GetCurrentItem());
	if (pItem)
	{
		if (pItem->IsMounted())
		{
			pItem->StopUse(GetEntityId());
			pItem=0;
		}
	}

	if (!IsMigrating())
	{
		SetHealth(GetMaxHealth());
	}

	m_teamId = teamId;

	return pItem;
}

//------------------------------------------------------------------------
void CActor::FinishRevive(CItem * pItem)
{
	// This will cover the case when the ClPickup RMI comes in before we're revived
	if (m_netLastSelectablePickedUp)
	{
		pItem=static_cast<CItem *>(m_pItemSystem->GetItem(m_netLastSelectablePickedUp));
	}

	m_netLastSelectablePickedUp=0;

	if (pItem)
	{
		bool soundEnabled=pItem->IsSoundEnabled();
		pItem->EnableSound(false);
		pItem->Select(false);
		pItem->EnableSound(soundEnabled);

		m_pItemSystem->SetActorItem(this, (EntityId)0);
		SelectItem(pItem->GetEntityId(), true);
	}

	if (IsClient())
	{
		SupressViewBlending(); // no view blending when respawning // CActor::Revive resets it.
	}

	SetActorState(kActorState_alive);
}

//------------------------------------------------------------------------
void CActor::NetReviveAt(const Vec3 &pos, const Quat &rot, int teamId)
{
	if (IVehicle *pVehicle=GetLinkedVehicle())
	{
		if (IVehicleSeat *pSeat=pVehicle->GetSeatForPassenger(GetEntityId()))
			pSeat->Exit(false);
	}

	CItem * currentItem = StartRevive(teamId);

	g_pGame->GetGameRules()->OnRevive(this, pos, rot, m_teamId);

	GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1,1,1), rot, pos));

	Revive(kRFR_Spawn);

	// we've revived and teleported to where we are going, make sure the
	// interpolator agrees
	m_netDesiredLocation = pos;

	g_pGame->GetGameRules()->EntityRevived_NotifyListeners(GetEntityId());

	if(gEnv->bServer)
	{
		m_netPhysCounter = (m_netPhysCounter + 1)&(PHYSICS_COUNTER_MAX - 1);
	}

	FinishRevive(currentItem);
}

//------------------------------------------------------------------------
void CActor::NetReviveInVehicle(EntityId vehicleId, int seatId, int teamId)
{
	CItem * currentItem = StartRevive(teamId);
	
	g_pGame->GetGameRules()->OnReviveInVehicle(this, vehicleId, seatId, m_teamId);

	Revive(kRFR_Spawn);

	// fix our physicalization, since it's need for some vehicle stuff, and it will be set correctly before the end of the frame
	// make sure we are alive, for when we transition from ragdoll to linked...
	if (!GetEntity()->GetPhysics() || GetEntity()->GetPhysics()->GetType()!=PE_LIVING)
		Physicalize();

	IVehicle *pVehicle=m_pGameFramework->GetIVehicleSystem()->GetVehicle(vehicleId);
	assert(pVehicle);
	if(pVehicle)
	{
		IVehicleSeat *pSeat=pVehicle->GetSeatById(seatId);
		if (pSeat && (!pSeat->GetPassenger() || pSeat->GetPassenger()==GetEntityId()))
			pSeat->Enter(GetEntityId(), false);
	}

	FinishRevive(currentItem);
}

//------------------------------------------------------------------------
void CActor::NetKill(const KillParams &killParams)
{
	if (killParams.ragdoll)
	{
		GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
	}

	static char weaponClassName[129]={0};
	m_pGameFramework->GetNetworkSafeClassName(weaponClassName, 128, killParams.weaponClassId);

	g_pGame->GetGameRules()->OnKill(this, killParams.shooterId, killParams.projectileId, weaponClassName, (int)killParams.damage, killParams.hit_joint, killParams.hit_type);

	m_netLastSelectablePickedUp=0;

	if (GetHealth() > 0)
	{
		SetHealth(0);
	}

	Kill();

	UnRegisterInAutoAimManager();

	g_pGame->GetGameRules()->OnKillMessage(GetEntityId(), killParams.shooterId, weaponClassName, killParams.damage, killParams.hit_joint, killParams.hit_type);
}

//------------------------------------------------------------------------
void CActor::NetSimpleKill()
{
	if (GetHealth()>0)
		SetHealth(0);

	Kill();
}

//------------------------------------------------------------------------
bool CActor::LooseHelmet(Vec3 hitDir, Vec3 hitPos)
{
	if(gEnv->bMultiplayer) // this feature is SP only
		return false;

	if(HasNanoSuit())
	{
		return false;
	}

	ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
	if(!pCharacter)
		return false;
	IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
	//get helmet attachment
	bool hasProtection = true;
	IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName("helmet");
	if(!pAttachment)
	{
		hasProtection = false;
		pAttachment = pAttachmentManager->GetInterfaceByName("hat");
	}

	if(pAttachment)
	{
		IAttachmentObject *pAttachmentObj = pAttachment->GetIAttachmentObject();
		if(pAttachmentObj)
		{
			IEntityClassRegistry* pClassRegistry = gEnv->pEntitySystem->GetClassRegistry();
			pClassRegistry->IteratorMoveFirst();
			IEntityClass* pEntityClass = pClassRegistry->FindClass("Default");
			if(!pEntityClass)
				return false;

			//check hit point and direction (simple test whether helmet or face takes the shot, it's all head in the material system)
			if(hitPos.len() && hitDir.len())
			{
				bool frontHit = false;
				SMovementState sMovementState;
				if(IMovementController *pMC = GetMovementController())
				{		
					pMC->GetMovementState(sMovementState);
					Vec3 faceDir = sMovementState.aimDirection;
					float fFront = -faceDir.Dot(hitDir);
					if(fFront > 0.48f)	//probably face hit - no helmet rolling ?
						frontHit = true;

					Vec3 eyePos = sMovementState.eyePosition;
					float hitDist = eyePos.GetDistance(hitPos);

					if(hitDist < 0.1f || (frontHit && hitDist < 0.33f)) //face hit
						return false;
				}
			}

			//spawn new helmet entity
			string helmetName(GetEntity()->GetName());
			helmetName.append("_helmet");
			SEntitySpawnParams params;
			params.sName = helmetName.c_str();
			params.nFlags = ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_MODIFIED_BY_PHYSICS | ENTITY_FLAG_SPAWNED;
			params.pClass = pEntityClass;

			IEntity* pEntity = gEnv->pEntitySystem->SpawnEntity(params, true);
			if (!pEntity)
				return false;

			IAttachmentObject::EType type = pAttachmentObj->GetAttachmentType();
			if(type != IAttachmentObject::eAttachment_StatObj)
			{
				gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());
				return false;
			}
			IStatObj *pStatObj = pAttachmentObj->GetIStatObj();
			
			//set helmet geometry to new entity
			pEntity->SetStatObj(pStatObj, 0, true, 5);
			IMaterial *pUsedMaterial = pAttachmentObj->GetMaterial();
			if(pUsedMaterial)
			{
				pEntity->SetMaterial(pUsedMaterial);
				m_lostHelmetMaterial = pUsedMaterial->GetName();
			}

			Vec3 pos(GetEntity()->GetWorldPos() + GetLocalEyePos(BONE_EYE_R));
			pos.z += 0.2f;
			pEntity->SetPos(pos);

			SEntityPhysicalizeParams pparams;
			pparams.type = PE_RIGID;
			pparams.nSlot = -1;
			pparams.mass = 5;
			pparams.density = 1.0f;
			pEntity->Physicalize(pparams);

			IPhysicalEntity *pPE = pEntity->GetPhysics();
			if(!pPE)
			{
				gEnv->pEntitySystem->RemoveEntity(pEntity->GetId());
				return false;
			}

			SmartScriptTable props;
			IScriptTable *pEntityScript=pEntity->GetScriptTable();
			if(pEntityScript && pEntityScript->GetValue("Properties", props))
				props->SetValue("bPickable", true);

			//some hit-impulse for the helmet
			if(hitDir.len())
			{
				hitDir.Normalize();
				pe_action_impulse imp;
				hitDir += Vec3(0,0,0.2f);
				float r = cry_frand();
				imp.impulse = hitDir * (30.0f * max(0.1f, r));
				imp.angImpulse = (r <= 0.8f && r > 0.3f)?Vec3(0,-1,0):Vec3(0,1,0);
				pPE->Action(&imp);
			}

			//remove old helmet
			pAttachment->ClearBinding();
			m_lostHelmet = pEntity->GetId();
			m_lostHelmetObj = pStatObj->GetFilePath();
			m_lostHelmetPos = hasProtection?"helmet":"hat";

			//add hair if necessary
			IAttachment *pHairAttachment = pAttachmentManager->GetInterfaceByName("hair");
			if(pHairAttachment)
			{
				if(pHairAttachment->IsAttachmentHidden())
					pHairAttachment->HideAttachment(0);
			}

			if(hasProtection)
				return true;
			return false;
		}
	}

	return false;
}

//------------------------------------------------------------------------
void CActor::ResetHelmetAttachment()
{
	//reattach old helmet to entity if possible
	if(m_lostHelmet)
	{
		ICharacterInstance* pCharacter = GetEntity()->GetCharacter(0);
		IAttachmentManager *pAttachmentManager = pCharacter->GetIAttachmentManager();
		IAttachment *pAttachment = pAttachmentManager->GetInterfaceByName(m_lostHelmetPos.c_str());
		if(pAttachment)
		{
			if(!pAttachment->GetIAttachmentObject())
			{
				IStatObj *pStatObj = gEnv->p3DEngine->LoadStatObj(m_lostHelmetObj.c_str());
				if(pStatObj)
				{
					CCGFAttachment *pStatObjAttachment = new CCGFAttachment;
					pStatObjAttachment->pObj = pStatObj;
					pAttachment->AddBinding(pStatObjAttachment);

					if(IMaterial *pMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(m_lostHelmetMaterial.c_str()))
						pStatObjAttachment->SetMaterial(pMat);

					IAttachment *pHairAttachment = pAttachmentManager->GetInterfaceByName("hair");
					if(pHairAttachment)
					{
						if(!pHairAttachment->IsAttachmentHidden())
							pHairAttachment->HideAttachment(1);
					}
				}
			}
		}

		m_lostHelmet = 0;
	}
}

//--------------------------------------------------------
const SNanoSuitGameParameters& CActor::GetActorSuitGameParameters() const
{
	static SNanoSuitGameParameters defaultSuitGameParameters;

	return defaultSuitGameParameters;
}

//-----------------------------------------------------------------------
bool CActor::CanRagDollize() const
{
	const SActorStats *pStats = GetActorStats();
	if(!gEnv->bMultiplayer && !m_isClient && pStats && pStats->isGrabbed)
		return false;

	return true;

}
//------------------------------------------------------------------------
void CActor::GetMemoryUsage(ICrySizer * s) const
{
	s->AddObject(this, sizeof(*this));
	GetInternalMemoryUsage(s);
	
}

void CActor::GetInternalMemoryUsage(ICrySizer * s) const
{
	s->AddObject(m_pInventory);
	s->AddObject(m_pAnimatedCharacter);	
	s->AddObject(m_pMovementController);
//	s->AddObject(m_testOldMats); //Diesel cut
//	s->AddObject(m_attchObjMats); //Diesel cut
	s->AddObject(m_IKLimbs);	

}
//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClRevive)
{
	IEntity *pSpawnPoint = gEnv->pEntitySystem->GetEntity(params.spawnPointId);
	Vec3 pos = pSpawnPoint->GetWorldPos();
	Quat rot = Quat::CreateRotationXYZ(pSpawnPoint->GetWorldAngles());

	NetReviveAt(pos, rot, params.teamId);

	m_netPhysCounter = params.physCounter;

	if (IsClient())
	{
		if (g_pGame->GetRecordingSystem())
		{
			g_pGame->GetRecordingSystem()->StartRecording();
		}
		
		CHUD *pHUD = g_pGame->GetHUD();
		pHUD->ActivateDefaultState();

		if (GetSpectatorState()==CActor::eASS_None)
		{
			SetSpectatorState(CActor::eASS_Ingame);
		}
	}

	return true;
}
/*
//------------------------------------------------------------------------
void CActor::SendRevive(const Vec3& position, const Quat& orientation, int team, bool clearInventory)
{
	Quat orientation = Quat::CreateRotationXYZ(angles);

	// get out of vehicles before reviving
	if (IVehicle *pVehicle=GetLinkedVehicle())
	{
		if (IVehicleSeat *pSeat=pVehicle->GetSeatForPassenger(GetEntityId()))
			pSeat->Exit(false);
	}

	// stop using any mounted weapons before reviving
	if (CItem *pItem=static_cast<CItem *>(GetCurrentItem()))
	{
		if (pItem->IsMounted())
			pItem->StopUse(GetEntityId());
	}

	int health = 100;
	if(!gEnv->bMultiplayer && IsClient())
		health = g_pGameCVars->g_playerHealthValue;
	SetMaxHealth(health);

	if (!m_pGameFramework->IsChannelOnHold(GetChannelId()))
		GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Alive);

	Matrix34 tm(GetEntity()->GetWorldTM());
	tm.SetTranslation(pos);
	tm.SetRotationXYZ(angles);

	GetEntity()->SetWorldTM(tm);
	//SetAngles(angles);

	if (clearInventory)
	{
		GetGameObject()->InvokeRMI(CActor::ClClearInventory(), CActor::NoParams(), 
			eRMI_ToAllClients|eRMI_NoLocalCalls);

		IInventory *pInventory=GetInventory();
		pInventory->Destroy();
		pInventory->Clear();
	}

	NetReviveAt(pos, Quat(angles), teamId);

	GetGameObject()->InvokeRMI(CActor::ClRevive(), CActor::ReviveParams(pos, angles, teamId), 
		eRMI_ToAllClients|eRMI_NoLocalCalls);
}
*/

void CActor::ServerSendKillcamData(IActor *sendto)
{
	CryLog("We killed someone!!  We should send them our killcam data!");
	CRecordingSystem *crs = g_pGame->GetRecordingSystem();
	if(crs)
	{
		uint8 *fpcamdataptr = NULL;
		size_t datasize = crs->GetFirstPersonData(&fpcamdataptr);
		CryLog("killcam data size %d, ptr %p", datasize, fpcamdataptr);

		size_t sizeleft = datasize;
		int numparts = MIN(((datasize/CActor::KillCamData::DATASIZE) + 1), 32);
		int offset = 0;

		for (int i = 0; i < numparts; i++)
		{
			int packetsize = (sizeleft > CActor::KillCamData::DATASIZE ? CActor::KillCamData::DATASIZE : sizeleft);
			CryLog("sv->cl killcam packet data offset %d, %d, %d", sizeleft, packetsize, offset);
			ServerSendKillcamDataPart(sendto, i, packetsize, numparts, &fpcamdataptr[offset]);
			sizeleft = sizeleft - packetsize;
			offset = offset + packetsize;
		}
	}
}

void CActor::ClientSendKillcamData(EntityId shooterId, EntityId victimId)
{
	CRecordingSystem *crs = g_pGame->GetRecordingSystem();
	if( crs )
	{
		IActor *a = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(shooterId);
		if (a && a->IsClient() && a->IsPlayer())
		{
			CPlayer *pPlayer=static_cast<CPlayer*>(this);

			CryLog("We killed someone!!  We should send them our killcam data!");
			
			uint8 *fpcamdataptr = NULL;
			size_t size = crs->GetFirstPersonData(&fpcamdataptr);
			CryLog("killcam data size %d, ptr %p", size, fpcamdataptr);

			size_t sizeleft = size;
			int numparts = MIN(((size/CActor::KillCamData::DATASIZE) + 1), 32);
			int offset = 0;

			for (int i = 0; i < numparts; i++)
			{
				int packetsize = (sizeleft > CActor::KillCamData::DATASIZE ? CActor::KillCamData::DATASIZE : sizeleft);
				CryLog("killcam packet data offset %d, %d, %d", sizeleft, packetsize, offset);
				GetGameObject()->InvokeRMI(SvKillCamData(), CActor::KillCamData(GetEntityId(), i, packetsize, numparts, &fpcamdataptr[offset]), eRMI_ToServer);
				sizeleft = sizeleft - packetsize;
				offset = offset + packetsize;
			}
		}
	}
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClKill)
{
	CryLogAlways("CActor::ClKill - %d shot by %d", GetEntityId(), params.shooterId);

	IActor *a = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(params.shooterId);
	if (a && a->IsClient() && a->IsPlayer())
	{
		CRecordingSystem *crs = g_pGame->GetRecordingSystem();
		if (crs)
		{
			crs->QueueSendRequest(SKillCamSendRequest(params.shooterId, GetEntityId()));
		}
	}

	NetKill(params);

	if (CGameRules *pGameRules=g_pGame->GetGameRules())
	{
		HitInfo  dummyHitInfo;
		dummyHitInfo.shooterId = params.shooterId;
		dummyHitInfo.targetId = GetEntityId();
		dummyHitInfo.weaponId = params.weaponClassId;
		dummyHitInfo.damage = params.damage;
		dummyHitInfo.type = params.hit_type;
		pGameRules->OnEntityKilled(dummyHitInfo);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClKillCamData)
{
	CryLog("CActor::SvKillCamData part %d/%d (%d bytes)", params.m_packetIndex, params.m_numParts, params.m_dataSize);
	CryLog("A CActor::ClKillCamData.  It's for me!  Wonderful!  I shall just tell the recording system.");
	CRecordingSystem *crs = g_pGame->GetRecordingSystem();
	if (crs)
	{
		crs->SetFirstPersonDataPart((uint8*)params.m_data, params.m_packetIndex, params.m_numParts, params.m_dataSize);
	}
	return true;
}

void CActor::ServerSendKillcamDataPart(IActor *sendto, int partindex, size_t datasize, int numparts, uint8 *fpcamdataptr)
{
	GetGameObject()->InvokeRMI(ClKillCamData(), CActor::KillCamData(sendto->GetEntityId(), partindex, datasize, numparts, fpcamdataptr), eRMI_ToClientChannel, sendto->GetChannelId());
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, SvKillCamData)
{
	CryLog("CActor::SvKillCamData part %d/%d (%d bytes)", params.m_packetIndex, params.m_numParts, params.m_dataSize);
	IActor *a = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(params.m_sendToId);

	if (a->IsClient())
	{
		CryLog("It's for me!  Wonderful!  I shall just tell the recording system.");
		CRecordingSystem *crs = g_pGame->GetRecordingSystem();
		if (crs)
		{
			crs->SetFirstPersonDataPart((uint8*)params.m_data, params.m_packetIndex, params.m_numParts, params.m_dataSize);
		}
	}
	else
	{
		CryLog("It's for someone else (%s)", a->GetEntity()->GetName());

		ServerSendKillcamDataPart(a, params.m_packetIndex, params.m_dataSize, params.m_numParts, (uint8 *) params.m_data);
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClSimpleKill)
{
	NetSimpleKill();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClMoveTo)
{
	//teleport
	GetEntity()->SetWorldTM(Matrix34::Create(Vec3(1,1,1), params.rot, params.pos));
	OnTeleported();

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClAddAmmo)
{
	IInventory *pInventory=GetInventory();
	if (pInventory)
	{
		IEntityClass* pClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(params.ammo.c_str());
		assert(pClass);

		int capacity = pInventory->GetAmmoCapacity(pClass);
		int current = pInventory->GetAmmoCount(pClass);
		if((!gEnv->IsEditor()) && (pInventory->GetAmmoCount(pClass)+params.count > capacity))
		{
			//If still there's some place, full inventory to maximum...
			pInventory->SetAmmoCount(pClass,capacity);
		}
		else
		{
			pInventory->SetAmmoCount(pClass, pInventory->GetAmmoCount(pClass)+params.count);
		}
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClPickUp)
{
	if (CItem *pItem = GetItem(params.itemId))
	{
		if(params.pickOnlyAmmo)
		{
			pItem->PickUpAmmo(GetEntityId());
		}
		else
		{
			pItem->PickUp(GetEntityId(), params.sound, params.select);

			const char *displayName = pItem->GetDisplayName();

			if (params.select)
				m_netLastSelectablePickedUp=params.itemId;
		}
	}

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClDrop)
{
	CItem *pItem=GetItem(params.itemId);
	if (pItem)
		pItem->Drop(params.impulseScale, params.selectNext, params.byDeath);

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClStartUse)
{
	CItem *pItem=GetItem(params.itemId);
	if (pItem)
		pItem->StartUse(GetEntityId());

	return true;
}

//------------------------------------------------------------------------
IMPLEMENT_RMI(CActor, ClStopUse)
{
	CItem *pItem=GetItem(params.itemId);
	if (pItem)
		pItem->StopUse(GetEntityId());

	return true;
}

//------------------------------------------------------------------------
void CActor::DumpActorInfo()
{
  IEntity* pEntity = GetEntity();

  CryLog("ActorInfo for %s", pEntity->GetName());
  CryLog("=====================================");
  
  Vec3 pos = pEntity->GetWorldPos();
  CryLog("Entity Pos: %.f %.f %.f", pos.x, pos.y, pos.z);
  CryLog("Active: %i", pEntity->IsActive());
  CryLog("Hidden: %i", pEntity->IsHidden());
  CryLog("Invisible: %i", pEntity->IsInvisible());  
  CryLog("Profile: %i", m_currentPhysProfile);
  CryLog("Health: %i", GetHealth());  
  
  if (IPhysicalEntity* pPhysics = pEntity->GetPhysics())
  { 
    CryLog("Physics type: %i", pPhysics->GetType());
    
    pe_status_pos pos;
    if (pPhysics->GetStatus(&pos))
    {
      CryLog("Physics pos: %.f %.f %.f", pos.pos.x, pos.pos.y, pos.pos.z);
    }

    pe_status_dynamics dyn;
    if (pPhysics->GetStatus(&dyn))
    {   
      CryLog("Mass: %.1f", dyn.mass);
      CryLog("Vel: %.2f %.2f %.2f", dyn.v.x, dyn.v.y, dyn.v.z);
    } 
  }  

  if (IVehicle* pVehicle = GetLinkedVehicle())
  {
    CryLog("Vehicle: %s (destroyed: %i)", pVehicle->GetEntity()->GetName(), pVehicle->IsDestroyed());
    
    IVehicleSeat* pSeat = pVehicle->GetSeatForPassenger(GetEntityId());
    CryLog("Seat %i", pSeat ? pSeat->GetSeatId() : 0);
  }

  if (IItem* pItem = GetCurrentItem())
  {
    CryLog("Item: %s", pItem->GetEntity()->GetName());
  }


  CryLog("=====================================");
}

//
//-----------------------------------------------------------------------------
Vec3 CActor::GetWeaponOffsetWithLean(EStance stance, float lean, float peekOver, const Vec3& eyeOffset)
{
	//for player just do it the old way - from stance info
	if(IsPlayer())
		return GetStanceInfo(stance)->GetWeaponOffsetWithLean(lean, peekOver);

	EntityId itemId = GetInventory()->GetCurrentItem();
	if(itemId)
	{
		if(CWeapon* weap = GetWeapon(itemId))
		{
			if(weap->AIUseEyeOffset())
				return eyeOffset;

			Vec3	overrideOffset;
			if(weap->AIUseOverrideOffset(stance, lean, peekOver, overrideOffset))
				return overrideOffset;
		}
	}
	return GetStanceInfo(stance)->GetWeaponOffsetWithLean(lean, peekOver);
}

//-------------------------------------------------------------------
//This function is called from Equipment Manager only for the client actor
void CActor::NotifyInventoryAmmoChange(IEntityClass* pAmmoClass, int amount)
{
	if(!pAmmoClass)
		return;
}

int CActor::IsFriendlyEntity(EntityId entityId)
{
	IEntity *e=gEnv->pEntitySystem->GetEntity(entityId);

	//Faster to not do an early out on entityId == actor->entityId
	if (!e)
	{
		return true;
	}

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (!gEnv->bMultiplayer)
	{
		if (e->GetAI())
		{
			if (!e->GetAI()->IsHostile(GetEntity()->GetAI()))
				return true;
		}
	}
	else
	{
		if(pGameRules)
		{
			if (pGameRules->GetTeamCount()>=2)
			{
				int iMyTeam=pGameRules->GetTeam(GetEntityId());
				int iClientTeam = pGameRules->GetTeam(entityId);
				return (iClientTeam==iMyTeam);
			}
			else
			{
				return entityId == GetEntityId();
			}
		}

	}
	return false;
}

void CActor::AddViewAngleOffsetForFrame( const Ang3 &offset )
{

}

const Ang3& CActor::GetViewAngleOffset() const
{
	const static Ang3 defaultAngles(0, 0, 0);

	return defaultAngles;
}
//------------------------------------------------------------------------
void CActor::EnableSwitchingItems( bool enable )
{
	m_enableSwitchingItems = enable;
}

//------------------------------------------------------------------------
void CActor::EnableIronSights( bool enable )
{
	m_enableIronSights = enable;
}

//------------------------------------------------------------------------
void CActor::EnablePickingUpItems( bool enable )
{
	m_enablePickupItems = enable;
}

void CActor::UpdateMountedGunController(bool forceIKUpdate)
{
}

void CActor::SetRagdollOnCollision()
{
	CRY_ASSERT_MESSAGE(!GetActorStats()->isRagDoll, "Queuing ragdoll when ragdoll is already enabled");

	m_ragdollOnCollision = true;
}

void CActor::UpdateRagdollOnCollision()
{
	const float RAGDOLL_TEST_RADIUS = 1.0f;
	const float RAGDOLL_TEST_GROUND_OFFSET = 0.5f;

	primitives::sphere sphPrim;
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	int spineID = GetBoneID(BONE_SPINE);
	
	CRY_TODO(08, 12, 2009, "GetBoneID works for humans only, Aliens have different bone IDs, would be better to improve GetBoneID to use a data driven solution")
	if (spineID == -1)
	{
		spineID = pCharacter->GetISkeletonPose()->GetJointIDByName("Spine01");
	}

	Vec3 vRootBone = pCharacter->GetISkeletonPose()->GetAbsJointByID(spineID).t; 
	vRootBone.z=0;
	sphPrim.center = GetEntity()->GetWorldTM() * vRootBone;
	sphPrim.center.z += RAGDOLL_TEST_RADIUS + RAGDOLL_TEST_GROUND_OFFSET;
	sphPrim.r = RAGDOLL_TEST_RADIUS;

	int collisionEntityTypes = ent_static|ent_ignore_noncolliding;
	float d = 0;
	d = gEnv->pPhysicalWorld->PrimitiveWorldIntersection( sphPrim.type, &sphPrim, Vec3(0,0,0),collisionEntityTypes, NULL ,0, geom_colltype0, 0);

	if (d != 0.0f)
	{
		GetGameObject()->SetAspectProfile(eEA_Physics, eAP_Ragdoll);
	}

#if !defined(_RELEASE)
	if (g_pGameCVars->g_debugHitReacts)
	{
		ColorB col(0xff, 0, 0, 0xff);
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(sphPrim.center, sphPrim.r, col);
	}
#endif
}

void CActor::RegisterInAutoAimManager()
{
	if (m_registeredInAutoAimMng)
		return;

	if (gEnv->pSystem->IsEditorMode())
		return;

	if (m_isClient || (GetHealth() <= 0))
		return;

	
	SAutoaimTargetRegisterParams registerParams;

	SmartScriptTable targetParams;
	if (GetAimTargetParams(targetParams))
	{
		CScriptSetGetChain chainParams(targetParams);
		int primaryTargetId = -1, secondaryTargetId = -1, physicsTargetId = -1;
		chainParams.GetValue("primaryTargetBone", primaryTargetId);
		chainParams.GetValue("physicsTargetBone", physicsTargetId);
		chainParams.GetValue("secondaryTargetBone", secondaryTargetId);
		chainParams.GetValue("fallbackOffset", registerParams.fallbackOffset);
		chainParams.GetValue("innerRadius", registerParams.innerRadius);
		chainParams.GetValue("outerRadius", registerParams.outerRadius);
		chainParams.GetValue("snapRadius", registerParams.snapRadius);
		chainParams.GetValue("secondaryRadius", registerParams.secondaryRadius);

		registerParams.primaryBoneId = (primaryTargetId > -1) ? GetBoneID(primaryTargetId, 0) : -1;
		registerParams.physicsBoneId = (physicsTargetId > -1) ? GetBoneID(physicsTargetId, 0) : -1;
		registerParams.secondaryBoneId = (secondaryTargetId > -1) ? GetBoneID(secondaryTargetId, 0) : -1;
	}

	m_registeredInAutoAimMng = ShouldRegisterAsAutoAimTarget() && g_pGame->GetAutoAimManager().RegisterAutoaimTarget(GetEntityId(), registerParams);
}

void CActor::UnRegisterInAutoAimManager()
{
	if (!m_registeredInAutoAimMng)
		return;

	g_pGame->GetAutoAimManager().UnregisterAutoaimTarget(GetEntityId());

	m_registeredInAutoAimMng = false;
}

bool CActor::GetAimTargetParams(SmartScriptTable& aimTarget)
{
	if (IScriptTable* pEntityScript = GetEntity()->GetScriptTable())
	{
		SmartScriptTable gameParams;
		if (pEntityScript->GetValue("gameParams", gameParams))
		{
			return gameParams->GetValue("autoAimTargetParams", aimTarget);
		}
	}
	
	return false;
}

void CActor::OnAIProxyEnabled( bool enabled )
{
	if (enabled)
	{
		RegisterInAutoAimManager();
	}
	else
	{
		UnRegisterInAutoAimManager();
	}
}

void CActor::StartInteractiveAction( EntityId entityId )
{
	
}

void CActor::StartInteractiveActionByName( const char* interaction )
{

}

void CActor::EndInteractiveAction( EntityId entityId )
{

}
