/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 17:1:2006   11:18 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "TracerManager.h"
#include "Game.h"
#include "GameCVars.h"
#include "Actor.h"
#include <I3DEngine.h>
#include "RecordingSystem.h"


#define TRACER_GEOM_SLOT  0
#define TRACER_FX_SLOT    1
//------------------------------------------------------------------------
CTracer::CTracer(const Vec3 &pos)
: m_pos(0,0,0),
	m_dest(0,0,0),
	m_startingpos(pos),
	m_age(0.0f),
	m_lifeTime(1.5f),
	m_entityId(0),
	m_geometrySlot(0),
	m_startFadeOutTime(0.0f),
	m_scale(1.0f),
	m_geometryOpacity(0.99f),
	m_tracerFlags(0)
{
	CreateEntity();
}

//------------------------------------------------------------------------
CTracer::CTracer()
: m_pos(0,0,0),
	m_dest(0,0,0),
	m_startingpos(0,0,0),
	m_age(0.0f),
	m_lifeTime(1.5f),
	m_entityId(0),
	m_geometrySlot(0),
	m_startFadeOutTime(0.0f),
	m_scale(1.0f),
	m_geometryOpacity(0.99f),
	m_tracerFlags(0)
{
}

//------------------------------------------------------------------------
CTracer::~CTracer()
{
	//DeleteEntity();
}

//------------------------------------------------------------------------
void CTracer::Reset(const Vec3 &pos)
{
	m_pos.zero();
	m_dest.zero();
	m_startingpos=pos;
	m_age=0.0f;
	m_lifeTime=1.5f;
	m_tracerFlags &= ~kTracerFlag_scaleToDistance;
	m_startFadeOutTime = 0.0f;

	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		pEntity->FreeSlot(TRACER_GEOM_SLOT);
		pEntity->FreeSlot(TRACER_FX_SLOT);
	}
}

//------------------------------------------------------------------------
void CTracer::CreateEntity()
{
	if (!m_entityId)
	{
		SEntitySpawnParams spawnParams;
		spawnParams.pClass = gEnv->pEntitySystem->GetClassRegistry()->GetDefaultClass();
		spawnParams.sName = "_tracer";
		spawnParams.nFlags = ENTITY_FLAG_NO_PROXIMITY | ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_NO_SAVE;

		if (IEntity *pEntity=gEnv->pEntitySystem->SpawnEntity(spawnParams))
			m_entityId=pEntity->GetId();
	}
}

void CTracer::DeleteEntity()
{
	if (m_entityId)
	{
		gEnv->pEntitySystem->RemoveEntity(m_entityId);
	}

	m_entityId=0;
}

//------------------------------------------------------------------------
void CTracer::GetMemoryStatistics(ICrySizer * s) const
{
	s->Add(*this);
}

//------------------------------------------------------------------------
void CTracer::SetGeometry(const char *name, float scale)
{
	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		m_geometrySlot =pEntity->LoadGeometry(TRACER_GEOM_SLOT, name);

		if (scale!=1.0f)
		{
			Matrix34 tm=Matrix34::CreateIdentity();
			tm.Scale(Vec3(scale, scale, scale));
			pEntity->SetSlotLocalTM(m_geometrySlot, tm);
		}

		// Set opacity
		IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
		pRenderProxy->SetOpacity(m_geometryOpacity);
	}
}

//------------------------------------------------------------------------
void CTracer::SetEffect(const char *name, float scale)
{
	IParticleEffect *pEffect = gEnv->pParticleManager->FindEffect(name);
	if (!pEffect)
		return;

	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		m_effectSlot = pEntity->LoadParticleEmitter(TRACER_FX_SLOT, pEffect,0,true);
		if (scale!=1.0f)
		{
			Matrix34 tm=Matrix34::CreateIdentity();
			tm.Scale(Vec3(scale, scale, scale));
			pEntity->SetSlotLocalTM(m_effectSlot, tm);
		}
	}
}

//------------------------------------------------------------------------
bool CTracer::Update(float frameTime, const Vec3 &camera)
{
	frameTime							= (float)__fsel(-m_age, 0.002f, frameTime);
	const float tracerAge	= (float)__fsel(-m_age, 0.002f, m_age+frameTime);

	m_age = tracerAge;

	if (tracerAge >= m_lifeTime)
		return false;

	Vec3 end(m_dest);

	Vec3 dp = end-m_pos;

	float len2 = dp.len2();

	if (len2 <= 0.25f)
		return false;
	
	float dist = sqrt_tpl(len2);
	Vec3 dir = dp/dist;

	float sqrRadius = g_pGameCVars->tracer_player_radiusSqr;

	float cameraDistance = (m_pos-camera).len2();
	float speed = m_speed;

	//Slow down tracer when near the player
	speed *= (float)__fsel(sqrRadius - cameraDistance, 0.35f + (cameraDistance/(sqrRadius*2)), 1.0f);
	
	Vec3 newPos = m_pos + dir * min(speed*frameTime, dist);
	m_pos = newPos;
	
	if((newPos-m_dest).len2()<0.25f)
		return false;

	// Now update visuals...
	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(m_entityId))
	{
		Matrix34 tm(Matrix33::CreateRotationVDir(dir));
		tm.AddTranslation(newPos);

		pEntity->SetWorldTM(tm);

		//Do not scale effects
		if(m_tracerFlags & kTracerFlag_useGeometry)
		{
			float minDistance = g_pGameCVars->tracer_min_distance;
			float maxDistance = g_pGameCVars->tracer_max_distance;
			float minScale = g_pGameCVars->tracer_min_scale;
			float maxScale = g_pGameCVars->tracer_max_scale;
			float scale;

			if (cameraDistance <= minDistance * minDistance)
				scale = minScale;
			else if (cameraDistance >= maxDistance * maxDistance)
				scale = maxScale;
			else
			{
				float t = (sqrtf(cameraDistance)-minDistance)/(maxDistance-minDistance);
				scale = minScale+t*(maxScale-minScale);
			}

			if(m_tracerFlags & kTracerFlag_scaleToDistance)
			{
				scale = dist * 0.5f;
			}

			tm.SetIdentity();
			tm.SetScale(Vec3(m_scale,scale,m_scale));
			pEntity->SetSlotLocalTM(m_geometrySlot,tm);
		}
	}

	return true;
}

//------------------------------------------------------------------------
void CTracer::SetLifeTime(float lifeTime)
{
	m_lifeTime = lifeTime;
}

//------------------------------------------------------------------------
CTracerManager::CTracerManager()
: m_numActiveTracers(0)
{
}

//------------------------------------------------------------------------
CTracerManager::~CTracerManager()
{
}

//------------------------------------------------------------------------
void CTracerManager::EmitTracer(const STracerParams &params)
{
	if(!gEnv->bClient)
		return;

	CRecordingSystem* pRecordingSystem = g_pGame->GetRecordingSystem();
	if (pRecordingSystem)
	{
		if (!pRecordingSystem->OnEmitTracer(params))
		{
			return;
		}
	}

	int idx=0;

	if(m_numActiveTracers < kMaxNumTracers)
	{
		//Add to end of array
		idx = m_numActiveTracers;
		m_tracerPool[idx].CreateEntity();
		m_tracerPool[idx].Reset(params.position);
		m_numActiveTracers++;
	}
	else
	{
		//Find the oldest existing tracer and remove
		assert(!"Too many tracers in existence! Removing oldest");

		float oldest = 0.0f;

		for(int i = 0; i < kMaxNumTracers; i++)
		{
			CryPrefetch(&m_tracerPool[i+4]);
			if(m_tracerPool[i].m_age > oldest)
			{
				oldest = m_tracerPool[i].m_age;
				idx = i;
			}
		}
	}

	CTracer& tracer = m_tracerPool[idx];

	tracer.m_tracerFlags = kTracerFlag_active;
	tracer.m_effectSlot = -1;
	tracer.m_geometryOpacity = params.geometryOpacity;
	if (params.geometry && params.geometry[0])
	{
		tracer.SetGeometry(params.geometry, params.scale);
		tracer.m_tracerFlags |= kTracerFlag_useGeometry;
	}
	if (params.effect && params.effect[0])
		tracer.SetEffect(params.effect, params.scale);

	tracer.SetLifeTime(params.lifetime);

	tracer.m_speed = params.speed;
	tracer.m_pos = params.position;
	tracer.m_dest = params.destination;
	tracer.m_fadeOutTime = params.delayBeforeDestroy;
	
	if(params.scaleToDistance)
	{
		tracer.m_tracerFlags |= kTracerFlag_scaleToDistance;
	}
	
	tracer.m_startFadeOutTime = params.startFadeOutTime;
	tracer.m_scale = params.scale;

	if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(tracer.m_entityId))
		pEntity->Hide(0);
}

//------------------------------------------------------------------------
void CTracerManager::Update(float frameTime)
{
	CryPrefetch(m_tracerPool);
	Vec3 cameraPosition = gEnv->pSystem->GetViewCamera().GetPosition();

	const int kNumActiveTracers = m_numActiveTracers;

	for(int i = 0; i < kNumActiveTracers;)
	{
		int nextTracerIndex = i + 1;
		CTracer& tracer = m_tracerPool[i];
		CryPrefetch(&m_tracerPool[nextTracerIndex]);
		
		//Update
		bool stillMoving = tracer.Update(frameTime, cameraPosition);

		//Is it worth putting this in another loop for instruction cache coherency? Profile when PS3 is up and running - Rich S
		if(stillMoving || (tracer.m_fadeOutTime > 0.f))
		{
			tracer.m_tracerFlags |= kTracerFlag_active;
			
			if (!stillMoving)
			{
				tracer.m_fadeOutTime -= frameTime;

				if (tracer.m_effectSlot > -1)
				{
					if (IEntity *pEntity = gEnv->pEntitySystem->GetEntity(tracer.m_entityId))
					{
						IParticleEmitter * emitter = pEntity->GetParticleEmitter(tracer.m_effectSlot);
						if (emitter)
						{
							emitter->Activate(false);
						}

						if (tracer.m_tracerFlags & kTracerFlag_useGeometry)
						{
							pEntity->SetSlotFlags(tracer.m_geometrySlot, pEntity->GetSlotFlags(tracer.m_geometrySlot) &~ (ENTITY_SLOT_RENDER|ENTITY_SLOT_RENDER_NEAREST));
						}

						tracer.m_effectSlot = -1;
					}
				}

				if(tracer.m_tracerFlags & kTracerFlag_useGeometry)
				{
					// Fade out geometry
					if((tracer.m_fadeOutTime < tracer.m_startFadeOutTime) && (tracer.m_startFadeOutTime > 0.0f))
					{
						if (IEntity *pEntity = gEnv->pEntitySystem->GetEntity(tracer.m_entityId))
						{
							IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pEntity->GetProxy(ENTITY_PROXY_RENDER);
							pRenderProxy->SetOpacity(max((tracer.m_fadeOutTime / tracer.m_startFadeOutTime) * tracer.m_geometryOpacity, 0.0f));
						}
					}
				}
			}
		}
		else
		{
			tracer.m_tracerFlags &= ~kTracerFlag_active;
		}

		i = nextTracerIndex;
	}

	//This is where we clear out the inactive tracers, so the counter isn't const
	int numActiveTracers = kNumActiveTracers;
	
	for(int i = kNumActiveTracers - 1; i >= 0;)
	{
		int nextIndex = i - 1;
		CTracer& tracer = m_tracerPool[i];
		CryPrefetch(&m_tracerPool[nextIndex]);
		
		if(!(tracer.m_tracerFlags & kTracerFlag_active))
		{
			if (IEntity *pEntity=gEnv->pEntitySystem->GetEntity(tracer.m_entityId))
			{
				pEntity->Hide(true);
				pEntity->SetWorldTM(Matrix34::CreateIdentity());
			}

			//Switch the inactive tracer so it's at the end of the array;
			const int lastTracer = numActiveTracers - 1;
			static CTracer temp;
			temp = tracer;			
			numActiveTracers = lastTracer;
			tracer = m_tracerPool[lastTracer];
			m_tracerPool[lastTracer] = temp;
		}

		i = nextIndex;
	}

	m_numActiveTracers = numActiveTracers;
}

//------------------------------------------------------------------------
void CTracerManager::Reset()
{
	for(int i = 0; i < kMaxNumTracers; i++)
	{
		m_tracerPool[i].DeleteEntity();
	}

	m_numActiveTracers = 0;	
}

void CTracerManager::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "TracerManager");
	//s->Add(*this);
	//s->AddObject(m_tracerPool);

	for (size_t i=0; i<m_numActiveTracers; i++)
		m_tracerPool[i].GetMemoryStatistics(s);
}