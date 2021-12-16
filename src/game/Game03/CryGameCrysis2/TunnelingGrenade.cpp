/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 01/05/2009 - Created By Tom Berry
- 19/08/2009 - Butchered By Claire Allan

*************************************************************************/
#include "StdAfx.h"
#include "TunnelingGrenade.h"
#include "Game.h"
#include "GameCVars.h"
#include "WeaponSystem.h"
#include "NetInputChainDebug.h"
#include <GameRules.h>

#include "IMaterialEffects.h"
#include "IRenderAuxGeom.h"
#include "ICryAnimation.h"

static float SURFACE_EFFECT_PROBE_HEIGHT = 0.7f;

int CTunnellingGrenade::s_attachNameID = 0;

//------------------------------------------------------------------------
CTunnellingGrenade::CTunnellingGrenade()
:
	m_mode(Airbourne),
	m_penetrationDepth(0.3f),
	m_failedExplosionScale(1.f),
	m_drilledDist(0.f),
	m_flags(0),
	m_enterSurfaceAnim(NULL),
	m_entryEffect(eDrillingEffect_wait),
	m_exitEffect(eDrillingEffect_wait),
	m_lastReflectedMode(Airbourne),
	m_pSoundProxy(NULL),
	m_soundId(INVALID_SOUNDID),
	m_soundDig(0.0f),
	m_groundPos(ZERO),
	m_attachJoint(-1),
	m_drilledActorId(0),
	m_attachPos(ZERO),
	m_attachRot(ZERO),
	m_characterAttachment(NULL),
	m_effectAttachment(NULL)
{
}

//------------------------------------------------------------------------
CTunnellingGrenade::~CTunnellingGrenade()
{
	if(m_pSoundProxy)
	{
		m_pSoundProxy->StopSound(m_soundId);
		m_pSoundProxy = NULL;	//Should be automatically release with the entity
	}
	
	if(m_characterAttachment || m_effectAttachment)
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_drilledActorId);
		if (pEntity)
		{
			IAttachmentManager* pAttachManager = pEntity->GetCharacter(0)->GetIAttachmentManager();
			pAttachManager->RemoveAttachmentByInterface(m_characterAttachment);
			pAttachManager->RemoveAttachmentByInterface(m_effectAttachment);
		}
	}
	
	if (!CheckAnyProjectileFlags(ePFlag_destroying))
	{
		Destroy();
	}
}

//------------------------------------------------------------------------
bool CTunnellingGrenade::Init(IGameObject *pGameObject)
{
	if (inherited::Init(pGameObject))
	{
		m_enterSurfaceAnim	= GetParam("enterSurfaceAnim", m_enterSurfaceAnim);
		m_launchAnim		= GetParam("launchAnim", m_launchAnim);
		m_drillActorEffect = GetParam("drillActorEffect", m_drillActorEffect);
		m_penetrationDepth	= GetParam("PenetrationDepth", m_penetrationDepth);
		m_failedExplosionScale = GetParam("failedExplosionScale", m_failedExplosionScale);
		
		if (m_launchAnim)
		{
			PlayAnimation(m_launchAnim, 0);
		}

		m_pSoundProxy = (IEntitySoundProxy*)GetEntity()->CreateProxy(ENTITY_PROXY_SOUND);
		if(m_pSoundProxy)
		{
			Vec3 zero = Vec3(0.0f, 0.0f, 0.0f);
			// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
			m_soundId = m_pSoundProxy->PlaySound("Sounds/crysiswars2:weapons:tunnel/tunnel_spindig", zero, zero, FLAG_SOUND_EVENT, eSoundSemantic_Weapon);
		}

		return true;
	}

	return false;
}

//------------------------------------------------------------------------
void CTunnellingGrenade::Update(SEntityUpdateContext &ctx, int updateSlot)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_GAME);

	inherited::Update(ctx, updateSlot);

	bool bDebug = g_pGameCVars->i_debug_projectiles > 0;

	if (bDebug)
	{
  		const char *MODE_NAME[] = { "Airbourne", "Penetrating", "PenetrateTunnelling", "Primed" };
		gEnv->pRenderer->DrawLabel(GetEntity()->GetWorldPos(), 10.0f, "%s", MODE_NAME[m_mode]);
	}
	
	switch(m_mode)
	{
		case Penetrating:
		{
			if (!(m_flags & Flags::HasPlayedPenetrateFX))
			{
				//--- Play Burrow down animation
				if (m_enterSurfaceAnim)
				{
					PlayAnimation(m_enterSurfaceAnim, 0, ePTIMER_DESCEND_TIME, 1.f);
				}
				else if (gEnv->bServer)
				{
					m_mode = PenetrateTunnelling;
				}
				m_flags |= Flags::HasPlayedPenetrateFX;
			}

			if(!m_drilledActorId)
			{
				Quat targetRot(Matrix33::CreateRotationVDir(m_groundDir));
				GetEntity()->SetRotation(targetRot);
				GetEntity()->SetPos(m_groundPos);
				m_entryEffect = m_entryEffect == eDrillingEffect_wait ? eDrillingEffect_request : m_entryEffect;
			}
		}
		break;

		case PenetrateTunnelling:
		{
			if(m_drilledActorId)
			{
				float distToTravel = 0.1f * ctx.fFrameTime;
				m_attachPos += (m_groundDir * distToTravel);
				m_drilledDist += distToTravel;
				m_characterAttachment->SetAttRelativeDefault(QuatT(m_attachRot, m_attachPos));

				if(m_drilledDist > 0.1f)
				{
					Explode(true, false, GetEntity()->GetWorldPos(), m_groundDir);
				}
			}
			else
			{
				CHANGED_NETWORK_STATE(this, ASPECT_TUNNELLER_POSITION);

				if (gEnv->bServer)
				{
					const static float PENETRATE_SPEED = 1.f;
					const static float EXIT_CLEARANCE  = 0.2f;

					m_groundPos += (m_groundDir * PENETRATE_SPEED * ctx.fFrameTime);
					Quat targetRot(Matrix33::CreateRotationVDir(m_groundDir));

					GetEntity()->SetPos(m_groundPos);
					GetEntity()->SetRotation(targetRot);

					if (bDebug)
					{
						static ColorB colourExitPt(RGBA8(0xff,0x00,0x00,0x70));
						static ColorB colourPt(RGBA8(0x00,0x00,0xff,0x70));
						static float SPHERE_RADIUS = 0.25f;
						gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( m_groundPos, SPHERE_RADIUS, colourPt);
						gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere( m_groundExitPt, SPHERE_RADIUS, colourExitPt);
						gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine( m_groundEntryPt, colourExitPt, m_groundEntryPt + (m_groundDir * -5.0f), colourPt);
					}

					if (m_groundDir.Dot(m_groundPos - m_groundEntryPt) > m_groundDir.Dot(m_groundExitPt - m_groundEntryPt))
					{
						if(m_flags & Flags::PenetrateFailed)
						{
							Explode(true, false, m_groundPos - (m_groundDir * 0.3f) , m_groundDir, ZERO, 0, m_failedExplosionScale);	
						}
						else
						{
							Explode(true, false, m_groundPos + (m_groundDir * 0.3f), -m_groundDir);
						}

					}
					else if (!(m_flags & Flags::PenetrateFailed) && m_exitEffect == eDrillingEffect_wait && ((m_groundPos - m_groundExitPt).GetLengthSquared() < EXIT_CLEARANCE))
					{
						m_exitEffect = eDrillingEffect_request;
						CHANGED_NETWORK_STATE(this, ASPECT_TUNNELLER_STATIC);
					}
				}	
			}
		}
		break;
	}

	if(m_entryEffect == eDrillingEffect_request)
	{
		m_entryEffect = eDrillingEffect_played;
		DoSurfaceEffect("tunnelgrenade_entry", m_groundEntryPt, m_groundDir * -SURFACE_EFFECT_PROBE_HEIGHT);
	}

	if(m_exitEffect == eDrillingEffect_request)
	{
		m_exitEffect = eDrillingEffect_played;
		DoSurfaceEffect("tunnelgrenade_exit", m_groundExitPt, m_groundDir * SURFACE_EFFECT_PROBE_HEIGHT);
	}	

	if (gEnv->bServer && m_lastReflectedMode != m_mode)
	{
		CHANGED_NETWORK_STATE(this, ASPECT_TUNNELLER_STATIC);
		m_lastReflectedMode = m_mode;
	}

	UpdateSound(ctx.fFrameTime);
}

void CTunnellingGrenade::UpdateSound(float dt)
{
	if(m_pSoundProxy && m_soundId != INVALID_SOUNDID)
	{
		const static float k_interpolateSpeed = 2.0f;
		switch(m_mode)
		{
		case Penetrating:
		case PenetrateTunnelling:
			{
				Interpolate(m_soundDig, 1.0f, k_interpolateSpeed, dt);
				break;
			}
		case Airbourne:
		case Primed:
			{
				Interpolate(m_soundDig, 0.0f, k_interpolateSpeed, dt);
				break;
			}
		}
		ISound* pSound = m_pSoundProxy->GetSound(m_soundId);
		if(pSound)
		{
			pSound->SetParam("spindig", m_soundDig);
		}
	}
}

bool CTunnellingGrenade::DoSurfaceEffect(const char *effectName, const Vec3 &pos, const Vec3 &offset) const
{	
	Vec3 fxProbeTop		= pos + offset;
	Vec3 fxProbeDepth	= -offset * 2.0f;
	IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
	ray_hit hit;

	if (pWorld->RayWorldIntersection(fxProbeTop, fxProbeDepth, DRILLER_PROBE_TYPES, DRILLER_PROBE_FLAGS, &hit, 1))
	{
		IMaterialEffects* pMaterialEffects = gEnv->pGame->GetIGameFramework()->GetIMaterialEffects();
		TMFXEffectId effectId = InvalidEffectId;
		int matID = pMaterialEffects->GetDefaultSurfaceIndex();
		effectId = pMaterialEffects->GetEffectIdByName("cw2_weaponImpacts", effectName);
		SMFXRunTimeEffectParams params;
		params.trgSurfaceId = hit.surface_idx;
		if (hit.pCollider && (hit.pCollider->GetiForeignData()==PHYS_FOREIGN_ID_STATIC))
		{
			params.trgRenderNode = (IRenderNode*)hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC);
		}

		params.angle = cry_atan2f(-m_groundDir.x, -m_groundDir.z);
		params.pos = hit.pt;
		params.decalPos = hit.pt;
		params.normal   = hit.n;
		params.dir[0]	= -hit.n;
		params.dir[1]	= hit.n;
		pMaterialEffects->ExecuteEffect(effectId, params);

		return true;
	}

	return false;
}

void CTunnellingGrenade::AnalyseSurfaces(const ray_hit *hits, int totalHits, const Vec3 &probeDir, TSurfaceList &surfaceList, bool finalPass)
{
	static float MIN_GAP_SIZE = 0.1f;
	
	if (totalHits)
	{
		float probeLength = probeDir.len() - 0.001f; //there is some rounding errors on surfaces which touch so take a little bit off here
		int count = 0;
		//--- Yuk, if there is no solid hit then index 0 is invalid & the count begins from 1
		int hitTot = (hits[0].dist < 0.0f) ? totalHits+1 : totalHits;

		for (int i = 0; i<hitTot; i++)
		{
			//--- Quick id reordering, the intention being to check all the hits before id 0 as that is hardcoded to be the non-piercable slot
			int hit = (i+1)%hitTot;

			if (hits[hit].dist >= 0.0f)
			{
				if (!CanPierceMaterialType(hits[i].surface_idx))
				{
					if(g_pGameCVars->i_debug_projectiles > 0)
					{
						g_pGame->GetIGameFramework()->GetIPersistantDebug()->Begin("DRILLER_CANTDRILL", false);
						g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(hits[hit].pt, 0.1f, ColorF(0.f,0.f,0.f,1.f), 30.f);
						g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddLine(hits[hit].pt, hits[hit].pt+ hits[hit].n, ColorF(0.f,0.f,0.f,1.f), 30.f);
					}

					surfaceList.clear();
					return;
				}
				
				if(g_pGameCVars->i_debug_projectiles > 0)
				{
					g_pGame->GetIGameFramework()->GetIPersistantDebug()->Begin("DRILLER_ANALYSE", false);
					g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(hits[hit].pt, 0.1f, ColorF(1.f,0.f,0.f,1.f), 30.f);
					g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddLine(hits[hit].pt, hits[hit].pt+ hits[hit].n, ColorF(1.f,0.f,0.f,1.f), 30.f);
				}

				bool in = probeDir.dot(hits[hit].n) <= 0.0f;
				if (in) 
				{
					if(!finalPass) //don't add surfaces in the final pass as forward facing hits will have already been found
					{
						//entered surface add to list
						Surface surf;
						surf.entryDist = hits[hit].dist;
						surf.entryPt = hits[hit].pt;
						surf.depth	 = -1.0f;
						surfaceList.push_back(surf);

						//CryLogAlways("entryDist %.5f", surf.entryDist);
					}
				}
				else
				{
					//exit surface find matching entry and close surface
					int numSurfaces = surfaceList.size();
					for (int i = numSurfaces-1; i >= 0; i--)
					{
						float dist = finalPass ? probeLength - hits[hit].dist : hits[hit].dist;

						//CryLogAlways("exitDist %.5f", dist);

						if(dist > surfaceList[i].entryDist)
						{
							float depth = dist - surfaceList[i].entryDist;
							surfaceList[i].depth = depth;
							surfaceList[i].exitPt = hits[hit].pt;
							break;
						}
					}
				}
			}
		}

		int numSurfaces = surfaceList.size();
						
		for (int i = numSurfaces-1; i > 0; i--)
		{
			//--- Check previous for appending these together
			
			bool openSurface = (surfaceList[i-1].depth <= 0);
			bool append = finalPass && openSurface;

			if (!openSurface)
			{
				float gap = surfaceList[i].entryDist - surfaceList[i-1].depth - surfaceList[i-1].entryDist;
				append = (gap < MIN_GAP_SIZE);
			}
				
			if (append)
			{
				//--- Tiny gap, add to the previous surface
				surfaceList[i-1].depth = surfaceList[i].depth > 0 ? surfaceList[i].entryDist - surfaceList[i-1].entryDist + surfaceList[i].depth : -1; 
				surfaceList[i-1].exitPt = surfaceList[i].exitPt;
			}
		}
	}
}
	

//-------------------------------------------------------------------------------
void CTunnellingGrenade::FullSerialize(TSerialize ser)
{
	inherited::FullSerialize(ser);

	ser.BeginGroup("tunneller");
	ser.Value("mode", m_mode, 'ui8');
	ser.Value("direction", m_groundDir, 'dir2');
	ser.EndGroup();
}

bool CTunnellingGrenade::NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags)
{
	if(aspect == ASPECT_TUNNELLER_POSITION)
	{
		ser.Value("position", static_cast<CTunnellingGrenade*>(this), &CTunnellingGrenade::GetCurrentPosition, &CTunnellingGrenade::SetCurrentPosition, 'wrld');
	}

	if (aspect == ASPECT_TUNNELLER_STATIC)
	{
		ser.BeginGroup("tunneller");
		ser.Value("mode", m_mode, 'ui8');
		ser.Value("direction", m_groundDir, 'dir2');
		ser.Value("entrypt", m_groundEntryPt, 'wrld');
		ser.Value("exitpt", m_groundExitPt, 'wrld');
		ser.Value("pos", m_attachPos, 'wrl3');
		ser.Value("rot", m_attachRot, 'ori1');
		ser.Value("joint", m_attachJoint, 'i16');
		ser.Value("drillActor", static_cast<CTunnellingGrenade*>(this), &CTunnellingGrenade::GetDrilledActor, &CTunnellingGrenade::SetDrilledActor, 'eid');
		ser.Value("playDrillOut", static_cast<CTunnellingGrenade*>(this), &CTunnellingGrenade::IsExiting, &CTunnellingGrenade::SetExiting, 'bool');
		ser.EndGroup();
	}
	return inherited::NetSerialize(ser, aspect, profile, flags);
}

bool CTunnellingGrenade::CanPierceMaterialType(int matID)
{
	IMaterialManager *mm = gEnv->p3DEngine->GetMaterialManager();
	ISurfaceType *surface = mm->GetSurfaceTypeManager()->GetSurfaceType(matID);

	//--- TODO! Have a property flagged in the surface rather than doing this string compare!
	if (surface && (strcmp(surface->GetType(), "metal") == 0)) 
	{
		return false;
	}

	return true;
}

void CTunnellingGrenade::ProcessEvent(SEntityEvent &event)
{
	switch(event.event)
	{
	case ENTITY_EVENT_TIMER:
		{
			switch(event.nParam[0])
			{
			case ePTIMER_DESCEND_TIME:
				if (gEnv->bServer)
				{
					if (m_mode == Penetrating)
					{
						m_mode = PenetrateTunnelling;
					}
				}
				break;
			}
		}
		break;
	}

	inherited::ProcessEvent(event);
}

void CTunnellingGrenade::HandleEvent(const SGameObjectEvent &event)
{
	if (CheckAnyProjectileFlags(ePFlag_destroying))
		return;

	CProjectile::HandleEvent(event);

	if (IsDemoPlayback())
		return;

	if (event.event == eGFE_OnCollision)
	{	
		SetAspectProfile(eEA_Physics, ePT_None);

		EventPhysCollision *pCollision = (EventPhysCollision *)event.ptr;
		bool bDebug = g_pGameCVars->i_debug_projectiles > 0;

		if (bDebug)
		{
			char *string_type[] = { "PE_NONE", "PE_STATIC", "PE_RIGID", "PE_WHEELEDVEHICLE", "PE_LIVING", "PE_PARTICLE", "PE_ARTICULATED", "PE_ROPE", "PE_SOFT", "PE_AREA" };
			CryLog("HIT! 0 = %s 1 = %s", string_type[pCollision->pEntity[0]->GetType()], string_type[pCollision->pEntity[1]->GetType()]);
		}

		if (m_mode == Airbourne)
		{
			// switch collision target id's to ensure target is not the driller itself
			int trgId = 1;
			int srcId = 0;
			IPhysicalEntity *pTarget = pCollision->pEntity[trgId];

			if (pTarget == GetEntity()->GetPhysics())
			{
				trgId = 0;
				srcId = 1;
				pTarget = pCollision->pEntity[trgId];
			}

			IEntity* pTargetEntity = gEnv->pEntitySystem->GetEntityFromPhysics(pTarget);

		//	CryLog("COLLIDED");

			
			m_groundDir = pCollision->vloc[srcId].GetNormalized();
			m_groundEntryPt = pCollision->pt;
			m_groundPos = pCollision->pt - (m_groundDir * 0.15f); //take off the length of the model
			m_groundExitPt  = m_groundPos + (m_groundDir * 0.1f);

			SetLifeTime(20000.f); //Give it time to drill through

			//--- Check for collision against the ground!
			if(pCollision->pEntity[trgId]->GetType() == PE_STATIC)
			{
				// CryLog("COLLIDED STATIC");
				//--- Test depth of surface, fire off a line hit from the max penetration dist back to us
				static float OFF_SURFACE_DIST  = 0.3f;

				if (bDebug)
				{
					IMaterialManager *mm = gEnv->p3DEngine->GetMaterialManager();
					ISurfaceType *surface = mm->GetSurfaceTypeManager()->GetSurfaceType(pCollision->idmat[trgId]);
					ISurfaceType *surface2 = mm->GetSurfaceTypeManager()->GetSurfaceType(pCollision->idmat[srcId]);
					if (surface) 
					{
						CryLogAlways("Surface Type: %s Name: %s", surface->GetType(), surface->GetName());
					}
					if (surface2)
					{
						CryLogAlways("Surface Type: %s Name: %s", surface2->GetType(), surface2->GetName());
					}
				}

				m_mode = Penetrating;
				m_flags |= Flags::PenetrateFailed;

				Vec3 probeFrom   = pCollision->pt + (m_groundDir * -OFF_SURFACE_DIST);
				Vec3 probeLength = m_groundDir * (m_penetrationDepth + OFF_SURFACE_DIST);
				
				if(g_pGameCVars->i_debug_projectiles > 0)
				{
					g_pGame->GetIGameFramework()->GetIPersistantDebug()->Begin("DRILLER_COLLISION", false);
					g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(probeFrom, 0.1f, ColorF(0.f,1.f,0.f,1.f), 60.f);
					g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddLine(probeFrom, probeFrom + probeLength, ColorF(0.f,1.f,0.f,1.f), 60.f);
				}

				//--- Physics probe to get target pos, point beneath the surface, just ahead of us
				IPhysicalWorld* pWorld = gEnv->pPhysicalWorld;
				const int MAX_HITS = 5;
				ray_hit hits[MAX_HITS];
				int flags   = DRILLER_PROBE_FLAGS&~(rwi_pierceability_mask|rwi_ignore_back_faces|rwi_ignore_solid_back_faces);
				int numHits = pWorld->RayWorldIntersection(probeFrom, probeLength, DRILLER_PROBE_TYPES, flags|rwi_separate_important_hits, hits, MAX_HITS);
				TSurfaceList surfaceList;
				AnalyseSurfaces(hits, numHits, probeLength, surfaceList, false);
			
				bool surfaceFound = (surfaceList.size() > 0);
				bool successful = surfaceFound && (surfaceList.front().depth > 0.0f);

				if (!successful && surfaceFound) //don't bother with the second linetest if no surfaces entered
				{
					if(g_pGameCVars->i_debug_projectiles > 0)
					{
						g_pGame->GetIGameFramework()->GetIPersistantDebug()->Begin("DRILLER_COLLISION", false);
						g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddSphere(probeFrom, 0.1f, ColorF(0.f,1.f,0.f,1.f), 60.f);
						g_pGame->GetIGameFramework()->GetIPersistantDebug()->AddLine(probeFrom, probeFrom + probeLength, ColorF(0.f,1.f,0.f,1.f), 60.f);
					}

					probeFrom += probeLength;
					
					numHits = pWorld->RayWorldIntersection(probeFrom, -probeLength, DRILLER_PROBE_TYPES, flags|rwi_separate_important_hits, hits, MAX_HITS);

					AnalyseSurfaces(hits, numHits, probeLength, surfaceList, true);

					successful = (surfaceList.front().depth > 0.0f);
				}

				if (successful)
				{
					m_groundExitPt  = surfaceList.front().exitPt + (m_groundDir * 0.15f);
					m_flags &= ~Flags::PenetrateFailed;
				}
			}
			else if (gEnv->bServer && pTargetEntity && g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(pTargetEntity->GetId()))
			{
				DrillActor(pTargetEntity, m_groundPos, pCollision->partid[trgId]);
				m_mode = Penetrating;
				CHANGED_NETWORK_STATE(this, ASPECT_TUNNELLER_STATIC);
			}
			else
			{
				Explode(true, false, pCollision->pt, -pCollision->n, ZERO, 0, m_failedExplosionScale);
			}
		}
	}
}

void CTunnellingGrenade::LoadGeometry()
{
	if (m_pAmmoParams && !m_pAmmoParams->fpGeometryName.empty())
	{
		//m_pAmmoParams->CacheGeometry(); //Ammo geometry is cached on load (the weapon that uses this ammo will take care)
		GetEntity()->LoadCharacter(0,m_pAmmoParams->fpGeometryName.c_str());
		GetEntity()->SetSlotLocalTM(0, m_pAmmoParams->fpLocalTM);
	}
}

void CTunnellingGrenade::PlayAnimation(const char *animName, int flags)
{
	//--- Play animation
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);
	if (pCharacter)
	{
		ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();
		if (pSkeletonAnim)
		{
			CryCharAnimationParams animationParams;
			animationParams.m_fTransTime = 0.1f;
			animationParams.m_nFlags = flags;

			pSkeletonAnim->StartAnimation(animName, animationParams);
		}
	}
}

void CTunnellingGrenade::PlayAnimation(const char *animName, int flags, int eventTimer, float atProp)
{
	ICharacterInstance *pCharacter = GetEntity()->GetCharacter(0);

	float duration=0.0f;
	if (pCharacter)
	{
		
		int animationId = pCharacter->GetIAnimationSet()->GetAnimIDByName(animName);
		if (animationId>=0)
		{
			duration = pCharacter->GetIAnimationSet()->GetDuration_sec(animationId);
		}
		
		PlayAnimation(animName, flags);
	}

	GetEntity()->SetTimer(eventTimer, (int)(duration*1000.0f*atProp));
}

void CTunnellingGrenade::AttachToCharacter(ICharacterInstance* pCharacter, const char* boneName)
{
	GetGameObject()->SetAspectProfile(eEA_Physics, ePT_None);

	char attachName[16] = "";		
	sprintf(attachName, "DRILL_%d", s_attachNameID++);

	m_characterAttachment = pCharacter->GetIAttachmentManager()->CreateAttachment(attachName, CA_BONE, boneName, false, false, true); 

	sprintf(attachName, "DRILL_%d", s_attachNameID++);

	m_effectAttachment = pCharacter->GetIAttachmentManager()->CreateAttachment(attachName, CA_BONE, boneName, false, false, true);
	if(m_characterAttachment)
	{
		m_characterAttachment->SetAttRelativeDefault(QuatT(m_attachRot, m_attachPos));

		CEntityAttachment *pEntityAttachment = new CEntityAttachment();
		pEntityAttachment->SetEntityId(GetEntityId());
	
		m_characterAttachment->AddBinding(pEntityAttachment);
	}
	if(m_effectAttachment)
	{
		m_effectAttachment->SetAttRelativeDefault(QuatT(m_attachRot, m_attachPos));
		
		CEffectAttachment *pEffectAttachment = new CEffectAttachment(m_drillActorEffect, Vec3(0.f,0.f,0.f), Vec3(0.f,1.f,0.f), 1.f);
		pEffectAttachment->CreateEffect(Matrix34A::CreateIdentity());

		m_effectAttachment->AddBinding(pEffectAttachment);
	}
}

void CTunnellingGrenade::DrillActor(IEntity* pEntity, Vec3& drillPos, int jointId)
{
	m_drilledActorId = pEntity->GetId();

	ICharacterInstance* pCharInstance = pEntity->GetCharacter(0);

	if (pCharInstance)	
	{			
		m_attachJoint = jointId;

		ISkeletonPose* skeleton = pCharInstance->GetISkeletonPose();
		const char* boneName = skeleton->GetJointNameByID(m_attachJoint);
		QuatT joint = skeleton->GetAbsJointByID(m_attachJoint);
		Matrix34 invTM = pEntity->GetWorldTM() * Matrix34(joint);

		invTM.Invert();
		m_attachPos = invTM * drillPos;

		invTM.SetTranslation(Vec3(0.f,0.f,0.f));

		m_groundDir = invTM * m_groundDir;
		m_attachRot = Quat(invTM) * GetEntity()->GetRotation();

		AttachToCharacter(pCharInstance, boneName);
	}
}

void CTunnellingGrenade::SetDrilledActor(EntityId actorId)
{
	IEntity* pTargetEntity = actorId ? gEnv->pEntitySystem->GetEntity(actorId) : NULL;
	ICharacterInstance* pCharacter = pTargetEntity ? pTargetEntity->GetCharacter(0) : NULL;

	if (gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(actorId) && pCharacter)
	{
		m_drilledActorId = actorId;
		const char* boneName = pCharacter->GetISkeletonPose()->GetJointNameByID(m_attachJoint);
		AttachToCharacter(pCharacter, boneName);			
	}
}