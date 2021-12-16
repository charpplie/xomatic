//==================================================================================================
// Name: CExplosionGameEffect
// Desc: Explosion game effect - handles screen filters like radial blur etc
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "ExplosionGameEffect.h"
#include "IGameRulesSystem.h"
#include "IMovementController.h"
#include "Player.h"
#include "GameCVars.h"
#include "ScreenEffects.h"
#include "GameForceFeedback.h"
#include "GameRules.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"
#include "RecordingSystem.h"

REGISTER_EFFECT_DEBUG_DATA(CExplosionGameEffect::DebugOnInputEvent,CExplosionGameEffect::DebugDisplay,Explosion);

//--------------------------------------------------------------------------------------------------
// Name: CExplosionGameEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CExplosionGameEffect::CExplosionGameEffect()
{
	m_cutSceneActive = false;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CExplosionGameEffect
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CExplosionGameEffect::~CExplosionGameEffect()
{
	
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initializes game effect
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::Initialise(const SGameEffectParams* gameEffectParams)
{
	CGameEffect::Initialise(gameEffectParams);

	
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Release
// Desc: Releases game effect
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::Release()
{
	CGameEffect::Release();
	
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates game effect
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::Update(float frameTime)
{
	CGameEffect::Update(frameTime);

	
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Explode
// Desc: Spawns explosion
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::Explode(ExplosionInfo &explosionInfo)
{
	if(IsFlagSet(GAME_EFFECT_ACTIVE))
	{
		SpawnParticleEffect(explosionInfo);

		if(!m_cutSceneActive)
		{
			SpawnScreenExplosionEffect(explosionInfo);
			QueueMaterialEffect(explosionInfo);
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SpawnParticleEffect
// Desc: Spawns the explosion's particle effect
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::SpawnParticleEffect(const ExplosionInfo &explosionInfo)
{
	if(gEnv->bClient)
	{
		if(explosionInfo.pParticleEffect)
		{
			const bool bIndependent = true;
			explosionInfo.pParticleEffect->Spawn(	bIndependent, IParticleEffect::ParticleLoc(explosionInfo.pos, explosionInfo.dir, explosionInfo.effect_scale) );
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SpawnScreenExplosionEffect
// Desc: Spawns screen explosion effect
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::SpawnScreenExplosionEffect(const ExplosionInfo &explosionInfo)
{
	// Disclaimer: this code was originally from GameRulesClientServer::ProcessClientExplosionScreenFX()

	if(explosionInfo.pressure < 1.0f)
		return;

	IActor *pClientActor = g_pGame->GetIGameFramework()->GetClientActor();
	if(pClientActor && !pClientActor->IsDead())
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pClientActor);
		bool hasFlashBangEffect = explosionInfo.blindAmount > 0.0f;

		// Flashbang friends and self?...
		if(hasFlashBangEffect)
		{
			bool ownFlashBang = pPlayer->GetEntityId() == explosionInfo.shooterId;
			if((!g_pGameCVars->g_flashBangSelf && ownFlashBang) || // FlashBang self?
				((g_pGameCVars->g_friendlyfireratio==0.0f) && (!g_pGameCVars->g_flashBangFriends) && (!ownFlashBang) && pPlayer->IsFriendlyEntity(explosionInfo.shooterId))) // FlashBang friends?
			{
				return;
			}
		}

		// Distance
		float dist = (pClientActor->GetEntity()->GetWorldPos() - explosionInfo.pos).len();

		// Is the explosion in Player's FOV (let's suppose the FOV a bit higher, like 80)
		SMovementState state;
		if(IMovementController *pMV = pClientActor->GetMovementController())
		{
			pMV->GetMovementState(state);
		}

		Vec3 eyeToExplosion = explosionInfo.pos - state.eyePosition;
		Vec3 eyeDir = pClientActor->GetLinkedVehicle() ? pPlayer->GetVehicleViewDir() : state.eyeDirection;
		eyeToExplosion.Normalize();
		float eyeDirectionDP = eyeDir.Dot(eyeToExplosion);
		bool inFOV = (eyeDirectionDP > 0.68f);

		// All explosions have radial blur (default 30m radius)
		float maxBlurDistance = (explosionInfo.maxblurdistance>0.0f)?explosionInfo.maxblurdistance:30.0f;
		if(maxBlurDistance>0.0f && g_pGameCVars->g_radialBlur>0.0f && explosionInfo.radius>0.5f)
		{		
			if (inFOV && dist < maxBlurDistance)
			{
				ray_hit hit;
				const int intersectionObjTypes = ent_static | ent_terrain;
				const unsigned int intersectionFlags = rwi_stop_at_pierceable|rwi_colltype_any;
				const int intersectionMaxHits = 1;

				int collision = gEnv->pPhysicalWorld->RayWorldIntersection(	explosionInfo.pos, 
																																		-eyeToExplosion*dist, 
																																		intersectionObjTypes, 
																																		intersectionFlags, 
																																		&hit, 
																																		intersectionMaxHits);

				//If there was no obstacle between explosion position and player
				if(!collision)
				{
					if(CScreenEffects* pScreenFX = g_pGame->GetScreenEffects())
					{
						float blurRadius = (-1.0f/maxBlurDistance)*dist + 1.0f;
						pScreenFX->ProcessExplosionEffect(blurRadius, explosionInfo.pos);
					}

					float distAmp = 1.0f - (dist / maxBlurDistance);
					GameForceFeedback_Always(SFFOutputEvent(eDI_XI, eFF_Rumble_Basic, 0.5f, distAmp*3.0f, 0.0f));
				}
			}
		}

		// Flashbang effect 
		if(hasFlashBangEffect && ((dist < (explosionInfo.radius*g_pGameCVars->g_flashBangNotInFOVRadiusFraction)) 
			|| (inFOV && (dist < explosionInfo.radius))))
		{
			ray_hit hit;
			const int intersectionObjTypes = ent_static | ent_terrain;
			const unsigned int intersectionFlags = rwi_stop_at_pierceable|rwi_colltype_any;
			const int intersectionMaxHits = 1;

			int collision = gEnv->pPhysicalWorld->RayWorldIntersection(	explosionInfo.pos, 
																																	-eyeToExplosion*dist, 
																																	intersectionObjTypes, 
																																	intersectionFlags, 
																																	&hit, 
																																	intersectionMaxHits);

			// If there was no obstacle between flashbang grenade and player
			if(!collision)
			{
				CCCPOINT (FlashBang_Explode_BlindLocalPlayer);
				float timeScale = max(0.0f, 1 - (dist/explosionInfo.radius));
				float lookingAt = max(g_pGameCVars->g_flashBangMinFOVMultiplier, (eyeDirectionDP + 1)*0.5f);

				float time = explosionInfo.flashbangScale * timeScale *lookingAt;	// time is determined by distance to explosion		

				CRY_ASSERT_MESSAGE(pClientActor->IsPlayer(),"Effect shouldn't be spawned if not a player");

				CPlayer* pPlayer = static_cast<CPlayer*>(pClientActor);
				SPlayerStats* pStats = static_cast<SPlayerStats*>(pPlayer->GetActorStats());

				CryLog( "CExplosionGameEffect::SpawnScreenExplosionEffect() Looking at player %d's perks.", (int)pClientActor->GetEntityId() );
				pPlayer->SendPerkEvent(EPE_FlashbangScale, &time);

				pPlayer->StartFlashbangEffects(time);

				g_pGame->GetGameRules()->SuccessfulFlashBang(explosionInfo);

				gEnv->p3DEngine->SetPostEffectParam("Flashbang_Time", time * 2.0f );	// seems to need to be doubled to get it in seconds...
				gEnv->p3DEngine->SetPostEffectParam("FlashBang_BlindAmount", explosionInfo.blindAmount);
				gEnv->p3DEngine->SetPostEffectParam("Flashbang_DifractionAmount", time);
				gEnv->p3DEngine->SetPostEffectParam("Flashbang_Active", 1.0f);

				CRecordingSystem *pRecordingSystem = g_pGame->GetRecordingSystem();
				if (pRecordingSystem)
				{
					pRecordingSystem->OnPlayerFlashed(time, explosionInfo.blindAmount);
				}
			}
			else
			{
				CCCPOINT (FlashBang_Explode_NearbyButBlockedByGeometry);
			}
		}
		else if(inFOV && (dist < explosionInfo.radius))
		{
			if(explosionInfo.damage>10.0f || explosionInfo.pressure>100.0f)
			{
				// Add some angular impulse to the client actor depending on distance, direction...
				float dt = (1.0f - dist/explosionInfo.radius);
				dt = dt * dt;
				float angleZ = gf_PI*0.15f*dt;
				float angleX = gf_PI*0.15f*dt;

				if (pClientActor)
				{
					static_cast<CActor*>(pClientActor)->AddAngularImpulse(Ang3(Random(-angleX*0.5f,angleX),0.0f,Random(-angleZ,angleZ)),0.0f,dt*2.0f);
				}
			}
		}

		float fDist2=(pClientActor->GetEntity()->GetWorldPos()-explosionInfo.pos).len2();
		if(fDist2<250.0f*250.0f)
		{		
			SAFE_HUD_FUNC(ShowSoundOnRadar(explosionInfo.pos, explosionInfo.hole_size));
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: QueueMaterialEffect
// Desc: Queues material effect and sets off a deferred linetest for later processing
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::QueueMaterialEffect(ExplosionInfo &explosionInfo)
{
	// If an effect was already specified, don't use MFX
	if(explosionInfo.pParticleEffect)
		return;

	const int intersectionObjTypes = ent_all;    
	const unsigned int intersectionFlags = rwi_stop_at_pierceable|rwi_colltype_any;

	if(explosionInfo.impact)
	{
		Vec3 explosionDir = explosionInfo.impact_velocity.normalized();
		explosionInfo.raycastHelper->CastRay(explosionInfo.pos-explosionDir*0.1f, explosionDir, intersectionObjTypes, intersectionFlags);
	}
	else
	{
		Vec3 gravity;
		pe_params_buoyancy buoyancy;
		gEnv->pPhysicalWorld->CheckAreas(explosionInfo.pos, gravity, &buoyancy);
		gravity.NormalizeFast();
		gravity *= g_pGameCVars->g_explosion_materialFX_raycastLength;
		explosionInfo.raycastHelper->CastRay(explosionInfo.pos, gravity, intersectionObjTypes, intersectionFlags);
	}

	explosionInfo.deferredRaycastState = eDeferredRaycastState_Dispatched;
}

//--------------------------------------------------------------------------------------------------
// Name: SpawnMaterialEffect
// Desc: Spawns material effect
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::SpawnMaterialEffect(const ExplosionInfo &explosionInfo)
{
	// Disclaimer: this code was originally from GameRulesClientServer::ProcessExplosionMaterialFX()


	// impact stuff here
	SMFXRunTimeEffectParams params;
	params.soundSemantic = eSoundSemantic_Explosion;
	params.pos = params.decalPos = explosionInfo.pos;
	params.trg = 0;
	params.trgRenderNode = 0;

	Vec3 gravity;
	pe_params_buoyancy buoyancy;
	gEnv->pPhysicalWorld->CheckAreas(params.pos, gravity, &buoyancy);

	// 0 for water, 1 for air
	Vec3 pos = params.pos;
	params.inWater = (buoyancy.waterPlane.origin.z > params.pos.z) && (gEnv->p3DEngine->GetWaterLevel(&pos)>=params.pos.z);
	params.inZeroG = (gravity.len2() < 0.0001f);
	params.trgSurfaceId = 0;

	const int intersectionObjTypes = ent_all;    
	const unsigned int intersectionFlags = rwi_stop_at_pierceable|rwi_colltype_any;

	ray_hit ray;

	if(explosionInfo.impact)
	{
		params.dir[0] = explosionInfo.impact_velocity.normalized();
		params.normal = explosionInfo.impact_normal;
	}
	else
	{
		params.dir[0] = gravity;
		params.normal = -gravity.normalized();
	}

	if(explosionInfo.deferredRaycastState == eDeferredRaycastState_ResultImpact)
	{
		params.trgSurfaceId = explosionInfo.mfxTargetSurfaceId;

		if (explosionInfo.pMfxTargetPhysEnt && explosionInfo.pMfxTargetPhysEnt->GetiForeignData()==PHYS_FOREIGN_ID_STATIC)
		{
			params.trgRenderNode = (IRenderNode*)explosionInfo.pMfxTargetPhysEnt->GetForeignData(PHYS_FOREIGN_ID_STATIC);
		}
	}

	// Create query name
	string effectClass = explosionInfo.effect_class;
	if(effectClass.empty())
		effectClass = "generic";

	string query = effectClass + "_explode";
	if(gEnv->p3DEngine->GetWaterLevel(&explosionInfo.pos)>explosionInfo.pos.z)
		query = query + "_underwater";

	// Get material effect id
	IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
	TMFXEffectId effectId = pMaterialEffects->GetEffectId(query.c_str(), params.trgSurfaceId);

	if(effectId == InvalidEffectId)
	{
		// Get default surface id
		effectId = pMaterialEffects->GetEffectId(query.c_str(), pMaterialEffects->GetDefaultSurfaceIndex());
	}

	// Execute material effect
	if(effectId != InvalidEffectId)
	{
		pMaterialEffects->ExecuteEffect(effectId, params);
	}
}//-------------------------------------------------------------------------------------------------

#if DEBUG_GAME_FX_SYSTEM
//--------------------------------------------------------------------------------------------------
// Name: DebugOnInputEvent
// Desc: Called when input events happen in debug builds
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::DebugOnInputEvent(int keyId)
{
	// Initialise static version of effect
	static CExplosionGameEffect explosionGameEffect;
	if(!explosionGameEffect.IsFlagSet(GAME_EFFECT_INITIALISED))
	{
		explosionGameEffect.Initialise();
		explosionGameEffect.SetActive(true);
	}

	// Get player pos
	EntityId localClientId = gEnv->pGame->GetIGameFramework()->GetClientActorId();
	IEntity* playerEntity = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(localClientId)->GetEntity();
	Vec3 playerDir = playerEntity->GetForwardDir();
	Vec3 playerPos = playerEntity->GetPos();

	// Distance from player controlled by keyboard
	static float distFromPlayer = 0.0f;
	static float distStep = 1.0f;

	switch(keyId)
	{
		case eKI_NP_Multiply:
		{
			distFromPlayer += distStep;
			break;
		}
		case eKI_NP_Divide:
		{
			distFromPlayer -= distStep;
			break;
		}
		case eKI_NP_Period:
		{
			distFromPlayer = 0.0f;
			break;
		}
		case eKI_NP_1:
		{
			// Frag
			ExplosionInfo explosionInfo;
			explosionInfo.pParticleEffect = gEnv->pParticleManager->FindEffect("Crysis2_weapon_explosives.frag.concrete");
			explosionInfo.pos = playerPos + (playerDir * distFromPlayer);
			explosionInfo.dir.Set(0.0f,-1.0f,0.0f);
			explosionInfo.effect_scale = 1.0f;
			explosionInfo.pressure = 1000.0f;
			explosionInfo.maxblurdistance = 10.0;
			explosionInfo.radius = 15.0;
			explosionInfo.blindAmount = 0.0f;
			explosionInfo.flashbangScale = 8.0f;
			explosionInfo.damage = 5.0f;
			explosionInfo.hole_size = 0.0f;
			explosionGameEffect.Explode(explosionInfo);
			break;
		}
		case eKI_NP_2:
		{
			// Flashbang
			ExplosionInfo explosionInfo;
			explosionInfo.pParticleEffect = gEnv->pParticleManager->FindEffect("Crysis2_weapon_explosives.grenades.flash_explosion");
			explosionInfo.pos = playerPos + (playerDir * distFromPlayer);
			explosionInfo.dir.Set(0.0f,-1.0f,0.0f);
			explosionInfo.effect_scale = 1.0f;
			explosionInfo.pressure = 1000.0f;
			explosionInfo.maxblurdistance = 10.0;
			explosionInfo.radius = 15.0;
			explosionInfo.blindAmount = 0.66f;
			explosionInfo.flashbangScale = 8.0f;
			explosionInfo.damage = 5.0f;
			explosionInfo.hole_size = 0.0f;
			explosionGameEffect.Explode(explosionInfo);
			break;
		}
		case eKI_NP_3:
		{
			// L-Tag
			ExplosionInfo explosionInfo;
			explosionInfo.pParticleEffect = gEnv->pParticleManager->FindEffect("Crysis2_weapon_fx.l-tag.rico_explosion");
			explosionInfo.pos = playerPos + (playerDir * distFromPlayer);
			explosionInfo.dir.Set(0.0f,-1.0f,0.0f);
			explosionInfo.effect_scale = 1.0f;
			explosionInfo.pressure = 1000.0f;
			explosionInfo.maxblurdistance = 10.0;
			explosionInfo.radius = 15.0;
			explosionInfo.blindAmount = 0.0f;
			explosionInfo.flashbangScale = 8.0f;
			explosionInfo.damage = 5.0f;
			explosionInfo.hole_size = 0.0f;
			explosionGameEffect.Explode(explosionInfo);
			break;
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: DebugDisplay
// Desc: Display when this effect is selected to debug through the game effects system
//--------------------------------------------------------------------------------------------------
void CExplosionGameEffect::DebugDisplay(const Vec2& textStartPos,float textSize,float textYStep)
{
	ColorF textCol(1.0f,1.0f,0.0f,1.0f);
	Vec2 currentTextPos = textStartPos;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Reset distance: NumPad .");
	currentTextPos.y += textYStep;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Add distance: NumPad *");
	currentTextPos.y += textYStep;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Subtract distance: NumPad /");
	currentTextPos.y += textYStep;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Frag: NumPad 1");
	currentTextPos.y += textYStep;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"Flashbang: NumPad 2");
	currentTextPos.y += textYStep;
	gEnv->pRenderer->Draw2dLabel(currentTextPos.x,currentTextPos.y,textSize,&textCol.r,false,"L-tag: NumPad 3");
}//-------------------------------------------------------------------------------------------------
#endif
